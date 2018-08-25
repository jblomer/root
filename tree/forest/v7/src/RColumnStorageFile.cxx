/// \file RColumnStorageFile.cxx
/// \ingroup Forest ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2018-07-19
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2015, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "ROOT/RColumnStorageFile.hxx"

#include "ROOT/RColumn.hxx"

#include "RZip.h"
#include "TError.h"

#include <sstream>


ROOT::Experimental::RColumnSinkFile::RColumnSinkFile(const ROOT::Experimental::RColumnFileSettings &settings)
   : fSettings(settings)
   , fZipBufferSize(1024 * 1024)
   , fZipBuffer(new char[fZipBufferSize])
   , fDirectory(nullptr)
{
}


ROOT::Experimental::RColumnSinkFile::~RColumnSinkFile()
{
}


void ROOT::Experimental::RColumnSinkFile::OnCreate(std::string_view name) {
   std::cout << "create" << std::endl;
   fDirectory = fSettings.fFile->mkdir(std::string(name).c_str());
   fDirectory->WriteObject(&fHeader, "RFH");
}


void ROOT::Experimental::RColumnSinkFile::OnAddColumn(RColumn *column)
{
   uint32_t id = fHeader.fColumns.size();
   fColumn2Id[column] = id;
   Internal::RFColumnFileDesc columnDesc;
   columnDesc.fId = id;
   columnDesc.fColumnType = static_cast<int>(column->GetModel().GetType());
   columnDesc.fCompressionSettings = fSettings.fCompressionSettings;
   columnDesc.fName = column->GetModel().GetName();
   fHeader.fColumns.emplace_back(columnDesc);

   Internal::RFColumnFileSlices columnSlices;
   columnSlices.fId = id;
   columnSlices.fNumElements = 0;
   fIndex.fColumnSlices.emplace_back(columnSlices);
}


void ROOT::Experimental::RColumnSinkFile::OnCommitSlice(RColumnSlice *slice, RColumn *column)
{
   std::uint32_t colId = fColumn2Id.find(column)->second;
   std::uint64_t sizeInMem = slice->GetSize();
   std::uint64_t elementsInSlice = sizeInMem / column->GetModel().GetElementSize();

   std::uint64_t sliceNum = fIndex.fColumnSlices[colId].fElementStart.size();
   fIndex.fColumnSlices[colId].fElementStart.push_back(slice->GetRangeStart());
   fIndex.fColumnSlices[colId].fNumElements += elementsInSlice;

   int sizeOnDisk = 0;
   const UChar_t* physicalBuffer;
   if (fHeader.fColumns[colId].fCompressionSettings > 0) {
      int dataSize = sizeInMem;
      int zipBufferSize = fZipBufferSize;
      int zipSize = 0;
      R__zipMultipleAlgorithm(
         fHeader.fColumns[colId].fCompressionSettings % 100,
         &dataSize, (char *)slice->GetBuffer(),
         &zipBufferSize, fZipBuffer, &zipSize,
         static_cast<ECompressionAlgorithm>(fHeader.fColumns[colId].fCompressionSettings / 100));
      //std::cout << "COMPRESSED " << size << " TO " << zipSize << std::endl;

      R__ASSERT(zipSize > 0);
      sizeOnDisk = zipSize;
      physicalBuffer = (const UChar_t *)fZipBuffer;
   } else {
      sizeOnDisk = sizeInMem;
      physicalBuffer = (const UChar_t *)(slice->GetBuffer());
   }

   std::ostringstream sliceName;
   sliceName << "RFS_" << colId << "_" << sliceNum;
   //std::cout << "committing slice " << sliceName.str() << " size "
   //          << sizeOnDisk << "  " << (long)physicalBuffer << std::endl;

   //Internal::RFSliceBuffer sliceBuffer(sizeOnDisk/4, (float *)physicalBuffer);
   Internal::RFSliceBuffer sliceBuffer;
   sliceBuffer.buf.resize(sizeOnDisk);
   memcpy(sliceBuffer.buf.data(), physicalBuffer, sizeOnDisk);
   fDirectory->WriteObject<ROOT::Experimental::Internal::RFSliceBuffer>(
      &sliceBuffer, sliceName.str().c_str());
}


void ROOT::Experimental::RColumnSinkFile::OnCommitCluster(OffsetColumn_t nentries)
{
   std::cout << "committing cluster, " << nentries << " TODO" << std::endl;
}


void ROOT::Experimental::RColumnSinkFile::OnCommitDataset(OffsetColumn_t nentries)
{
   std::cout << "committing data set" << std::endl;
   fIndex.fNentries = nentries;
   fDirectory->WriteObject(&fIndex, "RFI");
   fSettings.fFile->Close();
}

