/// \file ROOT/RColumnPointer.hxx
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

#ifndef ROOT7_RColumnPointer
#define ROOT7_RColumnPointer

#include <cstdint>

namespace ROOT {
namespace Experimental {

class RColumnPointer {
private:
  std::uint64_t fIndex;

public:
  static const std::uint64_t kInvalidIndex = std::uint64_t(-1);
  explicit RColumnPointer(std::uint64_t index)
    : fIndex(index)
  { }

  bool operator ==(const RColumnPointer &other) const {
    return fIndex == other.fIndex;
  }

  bool operator !=(const RColumnPointer &other) const {
    return fIndex != other.fIndex;
  }

  std::uint64_t GetIndex() const { return fIndex; }
}; // RColumnPointer

} // namespace Experimental
} // namespace ROOT

#endif
