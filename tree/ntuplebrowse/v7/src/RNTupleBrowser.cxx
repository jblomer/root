/// \file ROOT/RNTupleBrowser.cxx
/// \ingroup NTupleBrowse ROOT7
/// \author Simon Leisibach <simon.satoshi.rene.leisibach@cern.ch>
/// \date 2019-07-19
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
#include <ROOT/RFieldVisitor.hxx>
#include <ROOT/RNTupleBrowser.hxx>
#include <ROOT/RPageStorageFile.hxx>

#include <Rtypes.h>
#include <TDirectory.h>
#include <TFile.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>


//--------------------------- RNTupleBrowser -----------------------------


ROOT::Experimental::RNTupleBrowser::RNTupleBrowser(TBrowser *b, const std::string &ntupleName)
   : fBrowser(b)
{
   auto pageSource = Detail::RPageSourceFile::CreateFromTFile(ntupleName, dynamic_cast<TFile *>(gDirectory));
   fReader = std::make_unique<RNTupleReader>(std::unique_ptr<Detail::RPageSource>(pageSource));

   RBrowseVisitor browseVisitor(b, this);
   fReader->GetModel()->GetRootField()->AcceptVisitor(browseVisitor);
}


ROOT::Experimental::RNTupleBrowser::~RNTupleBrowser()
{
   // No need to delete fCurrentHist, because it's automatically deallocated when TBrowser is closed.
}


void ROOT::Experimental::RNTupleBrowser::SetCurrentHist(TH1F *h)
{
   delete fCurrentHist;
   fCurrentHist = h;
}


void ROOT::Experimental::RNTupleBrowser::AddBrowsable(RNTupleBrowsable *browsable)
{
   fBrowsables.emplace_back(std::unique_ptr<RNTupleBrowsable>(browsable));
}


//--------------------------- RNTupleBrowsable ---------------------------------


ClassImp(ROOT::Experimental::RNTupleBrowsable);


//---------------------------- NTupleBrowseFolder ------------------------------


ClassImp(ROOT::Experimental::RNTupleBrowseFolder);


void ROOT::Experimental::RNTupleBrowseFolder::Browse(TBrowser *b)
{
   RBrowseVisitor browseVisitor(b, fNtplBrowser);
   for (auto f : fField->GetSubFields()) {
      f->AcceptVisitor(browseVisitor);
   }
}


//------------------------------ RNTupleBrowseLeaf -----------------------------


ClassImp(ROOT::Experimental::RNTupleBrowseLeaf);


void ROOT::Experimental::RNTupleBrowseLeaf::Browse(TBrowser * /*b*/)
{
   RDisplayHistVisitor histVisitor(fNtplBrowser);
   fField->AcceptVisitor(histVisitor);
}
