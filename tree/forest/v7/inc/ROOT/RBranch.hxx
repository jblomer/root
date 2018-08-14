/// \file ROOT/RBranch.hxx
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

#ifndef ROOT7_RBranch
#define ROOT7_RBranch

#include <ROOT/RCargo.hxx>
#include <ROOT/RColumn.hxx>
#include <ROOT/RColumnStorage.hxx>
#include <ROOT/RColumnUtil.hxx>
#include <ROOT/RStringView.hxx>

#include <cassert>
#include <iterator>
#include <stack>
#include <string>
#include <string_view>
#include <vector>

#include "iterator_tpl.h"

namespace ROOT {
namespace Experimental {

class RBranchBase {
   friend class RTree;  // REMOVE ME

protected:
   RBranchBase *fParent;
   std::vector<RBranchBase *> fChildren; /* TODO unique_ptr */
   std::string fDescription;
   std::string fName;
   bool fIsSimple;
   RColumn *fPrincipalColumn;

   explicit RBranchBase(std::string_view name)
     : fParent(nullptr)
     , fName(name)
     , fIsSimple(false)
     , fPrincipalColumn(nullptr)
   { }

   virtual void DoAppend(RCargoBase *cargo) { assert(false); }
   virtual void DoRead(std::uint64_t num, RCargoBase *cargo) { assert(false); }
   //virtual void DoReadV(std::uint64_t num, RLeafBase *leaf) { assert(false); }

public:
  struct IteratorState {
    std::stack<unsigned> pos_stack;
    const RBranchBase *current;
    unsigned pos;
    inline void next(const RBranchBase* ref) {
      if (pos < current->fChildren.size()) {
        current = current->fChildren[pos];
        pos_stack.push(pos);
        pos = 0;
      }
      while ((pos == current->fChildren.size()) && !pos_stack.empty()) {
        current = current->fParent;
        pos = pos_stack.top() + 1;
        pos_stack.pop();
      }
    }
    inline void begin(const RBranchBase* ref) {
      current = ref;
      pos = 0;
    }
    inline void end(const RBranchBase* ref) {
      current = ref;
      pos = ref->fChildren.size();
    }
    inline RBranchBase* get(RBranchBase* ref) {
      return current->fChildren[pos];
    }
    inline const RBranchBase* get(const RBranchBase* ref)
    {
      return current->fChildren[pos];
    }
    inline bool cmp(const IteratorState& s) const {
      return (current != s.current) || (pos != s.pos);
    }
  };
  SETUP_ITERATORS(RBranchBase, RBranchBase*, IteratorState);

  std::string GetName() { return fName; }
  void PrependName(std::string_view parent) {
    fName = std::string(parent) + "/" + fName;
  }

  virtual ~RBranchBase() { }

  void Attach(RBranchBase *child) {
    child->fParent = this;
    fChildren.push_back(child);
  }

  virtual RColumn* GenerateColumns(RColumnSource *source, RColumnSink *sink)
    = 0;

  void Append(RCargoBase *__restrict__ cargo) {
    if (!fIsSimple) {
      DoAppend(cargo);
      return;
    }
    //std::cout << "Append branch " << fName << " with element "
    //          << cargo->fPrincipalElement->GetRawContent() << std::endl;
    fPrincipalColumn->Append(*(cargo->fPrincipalElement));
  }

  void Read(std::uint64_t num, RCargoBase *__restrict__ cargo) {
    if (!fIsSimple) {
      DoRead(num, cargo);
      return;
    }
    //std::cout << "READING " << num << std::endl;
    fPrincipalColumn->Read(num, cargo->fPrincipalElement);
  }

  void ReadV(std::uint64_t start, std::uint64_t num, void *dst)
  {
    assert(fIsSimple);
    fPrincipalColumn->ReadV(start, num, dst);
  }

  std::uint64_t GetNItems() {
    return fPrincipalColumn->GetNElements();
  }

  //void
};


class RBranchCollectionTag {};

template <typename T>
class RBranch : public RBranchBase {
};

template <>
class RBranch<RBranchCollectionTag> : public RBranchBase {
public:
  RBranch() : RBranchBase("") {
    fIsSimple = true;
  }
  explicit RBranch(std::string_view name) : RBranchBase(name) {
    fIsSimple = true;
  }

  virtual RColumn* GenerateColumns(RColumnSource *source, RColumnSink *sink)
    override
  {
    fPrincipalColumn = new RColumn(
      RColumnModel(fName, fDescription, EColumnType::kOffset, false),
      source, sink);
    return fPrincipalColumn;
  }

  bool IsRoot() { return fName.empty(); }
  void MakeSubBranch(std::string_view name) { fName = std::string(name); }
};


template <>
class RBranch<std::vector<float>> : public RBranchBase {
private:
  RColumn* fValueColumn;

public:
  RBranch() : RBranchBase("") {
    fIsSimple = false;
  }
  explicit RBranch(std::string_view name) : RBranchBase(name) {
    fIsSimple = false;
  }

  virtual RColumn* GenerateColumns(RColumnSource *source, RColumnSink *sink)
    override
  {
    fPrincipalColumn = new RColumn(
      RColumnModel(fName, fDescription, EColumnType::kOffset, false),
      source, sink);
    fValueColumn = new RColumn(
      RColumnModel(fName + "/@1", fDescription, EColumnType::kFloat, false),
      source, sink);

    return fPrincipalColumn;
  }

  virtual void DoAppend(RCargoBase *cargo) override {
    //fPrincipalColumn->Append(*(cargo->fPrincipalElement));
  }

  virtual void DoRead(std::uint64_t num, RCargoBase *cargo) override {
  }
};


template <>
class RBranch<float> : public RBranchBase {
public:
  explicit RBranch(std::string_view name) : RBranchBase(name) {
    fIsSimple = true;
  }

  virtual RColumn* GenerateColumns(RColumnSource *source, RColumnSink *sink)
    override
  {
    fPrincipalColumn = new RColumn(
      RColumnModel(fName, fDescription, EColumnType::kFloat, false),
      source, sink);
    return fPrincipalColumn;
  }
};

template <>
class RBranch<double> : public RBranchBase {
public:
  explicit RBranch(std::string_view name) : RBranchBase(name) {
    fIsSimple = true;
  }

  virtual RColumn* GenerateColumns(RColumnSource *source, RColumnSink *sink)
    override
  {
    fPrincipalColumn = new RColumn(
      RColumnModel(fName, fDescription, EColumnType::kDouble, false),
      source, sink);
    return fPrincipalColumn;
  }
};

template <>
class RBranch<std::int32_t> : public RBranchBase {
public:
  explicit RBranch(std::string_view name) : RBranchBase(name) {
    fIsSimple = true;
  }

  virtual RColumn* GenerateColumns(RColumnSource *source, RColumnSink *sink)
    override
  {
    fPrincipalColumn = new RColumn(
      RColumnModel(fName, fDescription, EColumnType::kInt32, false),
      source, sink);
    return fPrincipalColumn;
  }
};

template <>
class RBranch<OffsetColumn_t> : public RBranchBase {
public:
  explicit RBranch(std::string_view name) : RBranchBase(name) {
    fIsSimple = true;
  }

  virtual RColumn* GenerateColumns(RColumnSource *source, RColumnSink *sink)
    override
  {
    fPrincipalColumn = new RColumn(
      RColumnModel(fName, fDescription, EColumnType::kOffset, false),
      source, sink);
    return fPrincipalColumn;
  }
};

} // namespace Experimental
} // namespace ROOT

#endif
