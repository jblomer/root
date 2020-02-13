/// \file ROOT/RBrowseVisitor.hxx
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

#ifndef ROOT7_RBrowseVisitor
#define ROOT7_RBrowseVisitor

#include <ROOT/RField.hxx>
#include <ROOT/RFieldVisitor.hxx>
#include <ROOT/RNTupleBrowser.hxx>

#include <TH1.h>

#include <cassert>
#include <cmath>
<<<<<<< HEAD
#include <limits.h>
=======
#include <limits>
#include <type_traits>
>>>>>>> [ntuple] beautify ntuple browser code

class TBrowser;

namespace ROOT {
namespace Experimental {

// clang-format off
/**
\class ROOT::Experimental::RBrowseVisitor
\ingroup NTupleBrowse
\brief Visitor class which adds the RNTupleBrowseLeaf or RNTupleBrowseFolder shims to the TBrowser.
*/
// clang-format on
class RBrowseVisitor : public Detail::RNTupleVisitor {
private:
   /// Passed down to RNTupleBrowseLeaf or RNTupleBrowseFolder.
   TBrowser *fBrowser;
   /// Passed down to RNTupleBrowseLeaf or RNTupleBrowseFolder.
   RNTupleBrowser *fNtplBrowser;

public:
   RBrowseVisitor(TBrowser *b, RNTupleBrowser *ntplBrowser) : fBrowser(b), fNtplBrowser(ntplBrowser) {}

   /// Creates instance of RNTupleBrowseLeaf or RNTupleBrowseFolder and displays it in TBrowser.
<<<<<<< HEAD
   void VisitField(const Detail::RFieldBase &field, int level) final;
   // Do nothing for RootField
=======
   void VisitField(const Detail::RFieldBase &field) final;
   // Visit the top-level fields
>>>>>>> [ntuple] beautify ntuple browser code
   void VisitRootField(const RFieldRoot &field) final;
};


// clang-format off
/**
\class ROOT::Experimental::RDisplayHistVisitor
\ingroup NTupleBrowse
\brief Draws a histogram for scalar fields with numerical data.

Visits fields displayed in TBrowser and draws a histogram for appropriate fields. Instances of this class are created
 when a field without subfields is double-clicked in TBrowser. (called by RNTupleBrowseLeaf::Browse(TBrowser* b))
*/
// clang-format on
class RDisplayHistVisitor : public Detail::RNTupleVisitor {
private:
   /// Allows to access RNTupleBrowser::fCurrentHist and the ntuple reader
   RNTupleBrowser *fNtplBrowser;

public:
   RDisplayHistVisitor(RNTupleBrowser *ntplBrowser)
      : fNtplBrowser{ntplBrowser}
   {
   }

   void VisitField(const Detail::RFieldBase & /*field*/) final {}
   void VisitRootField(const RFieldRoot & /*field*/) final {}
   void VisitFloatField(const RField<float> &field) { DrawHistogram<float>(field); }
   void VisitDoubleField(const RField<double> &field) { DrawHistogram<double>(field); }
   void VisitInt32Field(const RField<std::int32_t> &field) { DrawHistogram<std::int32_t>(field); }
   void VisitUInt32Field(const RField<std::uint32_t> &field)
   {
      DrawHistogram<std::uint32_t>(field);
   }
   void VisitUInt64Field(const RField<std::uint64_t> &field, int /*level*/)
   {
      DrawHistogram<std::uint64_t>(field);
   }

   template <typename T>
   void DrawHistogram(const Detail::RFieldBase &field)
   {
      auto ntupleView = fNtplBrowser->GetReader()->GetView<T>(field.GetQualifiedName());

      // if min = 3 and max = 10, a histogram with a x-axis range of 3 to 10 is created with 8 bins (3, 4, 5, 6, 7, 8,
      // 9, 10)
      double max{LONG_MIN}, min{LONG_MAX};
      // TODO(lesimon): Think how RNTupleView can be interated only once.
      RNTupleGlobalRange viewRange(0, field.GetNElements());
      for (auto i : viewRange) {
         max = std::max(max, static_cast<double>(ntupleView(i)));
         min = std::min(min, static_cast<double>(ntupleView(i)));
      }
      if (min > max)
         return; // no histogram for empty field.

      // It doesn't make sense to create 100 bins if only integers from 3 to 10 are used to fill the histogram.
      int nbins = std::is_integral<T>::value ? std::min(100, static_cast<int>(std::round(max - min) + 1)) : 100;
      // deleting the old TH1-histogram after creating a new one makes cling complain if both histograms
      // have the same name.
      fNtplBrowser->SetCurrentHist(nullptr);
      auto h1 = new TH1F(field.GetName().c_str(), field.GetName().c_str(), nbins, min - 0.5, max + 0.5);
      for (auto i : viewRange) {
         h1->Fill(ntupleView(i));
      }
      h1->Draw();
      fNtplBrowser->SetCurrentHist(h1);
   }
};

} // namespace Experimental
} // namespace ROOT

#endif /* ROOT7_RBrowseVisitor */
