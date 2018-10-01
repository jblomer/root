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
#include <ROOT/RColumnElement.hxx>
#include <ROOT/RColumnStorage.hxx>
#include <ROOT/RColumnUtil.hxx>
#include <ROOT/RStringView.hxx>
#include <ROOT/RVec.hxx>

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
   std::string fType;
   bool fIsSimple;
   RColumn *fPrincipalColumn;

   explicit RBranchBase(std::string_view name, std::string type)
     : fParent(nullptr)
     , fName(name)
     , fType(type)
     , fIsSimple(false)
     , fPrincipalColumn(nullptr)
   { }

   virtual void DoAppend(RCargoBase *cargo) { assert(false); }
   virtual void DoRead(std::uint64_t num, RCargoBase *cargo) { assert(false); }
   virtual void DoFlush() { assert(false); }
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
  std::string GetType() { return fType; }
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
   virtual RCargoBase* GenerateCargo() = 0;

  void Append(RCargoBase *__restrict__ cargo) {
    if (!fIsSimple) {
      DoAppend(cargo);
      return;
    }
    //std::cout << "Append branch " << fName << " with element "
    //          << cargo->fPrincipalElement->GetRawContent() << std::endl;
    fPrincipalColumn->Append(*(cargo->fPrincipalElement));
  }

  void Flush() {
     if (!fIsSimple) {
        DoFlush();
        return;
     }
     fPrincipalColumn->Flush();
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
    assert(fIsSimple);  // TODO
    fPrincipalColumn->ReadV(start, num, dst);
  }

  std::uint64_t GetNItems() {
    return fPrincipalColumn->GetNElements();
  }

  RColumn *GetPrincipalColumn() {
    return fPrincipalColumn;
  }
};

class RBranchCollectionTag {};

template <typename T>
class RBranch : public RBranchBase {
};

template <>
class RBranch<RBranchCollectionTag> : public RBranchBase {
public:
  RBranch() : RBranchBase("", gColumnTypeNames[int(EColumnType::kOffset)]) {
    fIsSimple = true;
  }
  explicit RBranch(std::string_view name) : RBranchBase(name, gColumnTypeNames[int(EColumnType::kOffset)]) {
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

  RCargoBase* GenerateCargo() final { assert(false); return nullptr; }

  bool IsRoot() { return fName.empty(); }
  void MakeSubBranch(std::string_view name) { fName = std::string(name); }
};


template <typename T>
class RBranch<std::vector<T>> : public RBranchBase {
private:
  RColumn* fValueColumn;
  OffsetColumn_t fIndex;
  T fValue;
  RColumnElement<OffsetColumn_t> fElementIndex;
  RColumnElement<T> fElementValue;

public:
  RBranch()
    : RBranchBase("", "std::vector<" + gColumnTypeNames[MakeColumnType<T>()] + ">")
    , fValueColumn(nullptr)
    , fIndex(0)
    , fValue()
    , fElementIndex(&fIndex)
    , fElementValue(&fValue)
  {
    fIsSimple = false;
  }
  explicit RBranch(std::string_view name)
    : RBranchBase(name, "std::vector<" + gColumnTypeNames[MakeColumnType<T>()] + ">")
    , fValueColumn(nullptr)
    , fIndex(0)
    , fValue()
    , fElementIndex(&fIndex)
    , fElementValue(&fValue)
  {
    //std::cout << "NAME " << fParentName << "/" << fChildName << std::endl;
    fIsSimple = false;
  }

  RCargoBase* GenerateCargo() final { return new RCargo<std::vector<T>>(this); }

  virtual RColumn* GenerateColumns(RColumnSource *source, RColumnSink *sink)
    override
  {
    std::string fParentName;
    std::string fChildName;
    auto sep = fName.find('/');
    if (sep == std::string::npos) {
      fParentName = fName;
      fChildName = "@1";
    } else {
      fParentName = fName.substr(0, sep);
      fChildName = fName.substr(sep + 1);
    }

    fPrincipalColumn = new RColumn(
      RColumnModel(fParentName, fDescription, EColumnType::kOffset, false),
      source, sink);
    fValueColumn = new RColumn(
      RColumnModel(fParentName + "/" + fChildName, fDescription,
        MakeColumnType<T>(), false),
      source, sink);

    return fPrincipalColumn;
  }

  void DoAppend(RCargoBase *cargo) final {
    RCargo<std::vector<T>>* cargo_vec = reinterpret_cast<RCargo<std::vector<T>>*>(cargo);
    std::vector<T>* valueVec = cargo_vec->Get().get();
    for (auto element : *valueVec) {
       // TODO: less copies, append multi
       //std::cout << "APPENDING " << element << " to VALUE COLUMN" << std::endl;
       fValue = element;
       fValueColumn->Append(fElementValue);
    }
    cargo_vec->fOffset += valueVec->size();
    fIndex = cargo_vec->fOffset;
    fPrincipalColumn->Append(fElementIndex);
  }

  void DoRead(std::uint64_t num, RCargoBase *cargo) final {
    RCargo<std::vector<T>>* cargo_vec = reinterpret_cast<RCargo<std::vector<T>>*>(cargo);
    if (num == 0) {
      fPrincipalColumn->Read(0, &fElementIndex);
      cargo_vec->Get()->resize(fIndex);
      fValueColumn->ReadV(0, fIndex, cargo_vec->Get()->data());
    } else {
      fPrincipalColumn->Read(num - 1, &fElementIndex);
      OffsetColumn_t prev = fIndex;
      fPrincipalColumn->Read(num, &fElementIndex);
      OffsetColumn_t size = fIndex - prev;
      cargo_vec->Get()->resize(size);
      fValueColumn->ReadV(prev, size, cargo_vec->Get()->data());
    }
  }

