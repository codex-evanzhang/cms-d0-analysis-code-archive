#include <algorithm>
#include <cstdio>

#include "TFile.h"
#include "TTree.h"

void summarize_reduced_tree_zdc(const char *path, Long64_t maxEntries = 1000000) {
  TFile file(path, "READ");
  if (file.IsZombie()) {
    std::fprintf(stderr, "could_not_open\t%s\n", path);
    return;
  }
  auto *tree = static_cast<TTree *>(file.Get("Tree"));
  if (!tree) {
    std::fprintf(stderr, "missing_tree\tTree\n");
    return;
  }

  Float_t zdcPlus = 0.0;
  Float_t zdcMinus = 0.0;
  Float_t hfPlus = 0.0;
  Float_t hfMinus = 0.0;
  Bool_t zdcGammaN = false;
  Bool_t zdcNgamma = false;
  tree->SetBranchStatus("*", 0);
  tree->SetBranchStatus("ZDCsumPlus", 1);
  tree->SetBranchStatus("ZDCsumMinus", 1);
  tree->SetBranchStatus("HFEMaxPlus", 1);
  tree->SetBranchStatus("HFEMaxMinus", 1);
  tree->SetBranchStatus("ZDCgammaN", 1);
  tree->SetBranchStatus("ZDCNgamma", 1);
  tree->SetBranchAddress("ZDCsumPlus", &zdcPlus);
  tree->SetBranchAddress("ZDCsumMinus", &zdcMinus);
  tree->SetBranchAddress("HFEMaxPlus", &hfPlus);
  tree->SetBranchAddress("HFEMaxMinus", &hfMinus);
  tree->SetBranchAddress("ZDCgammaN", &zdcGammaN);
  tree->SetBranchAddress("ZDCNgamma", &zdcNgamma);

  const Long64_t entries = tree->GetEntries();
  const Long64_t n = std::min(entries, maxEntries);
  Long64_t zdcNonzero = 0;
  Long64_t zdcPlusGt1100 = 0;
  Long64_t zdcMinusGt1000 = 0;
  Long64_t gammaN = 0;
  Long64_t ngamma = 0;
  Long64_t hfNonzero = 0;
  double maxZdcPlus = 0.0;
  double maxZdcMinus = 0.0;
  double maxHfPlus = 0.0;
  double maxHfMinus = 0.0;

  for (Long64_t i = 0; i < n; ++i) {
    tree->GetEntry(i);
    if (zdcPlus != 0.0 || zdcMinus != 0.0) ++zdcNonzero;
    if (zdcPlus > 1100.0) ++zdcPlusGt1100;
    if (zdcMinus > 1000.0) ++zdcMinusGt1000;
    if (zdcGammaN) ++gammaN;
    if (zdcNgamma) ++ngamma;
    if (hfPlus != 0.0 || hfMinus != 0.0) ++hfNonzero;
    maxZdcPlus = std::max(maxZdcPlus, static_cast<double>(zdcPlus));
    maxZdcMinus = std::max(maxZdcMinus, static_cast<double>(zdcMinus));
    maxHfPlus = std::max(maxHfPlus, static_cast<double>(hfPlus));
    maxHfMinus = std::max(maxHfMinus, static_cast<double>(hfMinus));
  }

  std::printf("entries_total\t%lld\n", entries);
  std::printf("entries_checked\t%lld\n", n);
  std::printf("zdc_nonzero\t%lld\n", zdcNonzero);
  std::printf("zdc_plus_gt_1100\t%lld\n", zdcPlusGt1100);
  std::printf("zdc_minus_gt_1000\t%lld\n", zdcMinusGt1000);
  std::printf("ZDCgammaN_true\t%lld\n", gammaN);
  std::printf("ZDCNgamma_true\t%lld\n", ngamma);
  std::printf("HF_nonzero\t%lld\n", hfNonzero);
  std::printf("max_ZDCsumPlus\t%.6f\n", maxZdcPlus);
  std::printf("max_ZDCsumMinus\t%.6f\n", maxZdcMinus);
  std::printf("max_HFEMaxPlus\t%.6f\n", maxHfPlus);
  std::printf("max_HFEMaxMinus\t%.6f\n", maxHfMinus);
}
