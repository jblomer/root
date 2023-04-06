// Author: Enrico Guiraud CERN 04/2023

/*************************************************************************
 * Copyright (C) 1995-2023, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_RDF_RBULKSTATICARRAYREADER
#define ROOT_RDF_RBULKSTATICARRAYREADER

#include <ROOT/RDF/RBulkReaderBase.hxx>
#include <ROOT/RVec.hxx>

namespace ROOT {
namespace Internal {
namespace RDF {

template <typename T>
class RBulkStaticArrayReader final : public RBulkReaderBase {
   ROOT::RVec<T> fCachedValues;

public:
   RBulkStaticArrayReader(TBranch &branch, std::size_t maxEventsPerBulk)
      : RBulkReaderBase(branch), fCachedValues(maxEventsPerBulk)
   {
   }

   void *LoadImpl(const Internal::RDF::RMaskedEntryRange &, std::size_t) final
   {
      // FIXME
      throw std::runtime_error("Bulk reads of static sized arrays are unimplemented in RDF.");
      return &fCachedValues[0];
   }
};

} // namespace RDF
} // namespace Internal
} // namespace ROOT
#endif

