#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "TTreeReader.h"
#include "TTreeReaderArray.h"
#include "TTreeReaderValue.h"

namespace {

bool has_branch(TTree *tree, const char *name) {
  return tree && tree->GetBranch(name);
}

int require_tree_branches(TFile &file, const char *treeName,
                          const std::vector<const char *> &branches) {
  TTree *tree = nullptr;
  file.GetObject(treeName, tree);
  if (!tree) {
    std::cout << "TREE\t" << treeName << "\tMISSING\n";
    return 1;
  }
  int missing = 0;
  std::cout << "TREE\t" << treeName << "\tentries\t" << tree->GetEntries()
            << "\tbranches\t" << tree->GetListOfBranches()->GetEntries() << "\n";
  for (const char *branch : branches) {
    if (!has_branch(tree, branch)) {
      std::cout << "MISSING_BRANCH\t" << treeName << "\t" << branch << "\n";
      ++missing;
    }
  }
  return missing;
}

int require_absent_tree(TFile &file, const char *treeName) {
  TTree *tree = nullptr;
  file.GetObject(treeName, tree);
  if (tree) {
    std::cout << "UNEXPECTED_TREE\t" << treeName << "\tentries\t"
              << tree->GetEntries() << "\n";
    return 1;
  }
  std::cout << "ABSENT_TREE_OK\t" << treeName << "\n";
  return 0;
}

bool close_float(float a, float b, float tol = 1e-4f) {
  return std::fabs(a - b) <= tol * std::max(1.0f, std::max(std::fabs(a), std::fabs(b)));
}

float hf_max(const std::vector<int> &pfId, const std::vector<float> &pfEta,
             const std::vector<float> &pfE, float etaMin, float etaMax) {
  float maxE = 0.0f;
  const size_t n = std::min({pfId.size(), pfEta.size(), pfE.size()});
  for (size_t i = 0; i < n; ++i) {
    if ((pfId[i] == 6 || pfId[i] == 7) && pfEta[i] > etaMin && pfEta[i] < etaMax) {
      maxE = std::max(maxE, pfE[i]);
    }
  }
  return maxE;
}

int compare_values(TFile &minimal, TFile &reference, Long64_t maxEntries) {
  int mismatches = 0;

  auto check_entries = [&](const char *treeName) {
    TTree *a = nullptr;
    TTree *b = nullptr;
    minimal.GetObject(treeName, a);
    reference.GetObject(treeName, b);
    if (!a || !b) {
      std::cout << "COMPARE_TREE_MISSING\t" << treeName << "\n";
      ++mismatches;
      return Long64_t{0};
    }
    if (a->GetEntries() != b->GetEntries()) {
      std::cout << "ENTRY_MISMATCH\t" << treeName << "\tminimal\t"
                << a->GetEntries() << "\treference\t" << b->GetEntries() << "\n";
      ++mismatches;
    }
    return std::min({a->GetEntries(), b->GetEntries(), maxEntries});
  };

  Long64_t n = check_entries("hiEvtAnalyzer/HiTree");
  {
    TTree *ta = nullptr;
    TTree *tb = nullptr;
    minimal.GetObject("hiEvtAnalyzer/HiTree", ta);
    reference.GetObject("hiEvtAnalyzer/HiTree", tb);
    TTreeReader ra(ta), rb(tb);
    TTreeReaderValue<unsigned int> arun(ra, "run"), alumi(ra, "lumi");
    TTreeReaderValue<unsigned long long> aevt(ra, "evt");
    TTreeReaderValue<unsigned int> brun(rb, "run"), blumi(rb, "lumi");
    TTreeReaderValue<unsigned long long> bevt(rb, "evt");
    for (Long64_t i = 0; i < n && ra.Next() && rb.Next(); ++i) {
      if (*arun != *brun || *alumi != *blumi || *aevt != *bevt) {
        std::cout << "VALUE_MISMATCH\thiEvtAnalyzer/HiTree\tentry\t" << i << "\n";
        ++mismatches;
        break;
      }
    }
  }

  n = check_entries("zdcanalyzer/zdcrechit");
  {
    TTree *ta = nullptr;
    TTree *tb = nullptr;
    minimal.GetObject("zdcanalyzer/zdcrechit", ta);
    reference.GetObject("zdcanalyzer/zdcrechit", tb);
    TTreeReader ra(ta), rb(tb);
    TTreeReaderValue<float> aplus(ra, "sumPlus"), aminus(ra, "sumMinus");
    TTreeReaderValue<float> bplus(rb, "sumPlus"), bminus(rb, "sumMinus");
    for (Long64_t i = 0; i < n && ra.Next() && rb.Next(); ++i) {
      if (!close_float(*aplus, *bplus) || !close_float(*aminus, *bminus)) {
        std::cout << "VALUE_MISMATCH\tzdcanalyzer/zdcrechit\tentry\t" << i
                  << "\tminimal\t" << *aplus << "," << *aminus
                  << "\treference\t" << *bplus << "," << *bminus << "\n";
        ++mismatches;
        break;
      }
    }
  }

  n = check_entries("particleFlowAnalyser/pftree");
  {
    TTree *ta = nullptr;
    TTree *tb = nullptr;
    minimal.GetObject("particleFlowAnalyser/pftree", ta);
    reference.GetObject("particleFlowAnalyser/pftree", tb);
    TTreeReader ra(ta), rb(tb);
    TTreeReaderValue<std::vector<int>> aid(ra, "pfId"), bid(rb, "pfId");
    TTreeReaderValue<std::vector<float>> aeta(ra, "pfEta"), beta(rb, "pfEta");
    TTreeReaderValue<std::vector<float>> ae(ra, "pfE"), be(rb, "pfE");
    for (Long64_t i = 0; i < n && ra.Next() && rb.Next(); ++i) {
      const float aplus = hf_max(*aid, *aeta, *ae, 3.0f, 5.2f);
      const float aminus = hf_max(*aid, *aeta, *ae, -5.2f, -3.0f);
      const float bplus = hf_max(*bid, *beta, *be, 3.0f, 5.2f);
      const float bminus = hf_max(*bid, *beta, *be, -5.2f, -3.0f);
      if (!close_float(aplus, bplus) || !close_float(aminus, bminus)) {
        std::cout << "VALUE_MISMATCH\tparticleFlowAnalyser/pftree\tentry\t" << i
                  << "\tminimal_hf\t" << aplus << "," << aminus
                  << "\treference_hf\t" << bplus << "," << bminus << "\n";
        ++mismatches;
        break;
      }
    }
  }

  n = check_entries("Dfinder/ntDkpi");
  {
    TTree *ta = nullptr;
    TTree *tb = nullptr;
    minimal.GetObject("Dfinder/ntDkpi", ta);
    reference.GetObject("Dfinder/ntDkpi", tb);
    TTreeReader ra(ta), rb(tb);
    TTreeReaderValue<int> asize(ra, "Dsize"), bsize(rb, "Dsize");
    TTreeReaderArray<float> amass(ra, "Dmass"), bmass(rb, "Dmass");
    TTreeReaderArray<float> apt(ra, "Dpt"), bpt(rb, "Dpt");
    TTreeReaderArray<float> ay(ra, "Dy"), by(rb, "Dy");
    TTreeReaderArray<float> aalpha(ra, "Dalpha"), balpha(rb, "Dalpha");
    TTreeReaderArray<float> asvpv(ra, "DsvpvDistance"), bsvpv(rb, "DsvpvDistance");
    for (Long64_t i = 0; i < n && ra.Next() && rb.Next(); ++i) {
      if (*asize != *bsize) {
        std::cout << "VALUE_MISMATCH\tDfinder/ntDkpi\tDsize\tentry\t" << i
                  << "\tminimal\t" << *asize << "\treference\t" << *bsize << "\n";
        ++mismatches;
        break;
      }
      const int limit = std::min<int>(*asize, 5);
      for (int j = 0; j < limit; ++j) {
        if (!close_float(amass[j], bmass[j]) || !close_float(apt[j], bpt[j]) ||
            !close_float(ay[j], by[j]) || !close_float(aalpha[j], balpha[j]) ||
            !close_float(asvpv[j], bsvpv[j])) {
          std::cout << "VALUE_MISMATCH\tDfinder/ntDkpi\tentry\t" << i
                    << "\tcandidate\t" << j << "\n";
          ++mismatches;
          i = n;
          break;
        }
      }
    }
  }

  return mismatches;
}

}  // namespace

