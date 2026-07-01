#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "TFile.h"
#include "TH1D.h"
#include "TSystem.h"
#include "TTree.h"

#include "ZdcSelection.h"

namespace {
constexpr double D0_MASS = 1.86484;
constexpr double MASS_FIT_MIN = 1.70;
constexpr double MASS_FIT_MAX = 2.05;
constexpr double MASS_SIGNAL_MIN = 1.84;
constexpr double MASS_SIGNAL_MAX = 1.89;
constexpr double MASS_SB_LOW_MIN = D0_MASS - 0.12;
constexpr double MASS_SB_LOW_MAX = D0_MASS - 0.07;
constexpr double MASS_SB_HIGH_MIN = D0_MASS + 0.07;
constexpr double MASS_SB_HIGH_MAX = D0_MASS + 0.12;
constexpr double MASS_SIGNAL_WIDTH = MASS_SIGNAL_MAX - MASS_SIGNAL_MIN;
constexpr double MASS_SIDEBAND_WIDTH = (MASS_SB_LOW_MAX - MASS_SB_LOW_MIN) + (MASS_SB_HIGH_MAX - MASS_SB_HIGH_MIN);
constexpr int MASS_BINS = 70;

std::vector<std::string> Split(const std::string &line, char delim) {
  std::vector<std::string> out;
  std::string item;
  std::stringstream ss(line);
  while (std::getline(ss, item, delim)) out.push_back(item);
  if (!line.empty() && line.back() == delim) out.push_back("");
  return out;
}

double ToDouble(const std::map<std::string, std::string> &row, const std::string &key, double fallback = 0.0) {
  auto it = row.find(key);
  if (it == row.end() || it->second.empty()) return fallback;
  return std::atof(it->second.c_str());
}

int ToInt(const std::map<std::string, std::string> &row, const std::string &key, int fallback = 0) {
  auto it = row.find(key);
  if (it == row.end() || it->second.empty()) return fallback;
  return std::atoi(it->second.c_str());
}

std::string GetString(const std::map<std::string, std::string> &row, const std::string &key) {
  auto it = row.find(key);
  return it == row.end() ? "" : it->second;
}

struct Point {
  std::string pointId;
  std::string bin;
  int pointIndex = 0;
  int gridDistance = 0;
  double absYMin = 0.0;
  double absYMax = 0.0;
  double ptMin = 0.0;
  double ptMax = 0.0;
  double alphaMax = 0.0;
  double svpvSigMin = 0.0;
  double chi2clMin = 0.0;
  double dthetaMax = 0.0;
  double signalEfficiency = 0.0;
  double signalRetentionLoose = 0.0;
  double jointScoreRatio = 0.0;
  bool isANRow = false;
  TH1D *hist = nullptr;
  Long64_t fitRangeCount = 0;
  Long64_t massWindowCount = 0;
  Long64_t sidebandLowCount = 0;
  Long64_t sidebandHighCount = 0;
};

std::vector<Point> ReadPoints(const char *path) {
  std::ifstream in(path);
  if (!in) {
    std::cerr << "Could not open closure points file " << path << "\n";
    return {};
  }
  std::string line;
  if (!std::getline(in, line)) return {};
  const auto header = Split(line, '\t');
  std::vector<Point> points;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    const auto fields = Split(line, '\t');
    std::map<std::string, std::string> row;
    for (size_t i = 0; i < header.size() && i < fields.size(); ++i) row[header[i]] = fields[i];
    Point p;
    p.pointId = GetString(row, "pointId");
    p.bin = GetString(row, "bin");
    p.pointIndex = ToInt(row, "pointIndex");
    p.gridDistance = ToInt(row, "gridDistanceFromAN");
    p.absYMin = ToDouble(row, "absYMin");
    p.absYMax = ToDouble(row, "absYMax");
    p.ptMin = ToDouble(row, "ptMin");
    p.ptMax = ToDouble(row, "ptMax");
    p.alphaMax = ToDouble(row, "alphaMax");
    p.svpvSigMin = ToDouble(row, "svpvSigMin");
    p.chi2clMin = ToDouble(row, "chi2clMin");
    p.dthetaMax = ToDouble(row, "dthetaMax");
    p.signalEfficiency = ToDouble(row, "signalEfficiency");
    p.signalRetentionLoose = ToDouble(row, "signalRetentionLoose");
    p.jointScoreRatio = ToDouble(row, "jointScoreRatio");
    p.isANRow = GetString(row, "isANRow") == "true";
    const std::string histName = "h_" + p.pointId;
    p.hist = new TH1D(histName.c_str(), histName.c_str(), MASS_BINS, MASS_FIT_MIN, MASS_FIT_MAX);
    p.hist->SetDirectory(nullptr);
    points.push_back(p);
  }
  return points;
}

