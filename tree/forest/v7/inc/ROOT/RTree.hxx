/// \file ROOT/RTree.hxx
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

#ifndef ROOT7_RTree
#define ROOT7_RTree

#include <ROOT/RColumn.hxx>
#include <ROOT/RColumnUtil.hxx>
#include <ROOT/RStringView.hxx>
#include <ROOT/RTreeModel.hxx>
#include <ROOT/RTreeView.hxx>

#include <memory>
#include <string_view>
#include <vector>

namespace ROOT {
namespace Experimental {

class RColumnRange;
class RColumnSink;
class RColumnSource;
class RTree;
class RTreeEntry;

// TODO: ROTree and RITree

enum class ERangeType {
  kLazy,
  kSync,
};

class RTree {
   std::unique_ptr<RColumnSink> fSink;
   std::unique_ptr<RColumnSource> fSource;
   std::shared_ptr<RTreeModel> fModel;
   RColumnCollection fColumns;

   unsigned fNentries;

public:
  RTree(std::shared_ptr<RTreeModel> model,
        std::unique_ptr<RColumnSink> sink);
  RTree(std::shared_ptr<RTreeModel> model,
        std::unique_ptr<RColumnSource> source);
  ~RTree();

  unsigned GetNentries() const { return fNentries; }
  RColumnRange GetEntryRange(ERangeType type, RTreeEntry* entry = nullptr);

  template <typename T>
  RTreeView<T> GetView(std::string_view name) {
    auto branch = new RBranch<T>(name);  // TODO not with raw pointer
    branch->GenerateColumns(fSource.get(), nullptr);
    return RTreeView<T>(branch);
  }

  RTreeViewCollection GetViewCollection(std::string_view name) {
    auto branch = new RBranch<OffsetColumn_t>(name);
    branch->GenerateColumns(fSource.get(), nullptr);
    return RTreeViewCollection(branch, fSource.get());
  }

  void Fill() { Fill(&(fModel->fDefaultEntry)); }
  void Fill(RTreeEntry *entry);
  void FillV(RTreeEntry **entry, unsigned size);
}; // RTree

} // namespace Experimental
} // namespace ROOT

#endif
