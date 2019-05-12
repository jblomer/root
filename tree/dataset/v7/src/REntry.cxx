/// \file REntry.cxx
/// \ingroup DataSet ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2018-10-15
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <ROOT/RFieldValue.hxx>
#include <ROOT/REntry.hxx>

ROOT::Experimental::REntry::~REntry()
{
   for (auto idx : fManagedValues) {
      fValues[idx].GetField()->DestroyValue(fValues[idx]);
   }
}

void ROOT::Experimental::REntry::AddValue(const Detail::RFieldValueBase& value)
{
   fManagedValues.emplace_back(fValues.size());
   fValues.push_back(value);
}

void ROOT::Experimental::REntry::CaptureValue(const Detail::RFieldValueBase& value)
{
   fValues.push_back(value);
}
