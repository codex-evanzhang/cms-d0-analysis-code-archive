#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TChain.h"
#include "TFile.h"
#include "TF1.h"
#include "TGraph.h"
#include "TH1D.h"
#include "TLegend.h"
#include "TLine.h"
#include "TMath.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"

namespace {
constexpr double kNominalPlus = 1100.0;
constexpr double kNominalMinus = 1000.0;

struct Samples {
  std::vector<double> dataPlus;
  std::vector<double> dataMinus;
  std::vector<double> emptyPlus;
  std::vector<double> emptyMinus;
};

struct FitResult {
  std::string side;
  double mean = std::numeric_limits<double>::quiet_NaN();
  double sigma = std::numeric_limits<double>::quiet_NaN();
  double amplitude = std::numeric_limits<double>::quiet_NaN();
  double seed = std::numeric_limits<double>::quiet_NaN();
  double chi2 = std::numeric_limits<double>::quiet_NaN();
  double ndf = 0.0;
  int status = -1;
};

struct ThresholdRow {
  std::string side;
  std::string method;
  double threshold = std::numeric_limits<double>::quiet_NaN();
  double rounded100 = std::numeric_limits<double>::quiet_NaN();
  double nSigmaBelowMean = std::numeric_limits<double>::quiet_NaN();
  double oneNeutronEff = std::numeric_limits<double>::quiet_NaN();
  double emptyBxFake = std::numeric_limits<double>::quiet_NaN();
  double dataAbove = std::numeric_limits<double>::quiet_NaN();
  std::string note;
};

bool HasBranch(TTree *tree, const char *name) {
  return tree && tree->GetBranch(name);
}

bool Valid(double value) {
  return std::isfinite(value) && value > -9000.0;
}

double FractionAbove(const std::vector<double> &values, double threshold) {
  if (values.empty() || !std::isfinite(threshold)) return std::numeric_limits<double>::quiet_NaN();
  const auto n = std::count_if(values.begin(), values.end(), [&](double value) { return value > threshold; });
  return static_cast<double>(n) / static_cast<double>(values.size());
}

double RoundTo(double value, double step) {
  if (!std::isfinite(value) || step <= 0) return std::numeric_limits<double>::quiet_NaN();
  return step * std::round(value / step);
}

double GaussianSurvival(double threshold, double mean, double sigma) {
  if (!std::isfinite(threshold) || !std::isfinite(mean) || !std::isfinite(sigma) || sigma <= 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const double z = (threshold - mean) / sigma;
  return 0.5 * std::erfc(z / std::sqrt(2.0));
}

void LoadReducedTree(const char *path,
                     Long64_t maxEntries,
                     bool requireQuality,
                     std::vector<double> &plus,
                     std::vector<double> &minus) {
  TFile file(path, "READ");
  if (file.IsZombie()) {
    std::cerr << "could not open " << path << "\n";
    return;
  }
  auto *tree = static_cast<TTree *>(file.Get("Tree"));
  if (!tree) {
    std::cerr << "missing Tree in " << path << "\n";
    return;
  }

  Float_t zdcPlus = 0.0f;
  Float_t zdcMinus = 0.0f;
  Bool_t selectedVtxFilter = true;
  Bool_t clusterCompatibilityFilter = true;
  Int_t nVtx = 1;

  tree->SetBranchStatus("*", 0);
  tree->SetBranchStatus("ZDCsumPlus", 1);
  tree->SetBranchStatus("ZDCsumMinus", 1);
  tree->SetBranchAddress("ZDCsumPlus", &zdcPlus);
  tree->SetBranchAddress("ZDCsumMinus", &zdcMinus);

  const bool hasSelectedVtx = HasBranch(tree, "selectedVtxFilter");
  const bool hasCluster = HasBranch(tree, "ClusterCompatibilityFilter");
  const bool hasNVtx = HasBranch(tree, "nVtx");
  if (hasSelectedVtx) {
    tree->SetBranchStatus("selectedVtxFilter", 1);
    tree->SetBranchAddress("selectedVtxFilter", &selectedVtxFilter);
  }
  if (hasCluster) {
    tree->SetBranchStatus("ClusterCompatibilityFilter", 1);
    tree->SetBranchAddress("ClusterCompatibilityFilter", &clusterCompatibilityFilter);
  }
  if (hasNVtx) {
    tree->SetBranchStatus("nVtx", 1);
    tree->SetBranchAddress("nVtx", &nVtx);
  }

  const Long64_t entries = tree->GetEntries();
  const Long64_t n = maxEntries >= 0 ? std::min(entries, maxEntries) : entries;
  plus.reserve(std::min<Long64_t>(n, 1000000));
  minus.reserve(std::min<Long64_t>(n, 1000000));

  for (Long64_t i = 0; i < n; ++i) {
    tree->GetEntry(i);
    if (requireQuality) {
      if (hasSelectedVtx && !selectedVtxFilter) continue;
      if (hasCluster && !clusterCompatibilityFilter) continue;
      if (hasNVtx && (nVtx <= 0 || nVtx > 3)) continue;
    }
    if (!Valid(zdcPlus) || !Valid(zdcMinus)) continue;
    plus.push_back(zdcPlus);
    minus.push_back(zdcMinus);
  }
}

bool HasWildcard(const std::string &path) {
  return path.find('*') != std::string::npos || path.find('?') != std::string::npos ||
         path.find('[') != std::string::npos;
}

void LoadForestZdc(const char *path,
                   Long64_t maxEntries,
                   std::vector<double> &plus,
                   std::vector<double> &minus) {
  TChain chain("zdcanalyzer/zdcrechit");
  const int added = chain.Add(path);
  if (added <= 0) {
    std::cerr << "could not add forest ZDC path " << path << "\n";
    return;
  }

  Float_t zdcPlus = 0.0f;
  Float_t zdcMinus = 0.0f;
  chain.SetBranchStatus("*", 0);
  chain.SetBranchStatus("sumPlus", 1);
  chain.SetBranchStatus("sumMinus", 1);
  chain.SetBranchAddress("sumPlus", &zdcPlus);
  chain.SetBranchAddress("sumMinus", &zdcMinus);

  const Long64_t entries = chain.GetEntries();
  const Long64_t n = maxEntries >= 0 ? std::min(entries, maxEntries) : entries;
  plus.reserve(std::min<Long64_t>(n, 1000000));
  minus.reserve(std::min<Long64_t>(n, 1000000));
  for (Long64_t i = 0; i < n; ++i) {
    chain.GetEntry(i);
    if (!Valid(zdcPlus) || !Valid(zdcMinus)) continue;
    plus.push_back(zdcPlus);
    minus.push_back(zdcMinus);
  }
}

void LoadZdcInput(const char *path,
                  Long64_t maxEntries,
                  bool requireQuality,
                  std::vector<double> &plus,
                  std::vector<double> &minus) {
  const std::string pathString(path);
  if (HasWildcard(pathString)) {
    LoadForestZdc(path, maxEntries, plus, minus);
    return;
  }

  TFile file(path, "READ");
  if (file.IsZombie()) {
    std::cerr << "could not open " << path << "\n";
    return;
  }
  if (file.Get("Tree")) {
    file.Close();
    LoadReducedTree(path, maxEntries, requireQuality, plus, minus);
    return;
  }
  if (file.Get("zdcanalyzer/zdcrechit")) {
    file.Close();
    LoadForestZdc(path, maxEntries, plus, minus);
    return;
  }
  std::cerr << "no supported ZDC tree in " << path << "\n";
}

TH1D *MakeHist(const char *name, const char *title, const std::vector<double> &values,
               int bins = 180, double lo = 0.0, double hi = 9000.0) {
  auto *hist = new TH1D(name, title, bins, lo, hi);
  hist->SetDirectory(nullptr);
  for (double value : values) hist->Fill(value);
  return hist;
}

double SmoothBin(const TH1D *hist, int bin, int halfWindow = 2) {
  double sum = 0.0;
  int n = 0;
  for (int offset = -halfWindow; offset <= halfWindow; ++offset) {
    const int b = bin + offset;
    if (b < 1 || b > hist->GetNbinsX()) continue;
    sum += hist->GetBinContent(b);
    ++n;
  }
  return n > 0 ? sum / n : 0.0;
}

FitResult FitFirstNeutronPeak(TH1D *hist, const std::string &side) {
  FitResult out;
  out.side = side;

  const int loBin = hist->FindBin(1500.0);
  const int hiBin = hist->FindBin(3600.0);
  int maxBin = loBin;
  double maxVal = -1.0;
  for (int bin = loBin; bin <= hiBin; ++bin) {
    const double value = SmoothBin(hist, bin, 1);
    if (value > maxVal) {
      maxVal = value;
      maxBin = bin;
    }
  }

  out.seed = hist->GetBinCenter(maxBin);
  const double fitLo = std::max(700.0, out.seed - 1100.0);
  const double fitHi = std::min(4200.0, out.seed + 1000.0);
  TF1 fit((side + "_g1_plus_linear_bg").c_str(), "gaus(0)+pol1(3)", fitLo, fitHi);
  fit.SetParameters(std::max(1.0, maxVal), out.seed, 700.0, 1.0, 0.0);
  fit.SetParLimits(1, 1700.0, 3300.0);
  fit.SetParLimits(2, 250.0, 1500.0);
  auto status = hist->Fit(&fit, "RQS0");

  out.status = static_cast<int>(status);
  out.amplitude = fit.GetParameter(0);
  out.mean = fit.GetParameter(1);
  out.sigma = std::abs(fit.GetParameter(2));
  out.chi2 = fit.GetChisquare();
  out.ndf = fit.GetNDF();

  if (out.status != 0 || !std::isfinite(out.mean) || !std::isfinite(out.sigma) || out.sigma <= 0) {
    out.mean = out.seed;
    out.sigma = 750.0;
  }
  return out;
}

double RisingEdgeAtFraction(TH1D *hist, const FitResult &fit, double fraction) {
  if (!std::isfinite(fit.mean) || !std::isfinite(fit.amplitude) || fit.amplitude <= 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const double target = fit.amplitude * fraction;
  const int meanBin = hist->FindBin(fit.mean);
  const int minBin = hist->FindBin(200.0);
  for (int bin = meanBin; bin >= minBin; --bin) {
    if (SmoothBin(hist, bin, 2) < target) {
      return hist->GetBinCenter(bin + 1);
    }
  }
  return std::numeric_limits<double>::quiet_NaN();
}

double SignalOverEmptyBxCrossing(TH1D *emptyHist,
                                 const FitResult &fit,
                                 double dataIntegral,
                                 double emptyIntegral,
                                 double ratio) {
  if (!emptyHist || !std::isfinite(fit.mean) || !std::isfinite(fit.sigma) || fit.sigma <= 0 ||
      !std::isfinite(fit.amplitude) || fit.amplitude <= 0 || dataIntegral <= 0 || emptyIntegral <= 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const int meanBin = emptyHist->FindBin(fit.mean);
  const int minBin = emptyHist->FindBin(300.0);
  const double oneEventDensity = 0.5 / emptyIntegral;
  for (int bin = meanBin; bin >= minBin; --bin) {
    const double x = emptyHist->GetBinCenter(bin);
    const double signalDensity =
        fit.amplitude * std::exp(-0.5 * std::pow((x - fit.mean) / fit.sigma, 2.0)) / dataIntegral;
    const double emptyDensity = std::max(SmoothBin(emptyHist, bin, 1) / emptyIntegral, oneEventDensity);
    if (signalDensity < ratio * emptyDensity) {
      return emptyHist->GetBinCenter(std::min(bin + 1, emptyHist->GetNbinsX()));
    }
  }
  return std::numeric_limits<double>::quiet_NaN();
}

double LocalSignalOverEmptyBx(TH1D *emptyHist,
                              const FitResult &fit,
                              double dataIntegral,
                              double emptyIntegral,
                              double threshold) {
  if (!emptyHist || !std::isfinite(threshold) || !std::isfinite(fit.mean) || !std::isfinite(fit.sigma) ||
      fit.sigma <= 0 || !std::isfinite(fit.amplitude) || fit.amplitude <= 0 || dataIntegral <= 0 ||
      emptyIntegral <= 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const int bin = emptyHist->FindBin(threshold);
  const double signalDensity =
      fit.amplitude * std::exp(-0.5 * std::pow((threshold - fit.mean) / fit.sigma, 2.0)) / dataIntegral;
  const double emptyDensity = std::max(SmoothBin(emptyHist, bin, 1) / emptyIntegral, 0.5 / emptyIntegral);
  return signalDensity / emptyDensity;
}

double ChooseSignalNoiseFakeThreshold(TH1D *emptyHist,
                                      const FitResult &fit,
                                      const std::vector<double> &empty,
                                      double dataIntegral,
                                      double minSignalOverNoise = 2.0,
                                      double maxEmptyFake = 5.0e-5,
                                      double minOneNeutronEff = 0.975) {
  const double emptyIntegral = emptyHist ? emptyHist->Integral() : 0.0;
  for (double threshold = 400.0; threshold <= 2200.0; threshold += 25.0) {
    const double son = LocalSignalOverEmptyBx(emptyHist, fit, dataIntegral, emptyIntegral, threshold);
    const double fake = FractionAbove(empty, threshold);
    const double eff = GaussianSurvival(threshold, fit.mean, fit.sigma);
    if (std::isfinite(son) && std::isfinite(fake) && std::isfinite(eff) &&
        son >= minSignalOverNoise && fake <= maxEmptyFake && eff >= minOneNeutronEff) {
      return threshold;
    }
  }
  return std::numeric_limits<double>::quiet_NaN();
}

double EmptyBxDenseTailEnd(TH1D *emptyHist, double maxAverageCount = 5.0) {
  if (!emptyHist) return std::numeric_limits<double>::quiet_NaN();
  const int minBin = emptyHist->FindBin(300.0);
  const int maxBin = emptyHist->FindBin(1800.0);
  constexpr int window = 3;
  for (int bin = minBin; bin <= maxBin - window; ++bin) {
    double sum = 0.0;
    double maxCount = 0.0;
    for (int offset = 0; offset < window; ++offset) {
      const double count = emptyHist->GetBinContent(bin + offset);
      sum += count;
      maxCount = std::max(maxCount, count);
    }
    if (sum / window <= maxAverageCount && maxCount <= 2.0 * maxAverageCount) {
      return emptyHist->GetBinLowEdge(bin);
    }
  }
  return std::numeric_limits<double>::quiet_NaN();
}

ThresholdRow MakeRow(const std::string &side,
                     const std::string &method,
                     double threshold,
                     const FitResult &fit,
                     const std::vector<double> &data,
                     const std::vector<double> &empty,
                     const std::string &note) {
  ThresholdRow row;
  row.side = side;
  row.method = method;
  row.threshold = threshold;
  row.rounded100 = RoundTo(threshold, 100.0);
  if (std::isfinite(threshold) && std::isfinite(fit.mean) && std::isfinite(fit.sigma) && fit.sigma > 0) {
    row.nSigmaBelowMean = (fit.mean - threshold) / fit.sigma;
  }
  row.oneNeutronEff = GaussianSurvival(threshold, fit.mean, fit.sigma);
  row.emptyBxFake = FractionAbove(empty, threshold);
  row.dataAbove = FractionAbove(data, threshold);
  row.note = note;
  return row;
}

std::vector<ThresholdRow> CandidateRows(const std::string &side,
                                        TH1D *hist,
                                        TH1D *emptyHist,
                                        const FitResult &fit,
                                        const std::vector<double> &data,
                                        const std::vector<double> &empty,
                                        double nominal) {
  std::vector<ThresholdRow> rows;
  rows.push_back(MakeRow(side, "nominal_AN_calibrated_signal", nominal, fit, data, empty,
                         "AN threshold applied to ZDCsignal = HADsum + 0.1*EMsum"));
  rows.push_back(MakeRow(side, "mu_minus_1sigma", fit.mean - fit.sigma, fit, data, empty, "too high for near-full 1n efficiency"));
  rows.push_back(MakeRow(side, "mu_minus_1p8sigma", fit.mean - 1.8 * fit.sigma, fit, data, empty, "approximate low-side onset"));
  rows.push_back(MakeRow(side, "mu_minus_2sigma", fit.mean - 2.0 * fit.sigma, fit, data, empty, "simple natural 1n-onset rule"));
  rows.push_back(MakeRow(side, "mu_minus_2p2sigma", fit.mean - 2.2 * fit.sigma, fit, data, empty, "stricter signal-efficiency rule"));
  rows.push_back(MakeRow(side, "gaussian_height_20pct", fit.mean - std::sqrt(2.0 * std::log(5.0)) * fit.sigma,
                         fit, data, empty, "low edge where fitted 1n gaussian is 20% of peak height"));
  rows.push_back(MakeRow(side, "gaussian_height_10pct", fit.mean - std::sqrt(2.0 * std::log(10.0)) * fit.sigma,
                         fit, data, empty, "low edge where fitted 1n gaussian is 10% of peak height"));
  rows.push_back(MakeRow(side, "data_rising_edge_20pct", RisingEdgeAtFraction(hist, fit, 0.20),
                         fit, data, empty, "smoothed data rising edge at 20% of fitted 1n height"));
  rows.push_back(MakeRow(side, "data_rising_edge_10pct", RisingEdgeAtFraction(hist, fit, 0.10),
                         fit, data, empty, "smoothed data rising edge at 10% of fitted 1n height"));
  rows.push_back(MakeRow(side, "signal_emptybx_crossing_s_over_n_1",
                         SignalOverEmptyBxCrossing(emptyHist, fit, hist->Integral(), emptyHist->Integral(), 1.0),
                         fit, data, empty, "lowest edge where fitted 1n density dominates EmptyBX noise"));
  rows.push_back(MakeRow(side, "signal_emptybx_crossing_s_over_n_2",
                         SignalOverEmptyBxCrossing(emptyHist, fit, hist->Integral(), emptyHist->Integral(), 2.0),
                         fit, data, empty, "lowest edge where fitted 1n density is 2x EmptyBX noise"));
  rows.push_back(MakeRow(side, "signal_emptybx_crossing_s_over_n_3",
                         SignalOverEmptyBxCrossing(emptyHist, fit, hist->Integral(), emptyHist->Integral(), 3.0),
                         fit, data, empty, "lowest edge where fitted 1n density is 3x EmptyBX noise"));
  rows.push_back(MakeRow(side, "signal_emptybx_crossing_s_over_n_5",
                         SignalOverEmptyBxCrossing(emptyHist, fit, hist->Integral(), emptyHist->Integral(), 5.0),
                         fit, data, empty, "lowest edge where fitted 1n density is 5x EmptyBX noise"));
  rows.push_back(MakeRow(side, "signal_emptybx_crossing_s_over_n_10",
                         SignalOverEmptyBxCrossing(emptyHist, fit, hist->Integral(), emptyHist->Integral(), 10.0),
                         fit, data, empty, "lowest edge where fitted 1n density is 10x EmptyBX noise"));
  rows.push_back(MakeRow(side, "emptybx_dense_tail_end",
                         EmptyBxDenseTailEnd(emptyHist, 5.0),
                         fit, data, empty, "first 3-bin EmptyBX window with average <=5 counts and no bin >10 counts"));
  rows.push_back(MakeRow(side, "recommended_s_over_n2_fake5e-5_eff97p5",
                         ChooseSignalNoiseFakeThreshold(emptyHist, fit, empty, hist->Integral(), 2.0, 5.0e-5, 0.975),
                         fit, data, empty, "lowest threshold with local fitted-1n/EmptyBX >=2, EmptyBX tail <=5e-5, and Gaussian 1n efficiency >=97.5%"));
  return rows;
}

void WriteRows(const char *outDir, const std::vector<ThresholdRow> &rows) {
  std::ofstream out(std::string(outDir) + "/zdc_1n_threshold_candidates.tsv");
  out << "side\tmethod\tthreshold\trounded100\tnSigmaBelowMean\toneNeutronEffFromGaussian\temptyBxFakeAbove\tdataFractionAbove\tnote\n";
  out << std::fixed << std::setprecision(6);
  for (const auto &row : rows) {
    out << row.side << '\t' << row.method << '\t' << row.threshold << '\t' << row.rounded100 << '\t'
        << row.nSigmaBelowMean << '\t' << row.oneNeutronEff << '\t' << row.emptyBxFake << '\t'
        << row.dataAbove << '\t' << row.note << '\n';
  }
}

void WriteFitParams(const char *outDir, const FitResult &plus, const FitResult &minus,
                    size_t dataN, size_t emptyN) {
  std::ofstream out(std::string(outDir) + "/zdc_1n_fit_params.tsv");
  out << "side\tmean\tsigma\tseed\tamplitude\tchi2\tndf\tfitStatus\tdataEntries\temptyBxEntries\n";
  out << std::fixed << std::setprecision(6);
  for (const auto &fit : {plus, minus}) {
    out << fit.side << '\t' << fit.mean << '\t' << fit.sigma << '\t' << fit.seed << '\t'
        << fit.amplitude << '\t' << fit.chi2 << '\t' << fit.ndf << '\t' << fit.status << '\t'
        << dataN << '\t' << emptyN << '\n';
  }
}

void WriteScan(const char *outDir,
               const std::string &side,
               const FitResult &fit,
               const std::vector<double> &data,
               const std::vector<double> &empty) {
  std::ofstream out(std::string(outDir) + "/zdc_1n_threshold_scan_" + side + ".tsv");
  out << "side\tthreshold\tnSigmaBelowMean\toneNeutronEffFromGaussian\temptyBxFakeAbove\tdataFractionAbove\tgaussianHeightFraction\n";
  out << std::fixed << std::setprecision(6);
  for (double threshold = 400.0; threshold <= 2200.0; threshold += 50.0) {
    const double nsigma = (fit.mean - threshold) / fit.sigma;
    const double height = std::exp(-0.5 * nsigma * nsigma);
    out << side << '\t' << threshold << '\t' << nsigma << '\t'
        << GaussianSurvival(threshold, fit.mean, fit.sigma) << '\t'
        << FractionAbove(empty, threshold) << '\t' << FractionAbove(data, threshold) << '\t'
        << height << '\n';
  }
}

void SavePlot(const char *outDir,
              const std::string &side,
              TH1D *dataHist,
              TH1D *emptyHist,
              const FitResult &fit,
              const std::vector<ThresholdRow> &rows,
              double nominal) {
  const double dataIntegral = dataHist->Integral();
  const double emptyIntegral = emptyHist->Integral();
  auto *dataUnit = static_cast<TH1D *>(dataHist->Clone((side + "_data_unit").c_str()));
  auto *emptyUnit = static_cast<TH1D *>(emptyHist->Clone((side + "_empty_unit").c_str()));
  dataUnit->SetDirectory(nullptr);
  emptyUnit->SetDirectory(nullptr);
  if (dataIntegral > 0) dataUnit->Scale(1.0 / dataIntegral);
  if (emptyIntegral > 0) emptyUnit->Scale(1.0 / emptyIntegral);

  TCanvas canvas((side + "_canvas").c_str(), (side + "_canvas").c_str(), 980, 720);
  canvas.SetLogy();
  dataUnit->SetLineColor(kBlue + 1);
  dataUnit->SetLineWidth(2);
  emptyUnit->SetLineColor(kGray + 2);
  emptyUnit->SetLineWidth(2);
  emptyUnit->SetLineStyle(2);
  dataUnit->SetTitle(("ZDC " + side + " data-driven 1n threshold;ZDC energy sum (GeV);unit-normalized entries").c_str());
  dataUnit->SetMinimum(1e-7);
  dataUnit->SetMaximum(std::max(dataUnit->GetMaximum(), emptyUnit->GetMaximum()) * 20.0);
  dataUnit->Draw("hist");
  emptyUnit->Draw("hist same");

  TF1 g1((side + "_g1_draw").c_str(), "gaus", 0.0, 9000.0);
  g1.SetParameters(fit.amplitude / std::max(1.0, dataIntegral), fit.mean, fit.sigma);
  g1.SetLineColor(kGreen + 2);
  g1.SetLineWidth(2);
  g1.Draw("same");

  TLegend legend(0.58, 0.68, 0.90, 0.90);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.AddEntry(dataUnit, "2023 data", "l");
  legend.AddEntry(emptyUnit, "EmptyBX", "l");
  legend.AddEntry(&g1, "1n gaussian component", "l");

  auto drawLine = [&](double x, int color, int style, const char *label) {
    if (!std::isfinite(x)) return;
    TLine *line = new TLine(x, 1e-7, x, dataUnit->GetMaximum() / 2.0);
    line->SetLineColor(color);
    line->SetLineStyle(style);
    line->SetLineWidth(2);
    line->Draw();
    legend.AddEntry(line, label, "l");
  };

  drawLine(nominal, kRed + 1, 1, "AN nominal");
  for (const auto &row : rows) {
    if (row.side == side && row.method == "mu_minus_2sigma") {
      drawLine(row.rounded100, kGray + 2, 3, "round(mu-2sigma)");
    }
    if (row.side == side && row.method == "recommended_s_over_n2_fake5e-5_eff97p5") {
      drawLine(row.rounded100, kMagenta + 1, 2, "recommended rounded");
    }
  }

  legend.Draw();
  canvas.SaveAs((std::string(outDir) + "/zdc_" + side + "_1n_threshold.png").c_str());
  canvas.SaveAs((std::string(outDir) + "/zdc_" + side + "_1n_threshold.pdf").c_str());
  delete dataUnit;
  delete emptyUnit;
}

void WriteReadme(const char *outDir,
                 const char *dataPath,
                 const char *emptyPath,
                 Long64_t maxEntries,
                 const FitResult &plus,
                 const FitResult &minus,
                 const std::vector<ThresholdRow> &rows) {
  auto findRow = [&](const std::string &side, const std::string &method) -> ThresholdRow {
    for (const auto &row : rows) {
      if (row.side == side && row.method == method) return row;
    }
    return ThresholdRow{};
  };
  const auto plusTwo = findRow("plus", "mu_minus_2sigma");
  const auto minusTwo = findRow("minus", "mu_minus_2sigma");
  const auto plusRec = findRow("plus", "recommended_s_over_n2_fake5e-5_eff97p5");
  const auto minusRec = findRow("minus", "recommended_s_over_n2_fake5e-5_eff97p5");

  std::ofstream out(std::string(outDir) + "/README.md");
  out << "# ZDC 1n Threshold Validation\n\n";
  out << "<!-- DOC_OWNER: cms-analysis-manager ZDC threshold derivation. -->\n";
  out << "<!-- TOKEN_NOTE: compact report; inspect TSV/plots before rerunning ROOT over EOS. -->\n\n";
  out << "## Inputs\n\n";
  out << "- Data input: `" << dataPath << "`\n";
  out << "- EmptyBX input: `" << emptyPath << "`\n";
  out << "- Max entries per sample: `" << maxEntries << "`\n\n";
  out << "## Fitted 1n Peaks\n\n";
  out << "| Side | mean | sigma | fit status |\n";
  out << "|---|---:|---:|---:|\n";
  out << std::fixed << std::setprecision(2);
  out << "| plus | " << plus.mean << " | " << plus.sigma << " | " << plus.status << " |\n";
  out << "| minus | " << minus.mean << " | " << minus.sigma << " | " << minus.status << " |\n\n";
  out << "## Nominal AN ZDC Cut\n\n";
  out << "The nominal analysis cut is not chosen by the scan in this directory. It follows the AN recipe: build the offline ZDC signal as `HADsum + 0.1 * EMsum`, after ADC-to-fC conversion, pedestal subtraction, gain correction, OOTPU subtraction, and offline `TS2 - TS1`. The 0nXn class is an XOR: one ZDC side must be above its 1n threshold while the opposite side is below its threshold.\n\n";
  out << "- ZDC plus 1n threshold: `" << kNominalPlus << " GeV`\n";
  out << "- ZDC minus 1n threshold: `" << kNominalMinus << " GeV`\n";
  out << "- Xn0n side: `plus > " << kNominalPlus << "` and `minus < " << kNominalMinus << "`\n";
  out << "- 0nXn side: `minus > " << kNominalMinus << "` and `plus < " << kNominalPlus << "`\n\n";
  out << "## Validation Rule Check\n\n";
  out << "The saved data/EmptyBX scan is a validation check. It asks whether a local fitted 1n density threshold, an integrated EmptyBX tail requirement, and high fitted 1n efficiency naturally bracket the AN thresholds. It should not override the calibrated AN threshold definition.\n\n";
  out << "| Side | AN nominal | validation raw | validation rounded | Gaussian 1n efficiency | EmptyBX fake above raw |\n";
  out << "|---|---:|---:|---:|---:|---:|\n";
  out << std::setprecision(6);
  out << "| plus | " << kNominalPlus << " | " << plusRec.threshold << " | " << plusRec.rounded100
      << " | " << plusRec.oneNeutronEff
      << " | " << plusRec.emptyBxFake << " |\n";
  out << "| minus | " << kNominalMinus << " | " << minusRec.threshold << " | " << minusRec.rounded100
      << " | " << minusRec.oneNeutronEff
      << " | " << minusRec.emptyBxFake << " |\n\n";
  out << "The simpler `mu - 2 sigma` rule is also saved as a diagnostic. It tends to cut too high on these forest-level distributions:\n\n";
  out << "| Side | mu - 2 sigma | rounded | Gaussian 1n efficiency | EmptyBX fake above raw |\n";
  out << "|---|---:|---:|---:|---:|\n";
  out << "| plus | " << plusTwo.threshold << " | " << plusTwo.rounded100 << " | " << plusTwo.oneNeutronEff << " | " << plusTwo.emptyBxFake << " |\n";
  out << "| minus | " << minusTwo.threshold << " | " << minusTwo.rounded100 << " | " << minusTwo.oneNeutronEff << " | " << minusTwo.emptyBxFake << " |\n\n";
  out << "Use `zdc_1n_threshold_candidates.tsv` for the full method comparison, including rounded values and data fractions.\n\n";
  out << "## Efficiency Notes\n\n";
  out << "- The 0n-side efficiency is checked with EmptyBX events by counting noise above the 1n threshold; the AN states this rate is below `1e-6`.\n";
  out << "- The Xn-side efficiency is checked by integrating fitted 1n, 2n, and 3n ZDC distributions above the 1n threshold; the AN states the inefficiency is about `1%`.\n";
  out << "- Electromagnetic pileup survival is applied later as a correction/uncertainty, with AN reference probabilities about `0.96` nominal and `0.92` for a conservative peak-pileup scenario.\n\n";
  out << "## Outputs\n\n";
  out << "- `zdc_1n_fit_params.tsv`\n";
  out << "- `zdc_1n_threshold_candidates.tsv`\n";
  out << "- `zdc_1n_threshold_scan_plus.tsv`\n";
  out << "- `zdc_1n_threshold_scan_minus.tsv`\n";
  out << "- `zdc_plus_1n_threshold.png/pdf`\n";
  out << "- `zdc_minus_1n_threshold.png/pdf`\n\n";
  out << "## Caveats\n\n";
  out << "- This derives thresholds from the ZDC sums available in the supplied input. For forest patterns this is `zdcanalyzer/zdcrechit`; for reduced files this is `Tree::ZDCsumPlus/Minus`.\n";
  out << "- The EmptyBX tail is counted empirically. Confirm the branch definition/calibration against the AN before finalizing a fake-rate claim.\n";
  out << "- The AN cut is the nominal selection. The fitted/scan-derived alternatives are retained only as validation diagnostics.\n";
}
}  // namespace

void derive_zdc_1n_thresholds_data_driven(
    const char *dataPath,
    const char *emptyBxPath,
    const char *outDir,
    Long64_t maxEntries = 5000000) {
  gStyle->SetOptStat(0);
  gSystem->mkdir(outDir, true);

  Samples samples;
  LoadZdcInput(dataPath, maxEntries, true, samples.dataPlus, samples.dataMinus);
  LoadZdcInput(emptyBxPath, maxEntries, false, samples.emptyPlus, samples.emptyMinus);

  auto *hDataPlus = MakeHist("hDataPlus", "ZDC plus data", samples.dataPlus);
  auto *hDataMinus = MakeHist("hDataMinus", "ZDC minus data", samples.dataMinus);
  auto *hEmptyPlus = MakeHist("hEmptyPlus", "ZDC plus EmptyBX", samples.emptyPlus);
  auto *hEmptyMinus = MakeHist("hEmptyMinus", "ZDC minus EmptyBX", samples.emptyMinus);

  FitResult plusFit = FitFirstNeutronPeak(hDataPlus, "plus");
  FitResult minusFit = FitFirstNeutronPeak(hDataMinus, "minus");

  std::vector<ThresholdRow> rows;
  auto plusRows = CandidateRows("plus", hDataPlus, hEmptyPlus, plusFit, samples.dataPlus, samples.emptyPlus, kNominalPlus);
  auto minusRows = CandidateRows("minus", hDataMinus, hEmptyMinus, minusFit, samples.dataMinus, samples.emptyMinus, kNominalMinus);
  rows.insert(rows.end(), plusRows.begin(), plusRows.end());
  rows.insert(rows.end(), minusRows.begin(), minusRows.end());

  WriteFitParams(outDir, plusFit, minusFit, samples.dataPlus.size(), samples.emptyPlus.size());
  WriteRows(outDir, rows);
  WriteScan(outDir, "plus", plusFit, samples.dataPlus, samples.emptyPlus);
  WriteScan(outDir, "minus", minusFit, samples.dataMinus, samples.emptyMinus);
  SavePlot(outDir, "plus", hDataPlus, hEmptyPlus, plusFit, rows, kNominalPlus);
  SavePlot(outDir, "minus", hDataMinus, hEmptyMinus, minusFit, rows, kNominalMinus);
  WriteReadme(outDir, dataPath, emptyBxPath, maxEntries, plusFit, minusFit, rows);

  delete hDataPlus;
  delete hDataMinus;
  delete hEmptyPlus;
  delete hEmptyMinus;

  std::cout << "wrote " << outDir << "\n";
}
