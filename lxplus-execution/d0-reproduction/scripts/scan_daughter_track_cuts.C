#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "TFile.h"
#include "TSystem.h"
#include "TTree.h"

namespace {
constexpr double kD0Mass = 1.86484;

bool InFitRange(double mass) {
  return mass > 1.70 && mass < 2.05;
}

bool InSideband(double mass) {
  const double dm = std::abs(mass - kD0Mass);
  return dm > 0.07 && dm < 0.12;
}

bool InSignalWindow(double mass) {
  return std::abs(mass - kD0Mass) < 0.03;
}

struct SampleTree {
  std::unique_ptr<TFile> file;
  TTree *tree = nullptr;
  Bool_t selectedVtxFilter = true;
  Bool_t clusterCompatibilityFilter = true;
  int nVtx = 1;
  Float_t zdcPlus = 0;
  Float_t zdcMinus = 0;
  Float_t hfPlus = 0;
  Float_t hfMinus = 0;
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
  std::vector<bool> *DisSignalCalcPrompt = nullptr;
  std::vector<bool> *DisSignalCalc = nullptr;
  std::vector<int> *Dgen = nullptr;
  bool hasSelectedVtx = false;
  bool hasCluster = false;
  bool hasNVtx = false;

  bool Has(const char *name) const {
    return tree && tree->GetBranch(name);
  }

  template <typename T>
  void BindScalar(const char *name, T *ptr, bool *flag = nullptr) {
    if (Has(name)) {
      tree->SetBranchAddress(name, ptr);
      if (flag) *flag = true;
    } else if (flag) {
      *flag = false;
    }
  }

  template <typename T>
  void BindVector(const char *name, std::vector<T> **ptr) {
    if (Has(name)) tree->SetBranchAddress(name, ptr);
  }

  bool Open(const char *path) {
    file.reset(TFile::Open(path, "READ"));
    if (!file || file->IsZombie()) {
      std::cerr << "Could not open " << path << "\n";
      return false;
    }
    file->GetObject("Tree", tree);
    if (!tree) {
      std::cerr << "Could not find reduced Tree in " << path << "\n";
      return false;
    }
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
    BindVector("DisSignalCalcPrompt", &DisSignalCalcPrompt);
    BindVector("DisSignalCalc", &DisSignalCalc);
    BindVector("Dgen", &Dgen);
    return Dpt && Dy && Dmass && Dtrk1Pt && Dtrk2Pt && Dtrk1PtErr &&
           Dtrk2PtErr && Dtrk1Eta && Dtrk2Eta;
  }

  Long64_t Entries(Long64_t maxEntries) const {
    const Long64_t n = tree ? tree->GetEntries() : 0;
    return maxEntries >= 0 ? std::min(n, maxEntries) : n;
  }

