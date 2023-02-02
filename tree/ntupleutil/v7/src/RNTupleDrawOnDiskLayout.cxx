/// \file RNTupleDrawOnDiskLayout.cxx
/// \ingroup NTuple ROOT7
/// \author Simon Leisibach <simon.leisibach@gmail.com>, Jakob Blomer <jblomer@cern.ch>
/// \date 2023-02-02
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2023, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <ROOT/RColumnElement.hxx>
#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RNTupleInspector.hxx>
#include <ROOT/RError.hxx>

#include <TBox.h>
#include <TCanvas.h>
#include <TH1.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TLine.h>
#include <TPad.h>
#include <TText.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
#include <string>
#include <typeinfo>
#include <vector>
#include <utility>

ClassImp(ROOT::Experimental::Internal::RNTupleBox)
ClassImp(ROOT::Experimental::Internal::RNTupleMetaDataBox)
ClassImp(ROOT::Experimental::Internal::RNTuplePageBox)

namespace ROOT {
namespace Experimental {
namespace Internal {

class RNTupleBox;

struct RNTupleDrawOnDiskLayout {
   std::unique_ptr<RNTupleDescriptor> fDescriptor;
   std::vector<std::unique_ptr<TCanvas>> fCanvases;
   std::unique_ptr<TPad> fPad;
   std::unique_ptr<TH1F> fHist;
   std::vector<std::unique_ptr<TText>> fTexts;
   std::vector<std::unique_ptr<TLine>> fLines;
   std::unique_ptr<TLegend> fLegend;
   std::vector<std::unique_ptr<RNTupleBox>> fBoxes;
   std::string fAxisTitle;
   std::uint64_t fTotalNumBytes = 0;
   std::uint64_t fNPages = 0;
   // E.g. if the total number of bytes in a file is 42 MB, then fScalingFactorOfAxis is set to
   // 1000*1000, so that 42 can be displayed on the axis.
   std::uint64_t fScalingFactorOfAxis = 1;

   std::int32_t GetColorFromFieldId(ROOT::Experimental::DescriptorId_t fieldId) const;

