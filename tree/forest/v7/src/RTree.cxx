/// \file RTree.cxx
/// \ingroup Forest ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2018-10-04
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

#include "ROOT/RTreeModel.hxx"
#include "ROOT/RPageStorage.hxx"

#include <utility>

ROOT::Experimental::Detail::RTree::RTree(std::shared_ptr<ROOT::Experimental::RTreeModel> model)
   : fModel(model)
   , fNentries(0)
{
}

ROOT::Experimental::Detail::RTree::~RTree()
{
}


ROOT::Experimental::RInputTree::RInputTree(
   std::shared_ptr<ROOT::Experimental::RTreeModel> model,
   std::unique_ptr<ROOT::Experimental::Detail::RPageSource> source)
   : ROOT::Experimental::Detail::RTree(model)
   , fSource(std::move(source))
{
}


ROOT::Experimental::RInputTree::RInputTree(std::unique_ptr<ROOT::Experimental::Detail::RPageSource> source)
   : ROOT::Experimental::Detail::RTree(std::make_shared<ROOT::Experimental::RTreeModel>())
   , fSource(std::move(source))
{
}


ROOT::Experimental::RInputTree::~RInputTree()
{
}


ROOT::Experimental::ROutputTree::ROutputTree(
   std::shared_ptr<ROOT::Experimental::RTreeModel> model,
   std::unique_ptr<ROOT::Experimental::Detail::RPageSink> sink)
   : ROOT::Experimental::Detail::RTree(model)
   , fSink(std::move(sink))
{
}

ROOT::Experimental::ROutputTree::~ROutputTree()
{
}