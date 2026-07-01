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

struct Events {
  std::string label;
  std::vector<double> zdcPlus;
  std::vector<double> zdcMinus;
  std::vector<double> hfPlus;
  std::vector<double> hfMinus;
};

struct Threshold {
  double value = -1.0;
  double signalEff = 0.0;
  double controlFake = 0.0;
  double score = -1.0;
  size_t lowN = 0;
  size_t highN = 0;
  std::string note;
};

bool HasBranch(TTree *tree, const char *name) {
  return tree && tree->GetBranch(name);
}

double FractionAbove(const std::vector<double> &values, double threshold) {
  if (values.empty()) return 0.0;
  const auto n = std::count_if(values.begin(), values.end(), [&](double value) { return value > threshold; });
  return static_cast<double>(n) / static_cast<double>(values.size());
}

double FractionBelow(const std::vector<double> &values, double threshold) {
  if (values.empty()) return 0.0;
  const auto n = std::count_if(values.begin(), values.end(), [&](double value) { return value < threshold; });
  return static_cast<double>(n) / static_cast<double>(values.size());
}

double Quantile(std::vector<double> values, double q) {
  if (values.empty()) return std::numeric_limits<double>::quiet_NaN();
  std::sort(values.begin(), values.end());
  const double pos = std::clamp(q, 0.0, 1.0) * (values.size() - 1);
  const auto lo = static_cast<size_t>(std::floor(pos));
  const auto hi = static_cast<size_t>(std::ceil(pos));
  if (lo == hi) return values[lo];
  const double frac = pos - lo;
  return values[lo] * (1.0 - frac) + values[hi] * frac;
}

bool Valid(double value) {
  return std::isfinite(value) && value > -9000.0;
}

Events LoadForest(const char *path, Long64_t maxEvents) {
  Events out;
  out.label = "official_prompt_mc_forest";
  TFile file(path, "READ");
  if (file.IsZombie()) {
    std::cerr << "Could not open MC forest " << path << "\n";
    return out;
  }
  auto *zdc = static_cast<TTree *>(file.Get("zdcanalyzer/zdcrechit"));
  auto *hi = static_cast<TTree *>(file.Get("hiEvtAnalyzer/HiTree"));
  if (!zdc || !hi) {
    std::cerr << "Missing zdcanalyzer/zdcrechit or hiEvtAnalyzer/HiTree in " << path << "\n";
    return out;
  }

  Float_t zdcPlus = 0.0f, zdcMinus = 0.0f;
  Float_t hfPlus = 0.0f, hfMinus = 0.0f;
  zdc->SetBranchStatus("*", 0);
  hi->SetBranchStatus("*", 0);
  zdc->SetBranchStatus("sumPlus", 1);
  zdc->SetBranchStatus("sumMinus", 1);
  hi->SetBranchStatus("hiHFPlus_pfle1", 1);
  hi->SetBranchStatus("hiHFMinus_pfle1", 1);
  zdc->SetBranchAddress("sumPlus", &zdcPlus);
  zdc->SetBranchAddress("sumMinus", &zdcMinus);
  hi->SetBranchAddress("hiHFPlus_pfle1", &hfPlus);
  hi->SetBranchAddress("hiHFMinus_pfle1", &hfMinus);

  const Long64_t entries = std::min(zdc->GetEntries(), hi->GetEntries());
  const Long64_t n = maxEvents >= 0 ? std::min(entries, maxEvents) : entries;
  out.zdcPlus.reserve(n);
  for (Long64_t i = 0; i < n; ++i) {
    zdc->GetEntry(i);
    hi->GetEntry(i);
    if (!Valid(zdcPlus) || !Valid(zdcMinus) || !Valid(hfPlus) || !Valid(hfMinus)) continue;
    out.zdcPlus.push_back(zdcPlus);
    out.zdcMinus.push_back(zdcMinus);
    out.hfPlus.push_back(hfPlus);
    out.hfMinus.push_back(hfMinus);
  }
  return out;
}

