// Author: Enrico Guiraud CERN 04/2023

/*************************************************************************
 * Copyright (C) 1995-2023, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_RDF_RBULKREADERBASE
#define ROOT_RDF_RBULKREADERBASE

#include <ROOT/RDF/RColumnReaderBase.hxx>
#include <ROOT/RDF/RMaskedEntryRange.hxx>

#include <Rtypes.h>
#include <TBranch.h>
#include <TBufferFile.h>

namespace ROOT {
namespace Internal {
namespace RDF {

class RBulkReaderBase : public ROOT::Detail::RDF::RColumnReaderBase {
protected:
   TBufferFile fBuf;                     ///< Buffer into which we will load branch values.
   TBranch *fBranch;                     ///< Branch to load values from. Never null.
   ULong64_t fBulkBegin = 0ull;          ///< Event number of first entry loaded in fBuf.
   std::size_t fBulkSize = 0u;           ///< Number of entries loaded in fBuf.
   Long64_t fLoadedEntriesBegin = -1ull; ///< The entry number of the last RDF bulk we loaded.

public:
   RBulkReaderBase(TBranch &branch) : fBuf(TBuffer::kWrite, 10000), fBranch(&branch) {}
};

} // namespace RDF
} // namespace Internal
} // namespace ROOT
#endif
