/// \file RColumnElement.cxx
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2019-08-11
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <ROOT/RColumnElement.hxx>

#include <TError.h>

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <memory>
#include <utility>

std::unique_ptr<ROOT::Experimental::Detail::RColumnElementBase>
ROOT::Experimental::Detail::RColumnElementBase::Generate(EColumnType type) {
   switch (type) {
   case EColumnType::kReal32:
      return std::make_unique<RColumnElement<float, EColumnType::kReal32>>(nullptr);
   case EColumnType::kReal64:
      return std::make_unique<RColumnElement<double, EColumnType::kReal64>>(nullptr);
   case EColumnType::kByte:
      return std::make_unique<RColumnElement<std::uint8_t, EColumnType::kByte>>(nullptr);
   case EColumnType::kInt32:
      return std::make_unique<RColumnElement<std::int32_t, EColumnType::kInt32>>(nullptr);
   case EColumnType::kInt64:
      return std::make_unique<RColumnElement<std::int64_t, EColumnType::kInt64>>(nullptr);
   case EColumnType::kBit:
      return std::make_unique<RColumnElement<bool, EColumnType::kBit>>(nullptr);
   case EColumnType::kIndex:
      return std::make_unique<RColumnElement<ClusterSize_t, EColumnType::kIndex>>(nullptr);
   case EColumnType::kSwitch:
      return std::make_unique<RColumnElement<RColumnSwitch, EColumnType::kSwitch>>(nullptr);
   default:
      R__ASSERT(false);
   }
   // never here
   return nullptr;
}

std::size_t ROOT::Experimental::Detail::RColumnElementBase::GetBitsOnStorage(EColumnType type) {
   switch (type) {
   case EColumnType::kReal32:
      return 32;
   case EColumnType::kReal64:
      return 64;
   case EColumnType::kByte:
      return 8;
   case EColumnType::kInt32:
      return 32;
   case EColumnType::kInt64:
      return 64;
   case EColumnType::kBit:
      return 1;
   case EColumnType::kIndex:
      return 32;
   case EColumnType::kSwitch:
      return 64;
   default:
      R__ASSERT(false);
   }
   // never here
   return 0;
}

void ROOT::Experimental::Detail::RColumnElement<bool, ROOT::Experimental::EColumnType::kBit>::Pack(
  void *dst, void *src, std::size_t count) const
{
   bool *boolArray = reinterpret_cast<bool *>(src);
   char *charArray = reinterpret_cast<char *>(dst);
   std::bitset<8> bitSet;
   std::size_t i = 0;
   for (; i < count; ++i) {
      bitSet.set(i % 8, boolArray[i]);
      if (i % 8 == 7) {
         char packed = bitSet.to_ulong();
         charArray[i / 8] = packed;
      }
   }
   if (i % 8 != 0) {
      char packed = bitSet.to_ulong();
      charArray[i / 8] = packed;
   }
}

void ROOT::Experimental::Detail::RColumnElement<bool, ROOT::Experimental::EColumnType::kBit>::Unpack(
  void *dst, void *src, std::size_t count) const
{
   bool *boolArray = reinterpret_cast<bool *>(dst);
   char *charArray = reinterpret_cast<char *>(src);
   std::bitset<8> bitSet;
   for (std::size_t i = 0; i < count; i += 8) {
      bitSet = charArray[i / 8];
      for (std::size_t j = i; j < std::min(count, i + 8); ++j) {
         boolArray[j] = bitSet[j % 8];
      }
   }
}


void ROOT::Experimental::Detail::RColumnElement<float, ROOT::Experimental::EColumnType::kReal32>::Pack(
  void *dst, void *src, std::size_t count) const
{
   char *unsplitArray = reinterpret_cast<char *>(src);
   char *splitArray = reinterpret_cast<char *>(dst);
   for (std::size_t i = 0; i < count; ++i) {
      splitArray[i]             = unsplitArray[4 * i];
      splitArray[count + i]     = unsplitArray[4 * i + 1];
      splitArray[2 * count + i] = unsplitArray[4 * i + 2];
      splitArray[3 * count + i] = unsplitArray[4 * i + 3];
   }
}

