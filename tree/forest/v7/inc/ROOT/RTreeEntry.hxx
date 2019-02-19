/// \file ROOT/RTreeEntry.hxx
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

#ifndef ROOT7_RTreeEntry
#define ROOT7_RTreeEntry

#include <ROOT/RStringView.hxx>
#include <ROOT/RTreeField.hxx>
#include <ROOT/RTreeValue.hxx>

#include <memory>
#include <utility>
#include <vector>

namespace ROOT {
namespace Experimental {

// clang-format off
/**
\class ROOT::Experimental::RTreeEntry
\ingroup Forest
\brief The RTreeEntry is a collection of tree values corresponding to a complete row in the data set

The entry provides a memory-managed binder for a set of values. Through shared pointers, the memory locations
that are associated to values are managed.
*/
// clang-format on
class RTreeEntry {
   std::vector<Detail::RTreeValueBase> fTreeValues;
   /// The objects involed in serialization and deserialization might be used long after the entry is gone:
   /// hence the shared pointer
   std::vector<std::shared_ptr<void>> fValuePtrs;
   /// Points into fTreeValues and indicates the values that are owned by the entry and need to be destructed
   std::vector<std::size_t> fManagedValues;

public:
   RTreeEntry() = default;
   RTreeEntry(const RTreeEntry& other) = delete;
   RTreeEntry& operator=(const RTreeEntry& other) = delete;
   ~RTreeEntry();

   /// Adds a value whose storage is managed by the entry
   void AddValue(const Detail::RTreeValueBase& value);

   /// While building the entry, adds a new value to the list and return the value's shared pointer
   template<typename T, typename... ArgsT>
   std::shared_ptr<T> AddValue(RTreeField<T>* field, ArgsT&&... args) {
      auto ptr = std::make_shared<T>(std::forward<ArgsT>(args)...);
      fTreeValues.emplace_back(Detail::RTreeValueBase(field->CaptureValue(ptr.get())));
      fValuePtrs.emplace_back(ptr);
      return ptr;
   }

   template<typename T>
   T* Get(std::string_view fieldName) {
      for (auto& v : fTreeValues) {
         if (v.GetField()->GetName() == fieldName)
            return static_cast<T*>(v.GetRawPtr());
      }
      return nullptr;
   }

   decltype(fTreeValues)::iterator begin() { return fTreeValues.begin(); }
   decltype(fTreeValues)::iterator end() { return fTreeValues.end(); }
};

} // namespace Experimental
} // namespace ROOT

#endif
