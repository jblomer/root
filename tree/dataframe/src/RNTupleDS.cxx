/// \file RNTupleDS.cxx
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \author Enrico Guiraud <enrico.guiraud@cern.ch>
/// \date 2018-10-04
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2020, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <ROOT/RDF/RColumnReaderBase.hxx>
#include <ROOT/RField.hxx>
#include <ROOT/RFieldValue.hxx>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RNTupleDS.hxx>
#include <ROOT/RNTupleUtil.hxx>
#include <ROOT/RPageStorage.hxx>
#include <ROOT/RStringView.hxx>

#include <TError.h>

#include <cstdlib>
#include <string>
#include <vector>
#include <typeinfo>
#include <utility>

// clang-format off
/**
* \class ROOT::Experimental::RNTupleDS
* \ingroup dataframe
* \brief The RDataSource implementation for RNTuple. It lets RDataFrame read RNTuple data.
*
* An RDataFrame that reads RNTuple data can be constructed using FromRNTuple().
*
* For each column containing an array or a collection, a corresponding column `#colname` is available to access
* `colname.size()` without reading and deserializing the collection values.
*
**/
// clang-format on

namespace ROOT {
namespace Experimental {
namespace Internal {

/// An artificial field that transforms an RNTuple column that contains the offset of collections into
/// collection sizes. It is used to provide the "number of" RDF columns for collections, e.g.
/// `R_rdf_sizeof_jets` for a collection named `jets`.
///
/// This field owns the collection offset field but instead of exposing the collection offsets it exposes
/// the collection sizes (offset(N+1) - offset(N)).  For the time being, we offer this functionality only in RDataFrame.
/// TODO(jblomer): consider providing a general set of useful virtual fields as part of RNTuple.
class RRDFCardinalityField : public ROOT::Experimental::Detail::RFieldBase {
protected:
   std::unique_ptr<ROOT::Experimental::Detail::RFieldBase> CloneImpl(std::string_view /* newName */) const final
   {
      return std::make_unique<RRDFCardinalityField>();
   }

public:
   static std::string TypeName() { return "std::size_t"; }
   RRDFCardinalityField()
      : ROOT::Experimental::Detail::RFieldBase("", TypeName(), ENTupleStructure::kLeaf, false /* isSimple */) {}
   RRDFCardinalityField(RRDFCardinalityField &&other) = default;
   RRDFCardinalityField &operator=(RRDFCardinalityField &&other) = default;
   ~RRDFCardinalityField() = default;

   const RColumnRepresentations &GetColumnRepresentations() const final
   {
      static RColumnRepresentations representations({{EColumnType::kSplitIndex32}, {EColumnType::kIndex32}}, {{}});
      return representations;
   }
   // Field is only used for reading
   void GenerateColumnsImpl() final { assert(false && "Cardinality fields must only be used for reading"); }
   void GenerateColumnsImpl(const RNTupleDescriptor &desc) final
   {
      auto onDiskTypes = EnsureCompatibleColumnTypes(desc);
      fColumns.emplace_back(
         ROOT::Experimental::Detail::RColumn::Create<ClusterSize_t>(RColumnModel(onDiskTypes[0]), 0));
   }

   ROOT::Experimental::Detail::RFieldValue GenerateValue(void *where) final
   {
      return ROOT::Experimental::Detail::RFieldValue(this, static_cast<std::size_t *>(where));
   }
   ROOT::Experimental::Detail::RFieldValue CaptureValue(void *where) final
   {
      return ROOT::Experimental::Detail::RFieldValue(true /* captureFlag */, this, where);
   }
   size_t GetValueSize() const final { return sizeof(std::size_t); }
   size_t GetAlignment() const final { return alignof(std::size_t); }

   /// Get the number of elements of the collection identified by globalIndex
   void
   ReadGlobalImpl(ROOT::Experimental::NTupleSize_t globalIndex, ROOT::Experimental::Detail::RFieldValue *value) final
   {
      RClusterIndex collectionStart;
      ClusterSize_t size;
      fPrincipalColumn->GetCollectionInfo(globalIndex, &collectionStart, &size);
      *value->Get<std::size_t>() = size;
   }

