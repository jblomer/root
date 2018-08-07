/// \file ROOT/RColumnSlice.hxx
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

#ifndef ROOT7_RColumnSlice
#define ROOT7_RColumnSlice

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <iostream>

namespace ROOT {
namespace Experimental {

class RColumnSlice {
   unsigned char *fBuffer;
   std::size_t fCapacity;
   std::size_t fSize;
   std::uint64_t fRangeStart;

public:
   RColumnSlice(std::size_t capacity, std::uint64_t range_start);
   // TODO: copy, move
   ~RColumnSlice();

   std::size_t GetCapacity() const { return fCapacity; }
   std::size_t GetSize() const { return fSize; }
   void *GetBuffer() { return fBuffer; }
   std::uint64_t GetRangeStart() { return fRangeStart; }

   void *Reserve(std::size_t nbyte) {
      size_t pos = fSize;

      if (pos + nbyte > fCapacity) {
        return nullptr;
      }
      fSize += nbyte;

      return fBuffer + pos;
   }
   void Release() const {  }

   void Freeze() {
   }
   void Reset(uint64_t range_start) {
     fSize = 0;
     fRangeStart = range_start;
   }
};  // class RColumnSlice

} // namespace Experimental
} // namespace ROOT

#endif
