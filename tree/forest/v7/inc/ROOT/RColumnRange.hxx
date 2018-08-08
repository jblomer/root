/// \file ROOT/RColumnRange.hxx
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

#ifndef ROOT7_RColumnRange
#define ROOT7_RColumnRange

#include <ROOT/RColumnPointer.hxx>

#include <cstdint>

#include "iterator_tpl.h"

namespace ROOT {
namespace Experimental {

class RColumnRange {
private:
  std::uint64_t fBegin;
  std::uint64_t fEnd;

public:
  explicit RColumnRange(std::uint64_t b, std::uint64_t e)
    : fBegin(b), fEnd(e) { }

  std::uint64_t GetBegin() const { return fBegin; }
  std::uint64_t GetEnd() const { return fEnd; }
  std::uint64_t GetSize() const { return fEnd - fBegin; }

  struct ColumnIterator {
    std::uint64_t pos;
    inline void next(const RColumnRange* ref) {
      pos++;
    }
    inline void begin(const RColumnRange* ref) {
      pos = ref->fBegin;
    }
    inline void end(const RColumnRange* ref) {
      pos = ref->fEnd;
    }
    inline RColumnPointer get(RColumnRange* ref) {
      return RColumnPointer(pos);
    }
    inline const RColumnPointer get(const RColumnRange* ref)
    {
      return RColumnPointer(pos);
    }
    inline bool cmp(const ColumnIterator& s) const {
      return (pos != s.pos);
    }
  };
  SETUP_ITERATORS(RColumnRange, RColumnPointer, ColumnIterator);
}; // RColumnRange

} // namespace Experimental
} // namespace ROOT

#endif
