/// \file RNTupleDS.cxx
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2018-10-04
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleOptions.hxx>
#include <ROOT/RNTupleUtil.hxx>
#include <ROOT/RNTupleDS.hxx>
#include <ROOT/RStringView.hxx>

#include <TError.h>

#include <string>
#include <vector>
#include <typeinfo>
#include <unordered_map>
#include <utility>

namespace {

std::string BuildTypeName(const ROOT::Experimental::RNTupleDescriptor &ntupleDesc,
                          const ROOT::Experimental::RFieldDescriptor &fieldDesc,
                          const std::string &innerType)
{
   if (fieldDesc.GetParentId() == ROOT::Experimental::kInvalidDescriptorId) {
      return innerType;
   }

   const auto &parentDesc = ntupleDesc.GetFieldDescriptor(fieldDesc.GetParentId());
   switch (parentDesc.GetStructure()) {
   case ROOT::Experimental::ENTupleStructure::kCollection:
      return BuildTypeName(ntupleDesc, parentDesc, "std::vector<" + innerType + ">");
   case ROOT::Experimental::ENTupleStructure::kRecord:
      return BuildTypeName(ntupleDesc, parentDesc, innerType);
   default:
      return "";
   }
}

ROOT::Experimental::Detail::RFieldBase *
BuildField(const ROOT::Experimental::RNTupleDescriptor &ntupleDesc,
           const ROOT::Experimental::RFieldDescriptor &fieldDesc,
           const std::string &fieldAlias,
           const std::string &fieldType,
           ROOT::Experimental::Detail::RPageSource &pageSource,
           ROOT::Experimental::Detail::RFieldBase *innerField)
{
   if (fieldDesc.GetParentId() == ROOT::Experimental::kInvalidDescriptorId) {
      return innerField;
   }

   if (!innerField) {
      auto useType = fieldType.empty() ? fieldDesc.GetTypeName() : fieldType;
      innerField = ROOT::Experimental::Detail::RFieldBase::Create(fieldAlias, useType);
      ROOT::Experimental::Detail::RFieldFuse::Connect(fieldDesc.GetId(), pageSource, *innerField);

      std::unordered_map<const ROOT::Experimental::Detail::RFieldBase *, ROOT::Experimental::DescriptorId_t> field2Id;
      field2Id[innerField] = fieldDesc.GetId();
      for (auto &f : *innerField) {
         auto subFieldId = ntupleDesc.FindFieldId(f.GetName(), field2Id[f.GetParent()]);
         ROOT::Experimental::Detail::RFieldFuse::Connect(subFieldId, pageSource, f);
         field2Id[&f] = subFieldId;
      }
   }

   ROOT::Experimental::Detail::RFieldBase *parentField = nullptr;
   const auto &parentDesc = ntupleDesc.GetFieldDescriptor(fieldDesc.GetParentId());
   switch (parentDesc.GetStructure()) {
   case ROOT::Experimental::ENTupleStructure::kCollection:
      parentField = new ROOT::Experimental::RFieldVector(
         parentDesc.GetFieldName(), std::unique_ptr<ROOT::Experimental::Detail::RFieldBase>(innerField));
      ROOT::Experimental::Detail::RFieldFuse::Connect(parentDesc.GetId(), pageSource, *parentField);
      return BuildField(ntupleDesc, parentDesc, "", "", pageSource, parentField);
   case ROOT::Experimental::ENTupleStructure::kRecord:
      return BuildField(ntupleDesc, parentDesc, "", "", pageSource, innerField);
   default:
      return nullptr;
   }
}

} // anonymous namespace

