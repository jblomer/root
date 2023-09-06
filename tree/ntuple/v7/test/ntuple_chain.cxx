#include "ntuple_test.hxx"

TEST(RPageStorageChain, Null)
{
   FileRaii fileGuard1("test_ntuple_chain_null1.root");
   FileRaii fileGuard2("test_ntuple_chain_null2.root");

   {
      auto model1 = RNTupleModel::Create();
      auto model2 = model1->Clone();
      RNTupleWriter::Recreate(std::move(model1), "ntpl", fileGuard1.GetPath());
      RNTupleWriter::Recreate(std::move(model2), "ntpl", fileGuard2.GetPath());
   }

   std::vector<std::unique_ptr<RPageSource>> realSources;
   realSources.emplace_back(std::make_unique<RPageSourceFile>("ntpl", fileGuard1.GetPath(), RNTupleReadOptions()));
   realSources.emplace_back(std::make_unique<RPageSourceFile>("ntpl", fileGuard2.GetPath(), RNTupleReadOptions()));
   RPageSourceChain friendSource("myNTuple", realSources);
   friendSource.Attach();
   EXPECT_EQ(0u, friendSource.GetNEntries());
}
