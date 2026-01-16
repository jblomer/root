#include <gtest/gtest.h>

#include <TFile.h>
#include <TNamed.h>
#include <TXMLFile.h>

#include <string>

TEST(TBufferXML, CorruptHeader)
{
   auto f = TFile::Open("ttte.xml", "RECREATE");
   f->SetCompressionSettings(101);

   std::string stdstring(1000, 'x');
   TNamed tnamed;
   tnamed.SetName(stdstring.c_str());
   f->WriteObject(&stdstring, "stdstring");
   f->WriteObject(&tnamed, "tnamed");

   f->Close();
   delete f;

//
//   auto keysInfo = writableFile.WalkTKeys();
//   std::size_t posZipStdString = 0;
//   std::size_t posZipTNamed = 0;
//   for (const auto &ki : keysInfo) {
//      if (ki.fKeyName == "stdstring") {
//         EXPECT_LT(ki.fLen, ki.fObjLen); // ensure it's compressed
//         posZipStdString = ki.fSeekKey + ki.fKeyLen;
//      } else if (ki.fKeyName == "tnamed") {
//         EXPECT_LT(ki.fLen, ki.fObjLen); // ensure it's compressed
//         posZipTNamed = ki.fSeekKey + ki.fKeyLen;
//      }
//   }
//   EXPECT_GT(posZipStdString, 0);
//   EXPECT_GT(posZipTNamed, 0);
//
//   writableFile.Close();
//

//   TNamed named;
//   named.SetName(std::string(1000, 'x').c_str());
//
//   TBufferXML buffer(TBufferXML::kWrite);
//   buffer.WriteObject(&named, false /* cacheReuse */);
//
   //auto xml = TBufferXML::ConvertToXML(&named);
   //printf("%s\n", std::string(xml).c_str());

   //std::string s(1000, 'x');
   //TBufferXML buffer(TBufferXML::kWrite);
   //buffer.WriteBuf(s.data(), 1000);
}
