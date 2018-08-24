/// \file ROOT/RColumnStorage.hxx
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

#ifndef ROOT7_RColumnStorage
#define ROOT7_RColumnStorage

#include <ROOT/RColumnModel.hxx>
#include <ROOT/RColumnUtil.hxx>
#include <ROOT/RStringView.hxx>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ROOT {
namespace Experimental {

class RColumn;
class RColumnSlice;
class RColumnSinkFile;
class RColumnSinkRaw;
class RColumnSourceRaw;
class RColumnFileSettings;

namespace Internal {
struct RSliceInfo {
   RSliceInfo() : fFilePos(0), fSize(0) { }
   RSliceInfo(std::uint64_t p, std::uint64_t s) : fFilePos(p), fSize(s) { }
   std::uint64_t fFilePos;
   std::uint64_t fSize;  // compressed size
}__attribute__((__packed__));
} // namespace Internal


struct RColumnRawSettings {
   static constexpr std::size_t kDefaultEpochSize = 1024 * 1024 * 32;  // 32MB
   static constexpr int kDefaultCompressionSettings = 0;  // no compression

   explicit RColumnRawSettings(
      std::string_view path,
      std::size_t epoch_size = kDefaultEpochSize,
      int compression_settings = kDefaultCompressionSettings)
      : fPath(path)
      , fEpochSize(epoch_size)
      , fCompressionSettings(compression_settings)
   {
   }

   std::string fPath;
   std::size_t fEpochSize;
   int fCompressionSettings;
};

namespace Detail {
struct RColumnRawStats {
   RColumnRawStats()
     : fBPadding(0)
     , fNPadding(0)
     , fBMiniFooter(0)
     , fNMiniFooter(0)
     , fBFooter(0)
     , fBHeader(0)
     , fBSliceMem(0)
     , fBSliceDisk(0)
     , fNSlice(0)
     , fBTotal(0)
   { }
   std::uint64_t fBPadding;
   std::uint64_t fNPadding;
   std::uint64_t fBMiniFooter;
   std::uint64_t fNMiniFooter;
   std::uint64_t fBFooter;
   std::uint64_t fBHeader;
   std::uint64_t fBSliceMem;
   std::uint64_t fBSliceDisk;
   std::uint64_t fNSlice;
   std::uint64_t fBTotal;
   void Print();
};
}


class RColumnSink {
public:
   static std::unique_ptr<RColumnSinkRaw> MakeSinkRaw(std::string_view path);
   static std::unique_ptr<RColumnSinkRaw> MakeSinkRaw(const RColumnRawSettings &settings);
   static std::unique_ptr<RColumnSinkFile> MakeSinkFile(const RColumnFileSettings &settings);

   virtual ~RColumnSink() { }

   virtual void OnCreate(std::string_view name) = 0;
   virtual void OnAddColumn(RColumn *column) = 0;
   virtual void OnCommitSlice(RColumnSlice *slice, RColumn *column) = 0;
   virtual void OnCommitCluster(OffsetColumn_t nentries) = 0;
   virtual void OnCommitDataset(OffsetColumn_t nentries) = 0;

   // TODO: add possibility for RTree to install call-backs, e.g. to let the
   // storage inform RTree how many bytes it has written and that it's time for
   // a new cluster.
};


class RColumnSinkRaw : public RColumnSink {
   // Maps element start number to slice
   using SliceHeads_t = std::vector<std::pair<std::uint64_t, Internal::RSliceInfo>>;

   struct RColumnIndex {
      RColumnIndex(std::uint32_t id)
         : fId(id)
         , fNumElements(0)
      { }
      std::uint32_t fId;
      std::uint64_t fNumElements;
      SliceHeads_t fSliceHeads;
   };

   std::string fPath;
   std::size_t fEpochSize;
   int fCompressionSettings;
   int fd;
   int fZipBufferSize;
   char *fZipBuffer;

   std::size_t fFilePos;
   std::unordered_map<RColumn*, std::unique_ptr<RColumnIndex>> fGlobalIndex;
   std::unordered_map<RColumn*, std::unique_ptr<RColumnIndex>> fEpochIndex;
   // entry number, file position
   std::vector<std::pair<OffsetColumn_t, std::uint64_t>> fClusters;

