/// \file ROOT/RTreeEntry.hxx
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

#ifndef ROOT7_RTreeEntry
#define ROOT7_RTreeEntry

#include <ROOT/RCargo.hxx>

#include <memory>
#include <vector>
#include <utility>

namespace ROOT {
namespace Experimental {

class RTreeModel;

class RTreeEntry {
   friend class RTree;
   friend class RTreeModel;

   RTreeModel *fModel;
   CargoCollection fCargo;

   bool HasCompatibleModelId(RTreeModel *model);

   CargoCollection& GetCargoRefs() { return fCargo; }

public:
   RTreeEntry(RTreeModel *model) : fModel(model) { }

   template <typename T, typename... ArgsT>
   std::shared_ptr<T> AddCargo(ArgsT&&... args) {
     auto cargo = std::make_shared<RCargo<T>>(std::forward<ArgsT>(args)...);
     auto value_ptr = cargo->Get();
     fCargo.emplace_back(std::move(cargo));
     return value_ptr;
   }

   template <typename T, typename... ArgsT>
   void AddCargoCaptured(ArgsT&&... args) {
     auto cargo =
       std::make_shared<RCargoCaptured<T>>(std::forward<ArgsT>(args)...);
     fCargo.emplace_back(std::move(cargo));
   }

   void AddCargoSubtree(std::shared_ptr<RCargoSubtree> cargo) {
     fCargo.emplace_back(cargo);
   }

   bool IsCompatibleWith(RTreeModel *model) {
     return (model == fModel) || HasCompatibleModelId(model);
   }
};

} // namespace Experimental
} // namespace ROOT

#endif