int validate_minimal_2023_d0_forest(const char *minimalPath,
                                    const char *referencePath = "",
                                    Long64_t maxCompareEntries = 50) {
  TFile minimal(minimalPath, "READ");
  if (minimal.IsZombie()) {
    std::cerr << "ERROR\tcould_not_open_minimal\t" << minimalPath << "\n";
    return 2;
  }

  int failures = 0;
  failures += require_tree_branches(minimal, "hiEvtAnalyzer/HiTree",
                                    {"run", "lumi", "evt"});
  failures += require_tree_branches(minimal, "PbPbTracks/trackTree",
                                    {"nVtx", "zVtx", "ptSumVtx"});
  failures += require_tree_branches(minimal, "skimanalysis/HltTree",
                                    {"pprimaryVertexFilter", "pclusterCompatibilityFilter",
                                     "pzdcEnergyFilter0nOr"});
  failures += require_tree_branches(minimal, "l1MetFilterRecoTree/MetFilterRecoTree",
                                    {"cscTightHalo2015Filter"});
  failures += require_tree_branches(minimal, "zdcanalyzer/zdcrechit",
                                    {"sumPlus", "sumMinus", "n"});
  failures += require_tree_branches(minimal, "particleFlowAnalyser/pftree",
                                    {"nPF", "pfId", "pfEta", "pfE"});
  failures += require_tree_branches(minimal, "Dfinder/ntDkpi",
                                    {"RunNo", "EvtNo", "LumiNo", "Dsize", "Dtype",
                                     "Dmass", "Dpt", "Deta", "Dphi", "Dy",
                                     "Dchi2cl", "Ddtheta", "Dalpha",
                                     "DsvpvDistance", "DsvpvDisErr",
                                     "Dtrk1Pt", "Dtrk2Pt", "Dtrk1PtErr",
                                     "Dtrk2PtErr", "Dtrk1Eta", "Dtrk2Eta",
                                     "Dtrk1highPurity", "Dtrk2highPurity"});
  failures += require_absent_tree(minimal, "Dfinder/root");
  failures += require_absent_tree(minimal, "zdcanalyzer/zdcdigi");
  failures += require_absent_tree(minimal, "muonAnalyzer/MuonTree");
  failures += require_absent_tree(minimal, "hltMuTree/HLTMuTree");

  if (referencePath && std::string(referencePath).size() > 0) {
    TFile reference(referencePath, "READ");
    if (reference.IsZombie()) {
      std::cerr << "ERROR\tcould_not_open_reference\t" << referencePath << "\n";
      return 2;
    }
    failures += compare_values(minimal, reference, maxCompareEntries);
  }

  if (failures == 0) {
    std::cout << "MINIMAL_SCHEMA_STATUS\tPASS\n";
    return 0;
  }
  std::cout << "MINIMAL_SCHEMA_STATUS\tFAIL\tfailures=" << failures << "\n";
  return 1;
}
