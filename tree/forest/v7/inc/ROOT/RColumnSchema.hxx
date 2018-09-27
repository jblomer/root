/// \file ROOT/RColumnSchema.hxx
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

#ifndef ROOT7_RColumnSchema
#define ROOT7_RColumnSchema

#include <ROOT/RColumnModel.hxx>
#include <ROOT/RColumnUtil.hxx>

#include <memory>
#include <utility>
#include <vector>

namespace ROOT {
namespace Experimental {

class RColumnSchema {
   std::vector<std::unique_ptr<RColumnSchema>> fChildren;
   RColumnModel fModel;
   RColumnSchema *fParent;

   void AddChild(const RColumnModel &model) {
      auto schemaPtr = std::make_unique<RColumnSchema>(model);
      schemaPtr->fParent = this;
      fChildren.emplace_back(std::move(schemaPtr));
   }

public:
   RColumnSchema() : fParent(nullptr) { }
   explicit RColumnSchema(const RColumnModel &model) : fModel(model), fParent(nullptr) { }

   void Insert(const RColumnModel &/*model*/) {
      //auto tokens = SplitString(model, '/');
      //std::string
   }

   void SetModel(const RColumnModel &model) {
      fModel = model;
   }
};

} // namespace Experimental
} // namespace ROOT

#endif
