/// \file RCargo.cxx
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

#include "ROOT/RCargo.hxx"

#include "ROOT/RBranch.hxx"
#include "ROOT/RColumnUtil.hxx"

#include <vector>


void ROOT::Experimental::RCargoSubtree::Fill() {
  for (auto child_cargo : fChildren)
    child_cargo->fBranch->Append(child_cargo.get());
  fOffset++;
  //std::cout << "Filling " << fBranch->GetName() << " with " << fOffset
  //  << std::endl;
}

void ROOT::Experimental::RCargoSubtree::FillV(RCargoBase **cargo, unsigned size) {
  for (unsigned i = 0; i < size; ++i)
    cargo[i]->fBranch->Append(cargo[i]);
  fOffset += size;
}

template <>
void ROOT::Experimental::RCargo<ROOT::Experimental::OffsetColumn_t>::Init() {
  fPrincipalElement = new ROOT::Experimental::RColumnElement<OffsetColumn_t>(fValue.get());
  fIsSimple = true;
}

//template <>
//void ROOT::Experimental::RCargo<std::vector<float>>::Init() {
//  // TODO
//}

template <>
void ROOT::Experimental::RCargo<float>::Init() {
  fPrincipalElement = new ROOT::Experimental::RColumnElement<float>(fValue.get());
  fIsSimple = true;
}

template <>
void ROOT::Experimental::RCargoCaptured<float>::Init() {
  fPrincipalElement = new ROOT::Experimental::RColumnElement<float>(fValue);
  fIsSimple = true;
}

template <>
void ROOT::Experimental::RCargo<double>::Init() {
  fPrincipalElement = new ROOT::Experimental::RColumnElement<double>(fValue.get());
  fIsSimple = true;
}

template <>
void ROOT::Experimental::RCargo<std::int32_t>::Init() {
  fPrincipalElement = new ROOT::Experimental::RColumnElement<std::int32_t>(fValue.get());
  fIsSimple = true;
}
