#include "ntuple_test.hxx"

class RNTupleMergerTest : public ::testing::Test {
protected:
   std::vector<FileRaii> fFiles;
   std::vector<std::unique_ptr<RPageSource>> fMergeSources;
   std::string fNtplName = "some_ntuple";

   void SetUp() override {
      auto modelWrite = RNTupleModel::Create();
      auto wrPt = modelWrite->MakeField<float>("pt", 42.0);
      auto wrEnergy = modelWrite->MakeField<float>("energy", 7.0);
      auto wrTag = modelWrite->MakeField<std::string>("tag", "xyz");
      auto wrBools = modelWrite->MakeField<std::vector<bool>>("bools", std::vector<bool>{true, false, true, false});
      auto wrJets = modelWrite->MakeField<std::vector<float>>("jets");
      wrJets->push_back(1.0);
      wrJets->push_back(2.0);
      auto wrNnlo = modelWrite->MakeField<std::vector<std::vector<float>>>("nnlo");
      wrNnlo->push_back(std::vector<float>());
      wrNnlo->push_back(std::vector<float>{1.0});
      wrNnlo->push_back(std::vector<float>{1.0, 2.0, 4.0, 8.0});

      auto numFiles = 10;
      for (int i = 0; i < numFiles; ++i) {
         std::string file = "merger_input" + std::to_string(i) + ".root";
         fFiles.emplace_back(FileRaii(file));
         auto ntuple = RNTupleWriter::Recreate(
            std::unique_ptr<RNTupleModel>(modelWrite->Clone()), fNtplName, file);
         ntuple->Fill();
      }
      for (int i = 0; i < numFiles; ++i) {
         fMergeSources.push_back(RPageSource::Create(fNtplName, fFiles.at(i).GetPath()));
      }
   }
};

TEST_F(RNTupleMergerTest, LowLevelMerge)
{
   if (fMergeSources.size() == 0) {
      FAIL() << "no page sources opened to merge";
   }
   const auto& ref_desc = fMergeSources.front()->GetDescriptor();
   for (std::size_t i = 0; i < fMergeSources.size(); ++i) {
      std::cout << "file: " << fFiles.at(i).GetPath() << "\n";
      EXPECT_EQ(ref_desc, fMergeSources.at(i)->GetDescriptor());
   }
}

TEST(RFieldMerger, Merge)
{
   auto mergeResult = RFieldMerger::Merge(RFieldDescriptor(), RFieldDescriptor());
   EXPECT_FALSE(mergeResult);
}