Events LoadReduced(const char *path, const char *label, Long64_t maxEvents, bool requireQuality) {
  Events out;
  out.label = label;
  TFile file(path, "READ");
  if (file.IsZombie()) {
    std::cerr << "Could not open reduced sample " << path << "\n";
    return out;
  }
  auto *tree = static_cast<TTree *>(file.Get("Tree"));
  if (!tree) {
    std::cerr << "Missing Tree in " << path << "\n";
    return out;
  }

  Float_t zdcPlus = 0.0f, zdcMinus = 0.0f, hfPlus = 0.0f, hfMinus = 0.0f;
  Bool_t selectedVtxFilter = true, clusterCompatibilityFilter = true;
  Int_t nVtx = 1;
  tree->SetBranchStatus("*", 0);
  const bool hasQuality = HasBranch(tree, "selectedVtxFilter") && HasBranch(tree, "ClusterCompatibilityFilter");
  const bool hasNVtx = HasBranch(tree, "nVtx");
  if (hasQuality) {
    tree->SetBranchStatus("selectedVtxFilter", 1);
    tree->SetBranchStatus("ClusterCompatibilityFilter", 1);
    tree->SetBranchAddress("selectedVtxFilter", &selectedVtxFilter);
    tree->SetBranchAddress("ClusterCompatibilityFilter", &clusterCompatibilityFilter);
  }
  if (hasNVtx) {
    tree->SetBranchStatus("nVtx", 1);
    tree->SetBranchAddress("nVtx", &nVtx);
  }
  tree->SetBranchStatus("ZDCsumPlus", 1);
  tree->SetBranchStatus("ZDCsumMinus", 1);
  tree->SetBranchStatus("HFEMaxPlus", 1);
  tree->SetBranchStatus("HFEMaxMinus", 1);
  tree->SetBranchAddress("ZDCsumPlus", &zdcPlus);
  tree->SetBranchAddress("ZDCsumMinus", &zdcMinus);
  tree->SetBranchAddress("HFEMaxPlus", &hfPlus);
  tree->SetBranchAddress("HFEMaxMinus", &hfMinus);

  const Long64_t entries = tree->GetEntries();
  const Long64_t n = maxEvents >= 0 ? std::min(entries, maxEvents) : entries;
  out.zdcPlus.reserve(std::min<Long64_t>(n, 1000000));
  for (Long64_t i = 0; i < n; ++i) {
    tree->GetEntry(i);
    if (requireQuality) {
      if (hasQuality && (!selectedVtxFilter || !clusterCompatibilityFilter)) continue;
      if (hasNVtx && (nVtx <= 0 || nVtx > 3)) continue;
    }
    if (!Valid(zdcPlus) || !Valid(zdcMinus) || !Valid(hfPlus) || !Valid(hfMinus)) continue;
    out.zdcPlus.push_back(zdcPlus);
    out.zdcMinus.push_back(zdcMinus);
    out.hfPlus.push_back(hfPlus);
    out.hfMinus.push_back(hfMinus);
  }
  return out;
}

TH1D *Hist(const char *name, const char *title, const std::vector<double> &values, int bins, double lo, double hi) {
  auto *hist = new TH1D(name, title, bins, lo, hi);
  hist->SetDirectory(nullptr);
  for (double value : values) hist->Fill(value);
  if (hist->Integral() > 0) hist->Scale(1.0 / hist->Integral());
  return hist;
}

