/// \file RPageStorage.cxx
/// \ingroup Forest ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2018-10-04
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2015, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <ROOT/RPageStorage.hxx>
#include <ROOT/RColumn.hxx>
#include <ROOT/RPagePool.hxx>

#include <ROOT/RStringView.hxx>

std::atomic<ROOT::Experimental::TreeId_t> ROOT::Experimental::Detail::RPageStorage::fgTreeId(0);

ROOT::Experimental::Detail::RPageStorage::RPageStorage() : fTreeId(++fgTreeId)
{
}

ROOT::Experimental::Detail::RPageStorage::~RPageStorage()
{
}

ROOT::Experimental::Detail::RPageSource::RPageSource(std::string_view /*treeName*/)
{
}

ROOT::Experimental::Detail::RPageSource::~RPageSource()
{
}

ROOT::Experimental::Detail::RPageSink::RPageSink(std::string_view /*treeName*/)
{
}

ROOT::Experimental::Detail::RPageSink::~RPageSink()
{
}
