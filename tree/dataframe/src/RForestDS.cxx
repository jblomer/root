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

#include <ROOT/RForestDS.hxx>
#include <ROOT/RDFUtils.hxx>
#include <ROOT/RMakeUnique.hxx>

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace ROOT {

namespace RDF {

RForestDS::RForestDS(ROOT::Experimental::RColumnSource* source)
   : fSlots(0)
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


RDataSource::Record_t RForestDS::GetColumnReadersImpl(std::string_view name, const std::type_info &ti)
{
   std::cout << "GetColumnReadersImpl" << std::endl;
}


std::vector<std::pair<ULong64_t, ULong64_t>> RForestDS::GetEntryRanges()
{
   std::cout << "GetEntryRanges" << std::endl;
}


std::string RForestDS::GetTypeName(std::string_view colName) const
{
   std::cout << "GetTypeName" << std::endl;
}


bool RForestDS::HasColumn(std::string_view colName) const
{
   std::cout << "HasColumn" << std::endl;
}


void RForestDS::Initialise() {
   std::cout << "Initialise" << std::endl;
}


bool RForestDS::SetEntry(unsigned int /* slot */, ULong64_t entry)
{
   std::cout << "SetEntry" << std::endl;
}


void RForestDS::SetNSlots(unsigned int nSlots)
{
   std::cout << "SetNSlots" << std::endl;
   fSlots = nSlots;

   fSources[0]->Attach();
   for (unsigned i = 1; i < fSlots; ++i) {
      std::unique_ptr<ROOT::Experimental::RColumnSource> clone(fSources[0]->Clone());
      fSources.push_back(clone.get());
      clone->Attach();
      fSourceClones.emplace_back(std::move(clone));
   }

   ROOT::Experimental::RColumnSource::ColumnList_t columns = fSources[0]->ListColumns();
   for (auto c : columns) {
      fColumnNames.emplace_back(c.GetName());
   }
}


} // ns RDF

} // ns ROOT