   Detail::RColumnRawStats fStats;

   void Write(const void *buf, std::size_t size);
   void WriteFooter(std::uint64_t nentries);
   void WriteMiniFooter();
   void WritePadding(std::size_t padding);
   void WriteClusters();

public:
   RColumnSinkRaw(const RColumnRawSettings &settings);
   virtual ~RColumnSinkRaw();

   void SetCompressionSetting(int settings) { fCompressionSettings = settings; }
   void SetEpochSize(std::size_t size) { fEpochSize = size; }

   void OnCreate(std::string_view name) final;
   void OnAddColumn(RColumn *column) final;
   void OnCommitSlice(RColumnSlice *slice, RColumn *column) final;
   void OnCommitCluster(OffsetColumn_t nentries) final;
   void OnCommitDataset(OffsetColumn_t nentries) final;
};


class RColumnSource {
public:
   using ColumnList_t = std::vector<RColumnModel>;
   using ClusterList_t = std::vector<OffsetColumn_t>;

   static std::unique_ptr<RColumnSourceRaw> MakeSourceRaw(std::string_view path);

   virtual ~RColumnSource() { }

   virtual void Attach(std::string_view name) = 0;

   virtual const ColumnList_t& ListColumns() = 0;
   virtual void ListBranches() { /* High-level objects, TODO */ }
   virtual const ClusterList_t& ListClusters() = 0;

   virtual void OnAddColumn(RColumn *column) = 0;
   virtual void OnMapSlice(RColumn *column, std::uint64_t num, RColumnSlice *slice) = 0;
   virtual std::uint64_t GetNentries() = 0;
   virtual std::uint64_t GetNElements(RColumn *column) = 0;

   virtual std::unique_ptr<RColumnSource> Clone() = 0;
};

class RColumnSourceRaw : public RColumnSource {
   // TODO: reduce the index maps
   using Index_t = std::vector<std::pair<std::uint64_t, Internal::RSliceInfo>>;
   using ColumnIds_t = std::unordered_map<std::string, std::uint32_t>;
   using ColumnElementSizes_t = std::unordered_map<std::uint32_t, std::size_t>;
   using ColumnElements_t = std::unordered_map<std::uint32_t, std::uint64_t>;
   using ColumnCompressionSettings_t = std::unordered_map<std::uint32_t, std::uint32_t>;
   using LiveColumns_t = std::unordered_map<RColumn*, std::uint32_t>;

   std::string fPath;
   int fd;
   int fZipBufferSize;
   unsigned char *fZipBuffer;
   std::uint64_t fNentries;
   std::vector<std::unique_ptr<Index_t>> fIndex;
   ColumnIds_t fColumnIds;
   ColumnElementSizes_t fColumnElementSizes;
   ColumnElements_t fColumnElements;
   ColumnCompressionSettings_t fColumnCompressionSettings;
   LiveColumns_t fLiveColumns;
   // entry number, file position
   std::vector<std::pair<OffsetColumn_t, std::uint64_t>> fClusters;
   ClusterList_t fClusterList;

   // TODO: shall we have an iterator interface over columns?
   ColumnList_t fAllColumns;
   std::map<std::uint64_t, std::uint64_t> fIds;

   Detail::RColumnRawStats fStats;

   void Read(void *buf, std::size_t size);
   void Seek(std::size_t size);

public:
   RColumnSourceRaw(std::string_view path)
      : fPath(path)
      , fd(-1)
      , fZipBufferSize(1024 * 1024)  // TODO
      , fZipBuffer(new unsigned char[fZipBufferSize])
      , fNentries(0)
   { }
   ~RColumnSourceRaw();

   void Attach(std::string_view name) final;

   void OnAddColumn(RColumn *column) final;
   void OnMapSlice(RColumn *column, std::uint64_t num, RColumnSlice *slice) final;
   std::uint64_t GetNentries() final {
      return fNentries;
   }
   std::uint64_t GetNElements(RColumn *column) final;
   const ColumnList_t& ListColumns() final;
   const ClusterList_t &ListClusters() final;

   std::unique_ptr<RColumnSource> Clone() final;
};

} // namespace Experimental
} // namespace ROOT

#endif
