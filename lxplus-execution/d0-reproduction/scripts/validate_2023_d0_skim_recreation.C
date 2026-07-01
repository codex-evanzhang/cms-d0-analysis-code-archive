#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TKey.h>
#include <TString.h>
#include <TSystem.h>
#include <TTree.h>

#include <fstream>
#include <iostream>
#include <set>
#include <string>

namespace {

bool IsOwnedWritePath(const TString& path) {
  return path.BeginsWith("/afs/cern.ch/user/e/evzhang") ||
         path.BeginsWith("/afs/cern.ch/work/e/evzhang") ||
         path.BeginsWith("/eos/user/e/evzhang");
}

std::set<std::string> BranchNames(TTree* tree) {
  std::set<std::string> names;
  if (!tree) return names;
  for (auto* obj : *tree->GetListOfBranches()) {
    names.insert(obj->GetName());
  }
  return names;
}

void SaveMass(TTree* tree, const TString& expr, const TString& outPath) {
  TH1D hist("hist", ";m(K#pi) [GeV];Candidates", 160, 1.5, 2.3);
  tree->Draw("Dmass>>hist", expr, "goff");
  TCanvas canvas("canvas", "", 900, 700);
  hist.SetLineWidth(2);
  hist.Draw("hist");
  canvas.SaveAs(outPath);
}

}  // namespace

int validate_2023_d0_skim_recreation(const char* recreatedPath,
                                      const char* referencePath,
                                      const char* outDir) {
  if (!IsOwnedWritePath(TString(outDir))) {
    std::cerr << "ERROR: refusing to write outside Evan-owned CERN paths\n";
    return 1;
  }
  gSystem->mkdir(outDir, true);

  TFile recreated(recreatedPath, "READ");
  TFile reference(referencePath, "READ");
  auto* recreatedTree = dynamic_cast<TTree*>(recreated.Get("Tree"));
  auto* referenceTree = dynamic_cast<TTree*>(reference.Get("Tree"));
  if (!recreatedTree || !referenceTree) {
    std::cerr << "ERROR: missing Tree in recreated or reference file\n";
    return 1;
  }

  const auto recreatedBranches = BranchNames(recreatedTree);
  const auto referenceBranches = BranchNames(referenceTree);

  std::ofstream out((TString(outDir) + "/schema_comparison.tsv").Data());
  out << "metric\tvalue\n";
  out << "recreated_entries\t" << recreatedTree->GetEntries() << "\n";
  out << "reference_entries\t" << referenceTree->GetEntries() << "\n";
  out << "recreated_branch_count\t" << recreatedBranches.size() << "\n";
  out << "reference_branch_count\t" << referenceBranches.size() << "\n";
  int common = 0;
  for (const auto& branch : recreatedBranches) {
    if (referenceBranches.count(branch)) ++common;
  }
  out << "common_branch_count\t" << common << "\n";
  out << "\nmissing_from_recreated\n";
  for (const auto& branch : referenceBranches) {
    if (!recreatedBranches.count(branch)) out << branch << "\n";
  }
  out << "\nextra_in_recreated\n";
  for (const auto& branch : recreatedBranches) {
    if (!referenceBranches.count(branch)) out << branch << "\n";
  }
  out.close();

  SaveMass(recreatedTree, "DpassCut23PAS", TString(outDir) + "/recreated_dmass_passCut23PAS.png");
  SaveMass(referenceTree, "DpassCut23PAS", TString(outDir) + "/reference_dmass_passCut23PAS.png");

  std::cout << "Wrote validation to " << outDir << "\n";
  return 0;
}
