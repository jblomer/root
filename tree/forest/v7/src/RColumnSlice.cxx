/// \file RColumnSlice.cxx
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

#include "ROOT/RColumnSlice.hxx"

ROOT::Experimental::RColumnSlice::RColumnSlice(std::size_t capacity, std::uint64_t range_start)
   : fCapacity(capacity)
   , fSize(0)
   , fRangeStart(range_start)
{
   assert(fCapacity > 0);
   fBuffer = static_cast<unsigned char *>(std::malloc(fCapacity));
   assert(fBuffer);
}


ROOT::Experimental::RColumnSlice::~RColumnSlice()
{
   std::free(fBuffer);
}
