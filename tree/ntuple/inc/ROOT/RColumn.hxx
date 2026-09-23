/// \file ROOT/RColumn.hxx
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2018-10-09

/*************************************************************************
 * Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_RColumn
#define ROOT_RColumn

#include <ROOT/RConfig.hxx> // for R__likely
#include <ROOT/RColumnElementBase.hxx>
#include <ROOT/RNTupleTypes.hxx>
#include <ROOT/RPage.hxx>
#include <ROOT/RPageStorage.hxx>

#include <TError.h>

#include <cstring> // for memcpy
#include <memory>
#include <utility>

namespace ROOT::Internal {

// clang-format off
/**
\class ROOT::Internal::RColumn
\ingroup NTuple
\brief A column is a storage-backed array of a simple, fixed-size type, from which pages can be mapped into memory.
*/
// clang-format on
class RColumn {
private:
   /// Information needed when connected to a page sink
   struct RWriteInfo {
      ROOT::Internal::RPageSink *fPageSink = nullptr;
      /// The initial number of elements in a page
      ROOT::NTupleSize_t fInitialNElements = 1;
      /// The number of elements written
      ROOT::NTupleSize_t fNElements = 0;
      /// Global index of the first element in this column; usually == 0, unless it is a deferred column
      ROOT::NTupleSize_t fFirstElementIndex = 0;
      /// The page into which new elements are being written. The page will initially be small
      /// (RNTupleWriteOptions::fInitialUnzippedPageSize, which corresponds to fInitialElements) and expand as needed
      /// and as memory for page buffers is still available (RNTupleWriteOptions::fPageBufferBudget) or the maximum page
      /// size is reached (RNTupleWriteOptions::fMaxUnzippedPageSize).
      ROOT::Internal::RPage fWritePage;
   };

   /// Information needed when connected to a page source
   struct RReadInfo {
      ROOT::Internal::RPageSource *fPageSource = nullptr;
      /// The currently mapped page for reading
      ROOT::Internal::RPageRef fReadPageRef;
      /// The column team is a set of columns that serve the same column index for different representation IDs.
      /// Initially, the team has only one member, the very column it belongs to. Through MergeTeams(), two columns
      /// can join forces. The team is used to react on suppressed columns: if the current team member has a suppressed
      /// column for a MapPage() call, it get the page from the active column in the corresponding cluster.
      std::vector<RColumn *> fTeam;
      /// Points into fTeam to the column that successfully returned the last page.
      std::size_t fLastGoodTeamIdx = 0;
   };

   /// The column id in the column descriptor, once connected to a sink or source
   ROOT::DescriptorId_t fOnDiskId = ROOT::kInvalidDescriptorId;

   std::unique_ptr<RWriteInfo> fWriteInfo;
   std::unique_ptr<RReadInfo> fReadInfo;

   /// Used to pack and unpack pages on writing/reading
   std::unique_ptr<ROOT::Internal::RColumnElementBase> fElement;
   /// The type of fElement
   ROOT::ENTupleColumnType fType;
   /// Columns belonging to the same field are distinguished by their order.  E.g. for an std::string field, there is
   /// the offset column with index 0 and the character value column with index 1.
   std::uint32_t fIndex;
   /// Fields can have multiple column representations, distinguished by representation index
   std::uint16_t fRepresentationIndex;

   RColumn(ROOT::ENTupleColumnType type, std::uint32_t columnIndex, std::uint16_t representationIndex);

   ROOT::Internal::RPageStorage::ColumnHandle_t MakeHandle() { return {fOnDiskId, this}; }

   /// Used when trying to append to a full write page. If possible, expand the page. Otherwise, flush and reset
   /// to the minimal size.
   void HandleWritePageIfFull()
   {
      auto newMaxElements = fWriteInfo->fWritePage.GetMaxElements() * 2;
      if (newMaxElements * fElement->GetSize() > fWriteInfo->fPageSink->GetWriteOptions().GetMaxUnzippedPageSize()) {
         newMaxElements = fWriteInfo->fPageSink->GetWriteOptions().GetMaxUnzippedPageSize() / fElement->GetSize();
      }

      if (newMaxElements == fWriteInfo->fWritePage.GetMaxElements()) {
         // Maximum page size reached, flush and reset
         Flush();
      } else {
         auto expandedPage = fWriteInfo->fPageSink->ReservePage(MakeHandle(), newMaxElements);
         if (expandedPage.IsNull()) {
            Flush();
         } else {
            memcpy(expandedPage.GetBuffer(), fWriteInfo->fWritePage.GetBuffer(), fWriteInfo->fWritePage.GetNBytes());
            expandedPage.Reset(fWriteInfo->fNElements);
            expandedPage.GrowUnchecked(fWriteInfo->fWritePage.GetNElements());
            fWriteInfo->fWritePage = std::move(expandedPage);
         }
      }

      assert(fWriteInfo->fWritePage.GetNElements() < fWriteInfo->fWritePage.GetMaxElements());
   }

public:
   template <typename CppT>
   static std::unique_ptr<RColumn>
   Create(ROOT::ENTupleColumnType type, std::uint32_t columnIdx, std::uint16_t representationIdx)
   {
      auto column = std::unique_ptr<RColumn>(new RColumn(type, columnIdx, representationIdx));
      column->fElement = ROOT::Internal::RColumnElementBase::Generate<CppT>(type);
      return column;
   }