   /// Get the number of elements of the collection identified by clusterIndex
   void ReadInClusterImpl(const ROOT::Experimental::RClusterIndex &clusterIndex,
                          ROOT::Experimental::Detail::RFieldValue *value) final
   {
      RClusterIndex collectionStart;
      ClusterSize_t size;
      fPrincipalColumn->GetCollectionInfo(clusterIndex, &collectionStart, &size);
      *value->Get<std::size_t>() = size;
   }
};

/// Every RDF column is represented by exactly one RNTuple field
class RNTupleColumnReader : public ROOT::Detail::RDF::RColumnReaderBase {
   using RFieldBase = ROOT::Experimental::Detail::RFieldBase;
   using RFieldValue = ROOT::Experimental::Detail::RFieldValue;
   using RPageSource = ROOT::Experimental::Detail::RPageSource;

   class RBulk {
      RFieldBase *fField = nullptr;
      unsigned char *fValues = nullptr;
      std::size_t fCapacity = 0;
      std::uint64_t fFirstEntry = 0;
      std::size_t fNEntries = 0;
      std::vector<bool> fMask;
      std::size_t fNSetValues = 0;

   private:
      void ReleaseValues()
      {
         if (fField->GetTraits() & RFieldBase::kTraitTriviallyDestructible) {
            free(fValues);
            return;
         }

         const auto valSize = fField->GetValueSize();
         for (std::size_t i = 0; i < fCapacity; ++i) {
            auto val = fField->CaptureValue(&fValues[i * valSize]);
            fField->DestroyValue(val, true /* dtorOnly */);
         }
         free(fValues);
      }

   public:
      explicit RBulk(RFieldBase *field) : fField(field) {}
      ~RBulk() { ReleaseValues(); }
      RBulk(const RBulk &) = delete;
      RBulk& operator =(const RBulk &) = delete;

      void Reset(std::uint64_t firstEntry, std::size_t nEntries)
      {
         if (fCapacity < nEntries) {
            ReleaseValues();

            fValues = reinterpret_cast<unsigned char *>(
               aligned_alloc(fField->GetAlignment(), nEntries * fField->GetValueSize()));
            fCapacity = nEntries;

            const auto valSize = fField->GetValueSize();
            if (!(fField->GetTraits() & RFieldBase::kTraitTriviallyConstructible)) {
               for (unsigned i = 0; i < fCapacity; ++i) {
                  fField->GenerateValue(&fValues[i * valSize]);
               }
            }
         }

         fFirstEntry = firstEntry;
         fNEntries = nEntries;

         fMask.clear();
         fMask.resize(nEntries, false);
         fNSetValues = 0;
      }

      void SetValueAt(std::size_t idx) { fMask[idx] = true; fNSetValues++; }

      bool ContainsRange(std::uint64_t firstEntry, std::size_t nEntries) const
      {
         return (firstEntry >= fFirstEntry) && ((firstEntry + nEntries) <= (fFirstEntry + fNEntries));
      }

      void *GetValuePtrAt(std::size_t idx) const { return &fValues[idx * fField->GetValueSize()]; }

      std::uint64_t GetFirstEntry() const { return fFirstEntry; }
      std::size_t GetNEntries() const { return fNEntries; }
      const std::vector<bool> &GetMask() const { return fMask; }
      bool GetHasAllValuesSet() const { return fNSetValues == fNEntries; }
      void SetAllValues()
      {
         fMask.clear();
         fMask.resize(fNEntries, true);
         fNSetValues = fNEntries;
      }
   };

   std::unique_ptr<RFieldBase> fField; ///< The field backing the RDF column
   RBulk fBulk;
   std::vector<unsigned char> fRVecData;

public:
   RNTupleColumnReader(std::unique_ptr<RFieldBase> f)
      : fField(std::move(f)), fBulk(fField.get())
   {
   }
   ~RNTupleColumnReader() = default;

   /// Column readers are created as prototype and then cloned for every slot
   std::unique_ptr<RNTupleColumnReader> Clone()
   {
      return std::make_unique<RNTupleColumnReader>(fField->Clone(fField->GetName()));
   }

   /// Connect the field and its subfields to the page source
   void Connect(RPageSource &source)
   {
      fField->ConnectPageSource(source);
      for (auto &f : *fField)
         f.ConnectPageSource(source);
   }

