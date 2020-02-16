#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleBrowser.hxx>
#include <ROOT/RNTupleModel.hxx>

#include <TFile.h>

#include <cstdio>
#include <memory>

#include "gtest/gtest.h"

using RNTupleBrowser = ROOT::Experimental::RNTupleBrowser;
using RNTupleModel = ROOT::Experimental::RNTupleModel;
using RNTupleWriter = ROOT::Experimental::RNTupleWriter;

// Unit tests cannot test the entire TBrwoser-support, it has no access to the TBrowser GUI.

namespace {

/**
 * An RAII wrapper around an open temporary file on disk. It cleans up the guarded file when the wrapper object
 * goes out of scope.
 */
class FileRaii {
private:
   std::string fPath;
public:
   explicit FileRaii(const std::string &path) : fPath(path) { }
   FileRaii(const FileRaii&) = delete;
   FileRaii& operator=(const FileRaii&) = delete;
   ~FileRaii() { std::remove(fPath.c_str()); }
   std::string GetPath() const { return fPath; }
}; // FileRaii

} // anonymous namespace


TEST(RNTupleBrowse, Construct)
{
   FileRaii fileGuard("test_ntuple_browse_float.root");
   {
      auto model = RNTupleModel::Create();
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntpl", fileGuard.GetPath());
   }

   EXPECT_DEATH(RNTupleBrowser(nullptr, "ntpl"), ".*");

   // Set gDirectory to the file
   auto file = std::unique_ptr<TFile>(TFile::Open(fileGuard.GetPath().c_str(), "READ"));

   RNTupleBrowser ntplBrowser(nullptr, "ntpl");
   EXPECT_EQ(0U, ntplBrowser.GetReader()->GetNEntries());
}
