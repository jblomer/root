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
private:
   std::unique_ptr<Detail::RPageSource> fPageSource;
   std::unique_ptr<RNTupleDescriptor> fDescriptor;

   explicit RNTupleInput(std::unique_ptr<Detail::RPageSource> pageSource);

public:
   RNTupleInput(std::string_view ntupleName, std::string_view location,
                const RNTupleReadOptions &options = RNTupleReadOptions());
   RNTupleInput Clone() const;

   const RNTupleReadOptions &GetReadOptions() const;
   const RNTupleDescriptor &GetDescriptor() const { return *fDescriptor; }
};

} // namespace Experimental
} // namespace ROOT

#endif
