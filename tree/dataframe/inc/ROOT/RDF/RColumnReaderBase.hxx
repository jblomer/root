// Author: Enrico Guiraud CERN 09/2020

/*************************************************************************
 * Copyright (C) 1995-2020, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_INTERNAL_RDF_RCOLUMNREADERBASE
#define ROOT_INTERNAL_RDF_RCOLUMNREADERBASE

#include <ROOT/RDF/RMaskedEntryRange.hxx>

#include <Rtypes.h>

namespace ROOT {
namespace Detail {
namespace RDF {

namespace RDFInternal = ROOT::Internal::RDF;

/**
\class ROOT::Internal::RDF::RColumnReaderBase
\ingroup dataframe
\brief Pure virtual base class for all column reader types

This pure virtual class provides a common base class for the different column reader types, e.g. RTreeColumnReader and
RDSColumnReader.
**/
class R__CLING_PTRCHECK(off) RColumnReaderBase {
public:
   virtual ~RColumnReaderBase() = default;

   /// Load the column values for the given bulk of entries.
   /// \param entry The entry number to load.
   /// \param mask The entry mask. Values will be loaded only for entries for which the mask equals true.
   /// \return A pointer to the beginning of a contiguous array of column values for the bulk.
   void *Load(const RDFInternal::RMaskedEntryRange &mask, std::size_t bulkSize)
   {
      return this->LoadImpl(mask, bulkSize);
   }

private:
   /// A type-erased version of Load(). To be implemented by concrete column readers.
   virtual void *LoadImpl(const Internal::RDF::RMaskedEntryRange &mask, std::size_t bulkSize) = 0;
};

} // namespace RDF
} // namespace Detail
} // namespace ROOT

#endif
