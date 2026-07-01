#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
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
constexpr double NOM_HF_PLUS = 9.2;
constexpr double NOM_HF_MINUS = 8.6;

struct SideSample {
  std::vector<double> hf;
  std::vector<int> ntrk;
};

struct ReducedSample {
  std::string label;
  TFile *file = nullptr;
  TTree *tree = nullptr;

  Bool_t selectedVtxFilter = true;
  Bool_t clusterCompatibilityFilter = true;
  Bool_t zdcGammaN = false;
  Bool_t zdcNgamma = false;
  Int_t nVtx = 1;
  Int_t nTrack = 0;
  Float_t hfPlus = -9999.0f;
  Float_t hfMinus = -9999.0f;

  bool hasSelectedVtx = false;
  bool hasCluster = false;
  bool hasNVtx = false;
  bool hasNTrack = false;
  bool hasGammaN = false;
  bool hasNgamma = false;
  bool hasHfPlus = false;
  bool hasHfMinus = false;

  template <typename T>
  void Bind(const char *name, T *value, bool &flag) {
    if (tree && tree->GetBranch(name)) {
      tree->SetBranchStatus(name, 1);
      tree->SetBranchAddress(name, value);
      flag = true;
    }
  }

  bool Open(const char *sampleLabel, const char *path) {
    label = sampleLabel;
    file = TFile::Open(path, "READ");
    if (!file || file->IsZombie()) {
      std::cerr << "Could not open " << path << "\n";
      return false;
    }
    file->GetObject("Tree", tree);
    if (!tree) {
      std::cerr << "Could not find Tree in " << path << "\n";
      return false;
    }
    tree->SetBranchStatus("*", 0);
    Bind("selectedVtxFilter", &selectedVtxFilter, hasSelectedVtx);
    Bind("ClusterCompatibilityFilter", &clusterCompatibilityFilter, hasCluster);
    Bind("nVtx", &nVtx, hasNVtx);
    Bind("nTrackInAcceptanceHP", &nTrack, hasNTrack);
    Bind("ZDCgammaN", &zdcGammaN, hasGammaN);
    Bind("ZDCNgamma", &zdcNgamma, hasNgamma);
    Bind("HFEMaxPlus", &hfPlus, hasHfPlus);
    Bind("HFEMaxMinus", &hfMinus, hasHfMinus);
    return hasGammaN && hasNgamma && hasHfPlus && hasHfMinus;
  }

  bool Quality() const {
    if (hasSelectedVtx && !selectedVtxFilter) return false;
    if (hasCluster && !clusterCompatibilityFilter) return false;
    if (hasNVtx && (nVtx <= 0 || nVtx > 3)) return false;
    return true;
  }
};

struct HfCollection {
  SideSample plusGap;
  SideSample minusGap;
  std::vector<double> plusAll;
  std::vector<double> minusAll;
  Long64_t raw = 0;
  Long64_t quality = 0;
  Long64_t valid = 0;
  Long64_t gammaN = 0;
  Long64_t Ngamma = 0;
};

struct ThresholdRow {
  double threshold = 0.0;
  double mcEff = 0.0;
  bool hasMcGap = false;
  double dataLowEff = 0.0;
  double dataHighLeak = 0.0;
  double dataAllPass = 0.0;
  double emptyPass = 0.0;
  double score = 0.0;
  bool passesRule = false;
};

struct Recommendation {
  std::string side;
  double nominal = 0.0;
  double threshold = -1.0;
  double mcEff = 0.0;
  double dataLowEff = 0.0;
  double dataHighLeak = 0.0;
  double dataAllPass = 0.0;
  double emptyPass = 0.0;
  double score = 0.0;
  double nominalMcEff = 0.0;
  bool hasMcGap = false;
  bool nominalHasMcGap = false;
  double nominalDataLowEff = 0.0;
  double nominalDataHighLeak = 0.0;
  bool nominalPassesRule = false;
  double nominalScoreOverDerived = 0.0;
  double nominalMinusDerived = 0.0;
  int lowTrackMax = -1;
  int highTrackMin = -1;
  std::string note;
};

