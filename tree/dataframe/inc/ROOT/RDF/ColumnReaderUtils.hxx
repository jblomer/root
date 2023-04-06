// Author: Enrico Guiraud CERN 09/2020

/*************************************************************************
 * Copyright (C) 1995-2020, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_RDF_COLUMNREADERUTILS
#define ROOT_RDF_COLUMNREADERUTILS

#include "RBulkScalarReader.hxx"
#include "RBulkStaticArrayReader.hxx"
#include "RBulkDynamicArrayReader.hxx"
#include "RColumnReaderBase.hxx"
#include "RColumnRegister.hxx"
#include "RDefineBase.hxx"
#include "RDefineReader.hxx"
#include "RDSColumnReader.hxx"
#include "RLoopManager.hxx"
#include "RTreeColumnReader.hxx"
#include "RVariationBase.hxx"
#include "RVariationReader.hxx"

#include <ROOT/RDataSource.hxx>
#include <ROOT/TypeTraits.hxx>
#include <TTreeReader.h>

#include <array>
#include <cassert>
#include <map>
#include <memory>
#include <string>
#include <typeinfo> // for typeid
#include <vector>

namespace ROOT {
namespace Internal {
namespace RDF {

using namespace ROOT::TypeTraits;
namespace RDFDetail = ROOT::Detail::RDF;

// Check whether the branch is simple enough that we can read its contents via bulk I/O.
// Return the pointer to the branch if we can do bulk I/O, nullptr otherwise.
inline TBranch *CanUseBulkIO(TTreeReader &r, const std::string &colName)
{
   TBranch *b = r.GetTree()->GetBranch(colName.c_str());
   if (!b) // try harder, with FindBranch
      b = r.GetTree()->FindBranch(colName.c_str());
   if (!b) {
      const auto msg = "Could not find branch corresponding to column " + colName + ". This should never happen.";
      throw std::runtime_error(std::move(msg));
   }

   if (!b->SupportsBulkRead())
      return nullptr;

   TLeaf *l = static_cast<TLeaf *>(b->GetListOfLeaves()->UncheckedAt(0));
   TLeaf *lc = l->GetLeafCount();
   if (lc && l->GetLenStatic() > 1)
      return nullptr; // bulk I/O supports 2D arrays (1 static and one dynamic dimension) but RDF does not

   // otherwise we are ok for bulk I/O of this branch so we return a non-null branch address
   return b;
}

template <typename T>
RDFDetail::RColumnReaderBase *
MakeTreeReaderColumnReader(TTreeReader &r, const std::string &colName, unsigned int slot, RLoopManager &lm)
{
   auto treeColReader = std::make_unique<RTreeColumnReader<T>>(r, colName, lm.GetMaxEventsPerBulk());
   return lm.AddTreeColumnReader(slot, colName, std::move(treeColReader), typeid(T));
}

// fwd decl for MakeBulkColumnReader
template <typename T>
RDFDetail::RColumnReaderBase *GetColumnReader(unsigned int slot, RColumnReaderBase *defineOrVariationReader,
                                              RLoopManager &lm, TTreeReader *r, const std::string &colName);

// Overload for scalar column types.
template <typename T>
RDFDetail::RColumnReaderBase *MakeBulkColumnReader(unsigned int slot, RLoopManager &lm, TTreeReader &, TBranch &branch,
                                                   const std::string &colName, T *)
{
   std::unique_ptr<RColumnReaderBase> bulkReader{new RBulkScalarReader<T>(branch, lm.GetMaxEventsPerBulk())};
   return lm.AddTreeColumnReader(slot, colName, std::move(bulkReader), typeid(T));
}

// this overload is SFINAE-d out if there is no available overload for `frombuf(char*&, ColType*)`, because
// RBulkDynamicArrayReader needs that overload to be present.
template <typename ColType, typename SizeType,
          typename SupportsFromBuf = decltype(frombuf(*std::declval<char **>(),
                                                      (typename ColType::value_type *)nullptr))>
std::unique_ptr<RColumnReaderBase> MakeBulkDynamicArrayReader(unsigned int slot, RLoopManager &lm, TTreeReader &r,
                                                              const std::string &sizeColName, TBranch &branch)
{
   RColumnReaderBase *sizeReader = GetColumnReader<SizeType>(slot, nullptr, lm, &r, sizeColName);
   return std::unique_ptr<RColumnReaderBase>{
      new RBulkDynamicArrayReader<ColType, SizeType>(branch, *sizeReader, lm.GetMaxEventsPerBulk())};
}

// a fallback overload that should never be called, it's only here to make the code compile in cases in which
// RBulkDynamicArrayReader<ColType, SizeType> would be ill-formed (in which case it would not be used at runtime).
template <typename ColType, typename SizeType, typename... Args>
std::unique_ptr<RColumnReaderBase>
MakeBulkDynamicArrayReader(unsigned int, RLoopManager &, TTreeReader &, const std::string &, Args &...)
{
   std::abort();
   return nullptr;
}

// Overload for array column types.
template <typename T>
RDFDetail::RColumnReaderBase *MakeBulkColumnReader(unsigned int slot, RLoopManager &lm, TTreeReader &r, TBranch &branch,
                                                   const std::string &colName, ROOT::RVec<T> *)
{
   std::unique_ptr<RColumnReaderBase> bulkReader;

   // if the branch supports bulk reading, we must have one leaf
   TLeaf *l = static_cast<TLeaf *>(branch.GetListOfLeaves()->UncheckedAt(0));
   TLeaf *lc = l->GetLeafCount();

   if (!lc) { // array with static size
      assert(l->GetLenStatic() > 1);
      bulkReader.reset(new RBulkStaticArrayReader<ROOT::RVec<T>>(branch, lm.GetMaxEventsPerBulk()));
   } else { // array with dynamic size
      const auto sizeColName = std::string(lc->GetBranch()->GetFullName());
      const std::string leafType = lc->GetTypeName();
      if (leafType == "Int_t") {
         bulkReader = MakeBulkDynamicArrayReader<ROOT::RVec<T>, Int_t>(slot, lm, r, sizeColName, branch);
      } else if (leafType == "UInt_t") {
         bulkReader = MakeBulkDynamicArrayReader<ROOT::RVec<T>, UInt_t>(slot, lm, r, sizeColName, branch);
      } else if (leafType == "Short_t") {
         bulkReader = MakeBulkDynamicArrayReader<ROOT::RVec<T>, Short_t>(slot, lm, r, sizeColName, branch);
      } else if (leafType == "UShort_t") {
         bulkReader = MakeBulkDynamicArrayReader<ROOT::RVec<T>, UShort_t>(slot, lm, r, sizeColName, branch);
      } else if (leafType == "Long_t") {
         bulkReader = MakeBulkDynamicArrayReader<ROOT::RVec<T>, Long_t>(slot, lm, r, sizeColName, branch);
      } else if (leafType == "ULong_t") {
         bulkReader = MakeBulkDynamicArrayReader<ROOT::RVec<T>, ULong_t>(slot, lm, r, sizeColName, branch);
      } else if (leafType == "Long64_t") {
         bulkReader = MakeBulkDynamicArrayReader<ROOT::RVec<T>, Long64_t>(slot, lm, r, sizeColName, branch);
      } else if (leafType == "ULong64_t") {
         bulkReader = MakeBulkDynamicArrayReader<ROOT::RVec<T>, ULong64_t>(slot, lm, r, sizeColName, branch);
      }
   }

   return lm.AddTreeColumnReader(slot, colName, std::move(bulkReader), typeid(T));
}

// never returns a nullptr
template <typename T>
RDFDetail::RColumnReaderBase *GetColumnReader(unsigned int slot, RColumnReaderBase *defineOrVariationReader,
                                              RLoopManager &lm, TTreeReader *r, const std::string &colName)
{
   if (defineOrVariationReader != nullptr)
      return defineOrVariationReader;

   // Check if we already inserted a reader for this column in the dataset column readers (RDataSource or Tree/TChain
   // readers)
   auto *datasetColReader = lm.GetDatasetColumnReader(slot, colName, typeid(T));
   if (datasetColReader != nullptr)
      return datasetColReader;

   assert(r != nullptr && "We could not find a reader for this column, this should never happen at this point.");

   TBranch *b = CanUseBulkIO(*r, colName);
   if (b)
      return MakeBulkColumnReader(slot, lm, *r, *b, colName, (T *)nullptr);

   // otherwise use a normal (slower) RTreeColumnReader backed by a TTreeReaderValue or a TTreeReaderArray.
   return MakeTreeReaderColumnReader<T>(*r, colName, slot, lm);
}

/// This type aggregates some of the arguments passed to GetColumnReaders.
/// We need to pass a single RColumnReadersInfo object rather than each argument separately because with too many
/// arguments passed, gcc 7.5.0 and cling disagree on the ABI, which leads to the last function argument being read
/// incorrectly from a compiled GetColumnReaders symbols when invoked from a jitted symbol.
struct RColumnReadersInfo {
   const std::vector<std::string> &fColNames;
   RColumnRegister &fColRegister;
   const bool *fIsDefine;
   RLoopManager &fLoopManager;
};

/// Create a group of column readers, one per type in the parameter pack.
template <typename... ColTypes>
std::array<RDFDetail::RColumnReaderBase *, sizeof...(ColTypes)>
GetColumnReaders(unsigned int slot, TTreeReader *r, TypeList<ColTypes...>, const RColumnReadersInfo &colInfo,
                 const std::string &variationName = "nominal")
{
   // see RColumnReadersInfo for why we pass these arguments like this rather than directly as function arguments
   const auto &colNames = colInfo.fColNames;
   auto &lm = colInfo.fLoopManager;
   auto &colRegister = colInfo.fColRegister;

   int i = -1;
   std::array<RDFDetail::RColumnReaderBase *, sizeof...(ColTypes)> ret{
      (++i, GetColumnReader<ColTypes>(slot, colRegister.GetReader(slot, colNames[i], variationName, typeid(ColTypes)),
                                      lm, r, colNames[i]))...};
   return ret;
}

// Shortcut overload for the case of no columns
inline std::array<RDFDetail::RColumnReaderBase *, 0>
GetColumnReaders(unsigned int, TTreeReader *, TypeList<>, const RColumnReadersInfo &, const std::string & = "nominal")
{
   return {};
}

} // namespace RDF
} // namespace Internal
} // namespace ROOT

#endif // ROOT_RDF_COLUMNREADERS
