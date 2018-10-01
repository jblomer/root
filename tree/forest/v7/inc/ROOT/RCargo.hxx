/// \file ROOT/RCargoh.hxx
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

#ifndef ROOT7_RCargo
#define ROOT7_RCargo

#include <ROOT/RColumnElement.hxx>
#include <ROOT/RColumnUtil.hxx>
#include <ROOT/RVec.hxx>

#include <cassert>
#include <memory>
#include <utility>
#include <vector>


namespace ROOT {
namespace Experimental {

class RBranchBase;

/**
 * The "cargo" represents transient storage of simple or complex
 * C++ values that are supposed to be serialized on Fill or that just
 * have been deserialized for reading.
 */
// TODO: this class should manage a vector of things
class RCargoBase {
  friend class RBranchBase;
  friend class RCargoSubtree;

protected:
  RBranchBase* fBranch;
  RColumnElementBase *fPrincipalElement;
  bool fIsSimple;
  void *fRawPtr;

  RCargoBase(RBranchBase *branch)
    : fBranch(branch)
    , fIsSimple(false)
    , fRawPtr(nullptr)
  { }

public:
   virtual ~RCargoBase() { }

   RBranchBase* GetBranch() { return fBranch; }
   void** GetRawPtrPtr() { return &fRawPtr; }
};

using CargoCollection = std::vector<std::shared_ptr<RCargoBase>>;


template <typename T>
class RCargo : public RCargoBase {
   std::shared_ptr<T> fValue;

   void Init();

public:
   template <typename... ArgsT>
   RCargo(RBranchBase *branch, ArgsT&&... args) : RCargoBase(branch) {
     fValue = std::make_shared<T>(std::forward<ArgsT>(args)...);
     Init();
     fRawPtr = fValue.get();
   }

   std::shared_ptr<T> Get() { return fValue; }
};


template <typename T>
class RCargoCaptured : public RCargoBase {
   T *fValue;

   void Init();

public:
   RCargoCaptured(RBranchBase *branch, T *value)
     : RCargoBase(branch)
     , fValue(value)
   {
     Init();
     fRawPtr = fValue;
   }

   T *Get() { return fValue; }
};


class RCargoSubtree : public RCargoBase {
  std::uint64_t fOffset;
  CargoCollection fChildren;

  void Init();

public:
  RCargoSubtree(RBranchBase *branch, const CargoCollection &entry)
    : RCargoBase(branch)
    , fOffset(0)
  {
    //std::cout << "Making principal element from " << &fOffset << std::endl;
    fPrincipalElement = new RColumnElement<OffsetColumn_t>(&fOffset);
    fIsSimple = true;
    fChildren = entry;
  }

  void Fill();
  void FillV(RCargoBase **leafs, unsigned size);
};


template <typename T>
class RCargo<std::vector<T>> : public RCargoBase {
   std::shared_ptr<std::vector<T>> fValue;

public:
   OffsetColumn_t fOffset; // TODO Make me private

   template <typename... ArgsT>
   RCargo(RBranchBase *branch, ArgsT&&... args) : RCargoBase(branch) {
     fValue = std::make_shared<std::vector<T>>(std::forward<ArgsT>(args)...);
     fOffset = 0;
     fRawPtr = fValue.get();
   }

   std::shared_ptr<std::vector<T>> Get() { return fValue; }
};


template <typename T>
class RCargo<ROOT::VecOps::RVec<T>> : public RCargoBase {
   std::shared_ptr<ROOT::VecOps::RVec<T>> fValue;

public:
   OffsetColumn_t fOffset; // TODO Make me private

   template <typename... ArgsT>
   RCargo(RBranchBase *branch, ArgsT&&... args) : RCargoBase(branch) {
     fValue = std::make_shared<ROOT::VecOps::RVec<T>>(std::forward<ArgsT>(args)...);
     fOffset = 0;
     fRawPtr = fValue.get();
   }

   std::shared_ptr<ROOT::VecOps::RVec<T>> Get() { return fValue; }
};


} // namespace Experimental
} // namespace ROOT

#endif
