#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include "TFile.h"
#include "TSystem.h"
#include "TTree.h"

#include "ZdcSelection.h"

namespace {
constexpr double D0_MASS = 1.86484;
constexpr double MASS_FIT_MIN = 1.70;
constexpr double MASS_FIT_MAX = 2.05;
constexpr double AN_INNER = 0.07;
constexpr double AN_OUTER = 0.12;

struct PtYBin {
  const char *label;
  double absYMin;
  double absYMax;
  double ptMin;
  double ptMax;
};

const std::vector<PtYBin> &Bins() {
  static const std::vector<PtYBin> bins = {
      {"absy0to1_pt2to5", 0.0, 1.0, 2.0, 5.0},
      {"absy0to1_pt5to8", 0.0, 1.0, 5.0, 8.0},
      {"absy0to1_pt8to12", 0.0, 1.0, 8.0, 12.0},
      {"absy1to2_pt2to5", 1.0, 2.0, 2.0, 5.0},
      {"absy1to2_pt5to8", 1.0, 2.0, 5.0, 8.0},
      {"absy1to2_pt8to12", 1.0, 2.0, 8.0, 12.0},
  };
  return bins;
}

int FindBin(double pt, double y) {
  const double ay = std::abs(y);
  const auto &bins = Bins();
  for (size_t i = 0; i < bins.size(); ++i) {
    const bool inPt = pt >= bins[i].ptMin && (pt < bins[i].ptMax || (bins[i].ptMax == 12.0 && pt <= bins[i].ptMax));
    const bool inY = ay >= bins[i].absYMin && (ay < bins[i].absYMax || (bins[i].absYMax == 2.0 && ay <= bins[i].absYMax));
    if (inPt && inY) return static_cast<int>(i);
  }
  return -1;
}

struct CandidateVars {
  double mass = 0.0;
  double pt = 0.0;
  double y = 0.0;
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
  std::vector<bool> *DisSignalCalc = nullptr;
  std::vector<bool> *DisSignalCalcPrompt = nullptr;
  std::vector<int> *Dgen = nullptr;

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
    BindVector("DisSignalCalc", &DisSignalCalc);
    BindVector("DisSignalCalcPrompt", &DisSignalCalcPrompt);
    BindVector("Dgen", &Dgen);
  }

