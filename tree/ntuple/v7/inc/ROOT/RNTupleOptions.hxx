/// \file ROOT/RNTupleOptions.hxx
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2019-08-25
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_RNTupleOptions
#define ROOT7_RNTupleOptions

#include <Compression.h>

namespace ROOT {
namespace Experimental {

// clang-format off
/**
\class ROOT::Experimental::RNTupleWriteOptions
\ingroup NTuple
\brief Common user-tunable settings for storing ntuples

All page sink classes need to support the common options.
*/
// clang-format on
class RNTupleWriteOptions {
  int fCompression = RCompressionSetting::EDefaults::kUseAnalysis;
  unsigned int fNumElementsPerPage = 10000;
public:
  RNTupleWriteOptions() = default;
  int GetCompression() const { return fCompression; }
  unsigned int GetNumElementsPerPage() const { return fNumElementsPerPage; }
  void SetCompression(int val) { fCompression = val; }
  void SetCompression(RCompressionSetting::EAlgorithm algorithm, int compressionLevel) {
    fCompression = CompressionSettings(algorithm, compressionLevel);
  }
  void SetNumElementsPerPage(unsigned int val) { fNumElementsPerPage = val; }
};


// clang-format off
/**
\class ROOT::Experimental::RNTupleReadOptions
\ingroup NTuple
\brief Common user-tunable settings for reading ntuples

All page source classes need to support the common options.
*/
// clang-format on
class RNTupleReadOptions {
public:
   enum EClusterCache {
      kDefault,
      kOff,
      kOn,
      kMMap,
   };

private:
   EClusterCache fClusterCache = EClusterCache::kDefault;
   unsigned int fNumStreams = 4;

public:
   RNTupleReadOptions() = default;
   EClusterCache GetClusterCache() const { return fClusterCache; }
   void SetClusterCache(EClusterCache val) { fClusterCache = val; }
   unsigned int GetNumStreams() const { return fNumStreams; }
   void SetNumStreams(unsigned int val) { fNumStreams = val; }
};

} // namespace Experimental
} // namespace ROOT

#endif
