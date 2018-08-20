/// \file ROOT/RColumnElement.hxx
/// \ingroup Forest ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2018-07-20
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2015, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_RColumnElement
#define ROOT7_RColumnElement

#include <ROOT/RColumnModel.hxx>

#include <cassert>
#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

namespace ROOT {
namespace Experimental {

using OffsetColumn_t = std::uint64_t;

class RColumnElementBase {
protected:
   // We can indicate that on disk format == in memory format
   bool fIsMovable;
   void *fRawContent;
   unsigned fSize;

   EColumnType fColumnType;

   virtual void DoSerialize(void* /* destination */) const { assert(false); }
   virtual void DoDeserialize(void* /* source */) const { assert(false); }

public:
   RColumnElementBase()
     : fIsMovable(false)
     , fRawContent(nullptr)
     , fSize(0)
     , fColumnType(EColumnType::kByte) { }
   virtual ~RColumnElementBase() { }

   EColumnType GetColumnType() const { return fColumnType; }

   void Serialize(void *destination) const {
     if (!fIsMovable) {
       DoSerialize(destination);
       return;
     }
     //if (fColumnType == RTreeColumnType::kFloat)
     //  printf("WRITE VALUE %f\n", *((float *)fRawContent));
     //else
     //  printf("WRITE VALUE %lu\n", *((std::uint64_t *)fRawContent));
     std::memcpy(destination, fRawContent, fSize);
   }

   void Deserialize(void *source) {
     if (!fIsMovable) {
       DoDeserialize(source);
       return;
     }
     std::memcpy(fRawContent, source, fSize);
   }

   std::size_t GetSize() const {
     return fSize;
   }

   void* GetRawContent() { return fRawContent; }
   void* GetRawContentAddr() { return &fRawContent; }
   // Used to map directly from slices
   void SetRawContent(void *source) {
     if (!fIsMovable) {
       Deserialize(source);
       return;
     }
     fRawContent = source;
   }
};


template <typename T>
class RColumnElement : public RColumnElementBase {
   T* fValue;

   void Initialize();

public:
   template<typename... ArgsT>
   explicit RColumnElement(T *value) : fValue(value) {
     Initialize();
     fColumnType = MapType();
   }

   static EColumnType MapType();
   static std::string GetMemoryType();
};


RColumnElementBase MakeColumnElement(EColumnType type);


} // namespace Experimental
} // namespace ROOT

#endif