   void *LoadRVec(const ROOT::Internal::RDF::RMaskedEntryRange &mask)
   {
      auto rvecField = dynamic_cast<RRVecField *>(fField.get());
      auto itemField = rvecField->GetSubFields()[0];
      const auto itemSize = itemField->GetValueSize();

      RClusterIndex collectionStart;
      ClusterSize_t collectionSize;
      RClusterIndex itemFirstIndex;
      ClusterSize_t nItems;
      rvecField->GetCollectionInfo(fBulk.GetFirstEntry(), &itemFirstIndex, &collectionSize);
      rvecField->GetCollectionInfo(fBulk.GetFirstEntry() + fBulk.GetNEntries() - 1, &collectionStart, &collectionSize);
      nItems = collectionStart.GetIndex() + collectionSize - itemFirstIndex.GetIndex();

      fRVecData.resize(nItems * itemSize);
      auto val = itemField->CaptureValue(fRVecData.data());
      itemField->fPrincipalColumn->ReadV(itemFirstIndex, nItems, &val.fMappedElement);

      unsigned offset = 0;
      for (unsigned i = 0; i < fBulk.GetNEntries(); ++i) {
         rvecField->GetCollectionInfo(fBulk.GetFirstEntry() + i, &collectionStart, &collectionSize);
         auto beginPtr = reinterpret_cast<void **>(fBulk.GetValuePtrAt(i));
         *beginPtr = fRVecData.data() + offset;
         offset += collectionSize * itemSize;
         std::int32_t *sizePtr = new (reinterpret_cast<void *>(beginPtr + 1)) std::int32_t(collectionSize);
         new (sizePtr + 1) std::int32_t(-1);
      }

      std::uint64_t entryOffset = mask.FirstEntry() - fBulk.GetFirstEntry();
      fBulk.SetAllValues();
      return fBulk.GetValuePtrAt(entryOffset);
   }

