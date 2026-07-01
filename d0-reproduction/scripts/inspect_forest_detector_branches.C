#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

#include "TBranch.h"
#include "TDirectory.h"
#include "TFile.h"
#include "TKey.h"
#include "TTree.h"

namespace {
std::string Lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

bool Interesting(const std::string &name) {
  const std::string lower = Lower(name);
  const char *needles[] = {"zdc", "hf", "energy", "rechit", "eta", "pt", "phi", "sum"};
  for (const char *needle : needles) {
    if (lower.find(needle) != std::string::npos) return true;
  }
  return false;
}

void InspectTree(TTree *tree, const std::string &path) {
  if (!tree) return;
  std::cout << "TREE\t" << path << "\tentries=" << tree->GetEntries() << "\n";
  TObjArray *branches = tree->GetListOfBranches();
  for (int i = 0; i < branches->GetEntries(); ++i) {
    auto *branch = static_cast<TBranch *>(branches->At(i));
    const std::string name = branch->GetName();
    if (Interesting(name)) {
      std::cout << "BRANCH\t" << path << "\t" << name << "\t" << branch->GetClassName() << "\n";
    }
  }
}

void Walk(TDirectory *dir, const std::string &prefix) {
  TIter next(dir->GetListOfKeys());
  while (auto *obj = static_cast<TKey *>(next())) {
    TObject *read = obj->ReadObj();
    const std::string name = obj->GetName();
    const std::string path = prefix.empty() ? name : prefix + "/" + name;
    if (read->InheritsFrom(TTree::Class())) {
      InspectTree(static_cast<TTree *>(read), path);
    } else if (read->InheritsFrom(TDirectory::Class())) {
      Walk(static_cast<TDirectory *>(read), path);
    }
  }
}
}  // namespace

void inspect_forest_detector_branches(const char *path) {
  TFile file(path, "READ");
  if (file.IsZombie()) {
    std::cerr << "Could not open " << path << "\n";
    return;
  }
  Walk(&file, "");
}
