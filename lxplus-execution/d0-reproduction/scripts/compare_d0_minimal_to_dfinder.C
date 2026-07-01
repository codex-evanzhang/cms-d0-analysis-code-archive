#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TLegend.h>
#include <TLine.h>
#include <TROOT.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

struct EventKey {
  int run = 0;
  int lumi = 0;
  long long event = 0;

  bool operator==(const EventKey& other) const {
    return run == other.run && lumi == other.lumi && event == other.event;
  }
};

struct EventKeyHash {
  std::size_t operator()(const EventKey& key) const {
    std::size_t seed = std::hash<int>{}(key.run);
    seed ^= std::hash<int>{}(key.lumi) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<long long>{}(key.event) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};

bool isOwnedOutput(const TString& path) {
  return path.BeginsWith("/afs/cern.ch/user/e/evzhang") ||
         path.BeginsWith("/afs/cern.ch/work/e/evzhang") ||
         path.BeginsWith("/eos/user/e/evzhang");
}

TH1D* makeHist(const char* name, const char* title, int color) {
  auto* hist = new TH1D(name, title, 160, 1.5, 2.3);
  hist->Sumw2();
  hist->SetDirectory(nullptr);
  hist->SetLineColor(color);
  hist->SetMarkerColor(color);
  hist->SetLineWidth(2);
  hist->SetMarkerStyle(20);
  hist->SetMarkerSize(0.55);
  hist->GetXaxis()->SetTitle("m(K#pi) [GeV]");
  hist->GetYaxis()->SetTitle("Candidates");
  return hist;
}

void writeCsv(const TH1D* hist, const TString& path) {
  std::ofstream out(path.Data());
  out << "bin,low_edge,high_edge,center,count,error\n";
  for (int i = 1; i <= hist->GetNbinsX(); ++i) {
    out << i << "," << std::setprecision(8)
        << hist->GetXaxis()->GetBinLowEdge(i) << ","
        << hist->GetXaxis()->GetBinUpEdge(i) << ","
        << hist->GetXaxis()->GetBinCenter(i) << ","
        << hist->GetBinContent(i) << ","
        << hist->GetBinError(i) << "\n";
  }
}

TH1D* normalizedClone(const TH1D* hist, const char* name, int color) {
  auto* clone = static_cast<TH1D*>(hist->Clone(name));
  clone->SetDirectory(nullptr);
  clone->SetLineColor(color);
  clone->SetMarkerColor(color);
  if (clone->Integral() > 0) {
    clone->Scale(1.0 / clone->Integral());
  }
  clone->GetYaxis()->SetTitle("Normalized candidates");
  return clone;
}

void drawOverlay(const std::vector<TH1D*>& hists,
                 const std::vector<TString>& labels,
                 const TString& outputStem,
                 bool normalized = false) {
  gStyle->SetOptStat(0);
  TCanvas canvas("canvas", "", 950, 720);
  canvas.SetLeftMargin(0.12);
  canvas.SetRightMargin(0.04);
  canvas.SetTopMargin(0.08);
  canvas.SetBottomMargin(0.12);

  std::vector<TH1D*> drawHists;
  if (normalized) {
    const std::vector<int> colors = {kBlue + 2, kAzure + 1, kRed + 1, kMagenta + 1, kGreen + 2};
    for (std::size_t i = 0; i < hists.size(); ++i) {
      drawHists.push_back(normalizedClone(hists[i], TString::Format("%s_norm", hists[i]->GetName()), colors[i % colors.size()]));
    }
  } else {
    drawHists = hists;
  }

  double maxY = 0.0;
  for (const auto* hist : drawHists) maxY = std::max(maxY, hist->GetMaximum());

  bool first = true;
  for (auto* hist : drawHists) {
    hist->SetMaximum(maxY > 0 ? maxY * 1.25 : 1.0);
    hist->Draw(first ? "hist" : "hist same");
    first = false;
  }

  TLine pdg(1.86484, 0.0, 1.86484, maxY > 0 ? maxY * 1.15 : 1.0);
  pdg.SetLineColor(kGray + 2);
  pdg.SetLineStyle(2);
  pdg.SetLineWidth(2);
  pdg.Draw("same");

  TLegend legend(0.40, 0.62, 0.92, 0.88);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  for (std::size_t i = 0; i < drawHists.size(); ++i) {
    legend.AddEntry(drawHists[i], labels[i], "l");
  }
  legend.AddEntry(&pdg, "PDG D^{0} mass", "l");
  legend.Draw();

  canvas.SaveAs(outputStem + ".png");
  canvas.SaveAs(outputStem + ".pdf");
}

bool minimalBaseline(std::size_t i,
                     const std::vector<float>* dPt,
                     const std::vector<float>* dY,
                     const std::vector<float>* trk1Pt,
                     const std::vector<float>* trk2Pt,
                     const std::vector<float>* trk1Eta,
                     const std::vector<float>* trk2Eta,
                     const std::vector<int>* trk1HighPurity,
                     const std::vector<int>* trk2HighPurity,
                     const std::vector<float>* trk1PtError,
                     const std::vector<float>* trk2PtError) {
  if (!(dPt->at(i) > 2.0 && dPt->at(i) < 12.0 && std::abs(dY->at(i)) < 2.0)) return false;
  if (trk1Pt->at(i) <= 1.0 || trk2Pt->at(i) <= 1.0) return false;
  if (std::abs(trk1Eta->at(i)) >= 2.4 || std::abs(trk2Eta->at(i)) >= 2.4) return false;
  if (trk1HighPurity->at(i) != 1 || trk2HighPurity->at(i) != 1) return false;
  if (trk1PtError->at(i) < 0.0 || trk2PtError->at(i) < 0.0) return false;
  return trk1PtError->at(i) / trk1Pt->at(i) < 0.1 && trk2PtError->at(i) / trk2Pt->at(i) < 0.1;
}

}  // namespace

