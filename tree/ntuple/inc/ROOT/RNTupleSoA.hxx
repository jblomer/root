/// \file ROOT/RNTupleSoA.hxx
/// \ingroup NTuple
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2025-09-08

/*************************************************************************
 * Copyright (C) 1995-2025, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_RNTupleSoA
#define ROOT_RNTupleSoA

#include <ROOT/RVec.hxx>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace ROOT {
namespace Experimental {

class RNTupleSoAMetaInfo {
   const std::type_info *fRecordTypeInfo = nullptr;
   std::string fRecordTypeName;
   std::vector<std::string> fColumnNames;
   std::vector<std::string> fColumnTypes;
   std::vector<std::size_t> fColumnOffsets;

   std::unordered_map<std::string, std::size_t> fColumnName2Idx;

public:
   //template <typename RecordT>
   //static RNTupleSoAMetaInfo *Get()
   //{
   //}

   static RNTupleSoAMetaInfo *Get(const std::string &typeName);
};

class RNTupleSoALayoutRVecTag;

template <typename RecordT, typename LayoutT = RNTupleSoALayoutRVecTag>
class RNTupleSoA {
public:
   std::size_t fColumnLength = 0;
   RNTupleSoAMetaInfo *fSoAMetaInfo = nullptr;
   std::unique_ptr<unsigned char []> fSoA;
   std::unique_ptr<unsigned char []> fMemory;
   bool fIsOwning = false;

public:
   using Record_t = RecordT;
   using Layout_t = LayoutT;

   template <typename SoAT>
   SoAT &GetSoA() const
   {
      // runtime check if SoAT matches fSoA
   }

   template <typename ColumnT>
   ROOT::RVec<ColumnT> &GetRVec(std::string_view name)
   {

   }

   template <typename ColumnT>
   ColumnT *GetPtr(std::string_view name)
   {
      // runtime check if column type matches name
      return nullptr;
   }

   std::size_t GetColumnLength() const { return fColumnLength; }
   bool IsOwning() const { return fIsOwning; }
};

} // namespace Experimental
} // namespace ROOT

#endif