void ROOT::Experimental::Detail::RColumnElement<float, ROOT::Experimental::EColumnType::kReal32>::Unpack(
  void *dst, void *src, std::size_t count) const
{
   char *splitArray = reinterpret_cast<char *>(src);
   char *unsplitArray = reinterpret_cast<char *>(dst);
   for (std::size_t i = 0; i < count; ++i) {
      unsplitArray[4 * i]     = splitArray[i];
      unsplitArray[4 * i + 1] = splitArray[count + i];
      unsplitArray[4 * i + 2] = splitArray[2 * count + i];
      unsplitArray[4 * i + 3] = splitArray[3 * count + i];
   }
}


void ROOT::Experimental::Detail::RColumnElement<double, ROOT::Experimental::EColumnType::kReal64>::Pack(
  void *dst, void *src, std::size_t count) const
{
   char *unsplitArray = reinterpret_cast<char *>(src);
   char *splitArray = reinterpret_cast<char *>(dst);
   for (std::size_t i = 0; i < count; ++i) {
      splitArray[i]             = unsplitArray[8 * i];
      splitArray[count + i]     = unsplitArray[8 * i + 1];
      splitArray[2 * count + i] = unsplitArray[8 * i + 2];
      splitArray[3 * count + i] = unsplitArray[8 * i + 3];
      splitArray[4 * count + i] = unsplitArray[8 * i + 4];
      splitArray[5 * count + i] = unsplitArray[8 * i + 5];
      splitArray[6 * count + i] = unsplitArray[8 * i + 6];
      splitArray[7 * count + i] = unsplitArray[8 * i + 7];
   }
}

void ROOT::Experimental::Detail::RColumnElement<double, ROOT::Experimental::EColumnType::kReal64>::Unpack(
  void *dst, void *src, std::size_t count) const
{
   char *splitArray = reinterpret_cast<char *>(src);
   char *unsplitArray = reinterpret_cast<char *>(dst);
   for (std::size_t i = 0; i < count; ++i) {
      unsplitArray[8 * i]     = splitArray[i];
      unsplitArray[8 * i + 1] = splitArray[count + i];
      unsplitArray[8 * i + 2] = splitArray[2 * count + i];
      unsplitArray[8 * i + 3] = splitArray[3 * count + i];
      unsplitArray[8 * i + 4] = splitArray[4 * count + i];
      unsplitArray[8 * i + 5] = splitArray[5 * count + i];
      unsplitArray[8 * i + 6] = splitArray[6 * count + i];
      unsplitArray[8 * i + 7] = splitArray[7 * count + i];
   }
}


void ROOT::Experimental::Detail::RColumnElement<
  ROOT::Experimental::ClusterSize_t, ROOT::Experimental::EColumnType::kIndex>::Pack(
  void *dst, void *src, std::size_t count) const
{
   if (!count)
      return;

   std::uint32_t *unpackedOffsets = reinterpret_cast<std::uint32_t *>(src);
   std::uint32_t *packedOffsets = reinterpret_cast<std::uint32_t *>(dst);

   packedOffsets[0] = unpackedOffsets[0];
   for (std::size_t i = 1; i < count; ++i) {
      packedOffsets[i] = unpackedOffsets[i] - unpackedOffsets[i - 1];
   }
}

void ROOT::Experimental::Detail::RColumnElement<
  ROOT::Experimental::ClusterSize_t, ROOT::Experimental::EColumnType::kIndex>::Unpack(
  void *dst, void *src, std::size_t count) const
{
   if (!count)
      return;

   std::uint32_t *packedOffsets = reinterpret_cast<std::uint32_t *>(src);
   std::uint32_t *unpackedOffsets = reinterpret_cast<std::uint32_t *>(dst);

   unpackedOffsets[0] = packedOffsets[0];
   for (std::size_t i = 1; i < count; ++i) {
      unpackedOffsets[i] = unpackedOffsets[i-1] + packedOffsets[i];
   }
}