struct Candidate {
  double mass = 0.0;
  double pt = 0.0;
  double absY = 0.0;
  double alpha = 999.0;
  double svpvSig = -999.0;
  double chi2cl = -999.0;
  double dtheta = 999.0;
};

struct SampleTree {
  TFile *file = nullptr;
  TTree *tree = nullptr;

  Bool_t selectedVtxFilter = true;
  Bool_t clusterCompatibilityFilter = true;
  int nVtx = 1;
  Float_t zdcPlus = 0.0;
  Float_t zdcMinus = 0.0;
  Float_t hfPlus = 0.0;
  Float_t hfMinus = 0.0;

  std::vector<float> *Dpt = nullptr;
  std::vector<float> *Dy = nullptr;
  std::vector<float> *Dmass = nullptr;
  std::vector<float> *Dtrk1Pt = nullptr;
  std::vector<float> *Dtrk2Pt = nullptr;
  std::vector<float> *Dtrk1PtErr = nullptr;
  std::vector<float> *Dtrk2PtErr = nullptr;
  std::vector<float> *Dtrk1Eta = nullptr;
  std::vector<float> *Dtrk2Eta = nullptr;
  std::vector<float> *Dchi2cl = nullptr;
  std::vector<float> *DsvpvDistance = nullptr;
  std::vector<float> *DsvpvDisErr = nullptr;
  std::vector<float> *Dalpha = nullptr;
  std::vector<float> *Ddtheta = nullptr;

  bool hasSelectedVtx = false;
  bool hasCluster = false;
  bool hasNVtx = false;

  bool Open(const char *path, const char *treeName) {
    file = TFile::Open(path, "READ");
    if (!file || file->IsZombie()) {
      std::cerr << "Could not open " << path << "\n";
      return false;
    }
    file->GetObject(treeName, tree);
    if (!tree) {
      std::cerr << "Could not find tree `" << treeName << "` in " << path << "\n";
      return false;
    }
    Bind();
    return true;
  }

  bool Has(const char *name) const { return tree && tree->GetBranch(name); }

  template <typename T>
  void BindScalar(const char *name, T *value, bool *flag = nullptr) {
    if (Has(name)) {
      tree->SetBranchAddress(name, value);
      if (flag) *flag = true;
    } else if (flag) {
      *flag = false;
    }
  }

  template <typename T>
  void BindVector(const char *name, std::vector<T> **value) {
    if (Has(name)) tree->SetBranchAddress(name, value);
  }

  void Bind() {
    BindScalar("selectedVtxFilter", &selectedVtxFilter, &hasSelectedVtx);
    BindScalar("ClusterCompatibilityFilter", &clusterCompatibilityFilter, &hasCluster);
    BindScalar("nVtx", &nVtx, &hasNVtx);
    BindScalar("ZDCsumPlus", &zdcPlus);
    BindScalar("ZDCsumMinus", &zdcMinus);
    BindScalar("HFEMaxPlus", &hfPlus);
    BindScalar("HFEMaxMinus", &hfMinus);
    BindVector("Dpt", &Dpt);
    BindVector("Dy", &Dy);
    BindVector("Dmass", &Dmass);
    BindVector("Dtrk1Pt", &Dtrk1Pt);
    BindVector("Dtrk2Pt", &Dtrk2Pt);
    BindVector("Dtrk1PtErr", &Dtrk1PtErr);
    BindVector("Dtrk2PtErr", &Dtrk2PtErr);
    BindVector("Dtrk1Eta", &Dtrk1Eta);
    BindVector("Dtrk2Eta", &Dtrk2Eta);
    BindVector("Dchi2cl", &Dchi2cl);
    BindVector("DsvpvDistance", &DsvpvDistance);
    BindVector("DsvpvDisErr", &DsvpvDisErr);
    BindVector("Dalpha", &Dalpha);
    BindVector("Ddtheta", &Ddtheta);
  }

