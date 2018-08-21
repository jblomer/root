/// \file RColumn.cxx
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

#include "ROOT/RColumn.hxx"
#include "ROOT/RColumnStorage.hxx"

#include <cassert>
#include <iostream>

ROOT::Experimental::RColumn::RColumn(
   const RColumnModel &model,
   RColumnSource *source,
   RColumnSink *sink)
   : fModel(model)
   , fSource(source)
   , fSink(sink)
   , fHeadSlice(nullptr)
   , fMaxElement(0)
   , fCurrentSlice(nullptr)
   , fCurrentSliceStart(-1)
   , fCurrentSliceEnd(-1)
{
   if (fSink) {
      fSink->OnAddColumn(this);
      fHeadSlice =
        std::make_unique<RColumnSlice>(kDefaultSliceSize, 0);
   }
   if (fSource) {
      fSource->OnAddColumn(this);
      fCurrentSlice = std::make_unique<RColumnSlice>(kDefaultSliceSize, 0);
      fMaxElement = fSource->GetNElements(this);
   }
}


void ROOT::Experimental::RColumn::ShipHeadSlice()
{
   assert(fSink);
   fHeadSlice->Freeze();
   fSink->OnCommitSlice(fHeadSlice.get(), this);
   fHeadSlice->Reset(fMaxElement);
   //std::cout << "RESETTING TO " << fMaxElement << std::endl;
}


void ROOT::Experimental::RColumn::MapSlice(std::uint64_t num)
{
   fSource->OnMapSlice(this, num, fCurrentSlice.get());
   fCurrentSliceStart = fCurrentSlice->GetRangeStart();
   fCurrentSliceEnd = fCurrentSlice->GetRangeStart() +
      (fCurrentSlice->GetSize() / fModel.GetElementSize()) - 1;
}