   explicit RNTupleDrawOnDiskLayout(std::unique_ptr<RNTupleDescriptor> d);
   void Draw();
};

RNTupleDrawOnDiskLayout::RNTupleDrawOnDiskLayout(std::unique_ptr<RNTupleDescriptor> d) : fDescriptor(std::move(d))
{
   // Preparing the GUI elements:
   // 1. Prepare title and legend
   // 2. Create all boxes and colour them
   // 3. Sort RPageBoxes by page order and set Page Ids
   // 4. Create CumulativeNBytes to later to set x1 and x2 values for the boxes
   // 5. Prepare StorageSizeAxis
   // 6. Set x1 and x2 values for all boxes
   // 7. Include box entries in the legend
   // 8. Prepare ClusterAxis

   // 1. Prepare title and legend
   std::string title = "Storage layout of " + fDescriptor->GetName();
   fTexts.emplace_back(std::make_unique<TText>(.5, .94, title.c_str()));
   fTexts.back()->SetTextAlign(22);
   fTexts.back()->SetTextSize(0.08);

   fLegend = std::make_unique<TLegend>(0.05, 0.05, .95, .55);
   int nColumnsInLegend = 2;
   if (fDescriptor->GetNPhysicalColumns() > 150) {
      nColumnsInLegend = 10;
   } else if (fDescriptor->GetNPhysicalColumns() > 120) {
      nColumnsInLegend = 9;
   } else if (fDescriptor->GetNPhysicalColumns() > 100) {
      nColumnsInLegend = 8;
   } else if (fDescriptor->GetNPhysicalColumns() > 75) {
      nColumnsInLegend = 7;
   } else if (fDescriptor->GetNPhysicalColumns() > 33) {
      nColumnsInLegend = 6;
   } else if (fDescriptor->GetNPhysicalColumns() > 26) {
      nColumnsInLegend = 5;
   } else if (fDescriptor->GetNPhysicalColumns() > 19) {
      nColumnsInLegend = 4;
   } else if (fDescriptor->GetNPhysicalColumns() > 4) {
      nColumnsInLegend = 3;
   }
   fLegend->SetNColumns(nColumnsInLegend);

   // 2. Create all boxes and colour them
   constexpr double boxY1 = 0.;
   constexpr double boxY2 = 1.;

   auto headerBox = std::make_unique<RNTupleMetaDataBox>(0.0, boxY1, 0.0, boxY2, this);
   headerBox->fPosition = 0;
   headerBox->fNBytes = fDescriptor->GetOnDiskHeaderSize();
   headerBox->fDescription = "Header";
   headerBox->SetFillColor(kGray);
   fBoxes.emplace_back(std::move(headerBox));

   std::uint64_t maxPosition = fBoxes.front()->fNBytes;
   for (const auto &cluster : fDescriptor->GetClusterIterable()) {
      for (const auto &colId : cluster.GetColumnIds()) {
         auto fieldId = fDescriptor->GetColumnDescriptor(colId).GetFieldId();
         const auto &pageRange = cluster.GetPageRange(colId);
         for (std::size_t i = 0; i < pageRange.fPageInfos.size(); ++i) {
            const auto &locator = pageRange.fPageInfos.at(i).fLocator;
            auto pageBox = std::make_unique<RNTuplePageBox>(0.0, boxY1, 0.0, boxY2, this);
            pageBox->fPosition = locator.GetPosition<std::uint64_t>();
            pageBox->fNBytes = locator.fBytesOnStorage;
            pageBox->fFieldId = fieldId;
            pageBox->fClusterId = cluster.GetId();
            pageBox->fColumnId = colId;
            pageBox->SetFillColor(GetColorFromFieldId(fieldId));
            maxPosition = std::max(maxPosition, pageBox->fPosition + pageBox->fNBytes);
            fBoxes.emplace_back(std::move(pageBox));
         }
      }
   }

   for (const auto &cg : fDescriptor->GetClusterGroupIterable()) {
      const auto &locator = cg.GetPageListLocator();
      auto pageListBox = std::make_unique<RNTupleMetaDataBox>(0.0, boxY1, 0.0, boxY2, this);
      pageListBox->fPosition = locator.GetPosition<std::uint64_t>();
      pageListBox->fNBytes = locator.fBytesOnStorage;
      pageListBox->fDescription = "Page List";
      pageListBox->SetFillColor(kGray + 2);
      maxPosition = std::max(maxPosition, pageListBox->fPosition + pageListBox->fNBytes);
      fBoxes.emplace_back(std::move(pageListBox));
   }

   auto footerBox = std::make_unique<RNTupleMetaDataBox>(0.0, boxY1, 0.0, boxY2, this);
   footerBox->fPosition = maxPosition;
   footerBox->fNBytes = fDescriptor->GetOnDiskFooterSize();
   footerBox->fDescription = "Footer";
   footerBox->SetFillColor(kGray);
   fBoxes.emplace_back(std::move(footerBox));

   // 4. Sort RPageBoxes by page order and set Page Ids
   std::sort(fBoxes.begin(), fBoxes.end(),
             [](const std::unique_ptr<RNTupleBox> &a, const std::unique_ptr<RNTupleBox> &b) -> bool
             {
                return a->fPosition < b->fPosition;
             });

   // 5. Create CumulativeNBytes to later to set x1 and x2 values for the boxes
   // Size is +2, because +1 for header and +1 for footer
   std::vector<std::uint64_t> cumulativeNBytes;
   cumulativeNBytes.emplace_back(0);
   for (std::size_t i = 0; i < fBoxes.size(); ++i) {
      cumulativeNBytes.emplace_back(cumulativeNBytes.back() + fBoxes.at(i)->fNBytes);
      if (typeid(*fBoxes[i]) == typeid(RNTuplePageBox)) {
         dynamic_cast<RNTuplePageBox *>(fBoxes[i].get())->fPageNum = ++fNPages;
      }
   }

   // 6. Prepare StorageSizeAxis
   fAxisTitle = "#splitline{Data}{#splitline{Size}{#splitlie{in}{Bytes}}}";
   fTotalNumBytes = cumulativeNBytes.back();
   fScalingFactorOfAxis = 1;
   if (fTotalNumBytes > 1000 * 1000 * 1000) {
      fScalingFactorOfAxis = 1000 * 1000 * 1000;
      fAxisTitle = "#splitline{Data}{#splitline{Size}{in GB}}";
   } else if (fTotalNumBytes > 1000 * 1000) {
      fScalingFactorOfAxis = 1000 * 1000;
      fAxisTitle = "#splitline{Data}{#splitline{Size}{in MB}}";
   } else if (fTotalNumBytes > 1000) {
      fScalingFactorOfAxis = 1000;
      fAxisTitle = "#splitline{Data}{#splitline{Size}{in KB}}";
   }

   // 7. Set x1 and x2 values for all boxes
   for (std::size_t i = 0; i < fBoxes.size(); ++i) {
      fBoxes.at(i)->SetX1((double)cumulativeNBytes.at(i) / fScalingFactorOfAxis);
      fBoxes.at(i)->SetX2((double)cumulativeNBytes.at(i + 1) / fScalingFactorOfAxis);
   }

   // 8. Include box entries in the legend
   fLegend->AddEntry(fBoxes.front().get(), "Header", "f");
   // TODO: all fields recursively
   std::deque<DescriptorId_t> fieldIds;
   fieldIds.emplace_back(fDescriptor->GetFieldZeroId());
   while (!fieldIds.empty()) {
      const auto &f = fDescriptor->GetFieldDescriptor(fieldIds.front());
      fieldIds.pop_front();
      // for each field find the first PageBox which represents that field and add it to the legend
      auto vecIt = std::find_if(fBoxes.begin(), fBoxes.end(),
                                [&f](const std::unique_ptr<RNTupleBox> &a) -> bool
                                { return a->fFieldId == f.GetId(); });
      if (vecIt != fBoxes.end()) {
         fLegend->AddEntry((*vecIt).get(), fDescriptor->GetQualifiedFieldName(f.GetId()).c_str(), "f");
      }
      for (const auto &sf : fDescriptor->GetFieldIterable(f))
         fieldIds.emplace_back(sf.GetId());
   }
   fLegend->AddEntry(fBoxes.back().get(), "Footer", "f");
   fLegend->AddEntry((fBoxes.rbegin() + 1)->get(), "Page List", "f");

   // 9. Prepare ClusterAxis
   double distanceBetweenLines = 0.001 * (fTotalNumBytes / fScalingFactorOfAxis);
   std::size_t start = cumulativeNBytes.at(1);
   std::size_t end{0};
   auto nextClusterId = fDescriptor->FindClusterId(0, 0);
   while (nextClusterId != kInvalidDescriptorId) {
      const auto &cluster = fDescriptor->GetClusterDescriptor(nextClusterId);
      end = start + cluster.GetBytesOnStorage();
      double x1 = (double)start / fScalingFactorOfAxis + distanceBetweenLines / 2;
      double x2 = (double)end / fScalingFactorOfAxis - distanceBetweenLines / 2;
      fLines.emplace_back(std::make_unique<TLine>(x1, 1.05, x2, 1.05));
      fLines.back()->SetLineWidth(3);
      fTexts.emplace_back(std::make_unique<TText>((x1 + x2) / 2, 1.2, std::to_string(cluster.GetId()).c_str()));
      fTexts.back()->SetTextAlign(22);
      fTexts.back()->SetTextSize(0.15);
      start = end;
      nextClusterId = fDescriptor->FindNextClusterId(nextClusterId);
   }
   fLegend->AddEntry(fLines.back().get(), "Cluster Id", "l");
}

void RNTupleDrawOnDiskLayout::Draw()
{
   // Procedure:
   // 1. Check for special cases like empty ntuple
   // 2. Create a new canvas
   // 3. Create a TPad in the canvas so that when zooming only the boxes and axis get zoomed
   // 4. Draw an empty histogram without a y-axis for zooming
   // 5. Draw all boxes and add possibility to click on RPageBox to obtain information about a page
   // 6. Draw clusterAxis
   // 7. Return to canvas, draw title, legend and description of x-axis

   // 1. Check for special cases like empty ntuple
   if (fDescriptor->GetNPhysicalColumns() == 0)
      throw RException(R__FAIL("The ntuple has no columns. Cannot draw on-disk layout."));
   if (fDescriptor->GetNEntries() == 0)
      throw RException(R__FAIL("The ntuple has no entries. Cannot draw on-disk layout."));

   // 2. Create a new canvas
   static std::uint64_t uniqueId = 0;
   // Trying to delete multiple canvases with the same name leads to an error or when two canvases have the same name,
   // only 1 may get deleted, causing a memory leak.
   std::string canvasName = "RDrawOnDiskLayout" + std::to_string(++uniqueId);
   fCanvases.emplace_back(std::make_unique<TCanvas>(canvasName.c_str(), fDescriptor->GetName().c_str(), 1000, 300));

   // 3. Create a TPad in the canvas so that when zooming only the boxes and axis get zoomed
   constexpr double marginlength = 0.03;
   std::string padName = "RDrawOnDiskLayout" + std::to_string(uniqueId);
   fPad = std::make_unique<TPad>(padName.c_str(), "", marginlength, 0.55, 1 - marginlength, 0.87);
   fPad->SetTopMargin(0.2);
   fPad->SetBottomMargin(0.2);
   fPad->SetLeftMargin(0.01);
   fPad->SetRightMargin(0.01);
   fPad->Draw();
   fPad->cd();

   // 4. Draw an empty histogram without a y-axis for zooming
   std::string h1Name = "RDrawOnDiskLayoutTH1F" + std::to_string(uniqueId);
   fHist = std::make_unique<TH1F>(h1Name.c_str(), "", 500, 0, (double)fTotalNumBytes / fScalingFactorOfAxis);
   fHist->SetMaximum(1);
   fHist->SetMinimum(0);
   fHist->GetYaxis()->SetTickLength(0);
   fHist->GetYaxis()->SetLabelSize(0);
   fHist->GetXaxis()->SetLabelSize(0.18);
   fHist->SetStats(0);
   fHist->DrawCopy();

   // 5. Draw all boxes and add possibility to click on RPageBox to obtain information about a page
   for (const auto &b : fBoxes) {
      b->Draw();
   }
   fPad->AddExec("ShowPageDetails", "ROOT::Experimental::Internal::RDrawOnDiskLayoutHandler::RPageBoxClicked()");

   // 6. Draw clusterAxis
   // fTexts.at(0) points to Title so skip
   for (std::size_t i = 1; i < fTexts.size(); ++i) {
      fTexts.at(i)->Draw();
   }
   for (const auto &l : fLines) {
      l->Draw();
   }
   fPad->Update();

   // 7. Return to canvas, draw title, legend and description of x-axis
   fCanvases.back()->cd();
   fTexts.at(0)->Draw(); // Title
   fLegend->Draw();
   TLatex latex;
   latex.SetTextSize(0.04);
   latex.DrawLatex(0.955, 0.5, fAxisTitle.c_str());

   fCanvases.back()->Update();
}

std::int32_t RNTupleDrawOnDiskLayout::GetColorFromFieldId(ROOT::Experimental::DescriptorId_t fieldId) const
{
   std::int32_t color = 0;
   fieldId %= 61;
   switch (fieldId % 12) {
      case 0: color = kRed; break;
      case 1: color = kMagenta; break;
      case 2: color = kBlue; break;
      case 3: color = kCyan; break;
      case 4: color = kGreen; break;
      case 5: color = kYellow; break;
      case 6: color = kPink; break;
      case 7: color = kViolet; break;
      case 8: color = kAzure; break;
      case 9: color = kTeal; break;
      case 10: color = kSpring; break;
      case 11: color = kOrange; break;
      default:
         // never here
         assert(false);
         break;
   }
   switch (fieldId / 12) {
      case 0: color -= 2; break;
      case 1: break;
      case 2: color += 3; break;
      case 3: color -= 6; break;
      case 4: color -= 9; break;
      case 5:
         if (fieldId == 60)
            return kGray;
      default:
         // never here
         assert(false);
         break;
   }
   return color;
}

} // namespace Internal
} // namespace Experimental
} // namespace ROOT


