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

#include <string>
#include <vector>

#include <cassert>
#include <cstdint>

namespace ROOT {
namespace Experimental {

using OffsetColumn_t = std::uint64_t;

enum class EColumnType {
   kUnknown,
   kOffset,
   kByte,
   kFloat,
   kDouble,
   kInt32,
   // TODO: kBit needed?
};

constexpr char const* gColumnTypeNames[] = {
   "void",
   "ROOT::Experimental::OffsetColumn_t",
   "unsigned char",
   "float",
   "double",
   "std::int32_t"
};

template <typename T>
EColumnType MakeColumnType() { assert(false); return EColumnType::kUnknown; }

template <>
EColumnType MakeColumnType<float>();

template <>
EColumnType MakeColumnType<double>();

template <>
EColumnType MakeColumnType<OffsetColumn_t>();

template <>
EColumnType MakeColumnType<std::int32_t>();

std::vector<std::string> SplitString(
   const std::string &str,
   const char delim,
   const unsigned max_chunks = 0);

} // namespace Experimental
} // namespace ROOT



#endif