   void *LoadImpl(const ROOT::Internal::RDF::RMaskedEntryRange &mask, std::size_t bulkSize) final
   {
      // Can be called multiple times with different masks
      // first entry and bulk size move in non-overlapping blocks
      // Return a pointer to the value array (T[]), I own that; needs to stay valid until next LoadImpl or destruction

      if (!fBulk.ContainsRange(mask.FirstEntry(), bulkSize)) {
         fBulk.Reset(mask.FirstEntry(), bulkSize);
      }
      std::uint64_t entryOffset = mask.FirstEntry() - fBulk.GetFirstEntry();

      if (fBulk.GetHasAllValuesSet())
         return fBulk.GetValuePtrAt(entryOffset);

      if (fField->fIsSimple) {
         auto val = fField->CaptureValue(fBulk.GetValuePtrAt(entryOffset));
         fField->fPrincipalColumn->ReadV(mask.FirstEntry(), bulkSize, &val.fMappedElement);
         fBulk.SetAllValues();
         return val.GetRawPtr();
      }

      // Special handling of RVec of simple type
      auto rvecField = dynamic_cast<RRVecField *>(fField.get());
      if (rvecField && rvecField->GetSubFields()[0]->IsSimple()) {
         return LoadRVec(mask);
      }

      for (std::size_t i = 0; i < bulkSize; ++i) {
         // Value not needed
         if (!mask[i])
            continue;

         // Value already present
         if (fBulk.GetMask()[i + entryOffset])
            continue;

         //auto val = fField->GenerateValue(fBulk.GetValuePtrAt(i + entryOffset));
         auto val = fField->CaptureValue(fBulk.GetValuePtrAt(i + entryOffset));
         fField->Read(mask.FirstEntry() + i, &val);
         fBulk.SetValueAt(i + entryOffset);
      }

      return fBulk.GetValuePtrAt(entryOffset);
   }
};

} // namespace Internal

RNTupleDS::~RNTupleDS() = default;

void RNTupleDS::AddField(const RNTupleDescriptor &desc, std::string_view colName, DescriptorId_t fieldId,
                         std::vector<DescriptorId_t> skeinIDs)
{
   // As an example for the mapping of RNTuple fields to RDF columns, let's consider an RNTuple
   // using the following types and with a top-level field named "event" of type Event:
   //
   // struct Event {
   //    int id;
   //    std::vector<Track> tracks;
   // };
   // struct Track {
   //    std::vector<Hit> hits;
   // };
   // struct Hit {
   //    float x;
   //    float y;
   // };
   //
   // AddField() will be called from the constructor with the RNTuple root field (ENTupleStructure::kRecord).
   // From there, we recurse into the "event" sub field (also ENTupleStructure::kRecord) and further down the
   // tree of sub fields and expose the following RDF columns:
   //
   // "event"                             [Event]
   // "event.id"                          [int]
   // "event.tracks"                      [RVec<Track>]
   // "R_rdf_sizeof_event.tracks"         [unsigned int]
   // "event.tracks.hits"                 [RVec<RVec<Hit>>]
   // "R_rdf_sizeof_event.tracks.hits"    [RVec<unsigned int>]
   // "event.tracks.hits.x"               [RVec<RVec<float>>]
   // "R_rdf_sizeof_event.tracks.hits.x"  [RVec<unsigned int>]
   // "event.tracks.hits.y"               [RVec<RVec<float>>]
   // "R_rdf_sizeof_event.tracks.hits.y"  [RVec<unsigned int>]

   const auto &fieldDesc = desc.GetFieldDescriptor(fieldId);
   if (fieldDesc.GetStructure() == ENTupleStructure::kCollection) {
      // Inner fields of collections are provided as projected collections of only that inner field,
      // E.g. we provide a projected collection RVec<RVec<float>> for "event.tracks.hits.x" in the example
      // above.

      // We open a new collection scope with fieldID being the inner most collection. E.g. for "event.tracks.hits",
      // skeinIDs would already contain the fieldID of "event.tracks"
      skeinIDs.emplace_back(fieldId);

      if (fieldDesc.GetTypeName().empty()) {
         // Anonymous collection with one or several sub fields
         auto cardinalityField = std::make_unique<ROOT::Experimental::Internal::RRDFCardinalityField>();
         cardinalityField->SetOnDiskId(fieldId);
         fColumnNames.emplace_back("R_rdf_sizeof_" + std::string(colName));
         fColumnTypes.emplace_back(cardinalityField->GetType());
         auto cardColReader = std::make_unique<ROOT::Experimental::Internal::RNTupleColumnReader>(
            std::move(cardinalityField));
         fColumnReaderPrototypes.emplace_back(std::move(cardColReader));

         for (const auto &f : desc.GetFieldIterable(fieldDesc.GetId())) {
            AddField(desc, std::string(colName) + "." + f.GetFieldName(), f.GetId(), skeinIDs);
         }
      } else {
         // ROOT::RVec with exactly one sub field
         const auto &f = *desc.GetFieldIterable(fieldDesc.GetId()).begin();
         AddField(desc, colName, f.GetId(), skeinIDs);
      }
      // Note that at the end of the recursion, we handled the inner sub collections as well as the
      // collection as whole, so we are done.
      return;
   } else if (fieldDesc.GetStructure() == ENTupleStructure::kRecord) {
      // Inner fields of records are provided as individual RDF columns, e.g. "event.id"
      for (const auto &f : desc.GetFieldIterable(fieldDesc.GetId())) {
         auto innerName = colName.empty() ? f.GetFieldName() : (std::string(colName) + "." + f.GetFieldName());
         AddField(desc, innerName, f.GetId(), skeinIDs);
      }
   }

   // The fieldID could be the root field or the class of fieldId might not be loaded.
   // In these cases, only the inner fields are exposed as RDF columns.
   auto fieldOrException = Detail::RFieldBase::Create("", fieldDesc.GetTypeName());
   if (!fieldOrException)
      return;
   auto valueField = fieldOrException.Unwrap();
   valueField->SetOnDiskId(fieldId);
   std::unique_ptr<Detail::RFieldBase> cardinalityField;
   // Collections get the additional "number of" RDF column (e.g. "R_rdf_sizeof_tracks")
   if (!skeinIDs.empty()) {
      cardinalityField = std::make_unique<ROOT::Experimental::Internal::RRDFCardinalityField>();
      cardinalityField->SetOnDiskId(skeinIDs.back());
   }

   for (auto i = skeinIDs.rbegin(); i != skeinIDs.rend(); ++i) {
      valueField = std::make_unique<ROOT::Experimental::RRVecField>("", std::move(valueField));
      valueField->SetOnDiskId(*i);
      // Skip the inner-most collection level to construct the cardinality column
      if (i != skeinIDs.rbegin()) {
         cardinalityField = std::make_unique<ROOT::Experimental::RRVecField>("", std::move(cardinalityField));
         cardinalityField->SetOnDiskId(*i);
      }
   }

   if (cardinalityField) {
      fColumnNames.emplace_back("R_rdf_sizeof_" + std::string(colName));
      fColumnTypes.emplace_back(cardinalityField->GetType());
      auto cardColReader = std::make_unique<ROOT::Experimental::Internal::RNTupleColumnReader>(
         std::move(cardinalityField));
      fColumnReaderPrototypes.emplace_back(std::move(cardColReader));
   }

   skeinIDs.emplace_back(fieldId);
   fColumnNames.emplace_back(colName);
   fColumnTypes.emplace_back(valueField->GetType());
   auto valColReader = std::make_unique<ROOT::Experimental::Internal::RNTupleColumnReader>(std::move(valueField));
   fColumnReaderPrototypes.emplace_back(std::move(valColReader));
}

RNTupleDS::RNTupleDS(std::unique_ptr<Detail::RPageSource> pageSource)
{
   pageSource->Attach();
   auto descriptorGuard = pageSource->GetSharedDescriptorGuard();
   fSources.emplace_back(std::move(pageSource));

   AddField(descriptorGuard.GetRef(), "", descriptorGuard->GetFieldZeroId(), std::vector<DescriptorId_t>());
}

RDF::RDataSource::Record_t RNTupleDS::GetColumnReadersImpl(std::string_view /* name */, const std::type_info & /* ti */)
{
   // This datasource uses the GetColumnReaders2 API instead (better name in the works)
   return {};
}

std::unique_ptr<ROOT::Detail::RDF::RColumnReaderBase>
RNTupleDS::GetColumnReaders(unsigned int slot, std::string_view name, const std::type_info & /*tid*/)
{
   // at this point we can assume that `name` will be found in fColumnNames, RDF is in charge validation
   // TODO(jblomer): check incoming type
   const auto index = std::distance(fColumnNames.begin(), std::find(fColumnNames.begin(), fColumnNames.end(), name));
   auto clone = fColumnReaderPrototypes[index]->Clone();
   clone->Connect(*fSources[slot]);
   return clone;
}

bool RNTupleDS::SetEntry(unsigned int, ULong64_t)
{
   return true;
}

std::size_t RNTupleDS::GetBulkSize(unsigned int slot, ULong64_t rangeStart, std::size_t maxSize)
{
   auto descGuard = fSources[slot]->GetSharedDescriptorGuard();
   for (const auto &c : descGuard->GetClusterIterable()) {
      // TODO: consider cluster sharding
      if (c.GetFirstEntryIndex() > rangeStart)
         continue;
      if ((c.GetFirstEntryIndex() + c.GetNEntries()) <= rangeStart)
         continue;

      auto remainingEntries = c.GetFirstEntryIndex() + c.GetNEntries() - rangeStart;
      return maxSize < remainingEntries ? maxSize : remainingEntries;
   }
   // Never here?
   return 1;
}

std::vector<std::pair<ULong64_t, ULong64_t>> RNTupleDS::GetEntryRanges()
{
   // TODO(jblomer): use cluster boundaries for the entry ranges
   std::vector<std::pair<ULong64_t, ULong64_t>> ranges;
   if (fHasSeenAllRanges)
      return ranges;

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
   const auto index = std::distance(fColumnNames.begin(), std::find(fColumnNames.begin(), fColumnNames.end(), colName));
   return fColumnTypes[index];
}

bool RNTupleDS::HasColumn(std::string_view colName) const
{
   return std::find(fColumnNames.begin(), fColumnNames.end(), colName) != fColumnNames.end();
}

void RNTupleDS::Initialize()
{
   fHasSeenAllRanges = false;
}

void RNTupleDS::Finalize() {}

void RNTupleDS::SetNSlots(unsigned int nSlots)
{
   R__ASSERT(fNSlots == 0);
   R__ASSERT(nSlots > 0);
   fNSlots = nSlots;

   for (unsigned int i = 1; i < fNSlots; ++i) {
      fSources.emplace_back(fSources[0]->Clone());
      assert(i == (fSources.size() - 1));
      fSources[i]->Attach();
   }
}
} // namespace Experimental
} // namespace ROOT

ROOT::RDataFrame ROOT::RDF::Experimental::FromRNTuple(std::string_view ntupleName, std::string_view fileName)
{
   auto pageSource = ROOT::Experimental::Detail::RPageSource::Create(ntupleName, fileName);
   ROOT::RDataFrame rdf(std::make_unique<ROOT::Experimental::RNTupleDS>(std::move(pageSource)));
   return rdf;
}

ROOT::RDataFrame ROOT::RDF::Experimental::FromRNTuple(ROOT::Experimental::RNTuple *ntuple)
{
   ROOT::RDataFrame rdf(std::make_unique<ROOT::Experimental::RNTupleDS>(ntuple->MakePageSource()));
   return rdf;
}
