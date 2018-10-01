// Author: Jakob Blomer CERN  07/2018

/*************************************************************************
 * Copyright (C) 1995-2017, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_RSQLITEDS
#define ROOT_RSQLITEDS

#include <ROOT/RColumn.hxx>
#include <ROOT/RColumnElement.hxx>
#include <ROOT/RColumnStorage.hxx>
#include <ROOT/RDataFrame.hxx>
#include <ROOT/RDataSource.hxx>
#include <ROOT/RStringView.hxx>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ROOT {

namespace Experimental {
class RBranchBase;
class RCargoBase;
class RColumn;
class RColumnElementBase;
class RTree;
}

namespace RDF {

class RForestDS final : public ROOT::RDF::RDataSource {
private:
   unsigned fNSlots;
   std::uint64_t fNentries;
   bool fHasSeenAllRanges;  // TODO Remove me
   unsigned fNColumns;
   std::vector<ROOT::Experimental::RTree*> fTrees;

   std::vector<ROOT::Experimental::RColumnSource*> fSources;
   std::vector<std::unique_ptr<ROOT::Experimental::RColumnSource>> fSourceClones;
   ROOT::Experimental::RColumnSource::ClusterList_t fClusterList;
   ROOT::Experimental::RColumnSource::ColumnList_t fColumnList;
   std::vector<std::string> fColumnNames;
   std::vector<std::string> fTypeNames;
   std::vector<ROOT::Experimental::RCargoBase*> fCargos;
   std::vector<ROOT::Experimental::RBranchBase*> fBranches;

   // Per-slot column collections
   std::vector<std::vector<std::unique_ptr<ROOT::Experimental::RColumn>>> fColumns;
   std::vector<std::vector<std::unique_ptr<ROOT::Experimental::RColumnElementBase>>> fColumnElements;

public:
   RForestDS(ROOT::Experimental::RTree* tree);
   ~RForestDS();
   void SetNSlots(unsigned int nSlots) final;
   const std::vector<std::string> &GetColumnNames() const final;
   bool HasColumn(std::string_view colName) const final;
   std::string GetTypeName(std::string_view colName) const final;
   std::vector<std::pair<ULong64_t, ULong64_t>> GetEntryRanges() final;

   bool SetEntry(unsigned int slot, ULong64_t entry) final;

   void Initialise() final;

protected:
   Record_t GetColumnReadersImpl(std::string_view name, const std::type_info &) final;
};

} // ns RDF

} // ns ROOT

#endif
