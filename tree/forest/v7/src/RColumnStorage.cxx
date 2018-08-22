/// \file RColumnStorage.cxx
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

#include "ROOT/RColumn.hxx"
#include "ROOT/RColumnStorage.hxx"
#include "ROOT/RColumnSlice.hxx"

#include "RZip.h"
#include "TError.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <string>
#include <sstream>
#include <utility>

#define DEBUGON 0
#define DEBUGMSG(x) if (DEBUGON) { x }


void ROOT::Experimental::Detail::RColumnRawStats::Print() {
   std::cout
      << "---------------------- STATISTICS ------------------------" << std::endl
      << "Padding: " << fBPadding << " / " << fNPadding << std::endl
      << "Mini-Footer: " << fBMiniFooter << " / " << fNMiniFooter << std::endl
      << "Footer: " << fBFooter << std::endl
      << "Header: " << fBHeader << std::endl
      << "Slices: " << fBSliceDisk << " >> " << fBSliceMem << " / " << fNSlice << std::endl
      << "Total: " << fBTotal << std::endl
      << "----------------------------------------------------------" << std::endl;
}

std::unique_ptr<ROOT::Experimental::RColumnSinkRaw> ROOT::Experimental::RColumnSink::MakeSinkRaw(
  std::string_view path)
{
   return std::move(std::make_unique<ROOT::Experimental::RColumnSinkRaw>(
     ROOT::Experimental::RColumnRawSettings(path)));
}


std::unique_ptr<ROOT::Experimental::RColumnSinkRaw> ROOT::Experimental::RColumnSink::MakeSinkRaw(
  const ROOT::Experimental::RColumnRawSettings &settings)
{
   return std::move(std::make_unique<ROOT::Experimental::RColumnSinkRaw>(settings));
}


ROOT::Experimental::RColumnSinkRaw::RColumnSinkRaw(const RColumnRawSettings &settings)
   : fPath(settings.fPath)
   , fEpochSize(settings.fEpochSize)
   , fCompressionSettings(settings.fCompressionSettings)
   , fd(open(fPath.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600))
   , fZipBufferSize(1024 * 1024)
   , fZipBuffer(new char[fZipBufferSize])
{
}


ROOT::Experimental::RColumnSinkRaw::~RColumnSinkRaw()
{
}


void ROOT::Experimental::RColumnSinkRaw::OnCommitCluster(OffsetColumn_t nentries)
{
   // Only records the entry number, the user of RColumnSink has to make sure
   // slices are flushed accordingly
   fClusters.emplace_back(nentries, fFilePos);
}


void ROOT::Experimental::RColumnSinkRaw::OnCommitDataset(OffsetColumn_t nentries)
{
   if (fClusters.empty() || (fClusters[fClusters.size() - 1].first < nentries)) {
     fClusters.emplace_back(nentries, fFilePos);
   }
   WriteFooter(nentries);
   close(fd);
}


void ROOT::Experimental::RColumnSinkRaw::OnCreate()
{
   fFilePos = 0;
   std::cout << "WRITING HEADER" << std::endl;
   std::uint32_t num_cols = fGlobalIndex.size();
   Write(&num_cols, sizeof(num_cols));
   for (auto& iter_col : fGlobalIndex) {
      Write(&(iter_col.second->fId), sizeof(iter_col.second->fId));
      EColumnType type = iter_col.first->GetModel().GetType();
      Write(&type, sizeof(type));
      std::size_t element_size = iter_col.first->GetModel().GetElementSize();
      Write(&element_size, sizeof(element_size));
      //std::uint32_t compression_settings = iter_col.first->GetModel().GetCompressionSettings();
      std::uint32_t compression_settings = fCompressionSettings;
      Write(&compression_settings, sizeof(compression_settings));
      std::string name = iter_col.first->GetModel().GetName();
      std::uint32_t name_len = name.length();
      Write(&name_len, sizeof(name_len));
      Write(name.data(), name.length());
   }
   fStats.fBHeader = fFilePos;
}