  Long64_t Entries(Long64_t maxEvents) const {
    const Long64_t entries = tree ? tree->GetEntries() : 0;
    return maxEvents >= 0 ? std::min(entries, maxEvents) : entries;
  }

  size_t CandidateCount() const {
    if (!Dmass || !Dpt || !Dy) return 0;
    size_t n = Dmass->size();
    n = std::min(n, Dpt->size());
    n = std::min(n, Dy->size());
    if (Dalpha) n = std::min(n, Dalpha->size());
    if (DsvpvDistance) n = std::min(n, DsvpvDistance->size());
    if (DsvpvDisErr) n = std::min(n, DsvpvDisErr->size());
    if (Dchi2cl) n = std::min(n, Dchi2cl->size());
    if (Ddtheta) n = std::min(n, Ddtheta->size());
    return n;
  }
};

bool PassEventQuality(const SampleTree &s) {
  if (s.hasSelectedVtx && !s.selectedVtxFilter) return false;
  if (s.hasCluster && !s.clusterCompatibilityFilter) return false;
  if (s.hasNVtx && (s.nVtx <= 0 || s.nVtx > 3)) return false;
  return true;
}

bool PassHfGap(const SampleTree &s) {
  const bool gammaN = d0zdc::Pass0nXn(s.zdcPlus, s.zdcMinus);
  const bool Ngamma = d0zdc::PassXn0n(s.zdcPlus, s.zdcMinus);
  return (gammaN && s.hfPlus < 9.2) || (Ngamma && s.hfMinus < 8.6);
}

bool InFitRange(double mass) { return mass > MASS_FIT_MIN && mass < MASS_FIT_MAX; }
bool InMassWindow(double mass) { return mass > MASS_SIGNAL_MIN && mass < MASS_SIGNAL_MAX; }
bool InLowSideband(double mass) { return mass > MASS_SB_LOW_MIN && mass < MASS_SB_LOW_MAX; }
bool InHighSideband(double mass) { return mass > MASS_SB_HIGH_MIN && mass < MASS_SB_HIGH_MAX; }

bool PassCandidateBaseline(const SampleTree &s, size_t i) {
  const double mass = s.Dmass->at(i);
  if (!InFitRange(mass)) return false;
  const double pt = s.Dpt->at(i);
  const double y = s.Dy->at(i);
  if (pt <= 2.0 || pt > 12.0 || std::abs(y) > 2.0) return false;
  if (s.Dtrk1Pt && i < s.Dtrk1Pt->size() && s.Dtrk1Pt->at(i) <= 1.0) return false;
  if (s.Dtrk2Pt && i < s.Dtrk2Pt->size() && s.Dtrk2Pt->at(i) <= 1.0) return false;
  if (s.Dtrk1Eta && i < s.Dtrk1Eta->size() && std::abs(s.Dtrk1Eta->at(i)) >= 2.4) return false;
  if (s.Dtrk2Eta && i < s.Dtrk2Eta->size() && std::abs(s.Dtrk2Eta->at(i)) >= 2.4) return false;
  if (s.Dtrk1Pt && s.Dtrk1PtErr && i < s.Dtrk1Pt->size() && i < s.Dtrk1PtErr->size()) {
    if (s.Dtrk1Pt->at(i) <= 0 || s.Dtrk1PtErr->at(i) / s.Dtrk1Pt->at(i) >= 0.1) return false;
  }
  if (s.Dtrk2Pt && s.Dtrk2PtErr && i < s.Dtrk2Pt->size() && i < s.Dtrk2PtErr->size()) {
    if (s.Dtrk2Pt->at(i) <= 0 || s.Dtrk2PtErr->at(i) / s.Dtrk2Pt->at(i) >= 0.1) return false;
  }
  return true;
}

