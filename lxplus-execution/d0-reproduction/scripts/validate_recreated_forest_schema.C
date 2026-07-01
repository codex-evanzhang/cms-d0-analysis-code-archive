#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "TFile.h"
#include "TTree.h"

int validate_recreated_forest_schema(const char *path) {
  TFile file(path, "READ");
  if (file.IsZombie()) {
    std::cerr << "ERROR\tcould_not_open\t" << path << "\n";
    return 2;
  }

  const std::map<std::string, std::vector<std::string>> required = {
      {"Dfinder/ntDkpi",
       {"Dsize", "Dmass", "Dpt", "Dy", "Deta", "Dphi", "Dchi2cl",
        "Dalpha", "DsvpvDistance", "DsvpvDisErr", "Dtrk1Pt", "Dtrk2Pt",
        "Dtrk1Eta", "Dtrk2Eta", "Dtrk1Phi", "Dtrk2Phi",
        "Dtrk1highPurity", "Dtrk2highPurity", "Dtrk1Dz", "Dtrk2Dz",
        "Dtrk1Dxy", "Dtrk2Dxy"}},
      {"zdcanalyzer/zdcrechit",
       {"sumPlus", "sumMinus", "n", "zside", "section", "channel", "energy",
        "time", "TDCtime", "chargeWeightedTime", "energySOIp1",
        "ratioSOIp1", "saturation"}},
      {"particleFlowAnalyser/pftree",
       {"nPF", "pfId", "pfPt", "pfEta", "pfPhi", "pfE", "pfM"}},
      {"hiEvtAnalyzer/HiTree",
       {"run", "evt", "lumi", "vx", "vy", "vz", "hiHF_pf", "hiHFPlus_pf",
        "hiHFMinus_pf", "nCountsHF_pf", "nCountsHFPlus_pf",
        "nCountsHFMinus_pf"}},
      {"skimanalysis/HltTree",
       {"pprimaryVertexFilter", "pclusterCompatibilityFilter",
        "pzdcEnergyFilter0nOr"}},
      {"PbPbTracks/trackTree",
       {"nVtx", "xVtx", "yVtx", "zVtx", "trkPt", "trkEta", "trkPhi",
        "highPurity", "trkDzAssociatedVtx", "trkDxyAssociatedVtx"}},
      {"hltanalysis/HltTree",
       {"Event", "Run", "LumiBlock"}},
  };

  int missing = 0;
  std::cout << "FILE\t" << path << "\n";
  for (const auto &item : required) {
    TTree *tree = nullptr;
    file.GetObject(item.first.c_str(), tree);
    if (!tree) {
      std::cout << "TREE\t" << item.first << "\tMISSING\n";
      ++missing;
      continue;
    }
    std::cout << "TREE\t" << item.first << "\tentries\t" << tree->GetEntries()
              << "\n";
    for (const auto &branch : item.second) {
      if (!tree->GetBranch(branch.c_str())) {
        std::cout << "MISSING_BRANCH\t" << item.first << "\t" << branch
                  << "\n";
        ++missing;
      }
    }
  }

  const std::map<std::string, std::vector<std::string>> optional = {
      {"zdcanalyzer/zdcdigi",
       {"n", "zside", "section", "channel", "adcTs0", "chargefCTs0",
        "tdcTs0", "adcTs1", "chargefCTs1", "tdcTs1", "adcTs2",
        "chargefCTs2", "tdcTs2", "adcTs3", "chargefCTs3", "tdcTs3",
        "adcTs4", "chargefCTs4", "tdcTs4", "adcTs5", "chargefCTs5",
        "tdcTs5"}},
      {"skimanalysis/HltTree",
       {"pzdcEnergyFilter0nAnd", "pzdcEnergyFilter1nOr",
        "pzdcEnergyFilterXOr"}},
      {"hltanalysis/HltTree",
       {"HLT_HIUPC_ZDC1nOR_SinglePixelTrackLowPt_MaxPixelCluster400_v8",
        "HLT_HIUPC_ZeroBias_SinglePixelTrack_MaxPixelTrack_v8"}},
  };

  for (const auto &item : optional) {
    TTree *tree = nullptr;
    file.GetObject(item.first.c_str(), tree);
    if (!tree) {
      std::cout << "OPTIONAL_TREE\t" << item.first << "\tMISSING\n";
      continue;
    }
    for (const auto &branch : item.second) {
      if (!tree->GetBranch(branch.c_str())) {
        std::cout << "OPTIONAL_MISSING_BRANCH\t" << item.first << "\t"
                  << branch << "\n";
      }
    }
  }

  if (missing == 0) {
    std::cout << "SCHEMA_STATUS\tPASS\n";
    return 0;
  }
  std::cout << "SCHEMA_STATUS\tFAIL\tmissing=" << missing << "\n";
  return 1;
}