void ROOT::Experimental::RColumnSinkRaw::OnAddColumn(ROOT::Experimental::RColumn *column)
{
   std::uint32_t id = fGlobalIndex.size();
   fGlobalIndex[column] = std::make_unique<RColumnIndex>(id);
   fEpochIndex[column] = std::make_unique<RColumnIndex>(id);
}


void ROOT::Experimental::RColumnSinkRaw::OnCommitSlice(RColumnSlice *slice, RColumn *column)
{
   std::size_t size = slice->GetSize();
   R__ASSERT(size > 0);
   std::size_t num_elements = size / column->GetModel().GetElementSize();
   std::size_t epoch = fFilePos / fEpochSize;
   if (((fFilePos + size) / fEpochSize) > epoch) {
      size_t padding = (epoch + 1) * fEpochSize - fFilePos;
      WritePadding(padding);
      WriteMiniFooter();
   }

   //std::cout << "adding " << num_elements << " elements"
   //          << ", basket size " << size << std::endl;
   //if (column->GetColumnType() == RTreeColumnType::kOffset) {
   //  std::cout << "OFFSET BASKET " << column->GetName()
   //    << "  1st element " << *(uint64_t *)(basket->GetBuffer())
   //    << std::endl;
   //} else {
   //  std::cout << "FLOAT BASKET " << column->GetName()
   //     << "  1st element " << *(float *)(basket->GetBuffer())
   //     << std::endl;
   //}

   std::uint32_t sliceSize;
   //if (column->GetModel().GetCompressionSettings() > 0) {
   if (fCompressionSettings > 0) {
      int dataSize = size;
      int zipBufferSize = fZipBufferSize;
      int zipSize = 0;
      R__zipMultipleAlgorithm(
         fCompressionSettings % 100,
         &dataSize, (char *)slice->GetBuffer(),
         &zipBufferSize, fZipBuffer, &zipSize,
         static_cast<ECompressionAlgorithm>(fCompressionSettings / 100));
      //std::cout << "COMPRESSED " << size << " TO " << zipSize << std::endl;

      ssize_t retval = write(fd, fZipBuffer, zipSize);
      R__ASSERT(retval == zipSize);
      R__ASSERT(zipSize > 0);
      sliceSize = zipSize;
   } else {
      write(fd, slice->GetBuffer(), size);
      sliceSize = size;
   }

   fStats.fNSlice += 1;
   fStats.fBSliceMem += size;
   fStats.fBSliceDisk += sliceSize;
   fStats.fBTotal += sliceSize;

   std::pair<uint64_t, Internal::RSliceInfo> entry(slice->GetRangeStart(), Internal::RSliceInfo(fFilePos, sliceSize));
   auto iter_global_index = fGlobalIndex.find(column);
   auto iter_epoch_index = fEpochIndex.find(column);
   iter_global_index->second->fSliceHeads.push_back(entry);
   iter_epoch_index->second->fSliceHeads.push_back(entry);

   iter_global_index->second->fNumElements += num_elements;
   iter_epoch_index->second->fNumElements += num_elements;

   fFilePos += sliceSize;
}


void ROOT::Experimental::RColumnSinkRaw::Write(const void *buf, std::size_t size)
{
   fStats.fBTotal += size;
   ssize_t retval = write(fd, buf, size);
   R__ASSERT(retval >= 0);
   R__ASSERT(size_t(retval) == size);
   fFilePos += size;
}


void ROOT::Experimental::RColumnSinkRaw::WriteClusters() {
   std::uint64_t nClusters = fClusters.size();
   Write(&nClusters, sizeof(nClusters));
   for (unsigned i = 0; i < nClusters; ++i) {
      Write(&(fClusters[i]), sizeof(fClusters[i]));
   }
}


void ROOT::Experimental::RColumnSinkRaw::WriteFooter(std::uint64_t nentries)
{
   std::cout << "WRITING FOOTER" << std::endl;
   size_t footer_pos = fFilePos;
   Write(&nentries, sizeof(nentries));
   for (auto& iter_col : fGlobalIndex) {
      Write(&(iter_col.second->fId), sizeof(iter_col.second->fId));
      Write(&(iter_col.second->fNumElements),
            sizeof(iter_col.second->fNumElements));
      uint32_t nslices = iter_col.second->fSliceHeads.size();
      Write(&nslices, sizeof(nslices));
      if (nslices > 0) {
         Write(iter_col.second->fSliceHeads.data(),
               nslices * sizeof(std::pair<uint64_t, Internal::RSliceInfo>));
      }
      iter_col.second->fSliceHeads.clear();
   }
   WriteClusters();
   fStats.fBFooter = fFilePos - footer_pos;
   Write(&fStats, sizeof(fStats));
   Write(&footer_pos, sizeof(footer_pos));
}


