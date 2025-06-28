/// \file ROOT/RNTupleUtil.hxx
/// \ingroup NTuple
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2018-10-04

/*************************************************************************
 * Copyright (C) 1995-2020, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_RNTupleUtil
#define ROOT_RNTupleUtil

#include <ROOT/RError.hxx>
#include <ROOT/RNTupleTypes.hxx>

#include <cstddef>
#include <limits>
#include <memory>
#include <string_view>

namespace ROOT {
namespace Internal {

template <typename T>
auto MakeAliasedSharedPtr(T *rawPtr)
{
   const static std::shared_ptr<T> fgRawPtrCtrlBlock;
   return std::shared_ptr<T>(fgRawPtrCtrlBlock, rawPtr);
}

/// Make an array of default-initialized elements. This is useful for buffers that do not need to be initialized.
///
/// With C++20, this function can be replaced by std::make_unique_for_overwrite<T[]>.
template <typename T>
std::unique_ptr<T[]> MakeUninitArray(std::size_t size)
{
   // DO NOT use std::make_unique<T[]>, the array elements are value-initialized!
   return std::unique_ptr<T[]>(new T[size]);
}

inline constexpr ENTupleColumnType kTestFutureColumnType =
   static_cast<ENTupleColumnType>(std::numeric_limits<std::underlying_type_t<ENTupleColumnType>>::max() - 1);

inline constexpr ROOT::ENTupleStructure kTestFutureFieldStructure =
   static_cast<ROOT::ENTupleStructure>(std::numeric_limits<std::underlying_type_t<ROOT::ENTupleStructure>>::max() - 1);

inline constexpr RNTupleLocator::ELocatorType kTestLocatorType = static_cast<RNTupleLocator::ELocatorType>(0x7e);
static_assert(kTestLocatorType < RNTupleLocator::ELocatorType::kLastSerializableType);

/// Check whether a given string is a valid name according to the RNTuple specification
RResult<void> EnsureValidNameForRNTuple(std::string_view name, std::string_view where);

} // namespace Internal
} // namespace ROOT

#endif