void ROOT::Experimental::Internal::RNTupleMetaDataBox::Dump() const
{
   std::cout << " ==> Dumping Page information:\n";
   std::cout << "Description:            \t\t" << fDescription << std::endl;
   std::cout << "Size:                   \t\t" << fNBytes << " bytes" << std::endl;
}

void ROOT::Experimental::Internal::RNTupleMetaDataBox::Inspect() const
{
   static std::int64_t uniqueId = 0;
   std::string canvasName{"RNTupleMetaDataDetails" + std::to_string(++uniqueId)};

   fParent->fCanvases.emplace_back(
      std::make_unique<TCanvas>(canvasName.c_str(), "Meta Data Block Details", 500, 300));
   TLatex latex;

   // Draw Title
   latex.SetTextAlign(12);
   latex.SetTextSize(0.08);
   latex.DrawLatex(0.01, 0.96, fDescription.c_str());

   // Write Details
   latex.SetTextSize(0.06);
   std::string sizeString = "Size:" + std::string(30, ' ') + std::to_string(fNBytes) + " bytes";
   latex.DrawLatex(0.01, 0.85, sizeString.c_str());
   fParent->fCanvases.back()->Update();
}

void ROOT::Experimental::Internal::RNTuplePageBox::Dump() const
{
   using RColumnElementBase = ROOT::Experimental::Detail::RColumnElementBase;
   const auto &desc = *fParent->fDescriptor;
   const auto &fieldDesc = desc.GetFieldDescriptor(fFieldId);
   const auto &columnDesc = desc.GetColumnDescriptor(fColumnId);
   auto columnType = columnDesc.GetModel().GetType();

   std::cout << " ==> Dumping Page information:\n";
   std::cout << "Page number:            \t\t" << fPageNum << " / " << fParent->fNPages << std::endl;
   std::cout << "Cluster Id:             \t\t" << fClusterId << std::endl;
   std::cout << "Field Id:               \t\t" << fFieldId << std::endl;
   std::cout << "FieldName:              \t\t" << fieldDesc.GetFieldName() << std::endl;
   std::cout << "FieldType:              \t\t" << fieldDesc.GetTypeName() << std::endl;
   std::cout << "Column Id:              \t\t" << fColumnId << std::endl;
   std::cout << "Column type:            \t\t" << RColumnElementBase::GetTypeName(columnType) << std::endl;
   std::cout << "NElements:              \t\t" << desc.GetNElements(fColumnId) << std::endl;
   std::cout << "Element size on disk:   \t\t" << RColumnElementBase::GetBitsOnStorage(columnType) << " bits"
                                               << std::endl;
   //std::cout << "Page size on disk:      \t\t" << fNElements * fElementSizeOnDisk / 8 << " bytes" << std::endl;
   //std::cout << "Page size in memory:    \t\t" << fLocator.fBytesOnStorage << " bytes" << std::endl;
   //std::cout << "Global page range:      \t\t" << fGlobalRangeStart << " - " << fGlobalRangeStart + fNElements - 1
   //          << std::endl;
   //std::cout << "Cluster page range:     \t\t" << fClusterRangeStart << " - " << fClusterRangeStart + fNElements - 1
   //          << std::endl;
   //std::size_t totalNumBytes = fParent->GetTotalNumBytes();
   //std::size_t scalingFactorOfAxis = fParent->GetScalingFactorOfAxis();
   //std::cout.setf(std::ios::fixed);
   //std::cout << "Location in Storage:    \t\t" << static_cast<std::size_t>(GetX1() * scalingFactorOfAxis) << " / "
   //          << totalNumBytes << " bytes" << std::endl;
   std::cout.unsetf(std::ios::fixed);
}

