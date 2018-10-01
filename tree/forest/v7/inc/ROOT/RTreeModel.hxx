/// \file ROOT/RTreeModel.hxx
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

#ifndef ROOT7_RTreeModel
#define ROOT7_RTreeModel

#include <ROOT/RBranch.hxx>
#include <ROOT/RStringView.hxx>
#include <ROOT/RTreeEntry.hxx>

#include <atomic>
#include <cassert>
#include <memory>
#include <string_view>
#include <vector>
#include <utility>

namespace ROOT {
namespace Experimental {

class RTreeModel {
   friend class RTree;

   using ModelId = unsigned;

   static std::atomic<ModelId> gModelId;

   ModelId fModelId;
   RTreeEntry fDefaultEntry;
   RBranch<RBranchCollectionTag> fRootBranch;

public:
   RTreeModel() : fModelId(0), fDefaultEntry(this) { }

   template <typename T, typename... ArgsT>
   std::shared_ptr<T> Branch(std::string_view name, ArgsT&&... args) {
     assert(!IsFrozen());

     RBranch<T> *branch = new RBranch<T>(name);
     fRootBranch.Attach(branch);

     return fDefaultEntry.AddCargo<T>(branch, std::forward<ArgsT>(args)...);
   }

   void BranchDynamic(std::string_view name,
                      std::string_view type,
                      void* address)
   {
     if (type == "float") {
       RBranch<float> *branch = new RBranch<float>(name);
       fRootBranch.Attach(branch);
       fDefaultEntry.AddCargoCaptured<float>(
         branch, reinterpret_cast<float *>(address));
     } else {
       assert(false);
     }
   }

  std::shared_ptr<RCargoSubtree> BranchCollection(
    std::string_view name,
    std::shared_ptr<RTreeModel> model)
  {
    assert(!model->IsFrozen());
    for (auto branch : model->fRootBranch) {
      branch->PrependName(name);
    }
    model->fRootBranch.MakeSubBranch(name);
    model->Freeze();

    fRootBranch.Attach(&model->fRootBranch);

    auto cargo =
      std::make_shared<RCargoSubtree>(&model->fRootBranch,
                                     model->fDefaultEntry.fCargo);
    fDefaultEntry.AddCargoSubtree(cargo);
    return cargo;
  }

   // Model can be cloned and as long as it stays frozen the model id
   // is the same
   void Freeze() { if (fModelId == 0) fModelId = ++gModelId; }
   bool IsFrozen() { return fModelId > 0; }
   ModelId GetModelId() { return fModelId; }

   // Remove me
   RTreeEntry* GetDefaultEntry() { return &fDefaultEntry; }

   RBranch<RBranchCollectionTag>& GetRootBranch() { return fRootBranch; }
};

} // namespace Exerimental
} // namespace ROOT

#endif
