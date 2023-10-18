/// \file RNTupleIterator.cxx
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2023-10-17
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2023, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <ROOT/RError.hxx>
#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RNTupleIterator.hxx>
#include <ROOT/RPageStorage.hxx>

#include <algorithm>
#include <functional>
#include <iterator>
#include <utility>

std::unique_ptr<ROOT::Experimental::RNTupleIterator::RDataSet>
ROOT::Experimental::RNTupleIterator::RDataSet::Create(std::unique_ptr<Detail::RPageSource> source)
{
   auto result = std::unique_ptr<RDataSet>(new RDataSet());
   result->fType = EType::kSingle;
   result->fSource = std::move(source);
   return result;
}

std::unique_ptr<ROOT::Experimental::RNTupleIterator::RDataSet>
ROOT::Experimental::RNTupleIterator::RDataSet::CreateChain(std::span<std::unique_ptr<RDataSet>> parts)
{
   if (parts.size() == 0)
      throw RException(R__FAIL("empty chain"));

   auto result = std::unique_ptr<RDataSet>(new RDataSet());
   result->fType = EType::kChain;
   std::move(parts.begin(), parts.end(), std::back_inserter(result->fParts));
   return result;
}

std::unique_ptr<ROOT::Experimental::RNTupleIterator::RDataSet>
ROOT::Experimental::RNTupleIterator::RDataSet::CreateFriends(std::span<std::unique_ptr<RDataSet>> parts)
{
   if (parts.size() == 0)
      throw RException(R__FAIL("empty friends"));

   auto result = std::unique_ptr<RDataSet>(new RDataSet());
   result->fType = EType::kFriends;
   std::move(parts.begin(), parts.end(), std::back_inserter(result->fParts));
   return result;
}

void ROOT::Experimental::RNTupleIterator::RDataSet::EnsureDescriptor()
{
   if (fDescriptor)
      return;

   fSource->Attach();
   fDescriptor = fSource->GetSharedDescriptorGuard()->Clone();
}

ROOT::Experimental::RNTupleIterator::RDataSet::TopLevelDescriptor_t
ROOT::Experimental::RNTupleIterator::RDataSet::GetTopLevelDescriptor()
{
   TopLevelDescriptor_t result;
   switch (fType) {
   case EType::kSingle:
      EnsureDescriptor();
      for (const auto &fieldDesc : fDescriptor->GetTopLevelFields()) {
         result.emplace_back(std::cref(fieldDesc), std::cref(*fDescriptor));
      }
      return result;
   case EType::kChain:
      return fParts[0]->GetTopLevelDescriptor();
   case EType::kFriends:
      // for (auto &p : fParts) {
      // }
      return result;
   default:
      throw RException(R__FAIL("internal error, invalid data set type"));
   }
}
