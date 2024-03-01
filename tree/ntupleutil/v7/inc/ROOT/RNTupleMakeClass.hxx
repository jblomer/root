/// \file ROOT/RNTupleMakeClass.hxx
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2023-03-01
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2024, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_RNTupleMakeClass
#define ROOT7_RNTupleMakeClass

#include <iostream>
#include <map>
#include <set>
#include <string>

namespace ROOT {
namespace Experimental {

class RNTupleDescriptor;

// clang-format off
/**
\class ROOT::Experimental::RNTupleMakeClass
\ingroup NTuple
\brief Creates a C++ header file using the on-disk information of an RNTuple

The header file can be used to create a dictionary for an EDM found in an RNTuple.
*/
// clang-format on
class RNTupleMakeClass {
   std::set<std::string> fIncludes;
   std::map<std::string, std::string> fClassDecls;

public:
   RNTupleMakeClass(const RNTupleDescriptor &descriptor);

   void PrintHeader(std::ostream &output = std::cout);
   void PrintLinkdef(std::ostream &output = std::cout);
}; // class RNTupleMakeClass

} // namespace Experimental
} // namespace ROOT

#endif // ROOT7_RNTupleMakeClass
