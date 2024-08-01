/// \file ROOT/RPageAllocator.hxx
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2019-06-25
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_RPageAllocator
#define ROOT7_RPageAllocator

#include <ROOT/RNTupleUtil.hxx>
#include <ROOT/RPage.hxx>

#include <cstddef>
#include <functional>

namespace ROOT {
namespace Experimental {
namespace Internal {

// clang-format off
/**
\class ROOT::Experimental::Internal::RPageAllocator
\ingroup NTuple
\brief Abstract interface to allocate and release pages

The page allocator acquires and releases memory for pages.  It does not populate the pages, the returned pages
are empty but guaranteed to have enough contiguous space for the given number of elements.
*/
// clang-format on
class RPageAllocator {
public:
   /// Reserves memory large enough to hold nElements of the given size. The page is immediately tagged with
   /// a column id. Returns a default constructed page on out-of-memory condition.
   virtual RPage NewPage(ColumnId_t columnId, std::size_t elementSize, std::size_t nElements) = 0;
   /// Releases the memory pointed to by page and resets the page's information. Note that the memory of the
   /// zero page must not be deleted.
   virtual void DeletePage(RPage &page) = 0;
};

// clang-format off
/**
\class ROOT::Experimental::Internal::RPageAllocatorHeap
\ingroup NTuple
\brief Uses standard C++ memory allocation for the column data pages
*/
// clang-format on
class RPageAllocatorHeap : public RPageAllocator {
public:
   RPage NewPage(ColumnId_t columnId, std::size_t elementSize, std::size_t nElements) final;
   void DeletePage(RPage &page) final;
};

} // namespace Internal
} // namespace Experimental
} // namespace ROOT

#endif
