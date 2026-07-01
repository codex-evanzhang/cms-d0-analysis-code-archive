#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TLegend.h>
#include <TLine.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr int kMaxD = 250000;
constexpr int kNStages = 12;

struct EventKey {
  int run = 0;
  int lumi = 0;
  int event = 0;

  bool operator==(const EventKey& other) const {
    return run == other.run && lumi == other.lumi && event == other.event;
  }
};

struct EventKeyHash {
  std::size_t operator()(const EventKey& key) const {
    std::size_t seed = std::hash<int>{}(key.run);
    seed ^= std::hash<int>{}(key.lumi) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<int>{}(key.event) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};

struct Stage {
  const char* id;
  const char* label;
};

struct TopologyBin {
  double yLow;
  double yHigh;
  double ptLow;
  double ptHigh;
  double alphaMax;
  double svpvSigMin;
  double dthetaMax;
};

struct Candidate {
  float mass = std::numeric_limits<float>::quiet_NaN();
  float pt = std::numeric_limits<float>::quiet_NaN();
  float y = std::numeric_limits<float>::quiet_NaN();
  float trk1Pt = std::numeric_limits<float>::quiet_NaN();
  float trk2Pt = std::numeric_limits<float>::quiet_NaN();
  float trk1Eta = std::numeric_limits<float>::quiet_NaN();
  float trk2Eta = std::numeric_limits<float>::quiet_NaN();
  float trk1PtErr = std::numeric_limits<float>::quiet_NaN();
  float trk2PtErr = std::numeric_limits<float>::quiet_NaN();
  float alpha = std::numeric_limits<float>::quiet_NaN();
  float svpvDistance = std::numeric_limits<float>::quiet_NaN();
  float svpvDisErr = std::numeric_limits<float>::quiet_NaN();
  float chi2cl = std::numeric_limits<float>::quiet_NaN();
  float dtheta = std::numeric_limits<float>::quiet_NaN();
  bool trk1HighPurity = true;
  bool trk2HighPurity = true;
};

struct DReader {
  TTree* tree = nullptr;
  int run = 0;
  int lumi = 0;
  int event = 0;
  int dsize = 0;
  float mass[kMaxD];
  float pt[kMaxD];
  float y[kMaxD];
  float trk1Pt[kMaxD];
  float trk2Pt[kMaxD];
  float trk1Eta[kMaxD];
  float trk2Eta[kMaxD];
  float trk1PtErr[kMaxD];
  float trk2PtErr[kMaxD];
  float alpha[kMaxD];
  float svpvDistance[kMaxD];
  float svpvDisErr[kMaxD];
  float chi2cl[kMaxD];
  float dtheta[kMaxD];
  bool trk1HighPurity[kMaxD];
  bool trk2HighPurity[kMaxD];
};

const std::vector<Stage>& stages() {
  static const std::vector<Stage> value = {
      {"stage00_all", "all candidates"},
      {"stage01_mass", "|m-mD0|<0.2"},
      {"stage02_high_purity", "track high purity"},
      {"stage03_track_eta", "track |eta|<2.4"},
      {"stage04_track_pt", "track pT>1"},
      {"stage05_rel_pt_err", "track rel pT err<0.1"},
      {"stage06_dpt", "2<D pT<=12"},
      {"stage07_dy", "|D y|<=2"},
      {"stage08_alpha", "binned alpha"},
      {"stage09_svpv_sig", "binned SVPV sig"},
      {"stage10_sv_prob", "SV prob>0.1"},
      {"stage11_dtheta", "binned dtheta"},
  };
  return value;
}

const std::vector<TopologyBin>& topologyBins() {
  static const std::vector<TopologyBin> bins = {
      {0.0, 1.0, 2.0, 5.0, 0.40, 2.5, 0.50},
      {0.0, 1.0, 5.0, 8.0, 0.35, 3.5, 0.30},
      {0.0, 1.0, 8.0, 12.0, 0.40, 3.5, 0.30},
      {1.0, 2.0, 2.0, 5.0, 0.20, 2.5, 0.30},
      {1.0, 2.0, 5.0, 8.0, 0.25, 3.5, 0.30},
      {1.0, 2.0, 8.0, 12.0, 0.25, 3.5, 0.30},
  };
  return bins;
}

bool isOwnedOutput(const TString& path) {
  return path.BeginsWith("/afs/cern.ch/user/e/evzhang") ||
         path.BeginsWith("/afs/cern.ch/work/e/evzhang") ||
         path.BeginsWith("/eos/user/e/evzhang");
}

std::string keyString(const EventKey& key) {
  return std::to_string(key.run) + ":" + std::to_string(key.lumi) + ":" + std::to_string(key.event);
}

bool finite(float value) {
  return std::isfinite(static_cast<double>(value));
}

const TopologyBin* findTopologyBin(const Candidate& cand) {
  const double ay = std::abs(cand.y);
  for (const auto& bin : topologyBins()) {
    if (ay >= bin.yLow && ay < bin.yHigh && cand.pt >= bin.ptLow && cand.pt < bin.ptHigh) {
      return &bin;
    }
  }
  return nullptr;
}

bool passStage(const Candidate& cand, int stage) {
  if (stage <= 0) return finite(cand.mass);
  if (!passStage(cand, stage - 1)) return false;

  switch (stage) {
    case 1:
      return std::abs(cand.mass - 1.86484f) < 0.2f;
    case 2:
      return cand.trk1HighPurity && cand.trk2HighPurity;
    case 3:
      return std::abs(cand.trk1Eta) < 2.4f && std::abs(cand.trk2Eta) < 2.4f;
    case 4:
      return cand.trk1Pt > 1.0f && cand.trk2Pt > 1.0f;
    case 5:
      return cand.trk1Pt > 0.0f && cand.trk2Pt > 0.0f &&
             cand.trk1PtErr / cand.trk1Pt < 0.1f &&
             cand.trk2PtErr / cand.trk2Pt < 0.1f;
    case 6:
      return cand.pt > 2.0f && cand.pt <= 12.0f;
    case 7:
      return std::abs(cand.y) <= 2.0f;
    case 8: {
      const auto* bin = findTopologyBin(cand);
      return bin && cand.alpha < bin->alphaMax;
    }
    case 9: {
      const auto* bin = findTopologyBin(cand);
      return bin && cand.svpvDisErr > 0.0f && cand.svpvDistance / cand.svpvDisErr > bin->svpvSigMin;
    }
    case 10:
      return cand.chi2cl > 0.10f;
    case 11: {
      const auto* bin = findTopologyBin(cand);
      return bin && cand.dtheta < bin->dthetaMax;
    }
    default:
      return false;
  }
}

bool passFinalNoMassWindow(const Candidate& cand) {
  if (!finite(cand.mass)) return false;
  if (!(cand.trk1HighPurity && cand.trk2HighPurity)) return false;
  if (std::abs(cand.trk1Eta) >= 2.4f || std::abs(cand.trk2Eta) >= 2.4f) return false;
  if (cand.trk1Pt <= 1.0f || cand.trk2Pt <= 1.0f) return false;
  if (!(cand.trk1Pt > 0.0f && cand.trk2Pt > 0.0f)) return false;
  if (cand.trk1PtErr / cand.trk1Pt >= 0.1f || cand.trk2PtErr / cand.trk2Pt >= 0.1f) return false;
  if (!(cand.pt > 2.0f && cand.pt <= 12.0f && std::abs(cand.y) <= 2.0f)) return false;
  const auto* bin = findTopologyBin(cand);
  if (!bin) return false;
  if (cand.alpha >= bin->alphaMax) return false;
  if (!(cand.svpvDisErr > 0.0f && cand.svpvDistance / cand.svpvDisErr > bin->svpvSigMin)) return false;
  if (cand.chi2cl <= 0.10f) return false;
  if (cand.dtheta >= bin->dthetaMax) return false;
  return true;
}

bool requireBranch(TTree* tree, const char* name, const char* label) {
  if (!tree || !tree->GetBranch(name)) {
    std::cerr << "ERROR: missing " << name << " in " << label << "\n";
    return false;
  }
  return true;
}

bool bindReader(DReader& reader, TTree* tree, const char* label) {
  reader.tree = tree;
  const char* required[] = {
      "RunNo", "LumiNo", "EvtNo", "Dsize", "Dmass", "Dpt", "Dy",
      "Dtrk1Pt", "Dtrk2Pt", "Dtrk1Eta", "Dtrk2Eta",
      "Dtrk1PtErr", "Dtrk2PtErr", "Dalpha", "DsvpvDistance",
      "DsvpvDisErr", "Dchi2cl", "Ddtheta", "Dtrk1highPurity",
      "Dtrk2highPurity"};
  for (const char* branch : required) {
    if (!requireBranch(tree, branch, label)) return false;
  }

  tree->SetBranchStatus("*", 0);
  for (const char* branch : required) tree->SetBranchStatus(branch, 1);
  tree->SetBranchAddress("RunNo", &reader.run);
  tree->SetBranchAddress("LumiNo", &reader.lumi);
  tree->SetBranchAddress("EvtNo", &reader.event);
  tree->SetBranchAddress("Dsize", &reader.dsize);
  tree->SetBranchAddress("Dmass", reader.mass);
  tree->SetBranchAddress("Dpt", reader.pt);
  tree->SetBranchAddress("Dy", reader.y);
  tree->SetBranchAddress("Dtrk1Pt", reader.trk1Pt);
  tree->SetBranchAddress("Dtrk2Pt", reader.trk2Pt);
  tree->SetBranchAddress("Dtrk1Eta", reader.trk1Eta);
  tree->SetBranchAddress("Dtrk2Eta", reader.trk2Eta);
  tree->SetBranchAddress("Dtrk1PtErr", reader.trk1PtErr);
  tree->SetBranchAddress("Dtrk2PtErr", reader.trk2PtErr);
  tree->SetBranchAddress("Dalpha", reader.alpha);
  tree->SetBranchAddress("DsvpvDistance", reader.svpvDistance);
  tree->SetBranchAddress("DsvpvDisErr", reader.svpvDisErr);
  tree->SetBranchAddress("Dchi2cl", reader.chi2cl);
  tree->SetBranchAddress("Ddtheta", reader.dtheta);
  tree->SetBranchAddress("Dtrk1highPurity", reader.trk1HighPurity);
  tree->SetBranchAddress("Dtrk2highPurity", reader.trk2HighPurity);
  return true;
}

EventKey currentKey(const DReader& reader) {
  return {reader.run, reader.lumi, reader.event};
}

Candidate candidateAt(const DReader& reader, int index) {
  return {reader.mass[index],
          reader.pt[index],
          reader.y[index],
          reader.trk1Pt[index],
          reader.trk2Pt[index],
          reader.trk1Eta[index],
          reader.trk2Eta[index],
          reader.trk1PtErr[index],
          reader.trk2PtErr[index],
          reader.alpha[index],
          reader.svpvDistance[index],
          reader.svpvDisErr[index],
          reader.chi2cl[index],
          reader.dtheta[index],
          reader.trk1HighPurity[index],
          reader.trk2HighPurity[index]};
}

std::string fp(const Candidate& cand) {
  auto roundInt = [](float value, double scale) {
    if (!finite(value)) return static_cast<long long>(-999999999);
    return static_cast<long long>(std::llround(value * scale));
  };
  std::ostringstream out;
  out << roundInt(cand.mass, 100000.0) << ':'
      << roundInt(cand.pt, 10000.0) << ':'
      << roundInt(cand.y, 10000.0) << ':'
      << roundInt(cand.trk1Pt, 10000.0) << ':'
      << roundInt(cand.trk2Pt, 10000.0) << ':'
      << roundInt(cand.alpha, 10000.0) << ':'
      << roundInt(cand.svpvDistance, 10000.0) << ':'
      << roundInt(cand.dtheta, 10000.0);
  return out.str();
}

std::string compareSameIndex(const Candidate& a, const Candidate& b) {
  std::vector<std::string> diffs;
  auto check = [&](const char* name, float left, float right, float tol = 1e-5f) {
    if ((!finite(left) && finite(right)) || (finite(left) && !finite(right)) ||
        (finite(left) && finite(right) && std::abs(left - right) > tol)) {
      std::ostringstream out;
      out << name << " recreated=" << std::setprecision(8) << left << " official=" << right;
      diffs.push_back(out.str());
    }
  };
  check("Dmass", a.mass, b.mass);
  check("Dpt", a.pt, b.pt);
  check("Dy", a.y, b.y);
  check("Dtrk1Pt", a.trk1Pt, b.trk1Pt);
  check("Dtrk2Pt", a.trk2Pt, b.trk2Pt);
  check("Dtrk1Eta", a.trk1Eta, b.trk1Eta);
  check("Dtrk2Eta", a.trk2Eta, b.trk2Eta);
  check("Dalpha", a.alpha, b.alpha, 1e-4f);
  check("DsvpvDistance", a.svpvDistance, b.svpvDistance, 1e-4f);
  check("DsvpvDisErr", a.svpvDisErr, b.svpvDisErr, 1e-4f);
  check("Dchi2cl", a.chi2cl, b.chi2cl, 1e-5f);
  check("Ddtheta", a.dtheta, b.dtheta, 1e-4f);
  if (a.trk1HighPurity != b.trk1HighPurity) diffs.push_back("Dtrk1highPurity differs");
  if (a.trk2HighPurity != b.trk2HighPurity) diffs.push_back("Dtrk2highPurity differs");

  std::ostringstream joined;
  for (std::size_t i = 0; i < diffs.size(); ++i) {
    if (i) joined << "; ";
    joined << diffs[i];
  }
  return joined.str();
}

TH1D* makeMassHist(const TString& name, const TString& title, int color) {
  auto* hist = new TH1D(name, title, 160, 1.5, 2.3);
  hist->SetDirectory(nullptr);
  hist->Sumw2();
  hist->SetLineColor(color);
  hist->SetMarkerColor(color);
  hist->SetLineWidth(2);
  hist->GetXaxis()->SetTitle("m(K#pi) [GeV]");
  hist->GetYaxis()->SetTitle("Candidates");
  return hist;
}

void drawOverlay(TH1D* recreated, TH1D* official, const TString& outBase, const TString& title) {
  TCanvas canvas("canvas", "canvas", 900, 700);
  canvas.SetLeftMargin(0.12);
  canvas.SetBottomMargin(0.12);
  recreated->SetTitle(title);
  const double ymax = std::max(recreated->GetMaximum(), official->GetMaximum()) * 1.25 + 1.0;
  recreated->SetMaximum(ymax);
  recreated->Draw("hist");
  official->Draw("hist same");
  TLegend legend(0.56, 0.74, 0.88, 0.88);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.AddEntry(recreated, "recreated MINIAOD forest", "l");
  legend.AddEntry(official, "official Dfinder forest", "l");
  legend.Draw();
  TLine line(1.86484, 0, 1.86484, ymax * 0.85);
  line.SetLineStyle(2);
  line.Draw();
  canvas.SaveAs(outBase + ".png");
  canvas.SaveAs(outBase + ".pdf");
}

}  // namespace