Candidate MakeCandidate(const SampleTree &s, size_t i) {
  Candidate c;
  c.mass = s.Dmass->at(i);
  c.pt = s.Dpt->at(i);
  c.absY = std::abs(s.Dy->at(i));
  c.alpha = s.Dalpha ? s.Dalpha->at(i) : 999.0;
  c.chi2cl = s.Dchi2cl ? s.Dchi2cl->at(i) : -999.0;
  c.dtheta = s.Ddtheta ? s.Ddtheta->at(i) : 999.0;
  if (s.DsvpvDistance && s.DsvpvDisErr && i < s.DsvpvDistance->size() && i < s.DsvpvDisErr->size() && s.DsvpvDisErr->at(i) > 0) {
    c.svpvSig = s.DsvpvDistance->at(i) / s.DsvpvDisErr->at(i);
  }
  return c;
}

bool InBin(const Candidate &c, const Point &p) {
  const bool inPt = c.pt >= p.ptMin && (c.pt < p.ptMax || (p.ptMax == 12.0 && c.pt <= p.ptMax));
  const bool inY = c.absY >= p.absYMin && (c.absY < p.absYMax || (p.absYMax == 2.0 && c.absY <= p.absYMax));
  return inPt && inY;
}

bool PassTopology(const Candidate &c, const Point &p) {
  return c.alpha < p.alphaMax && c.svpvSig > p.svpvSigMin && c.chi2cl > p.chi2clMin && c.dtheta < p.dthetaMax;
}

void FillPoint(Point &p, const Candidate &c) {
  ++p.fitRangeCount;
  p.hist->Fill(c.mass);
  if (InMassWindow(c.mass)) ++p.massWindowCount;
  if (InLowSideband(c.mass)) ++p.sidebandLowCount;
  if (InHighSideband(c.mass)) ++p.sidebandHighCount;
}

