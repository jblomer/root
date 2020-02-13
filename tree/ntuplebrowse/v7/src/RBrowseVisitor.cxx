/// \file ROOT/RBrowseVisitor.cxx
/// \ingroup NTupleBrowse ROOT7
/// \author Simon Leisibach <simon.satoshi.rene.leisibach@cern.ch>
/// \date 2019-07-30
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <ROOT/RBrowseVisitor.hxx>

//------------------------ RBrowseVisitor -------------------------------

void ROOT::Experimental::RBrowseVisitor::VisitRootField(const RFieldRoot &field)
{
   for (auto f : field.GetSubFields())
      f->AcceptVisitor(*this);
}

void ROOT::Experimental::RBrowseVisitor::VisitField(const ROOT::Experimental::Detail::RFieldBase &field)
{
   if ((field.GetStructure() == kRecord) || (field.GetStructure() == kCollection)) {
      fBrowser->Add(new RNTupleBrowseFolder(fNTupleBrowserPtr, &field));
   } else {
      fBrowser->Add(new RNTupleBrowseLeaf(fNTupleBrowserPtr, &field));
   }
}