int compare_d0_minimal_to_dfinder(const char* minimalPath,
                                  const char* referencePath,
                                  const char* outputDir,
                                  const char* inputLabel = "",
                                  Long64_t maxReferenceEntries = 0) {
  TString outDir(outputDir);
  if (!isOwnedOutput(outDir)) {
    std::cerr << "ERROR: refusing to write outside Evan-owned paths: " << outDir << "\n";
    return 2;
  }
  gSystem->mkdir(outDir, true);

  TFile minimalFile(minimalPath, "READ");
  if (minimalFile.IsZombie()) {
    std::cerr << "ERROR: failed to open " << minimalPath << "\n";
    return 2;
  }
  auto* minimalTree = dynamic_cast<TTree*>(minimalFile.Get("d0MinimalForest/d0minimal"));
  if (!minimalTree) {
    std::cerr << "ERROR: missing d0MinimalForest/d0minimal\n";
    return 2;
  }

  TFile referenceFile(referencePath, "READ");
  if (referenceFile.IsZombie()) {
    std::cerr << "ERROR: failed to open " << referencePath << "\n";
    return 2;
  }
  auto* referenceTree = dynamic_cast<TTree*>(referenceFile.Get("Tree"));
  if (!referenceTree) {
    std::cerr << "ERROR: missing Dfinder Tree\n";
    return 2;
  }

  unsigned int run = 0, lumi = 0;
  unsigned long long event = 0;
  int nD0Candidates = 0;
  std::vector<float>* dMass = nullptr;
  std::vector<float>* dPt = nullptr;
  std::vector<float>* dY = nullptr;
  std::vector<float>* trk1Pt = nullptr;
  std::vector<float>* trk2Pt = nullptr;
  std::vector<float>* trk1Eta = nullptr;
  std::vector<float>* trk2Eta = nullptr;
  std::vector<int>* trk1HighPurity = nullptr;
  std::vector<int>* trk2HighPurity = nullptr;
  std::vector<float>* trk1PtError = nullptr;
  std::vector<float>* trk2PtError = nullptr;

  minimalTree->SetBranchAddress("run", &run);
  minimalTree->SetBranchAddress("lumi", &lumi);
  minimalTree->SetBranchAddress("event", &event);
  minimalTree->SetBranchAddress("nD0Candidates", &nD0Candidates);
  minimalTree->SetBranchAddress("dMass", &dMass);
  minimalTree->SetBranchAddress("dPt", &dPt);
  minimalTree->SetBranchAddress("dY", &dY);
  minimalTree->SetBranchAddress("trk1Pt", &trk1Pt);
  minimalTree->SetBranchAddress("trk2Pt", &trk2Pt);
  minimalTree->SetBranchAddress("trk1Eta", &trk1Eta);
  minimalTree->SetBranchAddress("trk2Eta", &trk2Eta);
  minimalTree->SetBranchAddress("trk1HighPurity", &trk1HighPurity);
  minimalTree->SetBranchAddress("trk2HighPurity", &trk2HighPurity);
  minimalTree->SetBranchAddress("trk1PtError", &trk1PtError);
  minimalTree->SetBranchAddress("trk2PtError", &trk2PtError);

  auto* hMinimalRaw = makeHist("h_minimal_raw_kin", "Minimal forest raw kinematic candidates", kBlue + 2);
  auto* hMinimalBase = makeHist("h_minimal_baseline", "Minimal forest baseline-track candidates", kAzure + 1);

  std::unordered_set<EventKey, EventKeyHash> eventKeys;
  Long64_t minimalTotalCandidates = 0;
  Long64_t minimalRawKinematic = 0;
  Long64_t minimalBaselineCandidates = 0;

  for (Long64_t entry = 0; entry < minimalTree->GetEntries(); ++entry) {
    minimalTree->GetEntry(entry);
    eventKeys.insert(EventKey{static_cast<int>(run), static_cast<int>(lumi), static_cast<long long>(event)});
    minimalTotalCandidates += nD0Candidates;
    for (std::size_t i = 0; i < dMass->size(); ++i) {
      if (dPt->at(i) > 2.0 && dPt->at(i) < 12.0 && std::abs(dY->at(i)) < 2.0) {
        hMinimalRaw->Fill(dMass->at(i));
        ++minimalRawKinematic;
      }
      if (minimalBaseline(i, dPt, dY, trk1Pt, trk2Pt, trk1Eta, trk2Eta,
                          trk1HighPurity, trk2HighPurity, trk1PtError, trk2PtError)) {
        hMinimalBase->Fill(dMass->at(i));
        ++minimalBaselineCandidates;
      }
    }
  }

  int refRun = 0, refLumi = 0;
  long long refEvent = 0;
  std::vector<float>* refDmass = nullptr;
  std::vector<float>* refDpt = nullptr;
  std::vector<float>* refDy = nullptr;
  std::vector<bool>* refPass = nullptr;

  referenceTree->SetBranchStatus("*", 0);
  for (const char* branch : {"Run", "Lumi", "Event", "Dmass", "Dpt", "Dy", "DpassCut23PAS"}) {
    referenceTree->SetBranchStatus(branch, 1);
  }
  referenceTree->SetBranchAddress("Run", &refRun);
  referenceTree->SetBranchAddress("Lumi", &refLumi);
  referenceTree->SetBranchAddress("Event", &refEvent);
  referenceTree->SetBranchAddress("Dmass", &refDmass);
  referenceTree->SetBranchAddress("Dpt", &refDpt);
  referenceTree->SetBranchAddress("Dy", &refDy);
  referenceTree->SetBranchAddress("DpassCut23PAS", &refPass);

  auto* hDfinderMatchedKin = makeHist("h_dfinder_matched_kin", "Dfinder matched-event kinematic candidates", kRed + 1);
  auto* hDfinderMatchedSelected = makeHist("h_dfinder_matched_selected", "Dfinder matched-event selected candidates", kMagenta + 1);
  auto* hDfinderFullSelected = makeHist("h_dfinder_full_selected", "Dfinder full selected reference", kGreen + 2);
  gROOT->cd();
  hDfinderFullSelected->SetDirectory(gROOT);
  hDfinderFullSelected->Reset();
  referenceTree->Draw("Dmass>>+h_dfinder_full_selected", "Dpt>2 && Dpt<12 && abs(Dy)<2 && DpassCut23PAS", "goff");
  hDfinderFullSelected->SetDirectory(nullptr);
  hDfinderFullSelected->SetLineColor(kGreen + 2);
  hDfinderFullSelected->SetLineWidth(2);
  const Long64_t fullSelected = static_cast<Long64_t>(hDfinderFullSelected->Integral());

  const Long64_t referenceEntries = referenceTree->GetEntries();
  const Long64_t limit = (maxReferenceEntries > 0 && maxReferenceEntries < referenceEntries) ? maxReferenceEntries : referenceEntries;
  Long64_t matchedEvents = 0;
  Long64_t matchedCandidateRows = 0;
  Long64_t matchedKinematic = 0;
  Long64_t matchedSelected = 0;

  TBranch* bRun = referenceTree->GetBranch("Run");
  TBranch* bLumi = referenceTree->GetBranch("Lumi");
  TBranch* bEvent = referenceTree->GetBranch("Event");
  TBranch* bDmass = referenceTree->GetBranch("Dmass");
  TBranch* bDpt = referenceTree->GetBranch("Dpt");
  TBranch* bDy = referenceTree->GetBranch("Dy");
  TBranch* bPass = referenceTree->GetBranch("DpassCut23PAS");

  for (Long64_t entry = 0; entry < limit; ++entry) {
    bRun->GetEntry(entry);
    bLumi->GetEntry(entry);
    bEvent->GetEntry(entry);
    const bool matched = eventKeys.find(EventKey{refRun, refLumi, refEvent}) != eventKeys.end();
    if (!matched) continue;

    bDmass->GetEntry(entry);
    bDpt->GetEntry(entry);
    bDy->GetEntry(entry);
    bPass->GetEntry(entry);

    ++matchedEvents;
    matchedCandidateRows += refDmass->size();
    for (std::size_t i = 0; i < refDmass->size(); ++i) {
      const bool kinematic = refDpt->at(i) > 2.0 && refDpt->at(i) < 12.0 && std::abs(refDy->at(i)) < 2.0;
      if (!kinematic) continue;
      hDfinderMatchedKin->Fill(refDmass->at(i));
      ++matchedKinematic;
      if (refPass->at(i)) {
        hDfinderMatchedSelected->Fill(refDmass->at(i));
        ++matchedSelected;
      }
    }
  }

  TFile outputRoot(outDir + "/minimal_vs_dfinder_histograms.root", "RECREATE");
  for (auto* hist : {hMinimalRaw, hMinimalBase, hDfinderMatchedKin, hDfinderMatchedSelected, hDfinderFullSelected}) {
    hist->Write();
    writeCsv(hist, outDir + "/" + hist->GetName() + ".csv");
  }
  outputRoot.Close();

  drawOverlay({hMinimalRaw, hMinimalBase, hDfinderMatchedKin, hDfinderMatchedSelected},
              {"Minimal raw, matched events", "Minimal baseline tracks, matched events",
               "Dfinder raw kinematic, matched events", "Dfinder selected, matched events"},
              outDir + "/minimal_vs_dfinder_matched_events", false);
  drawOverlay({hMinimalRaw, hMinimalBase, hDfinderMatchedKin, hDfinderMatchedSelected},
              {"Minimal raw, matched events", "Minimal baseline tracks, matched events",
               "Dfinder raw kinematic, matched events", "Dfinder selected, matched events"},
              outDir + "/minimal_vs_dfinder_matched_events_normalized", true);
  drawOverlay({hMinimalBase, hDfinderFullSelected},
              {"Minimal baseline tracks, processed events", "Dfinder full selected reference"},
              outDir + "/minimal_baseline_vs_full_dfinder_selected_normalized", true);

  std::ofstream manifest((outDir + "/minimal_vs_dfinder_manifest.md").Data());
  manifest << "# Minimal Forest vs Dfinder Comparison\n\n";
  manifest << "- Minimal forest: `" << minimalPath << "`\n";
  manifest << "- Dfinder reference: `" << referencePath << "`\n";
  manifest << "- Input label: `" << inputLabel << "`\n";
  manifest << "- Matched unique minimal events: `" << eventKeys.size() << "`\n\n";
  manifest << "## Counts\n\n";
  manifest << "- `minimal_events`: `" << minimalTree->GetEntries() << "`\n";
  manifest << "- `minimal_total_candidates`: `" << minimalTotalCandidates << "`\n";
  manifest << "- `minimal_raw_kinematic_candidates`: `" << minimalRawKinematic << "`\n";
  manifest << "- `minimal_baseline_candidates`: `" << minimalBaselineCandidates << "`\n";
  manifest << "- `dfinder_reference_entries`: `" << referenceEntries << "`\n";
  manifest << "- `dfinder_reference_scanned_entries`: `" << limit << "`\n";
  manifest << "- `dfinder_matched_events`: `" << matchedEvents << "`\n";
  manifest << "- `dfinder_matched_candidate_rows`: `" << matchedCandidateRows << "`\n";
  manifest << "- `dfinder_matched_kinematic_candidates`: `" << matchedKinematic << "`\n";
  manifest << "- `dfinder_matched_selected_candidates`: `" << matchedSelected << "`\n";
  manifest << "- `dfinder_full_selected_candidates`: `" << fullSelected << "`\n\n";
  manifest << "## Interpretation Guardrail\n\n";
  manifest << "The minimal forest lacks Dfinder's secondary-vertex fit, decay-length significance, "
           << "pointing-angle, and topological selections. Its baseline-track spectrum is expected "
           << "to be a broad combinatorial distribution, not a clean D0 peak.\n";
  manifest.close();

  std::cout << outDir << "/minimal_vs_dfinder_manifest.md\n";
  return 0;
}