void WriteOutputs(const std::vector<Point> &points, const std::string &outDir,
                  Long64_t scannedEvents, Long64_t qualityEvents, Long64_t eventPassEvents) {
  const double scale = MASS_SIGNAL_WIDTH / MASS_SIDEBAND_WIDTH;
  std::ofstream yields(outDir + "/closure_yields.tsv");
  yields << "pointId\tbin\tpointIndex\tgridDistanceFromAN\talphaMax\tsvpvSigMin\tchi2clMin\tdthetaMax\tisANRow\t"
         << "signalEfficiency\tsignalRetentionLoose\tjointScoreRatio\tfitRangeCount\tmassWindowCount\t"
         << "sidebandLowCount\tsidebandHighCount\tsidebandSubtractedYield\tsidebandSubtractedYieldErr\t"
         << "efficiencyCorrectedYield\tefficiencyCorrectedYieldErr\n";
  yields << std::setprecision(8);
  for (const auto &p : points) {
    const double sideband = static_cast<double>(p.sidebandLowCount + p.sidebandHighCount);
    const double rawYield = static_cast<double>(p.massWindowCount) - scale * sideband;
    const double rawErr = std::sqrt(static_cast<double>(p.massWindowCount) + scale * scale * sideband);
    const double corr = p.signalEfficiency > 0 ? rawYield / p.signalEfficiency : 0.0;
    const double corrErr = p.signalEfficiency > 0 ? rawErr / p.signalEfficiency : 0.0;
    yields << p.pointId << '\t' << p.bin << '\t' << p.pointIndex << '\t' << p.gridDistance << '\t'
           << p.alphaMax << '\t' << p.svpvSigMin << '\t' << p.chi2clMin << '\t' << p.dthetaMax << '\t'
           << (p.isANRow ? "true" : "false") << '\t' << p.signalEfficiency << '\t'
           << p.signalRetentionLoose << '\t' << p.jointScoreRatio << '\t' << p.fitRangeCount << '\t'
           << p.massWindowCount << '\t' << p.sidebandLowCount << '\t' << p.sidebandHighCount << '\t'
           << rawYield << '\t' << rawErr << '\t' << corr << '\t' << corrErr << '\n';
  }

  std::ofstream hist(outDir + "/closure_mass_hist.tsv");
  hist << "pointId\tbin\tisANRow\talphaMax\tdthetaMax\tmassBinLow\tmassBinHigh\tcount\n";
  hist << std::setprecision(8);
  for (const auto &p : points) {
    for (int ib = 1; ib <= p.hist->GetNbinsX(); ++ib) {
      hist << p.pointId << '\t' << p.bin << '\t' << (p.isANRow ? "true" : "false") << '\t'
           << p.alphaMax << '\t' << p.dthetaMax << '\t'
           << p.hist->GetXaxis()->GetBinLowEdge(ib) << '\t'
           << p.hist->GetXaxis()->GetBinUpEdge(ib) << '\t'
           << p.hist->GetBinContent(ib) << '\n';
    }
  }

  std::ofstream readme(outDir + "/README.md");
  readme << "# D0 Pointing-Cut Closure Inputs\n\n";
  readme << "This directory contains post-derivation closure outputs for nearby accepted `Dalpha`/`Ddtheta` plateau points.\n\n";
  readme << "## Event Accounting\n\n";
  readme << "- scanned events: `" << scannedEvents << "`\n";
  readme << "- quality events: `" << qualityEvents << "`\n";
  readme << "- ZDC/HF-gap events: `" << eventPassEvents << "`\n\n";
  readme << "## Fixed Choices\n\n";
  readme << "- Data tree: official 2023 reduced `Tree` skim.\n";
  readme << "- Event cuts: quality filters, AN calibrated ZDC XOR `plus > 1100 GeV` or `minus > 1000 GeV` with the opposite side below threshold, and AN-compatible HF `9.2/8.6`.\n";
  readme << "- Candidate cuts: track acceptance and quality, then the tested topology point.\n";
  readme << "- Raw closure yield: `1.84 < Dmass < 1.89` minus scaled AN-sideband counts.\n";
  readme << "- Corrected closure yield: raw closure yield divided by the frozen MC signal efficiency from the topology scan.\n\n";
  readme << "This is a diagnostic closure check, not the final AN mass-fit model.\n";
}
}

int check_topological_pointing_closure(const char *dataPath,
                                       const char *pointsPath,
                                       const char *outDir,
                                       Long64_t maxEvents = -1) {
  gSystem->mkdir(outDir, true);

  std::vector<Point> points = ReadPoints(pointsPath);
  if (points.empty()) {
    std::cerr << "No closure points loaded from " << pointsPath << "\n";
    return 1;
  }

  SampleTree data;
  if (!data.Open(dataPath, "Tree")) return 1;

  const Long64_t n = data.Entries(maxEvents);
  Long64_t qualityEvents = 0;
  Long64_t eventPassEvents = 0;
  for (Long64_t entry = 0; entry < n; ++entry) {
    data.tree->GetEntry(entry);
    if (!PassEventQuality(data)) continue;
    ++qualityEvents;
    if (!PassHfGap(data)) continue;
    ++eventPassEvents;
    const size_t nCand = data.CandidateCount();
    for (size_t i = 0; i < nCand; ++i) {
      if (!PassCandidateBaseline(data, i)) continue;
      const Candidate cand = MakeCandidate(data, i);
      for (auto &point : points) {
        if (!InBin(cand, point)) continue;
        if (!PassTopology(cand, point)) continue;
        FillPoint(point, cand);
      }
    }
  }

  WriteOutputs(points, outDir, n, qualityEvents, eventPassEvents);
  std::cout << "Pointing closure outputs written to " << outDir << "\n";
  return 0;
}
