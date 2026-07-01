#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "TDirectory.h"
#include "TFile.h"
#include "TKey.h"
#include "TTree.h"

namespace {

TDirectory *ensure_dir(TFile &out, const std::string &path) {
  TDirectory *dir = &out;
  size_t start = 0;
  while (start < path.size()) {
    const size_t slash = path.find('/', start);
    const std::string part = path.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
    if (!part.empty()) {
      TDirectory *next = dynamic_cast<TDirectory *>(dir->Get(part.c_str()));
      if (!next) next = dir->mkdir(part.c_str());
      dir = next;
    }
    if (slash == std::string::npos) break;
    start = slash + 1;
  }
  return dir;
}

bool clone_tree(TFile &in, TFile &out, const std::string &path,
                const std::vector<std::string> &branches) {
  TTree *tree = nullptr;
  in.GetObject(path.c_str(), tree);
  if (!tree) {
    std::cerr << "MISSING_TREE\t" << path << "\n";
    return false;
  }

  if (!branches.empty()) {
    tree->SetBranchStatus("*", 0);
    for (const auto &branch : branches) {
      if (!tree->GetBranch(branch.c_str())) {
        std::cerr << "MISSING_BRANCH\t" << path << "\t" << branch << "\n";
        return false;
      }
      tree->SetBranchStatus(branch.c_str(), 1);
    }
  }

  const size_t slash = path.rfind('/');
  const std::string dirPath = slash == std::string::npos ? "" : path.substr(0, slash);
  TDirectory *dir = dirPath.empty() ? static_cast<TDirectory *>(&out) : ensure_dir(out, dirPath);
  dir->cd();
  TTree *copy = tree->CloneTree(-1, "fast");
  copy->Write(tree->GetName(), TObject::kOverwrite);
  out.cd();
  tree->SetBranchStatus("*", 1);
  std::cout << "CLONED_TREE\t" << path << "\tentries\t" << copy->GetEntries()
            << "\tbranches\t" << copy->GetListOfBranches()->GetEntries() << "\n";
  return true;
}

bool copy_object(TFile &in, TFile &out, const std::string &path) {
  TObject *obj = in.Get(path.c_str());
  if (!obj) {
    std::cerr << "MISSING_OBJECT\t" << path << "\n";
    return false;
  }
  const size_t slash = path.rfind('/');
  const std::string dirPath = slash == std::string::npos ? "" : path.substr(0, slash);
  TDirectory *dir = dirPath.empty() ? static_cast<TDirectory *>(&out) : ensure_dir(out, dirPath);
  dir->cd();
  obj->Write(obj->GetName(), TObject::kOverwrite);
  out.cd();
  std::cout << "COPIED_OBJECT\t" << path << "\n";
  return true;
}

}  // namespace

int minimize_2023_d0_forest(const char *inputPath, const char *outputPath) {
  TFile input(inputPath, "READ");
  if (input.IsZombie()) {
    std::cerr << "ERROR\tcould_not_open_input\t" << inputPath << "\n";
    return 2;
  }
  TFile output(outputPath, "RECREATE");
  if (output.IsZombie()) {
    std::cerr << "ERROR\tcould_not_open_output\t" << outputPath << "\n";
    return 2;
  }

  int failures = 0;
  failures += !copy_object(input, output, "HiForestInfo/HiForest");
  failures += !clone_tree(input, output, "hiEvtAnalyzer/HiTree",
                          {"run", "lumi", "evt"});
  failures += !clone_tree(input, output, "PbPbTracks/trackTree",
                          {"nVtx", "zVtx", "ptSumVtx"});
  failures += !clone_tree(input, output, "skimanalysis/HltTree",
                          {"pprimaryVertexFilter", "pclusterCompatibilityFilter",
                           "pzdcEnergyFilter0nOr", "pzdcEnergyFilter0nAnd",
                           "pzdcEnergyFilter1nOr", "pzdcEnergyFilterXOr"});
  failures += !clone_tree(input, output, "l1MetFilterRecoTree/MetFilterRecoTree",
                          {"cscTightHalo2015Filter"});
  failures += !clone_tree(input, output, "zdcanalyzer/zdcrechit",
                          {"sumPlus", "sumMinus", "n"});
  failures += !clone_tree(input, output, "particleFlowAnalyser/pftree",
                          {"nPF", "pfId", "pfEta", "pfE"});
  failures += !clone_tree(input, output, "Dfinder/ntDkpi",
                          {"RunNo", "EvtNo", "LumiNo", "Dsize", "Dtype",
                           "Dmass", "Dpt", "Deta", "Dphi", "Dy", "Dchi2cl",
                           "Ddtheta", "Dalpha", "DsvpvDistance",
                           "DsvpvDisErr", "Dtrk1Pt", "Dtrk2Pt",
                           "Dtrk1PtErr", "Dtrk2PtErr", "Dtrk1Eta",
                           "Dtrk2Eta", "Dtrk1highPurity",
                           "Dtrk2highPurity"});

  output.Close();
  if (failures == 0) {
    std::cout << "MINIMIZE_STATUS\tPASS\n";
    return 0;
  }
  std::cout << "MINIMIZE_STATUS\tFAIL\tfailures=" << failures << "\n";
  return 1;
}
