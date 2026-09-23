/// \file RColumn.cxx
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2018-10-04

/*************************************************************************
 * Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <ROOT/RColumn.hxx>
#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RPageStorage.hxx>

#include <TError.h>

#include <algorithm>
#include <cassert>
#include <utility>

using ROOT::Internal::RPageSink;
using ROOT::Internal::RPageSource;

ROOT::Internal::RColumn::RColumn(ENTupleColumnType type, std::uint32_t columnIndex, std::uint16_t representationIndex)
   : fType(type), fIndex(columnIndex), fRepresentationIndex(representationIndex)
{
}

ROOT::Internal::RColumn::~RColumn()
{
   if (fWriteInfo)
      fWriteInfo->fPageSink->DropColumn(MakeHandle());
   if (fReadInfo)
      fReadInfo->fPageSource->DropColumn(MakeHandle());
}

void ROOT::Internal::RColumn::ConnectPageSink(ROOT::DescriptorId_t fieldId, RPageSink &pageSink,
                                              ROOT::NTupleSize_t firstElementIndex)
{
   fWriteInfo = std::make_unique<RWriteInfo>();

   fWriteInfo->fInitialNElements = pageSink.GetWriteOptions().GetInitialUnzippedPageSize() / fElement->GetSize();
   if (fWriteInfo->fInitialNElements < 1) {
      throw RException(R__FAIL("initial page size is too small for at least one element"));
   }

   fWriteInfo->fPageSink = &pageSink;
   fWriteInfo->fFirstElementIndex = firstElementIndex;
   fOnDiskId = fWriteInfo->fPageSink->AddColumn(fieldId, *this).fPhysicalId;
   fWriteInfo->fWritePage = fWriteInfo->fPageSink->ReservePage(MakeHandle(), fWriteInfo->fInitialNElements);
   if (fWriteInfo->fWritePage.IsNull())
      throw RException(R__FAIL("page buffer memory budget too small"));
}

void ROOT::Internal::RColumn::ConnectPageSource(ROOT::DescriptorId_t fieldId, RPageSource &pageSource)
{
   fReadInfo = std::make_unique<RReadInfo>();
   fReadInfo->fTeam.emplace_back(this);
   fReadInfo->fPageSource = &pageSource;
   fOnDiskId = fReadInfo->fPageSource->AddColumn(fieldId, *this).fPhysicalId;
}

void ROOT::Internal::RColumn::Flush()
{
   if (fWriteInfo->fWritePage.GetNElements() == 0)
      return;

   fWriteInfo->fPageSink->CommitPage(MakeHandle(), fWriteInfo->fWritePage);
   fWriteInfo->fWritePage = fWriteInfo->fPageSink->ReservePage(MakeHandle(), fWriteInfo->fInitialNElements);
   R__ASSERT(!fWriteInfo->fWritePage.IsNull());
   fWriteInfo->fWritePage.Reset(fWriteInfo->fNElements);
}

void ROOT::Internal::RColumn::CommitSuppressed()
{
   fWriteInfo->fPageSink->CommitSuppressedColumn(MakeHandle());
}

bool ROOT::Internal::RColumn::TryMapPage(ROOT::NTupleSize_t globalIndex)
{
   const auto nTeam = fReadInfo->fTeam.size();
   std::size_t iTeam = 1;
   do {
      fReadInfo->fReadPageRef = fReadInfo->fPageSource->LoadPage(
         fReadInfo->fTeam.at(fReadInfo->fLastGoodTeamIdx)->MakeHandle(), globalIndex);
      if (!fReadInfo->fReadPageRef.Get().IsNull())
         break;
      fReadInfo->fLastGoodTeamIdx = (fReadInfo->fLastGoodTeamIdx + 1) % nTeam;
      iTeam++;
   } while (iTeam <= nTeam);

   return fReadInfo->fReadPageRef.Get().Contains(globalIndex);
}

bool ROOT::Internal::RColumn::TryMapPage(RNTupleLocalIndex localIndex)
{
   const auto nTeam = fReadInfo->fTeam.size();
   std::size_t iTeam = 1;
   do {
      fReadInfo->fReadPageRef = fReadInfo->fPageSource->LoadPage(
         fReadInfo->fTeam.at(fReadInfo->fLastGoodTeamIdx)->MakeHandle(), localIndex);
      if (!fReadInfo->fReadPageRef.Get().IsNull())
         break;
      fReadInfo->fLastGoodTeamIdx = (fReadInfo->fLastGoodTeamIdx + 1) % nTeam;
      iTeam++;
   } while (iTeam <= nTeam);

   return fReadInfo->fReadPageRef.Get().Contains(localIndex);
}

void ROOT::Internal::RColumn::MergeTeams(RColumn &other)
{
   // We are working on very small vectors here, so quadratic complexity works
   for (auto *c : other.fReadInfo->fTeam) {
      if (std::find(fReadInfo->fTeam.begin(), fReadInfo->fTeam.end(), c) == fReadInfo->fTeam.end())
         fReadInfo->fTeam.emplace_back(c);
   }

   for (auto c : fReadInfo->fTeam) {
      if (c == this)
         continue;
      c->fReadInfo->fTeam = fReadInfo->fTeam;
   }
}
