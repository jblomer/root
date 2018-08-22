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


   void ReadV(std::int64_t start, std::uint64_t num, void *dst)
   {
     if ((start < fCurrentSliceStart) || (start > fCurrentSliceEnd)) {
       MapSlice(start);
       //std::cout << "Mapped slice [" << fCurrentSliceStart << "-"
       //          << fCurrentSliceEnd << "] for element " << num
       //          << std::endl;
     }
     void *buf = reinterpret_cast<unsigned char *>(fCurrentSlice->GetBuffer())
                 + (start - fCurrentSliceStart) * fModel.GetElementSize();
     std::uint64_t remaining = (fCurrentSliceEnd - start) + 1;
     num = std::min(num, remaining);
     memcpy(dst, buf, num * fModel.GetElementSize());

     if (remaining < num) {
        MapSlice(start + num + 1);
        memcpy(reinterpret_cast<unsigned char *>(dst) + num * fModel.GetElementSize(),
               fCurrentSlice->GetBuffer(),
               remaining * fModel.GetElementSize());
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
