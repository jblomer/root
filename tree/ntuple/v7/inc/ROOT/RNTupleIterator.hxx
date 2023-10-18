/// \file ROOT/RNTupleIterator.hxx
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2023-10-17
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2023, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_RNTupleIterator
#define ROOT7_RNTupleIterator

#include <ROOT/RSpan.hxx>

#include <memory>
#include <vector>
#include <utility>

namespace ROOT {
namespace Experimental {

class RFieldDescriptor;
class RNTupleDescriptor;
class RNTupleModel;
namespace Detail {
class RPageSource;
}

// clang-format off
/**
\class ROOT::Experimental::RNTupleIterator
\ingroup NTuple
\brief

*/
// clang-format on
class RNTupleIterator {
public:
   class RDataSet {
      enum class EType { kSingle, kChain, kFriends };

      EType fType;
      std::vector<std::unique_ptr<RDataSet>> fParts;
      std::unique_ptr<Detail::RPageSource> fSource;
      std::unique_ptr<RNTupleDescriptor> fDescriptor;

      RDataSet() = default;

      void EnsureDescriptor();

   public:
      using TopLevelDescriptor_t = std::vector<std::pair<const RFieldDescriptor &, const RNTupleDescriptor &>>;
      static std::unique_ptr<RDataSet> Create(std::unique_ptr<Detail::RPageSource> source);
      static std::unique_ptr<RDataSet> CreateChain(std::span<std::unique_ptr<RDataSet>> parts);
      static std::unique_ptr<RDataSet> CreateFriends(std::span<std::unique_ptr<RDataSet>> parts);

      // Iterate fields, find fields by (name, parent ID)
      // Get on-disk ID & real page source for (field, entry number)


      //// For RDF to build its own field tree
      //TopLevelDescriptor_t GetTopLevelDescriptor();
      //// default field tree from the on-disk information
      //std::unique_ptr<RNTupleModel> GenerateBareModel();
   }; // class RDataSet

private:
   std::unique_ptr<RDataSet> fDataSet;
}; // class RNTupleIterator

} // namespace Experimental
} // namespace ROOT

#endif
