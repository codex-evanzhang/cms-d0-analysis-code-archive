#include <TBranch.h>
#include <TFile.h>
#include <TObjArray.h>
#include <TTree.h>

#include <iostream>
#include <memory>

void list_zdc_tree_branches(const char *path) {
  std::unique_ptr<TFile> file(TFile::Open(path, "READ"));
  if (!file || file->IsZombie()) {
    std::cerr << "Could not open " << path << "\n";
    return;
  }
  const char *trees[] = {"zdcanalyzer/zdcrechit", "zdcanalyzer/zdcdigi"};
  for (const char *treeName : trees) {
    auto *tree = dynamic_cast<TTree *>(file->Get(treeName));
    if (!tree) {
      std::cout << "MISSING\t" << treeName << "\n";
      continue;
    }
    std::cout << "TREE\t" << treeName << "\tentries=" << tree->GetEntries() << "\n";
    TObjArray *branches = tree->GetListOfBranches();
    for (int i = 0; i < branches->GetEntries(); ++i) {
      auto *branch = dynamic_cast<TBranch *>(branches->At(i));
      if (!branch) continue;
      std::cout << "BRANCH\t" << branch->GetName() << "\t" << branch->GetClassName() << "\n";
    }
  }
}