int compare_two_d0_forests_same_events(
    const char* recreatedForest,
    const char* officialForest,
    const char* outDir,
    const char* treeName = "Dfinder/ntDkpi",
    int maxMismatchRows = 200) {
  if (!isOwnedOutput(outDir)) {
    std::cerr << "ERROR: refusing to write outside Evan-owned CERN paths: " << outDir << "\n";
    return 2;
  }
  gSystem->mkdir(outDir, true);
  gStyle->SetOptStat(0);

  TFile* recreatedFile = TFile::Open(recreatedForest, "READ");
  TFile* officialFile = TFile::Open(officialForest, "READ");
  if (!recreatedFile || recreatedFile->IsZombie()) {
    std::cerr << "ERROR: could not open recreated forest " << recreatedForest << "\n";
    return 3;
  }
  if (!officialFile || officialFile->IsZombie()) {
    std::cerr << "ERROR: could not open official forest " << officialForest << "\n";
    return 4;
  }

  TTree* recreatedTree = dynamic_cast<TTree*>(recreatedFile->Get(treeName));
  TTree* officialTree = dynamic_cast<TTree*>(officialFile->Get(treeName));
  if (!recreatedTree || !officialTree) {
    std::cerr << "ERROR: missing " << treeName << " in one input\n";
    return 5;
  }

  auto recreatedPtr = std::make_unique<DReader>();
  auto officialPtr = std::make_unique<DReader>();
  DReader& recreated = *recreatedPtr;
  DReader& official = *officialPtr;
  if (!bindReader(recreated, recreatedTree, "recreated")) return 6;
  if (!bindReader(official, officialTree, "official")) return 7;

  std::unordered_map<EventKey, Long64_t, EventKeyHash> officialEntries;
  const Long64_t officialN = officialTree->GetEntries();
  for (Long64_t i = 0; i < officialN; ++i) {
    officialTree->GetEntry(i);
    officialEntries[currentKey(official)] = i;
  }

  std::vector<TH1D*> recreatedMass;
  std::vector<TH1D*> officialMass;
  for (int stage = 0; stage < kNStages; ++stage) {
    recreatedMass.push_back(makeMassHist(Form("recreated_%s", stages()[stage].id),
                                         stages()[stage].label, kBlue + 1));
    officialMass.push_back(makeMassHist(Form("official_%s", stages()[stage].id),
                                       stages()[stage].label, kRed + 1));
  }
  TH1D* recreatedPeak = makeMassHist("recreated_final_no_mass_window",
                                     "Final selection without mass window", kBlue + 1);
  TH1D* officialPeak = makeMassHist("official_final_no_mass_window",
                                    "Final selection without mass window", kRed + 1);

  long long recreatedStageCounts[kNStages] = {0};
  long long officialStageCounts[kNStages] = {0};
  long long matchedEvents = 0;
  long long missingOfficialEvents = 0;
  long long dsizeMismatches = 0;
  long long sameIndexCandidateDiffs = 0;
  long long candidateMultisetMismatches = 0;
  long long totalRecreatedCandidates = 0;
  long long totalOfficialCandidates = 0;

  std::ofstream eventCsv(TString(outDir) + "/event_overlap.csv");
  eventCsv << "run,lumi,event,recreated_entry,official_entry,recreated_dsize,official_dsize,status\n";

  std::ofstream mismatchCsv(TString(outDir) + "/candidate_mismatches.csv");
  mismatchCsv << "run,lumi,event,candidate_index,kind,details\n";

  const Long64_t recreatedN = recreatedTree->GetEntries();
  for (Long64_t i = 0; i < recreatedN; ++i) {
    recreatedTree->GetEntry(i);
    const EventKey key = currentKey(recreated);
    auto found = officialEntries.find(key);
    if (found == officialEntries.end()) {
      ++missingOfficialEvents;
      eventCsv << key.run << "," << key.lumi << "," << key.event << "," << i
               << ",-1," << recreated.dsize << ",-1,missing_official\n";
      continue;
    }

    ++matchedEvents;
    officialTree->GetEntry(found->second);
    eventCsv << key.run << "," << key.lumi << "," << key.event << "," << i
             << "," << found->second << "," << recreated.dsize << "," << official.dsize
             << ",matched\n";

    const int nRecreated = std::min(recreated.dsize, kMaxD);
    const int nOfficial = std::min(official.dsize, kMaxD);
    totalRecreatedCandidates += nRecreated;
    totalOfficialCandidates += nOfficial;
    if (recreated.dsize != official.dsize) {
      ++dsizeMismatches;
      if (maxMismatchRows > 0) {
        mismatchCsv << key.run << "," << key.lumi << "," << key.event
                    << ",-1,Dsize,recreated=" << recreated.dsize
                    << " official=" << official.dsize << "\n";
        --maxMismatchRows;
      }
    }

    std::vector<std::string> recreatedFp;
    std::vector<std::string> officialFp;
    recreatedFp.reserve(nRecreated);
    officialFp.reserve(nOfficial);

    for (int j = 0; j < nRecreated; ++j) {
      const Candidate cand = candidateAt(recreated, j);
      recreatedFp.push_back(fp(cand));
      for (int stage = 0; stage < kNStages; ++stage) {
        if (passStage(cand, stage)) {
          ++recreatedStageCounts[stage];
          recreatedMass[stage]->Fill(cand.mass);
        }
      }
      if (passFinalNoMassWindow(cand)) recreatedPeak->Fill(cand.mass);
    }

    for (int j = 0; j < nOfficial; ++j) {
      const Candidate cand = candidateAt(official, j);
      officialFp.push_back(fp(cand));
      for (int stage = 0; stage < kNStages; ++stage) {
        if (passStage(cand, stage)) {
          ++officialStageCounts[stage];
          officialMass[stage]->Fill(cand.mass);
        }
      }
      if (passFinalNoMassWindow(cand)) officialPeak->Fill(cand.mass);
    }

    const int common = std::min(nRecreated, nOfficial);
    for (int j = 0; j < common; ++j) {
      const std::string diff = compareSameIndex(candidateAt(recreated, j), candidateAt(official, j));
      if (!diff.empty()) {
        ++sameIndexCandidateDiffs;
        if (maxMismatchRows > 0) {
          mismatchCsv << key.run << "," << key.lumi << "," << key.event << "," << j
                      << ",same_index_values,\"" << diff << "\"\n";
          --maxMismatchRows;
        }
      }
    }

    std::sort(recreatedFp.begin(), recreatedFp.end());
    std::sort(officialFp.begin(), officialFp.end());
    if (recreatedFp != officialFp) {
      ++candidateMultisetMismatches;
      if (maxMismatchRows > 0) {
        mismatchCsv << key.run << "," << key.lumi << "," << key.event
                    << ",-1,candidate_multiset,rounded candidate fingerprint sets differ\n";
        --maxMismatchRows;
      }
    }
  }

  std::ofstream stageCsv(TString(outDir) + "/stage_counts.csv");
  stageCsv << "stage,label,recreated_candidates,official_candidates,delta\n";
  for (int stage = 0; stage < kNStages; ++stage) {
    stageCsv << stages()[stage].id << ",\"" << stages()[stage].label << "\","
             << recreatedStageCounts[stage] << "," << officialStageCounts[stage]
             << "," << (recreatedStageCounts[stage] - officialStageCounts[stage]) << "\n";
  }
  stageCsv << "final_no_mass_window,\"final cuts except mass window\","
           << recreatedPeak->GetEntries() << "," << officialPeak->GetEntries()
           << "," << (recreatedPeak->GetEntries() - officialPeak->GetEntries()) << "\n";

  TFile histFile(TString(outDir) + "/forest_comparison_hists.root", "RECREATE");
  for (int stage = 0; stage < kNStages; ++stage) {
    recreatedMass[stage]->Write();
    officialMass[stage]->Write();
    drawOverlay(recreatedMass[stage], officialMass[stage],
                TString(outDir) + "/" + stages()[stage].id + "_mass_overlay",
                TString(stages()[stage].id) + ": " + stages()[stage].label);
  }
  recreatedPeak->Write();
  officialPeak->Write();
  drawOverlay(recreatedPeak, officialPeak,
              TString(outDir) + "/final_no_mass_window_mass_peak_overlay",
              "Final D0 mass peak selection without mass window");

  TH1D cutflow("cutflow", "D0 staged cutflow;Stage;Candidates", kNStages, 0, kNStages);
  TH1D cutflowOfficial("cutflow_official", "D0 staged cutflow;Stage;Candidates", kNStages, 0, kNStages);
  for (int stage = 0; stage < kNStages; ++stage) {
    cutflow.SetBinContent(stage + 1, recreatedStageCounts[stage]);
    cutflowOfficial.SetBinContent(stage + 1, officialStageCounts[stage]);
    cutflow.GetXaxis()->SetBinLabel(stage + 1, stages()[stage].id);
    cutflowOfficial.GetXaxis()->SetBinLabel(stage + 1, stages()[stage].id);
  }
  cutflow.SetLineColor(kBlue + 1);
  cutflowOfficial.SetLineColor(kRed + 1);
  cutflow.SetLineWidth(2);
  cutflowOfficial.SetLineWidth(2);
  cutflow.Write();
  cutflowOfficial.Write();
  histFile.Close();

  TCanvas cutCanvas("cutCanvas", "cutCanvas", 1000, 650);
  cutCanvas.SetLeftMargin(0.12);
  cutCanvas.SetBottomMargin(0.22);
  cutCanvas.SetLogy();
  cutflow.SetMaximum(std::max(cutflow.GetMaximum(), cutflowOfficial.GetMaximum()) * 2.0 + 1.0);
  cutflow.SetMinimum(0.5);
  cutflow.Draw("hist");
  cutflowOfficial.Draw("hist same");
  TLegend cutLegend(0.56, 0.76, 0.88, 0.89);
  cutLegend.SetBorderSize(0);
  cutLegend.SetFillStyle(0);
  cutLegend.AddEntry(&cutflow, "recreated MINIAOD forest", "l");
  cutLegend.AddEntry(&cutflowOfficial, "official Dfinder forest", "l");
  cutLegend.Draw();
  cutCanvas.SaveAs(TString(outDir) + "/cutflow_overlay.png");
  cutCanvas.SaveAs(TString(outDir) + "/cutflow_overlay.pdf");

  std::ofstream manifest(TString(outDir) + "/manifest.md");
  manifest << "# Recreated MINIAOD Forest vs Official Dfinder Forest\n\n";
  manifest << "<!-- DOC_OWNER: cms-analysis-manager validation output. -->\n";
  manifest << "<!-- TOKEN_NOTE: inspect CSVs/plots before rerunning ROOT scans. -->\n\n";
  manifest << "## Inputs\n\n";
  manifest << "- Recreated forest: `" << recreatedForest << "`\n";
  manifest << "- Official forest: `" << officialForest << "`\n";
  manifest << "- Tree: `" << treeName << "`\n\n";
  manifest << "## Summary\n\n";
  manifest << "- Recreated events: `" << recreatedN << "`\n";
  manifest << "- Official events in shard: `" << officialN << "`\n";
  manifest << "- Matched events: `" << matchedEvents << "`\n";
  manifest << "- Recreated events missing from official shard: `" << missingOfficialEvents << "`\n";
  manifest << "- Recreated candidates on matched events: `" << totalRecreatedCandidates << "`\n";
  manifest << "- Official candidates on matched events: `" << totalOfficialCandidates << "`\n";
  manifest << "- Events with Dsize mismatch: `" << dsizeMismatches << "`\n";
  manifest << "- Same-index candidate value differences: `" << sameIndexCandidateDiffs << "`\n";
  manifest << "- Events with rounded candidate-set mismatch: `" << candidateMultisetMismatches << "`\n\n";
  manifest << "## Outputs\n\n";
  manifest << "- `event_overlap.csv`\n";
  manifest << "- `stage_counts.csv`\n";
  manifest << "- `candidate_mismatches.csv`\n";
  manifest << "- `cutflow_overlay.png/.pdf`\n";
  manifest << "- `final_no_mass_window_mass_peak_overlay.png/.pdf`\n";
  manifest << "- `stageXX_mass_overlay.png/.pdf` for each staged cut\n";
  manifest << "- `forest_comparison_hists.root`\n";

  std::cout << "matched_events=" << matchedEvents << "\n";
  std::cout << "missing_official_events=" << missingOfficialEvents << "\n";
  std::cout << "dsize_mismatches=" << dsizeMismatches << "\n";
  std::cout << "same_index_candidate_diffs=" << sameIndexCandidateDiffs << "\n";
  std::cout << "candidate_multiset_mismatches=" << candidateMultisetMismatches << "\n";
  std::cout << "outDir=" << outDir << "\n";

  recreatedFile->Close();
  officialFile->Close();
  return (missingOfficialEvents == 0 && dsizeMismatches == 0 &&
          sameIndexCandidateDiffs == 0 && candidateMultisetMismatches == 0)
             ? 0
             : 1;
}