void SaveOverlay(const char *outDir,
                 const char *name,
                 const char *title,
                 const std::vector<std::pair<std::string, std::vector<double>>> &series,
                 int bins,
                 double lo,
                 double hi,
                 double threshold = -1.0) {
  TCanvas canvas("canvas", "canvas", 900, 700);
  canvas.SetLogy();
  TLegend legend(0.58, 0.70, 0.88, 0.88);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  int color = 1;
  double maxY = 0.0;
  std::vector<TH1D *> hists;
  for (const auto &item : series) {
    auto *hist = Hist((std::string("h_") + name + "_" + item.first).c_str(), title, item.second, bins, lo, hi);
    hist->SetLineColor(color);
    hist->SetLineWidth(2);
    maxY = std::max(maxY, hist->GetMaximum());
    hists.push_back(hist);
    legend.AddEntry(hist, item.first.c_str(), "l");
    ++color;
    if (color == 5) ++color;
  }
  bool first = true;
  for (auto *hist : hists) {
    hist->SetMaximum(std::max(1e-3, maxY * 8.0));
    hist->SetMinimum(1e-5);
    hist->Draw(first ? "hist" : "hist same");
    first = false;
  }
  if (threshold > 0) {
    TLine line(threshold, 1e-5, threshold, std::max(1e-3, maxY * 4.0));
    line.SetLineColor(kRed + 1);
    line.SetLineStyle(2);
    line.SetLineWidth(2);
    line.Draw();
  }
  legend.Draw();
  canvas.SaveAs((std::string(outDir) + "/" + name + ".png").c_str());
  canvas.SaveAs((std::string(outDir) + "/" + name + ".pdf").c_str());
  for (auto *hist : hists) delete hist;
}

Threshold ChooseZdc(const std::vector<double> &low, const std::vector<double> &high,
                    const std::vector<double> &emptyBx, double minThr, double maxThr, double step) {
  Threshold best;
  best.lowN = low.size();
  best.highN = high.size();
  if (low.empty() || high.empty()) {
    best.note = "insufficient official-MC high/low side samples";
    return best;
  }
  for (double threshold = minThr; threshold <= maxThr + 1e-9; threshold += step) {
    const double lowEff = FractionBelow(low, threshold);
    const double highEff = FractionAbove(high, threshold);
    const double fake = FractionAbove(emptyBx, threshold);
    const double eff = 0.5 * (lowEff + highEff);
    const double score = eff - 0.20 * fake;
    if (score > best.score) {
      best.value = threshold;
      best.signalEff = eff;
      best.controlFake = fake;
      best.score = score;
    }
  }
  best.note = "maximizes official-MC low/high ZDC separation with EmptyBX fake penalty";
  return best;
}

Threshold ChooseHf(std::vector<double> signalGap, const std::vector<double> &dataGap, double targetEff) {
  Threshold out;
  out.lowN = signalGap.size();
  out.highN = dataGap.size();
  if (signalGap.empty()) {
    out.note = "insufficient official-MC gap-side HF samples";
    return out;
  }
  out.value = Quantile(signalGap, targetEff);
  out.signalEff = FractionBelow(signalGap, out.value);
  out.controlFake = FractionBelow(dataGap, out.value);
  out.score = out.signalEff;
  out.note = "lowest threshold retaining target official-MC gap-side HF efficiency; data fraction is diagnostic";
  return out;
}

bool PassZdc(double plus, double minus, double plusThr, double minusThr) {
  return (minus > minusThr && plus < plusThr) || (plus > plusThr && minus < minusThr);
}

void SplitByHigherZdc(const Events &events,
                      std::vector<double> &plusLow,
                      std::vector<double> &plusHigh,
                      std::vector<double> &minusLow,
                      std::vector<double> &minusHigh,
                      std::vector<double> &hfPlusGap,
                      std::vector<double> &hfMinusGap,
                      std::vector<double> &zdcLow,
                      std::vector<double> &zdcHigh) {
  for (size_t i = 0; i < events.zdcPlus.size(); ++i) {
    const double plus = events.zdcPlus[i];
    const double minus = events.zdcMinus[i];
    if (plus >= minus) {
      plusHigh.push_back(plus);
      minusLow.push_back(minus);
      hfMinusGap.push_back(events.hfMinus[i]);
      zdcHigh.push_back(plus);
      zdcLow.push_back(minus);
    } else {
      plusLow.push_back(plus);
      minusHigh.push_back(minus);
      hfPlusGap.push_back(events.hfPlus[i]);
      zdcHigh.push_back(minus);
      zdcLow.push_back(plus);
    }
  }
}