  Long64_t Entries(Long64_t maxEvents) const {
    if (!tree) return 0;
    const Long64_t entries = tree->GetEntries();
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

bool PassNominalEventTopology(const SampleTree &s) {
  const bool gammaN = d0zdc::Pass0nXn(s.zdcPlus, s.zdcMinus);
  const bool Ngamma = d0zdc::PassXn0n(s.zdcPlus, s.zdcMinus);
  return (gammaN && s.hfPlus < 9.2) || (Ngamma && s.hfMinus < 8.6);
}

bool TruthMatchedPrompt(const SampleTree &s, size_t i) {
  if (s.DisSignalCalcPrompt && i < s.DisSignalCalcPrompt->size()) return s.DisSignalCalcPrompt->at(i);
  if (s.DisSignalCalc && i < s.DisSignalCalc->size()) return s.DisSignalCalc->at(i);
  if (s.Dgen && i < s.Dgen->size()) return s.Dgen->at(i) != 0;
  return false;
}

bool InFitRange(double mass) {
  return mass > MASS_FIT_MIN && mass < MASS_FIT_MAX;
}

bool PassCandidateBaseline(const SampleTree &s, size_t i, bool requireTruth) {
  const double mass = s.Dmass->at(i);
  if (!InFitRange(mass)) return false;
  if (requireTruth && !TruthMatchedPrompt(s, i)) return false;
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

CandidateVars MakeCandidate(const SampleTree &s, size_t i) {
  CandidateVars c;
  c.mass = s.Dmass->at(i);
  c.pt = s.Dpt->at(i);
  c.y = s.Dy->at(i);
  c.alpha = s.Dalpha ? s.Dalpha->at(i) : 999.0;
  c.chi2cl = s.Dchi2cl ? s.Dchi2cl->at(i) : -999.0;
  c.dtheta = s.Ddtheta ? s.Ddtheta->at(i) : 999.0;
  if (s.DsvpvDistance && s.DsvpvDisErr && i < s.DsvpvDistance->size() && i < s.DsvpvDisErr->size() && s.DsvpvDisErr->at(i) > 0) {
    c.svpvSig = s.DsvpvDistance->at(i) / s.DsvpvDisErr->at(i);
  }
  return c;
}

double Quantile(std::vector<double> values, double q) {
  if (values.empty()) return std::numeric_limits<double>::quiet_NaN();
  std::sort(values.begin(), values.end());
  const double pos = q * (values.size() - 1);
  const size_t lo = static_cast<size_t>(std::floor(pos));
  const size_t hi = static_cast<size_t>(std::ceil(pos));
  if (lo == hi) return values[lo];
  const double t = pos - lo;
  return values[lo] * (1.0 - t) + values[hi] * t;
}

double RoundOutward(double value, double step) {
  return std::ceil((value - 1e-12) / step) * step;
}

double RoundNearest(double value, double step) {
  return std::floor(value / step + 0.5) * step;
}

double Mean(const std::vector<double> &values) {
  if (values.empty()) return 0.0;
  return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

struct SidebandStats {
  int lower = 0;
  int upper = 0;
  double lowerAlphaMean = 0.0;
  double upperAlphaMean = 0.0;
  double lowerSvpvMean = 0.0;
  double upperSvpvMean = 0.0;
  double lowerChi2Mean = 0.0;
  double upperChi2Mean = 0.0;
  double lowerDthetaMean = 0.0;
  double upperDthetaMean = 0.0;
};

struct SidebandAccum {
  int lower = 0;
  int upper = 0;
  double lowerAlphaSum = 0.0;
  double upperAlphaSum = 0.0;
  double lowerSvpvSum = 0.0;
  double upperSvpvSum = 0.0;
  double lowerChi2Sum = 0.0;
  double upperChi2Sum = 0.0;
  double lowerDthetaSum = 0.0;
  double upperDthetaSum = 0.0;

  void FillLower(const CandidateVars &c) {
    ++lower;
    lowerAlphaSum += c.alpha;
    lowerSvpvSum += c.svpvSig;
    lowerChi2Sum += c.chi2cl;
    lowerDthetaSum += c.dtheta;
  }

  void FillUpper(const CandidateVars &c) {
    ++upper;
    upperAlphaSum += c.alpha;
    upperSvpvSum += c.svpvSig;
    upperChi2Sum += c.chi2cl;
    upperDthetaSum += c.dtheta;
  }

  SidebandStats Finalize() const {
    SidebandStats out;
    out.lower = lower;
    out.upper = upper;
    out.lowerAlphaMean = lower > 0 ? lowerAlphaSum / lower : 0.0;
    out.upperAlphaMean = upper > 0 ? upperAlphaSum / upper : 0.0;
    out.lowerSvpvMean = lower > 0 ? lowerSvpvSum / lower : 0.0;
    out.upperSvpvMean = upper > 0 ? upperSvpvSum / upper : 0.0;
    out.lowerChi2Mean = lower > 0 ? lowerChi2Sum / lower : 0.0;
    out.upperChi2Mean = upper > 0 ? upperChi2Sum / upper : 0.0;
    out.lowerDthetaMean = lower > 0 ? lowerDthetaSum / lower : 0.0;
    out.upperDthetaMean = upper > 0 ? upperDthetaSum / upper : 0.0;
    return out;
  }
};

bool InLowerSideband(double mass, double inner, double outer) {
  const double dm = mass - D0_MASS;
  return dm < -inner && dm > -outer;
}

bool InUpperSideband(double mass, double inner, double outer) {
  const double dm = mass - D0_MASS;
  return dm > inner && dm < outer;
}

bool InAnySideband(double mass, double inner, double outer) {
  const double absDm = std::abs(mass - D0_MASS);
  return absDm > inner && absDm < outer;
}

std::vector<std::vector<double>> CollectMcResiduals(SampleTree &mc, Long64_t maxEvents) {
  std::vector<std::vector<double>> residuals(Bins().size());
  const Long64_t n = mc.Entries(maxEvents);
  for (Long64_t entry = 0; entry < n; ++entry) {
    mc.tree->GetEntry(entry);
    if (!PassEventQuality(mc)) continue;
    const size_t nCand = mc.CandidateCount();
    for (size_t i = 0; i < nCand; ++i) {
      if (!PassCandidateBaseline(mc, i, true)) continue;
      const int bin = FindBin(mc.Dpt->at(i), mc.Dy->at(i));
      if (bin < 0) continue;
      residuals[bin].push_back(mc.Dmass->at(i) - D0_MASS);
    }
  }
  return residuals;
}

std::vector<std::vector<CandidateVars>> CollectDataCandidates(SampleTree &data, Long64_t maxEvents) {
  std::vector<std::vector<CandidateVars>> candidates(Bins().size());
  const Long64_t n = data.Entries(maxEvents);
  for (Long64_t entry = 0; entry < n; ++entry) {
    data.tree->GetEntry(entry);
    if (!PassEventQuality(data)) continue;
    if (!PassNominalEventTopology(data)) continue;
    const size_t nCand = data.CandidateCount();
    for (size_t i = 0; i < nCand; ++i) {
      if (!PassCandidateBaseline(data, i, false)) continue;
      const int bin = FindBin(data.Dpt->at(i), data.Dy->at(i));
      if (bin < 0) continue;
      candidates[bin].push_back(MakeCandidate(data, i));
    }
  }
  return candidates;
}

double LeakageFraction(const std::vector<double> &residuals, double inner, double outer) {
  if (residuals.empty()) return 0.0;
  int leaked = 0;
  for (double residual : residuals) {
    const double absDm = std::abs(residual);
    if (absDm > inner && absDm < outer) ++leaked;
  }
  return static_cast<double>(leaked) / residuals.size();
}

SidebandStats ComputeStats(const std::vector<CandidateVars> &candidates, double inner, double outer) {
  std::vector<double> lowerAlpha, upperAlpha;
  std::vector<double> lowerSvpv, upperSvpv;
  std::vector<double> lowerChi2, upperChi2;
  std::vector<double> lowerDtheta, upperDtheta;
  for (const auto &c : candidates) {
    if (InLowerSideband(c.mass, inner, outer)) {
      lowerAlpha.push_back(c.alpha);
      lowerSvpv.push_back(c.svpvSig);
      lowerChi2.push_back(c.chi2cl);
      lowerDtheta.push_back(c.dtheta);
    } else if (InUpperSideband(c.mass, inner, outer)) {
      upperAlpha.push_back(c.alpha);
      upperSvpv.push_back(c.svpvSig);
      upperChi2.push_back(c.chi2cl);
      upperDtheta.push_back(c.dtheta);
    }
  }
  return {
      static_cast<int>(lowerAlpha.size()),
      static_cast<int>(upperAlpha.size()),
      Mean(lowerAlpha),
      Mean(upperAlpha),
      Mean(lowerSvpv),
      Mean(upperSvpv),
      Mean(lowerChi2),
      Mean(upperChi2),
      Mean(lowerDtheta),
      Mean(upperDtheta),
  };
}

std::vector<std::vector<SidebandStats>> CollectDataSidebandStats(
    SampleTree &data,
    Long64_t maxEvents,
    const std::vector<std::pair<double, double>> &windows) {
  std::vector<std::vector<SidebandAccum>> accum(windows.size(), std::vector<SidebandAccum>(Bins().size()));
  const Long64_t n = data.Entries(maxEvents);
  for (Long64_t entry = 0; entry < n; ++entry) {
    data.tree->GetEntry(entry);
    if (!PassEventQuality(data)) continue;
    if (!PassNominalEventTopology(data)) continue;
    const size_t nCand = data.CandidateCount();
    for (size_t i = 0; i < nCand; ++i) {
      if (!PassCandidateBaseline(data, i, false)) continue;
      const int bin = FindBin(data.Dpt->at(i), data.Dy->at(i));
      if (bin < 0) continue;
      const CandidateVars cand = MakeCandidate(data, i);
      for (size_t iw = 0; iw < windows.size(); ++iw) {
        const double inner = windows[iw].first;
        const double outer = windows[iw].second;
        if (InLowerSideband(cand.mass, inner, outer)) {
          accum[iw][bin].FillLower(cand);
        } else if (InUpperSideband(cand.mass, inner, outer)) {
          accum[iw][bin].FillUpper(cand);
        }
      }
    }
  }

  std::vector<std::vector<SidebandStats>> out(windows.size(), std::vector<SidebandStats>(Bins().size()));
  for (size_t iw = 0; iw < windows.size(); ++iw) {
    for (size_t ib = 0; ib < Bins().size(); ++ib) {
      out[iw][ib] = accum[iw][ib].Finalize();
    }
  }
  return out;
}

void WriteSidebandScan(const std::string &outDir,
                       const std::vector<std::vector<double>> &residuals,
                       const std::vector<std::pair<double, double>> &windows,
                       const std::vector<std::vector<SidebandStats>> &statsByWindow) {
  std::ofstream out(outDir + "/sideband_stability_scan.tsv");
  out << "windowLabel\tbin\tinnerAbsDm\touterAbsDm\tsignalLeakageFraction\tlowerCount\tupperCount\tcountAsymmetry\t"
      << "lowerAlphaMean\tupperAlphaMean\talphaMeanDiff\tlowerSvpvMean\tupperSvpvMean\tsvpvMeanDiff\t"
      << "lowerChi2Mean\tupperChi2Mean\tchi2MeanDiff\tlowerDthetaMean\tupperDthetaMean\tdthetaMeanDiff\n";
  out << std::setprecision(8);
  for (size_t iw = 0; iw < windows.size(); ++iw) {
    const auto &window = windows[iw];
    const double inner = window.first;
    const double outer = window.second;
    const std::string label = std::to_string(inner) + "_" + std::to_string(outer);
    for (size_t ib = 0; ib < Bins().size(); ++ib) {
      const auto stats = statsByWindow[iw][ib];
      const int total = stats.lower + stats.upper;
      const double asym = total > 0 ? static_cast<double>(stats.upper - stats.lower) / total : 0.0;
      out << label << '\t' << Bins()[ib].label << '\t' << inner << '\t' << outer << '\t'
          << LeakageFraction(residuals[ib], inner, outer) << '\t'
          << stats.lower << '\t' << stats.upper << '\t' << asym << '\t'
          << stats.lowerAlphaMean << '\t' << stats.upperAlphaMean << '\t' << (stats.upperAlphaMean - stats.lowerAlphaMean) << '\t'
          << stats.lowerSvpvMean << '\t' << stats.upperSvpvMean << '\t' << (stats.upperSvpvMean - stats.lowerSvpvMean) << '\t'
          << stats.lowerChi2Mean << '\t' << stats.upperChi2Mean << '\t' << (stats.upperChi2Mean - stats.lowerChi2Mean) << '\t'
          << stats.lowerDthetaMean << '\t' << stats.upperDthetaMean << '\t' << (stats.upperDthetaMean - stats.lowerDthetaMean) << '\n';
    }
  }
}

void WriteReadme(const std::string &outDir,
                 const char *signalPath,
                 const char *dataPath,
                 Long64_t maxEvents,
                 double inner,
                 double outer) {
  std::ofstream out(outDir + "/README.md");
  out << "# D0 Topology Sideband Derivation\n\n";
  out << "This output derives topology-optimization sidebands without using the AN sideband constants as inputs.\n\n";
  out << "## Rule\n\n";
  out << "1. Use truth-matched prompt-MC D0 candidates after track and fiducial cuts.\n";
  out << "2. Estimate each pT/|y| bin mass resolution by `(q84.135 - q15.865) / 2`.\n";
  out << "3. Use one global sideband for all bins, based on the worst-bin resolution.\n";
  out << "4. Set the inner edge to `round(3 sigma_max / 10 MeV) * 10 MeV`.\n";
  out << "5. Set the outer edge to `round(5 sigma_max / 10 MeV) * 10 MeV`.\n";
  out << "6. Compare to the AN only after the window is frozen.\n\n";
  out << "## Result\n\n";
  out << "- derived sideband: `" << inner << " < |Dmass - 1.86484| < " << outer << "` GeV\n";
  out << "- AN sideband comparison target: `0.07 < |Dmass - 1.86484| < 0.12` GeV\n";
  out << "- matches AN: `" << ((std::abs(inner - AN_INNER) < 1e-9 && std::abs(outer - AN_OUTER) < 1e-9) ? "true" : "false") << "`\n\n";
  out << "## Inputs\n\n";
  out << "- signal MC: `" << signalPath << "`\n";
  out << "- data: `" << dataPath << "`\n";
  out << "- max events: `" << maxEvents << "`\n\n";
  out << "## Outputs\n\n";
  out << "- `sideband_resolution.tsv`\n";
  out << "- `recommended_sidebands.tsv`\n";
  out << "- `sideband_stability_scan.tsv`\n";
}
}

int derive_d0_topology_sidebands(const char *signalPath,
                                 const char *dataPath,
                                 const char *outDir,
                                 Long64_t maxEvents = -1) {
  gSystem->mkdir(outDir, true);
  const std::string outDirString(outDir);

  SampleTree mc, data;
  if (!mc.Open(signalPath, "Tree")) return 1;
  if (!data.Open(dataPath, "Tree")) return 1;

  const auto residuals = CollectMcResiduals(mc, maxEvents);

  std::ofstream resolution(outDirString + "/sideband_resolution.tsv");
  resolution << "bin\tmcSignalCount\tmedianResidual\tq15865\tq84135\tsigma68\tinner3sigma\touter5sigma\tinnerRounded10MeV\touterRounded10MeV\n";
  resolution << std::setprecision(8);

  double sigmaMax = 0.0;
  for (size_t ib = 0; ib < Bins().size(); ++ib) {
    const auto &values = residuals[ib];
    const double median = Quantile(values, 0.5);
    const double q16 = Quantile(values, 0.15865);
    const double q84 = Quantile(values, 0.84135);
    const double sigma = (q84 - q16) / 2.0;
    if (std::isfinite(sigma)) sigmaMax = std::max(sigmaMax, sigma);
    resolution << Bins()[ib].label << '\t' << values.size() << '\t' << median << '\t'
               << q16 << '\t' << q84 << '\t' << sigma << '\t'
               << 3.0 * sigma << '\t' << 5.0 * sigma << '\t'
               << RoundNearest(3.0 * sigma, 0.01) << '\t'
               << RoundNearest(5.0 * sigma, 0.01) << '\n';
  }

  const double inner = RoundNearest(3.0 * sigmaMax, 0.01);
  const double outer = std::max(inner + 0.01, RoundNearest(5.0 * sigmaMax, 0.01));

  std::ofstream rec(outDirString + "/recommended_sidebands.tsv");
  rec << "id\tsigmaSource\tmaxSigma68\tinnerAbsDm\touterAbsDm\tanInnerAbsDm\tanOuterAbsDm\tmatchesAN\tselectionRule\n";
  rec << std::setprecision(8);
  rec << "global_resolution_rule\tmax_over_bins\t" << sigmaMax << '\t' << inner << '\t' << outer << '\t'
      << AN_INNER << '\t' << AN_OUTER << '\t'
      << ((std::abs(inner - AN_INNER) < 1e-9 && std::abs(outer - AN_OUTER) < 1e-9) ? "true" : "false") << '\t'
      << "inner=round(3*sigmaMax/0.01)*0.01, outer=round(5*sigmaMax/0.01)*0.01\n";

  std::vector<std::pair<double, double>> windows;
  windows.push_back({inner, outer});
  windows.push_back({AN_INNER, AN_OUTER});
  if (inner >= 0.02) windows.push_back({inner - 0.01, outer});
  windows.push_back({inner + 0.01, outer});
  if (outer > inner + 0.02) windows.push_back({inner, outer - 0.01});
  windows.push_back({inner, outer + 0.01});
  std::sort(windows.begin(), windows.end());
  windows.erase(std::unique(windows.begin(), windows.end()), windows.end());
  const auto dataStats = CollectDataSidebandStats(data, maxEvents, windows);
  WriteSidebandScan(outDir, residuals, windows, dataStats);
  WriteReadme(outDir, signalPath, dataPath, maxEvents, inner, outer);

  std::cout << "Derived sideband: " << inner << " < |Dmass - " << D0_MASS << "| < " << outer << " GeV\n";
  std::cout << "Sideband derivation outputs written to " << outDir << "\n";
  return 0;
}
