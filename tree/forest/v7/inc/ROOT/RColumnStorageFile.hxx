/// \file ROOT/RColumnStorageFile.hxx
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

#ifndef ROOT7_RColumnStorageFile
#define ROOT7_RColumnStorageFile

#include <ROOT/RColumnStorage.hxx>
#include <ROOT/RStringView.hxx>

#include <TFile.h>

#include <cstdint>
#include <string>
#include <unordered_map>


namespace ROOT {
namespace Experimental {

struct RColumnFileSettings {
   explicit RColumnFileSettings(
      TFile* file,
      std::string_view forestName)
      : fFile(file)
      , fForestName(forestName)
      , fCompressionSettings(0)
   {
   }

   TFile* fFile;
   std::string fForestName;
   int fCompressionSettings;
};


namespace Internal {

struct RFSliceBuffer {
   //int n; //n
   //float* MyBuf; //[n]
//
   //RFSliceBuffer() : n(0), MyBuf(nullptr) {}
   //RFSliceBuffer(int s, float *b) : n(s), MyBuf(b) {}
   std::vector<unsigned char> buf;
};

struct RFColumnFileDesc {
   std::uint32_t fId;
   std::int32_t fColumnType;
   std::uint32_t fCompressionSettings;
   std::string fName;
};

struct RFColumnFileSlices {
   const std::vector<uint64_t>& GetElementStartRef() {
      return fElementStart;
   }

   std::uint32_t fId;
   std::uint32_t fNumElements;
   std::vector<uint64_t> fElementStart;
};

struct RFColumnFileHeader {
   std::vector<RFColumnFileDesc> fColumns;
};

struct RFColumnFileIndex {
   std::uint64_t fNentries = 0;
   std::vector<RFColumnFileSlices> fColumnSlices;
};

}


class RColumnSinkFile : public RColumnSink {
private:
   RColumnFileSettings fSettings;
   int fZipBufferSize;
   char *fZipBuffer;
   TDirectory* fDirectory;

   std::unordered_map<RColumn*, std::uint32_t> fColumn2Id;

   Internal::RFColumnFileHeader fHeader;
   Internal::RFColumnFileIndex fIndex;

public:
   explicit RColumnSinkFile(const RColumnFileSettings &settings);
   virtual ~RColumnSinkFile();

   void OnCreate(std::string_view name) final;
   void OnAddColumn(RColumn *column) final;
   void OnCommitSlice(RColumnSlice *slice, RColumn *column) final;
   void OnCommitCluster(OffsetColumn_t nentries) final;
   void OnCommitDataset(OffsetColumn_t nentries) final;
};


class RColumnSourceFile : public RColumnSource {
private:
   std::uint64_t fNentries;
   TFile *fFile;
   TDirectory *fDirectory;
   Internal::RFColumnFileHeader* fHeader;
   Internal::RFColumnFileIndex* fIndex;

   std::unordered_map<std::string, std::uint32_t> fName2Id;
   std::unordered_map<RColumn *, std::uint32_t> fColumn2Id;
   std::unordered_map<std::uint32_t, std::size_t> fId2IdxHeader;
   std::unordered_map<std::uint32_t, std::size_t> fId2IdxIndex;
   std::unordered_map<std::uint32_t, std::size_t> fId2ElementSize;

   ClusterList_t fClusterList;
   ColumnList_t fColumnList;
   RColumnSchema fColumnSchema;


public:
   explicit RColumnSourceFile(TFile *file);
   virtual ~RColumnSourceFile();

   void Attach(std::string_view name) final;

   void OnAddColumn(RColumn *column) final;
   void OnMapSlice(RColumn *column, std::uint64_t num, RColumnSlice *slice) final;
   std::uint64_t GetNentries() final {
      return fNentries;
   }
   std::uint64_t GetNElements(RColumn *column) final;
   const ColumnList_t& ListColumns() final;
   virtual const RColumnSchema& GetColumnSchema() final;
   const ClusterList_t &ListClusters() final;

   std::unique_ptr<RColumnSource> Clone() final;
};


} // namespace Experimental
} // namespace ROOT

#endif