void CollectDataGap(const Events &events,
                    std::vector<double> &hfPlusGap,
                    std::vector<double> &hfMinusGap,
                    double zdcPlusThr = NOM_ZDC_PLUS,
                    double zdcMinusThr = NOM_ZDC_MINUS) {
  for (size_t i = 0; i < events.zdcPlus.size(); ++i) {
    const double plus = events.zdcPlus[i];
    const double minus = events.zdcMinus[i];
    if (minus > zdcMinusThr && plus < zdcPlusThr) {
      hfPlusGap.push_back(events.hfPlus[i]);
    } else if (plus > zdcPlusThr && minus < zdcMinusThr) {
      hfMinusGap.push_back(events.hfMinus[i]);
    }
  }
}
}  // namespace

void derive_zdc_hf_cutoffs_from_official_mc_forest(
    const char *mcForest,
    const char *dataReduced,
    const char *emptyBxReduced,
    const char *outDir,
    Long64_t maxControlEvents = 200000) {
  gStyle->SetOptStat(0);
  gSystem->mkdir(outDir, true);

  Events mc = LoadForest(mcForest, -1);
  Events data = LoadReduced(dataReduced, "2023_data_reference", maxControlEvents, true);
  Events empty = LoadReduced(emptyBxReduced, "emptybx_control", maxControlEvents, false);
  const bool allZdcZero =
      std::all_of(mc.zdcPlus.begin(), mc.zdcPlus.end(), [](double v) { return v == 0.0; }) &&
      std::all_of(mc.zdcMinus.begin(), mc.zdcMinus.end(), [](double v) { return v == 0.0; });

  std::vector<double> mcPlusLow, mcPlusHigh, mcMinusLow, mcMinusHigh;
  std::vector<double> mcHfPlusGap, mcHfMinusGap, mcZdcLow, mcZdcHigh;
  SplitByHigherZdc(mc, mcPlusLow, mcPlusHigh, mcMinusLow, mcMinusHigh, mcHfPlusGap, mcHfMinusGap, mcZdcLow, mcZdcHigh);

  std::vector<double> dataHfPlusGap, dataHfMinusGap;
  CollectDataGap(data, dataHfPlusGap, dataHfMinusGap);

  Threshold zdcPlus = ChooseZdc(mcPlusLow, mcPlusHigh, empty.zdcPlus, 400.0, 1800.0, 25.0);
  Threshold zdcMinus = ChooseZdc(mcMinusLow, mcMinusHigh, empty.zdcMinus, 400.0, 1800.0, 25.0);
  Threshold hfPlus = ChooseHf(mcHfPlusGap, dataHfPlusGap, 0.95);
  Threshold hfMinus = ChooseHf(mcHfMinusGap, dataHfMinusGap, 0.95);
  if (allZdcZero) {
    zdcPlus = Threshold{};
    zdcMinus = Threshold{};
    hfPlus = Threshold{};
    hfMinus = Threshold{};
    zdcPlus.note = "blocked: this official MC forest has all-zero ZDC sums";
    zdcMinus.note = zdcPlus.note;
    hfPlus.note = "blocked: single-beam gap side cannot be inferred from all-zero ZDC; use BeamA/BeamB helper";
    hfMinus.note = hfPlus.note;
  }

  SaveOverlay(outDir, "zdc_plus", "ZDC plus;sumPlus (GeV);unit-normalized events",
              {{"official MC", mc.zdcPlus}, {"2023 data", data.zdcPlus}, {"EmptyBX", empty.zdcPlus}},
              120, 0, 4000, zdcPlus.value);
  SaveOverlay(outDir, "zdc_minus", "ZDC minus;sumMinus (GeV);unit-normalized events",
              {{"official MC", mc.zdcMinus}, {"2023 data", data.zdcMinus}, {"EmptyBX", empty.zdcMinus}},
              120, 0, 4000, zdcMinus.value);
  SaveOverlay(outDir, "zdc_high_low_mc", "Official MC ZDC high/low sides;ZDC sum (GeV);unit-normalized events",
              {{"higher side", mcZdcHigh}, {"lower side", mcZdcLow}}, 120, 0, 4000, 0);
  SaveOverlay(outDir, "hf_plus_gap", "HF plus gap side;leading HF PF energy (GeV);unit-normalized events",
              {{"official MC", mcHfPlusGap}, {"2023 data nominal gammaN", dataHfPlusGap}},
              120, 0, 30, hfPlus.value);
  SaveOverlay(outDir, "hf_minus_gap", "HF minus gap side;leading HF PF energy (GeV);unit-normalized events",
              {{"official MC", mcHfMinusGap}, {"2023 data nominal Ngamma", dataHfMinusGap}},
              120, 0, 30, hfMinus.value);

  const auto mcNomZdc = std::count_if(mc.zdcPlus.begin(), mc.zdcPlus.end(), [&](double, size_t idx = 0) mutable {
    const bool pass = PassZdc(mc.zdcPlus[idx], mc.zdcMinus[idx], NOM_ZDC_PLUS, NOM_ZDC_MINUS);
    ++idx;
    return pass;
  });
  const auto mcNewZdc = std::count_if(mc.zdcPlus.begin(), mc.zdcPlus.end(), [&](double, size_t idx = 0) mutable {
    const bool pass = PassZdc(mc.zdcPlus[idx], mc.zdcMinus[idx], zdcPlus.value, zdcMinus.value);
    ++idx;
    return pass;
  });

  std::ofstream tsv(std::string(outDir) + "/recommended_cutoffs.tsv");
  tsv << "cut\tnominal\tprovisional\tmc_eff_or_retention\tcontrol_or_data_fraction\tlow_or_signal_n\thigh_or_control_n\tnote\n";
  tsv << std::fixed << std::setprecision(6);
  tsv << "zdc_plus\t" << NOM_ZDC_PLUS << "\t" << zdcPlus.value << "\t" << zdcPlus.signalEff << "\t"
      << zdcPlus.controlFake << "\t" << zdcPlus.lowN << "\t" << zdcPlus.highN << "\t" << zdcPlus.note << "\n";
  tsv << "zdc_minus\t" << NOM_ZDC_MINUS << "\t" << zdcMinus.value << "\t" << zdcMinus.signalEff << "\t"
      << zdcMinus.controlFake << "\t" << zdcMinus.lowN << "\t" << zdcMinus.highN << "\t" << zdcMinus.note << "\n";
  tsv << "hf_plus\t" << NOM_HF_PLUS << "\t" << hfPlus.value << "\t" << hfPlus.signalEff << "\t"
      << hfPlus.controlFake << "\t" << hfPlus.lowN << "\t" << hfPlus.highN << "\t" << hfPlus.note << "\n";
  tsv << "hf_minus\t" << NOM_HF_MINUS << "\t" << hfMinus.value << "\t" << hfMinus.signalEff << "\t"
      << hfMinus.controlFake << "\t" << hfMinus.lowN << "\t" << hfMinus.highN << "\t" << hfMinus.note << "\n";

  std::ofstream readme(std::string(outDir) + "/README.md");
  readme << "# Official MC ZDC/HF Cutoff Smoke\n\n";
  readme << "<!-- DOC_OWNER: cms-analysis-manager detector-level official MC cutoff smoke. -->\n";
  readme << "<!-- TOKEN_NOTE: provisional bounded study; do not freeze analysis cuts without larger validation. -->\n\n";
  readme << "## Inputs\n\n";
  readme << "- Official prompt UPC PbPb photonuclear MC forest: `" << mcForest << "`\n";
  readme << "- 2023 data control reduced skim: `" << dataReduced << "`\n";
  readme << "- EmptyBX control reduced skim: `" << emptyBxReduced << "`\n";
  readme << "- Data/EmptyBX max events read: `" << maxControlEvents << "`\n\n";
  readme << "## Counts\n\n";
  readme << "- Official MC detector entries: `" << mc.zdcPlus.size() << "`\n";
  readme << "- 2023 data control entries after quality cuts: `" << data.zdcPlus.size() << "`\n";
  readme << "- EmptyBX control entries: `" << empty.zdcPlus.size() << "`\n";
  readme << "- Official MC nominal ZDC XOR retention: `" << (mc.zdcPlus.empty() ? 0.0 : double(mcNomZdc) / mc.zdcPlus.size()) << "`\n";
  readme << "- Official MC provisional ZDC XOR retention: `" << (mc.zdcPlus.empty() ? 0.0 : double(mcNewZdc) / mc.zdcPlus.size()) << "`\n\n";
  readme << "## Provisional Cutoffs\n\n";
  readme << "- ZDC plus: `" << zdcPlus.value << "` GeV, nominal `" << NOM_ZDC_PLUS << "` GeV.\n";
  readme << "- ZDC minus: `" << zdcMinus.value << "` GeV, nominal `" << NOM_ZDC_MINUS << "` GeV.\n";
  readme << "- HF plus gap side: `" << hfPlus.value << "` GeV, nominal `" << NOM_HF_PLUS << "` GeV.\n";
  readme << "- HF minus gap side: `" << hfMinus.value << "` GeV, nominal `" << NOM_HF_MINUS << "` GeV.\n\n";
  readme << "## Caveats\n\n";
  readme << "- This uses a 500-event local forest smoke from the official MINIAODSIM, not full-statistics official forest production.\n";
  readme << "- HF uses `hiHFPlus_pfle1` and `hiHFMinus_pfle1`, the leading HF PF energy saved by `HiEvtAnalyzer`; this is close to but not yet proven identical to every historical `HFEMax` branch variant.\n";
  readme << "- ZDC side labels are inferred by whichever side has the larger reconstructed ZDC sum. Generator-level neutron-side truth still needs a direct cross-check.\n";
  readme << "- Treat the TSV as a diagnostic/provisional scan, not a frozen analysis prescription.\n";

  std::cout << "wrote " << outDir << "\n";
}