void ROOT::Experimental::Internal::RNTuplePageBox::Inspect() const
{
   /*
   static std::int32_t index = 0;
   // The canvases need to have unique names, or else there will be an error saying that not all were found when trying
   // to delete them when quitting the program.
   std::string uniqueCanvasName{"PageDetails" + std::to_string(++index)};
   fParent->fCanvasPtrs.emplace_back(new TCanvas(uniqueCanvasName.c_str(), "Page Details", 500, 300));

      TLatex latex;
      // Draw Title
      latex.SetTextAlign(12);
      latex.SetTextSize(0.08);
      std::string pageNumbering =
         "Page No. " + std::to_string(fPageBoxId) + " / " + std::to_string(fParent->GetPageBoxSize());
      latex.DrawLatex(0.01, 0.96, pageNumbering.c_str());

      // Write details about page
      latex.SetTextSize(0.06);
      std::string clusterIdString = "Cluster Id:" + std::string(37, ' ') + std::to_string(fClusterId) + " / " +
                                    std::to_string(fParent->GetNClusters() - 1);
      latex.DrawLatex(0.01, 0.85, clusterIdString.c_str());
      std::string fieldIdString =
         "Field Id:" + std::string(41, ' ') + std::to_string(fFieldId) + " / " + std::to_string(fParent->GetNFields() - 1);
      latex.DrawLatex(0.01, 0.80, fieldIdString.c_str());
      std::string fieldName = "FieldName:" + std::string(35, ' ') + fFieldName;
      latex.DrawLatex(0.01, 0.75, fieldName.c_str());
      std::string fieldType = "FieldType:" + std::string(37, ' ') + fFieldType;
      latex.DrawLatex(0.01, 0.70, fieldType.c_str());
      std::string columnIdString = "Column Id:" + std::string(36, ' ') + std::to_string(fColumnId) + " / " +
                                   std::to_string(fParent->GetNColumns() - 1);
      latex.DrawLatex(0.01, 0.65, columnIdString.c_str());std::string columnTypeString = "ColumnType:" + std::string(32, ' ') + fColumnType;
      latex.DrawLatex(0.01, 0.60, columnTypeString.c_str());
      std::string nElements = "NElements:" + std::string(35, ' ') + std::to_string(fNElements);
      latex.DrawLatex(0.01, 0.55, nElements.c_str());
      std::string elementSizeOnDisk =
         "Element Size On Disk:" + std::string(17, ' ') + std::to_string(fElementSizeOnDisk) + " bits";
      latex.DrawLatex(0.01, 0.50, elementSizeOnDisk.c_str());
      std::string elementSizeOnStorage = "Element Size On Storage:" + std::string(11, ' ') +
                                         std::to_string(8 * fLocator.fBytesOnStorage / fNElements) + " bits";
      latex.DrawLatex(0.01, 0.45, elementSizeOnStorage.c_str());
      std::string pageSize =
         "Page Size On Disk:" + std::string(22, ' ') + std::to_string(fNElements * fElementSizeOnDisk / 8) + " bytes";
      latex.DrawLatex(0.01, 0.40, pageSize.c_str());
      std::string pageSizeStorage =
         "Page Size On Storage:" + std::string(16, ' ') + std::to_string(fLocator.fBytesOnStorage) + " bytes";
      latex.DrawLatex(0.01, 0.35, pageSizeStorage.c_str());
      std::string globalRange = "Global Page Range:" + std::string(21, ' ') + std::to_string(fGlobalRangeStart) + " - " +
                                std::to_string(fGlobalRangeStart + fNElements - 1);
      latex.DrawLatex(0.01, 0.30, globalRange.c_str());
      std::string clusterRange = "Cluster Page Range:" + std::string(20, ' ') + std::to_string(fClusterRangeStart) +
                                 " - " + std::to_string(fClusterRangeStart + fNElements - 1);
      latex.DrawLatex(0.01, 0.25, clusterRange.c_str());
      std::size_t totalNumBytes = fParent->GetTotalNumBytes();
      std::size_t scalingFactorOfAxis = fParent->GetScalingFactorOfAxis();
      std::string locationString = "Location in Storage:" + std::string(20, ' ') +
                                   std::to_string(static_cast<std::size_t>(GetX1() * scalingFactorOfAxis)) + " / " +
                                   std::to_string(totalNumBytes) + " bytes";
      latex.DrawLatex(0.01, 0.20, locationString.c_str());

      fParent->fCanvasPtrs.back()->Update();
      */
}

