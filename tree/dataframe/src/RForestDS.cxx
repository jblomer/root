// Author: Jakob Blomer CERN  07/2018

/*************************************************************************
 * Copyright (C) 1995-2017, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

// clang-format off
/** \class ROOT::RDF::RForestDS
    \ingroup dataframe
    \brief
*/
// clang-format on

#include <ROOT/RColumn.hxx>
#include <ROOT/RColumnElement.hxx>
#include <ROOT/RColumnUtil.hxx>
#include <ROOT/RForestDS.hxx>
#include <ROOT/RDFUtils.hxx>
#include <ROOT/RMakeUnique.hxx>
#include <ROOT/TSeq.hxx>

#include <TError.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace ROOT {

namespace RDF {

RForestDS::RForestDS(ROOT::Experimental::RColumnSource* source)
   : fNSlots(0)
   , fNentries(0)
   , fHasSeenAllRanges(false)
{
   fSources.push_back(source);
   std::cout << "Constructed" << std::endl;
}


RForestDS::~RForestDS()
{
   std::cout << "Destructed" << std::endl;
}


const std::vector<std::string>& RForestDS::GetColumnNames() const
{
   std::cout << "GetColumnNames" << std::endl;
   return fColumnNames;
}


RDataSource::Record_t RForestDS::GetColumnReadersImpl(std::string_view name, const std::type_info& /* ti */)
{
   std::cout << "GetColumnReadersImpl " << name << std::endl;
   ROOT::Experimental::RColumnModel columnModel;
   for (auto c : fColumnList) {
      if (c.GetName() == name) {
         columnModel = c;
         break;
      }
   }

   std::vector<void*> result;
   for (auto slot : ROOT::TSeqU(fNSlots)) {
      auto column = std::make_unique<ROOT::Experimental::RColumn>(
         columnModel, fSources[slot], nullptr);
      fColumns[slot].emplace_back(std::move(column));

      auto columnElement = std::make_unique<ROOT::Experimental::RColumnElementBase>(
         ROOT::Experimental::MakeColumnElement(columnModel.GetType()));
      result.push_back(columnElement->GetRawContentAddr());
      fColumnElements[slot].emplace_back(std::move(columnElement));
   }

   return result;
}


std::vector<std::pair<ULong64_t, ULong64_t>> RForestDS::GetEntryRanges()
{
   // TODO use cluster sizes
   std::cout << "GetEntryRanges" << std::endl;
   std::vector<std::pair<ULong64_t, ULong64_t>> result;
   if (fHasSeenAllRanges) return result;

   const auto chunkSize = fNentries / fNSlots;
   const auto reminder = 1U == fNSlots ? 0 : fNentries % fNSlots;
   auto start = 0UL;
   auto end = 0UL;
   for (auto i : ROOT::TSeqU(fNSlots)) {
      start = end;
      end += chunkSize;
      result.emplace_back(start, end);
      (void)i;
   }
   result.back().second += reminder;
   fHasSeenAllRanges = true;
   return result;
}


std::string RForestDS::GetTypeName(std::string_view colName) const
{
   std::cout << "GetTypeName" << std::endl;
   for (auto c : fColumnList) {
      if (c.GetName() == colName) {
         return ROOT::Experimental::gColumnTypeNames[static_cast<int>(c.GetType())];
      }
   }
   R__ASSERT(false);
}


bool RForestDS::HasColumn(std::string_view colName) const
{
   std::cout << "HasColumn" << std::endl;
   return std::find(fColumnNames.begin(), fColumnNames.end(), colName) !=
          fColumnNames.end();
}


void RForestDS::Initialise() {
   std::cout << "Initialise" << std::endl;
   fHasSeenAllRanges = false;
}


bool RForestDS::SetEntry(unsigned int slot, ULong64_t entry)
{
   //std::cout << "SetEntry" << std::endl;
   unsigned N = fColumns[slot].size();
   for (unsigned i = 0; i < N; ++i) {
      fColumns[slot][i]->Map(entry, fColumnElements[slot][i].get());
   }
   return true;
}


void RForestDS::SetNSlots(unsigned int nSlots)
{
   std::cout << "SetNSlots " << nSlots << std::endl;
   fNSlots = nSlots;

   fSources[0]->Attach();
   fNentries = fSources[0]->GetNentries();
   for (unsigned i = 1; i < fNSlots; ++i) {
      std::unique_ptr<ROOT::Experimental::RColumnSource> clone(fSources[0]->Clone());
      fSources.push_back(clone.get());
      clone->Attach();
      fSourceClones.emplace_back(std::move(clone));
   }

   fClusterList = fSources[0]->ListClusters();
   fColumnList = fSources[0]->ListColumns();
   for (auto c : fColumnList) {
      fColumnNames.emplace_back(c.GetName());
   }

   fColumns.resize(fNSlots);
   fColumnElements.resize(fNSlots);
}


} // ns RDF

} // ns ROOT
