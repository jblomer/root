/// \file ROOT/RTreeView.hxx
/// \ingroup Forest ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2018-10-05
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2015, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_RTreeView
#define ROOT7_RTreeView

#include <ROOT/RForestUtil.hxx>
#include <ROOT/RStringView.hxx>
#include <ROOT/RTreeField.hxx>

#include <memory>
#include <utility>

namespace ROOT {
namespace Experimental {

// clang-format off
/**
\class ROOT::Experimental::RTreeViewContext
\ingroup Forest
\brief The TreeViewContext wraps the entry number (index) for a group of TreeView objects
*/
// clang-format on
class RTreeViewContext {
   friend class RInputTree;

private:
   TreeIndex_t fIndex;
   Detail::RPageSource* fPageSource;

   explicit RTreeViewContext(Detail::RPageSource* pageSource) : fIndex(0), fPageSource(pageSource) {}
   RTreeViewContext(const RTreeViewContext& other) = delete;
   RTreeViewContext& operator=(const RTreeViewContext& other) = delete;
   ~RTreeViewContext() = default;

   void Advance() { fIndex++; }
   void Reset() { fIndex = 0; }

public:
   TreeIndex_t GetIndex() const { return fIndex; }
   Detail::RPageSource* GetPageSource() const { return fPageSource; }
};

class RTreeViewBase {
protected:
   const RTreeViewContext& fContext;
public:
   RTreeViewBase(RTreeViewContext* context) : fContext(*context) {}
   ~RTreeViewBase() = default;
};

// clang-format off
/**
\class ROOT::Experimental::RTreeView
\ingroup Forest
\brief An RTreeView provides read-only access to a single field of the tree

(NB(jblomer): The tree view is very close to TTreeReader. Do we simply want to teach TTreeReader to deal with Forest?)

The view owns a field and its underlying columns in order to fill a tree value object with data. Data can be
accessed by index. For top level fields, the index refers to the entry number. Fields that are part of
nested collections have global index numbers that are derived from their parent indexes.

The RTreeView object is an iterable. That means, all field values in the tree can be sequentially read from begin() to end().

For simple types, template specializations let the reading become a pure mapping into a page buffer.
*/
// clang-format on
template <typename T>
class RTreeView : public RTreeViewBase {
   friend class RInputTree;

private:
   RTreeField<T> fField;
   RTreeValue<T> fValue;
   RTreeView(std::string_view fieldName, RTreeViewContext* context)
      : RTreeViewBase(context), fField(fieldName), fValue(fField.GenerateValue())
   {
      fField.ConnectColumns(fContext.GetPageSource());
   }

public:
   RTreeView(const RTreeView& other) = delete;
   RTreeView(RTreeView&& other) = default;
   RTreeView& operator=(const RTreeView& other) = delete;
   RTreeView& operator=(RTreeView&& other) = default;
   ~RTreeView() { fField.DestroyValue(fValue); }

   const T& operator()() {
      fField.Read(fContext.GetIndex(), &fValue);
      return *fValue.Get();
   }
};


// clang-format off
/**
\class ROOT::Experimental::RTreeView<TreeIndex_t>
\ingroup Forest
\brief An RTreeView specialization for an index field without its sub fields
*/
// clang-format on
//template <>
//class RTreeView<TreeIndex_t> {
//protected:
//   std::unique_ptr<RTreeField<TreeIndex_t>> fField;
//   RTreeValue<TreeIndex_t> fValue;
//
//public:
//   RTreeView(std::unique_ptr<RTreeField<TreeIndex_t>> field);
//
//   /// Use the field to read the referenced element into the tree value object. To be inlined
//   TreeIndex_t operator ()(TreeIndex_t index);
//};


// clang-format off
/**
\class ROOT::Experimental::RTreeViewCollection
\ingroup Forest
\brief A tree view for a collection, that can itself generate new tree views for its nested fields.
*/
// clang-format on
//class RTreeViewCollection : public RTreeView<TreeIndex_t> {
//public:
//   template <typename T>
//   RTreeView<T> GetView(std::string_view fieldName) {
//      auto field = std::make_unique<RTreeField<T>>(fieldName);
//      // ...
//      return RTreeView<T>(std::move(field));
//   }
//
//   RTreeViewCollection GetViewCollection(std::string_view fieldName);
//};

} // namespace Experimental
} // namespace ROOT

#endif
