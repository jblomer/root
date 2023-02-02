// Author: Enrico Guiraud CERN 09/2020

/*************************************************************************
 * Copyright (C) 1995-2020, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_RDF_RDEFINEREADER
#define ROOT_RDF_RDEFINEREADER

#include "RColumnReaderBase.hxx"
#include "RDefineBase.hxx"
#include <Rtypes.h>  // Long64_t, R__CLING_PTRCHECK

#include <limits>
#include <type_traits>

namespace ROOT {
namespace Internal {
namespace RDF {

namespace RDFDetail = ROOT::Detail::RDF;

/// Column reader for defined columns.
class R__CLING_PTRCHECK(off) RDefineReader final : public ROOT::Detail::RDF::RColumnReaderBase {
   /// Non-owning reference to the node responsible for the defined column.
   RDFDetail::RDefineBase &fDefine;

   /// Non-owning ptr to the beginning of a contiguous array of values.
   void *fValuePtr = nullptr;

   /// The slot this value belongs to.
   unsigned int fSlot = std::numeric_limits<unsigned int>::max();

   /// Step size (in bytes) between one value and the next in a bulk.
   std::size_t fValueStep;

   void *GetImpl(std::size_t offset) final { return static_cast<char *>(fValuePtr) + offset*fValueStep; }

   void LoadImpl(const RMaskedEntryRange &mask, std::size_t bulkSize) final { fDefine.Update(fSlot, mask, bulkSize); }

public:
   RDefineReader(unsigned int slot, RDFDetail::RDefineBase &define)
      : fDefine(define), fValuePtr(define.GetValuePtr(slot)), fSlot(slot),
        fValueStep(fDefine.IsDefinePerSample() ? 0 : fDefine.GetTypeSize())
   {
   }
};

}
}
}

#endif
