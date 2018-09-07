/// \file ROOT/RColumn.hxx
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

#ifndef ROOT7_RColumn
#define ROOT7_RColumn

#include <ROOT/RColumnElement.hxx>
#include <ROOT/RColumnModel.hxx>
#include <ROOT/RColumnSlice.hxx>
#include <ROOT/RConfig.h>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


namespace ROOT {
namespace Experimental {

class RColumnSource;
class RColumnSink;

class RColumn {
   RColumnModel fModel;
   RColumnSource* fSource;
   RColumnSink* fSink;
   std::unique_ptr<RColumnSlice> fHeadSlice;
   std::uint64_t fMaxElement;

   std::unique_ptr<RColumnSlice> fCurrentSlice;
   std::int64_t fCurrentSliceStart;
   std::int64_t fCurrentSliceEnd;

   void ShipHeadSlice();
   void MapSlice(std::uint64_t num);

public:
   static const unsigned kDefaultSliceSize = 32000;

   // Points to a certain element in a slice, used in RDataFrame
   /*class RCursor {
   private:
      void* fElementPtr;
      RColumn* fColumn;
   public:
      RCursor(RColumn *column) : fElementPtr(nullptr), fColumn(column) { }
      void Set(const std::uint64_t num) {
         if ((num < fColumn->GetCurrentSliceStart()) ||
             (num > fColumn->GetCurrentSliceEnd())) {
            MapSlice(num);
         }
         RColumnElementBase *__restrict__ element
      }
    //  void *buf = reinterpret_cast<unsigned char *>(fCurrentSlice->GetBuffer())
    //             + (num - fCurrentSliceStart) * element->GetSize();
    // element->Deserialize(buf);
    //  }
      void* GetElementPtr { return fElementPtr; }
   }; // struct RCursor*/

   RColumn(const RColumnModel &model,
           RColumnSource *source, RColumnSink *sink);

   RColumnModel GetModel() { return fModel; }
   EColumnType GetColumnType() { return fModel.GetType(); }
   std::string GetName() { return fModel.GetName(); }

   void Append(const RColumnElementBase &element) {
     //std::cout << "appending to " << fModel.GetName()
     //          << "(max: " << fMaxElement << ")" << std::endl;
     //assert(element.GetColumnType() == fModel.GetType());
     void *dst = fHeadSlice->Reserve(element.GetSize());
     if (dst == nullptr) {
       ShipHeadSlice();
       dst = fHeadSlice->Reserve(element.GetSize());
       assert(dst != nullptr);
     }
     element.Serialize(dst);
     fMaxElement++;
     fHeadSlice->Release();
   }

   void Flush() {
     //std::cout << "flushing head basket" << std::endl;
     if (fMaxElement > fHeadSlice->GetRangeStart())
       ShipHeadSlice();
   }


   void Read(const std::int64_t num, RColumnElementBase *__restrict__ element) {
     if ((num < fCurrentSliceStart) || (num > fCurrentSliceEnd)) {
       //std::cout << "Mapping slice [" << fCurrentSliceStart << "-"
       //          << fCurrentSliceEnd << "] for element " << num
       //          << std::endl;
       MapSlice(num);
     }
     void *buf = reinterpret_cast<unsigned char *>(fCurrentSlice->GetBuffer())
                 + (num - fCurrentSliceStart) * element->GetSize();
     element->Deserialize(buf);
   }


   void Map(const std::int64_t num, RColumnElementBase *__restrict__ element) {
     if ((num < fCurrentSliceStart) || (num > fCurrentSliceEnd)) {
       //std::cout << "Mapping slice [" << fCurrentSliceStart << "-"
       //          << fCurrentSliceEnd << "] for element " << num
       //          << std::endl;
       MapSlice(num);
     }
     void *buf = reinterpret_cast<unsigned char *>(fCurrentSlice->GetBuffer())
                 + (num - fCurrentSliceStart) * element->GetSize();
     element->SetRawContent(buf);
   }


   void ReadV(const std::int64_t num, const std::uint64_t count, void *dst)
   {
     if ((num < fCurrentSliceStart) || (num > fCurrentSliceEnd)) {
       MapSlice(num);
       //std::cout << "Mapped slice [" << fCurrentSliceStart << "-"
       //          << fCurrentSliceEnd << "] for element " << num
       //          << std::endl;
     }
     void *buf = reinterpret_cast<unsigned char *>(fCurrentSlice->GetBuffer())
                 + (num - fCurrentSliceStart) * fModel.GetElementSize();
     std::uint64_t remaining_in_slice = fCurrentSliceEnd - num + 1;
     std::uint64_t processed = std::min(count, remaining_in_slice);
     memcpy(dst, buf, processed * fModel.GetElementSize());

     while (R__unlikely(processed < count)) {
       std::uint64_t remaining = count - processed;
       MapSlice(num + processed);
       remaining_in_slice = fCurrentSliceEnd - fCurrentSliceStart + 1;
       std::uint64_t processed_in_slice = std::min(remaining, remaining_in_slice);
       memcpy(reinterpret_cast<unsigned char *>(dst) + processed * fModel.GetElementSize(),
              fCurrentSlice->GetBuffer(),
              processed_in_slice * fModel.GetElementSize());
       processed += processed_in_slice;
     }
   }

   std::uint64_t GetNElements() { return fMaxElement; }
   std::uint64_t GetCurrentSliceStart() { return fCurrentSliceStart; }
   std::uint64_t GetCurrentSliceEnd() { return fCurrentSliceEnd; }
}; // RColumn

using RColumnCollection = std::vector<RColumn*>;

} // namespace Experimental
} // namespace ROOT

#endif
