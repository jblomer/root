#include "ntuple_test.hxx"

TEST(Packing, Bitfield)
{
   ROOT::Experimental::Detail::RColumnElement<bool, ROOT::Experimental::EColumnType::kBit> element(nullptr);
   element.Pack(nullptr, nullptr, 0);
   element.Unpack(nullptr, nullptr, 0);

   bool b = true;
   char c = 0;
   element.Pack(&c, &b, 1);
   EXPECT_EQ(1, c);
   bool e = false;
   element.Unpack(&e, &c, 1);
   EXPECT_TRUE(e);

   bool b8[] = {true, false, true, false, false, true, false, true};
   c = 0;
   element.Pack(&c, &b8, 8);
   bool e8[] = {false, false, false, false, false, false, false, false};
   element.Unpack(&e8, &c, 8);
   for (unsigned i = 0; i < 8; ++i) {
      EXPECT_EQ(b8[i], e8[i]);
   }

   bool b9[] = {true, false, true, false, false, true, false, true, true};
   char c2[2];
   element.Pack(&c2, &b9, 9);
   bool e9[] = {false, false, false, false, false, false, false, false, false};
   element.Unpack(&e9, &c2, 9);
   for (unsigned i = 0; i < 9; ++i) {
      EXPECT_EQ(b9[i], e9[i]);
   }
}


TEST(Packing, FloatSplit)
{
   ROOT::Experimental::Detail::RColumnElement<float, ROOT::Experimental::EColumnType::kReal32> element(nullptr);
   element.Pack(nullptr, nullptr, 0);
   element.Unpack(nullptr, nullptr, 0);

   float f = 42.0;
   char fPacked[4];
   element.Pack(fPacked, &f, 1);
   element.Unpack(&f, fPacked, 1);
   EXPECT_EQ(42.0, f);

   float mf[] = {1.0, 2.0, 3.0};
   char mfPacked[12];
   element.Pack(mfPacked, mf, 3);
   element.Unpack(mf, mfPacked, 3);
   EXPECT_EQ(1.0, mf[0]);
   EXPECT_EQ(2.0, mf[1]);
   EXPECT_EQ(3.0, mf[2]);
}


TEST(Packing, DoubleSplit)
{
   ROOT::Experimental::Detail::RColumnElement<double, ROOT::Experimental::EColumnType::kReal64> element(nullptr);
   element.Pack(nullptr, nullptr, 0);
   element.Unpack(nullptr, nullptr, 0);

   double f = 42.0;
   char fPacked[8];
   element.Pack(fPacked, &f, 1);
   element.Unpack(&f, fPacked, 1);
   EXPECT_EQ(42.0, f);

   double mf[] = {1.0, 2.0, 3.0};
   char mfPacked[24];
   element.Pack(mfPacked, mf, 3);
   element.Unpack(mf, mfPacked, 3);
   EXPECT_EQ(1.0, mf[0]);
   EXPECT_EQ(2.0, mf[1]);
   EXPECT_EQ(3.0, mf[2]);
}


TEST(Packing, OffsetSplit)
{
   ROOT::Experimental::Detail::RColumnElement<
      ROOT::Experimental::ClusterSize_t, ROOT::Experimental::EColumnType::kIndex> element(nullptr);
   element.Pack(nullptr, nullptr, 0);
   element.Unpack(nullptr, nullptr, 0);

   ROOT::Experimental::ClusterSize_t v(42);
   char vPacked[4];
   element.Pack(vPacked, &v, 1);
   element.Unpack(&v, vPacked, 1);
   EXPECT_EQ(42u, v);

   ROOT::Experimental::ClusterSize_t mv2[2];
   mv2[0] = 0;
   mv2[1] = 5;
   char mvPacked2[8];
   element.Pack(mvPacked2, mv2, 2);
   element.Unpack(mv2, mvPacked2, 2);
   EXPECT_EQ(0u, std::uint32_t(mv2[0]));
   EXPECT_EQ(5u, std::uint32_t(mv2[1]));

   ROOT::Experimental::ClusterSize_t mv3[3];
   mv3[0] = 1;
   mv3[1] = 4;
   mv3[2] = 5;
   char mvPacked3[12];
   element.Pack(mvPacked3, mv3, 3);
   element.Unpack(mv3, mvPacked3, 3);
   EXPECT_EQ(1u, std::uint32_t(mv3[0]));
   EXPECT_EQ(4u, std::uint32_t(mv3[1]));
   EXPECT_EQ(5u, std::uint32_t(mv3[2]));
}