namespace ROOT {
namespace Experimental {

ROOT::Experimental::RNTupleDS::RNTupleDS(std::unique_ptr<Detail::RPageSource> pageSource)
{
   pageSource->Attach();
   const auto &descriptor = pageSource->GetDescriptor();

   // TODO(jblomer): field range
   auto rootId = descriptor.FindRootFieldId();
   for (unsigned int i = 0; i < descriptor.GetNFields(); ++i) {
      if (i == rootId)
         continue;
      const auto &fieldDesc = descriptor.GetFieldDescriptor(i);
      fColumnNames.emplace_back(descriptor.GetQualifiedFieldName(i));
      fColumnTypes.emplace_back(BuildTypeName(descriptor, fieldDesc, fieldDesc.GetTypeName()));
      std::cout << *fColumnNames.rbegin() << " " << *fColumnTypes.rbegin() << std::endl;
      fColumnFieldIds.emplace_back(i);
      if (fieldDesc.GetStructure() == ENTupleStructure::kCollection) {
         fColumnNames.emplace_back(descriptor.GetQualifiedFieldName(i) + "_");
         fColumnTypes.emplace_back(BuildTypeName(descriptor, fieldDesc, "ROOT::Experimental::ClusterSize_t"));
         std::cout << *fColumnNames.rbegin() << " " << *fColumnTypes.rbegin() << std::endl;
         fColumnFieldIds.emplace_back(i);
      }
   }
   fSources.emplace_back(std::move(pageSource));
}

const std::vector<std::string>& RNTupleDS::GetColumnNames() const
{
   return fColumnNames;
}


RDF::RDataSource::Record_t RNTupleDS::GetColumnReadersImpl(std::string_view name, const std::type_info& /* ti */)
{
   const auto colIdx = std::distance(
      fColumnNames.begin(), std::find(fColumnNames.begin(), fColumnNames.end(), name));
   // TODO(jblomer): check expected type info like in, e.g., RRootDS.cxx
   // There is a problem extracting the type info for std::int32_t and company though

   std::cout << "TRY CONSTRUCTING " << name << std::endl;

   std::vector<void *> ptrs;
   for (unsigned int slot = 0; slot < fNSlots; ++slot) {
      if (!fValuePtrs[slot][colIdx]) {
         const auto &descriptor = fSources[slot]->GetDescriptor();
         const auto &fieldDesc = descriptor.GetFieldDescriptor(fColumnFieldIds[colIdx]);
         auto colName = fColumnNames[colIdx];

         std::string fieldName = colName;
         std::string fieldType = "";
         //auto fieldName = fieldDesc.GetFieldName();
         //auto fieldType = fieldDesc.GetTypeName();
         if (colName[colName.length() - 1] == '_') {
            fieldName = colName;
            fieldType = fColumnTypes[colIdx];
         }
         std::cout << "  AS " << fieldName << " " << fieldType << std::endl;
         fFields[slot][colIdx] = std::unique_ptr<Detail::RFieldBase>(
            BuildField(descriptor, fieldDesc, fieldName, fieldType, *fSources[slot], nullptr));
         std::cout << "  Really AS " << fFields[slot][colIdx]->GetType() << std::endl;
         fValues[slot][colIdx] = fFields[slot][colIdx]->GenerateValue();
         fValuePtrs[slot][colIdx] = fValues[slot][colIdx].GetRawPtr();
         if (slot == 0)
            fActiveColumns.emplace_back(colIdx);
      }
      ptrs.push_back(&fValuePtrs[slot][colIdx]);
   }

   return ptrs;
}

bool RNTupleDS::SetEntry(unsigned int slot, ULong64_t entryIndex)
{
   for (auto colIdx : fActiveColumns) {
      //std::cout << "READ " <<  fColumnNames[colIdx] << "/" << fFields[slot][colIdx]->GetType() << " / "
      //   << entryIndex << std::endl;
      fFields[slot][colIdx]->Read(entryIndex, &fValues[slot][colIdx]);
   }
   return true;
}

std::vector<std::pair<ULong64_t, ULong64_t>> RNTupleDS::GetEntryRanges()
{
   // TODO(jblomer): use cluster boundaries for the entry ranges
   std::vector<std::pair<ULong64_t, ULong64_t>> ranges;
   if (fHasSeenAllRanges) return ranges;

   auto nEntries = fSources[0]->GetNEntries();
   const auto chunkSize = nEntries / fNSlots;
   const auto reminder = 1U == fNSlots ? 0 : nEntries % fNSlots;
   auto start = 0UL;
   auto end = 0UL;
   for (auto i : ROOT::TSeqU(fNSlots)) {
      start = end;
      end += chunkSize;
      ranges.emplace_back(start, end);
      (void)i;
   }
   ranges.back().second += reminder;
   fHasSeenAllRanges = true;
   return ranges;
}


std::string RNTupleDS::GetTypeName(std::string_view colName) const
{
   const auto index = std::distance(
      fColumnNames.begin(), std::find(fColumnNames.begin(), fColumnNames.end(), colName));
   return fColumnTypes[index];
}


bool RNTupleDS::HasColumn(std::string_view colName) const
{
   return std::find(fColumnNames.begin(), fColumnNames.end(), colName) !=
          fColumnNames.end();
}


void RNTupleDS::Initialise()
{
   std::cout << "INITIALIZE" << std::endl;
   fHasSeenAllRanges = false;
}


void RNTupleDS::SetNSlots(unsigned int nSlots)
{
   R__ASSERT(fNSlots == 0);
   R__ASSERT(nSlots > 0);
   fNSlots = nSlots;

   fFields.resize(fNSlots);
   fValues.resize(fNSlots);
   fValuePtrs.resize(fNSlots);
   for (unsigned int i = 1; i < fNSlots; ++i) {
      fSources.emplace_back(fSources[0]->Clone());
      R__ASSERT(i == (fSources.size() - 1));
      fSources[i]->Attach();
   }

   auto nColumns = fColumnNames.size();
   for (unsigned int i = 0; i < fNSlots; ++i) {
      fFields[i].resize(nColumns);
      fValues[i].resize(nColumns);
      fValuePtrs[i].resize(nColumns, nullptr);
   }
}


RDataFrame MakeNTupleDataFrame(std::string_view ntupleName, std::string_view fileName)
{
   const RNTupleReadOptions options;
   auto pageSource = Detail::RPageSource::Create(ntupleName, fileName, options);
   ROOT::RDataFrame rdf(std::make_unique<RNTupleDS>(std::move(pageSource)));
   return rdf;
}

} // ns Experimental
} // ns ROOT
