#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleModel.hxx>

#include <iostream>
#include <memory>
#include <utility>


int main() {
   auto model = ROOT::Experimental::RNTupleModel::Create();
   std::shared_ptr<int> fldAge = model->MakeField<int>("Age");
   auto ntuple = ROOT::Experimental::RNTupleReader::Open(std::move(model), "Staff", "data.root");
   ntuple->PrintInfo();

   std::cout << "The first entry in JSON format:" << std::endl;
   ntuple->Show(0);
   for (auto entryId : *ntuple) {
      // Populate fldAge
      ntuple->LoadEntry(entryId);
      std::cout << *fldAge << " ";
   }
   std::cout << std::endl;
   return 0;
}
