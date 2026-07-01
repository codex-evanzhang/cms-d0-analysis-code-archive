#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1D.h"
#include "TLegend.h"
#include "TLine.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"

namespace {
constexpr double NOM_ZDC_PLUS = 1100.0;
constexpr double NOM_ZDC_MINUS = 1000.0;
constexpr double NOM_HF_PLUS = 9.2;
constexpr double NOM_HF_MINUS = 8.6;

struct Sample {
  std::string label;
  std::string path;
  TFile *file = nullptr;
  TTree *tree = nullptr;

  Bool_t selectedVtxFilter = true;
  Bool_t clusterCompatibilityFilter = true;
  Bool_t zdcGammaN = false;
  Bool_t zdcNgamma = false;
  Int_t nVtx = 1;
  Float_t zdcPlus = 0.0;
  Float_t zdcMinus = 0.0;
  Float_t hfPlus = 0.0;
  Float_t hfMinus = 0.0;

  bool hasSelectedVtx = false;
  bool hasCluster = false;
  bool hasNVtx = false;
  bool hasZdcLabels = false;

  bool Open(const char *sampleLabel, const char *samplePath) {
    label = sampleLabel;
    path = samplePath;
    file = TFile::Open(samplePath, "READ");
    if (!file || file->IsZombie()) {
      std::cerr << "Could not open " << samplePath << "\n";
      return false;
    }
    file->GetObject("Tree", tree);
    if (!tree) {
      std::cerr << "Could not find Tree in " << samplePath << "\n";
      return false;
    }
    tree->SetBranchStatus("*", 0);
    Bind("selectedVtxFilter", &selectedVtxFilter, hasSelectedVtx);
    Bind("ClusterCompatibilityFilter", &clusterCompatibilityFilter, hasCluster);
    Bind("nVtx", &nVtx, hasNVtx);
    bool hasGammaN = false, hasNgamma = false;
    Bind("ZDCgammaN", &zdcGammaN, hasGammaN);
    Bind("ZDCNgamma", &zdcNgamma, hasNgamma);
    hasZdcLabels = hasGammaN && hasNgamma;
    bool dummy = false;
    Bind("ZDCsumPlus", &zdcPlus, dummy);
    Bind("ZDCsumMinus", &zdcMinus, dummy);
    Bind("HFEMaxPlus", &hfPlus, dummy);
    Bind("HFEMaxMinus", &hfMinus, dummy);
    return true;
  }

  template <typename T>
  void Bind(const char *name, T *value, bool &flag) {
    if (tree->GetBranch(name)) {
      tree->SetBranchStatus(name, 1);
      tree->SetBranchAddress(name, value);
      flag = true;
    }
  }

  bool Quality() const {
    if (hasSelectedVtx && !selectedVtxFilter) return false;
    if (hasCluster && !clusterCompatibilityFilter) return false;
    if (hasNVtx && (nVtx <= 0 || nVtx > 3)) return false;
    return true;
  }

  bool ValidDetectorValues() const {
    return std::isfinite(zdcPlus) && std::isfinite(zdcMinus) &&
           std::isfinite(hfPlus) && std::isfinite(hfMinus) &&
           zdcPlus > -9000.0 && zdcMinus > -9000.0 &&
           hfPlus > -9000.0 && hfMinus > -9000.0;
  }
};

struct Dist {
  std::vector<double> zdcPlusAll;
  std::vector<double> zdcMinusAll;
  std::vector<double> hfPlusAll;
  std::vector<double> hfMinusAll;

  std::vector<double> plusLow;
  std::vector<double> plusHigh;
  std::vector<double> minusLow;
  std::vector<double> minusHigh;
  std::vector<double> hfGapPlus;
  std::vector<double> hfGapMinus;