  void DoFlush() final {
    fValueColumn->Flush();
    fPrincipalColumn->Flush();
  }
};


template <typename T>
class RBranch<ROOT::VecOps::RVec<T>> : public RBranchBase {
private:
  RColumn* fValueColumn;
  OffsetColumn_t fIndex;
  T fValue;
  RColumnElement<OffsetColumn_t> fElementIndex;
  RColumnElement<T> fElementValue;

public:
  RBranch()
    : RBranchBase("", "ROOT::VecOps::RVec<" + gColumnTypeNames[MakeColumnType<T>()] + ">")
    , fValueColumn(nullptr)
    , fIndex(0)
    , fValue()
    , fElementIndex(&fIndex)
    , fElementValue(&fValue)
  {
    fIsSimple = false;
  }
  explicit RBranch(std::string_view name)
    : RBranchBase(name, std::string("ROOT::VecOps::RVec<") + gColumnTypeNames[int(MakeColumnType<T>())] + ">")
    , fValueColumn(nullptr)
    , fIndex(0)
    , fValue()
    , fElementIndex(&fIndex)
    , fElementValue(&fValue)
  {
    //std::cout << "NAME " << fParentName << "/" << fChildName << std::endl;
    fIsSimple = false;
  }

  RCargoBase* GenerateCargo() final { return new RCargo<ROOT::VecOps::RVec<T>>(this); }

  virtual RColumn* GenerateColumns(RColumnSource *source, RColumnSink *sink)
    override
  {
    std::string fParentName;
    std::string fChildName;
    auto sep = fName.find('/');
    if (sep == std::string::npos) {
      fParentName = fName;
      fChildName = "@1";
    } else {
      fParentName = fName.substr(0, sep);
      fChildName = fName.substr(sep + 1);
    }

    fPrincipalColumn = new RColumn(
      RColumnModel(fParentName, fDescription, EColumnType::kOffset, false),
      source, sink);
    fValueColumn = new RColumn(
      RColumnModel(fParentName + "/" + fChildName, fDescription,
        MakeColumnType<T>(), false),
      source, sink);

    return fPrincipalColumn;
  }

  void DoAppend(RCargoBase *cargo) final {
    RCargo<ROOT::VecOps::RVec<T>>* cargo_vec = reinterpret_cast<RCargo<ROOT::VecOps::RVec<T>>*>(cargo);
    ROOT::VecOps::RVec<T>* valueVec = cargo_vec->Get().get();
    for (auto element : *valueVec) {
       // TODO: less copies, append multi
       //std::cout << "APPENDING " << element << " to VALUE COLUMN" << std::endl;
       fValue = element;
       fValueColumn->Append(fElementValue);
    }
    cargo_vec->fOffset += valueVec->size();
    fIndex = cargo_vec->fOffset;
    fPrincipalColumn->Append(fElementIndex);
  }

  void DoRead(std::uint64_t num, RCargoBase *cargo) final {
    RCargo<ROOT::VecOps::RVec<T>>* cargo_vec = reinterpret_cast<RCargo<ROOT::VecOps::RVec<T>>*>(cargo);
    if (num == 0) {
      fPrincipalColumn->Read(0, &fElementIndex);
      cargo_vec->Get()->resize(fIndex);
      fValueColumn->ReadV(0, fIndex, cargo_vec->Get()->data());
    } else {
      fPrincipalColumn->Read(num - 1, &fElementIndex);
      OffsetColumn_t prev = fIndex;
      fPrincipalColumn->Read(num, &fElementIndex);
      OffsetColumn_t size = fIndex - prev;
      //T* buf;
      //std::uint64_t mapped = fValueColumn->MapV(prev, size, (void **)&buf);
      //if (mapped < size) {
        cargo_vec->Get()->resize(size);
        fValueColumn->ReadV(prev, size, cargo_vec->Get()->data());
      //} else {
      //  ROOT::VecOps::RVec<T> vec_mapped(buf, mapped);
      //  *cargo_vec->Get() = std::move(vec_mapped);
      //}
    }
  }

  void DoFlush() final {
    fValueColumn->Flush();
    fPrincipalColumn->Flush();
  }
};


template <>
class RBranch<float> : public RBranchBase {
public:
  explicit RBranch(std::string_view name) : RBranchBase(name, "float") {
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

  RCargoBase* GenerateCargo() final { return new RCargo<float>(this); }
};

template <>
class RBranch<double> : public RBranchBase {
public:
  explicit RBranch(std::string_view name) : RBranchBase(name, "double") {
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

  RCargoBase* GenerateCargo() final { return new RCargo<double>(this); }
};

template <>
class RBranch<std::int32_t> : public RBranchBase {
public:
  explicit RBranch(std::string_view name) : RBranchBase(name, "std::int32_t") {
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

  RCargoBase* GenerateCargo() final { return new RCargo<std::int32_t>(this); }
};

template <>
class RBranch<OffsetColumn_t> : public RBranchBase {
public:
  explicit RBranch(std::string_view name) : RBranchBase(name, "ROOT::Experimental::OffsetColumn_t") {
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

  RCargoBase* GenerateCargo() final { return new RCargo<OffsetColumn_t>(this); }
};

} // namespace Experimental
} // namespace ROOT

#endif
