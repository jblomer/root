// Author: Enrico Guiraud, Danilo Piparo CERN  09/2018

/*************************************************************************
 * Copyright (C) 1995-2018, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "ROOT/RDF/RLoopManager.hxx"
#include "ROOT/RDF/RRangeBase.hxx"

using ROOT::Detail::RDF::RRangeBase;
using ROOT::Detail::RDF::RLoopManager;

RRangeBase::RRangeBase(RLoopManager *implPtr, unsigned int start, unsigned int stop, unsigned int stride,
                       const unsigned int nSlots)
   : RNodeBase(implPtr), fStart(start), fStop(stop), fStride(stride), fNSlots(nSlots) { }

void RRangeBase::ResetCounters()
{
   fLastCheckedEntry = -1;
   fNProcessedEntries = 0;
   fHasStopped = false;
}

RRangeBase::~RRangeBase()
{
   fLoopManager->Deregister(this);
}