  size_t CandidateCount() const {
    size_t n = Dmass ? Dmass->size() : 0;
    n = std::min(n, Dpt ? Dpt->size() : 0);
    n = std::min(n, Dy ? Dy->size() : 0);
    n = std::min(n, Dtrk1Pt ? Dtrk1Pt->size() : 0);
    n = std::min(n, Dtrk2Pt ? Dtrk2Pt->size() : 0);
    n = std::min(n, Dtrk1PtErr ? Dtrk1PtErr->size() : 0);
    n = std::min(n, Dtrk2PtErr ? Dtrk2PtErr->size() : 0);
    n = std::min(n, Dtrk1Eta ? Dtrk1Eta->size() : 0);
    n = std::min(n, Dtrk2Eta ? Dtrk2Eta->size() : 0);
    return n;
  }
};

bool PassEventQuality(const SampleTree &s) {
  if (s.hasSelectedVtx && !s.selectedVtxFilter) return false;
  if (s.hasCluster && !s.clusterCompatibilityFilter) return false;
  if (s.hasNVtx && (s.nVtx <= 0 || s.nVtx > 3)) return false;
  return true;
}

bool PassDataEventTopology(const SampleTree &s) {
  if (!PassEventQuality(s)) return false;
  const bool gammaN = s.zdcMinus > 1000.0 && s.zdcPlus < 1100.0;
  const bool Ngamma = s.zdcPlus > 1100.0 && s.zdcMinus < 1000.0;
  return (gammaN && s.hfPlus < 9.2) || (Ngamma && s.hfMinus < 8.6);
}

bool TruthMatched(const SampleTree &s, size_t i) {
  if (s.DisSignalCalcPrompt && i < s.DisSignalCalcPrompt->size()) return s.DisSignalCalcPrompt->at(i);
  if (s.DisSignalCalc && i < s.DisSignalCalc->size()) return s.DisSignalCalc->at(i);
  if (s.Dgen && i < s.Dgen->size()) return s.Dgen->at(i) != 0;
  return false;
}

bool PassFiducial(const SampleTree &s, size_t i) {
  const double pt = s.Dpt->at(i);
  const double y = s.Dy->at(i);
  return pt > 2.0 && pt <= 12.0 && std::abs(y) <= 2.0 && InFitRange(s.Dmass->at(i));
}

bool PassTrackCuts(const SampleTree &s, size_t i, double ptMin, double etaMax, double relErrMax) {
  const double p1 = s.Dtrk1Pt->at(i);
  const double p2 = s.Dtrk2Pt->at(i);
  if (p1 <= ptMin || p2 <= ptMin) return false;
  if (std::abs(s.Dtrk1Eta->at(i)) >= etaMax || std::abs(s.Dtrk2Eta->at(i)) >= etaMax) return false;
  if (p1 <= 0 || p2 <= 0) return false;
  if (s.Dtrk1PtErr->at(i) / p1 >= relErrMax) return false;
  if (s.Dtrk2PtErr->at(i) / p2 >= relErrMax) return false;
  return true;
}

struct Counts {
  long long signalBase = 0;
  long long signalPass = 0;
  long long dataSidebandBase = 0;
  long long dataSidebandPass = 0;
  long long dataSignalWindowBase = 0;
  long long dataSignalWindowPass = 0;
};

void FillCounts(SampleTree &sample, bool isSignalMc, Long64_t maxEntries,
                double ptMin, double etaMax, double relErrMax, Counts &out) {
  const Long64_t n = sample.Entries(maxEntries);
  for (Long64_t entry = 0; entry < n; ++entry) {
    sample.tree->GetEntry(entry);
    if (isSignalMc) {
      if (!PassEventQuality(sample)) continue;
    } else {
      if (!PassDataEventTopology(sample)) continue;
    }
    const size_t nc = sample.CandidateCount();
    for (size_t i = 0; i < nc; ++i) {
      if (!PassFiducial(sample, i)) continue;
      const double mass = sample.Dmass->at(i);
      const bool pass = PassTrackCuts(sample, i, ptMin, etaMax, relErrMax);
      if (isSignalMc) {
        if (!TruthMatched(sample, i)) continue;
        ++out.signalBase;
        if (pass) ++out.signalPass;
      } else {
        if (InSideband(mass)) {
          ++out.dataSidebandBase;
          if (pass) ++out.dataSidebandPass;
        }
        if (InSignalWindow(mass)) {
          ++out.dataSignalWindowBase;
          if (pass) ++out.dataSignalWindowPass;
        }
      }
    }
  }
}

double frac(long long pass, long long base) {
  return base > 0 ? static_cast<double>(pass) / static_cast<double>(base) : 0.0;
}

void WriteRow(std::ofstream &out, const char *family, const char *label,
              double ptMin, double etaMax, double relErrMax, const Counts &c) {
  out << family << '\t' << label << '\t' << ptMin << '\t' << etaMax << '\t' << relErrMax << '\t'
      << c.signalBase << '\t' << c.signalPass << '\t' << frac(c.signalPass, c.signalBase) << '\t'
      << c.dataSidebandBase << '\t' << c.dataSidebandPass << '\t'
      << frac(c.dataSidebandPass, c.dataSidebandBase) << '\t'
      << (1.0 - frac(c.dataSidebandPass, c.dataSidebandBase)) << '\t'
      << c.dataSignalWindowBase << '\t' << c.dataSignalWindowPass << '\t'
      << frac(c.dataSignalWindowPass, c.dataSignalWindowBase) << '\n';
}

struct ScanPoint {
  std::string family;
  std::string label;
  double ptMin;
  double etaMax;
  double relErrMax;
};

void FillAllScans(SampleTree &sample, bool isSignalMc, Long64_t maxEntries,
                  const std::vector<ScanPoint> &points, std::vector<Counts> &counts) {
  const Long64_t n = sample.Entries(maxEntries);
  for (Long64_t entry = 0; entry < n; ++entry) {
    sample.tree->GetEntry(entry);
    if (isSignalMc) {
      if (!PassEventQuality(sample)) continue;
    } else {
      if (!PassDataEventTopology(sample)) continue;
    }
    const size_t nc = sample.CandidateCount();
    for (size_t i = 0; i < nc; ++i) {
      if (!PassFiducial(sample, i)) continue;
      const double mass = sample.Dmass->at(i);
      const bool truth = isSignalMc && TruthMatched(sample, i);
      const bool sideband = !isSignalMc && InSideband(mass);
      const bool signalWindow = !isSignalMc && InSignalWindow(mass);
      if (!truth && !sideband && !signalWindow) continue;
      for (size_t j = 0; j < points.size(); ++j) {
        const bool pass = PassTrackCuts(sample, i, points[j].ptMin, points[j].etaMax, points[j].relErrMax);
        if (truth) {
          ++counts[j].signalBase;
          if (pass) ++counts[j].signalPass;
        }
        if (sideband) {
          ++counts[j].dataSidebandBase;
          if (pass) ++counts[j].dataSidebandPass;
        }
        if (signalWindow) {
          ++counts[j].dataSignalWindowBase;
          if (pass) ++counts[j].dataSignalWindowPass;
        }
      }
    }
  }
}
}