  Long64_t raw = 0;
  Long64_t quality = 0;
  Long64_t valid = 0;
  Long64_t gammaN = 0;
  Long64_t Ngamma = 0;
};

struct ThresholdResult {
  double threshold = -1.0;
  double score = -1.0;
  double lowEff = 0.0;
  double highEff = 0.0;
  double controlFake = 0.0;
  size_t lowN = 0;
  size_t highN = 0;
  size_t controlN = 0;
  std::string note;
};

bool PassNominalZdc(double plus, double minus) {
  return (minus > NOM_ZDC_MINUS && plus < NOM_ZDC_PLUS) ||
         (plus > NOM_ZDC_PLUS && minus < NOM_ZDC_MINUS);
}

bool PassNominalHf(double plus, double minus, double hfPlus, double hfMinus) {
  const bool gammaN = minus > NOM_ZDC_MINUS && plus < NOM_ZDC_PLUS;
  const bool Ngamma = plus > NOM_ZDC_PLUS && minus < NOM_ZDC_MINUS;
  return (gammaN && hfPlus < NOM_HF_PLUS) || (Ngamma && hfMinus < NOM_HF_MINUS);
}

double Quantile(std::vector<double> values, double q) {
  if (values.empty()) return std::numeric_limits<double>::quiet_NaN();
  std::sort(values.begin(), values.end());
  const double pos = std::clamp(q, 0.0, 1.0) * (values.size() - 1);
  const size_t lo = static_cast<size_t>(std::floor(pos));
  const size_t hi = static_cast<size_t>(std::ceil(pos));
  if (lo == hi) return values[lo];
  const double frac = pos - lo;
  return values[lo] * (1.0 - frac) + values[hi] * frac;
}

double FractionBelow(const std::vector<double> &values, double threshold) {
  if (values.empty()) return 0.0;
  return static_cast<double>(std::count_if(values.begin(), values.end(), [&](double v) { return v < threshold; })) /
         static_cast<double>(values.size());
}

double FractionAbove(const std::vector<double> &values, double threshold) {
  if (values.empty()) return 0.0;
  return static_cast<double>(std::count_if(values.begin(), values.end(), [&](double v) { return v > threshold; })) /
         static_cast<double>(values.size());
}

void Append(std::vector<double> &dst, const std::vector<double> &src) {
  dst.insert(dst.end(), src.begin(), src.end());
}

Dist Collect(Sample &sample, Long64_t maxEvents) {
  Dist out;
  const Long64_t entries = sample.tree->GetEntries();
  const Long64_t n = maxEvents >= 0 ? std::min(entries, maxEvents) : entries;
  out.raw = n;
  out.zdcPlusAll.reserve(static_cast<size_t>(std::min<Long64_t>(n, 1000000)));
  out.zdcMinusAll.reserve(out.zdcPlusAll.capacity());

  for (Long64_t entry = 0; entry < n; ++entry) {
    sample.tree->GetEntry(entry);
    if (!sample.Quality()) continue;
    ++out.quality;
    if (!sample.ValidDetectorValues()) continue;
    ++out.valid;

    out.zdcPlusAll.push_back(sample.zdcPlus);
    out.zdcMinusAll.push_back(sample.zdcMinus);
    out.hfPlusAll.push_back(sample.hfPlus);
    out.hfMinusAll.push_back(sample.hfMinus);

    if (!sample.hasZdcLabels) continue;
    const bool gammaN = sample.zdcGammaN && !sample.zdcNgamma;
    const bool Ngamma = sample.zdcNgamma && !sample.zdcGammaN;
    if (gammaN) {
      ++out.gammaN;
      out.plusLow.push_back(sample.zdcPlus);
      out.minusHigh.push_back(sample.zdcMinus);
      out.hfGapPlus.push_back(sample.hfPlus);
    } else if (Ngamma) {
      ++out.Ngamma;
      out.plusHigh.push_back(sample.zdcPlus);
      out.minusLow.push_back(sample.zdcMinus);
      out.hfGapMinus.push_back(sample.hfMinus);
    }
  }
  return out;
}

ThresholdResult ChooseZdcThreshold(const std::vector<double> &low,
                                   const std::vector<double> &high,
                                   const std::vector<double> &control,
                                   double minThr,
                                   double maxThr,
                                   double step) {
  ThresholdResult best;
  best.lowN = low.size();
  best.highN = high.size();
  best.controlN = control.size();
  if (low.empty() || high.empty()) {
    best.note = "insufficient low/high samples for MC separation";
    return best;
  }
  for (double t = minThr; t <= maxThr + 1e-9; t += step) {
    const double lowEff = FractionBelow(low, t);
    const double highEff = FractionAbove(high, t);
    const double fake = FractionAbove(control, t);
    const double score = 0.5 * (lowEff + highEff) - 0.05 * fake;
    if (score > best.score) {
      best.threshold = t;
      best.score = score;
      best.lowEff = lowEff;
      best.highEff = highEff;
      best.controlFake = fake;
    }
  }
  best.note = "balanced low-side/high-side separation with EmptyBX penalty";
  return best;
}

ThresholdResult ChooseHfThreshold(const std::vector<double> &signalGap,
                                  const std::vector<double> &dataGap,
                                  double minThr,
                                  double maxThr,
                                  double step,
                                  double targetSignalEff) {
  ThresholdResult best;
  best.lowN = signalGap.size();
  best.highN = dataGap.size();
  if (signalGap.empty()) {
    best.note = "insufficient MC gap-side HF sample";
    return best;
  }

  ThresholdResult fallback;
  fallback.score = -1.0;
  for (double t = minThr; t <= maxThr + 1e-9; t += step) {
    const double sigEff = FractionBelow(signalGap, t);
    const double dataPass = dataGap.empty() ? 0.0 : FractionBelow(dataGap, t);
    const double score = sigEff / std::sqrt(dataPass + (dataGap.empty() ? 1.0 : 1.0 / dataGap.size()));
    if (score > fallback.score) {
      fallback.threshold = t;
      fallback.score = score;
      fallback.lowEff = sigEff;
      fallback.highEff = dataPass;
    }
    if (sigEff >= targetSignalEff) {
      best.threshold = t;
      best.score = score;
      best.lowEff = sigEff;
      best.highEff = dataPass;
      best.note = "smallest threshold reaching target MC signal efficiency";
      return best;
    }
  }
  fallback.note = "target MC signal efficiency not reached; best score returned";
  return fallback;
}

TH1D *Hist(const char *name, const char *title, const std::vector<double> &values, int bins, double min, double max) {
  TH1D *hist = new TH1D(name, title, bins, min, max);
  hist->SetDirectory(nullptr);
  for (double value : values) {
    if (std::isfinite(value)) hist->Fill(value);
  }
  if (hist->Integral() > 0) hist->Scale(1.0 / hist->Integral());
  return hist;
}

void SaveOverlay(const std::string &outDir,
                 const std::string &name,
                 const std::string &title,
                 const std::vector<double> &a,
                 const char *aLabel,
                 const std::vector<double> &b,
                 const char *bLabel,
                 const std::vector<double> &c,
                 const char *cLabel,
                 int bins,
                 double min,
                 double max,
                 double recommended,
                 double nominal) {
  TCanvas canvas(("c_" + name).c_str(), "", 900, 650);
  canvas.SetLogy();
  TH1D *ha = Hist(("h_" + name + "_a").c_str(), title.c_str(), a, bins, min, max);
  TH1D *hb = Hist(("h_" + name + "_b").c_str(), title.c_str(), b, bins, min, max);
  TH1D *hc = Hist(("h_" + name + "_c").c_str(), title.c_str(), c, bins, min, max);
  ha->SetLineColor(kBlue + 1);
  hb->SetLineColor(kRed + 1);
  hc->SetLineColor(kGreen + 2);
  ha->SetLineWidth(2);
  hb->SetLineWidth(2);
  hc->SetLineWidth(2);
  ha->GetYaxis()->SetRangeUser(1e-6, 1.0);
  ha->Draw("hist");
  hb->Draw("hist same");
  hc->Draw("hist same");
  TLegend leg(0.56, 0.68, 0.88, 0.88);
  leg.AddEntry(ha, aLabel, "l");
  leg.AddEntry(hb, bLabel, "l");
  leg.AddEntry(hc, cLabel, "l");
  if (recommended > 0) {
    TLine *line = new TLine(recommended, 1e-6, recommended, 1.0);
    line->SetLineColor(kMagenta + 2);
    line->SetLineWidth(2);
    line->Draw("same");
    leg.AddEntry(line, "recommended", "l");
  }
  if (nominal > 0) {
    TLine *line = new TLine(nominal, 1e-6, nominal, 1.0);
    line->SetLineColor(kBlack);
    line->SetLineStyle(2);
    line->SetLineWidth(2);
    line->Draw("same");
    leg.AddEntry(line, "nominal", "l");
  }
  leg.Draw();
  canvas.SaveAs((outDir + "/" + name + ".png").c_str());
}

void WriteSummary(const std::string &outDir,
                  const Dist &mc,
                  const Dist &data,
                  const Dist &empty,
                  const ThresholdResult &zdcPlus,
                  const ThresholdResult &zdcMinus,
                  const ThresholdResult &hfPlus,
                  const ThresholdResult &hfMinus,
                  Long64_t maxEvents,
                  const char *mcPath,
                  const char *dataPath,
                  const char *emptyPath) {
  std::ofstream out(outDir + "/README.md");
  out << "# MC-Based ZDC/HF Cutoff Scan\n\n";
  out << "This scan uses the official reconstructed prompt UPC PbPb D0 MC skim as the signal-like detector-response sample. "
      << "It does not use the old pp Pythia smoke sample for ZDC/HF.\n\n";
  out << "- Max entries per sample: `" << maxEvents << "`\n";
  out << "- Prompt MC: `" << mcPath << "`\n";
  out << "- 2023 data: `" << dataPath << "`\n";
  out << "- EmptyBX: `" << emptyPath << "`\n\n";
  out << "## Sample Counts\n\n";
  out << "| sample | raw scanned | quality | valid detector values | gammaN | Ngamma |\n";
  out << "|---|---:|---:|---:|---:|---:|\n";
  out << "| prompt MC | " << mc.raw << " | " << mc.quality << " | " << mc.valid << " | " << mc.gammaN << " | " << mc.Ngamma << " |\n";
  out << "| data | " << data.raw << " | " << data.quality << " | " << data.valid << " | " << data.gammaN << " | " << data.Ngamma << " |\n";
  out << "| EmptyBX | " << empty.raw << " | " << empty.quality << " | " << empty.valid << " | " << empty.gammaN << " | " << empty.Ngamma << " |\n\n";
  out << "## Recommended Working Cutoffs\n\n";
  out << "| cut | nominal | recommended | MC/noise samples | efficiencies | note |\n";
  out << "|---|---:|---:|---:|---:|---|\n";
  out << "| ZDC plus | " << NOM_ZDC_PLUS << " | " << zdcPlus.threshold << " | low " << zdcPlus.lowN << ", high " << zdcPlus.highN << ", EBX " << zdcPlus.controlN
      << " | low " << zdcPlus.lowEff << ", high " << zdcPlus.highEff << ", EBX high " << zdcPlus.controlFake << " | " << zdcPlus.note << " |\n";
  out << "| ZDC minus | " << NOM_ZDC_MINUS << " | " << zdcMinus.threshold << " | low " << zdcMinus.lowN << ", high " << zdcMinus.highN << ", EBX " << zdcMinus.controlN
      << " | low " << zdcMinus.lowEff << ", high " << zdcMinus.highEff << ", EBX high " << zdcMinus.controlFake << " | " << zdcMinus.note << " |\n";
  out << "| HF plus gap | " << NOM_HF_PLUS << " | " << hfPlus.threshold << " | MC " << hfPlus.lowN << ", data " << hfPlus.highN
      << " | MC below " << hfPlus.lowEff << ", data below " << hfPlus.highEff << " | " << hfPlus.note << " |\n";
  out << "| HF minus gap | " << NOM_HF_MINUS << " | " << hfMinus.threshold << " | MC " << hfMinus.lowN << ", data " << hfMinus.highN
      << " | MC below " << hfMinus.lowEff << ", data below " << hfMinus.highEff << " | " << hfMinus.note << " |\n\n";
  out << "## Quantile Cross-Checks\n\n";
  out << "| distribution | q50 | q90 | q95 | q99 |\n";
  out << "|---|---:|---:|---:|---:|\n";
  auto qline = [&](const char *label, const std::vector<double> &v) {
    out << "| " << label << " | " << Quantile(v, 0.50) << " | " << Quantile(v, 0.90) << " | " << Quantile(v, 0.95) << " | " << Quantile(v, 0.99) << " |\n";
  };
  qline("MC plus low-side ZDC", mc.plusLow);
  qline("MC plus high-side ZDC", mc.plusHigh);
  qline("MC minus low-side ZDC", mc.minusLow);
  qline("MC minus high-side ZDC", mc.minusHigh);
  qline("MC HF plus gap", mc.hfGapPlus);
  qline("MC HF minus gap", mc.hfGapMinus);
  qline("EmptyBX ZDC plus", empty.zdcPlusAll);
  qline("EmptyBX ZDC minus", empty.zdcMinusAll);
  qline("Data HF plus gap", data.hfGapPlus);
  qline("Data HF minus gap", data.hfGapMinus);
  out << "\n## Caveats\n\n";
  out << "- The official prompt MC is `PhotonBeamA`; if one ZDC side has low MC high-side statistics, the corresponding side is less constrained.\n";
  out << "- HF recommendations are working thresholds based on MC signal retention. Final values still need the analysis background definition and systematic variation.\n";
  out << "- These outputs supersede the earlier edge-of-grid data/EmptyBX-only scan for event-threshold discussions.\n";
}
}

