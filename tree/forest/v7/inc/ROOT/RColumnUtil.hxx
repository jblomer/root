/// \file ROOT/RColumnUtil.hxx
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

#ifndef ROOT7_RColumnUtil
#define ROOT7_RColumnUtil

#include <cstdint>

namespace ROOT {
namespace Experimental {

using OffsetColumn_t = std::uint64_t;

enum class EColumnType {
   kUnknown,
   kFloat,
   kInt,
   kByte,
   kOffset,
   // TODO: kBit needed?
};

} // namespace Experimental
} // namespace ROOT

#endif
