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
#include "TKey.h"

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


//------------------------------------------------------------------------------


ROOT::Experimental::RColumnSourceFile::RColumnSourceFile(TFile *file)
   : fNentries(0)
   , fFile(file)
   , fDirectory(nullptr)
   , fHeader(nullptr)
   , fIndex(nullptr)
{
}


ROOT::Experimental::RColumnSourceFile::~RColumnSourceFile()
{
}


void ROOT::Experimental::RColumnSourceFile::Attach(std::string_view name)
{
   fDirectory = fFile->GetDirectory(std::string(name).c_str());
   auto keyHeader = fDirectory->GetKey("RFH");
   fHeader = keyHeader->ReadObject<Internal::RFColumnFileHeader>();
   auto keyIndex = fDirectory->GetKey("RFI");
   fIndex = keyIndex->ReadObject<Internal::RFColumnFileIndex>();

   std::cout << "Opened Forest with " << fIndex->fNentries << " elements" << std::endl;
   fNentries = fIndex->fNentries;
   R__ASSERT(fHeader->fColumns.size() == fIndex->fColumnSlices.size());
   for (std::uint32_t i = 0; i < fHeader->fColumns.size(); ++i) {
      R__ASSERT(fHeader->fColumns[i].fId == fIndex->fColumnSlices[i].fId);
      R__ASSERT(fHeader->fColumns[i].fId == i);
      fName2Id[fHeader->fColumns[i].fName] = i;
      fId2IdxHeader[i] = i;
      fId2IdxIndex[i] = i;
      auto dummy = MakeColumnElement(static_cast<EColumnType>(fHeader->fColumns[i].fColumnType));
      fId2ElementSize[i] = dummy.GetSize();
      std::cout << "Found column " << fHeader->fColumns[i].fName << std::endl;
   }
}


void ROOT::Experimental::RColumnSourceFile::OnAddColumn(ROOT::Experimental::RColumn *column)
{
   std::string name = column->GetModel().GetName();
   std::size_t id = fName2Id[name];
   fColumn2Id[column] = id;
   std::cout << "Mapping column " << name << " to " << id << std::endl;
}


void ROOT::Experimental::RColumnSourceFile::OnMapSlice(
   ROOT::Experimental::RColumn *column,
   std::uint64_t num,
   ROOT::Experimental::RColumnSlice *slice)
{
   std::uint32_t id = fColumn2Id[column];
   std::size_t idxHeader = fId2IdxHeader[id];
   std::size_t idxIndex = fId2IdxIndex[id];

   //std::cout << "Map slice for element " << num << " of column "
   //          << fHeader->fColumns[idxHeader].fName << std::endl;

   auto nelems = fIndex->fColumnSlices[idxIndex].fNumElements;
   auto entryPoints = fIndex->fColumnSlices[idxIndex].GetElementStartRef();

   std::uint64_t first_in_slice = 0;
   std::uint64_t first_outside_slice = nelems;
   std::uint64_t sliceNum = 0;

   std::size_t i_lower = 0;
   std::size_t i_upper = entryPoints.size() - 1;
   R__ASSERT(i_lower <= i_upper);
   unsigned i_last = i_upper;
   while (i_lower <= i_upper) {
      std::size_t i_pivot = (i_lower + i_upper) / 2;
      std::uint64_t pivot = entryPoints[i_pivot];
      if (pivot > num) {
         i_upper = i_pivot - 1;
      } else {
         auto next = nelems;
         if (i_pivot < i_last) next = entryPoints[i_pivot + 1];
         if ((pivot == num) || (next > num)) {
            first_outside_slice = next;
            first_in_slice = pivot;
            sliceNum = i_pivot;
            break;
         } else {
            i_lower = i_pivot + 1;
         }
      }
   }

   std::uint64_t elements_in_slice = first_outside_slice - first_in_slice;
   std::uint64_t slice_mem_size = elements_in_slice * fId2ElementSize[id];

   std::ostringstream sliceName;
   sliceName << "RFS_" << id << "_" << sliceNum;
   TKey* key = fDirectory->GetKey(sliceName.str().c_str());
   //std::cout << "  --> serialize " << sliceName.str() << " - " << (void *)key << std::endl;
   const auto sliceObj = key->ReadObject<Internal::RFSliceBuffer>();

   slice->Reset(first_in_slice);
   assert(slice->GetCapacity() >= slice_mem_size);
   slice->Reserve(slice_mem_size);

   if (fHeader->fColumns[idxHeader].fCompressionSettings == 0) {
      memcpy(slice->GetBuffer(), sliceObj->buf.data(), slice_mem_size);
   } else {
      //R__ASSERT(static_cast<int64_t>(sliceObj.buf.size()) <= fZipBufferSize);
      //Read(fZipBuffer, slice_disk_size);
      //int irep;
      //int srcsize = slice_disk_size;
      //int tgtsize = slice_mem_size;
      //R__unzip(&srcsize, fZipBuffer, &tgtsize, static_cast<unsigned char *>(slice->GetBuffer()), &irep);
      //R__ASSERT(uint64_t(irep) == slice_mem_size);
   }

   delete sliceObj;
}


std::uint64_t ROOT::Experimental::RColumnSourceFile::GetNElements(ROOT::Experimental::RColumn *column)
{
   std::uint32_t id = fColumn2Id[column];
   std::size_t idxIndex = fId2IdxIndex[id];
   std::uint64_t nElems = fIndex->fColumnSlices[idxIndex].fNumElements;
   std::cout << "Column " << id << " has " << nElems << " elements" << std::endl;
   return nElems;
}


const ROOT::Experimental::RColumnSource::ColumnList_t& ROOT::Experimental::RColumnSourceFile::ListColumns()
{
   // TODO
   return fColumnList;
}


const ROOT::Experimental::RColumnSource::ClusterList_t &ROOT::Experimental::RColumnSourceFile::ListClusters()
{
   // TODO
   return fClusterList;
}


std::unique_ptr<ROOT::Experimental::RColumnSource> ROOT::Experimental::RColumnSourceFile::Clone()
{
   // TODO
   return nullptr;
}