bool ValidHf(double value) {
  return std::isfinite(value) && value > -9000.0;
}

double FractionBelow(const std::vector<double> &values, double threshold) {
  if (values.empty()) return 0.0;
  const auto n = std::count_if(values.begin(), values.end(), [&](double value) { return value < threshold; });
  return static_cast<double>(n) / static_cast<double>(values.size());
}

double FractionBelowTrack(const SideSample &sample, int minTrack, int maxTrack, double threshold) {
  Long64_t denom = 0;
  Long64_t pass = 0;
  for (size_t i = 0; i < sample.hf.size(); ++i) {
    const int ntrk = i < sample.ntrk.size() ? sample.ntrk[i] : 0;
    if (ntrk < minTrack || ntrk > maxTrack) continue;
    ++denom;
    if (sample.hf[i] < threshold) ++pass;
  }
  return denom > 0 ? static_cast<double>(pass) / static_cast<double>(denom) : 0.0;
}

size_t CountTrack(const SideSample &sample, int minTrack, int maxTrack) {
  size_t out = 0;
  for (int ntrk : sample.ntrk) {
    if (ntrk >= minTrack && ntrk <= maxTrack) ++out;
  }
  return out;
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

double QuantileInt(std::vector<int> values, double q) {
  if (values.empty()) return std::numeric_limits<double>::quiet_NaN();
  std::sort(values.begin(), values.end());
  const double pos = std::clamp(q, 0.0, 1.0) * (values.size() - 1);
  const auto idx = static_cast<size_t>(std::round(pos));
  return values[std::min(idx, values.size() - 1)];
}

HfCollection Collect(ReducedSample &sample, Long64_t maxEvents, bool requireQuality) {
  HfCollection out;
  const Long64_t entries = sample.tree->GetEntries();
  const Long64_t n = maxEvents >= 0 ? std::min(entries, maxEvents) : entries;
  out.raw = n;
  for (Long64_t entry = 0; entry < n; ++entry) {
    sample.tree->GetEntry(entry);
    if (requireQuality && !sample.Quality()) continue;
    ++out.quality;
    if (!ValidHf(sample.hfPlus) || !ValidHf(sample.hfMinus)) continue;
    ++out.valid;
    out.plusAll.push_back(sample.hfPlus);
    out.minusAll.push_back(sample.hfMinus);
    const int ntrk = sample.hasNTrack ? sample.nTrack : 0;
    const bool gammaN = sample.zdcGammaN && !sample.zdcNgamma;
    const bool Ngamma = sample.zdcNgamma && !sample.zdcGammaN;
    if (gammaN) {
      ++out.gammaN;
      out.plusGap.hf.push_back(sample.hfPlus);
      out.plusGap.ntrk.push_back(ntrk);
    } else if (Ngamma) {
      ++out.Ngamma;
      out.minusGap.hf.push_back(sample.hfMinus);
      out.minusGap.ntrk.push_back(ntrk);
    }
  }
  return out;
}

int LowTrackMax(const SideSample &dataSide) {
  if (dataSide.ntrk.empty()) return 3;
  // Track multiplicity is used only as a background proxy. The lower quartile
  // gives a data-defined UPC-like control slice and is never allowed below 3.
  return std::max(3, static_cast<int>(std::floor(QuantileInt(dataSide.ntrk, 0.25))));
}

int HighTrackMin(const SideSample &dataSide) {
  if (dataSide.ntrk.empty()) return 15;
  // The upper quartile gives a data-defined hadronic-activity proxy.
  return std::max(10, static_cast<int>(std::ceil(QuantileInt(dataSide.ntrk, 0.75))));
}

std::vector<ThresholdRow> ScanSide(const SideSample &mcSide,
                                   const SideSample &dataSide,
                                   const std::vector<double> &emptySide,
                                   int lowTrackMax,
                                   int highTrackMin,
                                   double minThreshold,
                                   double maxThreshold,
                                   double step,
                                   double targetMcEff,
                                   double minLowTrackEff,
                                   double maxHighTrackLeak) {
  std::vector<ThresholdRow> rows;
  const bool hasMcGap = !mcSide.hf.empty();
  for (double t = minThreshold; t <= maxThreshold + 1e-9; t += step) {
    ThresholdRow row;
    row.threshold = std::round(t * 1000.0) / 1000.0;
    row.hasMcGap = hasMcGap;
    row.mcEff = hasMcGap ? FractionBelow(mcSide.hf, t) : std::numeric_limits<double>::quiet_NaN();
    row.dataLowEff = FractionBelowTrack(dataSide, 0, lowTrackMax, t);
    row.dataHighLeak = FractionBelowTrack(dataSide, highTrackMin, std::numeric_limits<int>::max(), t);
    row.dataAllPass = FractionBelow(dataSide.hf, t);
    row.emptyPass = FractionBelow(emptySide, t);
    const double mcFactor = hasMcGap ? row.mcEff : 1.0;
    row.score = mcFactor * row.dataLowEff / std::sqrt(std::max(1e-6, row.dataHighLeak));
    row.passesRule = (!hasMcGap || row.mcEff >= targetMcEff) &&
                     row.dataLowEff >= minLowTrackEff &&
                     row.dataHighLeak <= maxHighTrackLeak;
    rows.push_back(row);
  }
  return rows;
}

const ThresholdRow *AtOrBelowNominal(const std::vector<ThresholdRow> &rows, double nominal) {
  const ThresholdRow *best = nullptr;
  for (const auto &row : rows) {
    if (std::abs(row.threshold - nominal) < 1e-6) return &row;
    if (row.threshold <= nominal) best = &row;
  }
  return best;
}

Recommendation Choose(const std::string &side,
                      double nominal,
                      const std::vector<ThresholdRow> &rows,
                      int lowTrackMax,
                      int highTrackMin,
                      double targetMcEff,
                      double minLowTrackEff,
                      double maxHighTrackLeak) {
  Recommendation rec;
  rec.side = side;
  rec.nominal = nominal;
  rec.lowTrackMax = lowTrackMax;
  rec.highTrackMin = highTrackMin;
  const ThresholdRow *nom = AtOrBelowNominal(rows, nominal);
  if (nom) {
    rec.nominalMcEff = nom->mcEff;
    rec.nominalHasMcGap = nom->hasMcGap;
    rec.nominalDataLowEff = nom->dataLowEff;
    rec.nominalDataHighLeak = nom->dataHighLeak;
  }
  for (const auto &row : rows) {
    if (!row.passesRule) continue;
    rec.threshold = row.threshold;
    rec.mcEff = row.mcEff;
    rec.hasMcGap = row.hasMcGap;
    rec.dataLowEff = row.dataLowEff;
    rec.dataHighLeak = row.dataHighLeak;
    rec.dataAllPass = row.dataAllPass;
    rec.emptyPass = row.emptyPass;
    rec.score = row.score;
    if (nom) {
      rec.nominalPassesRule = nom->passesRule;
      rec.nominalScoreOverDerived = rec.score > 0.0 ? nom->score / rec.score : 0.0;
      rec.nominalMinusDerived = nom->threshold - rec.threshold;
    }
    rec.note = row.hasMcGap
                   ? "blind first threshold satisfying MC efficiency, data low-track acceptance, and high-track leakage constraints; nominal is posthoc only"
                   : "blind first threshold satisfying data low-track acceptance and high-track leakage constraints; no same-side reduced-MC gap sample available; nominal is posthoc only";
    return rec;
  }
  const auto best = std::max_element(rows.begin(), rows.end(), [](const ThresholdRow &a, const ThresholdRow &b) { return a.score < b.score; });
  if (best != rows.end()) {
    rec.threshold = best->threshold;
    rec.mcEff = best->mcEff;
    rec.hasMcGap = best->hasMcGap;
    rec.dataLowEff = best->dataLowEff;
    rec.dataHighLeak = best->dataHighLeak;
    rec.dataAllPass = best->dataAllPass;
    rec.emptyPass = best->emptyPass;
    rec.score = best->score;
    if (nom) {
      rec.nominalPassesRule = nom->passesRule;
      rec.nominalScoreOverDerived = rec.score > 0.0 ? nom->score / rec.score : 0.0;
      rec.nominalMinusDerived = nom->threshold - rec.threshold;
    }
    rec.note = "fallback best score; no threshold passed the configured constraints";
  }
  return rec;
}

TH1D *Hist(const std::string &name, const std::string &title, const std::vector<double> &values, int bins, double lo, double hi) {
  auto *hist = new TH1D(name.c_str(), title.c_str(), bins, lo, hi);
  hist->SetDirectory(nullptr);
  for (double value : values) {
    if (std::isfinite(value)) hist->Fill(value);
  }
  if (hist->Integral() > 0) hist->Scale(1.0 / hist->Integral());
  return hist;
}

std::vector<double> SelectTrack(const SideSample &sample, int minTrack, int maxTrack) {
  std::vector<double> out;
  for (size_t i = 0; i < sample.hf.size(); ++i) {
    const int ntrk = sample.ntrk[i];
    if (ntrk >= minTrack && ntrk <= maxTrack) out.push_back(sample.hf[i]);
  }
  return out;
}

void SaveDistributionPlot(const std::string &outDir,
                          const std::string &name,
                          const std::string &title,
                          const SideSample &mcSide,
                          const SideSample &dataSide,
                          const std::vector<double> &emptySide,
                          int lowTrackMax,
                          int highTrackMin,
                          double recommended,
                          double nominal) {
  TCanvas canvas(("c_" + name).c_str(), "", 900, 650);
  canvas.SetLogy();
  const auto dataLow = SelectTrack(dataSide, 0, lowTrackMax);
  const auto dataHigh = SelectTrack(dataSide, highTrackMin, std::numeric_limits<int>::max());
  auto *hMc = Hist("h_" + name + "_mc", title, mcSide.hf, 100, 0.0, 30.0);
  auto *hLow = Hist("h_" + name + "_datalow", title, dataLow, 100, 0.0, 30.0);
  auto *hHigh = Hist("h_" + name + "_datahigh", title, dataHigh, 100, 0.0, 30.0);
  auto *hEmpty = Hist("h_" + name + "_empty", title, emptySide, 100, 0.0, 30.0);
  hMc->SetLineColor(kBlue + 1);
  hLow->SetLineColor(kGreen + 2);
  hHigh->SetLineColor(kRed + 1);
  hEmpty->SetLineColor(kGray + 2);
  hMc->SetLineWidth(2);
  hLow->SetLineWidth(2);
  hHigh->SetLineWidth(2);
  hEmpty->SetLineWidth(2);
  hMc->SetMinimum(1e-6);
  hMc->SetMaximum(std::max({hMc->GetMaximum(), hLow->GetMaximum(), hHigh->GetMaximum(), hEmpty->GetMaximum(), 1e-5}) * 8.0);
  hMc->Draw("hist");
  hLow->Draw("hist same");
  hHigh->Draw("hist same");
  hEmpty->Draw("hist same");
  TLegend legend(0.54, 0.66, 0.88, 0.88);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.AddEntry(hMc, "MC gap side", "l");
  legend.AddEntry(hLow, "data low nTrack", "l");
  legend.AddEntry(hHigh, "data high nTrack", "l");
  legend.AddEntry(hEmpty, "EmptyBX side", "l");
  TLine recLine(recommended, 1e-6, recommended, hMc->GetMaximum() * 0.8);
  recLine.SetLineColor(kMagenta + 2);
  recLine.SetLineWidth(2);
  recLine.Draw();
  legend.AddEntry(&recLine, "blind derived", "l");
  TLine nomLine(nominal, 1e-6, nominal, hMc->GetMaximum() * 0.8);
  nomLine.SetLineColor(kBlack);
  nomLine.SetLineStyle(2);
  nomLine.SetLineWidth(2);
  nomLine.Draw();
  legend.AddEntry(&nomLine, "nominal", "l");
  legend.Draw();
  canvas.SaveAs((outDir + "/" + name + ".png").c_str());
  canvas.SaveAs((outDir + "/" + name + ".pdf").c_str());
  delete hMc;
  delete hLow;
  delete hHigh;
  delete hEmpty;
}

void SaveScanPlot(const std::string &outDir, const std::string &name, const std::vector<ThresholdRow> &rows, double recommended, double nominal) {
  TCanvas canvas(("c_" + name).c_str(), "", 900, 650);
  std::vector<double> x, mc, low, high, empty;
  const bool hasMcGap = std::any_of(rows.begin(), rows.end(), [](const ThresholdRow &row) { return row.hasMcGap; });
  for (const auto &row : rows) {
    x.push_back(row.threshold);
    mc.push_back(row.hasMcGap ? row.mcEff : 0.0);
    low.push_back(row.dataLowEff);
    high.push_back(row.dataHighLeak);
    empty.push_back(row.emptyPass);
  }
  TGraph gMc(x.size(), x.data(), mc.data());
  TGraph gLow(x.size(), x.data(), low.data());
  TGraph gHigh(x.size(), x.data(), high.data());
  TGraph gEmpty(x.size(), x.data(), empty.data());
  gMc.SetTitle((name + ";HF threshold (GeV);fraction passing").c_str());
  gMc.SetLineColor(kBlue + 1);
  gLow.SetLineColor(kGreen + 2);
  gHigh.SetLineColor(kRed + 1);
  gEmpty.SetLineColor(kGray + 2);
  gMc.SetLineWidth(2);
  gLow.SetLineWidth(2);
  gHigh.SetLineWidth(2);
  gEmpty.SetLineWidth(2);
  if (hasMcGap) {
    gMc.SetMinimum(0.0);
    gMc.SetMaximum(1.05);
    gMc.Draw("AL");
    gLow.Draw("L same");
  } else {
    gLow.SetTitle((name + ";HF threshold (GeV);fraction passing").c_str());
    gLow.SetMinimum(0.0);
    gLow.SetMaximum(1.05);
    gLow.Draw("AL");
  }
  gHigh.Draw("L same");
  gEmpty.Draw("L same");
  TLine recLine(recommended, 0.0, recommended, 1.05);
  recLine.SetLineColor(kMagenta + 2);
  recLine.SetLineWidth(2);
  recLine.Draw();
  TLine nomLine(nominal, 0.0, nominal, 1.05);
  nomLine.SetLineColor(kBlack);
  nomLine.SetLineStyle(2);
  nomLine.SetLineWidth(2);
  nomLine.Draw();
  TLegend legend(0.52, 0.18, 0.88, 0.42);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  if (hasMcGap) {
    legend.AddEntry(&gMc, "MC gap efficiency", "l");
  } else {
    legend.AddEntry((TObject *)nullptr, "MC gap efficiency N/A", "");
  }
  legend.AddEntry(&gLow, "data low nTrack pass", "l");
  legend.AddEntry(&gHigh, "data high nTrack leak", "l");
  legend.AddEntry(&gEmpty, "EmptyBX pass", "l");
  legend.AddEntry(&recLine, "blind derived", "l");
  legend.AddEntry(&nomLine, "nominal", "l");
  legend.Draw();
  canvas.SaveAs((outDir + "/" + name + ".png").c_str());
  canvas.SaveAs((outDir + "/" + name + ".pdf").c_str());
}

void WriteScan(const std::string &path, const std::vector<ThresholdRow> &rows) {
  std::ofstream out(path);
  out << "threshold\tmcGapEfficiency\tdataLowTrackPass\tdataHighTrackLeak\tdataAllPass\temptyBxPass\tscore\tpassesRule\n";
  out << std::fixed << std::setprecision(6);
  for (const auto &row : rows) {
    out << row.threshold << '\t';
    if (row.hasMcGap) {
      out << row.mcEff;
    } else {
      out << "NA";
    }
    out << '\t' << row.dataLowEff << '\t'
        << row.dataHighLeak << '\t' << row.dataAllPass << '\t' << row.emptyPass
        << '\t' << row.score << '\t' << (row.passesRule ? "true" : "false") << '\n';
  }
}

void WriteQuantiles(const std::string &outDir,
                    const SideSample &mcPlus,
                    const SideSample &mcMinus,
                    const SideSample &dataPlus,
                    const SideSample &dataMinus,
                    const std::vector<double> &emptyPlus,
                    const std::vector<double> &emptyMinus,
                    int lowPlus,
                    int highPlus,
                    int lowMinus,
                    int highMinus) {
  std::ofstream out(outDir + "/hf_quantiles.tsv");
  out << "distribution\tn\tq50\tq90\tq95\tq97\tq99\n";
  auto line = [&](const std::string &label, const std::vector<double> &values) {
    out << label << '\t' << values.size() << '\t'
        << Quantile(values, 0.50) << '\t' << Quantile(values, 0.90) << '\t'
        << Quantile(values, 0.95) << '\t' << Quantile(values, 0.97) << '\t'
        << Quantile(values, 0.99) << '\n';
  };
  line("mc_plus_gap", mcPlus.hf);
  line("mc_minus_gap", mcMinus.hf);
  line("data_plus_gap_all", dataPlus.hf);
  line("data_minus_gap_all", dataMinus.hf);
  line("data_plus_gap_low_ntrack", SelectTrack(dataPlus, 0, lowPlus));
  line("data_minus_gap_low_ntrack", SelectTrack(dataMinus, 0, lowMinus));
  line("data_plus_gap_high_ntrack", SelectTrack(dataPlus, highPlus, std::numeric_limits<int>::max()));
  line("data_minus_gap_high_ntrack", SelectTrack(dataMinus, highMinus, std::numeric_limits<int>::max()));
  line("emptybx_hf_plus", emptyPlus);
  line("emptybx_hf_minus", emptyMinus);
}

std::string Fmt(double value, int precision = 3) {
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(precision) << value;
  return ss.str();
}

std::string FmtAvailable(double value, bool available, int precision = 3) {
  if (!available) return "NA";
  return Fmt(value, precision);
}
}  // namespace

