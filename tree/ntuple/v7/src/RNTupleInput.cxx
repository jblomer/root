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

ROOT::Experimental::RNTupleInput::RNTupleInput(std::unique_ptr<Detail::RPageSource> pageSource)
   : fPageSource(std::move(pageSource))
{
   fPageSource->Attach();
   fDescriptor = fPageSource->GetSharedDescriptorGuard()->Clone();
}

ROOT::Experimental::RNTupleInput::RNTupleInput(std::string_view ntupleName, std::string_view location,
                                               const RNTupleReadOptions &options)
   : RNTupleInput(Detail::RPageSource::Create(ntupleName, location, options))
{}

ROOT::Experimental::RNTupleInput ROOT::Experimental::RNTupleInput::Clone() const
{
   return RNTupleInput(fPageSource->Clone());
}

const ROOT::Experimental::RNTupleReadOptions &ROOT::Experimental::RNTupleInput::GetReadOptions() const
{
   return fPageSource->GetReadOptions();
}
