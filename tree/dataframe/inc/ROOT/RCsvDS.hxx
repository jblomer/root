// Author: Enric Tejedor CERN  10/2017

/*************************************************************************
 * Copyright (C) 1995-2022, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_RCSVTDS
#define ROOT_RCSVTDS

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RDataSource.hxx"

#include <cstdint>
#include <deque>
#include <list>
#include <unordered_map>
#include <set>
#include <memory>
#include <vector>

#include <TRegexp.h>

namespace ROOT {

namespace Internal {
class RRawFile;
}

namespace RDF {

class RCsvDS final : public ROOT::RDF::RDataSource {
public:
   /// Options that control how the CSV file is parsed
   struct ROptions {
      bool fReadHeaders = true;          ///< The first line defines the column name
      char fDelimiter = ',';             ///< Column delimiter character
      bool fTrimLines = false;           ///< Leading and trailing whitespaces should be removed
      bool fSkipEmptyLines = true;       ///< Ignore empty lines (after trimming, if trimming is enabled)
      bool fCommentCharacter = '\0';     ///< Lines starting with this character are ignored
      std::int64_t fLinesChunkSize = -1; ///< Number of lines to read, -1 to read all
      /// Specify custom column types, accepts an unordered map with keys being column name, values being type alias
      /// ('O' for boolean, 'D' for double, 'L' for Long64_t, 'T' for std::string)
      std::unordered_map<std::string, char> fColTypes;
   };

private:
   // Possible values are D, O, L, T. This is possible only because we treat double, bool, Long64_t and string
   using ColType_t = char;
   static const std::unordered_map<ColType_t, std::string> fgColTypeMap;

   // Regular expressions for type inference
   static const TRegexp fgIntRegex, fgDoubleRegex1, fgDoubleRegex2, fgDoubleRegex3, fgTrueRegex, fgFalseRegex;

   ROptions fOptions;
   std::uint64_t fDataPos = 0;
   unsigned int fNSlots = 0U;
   std::unique_ptr<ROOT::Internal::RRawFile> fCsvFile;
   ULong64_t fEntryRangesRequested = 0ULL;
   ULong64_t fProcessedLines = 0ULL; // marks the progress of the consumption of the csv lines
   std::vector<std::string> fHeaders; // the column names
   std::unordered_map<std::string, ColType_t> fColTypes;
   std::set<std::string> fColContainingEmpty; // store columns which had empty entry
   std::list<ColType_t> fColTypesList; // column types, order is the same as fHeaders, values the same as fColTypes
   std::vector<std::vector<void *>> fColAddresses;         // fColAddresses[column][slot] (same ordering as fHeaders)
   std::vector<Record_t> fRecords;                         // fRecords[entry][column] (same ordering as fHeaders)
   std::vector<std::vector<double>> fDoubleEvtValues;      // one per column per slot
   std::vector<std::vector<Long64_t>> fLong64EvtValues;    // one per column per slot
   std::vector<std::vector<std::string>> fStringEvtValues; // one per column per slot
   // This must be a deque to avoid the specialisation vector<bool>. This would not
   // work given that the pointer to the boolean in that case cannot be taken
   std::vector<std::deque<bool>> fBoolEvtValues; // one per column per slot

   void Construct();

   bool Readln(std::string &line);
   void FillHeaders(const std::string &);
   void FillRecord(const std::string &, Record_t &);
   void GenerateHeaders(size_t);
   std::vector<void *> GetColumnReadersImpl(std::string_view, const std::type_info &) final;
   void ValidateColTypes(std::vector<std::string> &) const;
   void InferColTypes(std::vector<std::string> &);
   void InferType(const std::string &, unsigned int);
   std::vector<std::string> ParseColumns(const std::string &);
   size_t ParseValue(const std::string &, std::vector<std::string> &, size_t);
   ColType_t GetType(std::string_view colName) const;
   void FreeRecords();

protected:
   std::string AsString() final;

public:
   RCsvDS(std::string_view fileName, const ROptions &options);
   RCsvDS(std::string_view fileName, bool readHeaders = true, char delimiter = ',', Long64_t linesChunkSize = -1LL,
          std::unordered_map<std::string, char> &&colTypes = {});
   void Finalize() final;
   ~RCsvDS();
   const std::vector<std::string> &GetColumnNames() const final;
   std::vector<std::pair<ULong64_t, ULong64_t>> GetEntryRanges() final;
   std::string GetTypeName(std::string_view colName) const final;
   bool HasColumn(std::string_view colName) const final;
   bool SetEntry(unsigned int slot, ULong64_t entry) final;
   void SetNSlots(unsigned int nSlots) final;
   std::string GetLabel() final;
};

////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Factory method to create a CSV RDataFrame.
/// \param[in] fileName Path of the CSV file.
/// \param[in] options File parsing settings.
RDataFrame FromCSV(std::string_view fileName, const RCsvDS::ROptions &options);

////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Factory method to create a CSV RDataFrame.
/// \param[in] fileName Path of the CSV file.
/// \param[in] readHeaders `true` if the CSV file contains headers as first row, `false` otherwise
///                        (default `true`).
/// \param[in] delimiter Delimiter character (default ',').
/// \param[in] linesChunkSize bunch of lines to read, use -1 to read all
/// \param[in] colTypes Allow user to specify custom column types, accepts an unordered map with keys being
///                      column type, values being type alias ('O' for boolean, 'D' for double, 'L' for
///                      Long64_t, 'T' for std::string)
RDataFrame FromCSV(std::string_view fileName, bool readHeaders = true, char delimiter = ',',
                   Long64_t linesChunkSize = -1LL, std::unordered_map<std::string, char> &&colTypes = {});

} // ns RDF

} // ns ROOT

#endif