int derive_hf_gap_cutoffs_data_mc(const char *promptMcPath,
                                  const char *dataPath,
                                  const char *emptyBxPath,
                                  const char *outDir,
                                  Long64_t maxEvents = -1,
                                  double targetMcEff = 0.989,
                                  double minLowTrackEff = 0.9765,
                                  double maxHighTrackLeak = 0.85) {
  gStyle->SetOptStat(0);
  gSystem->mkdir(outDir, true);
  const std::string out = outDir;

  ReducedSample mc, data, empty;
  if (!mc.Open("prompt_mc", promptMcPath)) return 1;
  if (!data.Open("data", dataPath)) return 1;
  if (!empty.Open("emptybx", emptyBxPath)) return 1;

  const HfCollection mcDist = Collect(mc, maxEvents, true);
  const HfCollection dataDist = Collect(data, maxEvents, true);
  const HfCollection emptyDist = Collect(empty, maxEvents, false);

  const int lowPlus = LowTrackMax(dataDist.plusGap);
  const int highPlus = HighTrackMin(dataDist.plusGap);
  const int lowMinus = LowTrackMax(dataDist.minusGap);
  const int highMinus = HighTrackMin(dataDist.minusGap);

  const auto plusRows = ScanSide(dataDist.plusGap.hf.empty() ? SideSample{} : mcDist.plusGap,
                                 dataDist.plusGap,
                                 emptyDist.plusAll,
                                 lowPlus,
                                 highPlus,
                                 1.0,
                                 20.0,
                                 0.1,
                                 targetMcEff,
                                 minLowTrackEff,
                                 maxHighTrackLeak);
  const auto minusRows = ScanSide(dataDist.minusGap.hf.empty() ? SideSample{} : mcDist.minusGap,
                                  dataDist.minusGap,
                                  emptyDist.minusAll,
                                  lowMinus,
                                  highMinus,
                                  1.0,
                                  20.0,
                                  0.1,
                                  targetMcEff,
                                  minLowTrackEff,
                                  maxHighTrackLeak);

  const Recommendation plusRec = Choose("hf_plus_gap", NOM_HF_PLUS, plusRows, lowPlus, highPlus,
                                        targetMcEff, minLowTrackEff, maxHighTrackLeak);
  const Recommendation minusRec = Choose("hf_minus_gap", NOM_HF_MINUS, minusRows, lowMinus, highMinus,
                                         targetMcEff, minLowTrackEff, maxHighTrackLeak);

  WriteScan(out + "/hf_plus_threshold_scan.tsv", plusRows);
  WriteScan(out + "/hf_minus_threshold_scan.tsv", minusRows);
  WriteQuantiles(out, mcDist.plusGap, mcDist.minusGap, dataDist.plusGap, dataDist.minusGap,
                 emptyDist.plusAll, emptyDist.minusAll, lowPlus, highPlus, lowMinus, highMinus);

  std::ofstream rec(out + "/hf_recommended_cutoffs.tsv");
  rec << "side\tnominal\tderived\tmcGapEfficiency\tdataLowTrackPass\tdataHighTrackLeak\tdataAllPass\temptyBxPass\tscore\tnominalMcEfficiency\tnominalDataLowTrackPass\tnominalDataHighTrackLeak\tnominalPassesRule\tnominalScoreOverDerived\tnominalMinusDerived\tlowTrackMax\thighTrackMin\tnote\n";
  rec << std::fixed << std::setprecision(6);
  for (const auto &r : {plusRec, minusRec}) {
    rec << r.side << '\t' << r.nominal << '\t' << r.threshold << '\t'
        << FmtAvailable(r.mcEff, r.hasMcGap, 6) << '\t' << r.dataLowEff << '\t' << r.dataHighLeak << '\t'
        << r.dataAllPass << '\t' << r.emptyPass << '\t' << r.score << '\t'
        << FmtAvailable(r.nominalMcEff, r.nominalHasMcGap, 6) << '\t' << r.nominalDataLowEff << '\t' << r.nominalDataHighLeak << '\t'
        << (r.nominalPassesRule ? "true" : "false") << '\t' << r.nominalScoreOverDerived << '\t'
        << r.nominalMinusDerived << '\t' << r.lowTrackMax << '\t' << r.highTrackMin << '\t' << r.note << '\n';
  }

  SaveDistributionPlot(out, "hf_plus_gap_distributions",
                       "HF plus gap side;HFEMaxPlus (GeV);unit-normalized events",
                       mcDist.plusGap, dataDist.plusGap, emptyDist.plusAll, lowPlus, highPlus,
                       plusRec.threshold, NOM_HF_PLUS);
  SaveDistributionPlot(out, "hf_minus_gap_distributions",
                       "HF minus gap side;HFEMaxMinus (GeV);unit-normalized events",
                       mcDist.minusGap, dataDist.minusGap, emptyDist.minusAll, lowMinus, highMinus,
                       minusRec.threshold, NOM_HF_MINUS);
  SaveScanPlot(out, "hf_plus_threshold_efficiencies", plusRows, plusRec.threshold, NOM_HF_PLUS);
  SaveScanPlot(out, "hf_minus_threshold_efficiencies", minusRows, minusRec.threshold, NOM_HF_MINUS);

  std::ofstream readme(out + "/README.md");
  readme << "# HF Gap Cutoff Derivation\n\n";
  readme << "<!-- DOC_OWNER: cms-analysis-manager HF gap threshold derivation. -->\n";
  readme << "<!-- TOKEN_NOTE: HF-only derivation using MC gap efficiency and data track-multiplicity checks. -->\n\n";
  readme << "## Inputs\n\n";
  readme << "- Prompt MC reduced skim: `" << promptMcPath << "`\n";
  readme << "- 2023 data reduced skim: `" << dataPath << "`\n";
  readme << "- EmptyBX reduced skim: `" << emptyBxPath << "`\n";
  readme << "- Max events per sample: `" << maxEvents << "`\n\n";
  readme << "## Method\n\n";
  readme << "- Use `ZDCgammaN` events for the plus-side HF gap and `ZDCNgamma` events for the minus-side HF gap.\n";
  readme << "- Use prompt MC gap-side HF distributions for signal retention when the same-side reduced MC sample exists; otherwise report MC efficiency as `NA` and use data/EmptyBX controls for that side.\n";
  readme << "- Use data low-track events as a UPC-like control slice and data high-track events as a hadronic-activity proxy.\n";
  readme << "- Low/high track slices are data-defined side-by-side: plus low `nTrack <= " << lowPlus
         << "`, plus high `nTrack >= " << highPlus << "`, minus low `nTrack <= " << lowMinus
         << "`, minus high `nTrack >= " << highMinus << "`.\n";
  readme << "- Blind rule: choose the first threshold satisfying MC efficiency >= `" << targetMcEff
         << "`, data low-track pass >= `" << minLowTrackEff
         << "`, and data high-track leakage <= `" << maxHighTrackLeak << "`.\n\n";
  readme << "## Counts\n\n";
  readme << "| sample | raw | quality | valid HF | plus-gap | minus-gap |\n";
  readme << "|---|---:|---:|---:|---:|---:|\n";
  readme << "| prompt MC | " << mcDist.raw << " | " << mcDist.quality << " | " << mcDist.valid
         << " | " << mcDist.gammaN << " | " << mcDist.Ngamma << " |\n";
  readme << "| data | " << dataDist.raw << " | " << dataDist.quality << " | " << dataDist.valid
         << " | " << dataDist.gammaN << " | " << dataDist.Ngamma << " |\n";
  readme << "| EmptyBX | " << emptyDist.raw << " | " << emptyDist.quality << " | " << emptyDist.valid
         << " | " << emptyDist.gammaN << " | " << emptyDist.Ngamma << " |\n\n";
  readme << "## Blind Derived Cutoffs\n\n";
  readme << "| side | nominal | derived | MC eff. | data low pass | data high leak | nominal MC eff. |\n";
  readme << "|---|---:|---:|---:|---:|---:|---:|\n";
  readme << "| plus gap | " << NOM_HF_PLUS << " | " << plusRec.threshold << " | " << Fmt(plusRec.mcEff)
         << " | " << Fmt(plusRec.dataLowEff) << " | " << Fmt(plusRec.dataHighLeak)
         << " | " << FmtAvailable(plusRec.nominalMcEff, plusRec.nominalHasMcGap) << " |\n";
  readme << "| minus gap | " << NOM_HF_MINUS << " | " << minusRec.threshold << " | " << FmtAvailable(minusRec.mcEff, minusRec.hasMcGap)
         << " | " << Fmt(minusRec.dataLowEff) << " | " << Fmt(minusRec.dataHighLeak)
         << " | " << FmtAvailable(minusRec.nominalMcEff, minusRec.nominalHasMcGap) << " |\n\n";
  readme << "## Interpretation\n\n";
  readme << "- This is a threshold derivation from MC signal retention plus data track-multiplicity behavior; it does not use the old EmptyBX-only score as the decision rule.\n";
  readme << "- The nominal thresholds are retained only as posthoc reference markers in the plots and recommendation table; they cannot change the blind derived threshold.\n";
  readme << "- If the blind derived and nominal values disagree, treat the nominal value as a closure/systematic comparison point rather than silently snapping the cut to the AN.\n\n";
  readme << "## Outputs\n\n";
  readme << "- `hf_recommended_cutoffs.tsv`\n";
  readme << "- `hf_plus_threshold_scan.tsv`\n";
  readme << "- `hf_minus_threshold_scan.tsv`\n";
  readme << "- `hf_quantiles.tsv`\n";
  readme << "- `hf_plus_gap_distributions.png/pdf`\n";
  readme << "- `hf_minus_gap_distributions.png/pdf`\n";
  readme << "- `hf_plus_threshold_efficiencies.png/pdf`\n";
  readme << "- `hf_minus_threshold_efficiencies.png/pdf`\n";

  std::cout << "Wrote HF gap cutoff derivation to " << outDir << std::endl;
  return 0;
}
