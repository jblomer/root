/// \file ROOT/RNTupleBrowser.hxx
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

#ifndef ROOT7_RNTupleBrowser
#define ROOT7_RNTupleBrowser

#include <ROOT/RField.hxx>
#include <ROOT/RNTuple.hxx>

#include <TBrowser.h>
#include <TH1.h>
#include <TNamed.h>

#include <memory>
#include <vector>

/* The sequence of actions between TBrowser and RNTuple browser support:
 *
 * Double-clicking a ROOT file containing an RNTuple:
 *  - The TBrowser finds a key of type RNTuple (it is a TKey object in the TBrowser code)
 *  - The etc/mimes file tells the TBrowser to create a new RNTupleBrowser upon double-clicking the RNTuple
 *
 * Double-clicking the RNTupleBrowser
 *  - A new instance of RNTupleBrowser is created through the interpreter
 *  - The RNTupleBrowser creates a RNTupleReader based on the RNTuple name and the file that gDirectory points to
 *  - For all direct children of the root field, it visits the RBrowseVisitor
 *
 * RBrowseVisitor
 *  - A scalar field adds a new RNTupleBrowseLeaf object to the TBrowser
 *  - A collection or record adds a new RNTupleBrowseFolder object to the TBrowser
 *
 * Double-clicking RNTupleBrowseLeaf
 *  - If the field is numeric, creates a histogram
 *
 * * Double-clicking RNTupleBrowseFolder
 *  - For all direct children of the root field, it visits the RBrowseVisitor
 */

namespace ROOT {
namespace Experimental {

// clang-format off
/**
\class ROOT::Experimental::RNTupleBrowser
\ingroup NTupleBrowse
\brief Coordinates the communication between TBrowser and RNTupleReader

The RNTupleBrowser keeps the RNTupleReader used to traverse the fields. It is created by the TBrowser
via the interpreter, as specified in the ROOT mime file.  It also stores the histogram that is used for
scalar, numeric fields.
*/
// clang-format on

class RNTupleBrowser {
private:
   /// The browser instance that called RNTupleBrowser
   TBrowser *fBrowser;
   /// The reader helps in traversing the fields and in reading data for histograms
   std::unique_ptr<RNTupleReader> fReader;
   /// Keeps the histogram for the right side of the browser. This changes from on scalar, numeric field
   /// to another. Points to currently displayed histogram.
   TH1F *fCurrentHist = nullptr;

public:
   RNTupleBrowser(TBrowser *b, const std::string &ntupleName);
   ~RNTupleBrowser();

   Bool_t IsFolder() const { return kTRUE; }

   TBrowser *GetBrowser() const { return fBrowser; }
   RNTupleReader *GetReader() const { return fReader.get(); }
   void SetCurrentHist(TH1F *h);
};


// clang-format off
/**
\class ROOT::Experimental::RNTupleBrowseFolder
\ingroup NTupleBrowse
\brief Is displayed in the TBrowser as a field with children.

It represents an RNTuple-field which has children. It is displayed in the TBrowser with a folder symbol. The
subfields can be displayed by double-clicking this object, which calls Browse(TBrowser *b)
*/
// clang-format on
class RNTupleBrowseFolder : public TNamed {
private:
   /// Pointer to the field it represents.
   const Detail::RFieldBase *fField = nullptr;
   /// Pointer to the instance of RNTupleBrowser which created it through RBrowseVisitor::VisitField().
   RNTupleBrowser *fNtplBrowser = nullptr;

public:
   // ClassDef requires a constructor which can be called without any arguments. Never actually called.
   RNTupleBrowseFolder() = default;

   RNTupleBrowseFolder(RNTupleBrowser *ntplBrowser, const Detail::RFieldBase *field)
      : fField(field), fNtplBrowser(ntplBrowser)
   {
      fName = TString(field->GetName());
   }

   /// Displays subfields in TBrowser. Called when double-clicked.
   void Browse(TBrowser *b) final;
   Bool_t IsFolder() const final { return true; }

   ClassDef(RNTupleBrowseFolder, 0)
};


// clang-format off
/**
\class ROOT::Experimental::RNTupleBrowseLeaf
\ingroup NTupleBrowse
\brief Is displayed in the TBrowser as a field without children.

It represents scalar ntuple field. If the field is numeric, double-clicking show the histogram of the field's data.
*/
// clang-format on
class RNTupleBrowseLeaf : public TNamed {
private:
   /// Pointer to the instance of RNTupleBrowser which created it through RBrowseVisitor::VisitField().
   RNTupleBrowser *fNtplBrowser;
   /// Pointer to the field it represents in TBrowser.
   const Detail::RFieldBase *fField;

public:
   // ClassDef requires a constructor which can be called without any arguments. Never actually called.
   RNTupleBrowseLeaf() = default;

   RNTupleBrowseLeaf(RNTupleBrowser *ntplBrowser, const Detail::RFieldBase *field)
      : fNtplBrowser(ntplBrowser), fField(field)
   {
      fName = TString(field->GetName());
   }

   /// Displays a histogram for numerical fields in TBrowser. Called when double-clicked.
   void Browse(TBrowser *b) final;
   Bool_t IsFolder() const final { return false; }

   ClassDef(RNTupleBrowseLeaf, 0)
};

} // namespace Experimental
} // namespace ROOT

#endif // ROOT7_RNTupleBrowser
