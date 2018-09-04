/// \file RTree.cxx
/// \ingroup Forest ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2018-07-19
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2015, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "ROOT/RTree.hxx"

#include "ROOT/RColumnRange.hxx"
#include "ROOT/RColumnStorage.hxx"
#include "ROOT/RTreeEntry.hxx"
#include "ROOT/RTreeModel.hxx"

#include <cassert>
#include <iterator>
#include <iostream>
#include <utility>


ROOT::Experimental::RColumnRange ROOT::Experimental::RTree::GetEntryRange(ERangeType type, RTreeEntry *entry) {
  return RColumnRange(0, fNentries);
}

ROOT::Experimental::RTree::RTree(
   std::shared_ptr<RTreeModel> model,
   std::unique_ptr<RColumnSink> sink)
   : fSink(std::move(sink))
   , fModel(model)
   , fNentries(0)
   , fClusterSizeEntries(kDefaultClusterSizeEntries)
   , fIsCommitted(false)
{
  fModel->Freeze();

  for (auto branch : fModel->fRootBranch) {
    // Todo: column parent-children relationship
    branch->GenerateColumns(nullptr, fSink.get());
    std::cout << "Registering " << branch->GetName() << std::endl;
  }

  fSink->OnCreate("Forest" /* TODO */);
}


ROOT::Experimental::RTree::RTree(
  std::shared_ptr<RTreeModel> model,
  std::unique_ptr<RColumnSource> source)
  : fSource(std::move(source))
  , fModel(model)
{
  fModel->Freeze();
  std::cout << "CREATING TREE FOR READING" << std::endl;
  fSource->Attach("Forest" /* TODO */);
  fNentries = fSource->GetNentries();
}


ROOT::Experimental::RTree::~RTree() {
  Commit();
}


void ROOT::Experimental::RTree::FlushBranches() {
  for (auto branch : fModel->fRootBranch) {
    branch->Flush();
  }
}


void ROOT::Experimental::RTree::Commit() {
  //std::cout << "FLUSHING ALL COLUMNS" << std::endl;
  if (fIsCommitted) return;

  FlushBranches();
  if (fSink) fSink->OnCommitDataset(fNentries);
  fIsCommitted = true;
}


void ROOT::Experimental::RTree::MakeCluster() {
   FlushBranches();
   fSink->OnCommitCluster(fNentries);
}


void ROOT::Experimental::RTree::FillV(RTreeEntry **entry, unsigned size) {
  for (unsigned i = 0; i < size; ++i) {
    assert(entry[i]);
    assert(entry[i]->IsCompatibleWith(fModel.get()));

    Fill(entry[i]);
  }
  fNentries += size;
}


//void ROOT::Experimental::RTree::Fill(RTreeEntry *entry) {
//  //assert(entry);
//  //assert(entry->IsCompatibleWith(fModel.get()));
//
//  for (auto&& ptr_cargo : entry->GetCargoRefs()) {
//    //std::cout << "Filling " << ptr_cargo->GetBranch()->GetName() << std::endl;
//    ptr_cargo->GetBranch()->Append(ptr_cargo.get());
//  }
//  fNentries++;
//}
