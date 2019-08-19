/// \file RRawFileUnix.cxx
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2019-08-19
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "ROOT/RRawFileUnix.hxx"

#include "TError.h"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
constexpr int kDefaultBlockSize = 4096; // If fstat() does not provide a block size hint, use this value instead
} // anonymous namespace

ROOT::Experimental::Detail::RRawFileUnix::RRawFileUnix(std::string_view url, ROptions options)
   : ROOT::Experimental::Detail::RRawFile(url, options), fFileDes(-1)
{
}

ROOT::Experimental::Detail::RRawFileUnix::~RRawFileUnix()
{
   if (fFileDes >= 0)
      close(fFileDes);
}

std::uint64_t ROOT::Experimental::Detail::RRawFileUnix::DoGetSize()
{
   struct stat info;
   int res = fstat(fFileDes, &info);
   if (res != 0)
      throw std::runtime_error("Cannot call fstat on '" + fUrl + "', error: " + std::string(strerror(errno)));
   return info.st_size;
}

void ROOT::Experimental::Detail::RRawFileUnix::DoOpen()
{
   fFileDes = open(GetLocation(fUrl).c_str(), O_RDONLY);
   if (fFileDes < 0) {
      throw std::runtime_error("Cannot open '" + fUrl + "', error: " + std::string(strerror(errno)));
   }

   if (fOptions.fBlockSize >= 0)
      return;

   struct stat info;
   int res = fstat(fFileDes, &info);
   if (res != 0) {
      throw std::runtime_error("Cannot call fstat on '" + fUrl + "', error: " + std::string(strerror(errno)));
   }
   if (info.st_blksize > 0) {
      fOptions.fBlockSize = info.st_blksize;
   } else {
      fOptions.fBlockSize = kDefaultBlockSize;
   }
}

size_t ROOT::Experimental::Detail::RRawFileUnix::DoReadAt(void *buffer, size_t nbytes, std::uint64_t offset)
{
   size_t total_bytes = 0;
   while (nbytes) {
      ssize_t res = pread(fFileDes, buffer, nbytes, offset);
      if (res < 0) {
         if (errno == EINTR)
            continue;
         throw std::runtime_error("Cannot read from '" + fUrl + "', error: " + std::string(strerror(errno)));
      } else if (res == 0) {
         return total_bytes;
      }
      R__ASSERT(static_cast<size_t>(res) <= nbytes);
      buffer = reinterpret_cast<unsigned char *>(buffer) + res;
      nbytes -= res;
      total_bytes += res;
      offset += res;
   }
   return total_bytes;
}
