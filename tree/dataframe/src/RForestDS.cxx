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

#include <string>
#include <vector>

namespace ROOT {

namespace RDF {

RForestDS::RForestDS(ROOT::Experimental::RColumnSource *source)
{
}


RForestDS::~RForestDS()
{
}


const std::vector<std::string>& RForestDS::GetColumnNames() const
{
}


RDataSource::Record_t RForestDS::GetColumnReadersImpl(std::string_view name, const std::type_info &ti)
{
}


std::vector<std::pair<ULong64_t, ULong64_t>> RForestDS::GetEntryRanges()
{
}


std::string RForestDS::GetTypeName(std::string_view colName) const
{
}


bool RForestDS::HasColumn(std::string_view colName) const
{
}


void RForestDS::Initialise() {
}


bool RForestDS::SetEntry(unsigned int /* slot */, ULong64_t entry)
{
}


void RForestDS::SetNSlots(unsigned int nSlots)
{
}


} // ns RDF

} // ns ROOT
