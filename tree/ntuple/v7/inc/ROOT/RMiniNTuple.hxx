/// \file ROOT/RMiniNTuple.hxx
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2020-05-26
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2020, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_RMiniNTuple
#define ROOT7_RMiniNTuple

#ifdef __cplusplus
extern "C" {
#endif

// Discussion on
//  - Naming convention: ROOT_ntpl_... ?
//  - API versioning
//  - Error handling: NULL pointers and ROOT_ntpl_error(ntpl)? What about open()?
//  - API Completeness for a first usable version

struct ROOT_ntpl;
struct ROOT_ntpl_view;

struct ROOT_ntpl_list {
  struct ROOT_ntpl_field *next;
};

struct ROOT_ntpl_field {
  ROOT_ntpl_list list;
  char *name;
  char *parent;
  char *type;
};

struct ROOT_ntpl_cluster {
  ROOT_ntpl_list list;
  int first_entry;
  int nentries;
};

#define ROOT_NTPL_COLLECTION  0
#define ROOT_NTPL_FLOAT       1
#define ROOT_NTPL_DOUBLE      2

// ROOT_ntpl * and derived objects are thread friendly. They can be used from multiple threads but
// not concurrently.  Multiple ROOT_ntpl * objects can be opened for the same RNTuple.
ROOT_ntpl *ROOT_ntpl_open(const char *ntpl_name, const char *path);
int ROOT_ntpl_error(ROOT_ntpl *ntpl);
void ROOT_ntpl_close(ROOT_ntpl *ntpl);

// Meta-data
ROOT_ntpl_field *ROOT_ntpl_list_fields(ROOT_ntpl *ntpl);
ROOT_ntpl_cluster *ROOT_ntpl_list_clusters(ROOT_ntpl *ntpl);
void ROOT_ntpl_list_free(ROOT_ntpl_list *list);

// Iteration
bool ROOT_ntpl_entry_next(ROOT_ntpl *ntpl);
void ROOT_ntpl_entry_rewind(ROOT_ntpl *ntpl);
bool ROOT_ntpl_collection_next(ROOT_ntpl_view *view);
void ROOT_ntpl_collection_rewind(ROOT_ntpl_view *view);

// Runtime type check
ROOT_ntpl_view *ROOT_ntpl_view_get(void *ntpl_or_view, const char *field, int type);
void ROOT_ntpl_view_free(ROOT_ntpl_view *view);

float ROOT_ntpl_float(ROOT_ntpl_view *view);
double ROOT_ntpl_double(ROOT_ntpl_view *view);
int ROOT_ntpl_int(ROOT_ntpl_view *view);

int ROOT_ntpl_size(void *ntpl_or_view);

////// Usage

ROOT_ntpl *ntpl = ROOT_ntpl_open("MyNtuple", "data.root");

ROOT_ntpl_view *pt           = ROOT_ntpl_view(ntpl,   "pt",     ROOT_NTPL_FLOAT);
ROOT_ntpl_view *tracks       = ROOT_ntpl_view(ntpl,   "tracks", ROOT_NTPL_COLLECTION);
// accesses tracks.E
ROOT_ntpl_view *track_energy = ROOT_ntpl_view(tracks, "E",      ROOT_NTPL_FLOAT);

printf("%d entries\n"), ROOT_ntpl_size(ntpl));

while (ROOT_ntpl_entry_next(ntpl)) {
  printf("pt: %f\n", ROOT_ntpl_float(pt));

  printf("%d tracks\n", ROOT_ntpl_size(tracks));
  int i = 0;
  while (ROOT_ntpl_collection_next(tracks)) {
    printf("E(track %d): %f\n", i, ROOT_ntpl_float(track_energy));
    i++;
  }
}

ROOT_ntpl_close(ntpl);

//////


#ifdef __cplusplus
} // extern "C"
#endif

#endif // ROOT7_RMiniNTuple
