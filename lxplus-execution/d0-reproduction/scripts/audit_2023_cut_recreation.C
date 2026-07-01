#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TAxis.h"
#include "TSystem.h"

namespace {
struct DCutBin {
  const char *label;
  float absYMin;
  float absYMax;
  float ptMin;
  float ptMax;
  float alphaMax;
  float svpvSigMin;
  float vertexProbMin;
  float dthetaMax;
};

const std::vector<DCutBin> &CutBins() {
  static const std::vector<DCutBin> bins = {
      {"absy0to1_pt2to5", 0.0f, 1.0f, 2.0f, 5.0f, 0.4f, 2.5f, 0.1f, 0.5f},
      {"absy0to1_pt5to8", 0.0f, 1.0f, 5.0f, 8.0f, 0.35f, 3.5f, 0.1f, 0.3f},
      {"absy0to1_pt8to12", 0.0f, 1.0f, 8.0f, 12.0f, 0.4f, 3.5f, 0.1f, 0.3f},
      {"absy1to2_pt2to5", 1.0f, 2.0f, 2.0f, 5.0f, 0.2f, 2.5f, 0.1f, 0.3f},
      {"absy1to2_pt5to8", 1.0f, 2.0f, 5.0f, 8.0f, 0.25f, 3.5f, 0.1f, 0.3f},
      {"absy1to2_pt8to12", 1.0f, 2.0f, 8.0f, 12.0f, 0.25f, 3.5f, 0.1f, 0.3f},
  };
  return bins;
}

TH1 *GetH1(TFile &file, const char *name) {
  TH1 *hist = nullptr;
  file.GetObject(name, hist);
  if (!hist) std::cerr << "Missing histogram: " << name << std::endl;
  return hist;
}

TH2 *GetH2(TFile &file, const char *name) {
  TH2 *hist = nullptr;
  file.GetObject(name, hist);
  if (!hist) std::cerr << "Missing histogram: " << name << std::endl;
  return hist;
}

void WriteCutFlow(TH1 *hist, const std::string &path) {
  std::ofstream out(path);
  out << "bin\tstage\tcount\n";
  if (!hist) return;
  for (int i = 1; i <= hist->GetNbinsX(); ++i) {
    std::string label = hist->GetXaxis()->GetBinLabel(i);
    if (label.empty()) label = "bin_" + std::to_string(i);
    out << i << '\t' << label << '\t' << std::setprecision(12) << hist->GetBinContent(i) << '\n';
  }
}

double IntegralBelow(TH1 *hist, double threshold) {
  if (!hist) return 0.0;
  double total = 0.0;
  for (int i = 1; i <= hist->GetNbinsX(); ++i) {
    if (hist->GetXaxis()->GetBinCenter(i) < threshold) total += hist->GetBinContent(i);
  }
  return total;
}

double IntegralAbove(TH1 *hist, double threshold) {
  if (!hist) return 0.0;
  double total = 0.0;
  for (int i = 1; i <= hist->GetNbinsX(); ++i) {
    if (hist->GetXaxis()->GetBinCenter(i) > threshold) total += hist->GetBinContent(i);
  }
  return total;
}

double ZdcXorCount(TH2 *hist, double plusThreshold, double minusThreshold) {
  if (!hist) return 0.0;
  double count = 0.0;
  for (int ix = 1; ix <= hist->GetNbinsX(); ++ix) {
    const double plus = hist->GetXaxis()->GetBinCenter(ix);
    for (int iy = 1; iy <= hist->GetNbinsY(); ++iy) {
      const double minus = hist->GetYaxis()->GetBinCenter(iy);
      const bool gammaN = (minus > minusThreshold && plus < plusThreshold);
      const bool Ngamma = (plus > plusThreshold && minus < minusThreshold);
      if (gammaN || Ngamma) count += hist->GetBinContent(ix, iy);
    }
  }
  return count;
}

void WriteEventCuts(const std::string &path) {
  std::ofstream out(path);
  out << "cut_id\tstage\tnominal\trecreation_method\n";
  out << "evt_pv_filter\tprimary_vertex_filter\tpprimaryVertexFilter == 1\tstandard filter; validate cutflow and signal efficiency\n";
  out << "evt_vz\tprimary_vertex_z\tabs(bestVz) < 15 cm\tstandard good-vertex window; validate before/after distribution\n";
  out << "evt_nvtx\tvertex_multiplicity\t1 <= nVtx <= 3 in code\tvalidate AN wording versus code and cutflow\n";
  out << "evt_cluster\tcluster_compatibility\tpclusterCompatibilityFilter == 1\tstandard UPC/hadronic rejection filter\n";
  out << "evt_csc_halo\tcsc_halo_filter\tcscTightHalo2015Filter == true\tstandard halo rejection filter\n";
  out << "evt_zdc_plus\tzdc_plus_1n_threshold\t1100 GeV\tfit 1n peak and EBX noise tail\n";
  out << "evt_zdc_minus\tzdc_minus_1n_threshold\t1000 GeV\tfit 1n peak and EBX noise tail\n";
  out << "evt_hf_plus\thf_plus_gap_threshold\t9.2 GeV\tEBX noise plus MC/data plateau threshold scan\n";
  out << "evt_hf_minus\thf_minus_gap_threshold\t8.6 GeV\tEBX noise plus MC/data plateau threshold scan\n";
}

void WriteDNominalCuts(const std::string &path) {
  std::ofstream out(path);
  out << "label\tabsYMin\tabsYMax\tptMin\tptMax\talphaMax\tsvpvSigMin\tvertexProbMin\tdthetaMax\n";
  for (const auto &bin : CutBins()) {
    out << bin.label << '\t' << bin.absYMin << '\t' << bin.absYMax << '\t'
        << bin.ptMin << '\t' << bin.ptMax << '\t' << bin.alphaMax << '\t'
        << bin.svpvSigMin << '\t' << bin.vertexProbMin << '\t' << bin.dthetaMax << '\n';
  }
}

void WriteZdcScan(TH1 *plus, TH1 *minus, TH2 *plusMinus, const std::string &path) {
  std::ofstream out(path);
  out << "plusThreshold\tminusThreshold\tplusAbove\tminusAbove\txorCount\n";
  const std::vector<std::pair<double, double>> thresholds = {
      {900.0, 900.0}, {1000.0, 1000.0}, {1100.0, 1000.0},
      {1100.0, 1100.0}, {1200.0, 1200.0}, {1500.0, 1500.0}};
  for (const auto &item : thresholds) {
    out << item.first << '\t' << item.second << '\t'
        << IntegralAbove(plus, item.first) << '\t'
        << IntegralAbove(minus, item.second) << '\t'
        << ZdcXorCount(plusMinus, item.first, item.second) << '\n';
  }
}

void WriteHfScan(TH1 *plus, TH1 *minus, const std::string &path) {
  std::ofstream out(path);
  out << "threshold\tplusBelow\tplusTotal\tplusPassFraction\tminusBelow\tminusTotal\tminusPassFraction\n";
  const std::vector<double> thresholds = {5.5, 8.6, 9.2, 15.0};
  const double plusTotal = plus ? plus->Integral(1, plus->GetNbinsX()) : 0.0;
  const double minusTotal = minus ? minus->Integral(1, minus->GetNbinsX()) : 0.0;
  for (double threshold : thresholds) {
    const double plusBelow = IntegralBelow(plus, threshold);
    const double minusBelow = IntegralBelow(minus, threshold);
    out << threshold << '\t'
        << plusBelow << '\t' << plusTotal << '\t' << (plusTotal > 0 ? plusBelow / plusTotal : 0.0) << '\t'
        << minusBelow << '\t' << minusTotal << '\t' << (minusTotal > 0 ? minusBelow / minusTotal : 0.0) << '\n';
  }
}
}