void ROOT::Experimental::RColumnSinkRaw::WritePadding(std::size_t padding)
{
   fStats.fBPadding += padding;
   fStats.fNPadding += 1;
   std::array<unsigned char, 4096> zeros;
   size_t written = 0;
   while (written < padding) {
      size_t to_write = std::min(padding - written, std::size_t(4096));
      write(fd, zeros.data(), to_write);
      written += to_write;
   }
   fFilePos += padding;
}


void ROOT::Experimental::RColumnSinkRaw::WriteMiniFooter()
{
   std::cout << "Write Mini Footer at " << fFilePos << std::endl;
   std::uint64_t fPos = fFilePos;
   for (auto& iter_col : fEpochIndex) {
      Write(&(iter_col.second->fId), sizeof(iter_col.second->fId));
      Write(&(iter_col.second->fNumElements),
            sizeof(iter_col.second->fNumElements));
      uint32_t nslices = iter_col.second->fSliceHeads.size();
      Write(&nslices, sizeof(nslices));
      if (nslices > 0) {
         Write(iter_col.second->fSliceHeads.data(),
               nslices * sizeof(std::pair<uint64_t, uint64_t>));
      }
      iter_col.second->fSliceHeads.clear();
   }
   WriteClusters();
   fStats.fBMiniFooter += fFilePos - fPos;
   fStats.fNMiniFooter += 1;
}


//------------------------------------------------------------------------------


std::unique_ptr<ROOT::Experimental::RColumnSourceRaw> ROOT::Experimental::RColumnSource::MakeSourceRaw(
  std::string_view path)
{
   return std::move(std::make_unique<ROOT::Experimental::RColumnSourceRaw>(path));
}


void ROOT::Experimental::RColumnSourceRaw::OnAddColumn(ROOT::Experimental::RColumn *column) {
   auto iter = fColumnIds.find(column->GetModel().GetName());
   if (iter == fColumnIds.end()) throw std::string("not found");
   std::uint32_t column_id = iter->second;
   DEBUGMSG(std::cout << "Found column " << column->GetModel().GetName()
                      << " under id " << column_id << std::endl;)
   fLiveColumns[column] = column_id;
}


void ROOT::Experimental::RColumnSourceRaw::OnMapSlice(
   RColumn *column,
   std::uint64_t num,
   RColumnSlice *slice)
{
   auto iter = fLiveColumns.find(column);
   if (iter == fLiveColumns.end()) throw "not found";
   std::uint32_t column_id = iter->second;
   //std::cout << "OnMapSlice column " << column_id
   //  << " #" << num << std::endl;
   std::uint64_t nelems = fColumnElements[column_id];

   std::uint64_t file_pos = 0;
   std::uint64_t first_in_slice = 0;
   std::uint64_t first_outside_slice = fColumnElements[column_id];
   std::uint64_t slice_disk_size = 0;

   Index_t *idx = fIndex[column_id].get();
   unsigned i_lower = 0;
   unsigned i_upper = idx->size() - 1;
   R__ASSERT(i_lower <= i_upper);
   unsigned i_last = i_upper;
   while (i_lower <= i_upper) {
      unsigned i_pivot = (i_lower + i_upper) / 2;
      std::uint64_t pivot = (*idx)[i_pivot].first;
      if (pivot > num) {
         i_upper = i_pivot - 1;
      } else {
         std::uint64_t next = nelems;
         if (i_pivot < i_last) next = (*idx)[i_pivot + 1].first;
         if ((pivot == num) || (next > num)) {
            first_outside_slice = next;
            first_in_slice = pivot;
            file_pos = (*idx)[i_pivot].second.fFilePos;
            slice_disk_size = (*idx)[i_pivot].second.fSize;
            break;
         } else {
            i_lower = i_pivot + 1;
         }
      }
   }

   std::uint64_t elements_in_slice = first_outside_slice - first_in_slice;
   std::uint64_t slice_mem_size = elements_in_slice * fColumnElementSizes[column_id];

   //  std::cout << "Basket has disk size " << slice_disk_size << " and " <<
   //               elements_in_slice << " elements" << std::endl;
   //  std::cout << "Mapping slice for element number " << num
   //            << " for column id " << column_id << std::endl;

   slice->Reset(first_in_slice);
   assert(slice->GetCapacity() >= slice_mem_size);
   slice->Reserve(slice_mem_size);
   Seek(file_pos);
   if (fColumnCompressionSettings[column_id] == 0) {
      Read(slice->GetBuffer(), slice_disk_size);
   } else {
      R__ASSERT(slice_disk_size <= fZipBufferSize);
      Read(fZipBuffer, slice_disk_size);
      int irep;
      int srcsize = slice_disk_size;
      int tgtsize = slice_mem_size;
      R__unzip(&srcsize, fZipBuffer, &tgtsize, static_cast<unsigned char *>(slice->GetBuffer()), &irep);
      R__ASSERT(uint64_t(irep) == slice_mem_size);
   }
}


