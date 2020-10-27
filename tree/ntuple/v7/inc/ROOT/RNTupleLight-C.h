/// \file ROOT/RNTupleLight-C.h
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2020-10-27
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2020, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_RNTupleLightC
#define ROOT7_RNTupleLightC

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Discussion on
//  - Naming convention: ROOT_ntpl_... ?
//  - No guarantees on backwards compatibility
//  - API versioning
//  - Error handling: NULL pointers and ROOT_ntpl_error(ntpl)? What about open()?
//  - API Completeness for a first usable version

#define ROOT_NTPL_ID uint64_t
#define ROOT_NTPL_SIZE uint64_t

// Column types
#define ROOT_NTPL_TYPE_OFFSET      1
#define ROOT_NTPL_TYPE_FLOAT       2
#define ROOT_NTPL_TYPE_DOUBLE      3
// ...

// Error codes
#define ROOT_NTPL_ERR_INVALID_ID   0
#define ROOT_NTPL_ERR_UNKOWN       1
// ...


struct ROOT_rawfile;
struct ROOT_ntpl;

struct ROOT_ntpl_column {
  ROOT_ntpl_column_list *next;
  ROOT_NTPL_ID id;
  int type;
};

struct ROOT_ntpl_field {
  ROOT_ntpl_field_list *next;
  ROOT_NTPL_ID id;
  char *name;
  char *type;
  ROOT_ntpl_field_list *parent;
  ROOT_ntpl_column_list *columns;
};

struct ROOT_ntpl_cluster {
  ROOT_ntpl_cluster_list *next;
  ROOT_NTPL_ID id;
  ROOT_NTPL_SIZE first_entry;
  ROOT_NTPL_SIZE nentries;
};

struct ROOT_ntpl_page {
  ROOT_ntpl_page_list *next;
  ROOT_NTPL_ID id;
  ROOT_NTPL_SIZE first_element;
  ROOT_NTPL_SIZE nelements;
};

struct ROOT_ntpl_page_buffer {
  void *buffer;
  ROOT_NTPL_SIZE first_element;
  ROOT_NTPL_SIZE nelements;
};


ROOT_rawfile *ROOT_rawfile_open(const char *locator);
int ROOT_rawfile_error(ROOT_rawfile *rawfile);
void ROOT_rawfile_close(ROOT_rawfile *rawfile);


// ROOT_ntpl * and derived objects are thread friendly. They can be used from multiple threads but
// not concurrently.  Multiple ROOT_ntpl * objects can be opened for the same RNTuple.
ROOT_ntpl *ROOT_ntpl_open(ROOT_rawfile *rawfile, const char *path);
int ROOT_ntpl_error(ROOT_ntpl *ntpl);
void ROOT_ntpl_close(ROOT_ntpl *ntpl);

// Meta-data
ROOT_ntpl_field *ROOT_ntpl_list_fields(ROOT_ntpl *ntpl);
ROOT_ntpl_cluster *ROOT_ntpl_list_clusters(ROOT_ntpl *ntpl);
ROOT_ntpl_page *ROOT_ntpl_list_pages(ROOT_NTPL_ID column_id, ROOT_NTPL_ID cluster_id);
void ROOT_ntpl_list_free(void *list);

ROOT_ntpl_page_buffer ROOT_ntpl_page_get(ROOT_NTPL_ID column_id, ROOT_NTPL_ID cluster_id, ROOT_NTPL_ID page_id);
ROOT_ntpl_page_buffer ROOT_ntpl_page_find(ROOT_NTPL_ID column_id, ROOT_NTPL_ID cluster_id,
                                          ROOT_NTPL_SIZE element_index);
// Releases (internal) resources acquired by ROOT_ntpl_page_get()
void ROOT_ntpl_page_release(ROOT_ntpl_page_buffer buffer);


////// Usage sketch

ROOT_rawfile *rawfile = ROOT_rawfile_open("data.root");
ROOT_ntpl *ntpl = ROOT_ntpl_open("MyNtuple", rawfile);

// Collect column ids
ROOT_ntpl_field *fields = ROOT_ntpl_list_fields(ntpl);
ROOT_NTPL_ID column_pt;
ROOT_NTPL_ID column_jets;
ROOT_NTPL_ID column_jet_E;
while (fields)
  if (!fields->parent && strcmp(fields->name, "pt") == 0) {
    column_pt = fields->columns->id;
  } else if (!fields->parent && strcmp(fields->name, "jets") == 0) {
    column_jets = fields->columns->id;
  } else if (fields->parent && strcmp(fields->parent->name, "jets") == 0 &&
             strcmp(fields->name, "jets") == 0)
  {
    column_jet_E = fields->columns->id;
  }
  fields = fields->next;
};
ROOT_ntpl_list_free(fields);

// Iterate through the data set
ROOT_ntpl_cluster *clusters = ROOT_ntpl_list_clusters(ntpl);
while (clusters) {
  ROOT_ntpl_page *pages_pt = ROOT_ntpl_list_pages(column_pt, clusters->id);
  while (pages_pt) {
    ROOT_ntpl_page_buffer buffer_pt = ROOT_ntpl_page_get(column_pt, clusters->id, pages_pt->id);
    float *pt = (float *)buffer_pt.buffer;
    for (unsigned i = 0; i < pages_pt->nelements; ++i) {
      //...
    }
    ROOT_ntpl_page_release(buffer_pt);
    pages_pt = pages_pt->next;
  }
  ROOT_ntpl_list_free(pages_pt);

  ROOT_ntpl_page *pages_jets = ROOT_ntpl_list_pages(column_jets, clusters->id);
  while (pages_jets) {
    ROOT_ntpl_page_buffer buffer_jets = ROOT_ntpl_page_get(column_jets, clusters->id, pages_jets->id);
    ROOT_ntpl_page_buffer buffer_jet_E = ROOT_ntpl_page_find(column_jets_e, clusters->id, 0);
    std::uint32_t *offsets = (std::uint32_t *)buffer_jets.buffer;
    for (unsigned i = 0; i < pages_jets->nelements; ++i) {
      printf("jet vector size: %u\n", i ? (offsets[i] - offsets[i-1]) : offsets[0]);

      // Note: this accesses only the first element of jets.E, subsequent elements might be
      // in the following pages
      if (offsets[i] < buffer_jet_E.first_element + buffer_jet_E.nelements) {
        // in current page
      } else {
        ROOT_ntpl_page_release(buffer_jet_E);
        buffer_jet_E = ROOT_ntpl_page_find(column_jets_e, clusters->id, offsets[i]);
      }
      // Use ROOT_ntpl_page_find to get the page of jets.E that corresponds to the offset
    }
    ROOT_ntpl_page_release(buffer_jet_E);
    ROOT_ntpl_page_release(buffer_jets);
    pages_jets = pages_jets->next;
  }
  ROOT_ntpl_list_free(pages_jets);

  clusters = clusters->next;
}
ROOT_ntpl_list_free(clusters);

ROOT_ntpl_close(ntpl);
ROOT_rawfile_close(rawfile);

//////


#ifdef __cplusplus
} // extern "C"
#endif

#endif // ROOT7_RNTupleLightC