int audit_2023_cut_recreation(const char *section5Path,
                              const char *section7Path,
                              const char *outDir) {
  gSystem->mkdir(outDir, true);

  TFile section5(section5Path, "READ");
  if (section5.IsZombie()) {
    std::cerr << "Could not open Section 5 ROOT file: " << section5Path << std::endl;
    return 1;
  }
  TFile section7(section7Path, "READ");
  if (section7.IsZombie()) {
    std::cerr << "Could not open Section 7 ROOT file: " << section7Path << std::endl;
    return 1;
  }

  WriteEventCuts(std::string(outDir) + "/nominal_event_cuts.tsv");
  WriteDNominalCuts(std::string(outDir) + "/nominal_d_topological_cuts.tsv");
  WriteCutFlow(GetH1(section5, "hEventCutFlow"), std::string(outDir) + "/section5_event_cutflow.tsv");
  WriteCutFlow(GetH1(section7, "hDCutFlow"), std::string(outDir) + "/section7_d_cutflow.tsv");
  WriteZdcScan(GetH1(section5, "hZDCPlus_before_cut"),
               GetH1(section5, "hZDCMinus_before_cut"),
               GetH2(section5, "hZDCPlusMinus_before_cut"),
               std::string(outDir) + "/zdc_threshold_scan.tsv");
  WriteHfScan(GetH1(section5, "hHFEMaxPlus_before_gap_cut"),
              GetH1(section5, "hHFEMaxMinus_before_gap_cut"),
              std::string(outDir) + "/hf_gap_threshold_scan.tsv");

  std::ofstream manifest(std::string(outDir) + "/README.md");
  manifest << "# 2023 D0 Cut Recreation Audit\n\n";
  manifest << "Inputs:\n\n";
  manifest << "- Section 5 ROOT: `" << section5Path << "`\n";
  manifest << "- Section 7 ROOT: `" << section7Path << "`\n\n";
  manifest << "Outputs:\n\n";
  manifest << "- `nominal_event_cuts.tsv`\n";
  manifest << "- `nominal_d_topological_cuts.tsv`\n";
  manifest << "- `section5_event_cutflow.tsv`\n";
  manifest << "- `section7_d_cutflow.tsv`\n";
  manifest << "- `zdc_threshold_scan.tsv`\n";
  manifest << "- `hf_gap_threshold_scan.tsv`\n\n";
  manifest << "Scope:\n\n";
  manifest << "This is the first audit layer. It makes the current copied cuts and threshold scans reproducible from the selected-skim outputs. It does not yet replace EBX/MC-backed derivations.\n";

  std::cout << "Wrote cut recreation audit to " << outDir << std::endl;
  return 0;
}