   RColumn(const RColumn &) = delete;
   RColumn &operator=(const RColumn &) = delete;
   ~RColumn();

   /// Connect the column to a page sink.  `firstElementIndex` can be used to specify the first column element index
   /// with backing storage for this column.  On read back, elements before `firstElementIndex` will cause the zero page
   /// to be mapped.
   void ConnectPageSink(ROOT::DescriptorId_t fieldId, ROOT::Internal::RPageSink &pageSink,
                        ROOT::NTupleSize_t firstElementIndex = 0U);
   /// Connect the column to a page source.
   void ConnectPageSource(ROOT::DescriptorId_t fieldId, ROOT::Internal::RPageSource &pageSource);

   void Append(const void *from)
   {
      if (fWriteInfo->fWritePage.GetNElements() == fWriteInfo->fWritePage.GetMaxElements()) {
         HandleWritePageIfFull();
      }

      void *dst = fWriteInfo->fWritePage.GrowUnchecked(1);

      std::memcpy(dst, from, fElement->GetSize());
      fWriteInfo->fNElements++;
   }

   void AppendV(const void *from, std::size_t count)
   {
      auto src = reinterpret_cast<const unsigned char *>(from);
      // TODO(jblomer): A future optimization should grow the page in one go, up to the maximum unzipped page size
      while (count > 0) {
         std::size_t nElementsRemaining =
            fWriteInfo->fWritePage.GetMaxElements() - fWriteInfo->fWritePage.GetNElements();
         if (nElementsRemaining == 0) {
            HandleWritePageIfFull();
            nElementsRemaining = fWriteInfo->fWritePage.GetMaxElements() - fWriteInfo->fWritePage.GetNElements();
         }

         assert(nElementsRemaining > 0);
         auto nBatch = std::min(count, nElementsRemaining);

         void *dst = fWriteInfo->fWritePage.GrowUnchecked(nBatch);
         std::memcpy(dst, src, nBatch * fElement->GetSize());
         src += nBatch * fElement->GetSize();
         count -= nBatch;
         fWriteInfo->fNElements += nBatch;
      }
   }

   void Read(const ROOT::NTupleSize_t globalIndex, void *to)
   {
      if (!fReadInfo->fReadPageRef.Get().Contains(globalIndex)) {
         MapPage(globalIndex);
      }
      const auto elemSize = fElement->GetSize();
      void *from = static_cast<unsigned char *>(fReadInfo->fReadPageRef.Get().GetBuffer()) +
                   (globalIndex - fReadInfo->fReadPageRef.Get().GetGlobalRangeFirst()) * elemSize;
      std::memcpy(to, from, elemSize);
   }

   void Read(RNTupleLocalIndex localIndex, void *to)
   {
      if (!fReadInfo->fReadPageRef.Get().Contains(localIndex)) {
         MapPage(localIndex);
      }
      const auto elemSize = fElement->GetSize();
      void *from = static_cast<unsigned char *>(fReadInfo->fReadPageRef.Get().GetBuffer()) +
                   (localIndex.GetIndexInCluster() - fReadInfo->fReadPageRef.Get().GetLocalRangeFirst()) * elemSize;
      std::memcpy(to, from, elemSize);
   }