int scan_daughter_track_cuts(const char *mcPath,
                             const char *dataPath,
                             const char *outDir,
                             Long64_t maxEntries = 2000000) {
  gSystem->mkdir(outDir, true);
  SampleTree mc, data;
  if (!mc.Open(mcPath) || !data.Open(dataPath)) return 1;

  const std::vector<double> ptCuts = {0.5, 0.7, 0.9, 1.0, 1.1, 1.3, 1.5};
  const std::vector<double> etaCuts = {1.8, 2.0, 2.2, 2.4, 2.6};
  const std::vector<double> errCuts = {0.03, 0.05, 0.08, 0.10, 0.15, 0.20, 0.30};

  std::ofstream out(std::string(outDir) + "/daughter_track_cut_scan.tsv");
  out << "family\tlabel\tptMin\tetaMax\trelPtErrMax\tsignalBase\tsignalPass\tsignalRetention\t"
      << "dataSidebandBase\tdataSidebandPass\tdataSidebandPassFraction\tdataSidebandRejection\t"
      << "dataSignalWindowBase\tdataSignalWindowPass\tdataSignalWindowPassFraction\n";

  std::vector<ScanPoint> points;
  for (double pt : ptCuts) {
    points.push_back({"pt_min", pt == 1.0 ? "nominal" : "scan", pt, 2.4, 0.10});
  }
  for (double eta : etaCuts) {
    points.push_back({"eta_max", eta == 2.4 ? "nominal" : "scan", 1.0, eta, 0.10});
  }
  for (double err : errCuts) {
    points.push_back({"rel_pt_err_max", err == 0.10 ? "nominal" : "scan", 1.0, 2.4, err});
  }
  std::vector<Counts> counts(points.size());
  FillAllScans(mc, true, maxEntries, points, counts);
  FillAllScans(data, false, maxEntries, points, counts);
  for (size_t i = 0; i < points.size(); ++i) {
    WriteRow(out, points[i].family.c_str(), points[i].label.c_str(),
             points[i].ptMin, points[i].etaMax, points[i].relErrMax, counts[i]);
  }

  std::ofstream readme(std::string(outDir) + "/README.md");
  readme << "# Daughter Track Cut Scan\n\n";
  readme << "<!-- DOC_OWNER: cms-analysis-manager daughter-track cut justification. -->\n";
  readme << "<!-- TOKEN_NOTE: exploratory scan; AN values are posthoc labels only. -->\n\n";
  readme << "- Signal proxy: truth-matched prompt D0 MC after event topology, fit range, and D0 fiducial cuts.\n";
  readme << "- Background proxy: 2023 data sidebands after event topology, fit range, and D0 fiducial cuts.\n";
  readme << "- Signal-window data is included only as a mass-shape sanity check, not as an optimizer.\n";
  readme << "- Scan families vary one daughter-track cut at a time while keeping the other two at nominal values.\n";
  readme << "- Nominal markers: `pt > 1.0 GeV`, `|eta| < 2.4`, and `ptErr/pt < 0.10`.\n";
  readme << "- Max entries per sample: `" << maxEntries << "`.\n\n";
  readme << "Output: `daughter_track_cut_scan.tsv`.\n";

  std::cout << "wrote " << outDir << "/daughter_track_cut_scan.tsv\n";
  return 0;
}