void ROOT::Experimental::Internal::RDrawOnDiskLayoutHandler::RPageBoxClicked()
{
   int event = gPad->GetEvent();
   if (event != kButton1Up)
      return;
   TObject *select = gPad->GetSelected();
   if (!select)
      return;
   if (select->InheritsFrom(ROOT::Experimental::Internal::RNTuplePageBox::Class())) {
      auto pageBox = reinterpret_cast<ROOT::Experimental::Internal::RNTuplePageBox *>(select);
      pageBox->Inspect();
   } else if (select->InheritsFrom(ROOT::Experimental::Internal::RNTupleMetaDataBox::Class())) {
      auto metaBox = reinterpret_cast<ROOT::Experimental::Internal::RNTupleMetaDataBox *>(select);
      metaBox->Inspect();
   }
}

void ROOT::Experimental::RNTupleInspector::RDrawOnDiskLayoutDeleter::operator() (Internal::RNTupleDrawOnDiskLayout *p)
{
   delete p;
}

void ROOT::Experimental::RNTupleInspector::DrawOnDiskLayout()
{
   fDrawOnDiskLayout = decltype(fDrawOnDiskLayout)(new Internal::RNTupleDrawOnDiskLayout(
      fPageSource->GetSharedDescriptorGuard()->Clone()));
   fDrawOnDiskLayout->Draw();
}
