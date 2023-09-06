/// \file RPageSourceChain.cxx
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2023-09-05
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2023, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <ROOT/RColumn.hxx>
#include <ROOT/RPageSourceChain.hxx>

ROOT::Experimental::Detail::RPageSourceChain::RPageSourceChain(
    std::string_view ntupleName, std::span<std::unique_ptr<RPageSource>> sources)
    : RPageSource(ntupleName, RNTupleReadOptions())
    , fMetrics(std::string(ntupleName))
{
   for (auto &s : sources) {
      fSources.emplace_back(std::move(s));
      fMetrics.ObserveMetrics(fSources.back()->GetMetrics());
   }
}

std::unique_ptr<ROOT::Experimental::Detail::RPageSource> ROOT::Experimental::Detail::RPageSourceChain::Clone() const
{
   std::vector<std::unique_ptr<RPageSource>> cloneSources;
   for (const auto &f : fSources)
      cloneSources.emplace_back(f->Clone());
   return std::make_unique<RPageSourceChain>(fNTupleName, cloneSources);
}

ROOT::Experimental::RNTupleDescriptor ROOT::Experimental::Detail::RPageSourceChain::AttachImpl()
{

}

ROOT::Experimental::Detail::RPageStorage::ColumnHandle_t
ROOT::Experimental::Detail::RPageSourceChain::AddColumn(DescriptorId_t fieldId, const RColumn &column)
{

}

void ROOT::Experimental::Detail::RPageSourceChain::DropColumn(ColumnHandle_t columnHandle)
{

}

ROOT::Experimental::Detail::RPage
ROOT::Experimental::Detail::RPageSourceChain::PopulatePage(ColumnHandle_t columnHandle, NTupleSize_t globalIndex)
{

}

ROOT::Experimental::Detail::RPage
ROOT::Experimental::Detail::RPageSourceChain::PopulatePage(ColumnHandle_t columnHandle,
                                                           const RClusterIndex &clusterIndex)
{

}

void ROOT::Experimental::Detail::RPageSourceChain::ReleasePage(RPage &page)
{

}

void ROOT::Experimental::Detail::RPageSourceChain::LoadSealedPage(
   DescriptorId_t physicalColumnId, const RClusterIndex &clusterIndex, RSealedPage &sealedPage)
{

}

std::vector<std::unique_ptr<ROOT::Experimental::Detail::RCluster>>
ROOT::Experimental::Detail::RPageSourceChain::LoadClusters(std::span<RCluster::RKey> clusterKeys)
{

}
