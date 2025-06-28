/// \file RNTupleTypes.cxx
/// \ingroup NTuple
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2025-06-30

/*************************************************************************
 * Copyright (C) 1995-2025, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "ROOT/RNTupleTypes.hxx"

#include "ROOT/RLogger.hxx"

ROOT::RLogChannel &ROOT::Internal::NTupleLog()
{
   static RLogChannel sLog("ROOT.NTuple");
   return sLog;
}
