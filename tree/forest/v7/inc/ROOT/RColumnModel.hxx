/// \file ROOT/RColumnModel.hxx
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

#ifndef ROOT7_RColumnModel
#define ROOT7_RColumnModel

#include <ROOT/RColumnUtil.hxx>
#include <ROOT/RStringView.hxx>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ROOT {
namespace Experimental {

///\class RColumnModel
/// Meta-data of a column
class RColumnModel {
   std::string fName;
   std::string fBranchProvenance;
   EColumnType fType;
   bool fIsSorted;
   std::size_t fElementSize;
   std::uint32_t fCompressionSettings;

public:
   static constexpr std::uint32_t kDefaultCompression = 1;

   RColumnModel()
      : fName()
      , fBranchProvenance()
      , fType(EColumnType::kUnknown)
      , fIsSorted(false)
      , fElementSize(0)
      , fCompressionSettings(kDefaultCompression)
   {
   }

   RColumnModel(std::string_view name,
                std::string_view provenance,
                EColumnType type,
                bool is_sorted)
     : fName(name)
     , fBranchProvenance(provenance)
     , fType(type)
     , fIsSorted(is_sorted)
     , fCompressionSettings(kDefaultCompression)
   {
      switch (fType) {
      case EColumnType::kFloat:
         fElementSize = sizeof(float);
         break;
      case EColumnType::kOffset:
         fElementSize = sizeof(std::uint64_t);
         break;
      default:
         fElementSize = 0;
      }
   }

  EColumnType GetType() const { return fType; }
  std::string GetName() const { return fName; }
  std::size_t GetElementSize() const { return fElementSize; }
  std::uint32_t GetCompressionSettings() const { return fCompressionSettings; }
};

} // namespace Experimental
} // namespace ROOT

#endif