int derive_zdc_hf_mc_cutoffs(const char *promptMcPath,
                             const char *dataPath,
                             const char *emptyBxPath,
                             const char *outDir,
                             Long64_t maxEvents = 1000000) {
  gStyle->SetOptStat(0);
  gSystem->mkdir(outDir, true);
  const std::string out = outDir;

  Sample mc, data, empty;
  if (!mc.Open("prompt_mc", promptMcPath)) return 1;
  if (!data.Open("data", dataPath)) return 1;
  if (!empty.Open("emptybx", emptyBxPath)) return 1;

  Dist mcDist = Collect(mc, maxEvents);
  Dist dataDist = Collect(data, maxEvents);
  Dist emptyDist = Collect(empty, maxEvents);

  std::vector<double> plusLow = mcDist.plusLow;
  Append(plusLow, emptyDist.zdcPlusAll);
  std::vector<double> minusLow = mcDist.minusLow;
  Append(minusLow, emptyDist.zdcMinusAll);

  ThresholdResult zdcPlus = ChooseZdcThreshold(plusLow, mcDist.plusHigh, emptyDist.zdcPlusAll, 200.0, 2500.0, 25.0);
  ThresholdResult zdcMinus = ChooseZdcThreshold(minusLow, mcDist.minusHigh, emptyDist.zdcMinusAll, 200.0, 2500.0, 25.0);
  ThresholdResult hfPlus = ChooseHfThreshold(mcDist.hfGapPlus, dataDist.hfGapPlus, 1.0, 25.0, 0.1, 0.95);
  ThresholdResult hfMinus = ChooseHfThreshold(mcDist.hfGapMinus, dataDist.hfGapMinus, 1.0, 25.0, 0.1, 0.95);

  std::ofstream scan(out + "/recommended_cutoffs.tsv");
  scan << "cut\tnominal\trecommended\tlow_or_signal_n\thigh_or_data_n\tcontrol_n\tlow_or_signal_eff\thigh_or_data_eff\tcontrol_fake_or_blank\tscore\tnote\n";
  scan << "zdc_plus\t" << NOM_ZDC_PLUS << '\t' << zdcPlus.threshold << '\t' << zdcPlus.lowN << '\t' << zdcPlus.highN << '\t' << zdcPlus.controlN << '\t'
       << zdcPlus.lowEff << '\t' << zdcPlus.highEff << '\t' << zdcPlus.controlFake << '\t' << zdcPlus.score << '\t' << zdcPlus.note << '\n';
  scan << "zdc_minus\t" << NOM_ZDC_MINUS << '\t' << zdcMinus.threshold << '\t' << zdcMinus.lowN << '\t' << zdcMinus.highN << '\t' << zdcMinus.controlN << '\t'
       << zdcMinus.lowEff << '\t' << zdcMinus.highEff << '\t' << zdcMinus.controlFake << '\t' << zdcMinus.score << '\t' << zdcMinus.note << '\n';
  scan << "hf_plus\t" << NOM_HF_PLUS << '\t' << hfPlus.threshold << '\t' << hfPlus.lowN << '\t' << hfPlus.highN << "\t0\t"
       << hfPlus.lowEff << '\t' << hfPlus.highEff << "\t\t" << hfPlus.score << '\t' << hfPlus.note << '\n';
  scan << "hf_minus\t" << NOM_HF_MINUS << '\t' << hfMinus.threshold << '\t' << hfMinus.lowN << '\t' << hfMinus.highN << "\t0\t"
       << hfMinus.lowEff << '\t' << hfMinus.highEff << "\t\t" << hfMinus.score << '\t' << hfMinus.note << '\n';

  SaveOverlay(out, "zdc_plus_separation", "ZDC plus separation;ZDCsumPlus;unit-normalized events",
              plusLow, "MC low + EmptyBX", mcDist.plusHigh, "MC high", dataDist.zdcPlusAll, "data all",
              120, -500.0, 6000.0, zdcPlus.threshold, NOM_ZDC_PLUS);
  SaveOverlay(out, "zdc_minus_separation", "ZDC minus separation;ZDCsumMinus;unit-normalized events",
              minusLow, "MC low + EmptyBX", mcDist.minusHigh, "MC high", dataDist.zdcMinusAll, "data all",
              120, -500.0, 6000.0, zdcMinus.threshold, NOM_ZDC_MINUS);
  SaveOverlay(out, "hf_plus_gap", "HF plus gap side;HFEMaxPlus;unit-normalized events",
              mcDist.hfGapPlus, "MC gap", dataDist.hfGapPlus, "data gap", emptyDist.hfPlusAll, "EmptyBX all",
              100, 0.0, 30.0, hfPlus.threshold, NOM_HF_PLUS);
  SaveOverlay(out, "hf_minus_gap", "HF minus gap side;HFEMaxMinus;unit-normalized events",
              mcDist.hfGapMinus, "MC gap", dataDist.hfGapMinus, "data gap", emptyDist.hfMinusAll, "EmptyBX all",
              100, 0.0, 30.0, hfMinus.threshold, NOM_HF_MINUS);

  WriteSummary(out, mcDist, dataDist, emptyDist, zdcPlus, zdcMinus, hfPlus, hfMinus,
               maxEvents, promptMcPath, dataPath, emptyBxPath);
  std::cout << "Wrote MC ZDC/HF cutoff scan to " << outDir << std::endl;
  return 0;
}