std::uint64_t ROOT::Experimental::RColumnSourceRaw::GetNElements(ROOT::Experimental::RColumn *column)
{
   auto iter = fLiveColumns.find(column);
   if (iter == fLiveColumns.end())
      throw "not found";
   std::uint32_t column_id = iter->second;
   std::uint64_t nelements = fColumnElements[column_id];
   DEBUGMSG(std::cout << "Column #" << column_id << " has " << nelements << " elements"
                     << std::endl;)
   return fColumnElements[column_id];
}


void ROOT::Experimental::RColumnSourceRaw::Seek(size_t pos)
{
   lseek(fd, pos, SEEK_SET);
}


void ROOT::Experimental::RColumnSourceRaw::Read(void *buf, size_t size)
{
   ssize_t retval = read(fd, buf, size);
   //std::cout << "HAVE READ " << retval << "B, expected " << size << "B" << std::endl;
   R__ASSERT(retval >= 0);
   R__ASSERT(size_t(retval) == size);
}


void ROOT::Experimental::RColumnSourceRaw::Attach()
{
   fd = open(fPath.c_str(), O_RDONLY);

   std::uint32_t num_cols;
   Read(&num_cols, sizeof(num_cols));
   DEBUGMSG(std::cout << "Found " << num_cols << " columns" << std::endl;)
   for (unsigned i = 0; i < num_cols; ++i) {
      uint32_t id;
      Read(&id, sizeof(id));
      EColumnType type;
      Read(&type, sizeof(type));
      std::size_t element_size;
      Read(&element_size, sizeof(element_size));
      std::uint32_t compressionSettings;
      Read(&compressionSettings, sizeof(compressionSettings));
      uint32_t name_len;
      Read(&name_len, sizeof(name_len));
      char *name_raw = new char[name_len];
      Read(name_raw, name_len);
      std::string name(name_raw, name_len);
      delete[] name_raw;

      DEBUGMSG(std::cout << "Column " << name << ", id " << id << ", type "
                         << int(type) << ", element size " << element_size
                         << ", compression " << compressionSettings << std::endl;)
      fIndex.push_back(nullptr);
      fColumnIds[name] = id;
      fColumnElementSizes[id] = element_size;
      fColumnCompressionSettings[id] = compressionSettings;

      fAllColumns.emplace_back(RColumnModel(name, "TODO", type, false /*todo*/));
      fIds[i] = id;
   }

   size_t footer_pos;
   lseek(fd, -sizeof(footer_pos), SEEK_END);
   std::size_t eof_pos = lseek(fd, 0, SEEK_CUR);
   Read(&footer_pos, sizeof(footer_pos));
   DEBUGMSG(std::cout << "Found footer pos at " << footer_pos
                      << ", eof pos at " << eof_pos << std::endl;)

   lseek(fd, footer_pos, SEEK_SET);
   Read(&fNentries, sizeof(fNentries));
   DEBUGMSG(std::cout << "Found #" << fNentries << " entries in file" << std::endl;)
   std::size_t cur_pos = footer_pos + sizeof(fNentries);
   for (unsigned c = 0; c < num_cols; ++c) {
      std::uint32_t id;
      //std::cout << "Reading in ID" << std::endl;
      Read(&id, sizeof(id));  cur_pos += sizeof(id);
      //std::cout << "Reading in num_elements" << std::endl;
      std::uint64_t num_elements;
      Read(&num_elements, sizeof(num_elements));  cur_pos += sizeof(num_elements);
      std::uint32_t nslices;
      //std::cout << "Reading in slices count" << std::endl;
      Read(&nslices, sizeof(nslices));  cur_pos += sizeof(nslices);
      Index_t* col_index = new Index_t();
      DEBUGMSG(std::cout << "Reading in slices detail" << std::endl;)
      for (unsigned i = 0; i < nslices; ++i) {
         std::pair<std::uint64_t, Internal::RSliceInfo> index_entry;
         Read(&index_entry, sizeof(index_entry));  cur_pos += sizeof(index_entry);
         //std::cout << "  READ " << index_entry.first << "/" << index_entry.second.fFilePos << std::endl;
         col_index->push_back(index_entry);
      }
      fIndex[id] = std::move(std::unique_ptr<Index_t>(col_index));
      fColumnElements[id] = num_elements;
      DEBUGMSG(std::cout << "Read index of column " << id <<
               " with " << nslices << " slices" <<
               " and " << num_elements << " elements" << std::endl;)
   }

   std::uint64_t nclusters;
   Read(&nclusters, sizeof(nclusters));
   for (unsigned i = 0; i < nclusters; ++i) {
     std::pair<OffsetColumn_t, uint64_t> nelem;
     Read(&nelem, sizeof(nelem));
     fClusters.push_back(nelem);
     fClusterList.push_back(nelem.first);
     DEBUGMSG(std::cout << "Found cluster boundary at " << nelem.first
                        << " (" << nelem.second << ")" << std::endl;)
   }

   Read(&fStats, sizeof(fStats));
   fStats.Print();

   DEBUGMSG(std::vector<std::string> strStats;
   std::uint64_t totalDiskSize = 0;
   std::uint64_t totalMemSize = 0;
   for (unsigned c = 0; c < fAllColumns.size(); ++c) {
      std::ostringstream sstr;
      std::uint64_t memSize = fColumnElements[fIds[c]] * fColumnElementSizes[fIds[c]];
      std::uint64_t diskSize = 0;
      for (unsigned j = 0; j < fIndex[fIds[c]]->size(); ++j) {
         diskSize += (*fIndex[fIds[c]])[j].second.fSize;
      }
      sstr << "STATS FOR " << fAllColumns[c].GetName() << ": "
           << memSize << " >> " << diskSize << " --> "
           << (double(memSize) / double(diskSize)) << std::endl;
      totalDiskSize += diskSize;
      totalMemSize += memSize;
      strStats.push_back(sstr.str());
   }
   std::ostringstream sstr;
   sstr << "ALL SLICES COMPRESSION "
        << (double(totalMemSize) / double(totalDiskSize)) << std::endl;
   strStats.push_back(sstr.str());
   std::sort(strStats.begin(), strStats.end());
   for (auto s : strStats) std::cout << s;)
}

ROOT::Experimental::RColumnSourceRaw::~RColumnSourceRaw()
{
   ::close(fd);
}


std::unique_ptr<ROOT::Experimental::RColumnSource> ROOT::Experimental::RColumnSourceRaw::Clone()
{
   std::unique_ptr<RColumnSource> result(new RColumnSourceRaw(fPath));
   return std::move(result);
}


const ROOT::Experimental::RColumnSource::ColumnList_t& ROOT::Experimental::RColumnSourceRaw::ListColumns()
{
   return fAllColumns;
}


const ROOT::Experimental::RColumnSource::ClusterList_t& ROOT::Experimental::RColumnSourceRaw::ListClusters()
{
   return fClusterList;
}
