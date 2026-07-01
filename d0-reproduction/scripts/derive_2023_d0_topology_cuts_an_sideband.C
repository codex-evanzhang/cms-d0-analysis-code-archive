#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
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
constexpr double MASS_SIGNAL_MIN = 1.84;
constexpr double MASS_SIGNAL_MAX = 1.89;
double gSidebandInner = 0.07;
double gSidebandOuter = 0.12;

struct PtYBin {
  const char *label;
  double absYMin;
  double absYMax;
  double ptMin;
  double ptMax;
  double nominalAlphaMax;
  double nominalSvpvSigMin;
  double nominalChi2ClMin;
  double nominalDthetaMax;
};

const std::vector<PtYBin> &Bins() {
  static const std::vector<PtYBin> bins = {
      {"absy0to1_pt2to5", 0.0, 1.0, 2.0, 5.0, 0.40, 2.5, 0.10, 0.50},
      {"absy0to1_pt5to8", 0.0, 1.0, 5.0, 8.0, 0.35, 3.5, 0.10, 0.30},
      {"absy0to1_pt8to12", 0.0, 1.0, 8.0, 12.0, 0.40, 3.5, 0.10, 0.30},
      {"absy1to2_pt2to5", 1.0, 2.0, 2.0, 5.0, 0.20, 2.5, 0.10, 0.30},
      {"absy1to2_pt5to8", 1.0, 2.0, 5.0, 8.0, 0.25, 3.5, 0.10, 0.30},
      {"absy1to2_pt8to12", 1.0, 2.0, 8.0, 12.0, 0.25, 3.5, 0.10, 0.30},
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

bool InSideband(double mass) {
  const double absDm = std::abs(mass - D0_MASS);
  return absDm > gSidebandInner && absDm < gSidebandOuter;
}

bool InFitRange(double mass) {
  return mass > MASS_FIT_MIN && mass < MASS_FIT_MAX;
}

struct Candidate {
  double alpha = 999.0;
  double svpvSig = -999.0;
  double chi2cl = -999.0;
  double dtheta = 999.0;
};

struct SampleTree {
  TFile *file = nullptr;
  TTree *tree = nullptr;
  std::string label;

  Bool_t selectedVtxFilter = true;
  Bool_t clusterCompatibilityFilter = true;
  int nVtx = 1;
  Float_t zdcPlus = 0.0;
  Float_t zdcMinus = 0.0;
  Float_t hfPlus = 0.0;
  Float_t hfMinus = 0.0;
  int dSize = 0;

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
  bool hasTruthPrompt = false;
  bool hasTruthSignal = false;
  bool hasDgen = false;

  bool Open(const char *path, const char *treeName, const char *sampleLabel) {
    label = sampleLabel;
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

  bool Has(const char *name) const {
    return tree && tree->GetBranch(name);
  }

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
  void BindVector(const char *name, std::vector<T> **value, bool *flag = nullptr) {
    if (Has(name)) {
      tree->SetBranchAddress(name, value);
      if (flag) *flag = true;
    } else if (flag) {
      *flag = false;
    }
  }

  void Bind() {
    BindScalar("selectedVtxFilter", &selectedVtxFilter, &hasSelectedVtx);
    BindScalar("ClusterCompatibilityFilter", &clusterCompatibilityFilter, &hasCluster);
    BindScalar("nVtx", &nVtx, &hasNVtx);
    BindScalar("ZDCsumPlus", &zdcPlus);
    BindScalar("ZDCsumMinus", &zdcMinus);
    BindScalar("HFEMaxPlus", &hfPlus);
    BindScalar("HFEMaxMinus", &hfMinus);
    BindScalar("Dsize", &dSize);
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
    BindVector("DisSignalCalc", &DisSignalCalc, &hasTruthSignal);
    BindVector("DisSignalCalcPrompt", &DisSignalCalcPrompt, &hasTruthPrompt);
    BindVector("Dgen", &Dgen, &hasDgen);
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

bool ZdcGammaN(const SampleTree &s, double plusThr, double minusThr) {
  return s.zdcMinus > minusThr && s.zdcPlus < plusThr;
}

bool ZdcNgamma(const SampleTree &s, double plusThr, double minusThr) {
  return s.zdcPlus > plusThr && s.zdcMinus < minusThr;
}

bool PassHfGap(const SampleTree &s, double plusThr, double minusThr, double hfPlusThr, double hfMinusThr) {
  const bool gammaN = ZdcGammaN(s, plusThr, minusThr);
  const bool Ngamma = ZdcNgamma(s, plusThr, minusThr);
  return (gammaN && s.hfPlus < hfPlusThr) || (Ngamma && s.hfMinus < hfMinusThr);
}

bool PassNominalHfGap(const SampleTree &s, double hfPlusThr, double hfMinusThr) {
  const bool gammaN = d0zdc::Pass0nXn(s.zdcPlus, s.zdcMinus);
  const bool Ngamma = d0zdc::PassXn0n(s.zdcPlus, s.zdcMinus);
  return (gammaN && s.hfPlus < hfPlusThr) || (Ngamma && s.hfMinus < hfMinusThr);
}

bool TruthMatchedPrompt(const SampleTree &s, size_t i) {
  if (s.DisSignalCalcPrompt && i < s.DisSignalCalcPrompt->size()) return s.DisSignalCalcPrompt->at(i);
  if (s.DisSignalCalc && i < s.DisSignalCalc->size()) return s.DisSignalCalc->at(i);
  if (s.Dgen && i < s.Dgen->size()) return s.Dgen->at(i) != 0;
  return false;
}

bool PassCandidateBaseline(const SampleTree &s, size_t i, bool requireTruth, bool requireSideband) {
  const double mass = s.Dmass->at(i);
  if (!InFitRange(mass)) return false;
  if (requireSideband && !InSideband(mass)) return false;
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

Candidate MakeCandidate(const SampleTree &s, size_t i) {
  Candidate c;
  c.alpha = s.Dalpha ? s.Dalpha->at(i) : 999.0;
  c.chi2cl = s.Dchi2cl ? s.Dchi2cl->at(i) : -999.0;
  c.dtheta = s.Ddtheta ? s.Ddtheta->at(i) : 999.0;
  if (s.DsvpvDistance && s.DsvpvDisErr && i < s.DsvpvDistance->size() && i < s.DsvpvDisErr->size() && s.DsvpvDisErr->at(i) > 0) {
    c.svpvSig = s.DsvpvDistance->at(i) / s.DsvpvDisErr->at(i);
  }
  return c;
}

struct EventThresholdBest {
  double score = -1.0;
  double first = 0.0;
  double second = 0.0;
  double mcEff = 0.0;
  double controlFake = 0.0;
};

struct EventSummary {
  bool quality = false;
  double zdcPlus = 0.0;
  double zdcMinus = 0.0;
  double hfPlus = 0.0;
  double hfMinus = 0.0;
};

struct TopologyBest {
  double score = -1.0;
  double alpha = 0.0;
  double svpv = 0.0;
  double chi2 = 0.0;
  double dtheta = 0.0;
  double sigEff = 0.0;
  double bkgFrac = 0.0;
  int sigBase = 0;
  int bkgBase = 0;
  int sigPass = 0;
  int bkgPass = 0;
};

bool PassTopology(const Candidate &c, double alphaMax, double svpvMin, double chi2Min, double dthetaMax) {
  return c.alpha < alphaMax && c.svpvSig > svpvMin && c.chi2cl > chi2Min && c.dtheta < dthetaMax;
}

void WriteRationale(const std::string &outDir) {
  std::ofstream out(outDir + "/cut_rationale.md");
  out << "# D0 Cut Rationale\n\n";
  out << "- `selectedVtxFilter`, `ClusterCompatibilityFilter`, and `nVtx` remove bad-event topology before candidate work.\n";
  out << "- ZDC XOR thresholds define the `0nXn` UPC neutron topology and suppress ZDC noise/ambiguous `XnXn` events.\n";
  out << "- HF gap thresholds require low forward activity on the photon-going side, rejecting hadronic contamination.\n";
  out << "- Daughter `pT`, `eta`, and relative `pT` error cuts keep well-measured kaon/pion tracks in tracker acceptance.\n";
  out << "- `Dalpha` requires the D momentum to point back to the primary vertex.\n";
  out << "- `DsvpvSig` selects displaced D decay vertices and suppresses prompt random track pairs.\n";
  out << "- `Dchi2cl` rejects poorly fit secondary vertices.\n";
  out << "- `Ddtheta` rejects angularly inconsistent Kpi combinations.\n\n";
  out << "The reduced official skim `Tree` does not expose track high-purity flags. Those are still part of the from-forest reproduction, but this MC/data cut scan can only use the reduced-tree branches it can read.\n";
}

void CollectCandidates(SampleTree &sample, bool signal, bool requireNominalEventTopology, Long64_t maxEvents,
                       std::vector<std::vector<Candidate>> &out,
                       std::vector<int> &eventCounts) {
  const Long64_t n = sample.Entries(maxEvents);
  for (Long64_t entry = 0; entry < n; ++entry) {
    sample.tree->GetEntry(entry);
    if (!PassEventQuality(sample)) continue;
    if (requireNominalEventTopology && !PassNominalHfGap(sample, 9.2, 8.6)) continue;
    const size_t nCand = sample.CandidateCount();
    for (size_t i = 0; i < nCand; ++i) {
      const bool keep = PassCandidateBaseline(sample, i, signal, !signal);
      if (!keep) continue;
      const int bin = FindBin(sample.Dpt->at(i), sample.Dy->at(i));
      if (bin < 0) continue;
      out[bin].push_back(MakeCandidate(sample, i));
      eventCounts[bin]++;
    }
  }
}

void ScanEventThresholds(SampleTree &data, SampleTree &control, Long64_t maxEvents, const std::string &outDir) {
  const std::vector<double> zdcPlus = {800, 900, 1000, 1100, 1200, 1300, 1500};
  const std::vector<double> zdcMinus = {800, 900, 1000, 1100, 1200, 1300, 1500};
  const std::vector<double> hfPlus = {5.5, 7.0, 8.6, 9.2, 11.0, 15.0};
  const std::vector<double> hfMinus = {5.5, 7.0, 8.6, 9.2, 11.0, 15.0};

  auto loadEvents = [&](SampleTree &s) {
    std::vector<EventSummary> rows;
    const Long64_t n = s.Entries(maxEvents);
    rows.reserve(static_cast<size_t>(n));
    for (Long64_t entry = 0; entry < n; ++entry) {
      s.tree->GetEntry(entry);
      rows.push_back({PassEventQuality(s), s.zdcPlus, s.zdcMinus, s.hfPlus, s.hfMinus});
    }
    return rows;
  };

  const std::vector<EventSummary> dataEvents = loadEvents(data);
  const std::vector<EventSummary> controlEvents = loadEvents(control);
  Long64_t dataQuality = 0;
  for (const auto &row : dataEvents) {
    if (row.quality) ++dataQuality;
  }
  const Long64_t controlRaw = static_cast<Long64_t>(controlEvents.size());

  std::ofstream zdcOut(outDir + "/zdc_threshold_scan.tsv");
  zdcOut << "plusThreshold\tminusThreshold\tdataQualityEvents\tdataXorEvents\tdataRetention\tcontrolRawEvents\tcontrolXorEvents\tcontrolFakeFraction\tscore\n";
  EventThresholdBest bestZdc;
  for (double p : zdcPlus) {
    for (double m : zdcMinus) {
      Long64_t dataPass = 0, controlPass = 0;
      for (const auto &row : dataEvents) {
        const bool gammaN = row.zdcMinus > m && row.zdcPlus < p;
        const bool Ngamma = row.zdcPlus > p && row.zdcMinus < m;
        if (row.quality && (gammaN || Ngamma)) ++dataPass;
      }
      for (const auto &row : controlEvents) {
        const bool gammaN = row.zdcMinus > m && row.zdcPlus < p;
        const bool Ngamma = row.zdcPlus > p && row.zdcMinus < m;
        if (gammaN || Ngamma) ++controlPass;
      }
      const double dataEff = dataQuality > 0 ? static_cast<double>(dataPass) / dataQuality : 0.0;
      const double fake = controlRaw > 0 ? static_cast<double>(controlPass) / controlRaw : 0.0;
      const double score = dataEff / std::sqrt(fake + (controlRaw > 0 ? 1.0 / controlRaw : 1.0));
      zdcOut << p << '\t' << m << '\t' << dataQuality << '\t' << dataPass << '\t' << dataEff << '\t'
             << controlRaw << '\t' << controlPass << '\t' << fake << '\t' << score << '\n';
      if (score > bestZdc.score) bestZdc = {score, p, m, dataEff, fake};
    }
  }

  std::ofstream hfOut(outDir + "/hf_gap_threshold_scan.tsv");
  hfOut << "hfPlusThreshold\thfMinusThreshold\tdataQualityEvents\tdataPassEvents\tdataRetention\tcontrolRawEvents\tcontrolPassEvents\tcontrolFakeFraction\tscore\n";
  EventThresholdBest bestHf;
  for (double p : hfPlus) {
    for (double m : hfMinus) {
      Long64_t dataPass = 0, controlPass = 0;
      for (const auto &row : dataEvents) {
        const bool gammaN = d0zdc::Pass0nXn(row.zdcPlus, row.zdcMinus);
        const bool Ngamma = d0zdc::PassXn0n(row.zdcPlus, row.zdcMinus);
        const bool gap = (gammaN && row.hfPlus < p) || (Ngamma && row.hfMinus < m);
        if (row.quality && gap) ++dataPass;
      }
      for (const auto &row : controlEvents) {
        const bool gammaN = d0zdc::Pass0nXn(row.zdcPlus, row.zdcMinus);
        const bool Ngamma = d0zdc::PassXn0n(row.zdcPlus, row.zdcMinus);
        const bool gap = (gammaN && row.hfPlus < p) || (Ngamma && row.hfMinus < m);
        if (gap) ++controlPass;
      }
      const double dataEff = dataQuality > 0 ? static_cast<double>(dataPass) / dataQuality : 0.0;
      const double fake = controlRaw > 0 ? static_cast<double>(controlPass) / controlRaw : 0.0;
      const double score = dataEff / std::sqrt(fake + (controlRaw > 0 ? 1.0 / controlRaw : 1.0));
      hfOut << p << '\t' << m << '\t' << dataQuality << '\t' << dataPass << '\t' << dataEff << '\t'
            << controlRaw << '\t' << controlPass << '\t' << fake << '\t' << score << '\n';
      if (score > bestHf.score) bestHf = {score, p, m, dataEff, fake};
    }
  }

  std::ofstream rec(outDir + "/recommended_event_thresholds.tsv");
  rec << "cut_family\tplus_or_hfPlus\tminus_or_hfMinus\tdataRetention\tcontrolFakeFraction\tscore\tnote\n";
  rec << "zdc_xor\t" << bestZdc.first << '\t' << bestZdc.second << '\t' << bestZdc.mcEff << '\t'
      << bestZdc.controlFake << '\t' << bestZdc.score << "\tprovisional score optimum; compare to nominal 1100/1000 GeV and ZDC peak fits\n";
  rec << "hf_gap\t" << bestHf.first << '\t' << bestHf.second << '\t' << bestHf.mcEff << '\t'
      << bestHf.controlFake << '\t' << bestHf.score << "\tprovisional score optimum; compare to nominal 9.2/8.6 GeV and plateau scans\n";
}

void ScanTopology(SampleTree &mc, SampleTree &data, Long64_t maxEvents, const std::string &outDir) {
  std::vector<std::vector<Candidate>> signal(Bins().size());
  std::vector<std::vector<Candidate>> background(Bins().size());
  std::vector<int> signalCount(Bins().size(), 0);
  std::vector<int> backgroundCount(Bins().size(), 0);

  CollectCandidates(mc, true, false, maxEvents, signal, signalCount);
  CollectCandidates(data, false, true, maxEvents, background, backgroundCount);

  const std::vector<double> alphaMax = {0.15, 0.20, 0.25, 0.30, 0.35, 0.40, 0.50};
  const std::vector<double> svpvMin = {1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 5.0};
  const std::vector<double> chi2Min = {0.02, 0.05, 0.10, 0.15, 0.20};
  const std::vector<double> dthetaMax = {0.20, 0.25, 0.30, 0.40, 0.50};

  std::ofstream scan(outDir + "/d_topology_scan.tsv");
  scan << "bin\talphaMax\tsvpvSigMin\tchi2clMin\tdthetaMax\tsignalBase\tbackgroundBase\tsignalPass\tbackgroundPass\tsignalEfficiency\tbackgroundFraction\tscore\n";
  std::vector<TopologyBest> best(Bins().size());

  for (size_t ib = 0; ib < Bins().size(); ++ib) {
    for (double a : alphaMax) {
      for (double s : svpvMin) {
        for (double c : chi2Min) {
          for (double d : dthetaMax) {
            int sigPass = 0, bkgPass = 0;
            for (const auto &cand : signal[ib]) {
              if (PassTopology(cand, a, s, c, d)) ++sigPass;
            }
            for (const auto &cand : background[ib]) {
              if (PassTopology(cand, a, s, c, d)) ++bkgPass;
            }
            const int sigBase = static_cast<int>(signal[ib].size());
            const int bkgBase = static_cast<int>(background[ib].size());
            const double sigEff = sigBase > 0 ? static_cast<double>(sigPass) / sigBase : 0.0;
            const double bkgFrac = bkgBase > 0 ? static_cast<double>(bkgPass) / bkgBase : 0.0;
            const double floor = bkgBase > 0 ? 1.0 / bkgBase : 1.0;
            const double score = sigEff / std::sqrt(bkgFrac + floor);
            scan << Bins()[ib].label << '\t' << a << '\t' << s << '\t' << c << '\t' << d << '\t'
                 << sigBase << '\t' << bkgBase << '\t' << sigPass << '\t' << bkgPass << '\t'
                 << sigEff << '\t' << bkgFrac << '\t' << score << '\n';
            if (score > best[ib].score && sigEff >= 0.20) {
              best[ib] = {score, a, s, c, d, sigEff, bkgFrac, sigBase, bkgBase, sigPass, bkgPass};
            }
          }
        }
      }
    }
  }

  std::ofstream rec(outDir + "/recommended_d_topological_cuts.tsv");
  rec << "bin\tptMin\tptMax\tabsYMin\tabsYMax\talphaMax\tsvpvSigMin\tchi2clMin\tdthetaMax\tsignalBase\tbackgroundBase\tsignalPass\tbackgroundPass\tsignalEfficiency\tbackgroundFraction\tscore\tnominalAlphaMax\tnominalSvpvSigMin\tnominalChi2clMin\tnominalDthetaMax\tnote\n";
  for (size_t ib = 0; ib < Bins().size(); ++ib) {
    const auto &bin = Bins()[ib];
    const auto &b = best[ib];
    const bool lowStats = b.sigBase < 20 || b.bkgBase < 20;
    rec << bin.label << '\t' << bin.ptMin << '\t' << bin.ptMax << '\t' << bin.absYMin << '\t' << bin.absYMax << '\t'
        << b.alpha << '\t' << b.svpv << '\t' << b.chi2 << '\t' << b.dtheta << '\t'
        << b.sigBase << '\t' << b.bkgBase << '\t' << b.sigPass << '\t' << b.bkgPass << '\t'
        << b.sigEff << '\t' << b.bkgFrac << '\t' << b.score << '\t'
        << bin.nominalAlphaMax << '\t' << bin.nominalSvpvSigMin << '\t' << bin.nominalChi2ClMin << '\t' << bin.nominalDthetaMax << '\t'
        << (lowStats ? "LOW_STATS_SMOKE_DO_NOT_FREEZE" : "provisional_score_optimum_compare_to_nominal") << '\n';
  }
}

void WriteReadme(const std::string &outDir, Long64_t maxEvents,
                 const char *signalPath, const char *dataPath, const char *controlPath) {
  std::ofstream out(outDir + "/README.md");
  out << "# 2023 D0 Cut Derivation Output\n\n";
  out << "This directory was produced by `derive_2023_d0_cuts.C`.\n\n";
  out << "## Inputs\n\n";
  out << "- signal MC: `" << signalPath << "`\n";
  out << "- data sidebands: `" << dataPath << "`\n";
  out << "- EBX/control: `" << controlPath << "`\n";
  out << "- max events per sample: `" << maxEvents << "`\n\n";
  out << "Topology background sidebands use: `" << gSidebandInner << " < |Dmass - 1.86484| < " << gSidebandOuter << "` GeV.\n";
  out << "If this is not the default `0.07-0.12` GeV window, the sideband source should be documented in the run output.\n\n";
  out << "## Outputs\n\n";
  out << "- `zdc_threshold_scan.tsv`: data retention and EBX fake rate versus ZDC XOR thresholds.\n";
  out << "- `hf_gap_threshold_scan.tsv`: data retention and EBX fake rate versus HF gap thresholds.\n";
  out << "- `d_topology_scan.tsv`: MC-signal/data-sideband scan over D topology cuts.\n";
  out << "- `recommended_event_thresholds.tsv`: provisional event-threshold score optima.\n";
  out << "- `recommended_d_topological_cuts.tsv`: provisional D-topology score optima per pT/y bin.\n";
  out << "- `cut_rationale.md`: short explanation of what each cut is doing.\n\n";
  out << "## Caveat\n\n";
  out << "Default bounded runs are smoke derivations. They validate the pipeline and give a first comparison to nominal cuts, but final analysis cuts require full-statistics scans, validation plots, and Evan/signoff review.\n";
}
}

int derive_2023_d0_topology_cuts_an_sideband(const char *signalPath,
                                             const char *dataPath,
                                             const char *controlPath,
                                             const char *outDir,
                                             Long64_t maxEvents = 200000,
                                             double sidebandInner = 0.07,
                                             double sidebandOuter = 0.12) {
  gSystem->mkdir(outDir, true);
  if (sidebandInner <= 0.0 || sidebandOuter <= sidebandInner || sidebandOuter > 0.25) {
    std::cerr << "Invalid sideband window: inner=" << sidebandInner << ", outer=" << sidebandOuter << "\n";
    return 1;
  }
  gSidebandInner = sidebandInner;
  gSidebandOuter = sidebandOuter;

  SampleTree signal, data, control;
  if (!signal.Open(signalPath, "Tree", "signal_mc")) return 1;
  if (!data.Open(dataPath, "Tree", "data_sideband")) return 1;
  if (!control.Open(controlPath, "Tree", "emptybx_control")) return 1;

  ScanEventThresholds(data, control, maxEvents, outDir);
  ScanTopology(signal, data, maxEvents, outDir);
  WriteRationale(outDir);
  WriteReadme(outDir, maxEvents, signalPath, dataPath, controlPath);

  std::cout << "Cut derivation outputs written to " << outDir << std::endl;
  return 0;
}
