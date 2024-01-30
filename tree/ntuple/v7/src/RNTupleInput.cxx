/// \file RNTupleInput.cxx
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2023-01-29
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2024, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RNTupleInput.hxx>
#include <ROOT/RPageStorage.hxx>

#include <mutex>

ROOT::Experimental::RNTupleInput::RNTupleInput(std::unique_ptr<Detail::RPageSource> pageSource)
   : fPageSource(std::move(pageSource))
{
   fPageSource->Attach();
}

std::unique_ptr<ROOT::Experimental::RNTupleInput> ROOT::Experimental::RNTupleInput::Create(
   std::string_view ntupleName, std::string_view location, const RNTupleReadOptions &options)
{
   return std::unique_ptr<RNTupleInput>(new RNTupleInput(Detail::RPageSource::Create(ntupleName, location, options)));
}

std::unique_ptr<ROOT::Experimental::RNTupleInput> ROOT::Experimental::RNTupleInput::Clone() const
{
   return std::unique_ptr<RNTupleInput>(new RNTupleInput(fPageSource->Clone()));
}

const ROOT::Experimental::RNTupleReadOptions &ROOT::Experimental::RNTupleInput::GetReadOptions() const
{
   return fPageSource->GetReadOptions();
}

const ROOT::Experimental::RNTupleDescriptor &ROOT::Experimental::RNTupleInput::GetDescriptor() const
{
   static std::mutex gMutex;
   const std::lock_guard<std::mutex> lock(gMutex);

   auto descriptorGuard = fPageSource->GetSharedDescriptorGuard();
   if (!fCachedDescriptor || fCachedDescriptor->GetGeneration() != descriptorGuard->GetGeneration())
      fCachedDescriptor = descriptorGuard->Clone();
   return *fCachedDescriptor;
}

void ROOT::Experimental::RNTupleInput::SetEntryRange(const REntryRange &range)
{
   fPageSource->SetEntryRange({range.fFirstEntry, range.fNEntries});
}

ROOT::Experimental::RNTupleInput::REntryRange ROOT::Experimental::RNTupleInput::GetEntryRange() const
{
   auto range = fPageSource->GetEntryRange();
   return REntryRange{range.fFirstEntry, range.fNEntries};
}
