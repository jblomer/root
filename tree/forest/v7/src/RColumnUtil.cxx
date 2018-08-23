/// \file RColumnUtil.cxx
/// \ingroup Forest ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2018-08-22
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2015, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "ROOT/RColumnUtil.hxx"

template <>
ROOT::Experimental::EColumnType ROOT::Experimental::MakeColumnType<ROOT::Experimental::OffsetColumn_t>()
{
   return ROOT::Experimental::EColumnType::kOffset;
}

template <>
ROOT::Experimental::EColumnType ROOT::Experimental::MakeColumnType<double>()
{
   return ROOT::Experimental::EColumnType::kDouble;
}

template <>
ROOT::Experimental::EColumnType ROOT::Experimental::MakeColumnType<float>()
{
   return ROOT::Experimental::EColumnType::kFloat;
}

template <>
ROOT::Experimental::EColumnType ROOT::Experimental::MakeColumnType<std::int32_t>()
{
   return ROOT::Experimental::EColumnType::kInt32;
}

