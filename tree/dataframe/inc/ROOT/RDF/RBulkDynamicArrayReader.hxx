// Author: Enrico Guiraud CERN 04/2023

/*************************************************************************
 * Copyright (C) 1995-2023, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_RDF_RBULKDYNAMICARRAYREADER
#define ROOT_RDF_RBULKDYNAMICARRAYREADER

#include <ROOT/RDF/RBulkReaderBase.hxx>
#include <ROOT/RVec.hxx>

namespace ROOT {
namespace Internal {
namespace RDF {

template <typename ColType, typename SizeType>
class RBulkDynamicArrayReader final : public RBulkReaderBase {
   using Element_t = typename ColType::value_type;
   ROOT::RVec<Element_t> fCachedFlattenedElements;
   ROOT::RVec<ROOT::RVec<Element_t>> fCachedArrays;
   RColumnReaderBase *fSizeReader; ///< Pointer to the column reader for the size of the dynamic arrays. Never null.
   /// Index of the element in the branch bulk after the last one we already read.
   std::size_t fBulkElementsOffset = 0u;

   // Copy N entries starting from fBuf to fCachedValues.
   // Expects fBuf to have enough values available and fCachedValues to have enough room for them.
   std::size_t LoadN(std::size_t N, std::size_t arrayOffset, std::size_t elementsOffset, SizeType *sizes)
   {
      // work on raw pointers to side-step RVec::operator[] which is slightly more costly than simple pointer arithmetic
      Element_t *elementsCache = &fCachedFlattenedElements[0];
      ROOT::RVec<Element_t> *arrayCache = &fCachedArrays[0];

      char *data = reinterpret_cast<char *>(fBuf.GetCurrent());
      data += fBulkElementsOffset * sizeof(Element_t); // advance to the first element we are interested in

      // copy the new elements in fCachedFlattenedElements, make the RVecs in fCachedArrays point to those values
      for (std::size_t i = 0u; i < N; ++i) {
         const std::size_t size = sizes[arrayOffset + i];
         for (std::size_t j = 0u; j < size; ++j)
            frombuf(data, elementsCache + elementsOffset + j); // `frombuf` also advances the `data` pointer
         ROOT::Internal::VecOps::ResetView(*(arrayCache + arrayOffset + i), elementsCache + elementsOffset, size);
         elementsOffset += size;
      }

      return elementsOffset;
   }

public:
   RBulkDynamicArrayReader(TBranch &branch, RColumnReaderBase &sizeReader, std::size_t maxEventsPerBulk)
      : RBulkReaderBase(branch), fCachedArrays(maxEventsPerBulk), fSizeReader(&sizeReader)
   {
      static_assert(VecOps::IsRVec<ColType>::value,
                    "Something went wrong, RBulkDynamicArrayReader should only be used for RVec columns.");

      // Put all fCachedArrays in non-owning mode (aka view mode), so later we can just call ResetView on them.
      fCachedFlattenedElements.resize(1ull);
      for (auto &vec : fCachedArrays) {
         ROOT::RVec<Element_t> tmp(&fCachedFlattenedElements[0], 1u);
         std::swap(vec, tmp);
      }
   }

   void *LoadImpl(const Internal::RDF::RMaskedEntryRange &requestedMask, std::size_t bulkSize) final
   {
      if (requestedMask.FirstEntry() == fLoadedEntriesBegin)
         return &fCachedArrays[0]; // the requested bulk is already loaded, nothing to do

      // Load array sizes.
      // We actually need all values, not just the ones from the `requestedMask`, because for branches that support bulk
      // reading we always read all values ignoring the mask. The assumption here is that size leafs of bulk branches
      // always support bulk themselves, so we can just pass the same `requestedMask` here and it will be ignored,
      // just like we are going to ignore it here.
      auto *sizes = static_cast<SizeType *>(fSizeReader->Load(requestedMask, bulkSize));

      // Make sure we have enough space in the array elements cache
      const auto nTotalElements = std::accumulate(sizes, sizes + bulkSize, std::size_t(0u));
      fCachedFlattenedElements.resize(nTotalElements);

      fLoadedEntriesBegin = requestedMask.FirstEntry();
      std::size_t nLoaded = 0u;
      std::size_t elementsOffset = 0u; // how many array elements we have read for this bulk

      if (fBulkSize > 0u) { // we have leftover values in the bulk from the previous call to LoadImpl
         const std::size_t nAvailable = (fBulkBegin + fBulkSize) - fLoadedEntriesBegin;
         const auto nToLoad = std::min(nAvailable, bulkSize);
         elementsOffset = LoadN(nToLoad, /*arrayOffset*/ 0u, /*elementsOffset*/ 0u, sizes);
         fBulkElementsOffset += elementsOffset;
         nLoaded += nToLoad;
      }

      while (nLoaded < bulkSize) {
         // assert we either have not loaded a bulk yet or we exhausted the last branch bulk
         assert(fBulkSize == 0u || fBulkBegin + fBulkSize == fLoadedEntriesBegin + nLoaded);

         // read in new branch bulk
         fBulkBegin = fLoadedEntriesBegin + nLoaded;
         fBulkElementsOffset = 0u;
         const auto ret = fBranch->GetBulkRead().GetEntriesSerialized(fBulkBegin, fBuf);
         if (ret == -1)
            throw std::runtime_error("Could not load branch values. This should never happen.");
         fBulkSize = ret;

         const auto nToLoad = std::min(fBulkSize, bulkSize - nLoaded);
         const auto newElementsOffset = LoadN(nToLoad, /*arrayOffset*/ nLoaded, elementsOffset, sizes);
         fBulkElementsOffset = newElementsOffset - elementsOffset;
         elementsOffset = newElementsOffset;
         nLoaded += nToLoad;
      }

      return &fCachedArrays[0];
   }
};

} // namespace RDF
} // namespace Internal
} // namespace ROOT
#endif