   void ReadV(ROOT::NTupleSize_t globalIndex, ROOT::NTupleSize_t count, void *to)
   {
      const auto elemSize = fElement->GetSize();
      auto tail = static_cast<unsigned char *>(to);

      while (count > 0) {
         if (!fReadInfo->fReadPageRef.Get().Contains(globalIndex)) {
            MapPage(globalIndex);
         }
         const ROOT::NTupleSize_t idxInPage = globalIndex - fReadInfo->fReadPageRef.Get().GetGlobalRangeFirst();

         const void *from =
            static_cast<unsigned char *>(fReadInfo->fReadPageRef.Get().GetBuffer()) + idxInPage * elemSize;
         const ROOT::NTupleSize_t nBatch = std::min(fReadInfo->fReadPageRef.Get().GetNElements() - idxInPage, count);

         std::memcpy(tail, from, elemSize * nBatch);

         tail += nBatch * elemSize;
         count -= nBatch;
         globalIndex += nBatch;
      }
   }

   void ReadV(RNTupleLocalIndex localIndex, ROOT::NTupleSize_t count, void *to)
   {
      const auto elemSize = fElement->GetSize();
      auto tail = static_cast<unsigned char *>(to);

      while (count > 0) {
         if (!fReadInfo->fReadPageRef.Get().Contains(localIndex)) {
            MapPage(localIndex);
         }
         ROOT::NTupleSize_t idxInPage =
            localIndex.GetIndexInCluster() - fReadInfo->fReadPageRef.Get().GetLocalRangeFirst();

         const void *from =
            static_cast<unsigned char *>(fReadInfo->fReadPageRef.Get().GetBuffer()) + idxInPage * elemSize;
         const ROOT::NTupleSize_t nBatch = std::min(count, fReadInfo->fReadPageRef.Get().GetNElements() - idxInPage);

         std::memcpy(tail, from, elemSize * nBatch);

         tail += nBatch * elemSize;
         count -= nBatch;
         localIndex = RNTupleLocalIndex(localIndex.GetClusterId(), localIndex.GetIndexInCluster() + nBatch);
      }
   }

   template <typename CppT>
   CppT *Map(const ROOT::NTupleSize_t globalIndex)
   {
      ROOT::NTupleSize_t nItems;
      return MapV<CppT>(globalIndex, nItems);
   }

   template <typename CppT>
   CppT *Map(RNTupleLocalIndex localIndex)
   {
      ROOT::NTupleSize_t nItems;
      return MapV<CppT>(localIndex, nItems);
   }

   template <typename CppT>
   CppT *MapV(const ROOT::NTupleSize_t globalIndex, ROOT::NTupleSize_t &nItems)
   {
      if (R__unlikely(!fReadInfo->fReadPageRef.Get().Contains(globalIndex))) {
         MapPage(globalIndex);
      }
      // +1 to go from 0-based indexing to 1-based number of items
      nItems = fReadInfo->fReadPageRef.Get().GetGlobalRangeLast() - globalIndex + 1;
      return reinterpret_cast<CppT *>(
         static_cast<unsigned char *>(fReadInfo->fReadPageRef.Get().GetBuffer()) +
         (globalIndex - fReadInfo->fReadPageRef.Get().GetGlobalRangeFirst()) * sizeof(CppT));
   }

   template <typename CppT>
   CppT *MapV(RNTupleLocalIndex localIndex, ROOT::NTupleSize_t &nItems)
   {
      if (!fReadInfo->fReadPageRef.Get().Contains(localIndex)) {
         MapPage(localIndex);
      }
      // +1 to go from 0-based indexing to 1-based number of items
      nItems = fReadInfo->fReadPageRef.Get().GetLocalRangeLast() - localIndex.GetIndexInCluster() + 1;
      return reinterpret_cast<CppT *>(static_cast<unsigned char *>(
         fReadInfo->fReadPageRef.Get().GetBuffer()) +
         (localIndex.GetIndexInCluster() - fReadInfo->fReadPageRef.Get().GetLocalRangeFirst()) * sizeof(CppT));
   }

   ROOT::NTupleSize_t GetGlobalIndex(RNTupleLocalIndex clusterIndex)
   {
      if (!fReadInfo->fReadPageRef.Get().Contains(clusterIndex)) {
         MapPage(clusterIndex);
      }
      return fReadInfo->fReadPageRef.Get().GetClusterInfo().GetIndexOffset() + clusterIndex.GetIndexInCluster();
   }

   RNTupleLocalIndex GetClusterIndex(ROOT::NTupleSize_t globalIndex)
   {
      if (!fReadInfo->fReadPageRef.Get().Contains(globalIndex)) {
         MapPage(globalIndex);
      }
      return RNTupleLocalIndex(fReadInfo->fReadPageRef.Get().GetClusterInfo().GetId(),
                               globalIndex - fReadInfo->fReadPageRef.Get().GetClusterInfo().GetIndexOffset());
   }

