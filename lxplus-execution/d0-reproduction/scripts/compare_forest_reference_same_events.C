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
#include <cstdint>
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
    seed ^= std::hash<int>{}(key.lumi) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    seed ^= std::hash<long long>{}(key.event) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
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
  bool hasHighPurity = false;
  bool trk1HighPurity = true;
  bool trk2HighPurity = true;
  bool hasReferencePass = false;
  bool referencePass = false;
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

bool hasBranch(TTree* tree, const char* name) {
  return tree && tree->GetBranch(name);
}

std::string keyString(const EventKey& key) {
  std::ostringstream out;
  out << key.run << ':' << key.lumi << ':' << key.event;
  return out.str();
}

bool finite(float value) {
  return std::isfinite(static_cast<double>(value));
}

bool inTopologyBin(const Candidate& cand, const TopologyBin& bin) {
  const double ay = std::abs(cand.y);
  return ay >= bin.yLow && ay < bin.yHigh && cand.pt >= bin.ptLow && cand.pt < bin.ptHigh;
}

const TopologyBin* findTopologyBin(const Candidate& cand) {
  for (const auto& bin : topologyBins()) {
    if (inTopologyBin(cand, bin)) return &bin;
  }
  return nullptr;
}

bool passStage(const Candidate& cand, int stage, bool requireHighPurity) {
  if (stage <= 0) return finite(cand.mass);
  if (!passStage(cand, stage - 1, requireHighPurity)) return false;

  switch (stage) {
    case 1:
      return std::abs(cand.mass - 1.86484f) < 0.2f;
    case 2:
      return !requireHighPurity || (cand.hasHighPurity && cand.trk1HighPurity && cand.trk2HighPurity);
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

bool passFinalNoMassWindow(const Candidate& cand, bool requireHighPurity) {
  if (!finite(cand.mass)) return false;
  if (requireHighPurity && !(cand.hasHighPurity && cand.trk1HighPurity && cand.trk2HighPurity)) return false;
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

bool passDptDy(const Candidate& cand) {
  return finite(cand.mass) && cand.pt > 2.0f && cand.pt <= 12.0f && std::abs(cand.y) <= 2.0f;
}

bool closeFloat(float left, float right, float tolerance = 1e-5f) {
  if (!finite(left) && !finite(right)) return true;
  return std::abs(left - right) <= tolerance;
}

std::string compareSameIndex(const Candidate& forest, const Candidate& reference) {
  std::vector<std::string> diffs;
  auto check = [&](const char* name, float a, float b, float tolerance = 1e-5f) {
    if (!closeFloat(a, b, tolerance)) {
      std::ostringstream out;
      out << name << ": forest=" << std::setprecision(8) << a << " reference=" << b;
      diffs.push_back(out.str());
    }
  };
  check("Dmass", forest.mass, reference.mass);
  check("Dpt", forest.pt, reference.pt);
  check("Dy", forest.y, reference.y);
  check("Dtrk1Pt", forest.trk1Pt, reference.trk1Pt);
  check("Dtrk2Pt", forest.trk2Pt, reference.trk2Pt);
  check("Dtrk1Eta", forest.trk1Eta, reference.trk1Eta);
  check("Dtrk2Eta", forest.trk2Eta, reference.trk2Eta);
  check("Dtrk1PtErr", forest.trk1PtErr, reference.trk1PtErr);
  check("Dtrk2PtErr", forest.trk2PtErr, reference.trk2PtErr);
  check("Dalpha", forest.alpha, reference.alpha, 1e-4f);
  check("DsvpvDistance", forest.svpvDistance, reference.svpvDistance, 1e-4f);
  check("DsvpvDisErr", forest.svpvDisErr, reference.svpvDisErr, 1e-4f);
  check("Dchi2cl", forest.chi2cl, reference.chi2cl, 1e-5f);
  check("Ddtheta", forest.dtheta, reference.dtheta, 1e-4f);
  if (diffs.empty()) return "";
  std::ostringstream joined;
  for (std::size_t i = 0; i < diffs.size(); ++i) {
    if (i) joined << "; ";
    joined << diffs[i];
  }
  return joined.str();
}

void requireBranch(TTree* tree, const char* name, const char* treeName) {
  if (!hasBranch(tree, name)) {
    std::cerr << "ERROR: missing " << name << " in " << treeName << "\n";
    throw std::runtime_error(name);
  }
}

TH1D* makeMassHist(const char* name, const char* title, int color) {
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

void writeHistCsv(const TH1D* hist, const TString& path) {
  std::ofstream out(path.Data());
  out << "bin,low_edge,high_edge,center,count,error\n";
  for (int i = 1; i <= hist->GetNbinsX(); ++i) {
    out << i << ',' << std::setprecision(8)
        << hist->GetXaxis()->GetBinLowEdge(i) << ','
        << hist->GetXaxis()->GetBinUpEdge(i) << ','
        << hist->GetXaxis()->GetBinCenter(i) << ','
        << hist->GetBinContent(i) << ','
        << hist->GetBinError(i) << '\n';
  }
}

TH1D* normalizedClone(const TH1D* hist, const char* name, int color) {
  auto* clone = static_cast<TH1D*>(hist->Clone(name));
  clone->SetDirectory(nullptr);
  clone->SetLineColor(color);
  clone->SetLineWidth(2);
  if (clone->Integral() > 0) clone->Scale(1.0 / clone->Integral());
  clone->GetYaxis()->SetTitle("Normalized candidates");
  return clone;
}

void drawOverlay(const TH1D* forest, const TH1D* reference, const TString& outStem, bool normalized) {
  gStyle->SetOptStat(0);
  TCanvas canvas("canvas", "", 950, 700);
  canvas.SetLeftMargin(0.12);
  canvas.SetRightMargin(0.04);
  canvas.SetTopMargin(0.08);
  canvas.SetBottomMargin(0.12);

  TH1D* f = const_cast<TH1D*>(forest);
  TH1D* r = const_cast<TH1D*>(reference);
  if (normalized) {
    f = normalizedClone(forest, "forest_selected_norm", kBlue + 2);
    r = normalizedClone(reference, "reference_selected_norm", kRed + 1);
  }

  const double maxY = std::max(f->GetMaximum(), r->GetMaximum());
  f->SetMaximum(maxY > 0 ? maxY * 1.25 : 1.0);
  f->Draw("hist");
  r->Draw("hist same");

  TLine pdg(1.86484, 0.0, 1.86484, maxY > 0 ? maxY * 1.15 : 1.0);
  pdg.SetLineColor(kGray + 2);
  pdg.SetLineStyle(2);
  pdg.SetLineWidth(2);
  pdg.Draw("same");

  TLegend legend(0.42, 0.68, 0.92, 0.88);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.AddEntry(f, "Official forest Dfinder tree", "l");
  legend.AddEntry(r, "Reference reduced skim", "l");
  legend.AddEntry(&pdg, "PDG D^{0} mass", "l");
  legend.Draw();

  canvas.SaveAs(outStem + (normalized ? "_normalized.png" : ".png"));
  canvas.SaveAs(outStem + (normalized ? "_normalized.pdf" : ".pdf"));
}

Candidate forestCandidate(int i,
                          const std::vector<float>& mass,
                          const std::vector<float>& pt,
                          const std::vector<float>& y,
                          const std::vector<float>& trk1Pt,
                          const std::vector<float>& trk2Pt,
                          const std::vector<float>& trk1Eta,
                          const std::vector<float>& trk2Eta,
                          const std::vector<float>& trk1PtErr,
                          const std::vector<float>& trk2PtErr,
                          const std::vector<float>& alpha,
                          const std::vector<float>& svpvDistance,
                          const std::vector<float>& svpvDisErr,
                          const std::vector<float>& chi2cl,
                          const std::vector<float>& dtheta,
                          const Bool_t* trk1HighPurity,
                          const Bool_t* trk2HighPurity) {
  Candidate cand;
  cand.mass = mass[i];
  cand.pt = pt[i];
  cand.y = y[i];
  cand.trk1Pt = trk1Pt[i];
  cand.trk2Pt = trk2Pt[i];
  cand.trk1Eta = trk1Eta[i];
  cand.trk2Eta = trk2Eta[i];
  cand.trk1PtErr = trk1PtErr[i];
  cand.trk2PtErr = trk2PtErr[i];
  cand.alpha = alpha[i];
  cand.svpvDistance = svpvDistance[i];
  cand.svpvDisErr = svpvDisErr[i];
  cand.chi2cl = chi2cl[i];
  cand.dtheta = dtheta[i];
  cand.hasHighPurity = true;
  cand.trk1HighPurity = trk1HighPurity[i];
  cand.trk2HighPurity = trk2HighPurity[i];
  return cand;
}

template <typename T>
float vecAt(const std::vector<T>* values, std::size_t i) {
  return values ? static_cast<float>(values->at(i)) : std::numeric_limits<float>::quiet_NaN();
}

Candidate referenceCandidate(std::size_t i,
                             const std::vector<float>* mass,
                             const std::vector<float>* pt,
                             const std::vector<float>* y,
                             const std::vector<float>* trk1Pt,
                             const std::vector<float>* trk2Pt,
                             const std::vector<float>* trk1Eta,
                             const std::vector<float>* trk2Eta,
                             const std::vector<float>* trk1PtErr,
                             const std::vector<float>* trk2PtErr,
                             const std::vector<float>* alpha,
                             const std::vector<float>* svpvDistance,
                             const std::vector<float>* svpvDisErr,
                             const std::vector<float>* chi2cl,
                             const std::vector<float>* dtheta,
                             const std::vector<bool>* pass) {
  Candidate cand;
  cand.mass = vecAt(mass, i);
  cand.pt = vecAt(pt, i);
  cand.y = vecAt(y, i);
  cand.trk1Pt = vecAt(trk1Pt, i);
  cand.trk2Pt = vecAt(trk2Pt, i);
  cand.trk1Eta = vecAt(trk1Eta, i);
  cand.trk2Eta = vecAt(trk2Eta, i);
  cand.trk1PtErr = vecAt(trk1PtErr, i);
  cand.trk2PtErr = vecAt(trk2PtErr, i);
  cand.alpha = vecAt(alpha, i);
  cand.svpvDistance = vecAt(svpvDistance, i);
  cand.svpvDisErr = vecAt(svpvDisErr, i);
  cand.chi2cl = vecAt(chi2cl, i);
  cand.dtheta = vecAt(dtheta, i);
  cand.hasHighPurity = false;
  cand.hasReferencePass = pass != nullptr;
  cand.referencePass = pass && pass->at(i);
  return cand;
}

}  // namespace

int compare_forest_reference_same_events(const char* forestPath,
                                         const char* referencePath,
                                         const char* outputDir,
                                         Long64_t maxReferenceEntries = 0,
                                         Long64_t maxMismatchRows = 100) {
  TString outDir(outputDir);
  if (!isOwnedOutput(outDir)) {
    std::cerr << "ERROR: refusing to write outside Evan-owned CERN paths: " << outDir << "\n";
    return 2;
  }
  gSystem->mkdir(outDir, true);

  std::unique_ptr<TFile> forestFile(TFile::Open(forestPath, "READ"));
  if (!forestFile || forestFile->IsZombie()) {
    std::cerr << "ERROR: could not open forest file: " << forestPath << "\n";
    return 2;
  }
  auto* forestTree = dynamic_cast<TTree*>(forestFile->Get("Dfinder/ntDkpi"));
  if (!forestTree) {
    std::cerr << "ERROR: missing Dfinder/ntDkpi in forest file\n";
    return 2;
  }

  std::unique_ptr<TFile> referenceFile(TFile::Open(referencePath, "READ"));
  if (!referenceFile || referenceFile->IsZombie()) {
    std::cerr << "ERROR: could not open reference file: " << referencePath << "\n";
    return 2;
  }
  auto* referenceTree = dynamic_cast<TTree*>(referenceFile->Get("Tree"));
  if (!referenceTree) {
    std::cerr << "ERROR: missing Tree in reference file\n";
    return 2;
  }

  try {
    for (const char* branch : {"RunNo", "LumiNo", "EvtNo", "Dsize", "Dmass", "Dpt", "Dy",
                               "Dtrk1Pt", "Dtrk2Pt", "Dtrk1Eta", "Dtrk2Eta",
                               "Dtrk1PtErr", "Dtrk2PtErr", "Dalpha", "DsvpvDistance",
                               "DsvpvDisErr", "Dchi2cl", "Ddtheta",
                               "Dtrk1highPurity", "Dtrk2highPurity"}) {
      requireBranch(forestTree, branch, "Dfinder/ntDkpi");
    }
    for (const char* branch : {"Run", "Lumi", "Event", "Dsize", "Dmass", "Dpt", "Dy",
                               "Dtrk1Pt", "Dtrk2Pt", "Dtrk1Eta", "Dtrk2Eta",
                               "Dtrk1PtErr", "Dtrk2PtErr", "Dalpha", "DsvpvDistance",
                               "DsvpvDisErr", "Dchi2cl", "Ddtheta", "DpassCut23PAS"}) {
      requireBranch(referenceTree, branch, "reference Tree");
    }
  } catch (...) {
    return 2;
  }

  forestTree->SetBranchStatus("*", 0);
  for (const char* branch : {"RunNo", "LumiNo", "EvtNo", "Dsize", "Dmass", "Dpt", "Dy",
                             "Dtrk1Pt", "Dtrk2Pt", "Dtrk1Eta", "Dtrk2Eta",
                             "Dtrk1PtErr", "Dtrk2PtErr", "Dalpha", "DsvpvDistance",
                             "DsvpvDisErr", "Dchi2cl", "Ddtheta",
                             "Dtrk1highPurity", "Dtrk2highPurity"}) {
    forestTree->SetBranchStatus(branch, 1);
  }

  referenceTree->SetBranchStatus("*", 0);
  for (const char* branch : {"Run", "Lumi", "Event", "Dsize", "Dmass", "Dpt", "Dy",
                             "Dtrk1Pt", "Dtrk2Pt", "Dtrk1Eta", "Dtrk2Eta",
                             "Dtrk1PtErr", "Dtrk2PtErr", "Dalpha", "DsvpvDistance",
                             "DsvpvDisErr", "Dchi2cl", "Ddtheta", "DpassCut23PAS"}) {
    referenceTree->SetBranchStatus(branch, 1);
  }

  constexpr int kMaxD = 200000;
  int fRun = 0, fLumi = 0, fEvent = 0, fDsize = 0;
  std::vector<float> fDmass(kMaxD), fDpt(kMaxD), fDy(kMaxD);
  std::vector<float> fDtrk1Pt(kMaxD), fDtrk2Pt(kMaxD), fDtrk1Eta(kMaxD), fDtrk2Eta(kMaxD);
  std::vector<float> fDtrk1PtErr(kMaxD), fDtrk2PtErr(kMaxD), fDalpha(kMaxD);
  std::vector<float> fDsvpvDistance(kMaxD), fDsvpvDisErr(kMaxD), fDchi2cl(kMaxD), fDdtheta(kMaxD);
  auto fDtrk1HighPurity = std::make_unique<Bool_t[]>(kMaxD);
  auto fDtrk2HighPurity = std::make_unique<Bool_t[]>(kMaxD);

  forestTree->SetBranchAddress("RunNo", &fRun);
  forestTree->SetBranchAddress("LumiNo", &fLumi);
  forestTree->SetBranchAddress("EvtNo", &fEvent);
  forestTree->SetBranchAddress("Dsize", &fDsize);
  forestTree->SetBranchAddress("Dmass", fDmass.data());
  forestTree->SetBranchAddress("Dpt", fDpt.data());
  forestTree->SetBranchAddress("Dy", fDy.data());
  forestTree->SetBranchAddress("Dtrk1Pt", fDtrk1Pt.data());
  forestTree->SetBranchAddress("Dtrk2Pt", fDtrk2Pt.data());
  forestTree->SetBranchAddress("Dtrk1Eta", fDtrk1Eta.data());
  forestTree->SetBranchAddress("Dtrk2Eta", fDtrk2Eta.data());
  forestTree->SetBranchAddress("Dtrk1PtErr", fDtrk1PtErr.data());
  forestTree->SetBranchAddress("Dtrk2PtErr", fDtrk2PtErr.data());
  forestTree->SetBranchAddress("Dalpha", fDalpha.data());
  forestTree->SetBranchAddress("DsvpvDistance", fDsvpvDistance.data());
  forestTree->SetBranchAddress("DsvpvDisErr", fDsvpvDisErr.data());
  forestTree->SetBranchAddress("Dchi2cl", fDchi2cl.data());
  forestTree->SetBranchAddress("Ddtheta", fDdtheta.data());
  forestTree->SetBranchAddress("Dtrk1highPurity", fDtrk1HighPurity.get());
  forestTree->SetBranchAddress("Dtrk2highPurity", fDtrk2HighPurity.get());

  int rRun = 0, rLumi = 0, rDsize = 0;
  long long rEvent = 0;
  std::vector<float>* rDmass = nullptr;
  std::vector<float>* rDpt = nullptr;
  std::vector<float>* rDy = nullptr;
  std::vector<float>* rDtrk1Pt = nullptr;
  std::vector<float>* rDtrk2Pt = nullptr;
  std::vector<float>* rDtrk1Eta = nullptr;
  std::vector<float>* rDtrk2Eta = nullptr;
  std::vector<float>* rDtrk1PtErr = nullptr;
  std::vector<float>* rDtrk2PtErr = nullptr;
  std::vector<float>* rDalpha = nullptr;
  std::vector<float>* rDsvpvDistance = nullptr;
  std::vector<float>* rDsvpvDisErr = nullptr;
  std::vector<float>* rDchi2cl = nullptr;
  std::vector<float>* rDdtheta = nullptr;
  std::vector<bool>* rDpassCut23PAS = nullptr;

  referenceTree->SetBranchAddress("Run", &rRun);
  referenceTree->SetBranchAddress("Lumi", &rLumi);
  referenceTree->SetBranchAddress("Event", &rEvent);
  referenceTree->SetBranchAddress("Dsize", &rDsize);
  referenceTree->SetBranchAddress("Dmass", &rDmass);
  referenceTree->SetBranchAddress("Dpt", &rDpt);
  referenceTree->SetBranchAddress("Dy", &rDy);
  referenceTree->SetBranchAddress("Dtrk1Pt", &rDtrk1Pt);
  referenceTree->SetBranchAddress("Dtrk2Pt", &rDtrk2Pt);
  referenceTree->SetBranchAddress("Dtrk1Eta", &rDtrk1Eta);
  referenceTree->SetBranchAddress("Dtrk2Eta", &rDtrk2Eta);
  referenceTree->SetBranchAddress("Dtrk1PtErr", &rDtrk1PtErr);
  referenceTree->SetBranchAddress("Dtrk2PtErr", &rDtrk2PtErr);
  referenceTree->SetBranchAddress("Dalpha", &rDalpha);
  referenceTree->SetBranchAddress("DsvpvDistance", &rDsvpvDistance);
  referenceTree->SetBranchAddress("DsvpvDisErr", &rDsvpvDisErr);
  referenceTree->SetBranchAddress("Dchi2cl", &rDchi2cl);
  referenceTree->SetBranchAddress("Ddtheta", &rDdtheta);
  referenceTree->SetBranchAddress("DpassCut23PAS", &rDpassCut23PAS);

  std::unordered_map<EventKey, Long64_t, EventKeyHash> forestEntries;
  forestEntries.reserve(forestTree->GetEntries() * 2);
  Long64_t forestTotalCandidates = 0;
  int forestMaxDsize = 0;
  TBranch* fBRun = forestTree->GetBranch("RunNo");
  TBranch* fBLumi = forestTree->GetBranch("LumiNo");
  TBranch* fBEvent = forestTree->GetBranch("EvtNo");
  TBranch* fBDsize = forestTree->GetBranch("Dsize");
  for (Long64_t entry = 0; entry < forestTree->GetEntries(); ++entry) {
    fBRun->GetEntry(entry);
    fBLumi->GetEntry(entry);
    fBEvent->GetEntry(entry);
    fBDsize->GetEntry(entry);
    if (fDsize >= kMaxD) {
      std::cerr << "ERROR: Dsize exceeds fixed buffer: " << fDsize << "\n";
      return 2;
    }
    forestEntries[EventKey{fRun, fLumi, static_cast<long long>(fEvent)}] = entry;
    forestTotalCandidates += fDsize;
    forestMaxDsize = std::max(forestMaxDsize, fDsize);
  }

  std::unordered_map<EventKey, Long64_t, EventKeyHash> referenceMatches;
  referenceMatches.reserve(forestEntries.size() * 2);
  const Long64_t referenceEntries = referenceTree->GetEntries();
  const Long64_t referenceLimit = (maxReferenceEntries > 0 && maxReferenceEntries < referenceEntries)
                                      ? maxReferenceEntries
                                      : referenceEntries;
  Long64_t referenceRowsWithD = 0;
  Long64_t referenceTotalCandidatesScanned = 0;
  Long64_t duplicateMatchedReferenceKeys = 0;

  TBranch* bRun = referenceTree->GetBranch("Run");
  TBranch* bLumi = referenceTree->GetBranch("Lumi");
  TBranch* bEvent = referenceTree->GetBranch("Event");
  TBranch* bDsize = referenceTree->GetBranch("Dsize");
  for (Long64_t entry = 0; entry < referenceLimit; ++entry) {
    bRun->GetEntry(entry);
    bLumi->GetEntry(entry);
    bEvent->GetEntry(entry);
    bDsize->GetEntry(entry);
    if (rDsize > 0) ++referenceRowsWithD;
    referenceTotalCandidatesScanned += rDsize;
    EventKey key{rRun, rLumi, rEvent};
    if (forestEntries.find(key) == forestEntries.end()) continue;
    auto inserted = referenceMatches.emplace(key, entry);
    if (!inserted.second) ++duplicateMatchedReferenceKeys;
  }

  std::vector<double> forestStageCounts(stages().size(), 0.0);
  std::vector<double> referenceFormulaStageCounts(stages().size(), 0.0);
  std::vector<double> referenceFlagFinalCounts(stages().size(), 0.0);
  std::vector<double> referenceFlagAndFormulaStageCounts(stages().size(), 0.0);
  std::vector<double> sameIndexMismatchCounts(stages().size(), 0.0);

  auto* hForestSelected = makeMassHist("h_forest_stage11_selected", "Forest selected candidates", kBlue + 2);
  auto* hReferenceFormulaSelected = makeMassHist("h_reference_stage11_formula_selected", "Reference formula-selected candidates", kRed + 1);
  auto* hReferenceFlagSelected = makeMassHist("h_reference_DpassCut23PAS_selected", "Reference DpassCut23PAS candidates", kGreen + 2);
  auto* hForestNoMass = makeMassHist("h_forest_nomass_selected", "Forest no-mass-window selected candidates", kBlue + 2);
  auto* hReferenceNoMass = makeMassHist("h_reference_nomass_formula_selected", "Reference no-mass-window formula candidates", kRed + 1);
  auto* hReferenceDpassDptDy = makeMassHist("h_reference_Dpass_DptDy_selected", "Reference DpassCut23PAS plus D kinematics candidates", kGreen + 2);

  struct MatchedPair {
    EventKey key;
    Long64_t forestEntry = -1;
    Long64_t referenceEntry = -1;
  };
  std::vector<MatchedPair> matchedPairs;
  matchedPairs.reserve(referenceMatches.size());

  std::ofstream mismatch((outDir + "/mismatch_samples.csv").Data());
  mismatch << "kind,run,lumi,event,forest_entry,reference_entry,candidate_index,details\n";
  Long64_t mismatchRows = 0;
  auto writeMismatch = [&](const char* kind, const EventKey& key, Long64_t fEntry, Long64_t rEntry,
                           long long candIndex, const std::string& details) {
    if (mismatchRows >= maxMismatchRows) return;
    mismatch << kind << ',' << key.run << ',' << key.lumi << ',' << key.event << ','
             << fEntry << ',' << rEntry << ',' << candIndex << ",\"";
    for (char ch : details) {
      if (ch == '"') mismatch << "\"\"";
      else mismatch << ch;
    }
    mismatch << "\"\n";
    ++mismatchRows;
  };

  Long64_t matchedEvents = 0;
  Long64_t missingReferenceEvents = 0;
  Long64_t dsizeMatchedEvents = 0;
  Long64_t dsizeMismatchedEvents = 0;
  Long64_t sameIndexComparedCandidates = 0;
  Long64_t sameIndexPerfectCandidates = 0;
  Long64_t referenceFlagMismatches = 0;
  Long64_t forestNoMassSelected = 0;
  Long64_t referenceNoMassFormulaSelected = 0;
  Long64_t referenceDpassDptDySelected = 0;

  for (const auto& item : forestEntries) {
    const EventKey& key = item.first;
    const Long64_t fEntry = item.second;
    auto refIt = referenceMatches.find(key);
    if (refIt == referenceMatches.end()) {
      ++missingReferenceEvents;
      writeMismatch("missing_reference_event", key, fEntry, -1, -1, "forest event not present in scanned reference rows");
      continue;
    }
    matchedPairs.push_back(MatchedPair{key, fEntry, refIt->second});
  }

  std::sort(matchedPairs.begin(), matchedPairs.end(), [](const MatchedPair& left, const MatchedPair& right) {
    return left.referenceEntry < right.referenceEntry;
  });

  for (const auto& match : matchedPairs) {
    ++matchedEvents;
    const EventKey& key = match.key;
    const Long64_t fEntry = match.forestEntry;
    const Long64_t rEntry = match.referenceEntry;
    forestTree->GetEntry(fEntry);
    referenceTree->GetEntry(rEntry);

    if (fDsize == rDsize) {
      ++dsizeMatchedEvents;
    } else {
      ++dsizeMismatchedEvents;
      writeMismatch("dsize_mismatch", key, fEntry, rEntry, -1,
                    TString::Format("forest Dsize=%d reference Dsize=%d", fDsize, rDsize).Data());
    }

    for (int i = 0; i < fDsize; ++i) {
      const Candidate cand = forestCandidate(i, fDmass, fDpt, fDy, fDtrk1Pt, fDtrk2Pt,
                                             fDtrk1Eta, fDtrk2Eta, fDtrk1PtErr, fDtrk2PtErr,
                                             fDalpha, fDsvpvDistance, fDsvpvDisErr,
                                             fDchi2cl, fDdtheta, fDtrk1HighPurity.get(), fDtrk2HighPurity.get());
      for (std::size_t stage = 0; stage < stages().size(); ++stage) {
        if (passStage(cand, static_cast<int>(stage), true)) forestStageCounts[stage] += 1.0;
      }
      if (passStage(cand, 11, true)) hForestSelected->Fill(cand.mass);
      if (passFinalNoMassWindow(cand, true)) {
        ++forestNoMassSelected;
        hForestNoMass->Fill(cand.mass);
      }
    }

    const std::size_t nReference = rDmass ? rDmass->size() : 0;
    for (std::size_t i = 0; i < nReference; ++i) {
      const Candidate cand = referenceCandidate(i, rDmass, rDpt, rDy, rDtrk1Pt, rDtrk2Pt,
                                                rDtrk1Eta, rDtrk2Eta, rDtrk1PtErr, rDtrk2PtErr,
                                                rDalpha, rDsvpvDistance, rDsvpvDisErr,
                                                rDchi2cl, rDdtheta, rDpassCut23PAS);
      for (std::size_t stage = 0; stage < stages().size(); ++stage) {
        if (passStage(cand, static_cast<int>(stage), false)) referenceFormulaStageCounts[stage] += 1.0;
        if (cand.referencePass && passStage(cand, static_cast<int>(stage), false)) {
          referenceFlagAndFormulaStageCounts[stage] += 1.0;
        }
      }
      if (passStage(cand, 11, false)) hReferenceFormulaSelected->Fill(cand.mass);
      if (cand.referencePass) hReferenceFlagSelected->Fill(cand.mass);
      if (cand.referencePass) referenceFlagFinalCounts[11] += 1.0;
      if (passFinalNoMassWindow(cand, false)) {
        ++referenceNoMassFormulaSelected;
        hReferenceNoMass->Fill(cand.mass);
      }
      if (cand.referencePass && passDptDy(cand)) {
        ++referenceDpassDptDySelected;
        hReferenceDpassDptDy->Fill(cand.mass);
      }
      const bool formulaFinal = passStage(cand, 11, false);
      if (cand.referencePass != formulaFinal) {
        ++referenceFlagMismatches;
        writeMismatch("reference_flag_formula_mismatch", key, fEntry, rEntry, static_cast<long long>(i),
                      TString::Format("DpassCut23PAS=%d formula_stage11=%d", cand.referencePass, formulaFinal).Data());
      }
    }

    if (fDsize == rDsize) {
      for (int i = 0; i < fDsize; ++i) {
        const Candidate fCand = forestCandidate(i, fDmass, fDpt, fDy, fDtrk1Pt, fDtrk2Pt,
                                                fDtrk1Eta, fDtrk2Eta, fDtrk1PtErr, fDtrk2PtErr,
                                                fDalpha, fDsvpvDistance, fDsvpvDisErr,
                                                fDchi2cl, fDdtheta, fDtrk1HighPurity.get(), fDtrk2HighPurity.get());
        const Candidate rCand = referenceCandidate(i, rDmass, rDpt, rDy, rDtrk1Pt, rDtrk2Pt,
                                                   rDtrk1Eta, rDtrk2Eta, rDtrk1PtErr, rDtrk2PtErr,
                                                   rDalpha, rDsvpvDistance, rDsvpvDisErr,
                                                   rDchi2cl, rDdtheta, rDpassCut23PAS);
        ++sameIndexComparedCandidates;
        const std::string diff = compareSameIndex(fCand, rCand);
        if (diff.empty()) {
          ++sameIndexPerfectCandidates;
        } else {
          writeMismatch("same_index_candidate_mismatch", key, fEntry, rEntry, i, diff);
          for (std::size_t stage = 0; stage < stages().size(); ++stage) {
            if (passStage(fCand, static_cast<int>(stage), true) != passStage(rCand, static_cast<int>(stage), false)) {
              sameIndexMismatchCounts[stage] += 1.0;
            }
          }
        }
      }
    }
  }
  mismatch.close();

  std::ofstream overlap((outDir + "/event_overlap.csv").Data());
  overlap << "metric,value\n";
  overlap << "forest_entries," << forestTree->GetEntries() << "\n";
  overlap << "forest_unique_events," << forestEntries.size() << "\n";
  overlap << "forest_total_candidates," << forestTotalCandidates << "\n";
  overlap << "forest_max_dsize," << forestMaxDsize << "\n";
  overlap << "reference_entries_total," << referenceEntries << "\n";
  overlap << "reference_entries_scanned," << referenceLimit << "\n";
  overlap << "reference_rows_with_d_gt_0," << referenceRowsWithD << "\n";
  overlap << "reference_total_candidates_scanned," << referenceTotalCandidatesScanned << "\n";
  overlap << "matched_events," << matchedEvents << "\n";
  overlap << "missing_reference_events," << missingReferenceEvents << "\n";
  overlap << "duplicate_matched_reference_keys," << duplicateMatchedReferenceKeys << "\n";
  overlap << "dsize_matched_events," << dsizeMatchedEvents << "\n";
  overlap << "dsize_mismatched_events," << dsizeMismatchedEvents << "\n";
  overlap << "same_index_compared_candidates," << sameIndexComparedCandidates << "\n";
  overlap << "same_index_perfect_candidates," << sameIndexPerfectCandidates << "\n";
  overlap << "reference_flag_formula_mismatches," << referenceFlagMismatches << "\n";
  overlap.close();

  std::ofstream stageOut((outDir + "/stage_counts_same_events.csv").Data());
  stageOut << "stage,label,forest_count,reference_formula_count,reference_flag_and_formula_stage_count,reference_flag_final_count,same_index_stage_mismatch_count\n";
  for (std::size_t i = 0; i < stages().size(); ++i) {
    stageOut << stages()[i].id << ",\"" << stages()[i].label << "\","
             << std::setprecision(12) << forestStageCounts[i] << ','
             << referenceFormulaStageCounts[i] << ','
             << referenceFlagAndFormulaStageCounts[i] << ','
             << referenceFlagFinalCounts[i] << ','
             << sameIndexMismatchCounts[i] << "\n";
  }
  stageOut.close();

  std::ofstream massSelectionOut((outDir + "/mass_spectrum_selection_counts.csv").Data());
  massSelectionOut << "selection,count,notes\n";
  massSelectionOut << "forest_formula_no_mass_window," << forestNoMassSelected
                   << ",all Table-6/Table-7-style cuts except the mass window\n";
  massSelectionOut << "reference_formula_no_mass_window," << referenceNoMassFormulaSelected
                   << ",same as forest formula but reference has no high-purity branch\n";
  massSelectionOut << "reference_DpassCut23PAS_and_DptDy," << referenceDpassDptDySelected
                   << ",stored DpassCut23PAS plus 2<Dpt<=12 and abs(Dy)<=2\n";
  massSelectionOut.close();

  TFile outRoot(outDir + "/same_event_validation.root", "RECREATE");
  hForestSelected->Write();
  hReferenceFormulaSelected->Write();
  hReferenceFlagSelected->Write();
  hForestNoMass->Write();
  hReferenceNoMass->Write();
  hReferenceDpassDptDy->Write();
  outRoot.Close();
  writeHistCsv(hForestSelected, outDir + "/h_forest_stage11_selected.csv");
  writeHistCsv(hReferenceFormulaSelected, outDir + "/h_reference_stage11_formula_selected.csv");
  writeHistCsv(hReferenceFlagSelected, outDir + "/h_reference_DpassCut23PAS_selected.csv");
  writeHistCsv(hForestNoMass, outDir + "/h_forest_nomass_selected.csv");
  writeHistCsv(hReferenceNoMass, outDir + "/h_reference_nomass_formula_selected.csv");
  writeHistCsv(hReferenceDpassDptDy, outDir + "/h_reference_Dpass_DptDy_selected.csv");
  drawOverlay(hForestSelected, hReferenceFlagSelected, outDir + "/same_event_selected_mass_overlay", false);
  drawOverlay(hForestSelected, hReferenceFlagSelected, outDir + "/same_event_selected_mass_overlay", true);
  drawOverlay(hForestNoMass, hReferenceDpassDptDy, outDir + "/same_event_mass_spectrum_selection_overlay", false);
  drawOverlay(hForestNoMass, hReferenceDpassDptDy, outDir + "/same_event_mass_spectrum_selection_overlay", true);

  std::ofstream manifest((outDir + "/README.md").Data());
  manifest << "# Same-Event Forest vs Reference Validation\n\n";
  manifest << "- Official forest input: `" << forestPath << "`\n";
  manifest << "- Reference reduced skim: `" << referencePath << "`\n";
  manifest << "- Forest tree: `Dfinder/ntDkpi`\n";
  manifest << "- Reference tree: `Tree`\n";
  manifest << "- Event key: `(run,lumi,event)` mapped as `RunNo,LumiNo,EvtNo` -> `Run,Lumi,Event`.\n\n";
  manifest << "## Main Counts\n\n";
  manifest << "- Forest entries: `" << forestTree->GetEntries() << "`\n";
  manifest << "- Forest unique events: `" << forestEntries.size() << "`\n";
  manifest << "- Matched events in reference scan: `" << matchedEvents << "`\n";
  manifest << "- Missing forest events in reference scan: `" << missingReferenceEvents << "`\n";
  manifest << "- Dsize-matched events: `" << dsizeMatchedEvents << "`\n";
  manifest << "- Dsize-mismatched events: `" << dsizeMismatchedEvents << "`\n";
  manifest << "- Same-index compared candidates: `" << sameIndexComparedCandidates << "`\n";
  manifest << "- Same-index perfect candidates: `" << sameIndexPerfectCandidates << "`\n";
  manifest << "- Reference `DpassCut23PAS` vs local formula mismatches: `" << referenceFlagMismatches << "`\n\n";
  manifest << "## Outputs\n\n";
  manifest << "- `event_overlap.csv`: compact event-level overlap metrics.\n";
  manifest << "- `stage_counts_same_events.csv`: candidate counts after each analysis-stage cut, restricted to matched events.\n";
  manifest << "- `mass_spectrum_selection_counts.csv`: no-mass-window selection counts for plotting mass spectra.\n";
  manifest << "- `mismatch_samples.csv`: first mismatch rows for event, Dsize, candidate-value, and reference-flag checks.\n";
  manifest << "- `same_event_selected_mass_overlay.{png,pdf}`: selected mass overlay for the same matched event set.\n";
  manifest << "- `same_event_validation.root`: ROOT histograms for later checks.\n\n";
  manifest << "## Guardrail\n\n";
  manifest << "If `matched_events` is small or zero, this file is not a physics comparison. It is only a provenance diagnostic telling us the chosen forest file is not represented in the scanned reference rows.\n";
  manifest.close();

  std::cout << outDir << "/README.md\n";
  return 0;
}
