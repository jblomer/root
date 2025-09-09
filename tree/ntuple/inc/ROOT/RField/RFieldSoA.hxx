/// \file ROOT/RField/RFieldSoA.hxx
/// \ingroup NTuple
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2025-09-11

/*************************************************************************
 * Copyright (C) 1995-2025, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_RField_SoA
#define ROOT_RField_SoA

#ifndef ROOT_RField
#error "Please include RField.hxx!"
#endif

#include <ROOT/RFieldBase.hxx>
#include <ROOT/RNTupleTypes.hxx>

namespace ROOT {
namespace Experimental {

class RSoAField : public RFieldBase {
protected:
   std::unique_ptr<RFieldBase> CloneImpl(std::string_view newName) const final;

   void ConstructValue(void *where) const final;
   std::unique_ptr<RDeleter> GetDeleter() const final;

   std::size_t AppendImpl(const void *from) final;
   void ReadGlobalImpl(ROOT::NTupleSize_t globalIndex, void *to) final;
   void ReadInClusterImpl(RNTupleLocalIndex localIndex, void *to) final;

   void ReconcileOnDiskField(const RNTupleDescriptor &desc) final;

public:
   RSoAField(std::string_view fieldName, std::string_view typeName, std::unique_ptr<RFieldBase> itemField);
   RSoAField(RAtomicField &&other) = default;
   RSoAField &operator=(RAtomicField &&other) = default;
   ~RSoAField() override = default;

   std::vector<RValue> SplitValue(const RValue &value) const final;

   size_t GetValueSize() const final;
   size_t GetAlignment() const final;

   void AcceptVisitor(ROOT::Detail::RFieldVisitor &visitor) const final;
};

}
}

#endif
