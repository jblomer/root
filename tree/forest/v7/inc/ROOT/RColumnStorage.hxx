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
class RColumnSinkRaw;
class RColumnSourceRaw;

class RColumnSink {
public:
   static std::unique_ptr<RColumnSinkRaw> MakeSinkRaw(std::string_view path);

   virtual ~RColumnSink() { }

   virtual void OnCreate() = 0;
   virtual void OnAddColumn(RColumn *column) = 0;
   virtual void OnFullSlice(RColumnSlice *slice, RColumn *column) = 0;
   virtual void OnCommit(OffsetColumn_t nentries) = 0;
};


class RColumnSinkRaw : public RColumnSink {
   static constexpr std::size_t kEpochSize = 1024 * 1024 * 10;

   using SliceHeads_t = std::vector<std::pair<uint64_t, uint64_t>>;

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
   int fd;
   std::size_t fFilePos;
   std::unordered_map<RColumn*, std::unique_ptr<RColumnIndex>> fGlobalIndex;
   std::unordered_map<RColumn*, std::unique_ptr<RColumnIndex>> fEpochIndex;

   void Write(const void *buf, std::size_t size);
   void WriteFooter(std::uint64_t nentries);
   void WriteMiniFooter();
   void WritePadding(std::size_t padding);

public:
   RColumnSinkRaw(std::string_view path);
   virtual ~RColumnSinkRaw();

   void OnCreate() final;
   void OnAddColumn(RColumn *column) final;
   void OnFullSlice(RColumnSlice *slice, RColumn *column) final;
   void OnCommit(OffsetColumn_t nentries) final;
};


class RColumnSource {
public:
   static std::unique_ptr<RColumnSourceRaw> MakeSourceRaw(std::string_view path);

   virtual ~RColumnSource() { }

   virtual void Attach() = 0;

   virtual void OnAddColumn(RColumn *column) = 0;
   virtual void OnMapSlice(RColumn *column, std::uint64_t num, RColumnSlice *slice) = 0;
   virtual std::uint64_t GetNentries() = 0;
   virtual std::uint64_t GetNElements(RColumn *column) = 0;
};

class RColumnSourceRaw : public RColumnSource {
   using Index_t = std::vector<std::pair<std::uint64_t, std::uint64_t>>;
   using ColumnIds_t = std::unordered_map<std::string, std::uint32_t>;
   using ColumnElementSizes_t = std::unordered_map<std::uint32_t, std::size_t>;
   using ColumnElements_t = std::unordered_map<std::uint32_t, std::uint64_t>;
   using LiveColumns_t = std::unordered_map<RColumn*, std::uint32_t>;

   std::string fPath;
   int fd;
   std::uint64_t fNentries;
   std::vector<std::unique_ptr<Index_t>> fIndex;
   ColumnIds_t fColumnIds;
   ColumnElementSizes_t fColumnElementSizes;
   ColumnElements_t fColumnElements;
   LiveColumns_t fLiveColumns;

   void Read(void *buf, std::size_t size);
   void Seek(std::size_t size);

public:
   RColumnSourceRaw(std::string_view path)
      : fPath(path)
      , fd(-1)
      , fNentries(0)
   { }
   ~RColumnSourceRaw();

   void Attach() final;

   void OnAddColumn(RColumn *column) final;
   void OnMapSlice(RColumn *column, std::uint64_t num, RColumnSlice *slice) final;
   std::uint64_t GetNentries() final {
      return fNentries;
   }
   std::uint64_t GetNElements(RColumn *column) final;
};

} // namespace Experimental
} // namespace ROOT

#endif
