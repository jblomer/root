/// \file ROOT/RTreeView.hxx
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

#ifndef ROOT7_RTreeView
#define ROOT7_RTreeView

#include <ROOT/RBranch.hxx>
#include <ROOT/RCargo.hxx>
#include <ROOT/RColumnPointer.hxx>
#include <ROOT/RColumnRange.hxx>
#include <ROOT/RColumnUtil.hxx>
#include <ROOT/RStringView.hxx>

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "iterator_tpl.h"

namespace ROOT {
namespace Experimental {

class RColumnSource;

template <typename T>
class RTreeView {
private:
  std::unique_ptr<RBranch<T>> fBranch;
  RCargo<T> fCargo;

public:
  RTreeView(RBranch<T> *branch)
    : fBranch(branch)
    , fCargo(fBranch.get())
  { }

  const T& operator ()(const RColumnPointer &p) {
    fBranch->Read(p.GetIndex(), &fCargo);
    return *fCargo.Get();
  }

  void ReadBulk(std::uint64_t start, std::uint64_t num, T *buf) {
    fBranch->ReadV(start, num, buf);
  }

  struct ViewIterator {
    std::uint64_t pos;
    inline void next(const RTreeView<T>* ref) {
      pos++;
    }
    inline void begin(const RTreeView<T>* ref) {
      pos = 0;
    }
    inline void end(const RTreeView<T>* ref) {
      pos = ref->fBranch->GetNItems();
    }
    inline T get(RTreeView<T>* ref) {
      return (*ref)(RColumnPointer(pos));
    }
    inline const T get(const RTreeView<T>* ref)
    {
      return (*ref)(RColumnPointer(pos));
    }
    inline bool cmp(const ViewIterator& s) const {
      return (pos != s.pos);
    }
  };
  SETUP_ITERATORS(RTreeView, T, ViewIterator);
};


template <>
class RTreeView<OffsetColumn_t> {
protected:
  RBranch<OffsetColumn_t> *fBranch;

private:
  // For offset columns, read both the index and the one before to
  // get the size TODO
  OffsetColumn_t fOffsetPair[2];
  RCargo<OffsetColumn_t> fCargo;

public:
  RTreeView(RBranch<OffsetColumn_t> *branch)
    : fBranch(branch)
    , fCargo(fBranch)
  { }

  RColumnRange GetRange(const RColumnPointer &p) {
    if (p.GetIndex() == 0) {
      fBranch->Read(0, &fCargo);
      return RColumnRange(0, *fCargo.Get());
    }
    fBranch->Read(p.GetIndex() - 1, &fCargo);
    OffsetColumn_t lower = *fCargo.Get();
    fBranch->Read(p.GetIndex(), &fCargo);
    return RColumnRange(lower, *fCargo.Get());
  }

  OffsetColumn_t operator ()(const RColumnPointer &p) {
    return GetRange(p).GetSize();
  }

  void ReadBulk(std::uint64_t start, std::uint64_t num, OffsetColumn_t *buf) {
    fBranch->ReadV(start, num, buf);
  }
};


class RTreeViewCollection : public RTreeView<OffsetColumn_t> {
private:
  RColumnSource *fSource;
public:
  RTreeViewCollection(RBranch<OffsetColumn_t> *b, RColumnSource *s) :
    RTreeView<OffsetColumn_t>(b), fSource(s) { }

  template <typename T>
  RTreeView<T> GetView(std::string_view name) {
    // TODO not with raw pointer
    auto branch = new RBranch<T>(fBranch->GetName() + "/" + std::string(name));
    branch->GenerateColumns(fSource, nullptr);
    return RTreeView<T>(branch);
  }

  RTreeViewCollection GetViewCollection(std::string_view name) {
    auto branch = new RBranch<OffsetColumn_t>(
      fBranch->GetName() + "/" + std::string(name));
    branch->GenerateColumns(fSource, nullptr);
    return RTreeViewCollection(branch, fSource);
  }
};

} // namespace Experimental
} // namespace ROOT

#endif
