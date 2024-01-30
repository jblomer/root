/// \file ROOT/RNTuple.hxx
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2024-01-29
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2024, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_RNTupleInput
#define ROOT7_RNTupleInput

#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RNTupleOptions.hxx>
#include <ROOT/RNTupleUtil.hxx>

#include <memory>
#include <string_view>

namespace ROOT {
namespace Experimental {

class RNTupleDescriptor;

namespace Detail {
class RPageSource;
}

// clang-format off
/**
\class ROOT::Experimental::RNTupleInput
\ingroup NTuple
\brief Handle of an RNTuple open for reading

The class provides the public front of a page source.
*/
// clang-format on
class RNTupleInput {
   friend class RNTuple;
   friend class RFieldBase;

private:
   std::unique_ptr<Detail::RPageSource> fPageSource;
   /// The ntuple descriptor in the page source is protected by a read-write lock. We don't expose that to the
   /// users of RNTupleReader::GetDescriptor().  Instead, if descriptor information is needed, we clone the
   /// descriptor.  Using the descriptor's generation number, we know if the cached descriptor is stale.
   mutable std::unique_ptr<RNTupleDescriptor> fCachedDescriptor;

   explicit RNTupleInput(std::unique_ptr<Detail::RPageSource> pageSource);

public:
   /// Used in SetEntryRange / GetEntryRange
   struct REntryRange {
      NTupleSize_t fFirstEntry = kInvalidNTupleIndex;
      NTupleSize_t fNEntries = 0;
   };

   static std::unique_ptr<RNTupleInput> Create(std::string_view ntupleName, std::string_view location,
                                               const RNTupleReadOptions &options = RNTupleReadOptions());
   std::unique_ptr<RNTupleInput> Clone() const;

   /// Promise to only read from the given entry range. If set, prevents the cluster pool from reading-ahead beyond
   /// the given range. The range needs to be within [0, GetNEntries()).
   void SetEntryRange(const REntryRange &range);
   REntryRange GetEntryRange() const;

   const RNTupleReadOptions &GetReadOptions() const;
   /// Returns a cached copy of the page source descriptor.
   const RNTupleDescriptor &GetDescriptor() const;
};

} // namespace Experimental
} // namespace ROOT

#endif
