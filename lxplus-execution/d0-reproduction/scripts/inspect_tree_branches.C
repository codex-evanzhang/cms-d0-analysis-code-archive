#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "TBranch.h"
#include "TFile.h"
#include "TKey.h"
#include "TTree.h"

namespace {
void ListKeys(TDirectory *dir, const std::string &prefix, int depth) {
  if (!dir || depth > 2) return;
  TIter next(dir->GetListOfKeys());
  while (TKey *key = static_cast<TKey *>(next())) {
    std::string name = key->GetName();
    std::string cls = key->GetClassName();
    std::cout << "KEY\t" << prefix << name << "\t" << cls << "\n";
    TObject *obj = key->ReadObj();
    if (obj && obj->InheritsFrom(TDirectory::Class())) {
      ListKeys(static_cast<TDirectory *>(obj), prefix + name + "/", depth + 1);
    }
    delete obj;
  }
}

bool WantBranch(const std::string &name) {
  static const std::vector<std::string> needles = {
      "D", "trk", "Track", "mass", "pt", "eta", "y", "alpha", "svpv",
      "chi", "prob", "theta", "match", "gen", "G", "HLT", "ZDC", "HF",
      "vz", "Vtx", "event", "run", "lumi"};
  for (const auto &needle : needles) {
    if (name.find(needle) != std::string::npos) return true;
  }
  return false;
}

TTree *FindTree(TFile &file, const std::vector<std::string> &names) {
  for (const auto &name : names) {
    TTree *tree = nullptr;
    file.GetObject(name.c_str(), tree);
    if (tree) return tree;
  }
  return nullptr;
}
}

int inspect_tree_branches(const char *path, const char *treeName = "") {
  TFile file(path, "READ");
  if (file.IsZombie()) {
    std::cerr << "Could not open " << path << "\n";
    return 1;
  }

  std::cout << "FILE\t" << path << "\n";
  ListKeys(&file, "", 0);

  TTree *tree = nullptr;
  if (std::string(treeName).size()) {
    file.GetObject(treeName, tree);
  } else {
    tree = FindTree(file, {"Tree", "SelectedD", "Dfinder/ntDkpi", "ntDkpi"});
  }
  if (!tree) {
    std::cout << "TREE\tNONE\n";
    return 0;
  }

  std::cout << "TREE\t" << tree->GetName() << "\tentries\t" << tree->GetEntries() << "\n";
  TObjArray *branches = tree->GetListOfBranches();
  for (int i = 0; i < branches->GetEntriesFast(); ++i) {
    auto *branch = static_cast<TBranch *>(branches->At(i));
    if (!branch) continue;
    std::string name = branch->GetName();
    if (WantBranch(name)) {
      std::cout << "BRANCH\t" << name << "\t" << branch->GetClassName() << "\t"
                << branch->GetTitle() << "\n";
    }
  }
  return 0;
}