   /// For offset columns only, look at the two adjacent values that define a collection's coordinates
   void GetCollectionInfo(const ROOT::NTupleSize_t globalIndex, RNTupleLocalIndex *collectionStart,
                          ROOT::NTupleSize_t *collectionSize)
   {
      ROOT::NTupleSize_t idxStart = 0;
      ROOT::NTupleSize_t idxEnd;
      // Try to avoid jumping back to the previous page and jumping back to the previous cluster
      if (R__likely(globalIndex > 0)) {
         if (R__likely(fReadInfo->fReadPageRef.Get().Contains(globalIndex - 1))) {
            idxStart = *Map<ROOT::Internal::RColumnIndex>(globalIndex - 1);
            idxEnd = *Map<ROOT::Internal::RColumnIndex>(globalIndex);
            if (R__unlikely(fReadInfo->fReadPageRef.Get().GetClusterInfo().GetIndexOffset() == globalIndex))
               idxStart = 0;
         } else {
            idxEnd = *Map<ROOT::Internal::RColumnIndex>(globalIndex);
            auto selfOffset = fReadInfo->fReadPageRef.Get().GetClusterInfo().GetIndexOffset();
            idxStart = (globalIndex == selfOffset) ? 0 : *Map<ROOT::Internal::RColumnIndex>(globalIndex - 1);
         }
      } else {
         idxEnd = *Map<ROOT::Internal::RColumnIndex>(globalIndex);
      }
      *collectionSize = idxEnd - idxStart;
      *collectionStart = RNTupleLocalIndex(fReadInfo->fReadPageRef.Get().GetClusterInfo().GetId(), idxStart);
   }

   void GetCollectionInfo(RNTupleLocalIndex localIndex, RNTupleLocalIndex *collectionStart,
                          ROOT::NTupleSize_t *collectionSize)
   {
      auto index = localIndex.GetIndexInCluster();
      auto idxStart = (index == 0) ? 0 : *Map<ROOT::Internal::RColumnIndex>(localIndex - 1);
      auto idxEnd = *Map<ROOT::Internal::RColumnIndex>(localIndex);
      *collectionSize = idxEnd - idxStart;
      *collectionStart = RNTupleLocalIndex(localIndex.GetClusterId(), idxStart);
   }

   /// Get the currently active cluster id
   void GetSwitchInfo(ROOT::NTupleSize_t globalIndex, RNTupleLocalIndex *varIndex, std::uint32_t *tag)
   {
      auto varSwitch = Map<ROOT::Internal::RColumnSwitch>(globalIndex);
      *varIndex = RNTupleLocalIndex(fReadInfo->fReadPageRef.Get().GetClusterInfo().GetId(), varSwitch->GetIndex());
      *tag = varSwitch->GetTag();
   }

   void Flush();
   void CommitSuppressed();

   void MapPage(ROOT::NTupleSize_t globalIndex) { R__ASSERT(TryMapPage(globalIndex)); }
   void MapPage(RNTupleLocalIndex localIndex) { R__ASSERT(TryMapPage(localIndex)); }
   bool TryMapPage(ROOT::NTupleSize_t globalIndex);
   bool TryMapPage(RNTupleLocalIndex localIndex);

   void MergeTeams(RColumn &other);

   ROOT::Internal::RColumnElementBase *GetElement() const { return fElement.get(); }
   ROOT::ENTupleColumnType GetType() const { return fType; }
   std::uint16_t GetBitsOnStorage() const { return static_cast<std::uint16_t>(fElement->GetBitsOnStorage()); }
   std::optional<std::pair<double, double>> GetValueRange() const { return fElement->GetValueRange(); }
   std::uint32_t GetIndex() const { return fIndex; }
   std::uint16_t GetRepresentationIndex() const { return fRepresentationIndex; }
   ROOT::DescriptorId_t GetOnDiskId() const { return fOnDiskId; }

   void SetBitsOnStorage(std::size_t bits) { fElement->SetBitsOnStorage(bits); }
   void SetValueRange(double min, double max) { fElement->SetValueRange(min, max); }

   std::size_t GetWritePageCapacity() const { return fWriteInfo->fWritePage.GetCapacity(); }
   ROOT::NTupleSize_t GetFirstElementIndex() const { return fWriteInfo->fFirstElementIndex; }
}; // class RColumn

} // namespace ROOT::Internal

#endif
