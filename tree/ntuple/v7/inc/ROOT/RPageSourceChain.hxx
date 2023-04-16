/// \file ROOT/RPageSourceChain.hxx
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2023-04-16
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2023, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_RPageSourceChain
#define ROOT7_RPageSourceChain

#include <ROOT/RPageStorage.hxx>
#include <ROOT/RNTupleMetrics.hxx>

namespace ROOT {
namespace Experimental {
namespace Detail {

// clang-format off
/**
\class ROOT::Experimental::Detail::RPageSourceChain
\ingroup NTuple
\brief Virtual storage that combines several other sources vertically
*/
// clang-format on
class RPageSourceChain final : public RPageSource {
private:
   RNTupleMetrics fMetrics;

protected:
   RNTupleDescriptor AttachImpl() final;

public:
   RPageSourceChain(std::string_view ntupleName, std::span<std::unique_ptr<RPageSource>> sources);

   std::unique_ptr<RPageSource> Clone() const final;
   ~RPageSourceChain() final;

   ColumnHandle_t AddColumn(DescriptorId_t fieldId, const RColumn &column) final;
   void DropColumn(ColumnHandle_t columnHandle) final;

   RPage PopulatePage(ColumnHandle_t columnHandle, NTupleSize_t globalIndex) final;
   RPage PopulatePage(ColumnHandle_t columnHandle, const RClusterIndex &clusterIndex) final;
   void ReleasePage(RPage &page) final;

   void
   LoadSealedPage(DescriptorId_t physicalColumnId, const RClusterIndex &clusterIndex, RSealedPage &sealedPage) final;

   std::vector<std::unique_ptr<RCluster>> LoadClusters(std::span<RCluster::RKey> clusterKeys) final;

   RNTupleMetrics &GetMetrics() final { return fMetrics; }
}; // class RPageSourceChain

} // namespace Detail
} // namespace Experimental
} // namespace ROOT

#endif