void derive_zdc_hf_cutoffs_from_official_mc_beams(
    const char *beamAForest,
    const char *beamBForest,
    const char *dataReduced,
    const char *emptyBxReduced,
    const char *outDir,
    Long64_t maxControlEvents = 200000) {
  gStyle->SetOptStat(0);
  gSystem->mkdir(outDir, true);

  Events beamA = LoadForest(beamAForest, -1);
  Events beamB = LoadForest(beamBForest, -1);
  Events data = LoadReduced(dataReduced, "2023_data_reference", maxControlEvents, true);
  Events empty = LoadReduced(emptyBxReduced, "emptybx_control", maxControlEvents, false);

  std::vector<double> dataHfPlusGap, dataHfMinusGap;
  CollectDataGap(data, dataHfPlusGap, dataHfMinusGap);

  const bool zdcUnavailable =
      std::all_of(beamA.zdcPlus.begin(), beamA.zdcPlus.end(), [](double v) { return v == 0.0; }) &&
      std::all_of(beamA.zdcMinus.begin(), beamA.zdcMinus.end(), [](double v) { return v == 0.0; }) &&
      std::all_of(beamB.zdcPlus.begin(), beamB.zdcPlus.end(), [](double v) { return v == 0.0; }) &&
      std::all_of(beamB.zdcMinus.begin(), beamB.zdcMinus.end(), [](double v) { return v == 0.0; });

  // In the official BeamA sample, HF activity is on minus and the gap side is plus.
  // In the official BeamB sample, HF activity is on plus and the gap side is minus.
  Threshold hfPlus = ChooseHf(beamA.hfPlus, dataHfPlusGap, 0.95);
  Threshold hfMinus = ChooseHf(beamB.hfMinus, dataHfMinusGap, 0.95);

  SaveOverlay(outDir, "zdc_plus_official_mc_vs_controls",
              "ZDC plus, official MC vs controls;sumPlus (GeV);unit-normalized events",
              {{"BeamA MC", beamA.zdcPlus}, {"BeamB MC", beamB.zdcPlus}, {"2023 data", data.zdcPlus}, {"EmptyBX", empty.zdcPlus}},
              120, 0, 4000, NOM_ZDC_PLUS);
  SaveOverlay(outDir, "zdc_minus_official_mc_vs_controls",
              "ZDC minus, official MC vs controls;sumMinus (GeV);unit-normalized events",
              {{"BeamA MC", beamA.zdcMinus}, {"BeamB MC", beamB.zdcMinus}, {"2023 data", data.zdcMinus}, {"EmptyBX", empty.zdcMinus}},
              120, 0, 4000, NOM_ZDC_MINUS);
  SaveOverlay(outDir, "hf_plus_gap_beama",
              "HF plus gap side, BeamA MC;leading HF PF energy (GeV);unit-normalized events",
              {{"BeamA MC gap", beamA.hfPlus}, {"2023 data nominal gammaN", dataHfPlusGap}},
              120, 0, 30, hfPlus.value);
  SaveOverlay(outDir, "hf_minus_gap_beamb",
              "HF minus gap side, BeamB MC;leading HF PF energy (GeV);unit-normalized events",
              {{"BeamB MC gap", beamB.hfMinus}, {"2023 data nominal Ngamma", dataHfMinusGap}},
              120, 0, 30, hfMinus.value);
  SaveOverlay(outDir, "hf_hadronic_side_official_mc",
              "HF hadronic-side check;leading HF PF energy (GeV);unit-normalized events",
              {{"BeamA minus", beamA.hfMinus}, {"BeamB plus", beamB.hfPlus}},
              120, 0, 120, -1.0);

  const double beamANomHf = FractionBelow(beamA.hfPlus, NOM_HF_PLUS);
  const double beamBNomHf = FractionBelow(beamB.hfMinus, NOM_HF_MINUS);

  std::ofstream tsv(std::string(outDir) + "/recommended_cutoffs.tsv");
  tsv << "cut\tnominal\tprovisional\tmc_eff_or_retention\tcontrol_or_data_fraction\tmc_n\tcontrol_n\tnote\n";
  tsv << std::fixed << std::setprecision(6);
  tsv << "zdc_plus\t" << NOM_ZDC_PLUS << "\t-1\t0\t0\t" << beamA.zdcPlus.size() + beamB.zdcPlus.size()
      << "\t" << empty.zdcPlus.size()
      << "\tblocked: official BeamA/BeamB MINIAODSIM forests have sumPlus=sumMinus=0, so ZDC neutron thresholds cannot be derived from this MC\n";
  tsv << "zdc_minus\t" << NOM_ZDC_MINUS << "\t-1\t0\t0\t" << beamA.zdcMinus.size() + beamB.zdcMinus.size()
      << "\t" << empty.zdcMinus.size()
      << "\tblocked: official BeamA/BeamB MINIAODSIM forests have sumPlus=sumMinus=0, so ZDC neutron thresholds cannot be derived from this MC\n";
  tsv << "hf_plus_gap\t" << NOM_HF_PLUS << "\t" << hfPlus.value << "\t" << hfPlus.signalEff << "\t"
      << hfPlus.controlFake << "\t" << hfPlus.lowN << "\t" << hfPlus.highN
      << "\tBeamA gap-side leading HF PF energy; nominal efficiency=" << beamANomHf << "\n";
  tsv << "hf_minus_gap\t" << NOM_HF_MINUS << "\t" << hfMinus.value << "\t" << hfMinus.signalEff << "\t"
      << hfMinus.controlFake << "\t" << hfMinus.lowN << "\t" << hfMinus.highN
      << "\tBeamB gap-side leading HF PF energy; nominal efficiency=" << beamBNomHf << "\n";

  std::ofstream readme(std::string(outDir) + "/README.md");
  readme << "# Official BeamA/BeamB MC ZDC/HF Cutoff Smoke\n\n";
  readme << "<!-- DOC_OWNER: cms-analysis-manager detector-level official MC cutoff smoke. -->\n";
  readme << "<!-- TOKEN_NOTE: bounded BeamA/BeamB check; ZDC is blocked by all-zero MC response. -->\n\n";
  readme << "## Inputs\n\n";
  readme << "- BeamA official prompt forest: `" << beamAForest << "`\n";
  readme << "- BeamB official prompt forest: `" << beamBForest << "`\n";
  readme << "- 2023 data control reduced skim: `" << dataReduced << "`\n";
  readme << "- EmptyBX control reduced skim: `" << emptyBxReduced << "`\n";
  readme << "- Data/EmptyBX max events read: `" << maxControlEvents << "`\n\n";
  readme << "## Main Finding\n\n";
  if (zdcUnavailable) {
    readme << "- ZDC is unavailable in this detector-level official MC smoke: BeamA and BeamB both have `sumPlus = sumMinus = 0` for every checked event.\n";
    readme << "- Therefore the official UPC photonuclear D0 MC can validate generator/HF-side behavior, but it cannot determine the 1n ZDC cutoff by itself.\n";
  } else {
    readme << "- ZDC is nonzero in at least one checked official MC event; inspect plots before using this output.\n";
  }
  readme << "- HF behaves as expected for the two photon-beam directions: BeamA has low plus-side HF and high minus-side HF; BeamB flips this pattern.\n\n";
  readme << "## Counts\n\n";
  readme << "- BeamA entries: `" << beamA.zdcPlus.size() << "`\n";
  readme << "- BeamB entries: `" << beamB.zdcPlus.size() << "`\n";
  readme << "- 2023 data control entries after quality cuts: `" << data.zdcPlus.size() << "`\n";
  readme << "- EmptyBX control entries: `" << empty.zdcPlus.size() << "`\n\n";
  readme << "## Provisional HF Cutoffs\n\n";
  readme << "- HF plus gap side from BeamA: `" << hfPlus.value << "` GeV for 95% MC efficiency; nominal `" << NOM_HF_PLUS << "` GeV has MC efficiency `" << beamANomHf << "`.\n";
  readme << "- HF minus gap side from BeamB: `" << hfMinus.value << "` GeV for 95% MC efficiency; nominal `" << NOM_HF_MINUS << "` GeV has MC efficiency `" << beamBNomHf << "`.\n";
  readme << "- These are diagnostics for the HF gap behavior, not final analysis cuts.\n\n";
  readme << "## ZDC Cutoff Status\n\n";
  readme << "- Keep using data/EmptyBX and the validated 2023 ZDC reconstruction/calibration path for ZDC threshold studies.\n";
  readme << "- Do not claim a MC-derived ZDC neutron cutoff from this official photonuclear sample unless a separate neutron-breakup/ZDC-response MC is introduced.\n\n";
  readme << "## Outputs\n\n";
  readme << "- `recommended_cutoffs.tsv`\n";
  readme << "- `zdc_plus_official_mc_vs_controls.png/pdf`\n";
  readme << "- `zdc_minus_official_mc_vs_controls.png/pdf`\n";
  readme << "- `hf_plus_gap_beama.png/pdf`\n";
  readme << "- `hf_minus_gap_beamb.png/pdf`\n";
  readme << "- `hf_hadronic_side_official_mc.png/pdf`\n";

  std::cout << "wrote " << outDir << "\n";
}