TEST(Packing, Int32Split)
{
   ROOT::Experimental::Detail::RColumnElement<std::int32_t, ROOT::Experimental::EColumnType::kInt32> element(nullptr);
   element.Pack(nullptr, nullptr, 0);
   element.Unpack(nullptr, nullptr, 0);

   std::int32_t i = 42;
   char iPacked[4];
   element.Pack(iPacked, &i, 1);
   element.Unpack(&i, iPacked, 1);
   EXPECT_EQ(42, i);

   std::int32_t mi[] = {-1, 2, -3};
   char miPacked[12];
   element.Pack(miPacked, mi, 3);
   element.Unpack(mi, miPacked, 3);
   EXPECT_EQ(-1, mi[0]);
   EXPECT_EQ(2, mi[1]);
   EXPECT_EQ(-3, mi[2]);
}


TEST(Packing, UInt32Split)
{
   ROOT::Experimental::Detail::RColumnElement<std::uint32_t, ROOT::Experimental::EColumnType::kInt32> element(nullptr);
   element.Pack(nullptr, nullptr, 0);
   element.Unpack(nullptr, nullptr, 0);

   std::uint32_t i = 42;
   char iPacked[4];
   element.Pack(iPacked, &i, 1);
   element.Unpack(&i, iPacked, 1);
   EXPECT_EQ(42u, i);

   std::uint32_t mi[] = {1, 2, 3};
   char miPacked[12];
   element.Pack(miPacked, mi, 3);
   element.Unpack(mi, miPacked, 3);
   EXPECT_EQ(1u, mi[0]);
   EXPECT_EQ(2u, mi[1]);
   EXPECT_EQ(3u, mi[2]);
}


TEST(Packing, Int64Split)
{
   ROOT::Experimental::Detail::RColumnElement<std::int64_t, ROOT::Experimental::EColumnType::kInt64> element(nullptr);
   element.Pack(nullptr, nullptr, 0);
   element.Unpack(nullptr, nullptr, 0);

   std::int64_t i = 42;
   char iPacked[8];
   element.Pack(iPacked, &i, 1);
   element.Unpack(&i, iPacked, 1);
   EXPECT_EQ(42, i);

   std::int64_t mi[] = {-1, 2, -3};
   char miPacked[24];
   element.Pack(miPacked, mi, 3);
   element.Unpack(mi, miPacked, 3);
   EXPECT_EQ(-1, mi[0]);
   EXPECT_EQ(2, mi[1]);
   EXPECT_EQ(-3, mi[2]);
}


TEST(Packing, UInt64Split)
{
   ROOT::Experimental::Detail::RColumnElement<std::uint64_t, ROOT::Experimental::EColumnType::kInt64> element(nullptr);
   element.Pack(nullptr, nullptr, 0);
   element.Unpack(nullptr, nullptr, 0);

   std::uint64_t i = 42;
   char iPacked[8];
   element.Pack(iPacked, &i, 1);
   element.Unpack(&i, iPacked, 1);
   EXPECT_EQ(42u, i);

   std::uint64_t mi[] = {1, 2, 3};
   char miPacked[24];
   element.Pack(miPacked, mi, 3);
   element.Unpack(mi, miPacked, 3);
   EXPECT_EQ(1u, mi[0]);
   EXPECT_EQ(2u, mi[1]);
   EXPECT_EQ(3u, mi[2]);
}
