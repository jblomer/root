/// \file RColumnElement.cxx
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

#include "ROOT/RColumnElement.hxx"


template <>
ROOT::Experimental::EColumnType  ROOT::Experimental::RColumnElement<float>::MapType()
{
   return ROOT::Experimental::EColumnType::kFloat;
}


template <>
void ROOT::Experimental::RColumnElement<float>::Initialize()
{
   fIsMovable = true;
   fRawContent = fValue;
   fSize = sizeof(float);
}


template <>
std::string ROOT::Experimental::RColumnElement<float>::GetMemoryType()
{
   return "float";
}


template <>
ROOT::Experimental::EColumnType  ROOT::Experimental::RColumnElement<ROOT::Experimental::OffsetColumn_t>::MapType()
{
   return ROOT::Experimental::EColumnType::kOffset;
}


template <>
void ROOT::Experimental::RColumnElement< ROOT::Experimental::OffsetColumn_t>::Initialize()
{
   fIsMovable = true;
   fRawContent = fValue;
   fSize = sizeof(ROOT::Experimental::OffsetColumn_t);
}


template <>
std::string  ROOT::Experimental::RColumnElement<ROOT::Experimental::OffsetColumn_t>::GetMemoryType()
{
   return "index";
}
