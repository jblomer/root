// @(#)root/io:$Id$
// Author: Jakob Blomer

/*************************************************************************
 * Copyright (C) 1995-2018, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "ROOT/RRawFileDavix.hxx"
#include "ROOT/RMakeUnique.hxx"

#include "TError.h"

#include <stdexcept>

#include <davix.hpp>
#include <sys/stat.h>

namespace {
constexpr int kDefaultBlockSize = 128 * 1024; // Read in relatively large 128k blocks for better network utilization
} // anonymous namespace

namespace ROOT {
namespace Experimental {
namespace Detail {
namespace Internal {

struct RDavixFileDes {
   RDavixFileDes() : fd(nullptr), pos(&ctx) {}
   RDavixFileDes(const RDavixFileDes &) = delete;
   RDavixFileDes &operator=(const RDavixFileDes &) = delete;
   ~RDavixFileDes() = default;

   DAVIX_FD *fd;
   Davix::Context ctx;
   Davix::DavPosix pos;
   Davix::RequestParams parm;
};

} // namespace Internal
} // namespace Detail
} // namespace Experimental
} // namespace ROOT

ROOT::Experimental::Detail::RRawFileDavix::RRawFileDavix(std::string_view url, ROptions options)
   : RRawFile(url, options), fFileDes(new Internal::RDavixFileDes())
{
}

ROOT::Experimental::Detail::RRawFileDavix::~RRawFileDavix()
{
   if (fFileDes->fd != nullptr)
      fFileDes->pos.close(fFileDes->fd, nullptr);
}

std::unique_ptr<ROOT::Experimental::Detail::RRawFile> ROOT::Experimental::Detail::RRawFileDavix::Clone() const
{
   return std::make_unique<RRawFileDavix>(fUrl, fOptions);
}

std::uint64_t ROOT::Experimental::Detail::RRawFileDavix::DoGetSize()
{
   struct stat buf;
   Davix::DavixError *err = nullptr;
   if (fFileDes->pos.stat(nullptr, fUrl, &buf, &err) == -1) {
      throw std::runtime_error("Cannot determine size of '" + fUrl + "', error: " + err->getErrMsg());
   }
   return buf.st_size;
}

void ROOT::Experimental::Detail::RRawFileDavix::DoOpen()
{
   fFileDes->parm.setMetalinkMode(Davix::MetalinkMode::Disable);
   Davix::DavixError *err = nullptr;
   fFileDes->fd = fFileDes->pos.open(&fFileDes->parm, fUrl, O_RDONLY, &err);
   if (fFileDes->fd == nullptr) {
      throw std::runtime_error("Cannot open '" + fUrl + "', error: " + err->getErrMsg());
   }
   fFileDes->pos.fadvise(fFileDes->fd, 0, 300, Davix::AdviseRandom);
   if (fOptions.fBlockSize < 0)
      fOptions.fBlockSize = kDefaultBlockSize;
}

size_t ROOT::Experimental::Detail::RRawFileDavix::DoReadAt(void *buffer, size_t nbytes, std::uint64_t offset)
{
   Davix::DavixError *err = nullptr;
   auto retval = fFileDes->pos.pread(fFileDes->fd, buffer, nbytes, offset, &err);
   if (retval < 0) {
      throw std::runtime_error("Cannot read from '" + fUrl + "', error: " + err->getErrMsg());
   }
   return static_cast<size_t>(retval);
}

void ROOT::Experimental::Detail::RRawFileDavix::DoReadV(RIOVec *ioVec, unsigned int nReq)
{
   Davix::DavixError *davixErr = NULL;
   Davix::DavIOVecInput in[nReq];
   Davix::DavIOVecOuput out[nReq];

   for (unsigned int i = 0; i < nReq; ++i) {
      in[i].diov_buffer = ioVec[i].fBuffer;
      in[i].diov_offset = ioVec[i].fOffset;
      in[i].diov_size = ioVec[i].fSize;
      R__ASSERT(ioVec[i].fSize > 0);
   }

   auto ret = fFileDes->pos.preadVec(fFileDes->fd, in, out, nReq, &davixErr);
   if (ret < 0) {
      throw std::runtime_error("Cannot do vector read from '" + fUrl + "', error: " + davixErr->getErrMsg());
   }

   for (unsigned int i = 0; i < nReq; ++i) {
      ioVec[i].fOutBytes = out[i].diov_size;
   }
}
