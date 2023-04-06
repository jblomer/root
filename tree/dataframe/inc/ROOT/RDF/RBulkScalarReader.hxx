// Author: Enrico Guiraud CERN 04/2023

/*************************************************************************
 * Copyright (C) 1995-2023, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_RDF_RBULKSCALARREADER
#define ROOT_RDF_RBULKSCALARREADER

#include <ROOT/RDF/RBulkReaderBase.hxx>
#include <ROOT/RVec.hxx>

#include <cassert>

namespace ROOT {
namespace Internal {
namespace RDF {

template <typename T>
class RBulkScalarReader final : public RBulkReaderBase {
   ROOT::RVec<T> fCachedValues;

   // Copy N entries starting from fBuf to fCachedValues, indexing them using bufOffset and cacheOffset respectively.
   // Expects fBuf to have enough values available and fCachedValues to have enough room for them.
   void LoadN(std::size_t N, std::size_t cacheOffset, std::size_t bufOffset)
   {
      T *data = reinterpret_cast<T *>(fBuf.GetCurrent());

      for (std::size_t i = 0u; i < N; ++i)
         fCachedValues[i + cacheOffset] = data[i + bufOffset];
   }

public:
   RBulkScalarReader(TBranch &branch, std::size_t maxEventsPerBulk)
      : RBulkReaderBase(branch), fCachedValues(maxEventsPerBulk)
   {
   }

   void *LoadImpl(const Internal::RDF::RMaskedEntryRange &requestedMask, std::size_t bulkSize) final
   {
      if (requestedMask.FirstEntry() == fLoadedEntriesBegin)
         return &fCachedValues[0]; // the requested bulk is already loaded, nothing to do

      fLoadedEntriesBegin = requestedMask.FirstEntry();
      std::size_t nLoaded = 0u;

      if (fBulkSize > 0u) { // we have leftover values in the bulk from the previous call to LoadImpl
         const std::size_t nAvailable = (fBulkBegin + fBulkSize) - fLoadedEntriesBegin;
         const auto nToLoad = std::min(nAvailable, bulkSize);
         LoadN(nToLoad, 0u, fLoadedEntriesBegin - fBulkBegin);
         nLoaded += nToLoad;
      }

      while (nLoaded < bulkSize) {
         // assert we either have not loaded a bulk yet or we exhausted the last branch bulk
         assert(fBulkSize == 0u || fBulkBegin + fBulkSize == fLoadedEntriesBegin + nLoaded);
         fBulkBegin = fLoadedEntriesBegin + nLoaded;
         const auto ret = fBranch->GetBulkRead().GetBulkEntries(fBulkBegin, fBuf);
         if (ret == -1)
            throw std::runtime_error("Could not load branch values. This should never happen.");
         fBulkSize = ret;
         const auto nToLoad = std::min(fBulkSize, bulkSize - nLoaded);
         LoadN(nToLoad, /*cacheOffset*/nLoaded, /*bufOffset*/0u);
         nLoaded += nToLoad;
      }

      return &fCachedValues[0];
   }
};

} // namespace RDF
} // namespace Internal
} // namespace ROOT
#endif
