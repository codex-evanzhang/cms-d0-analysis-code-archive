#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TF1.h"
#include "TFile.h"
#include "TH1D.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLine.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"

namespace {

struct Samples {
  std::vector<double> plus;
  std::vector<double> minus;
  std::vector<double> emptyPlus;
  std::vector<double> emptyMinus;
  Long64_t dataScanned = 0;
  Long64_t emptyScanned = 0;
};

struct PeakFit {
  std::string side;
  int peak = 0;
  double seed = std::numeric_limits<double>::quiet_NaN();
  double mean = std::numeric_limits<double>::quiet_NaN();
  double sigma = std::numeric_limits<double>::quiet_NaN();
  double amplitude = std::numeric_limits<double>::quiet_NaN();
  double chi2ndf = std::numeric_limits<double>::quiet_NaN();
  int status = -1;
};

bool has_branch(TTree *tree, const char *name) {
  return tree && tree->GetBranch(name);
}

bool valid(double value) {
  return std::isfinite(value) && value > -9000.0;
}

void stamp(TCanvas &canvas, const char *text) {
  TLatex latex;
  latex.SetNDC();
  latex.SetTextFont(42);
  latex.SetTextSize(0.030);
  latex.DrawLatex(0.12, 0.93, text);
}

void normalize(TH1D *hist) {
  if (hist && hist->Integral() > 0.0) hist->Scale(1.0 / hist->Integral());
}

void save_canvas(TCanvas &canvas, const std::string &out, const std::string &name) {
  canvas.SaveAs((out + "/" + name + ".png").c_str());
  canvas.SaveAs((out + "/" + name + ".pdf").c_str());
}

void load_tree_sums(const char *path,
                    Long64_t maxEntries,
                    bool requireQuality,
                    std::vector<double> &plus,
                    std::vector<double> &minus,
                    Long64_t &scanned) {
  TFile file(path, "READ");
  if (file.IsZombie()) {
    std::cerr << "could not open " << path << "\n";
    return;
  }
  auto *tree = static_cast<TTree *>(file.Get("Tree"));
  if (!tree) {
    std::cerr << "missing reduced Tree in " << path << "\n";
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

  const bool hasSelectedVtx = has_branch(tree, "selectedVtxFilter");
  const bool hasCluster = has_branch(tree, "ClusterCompatibilityFilter");
  const bool hasNVtx = has_branch(tree, "nVtx");
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
  scanned = n;
  plus.reserve(std::min<Long64_t>(n, 1000000));
  minus.reserve(std::min<Long64_t>(n, 1000000));
  for (Long64_t i = 0; i < n; ++i) {
    tree->GetEntry(i);
    if (requireQuality) {
      if (hasSelectedVtx && !selectedVtxFilter) continue;
      if (hasCluster && !clusterCompatibilityFilter) continue;
      if (hasNVtx && (nVtx <= 0 || nVtx > 3)) continue;
    }
    if (valid(zdcPlus)) plus.push_back(zdcPlus);
    if (valid(zdcMinus)) minus.push_back(zdcMinus);
  }
}

TH1D *make_hist(const char *name, const char *title, const std::vector<double> &values) {
  auto *hist = new TH1D(name, title, 240, 0.0, 12000.0);
  hist->SetDirectory(nullptr);
  for (double value : values) hist->Fill(value);
  return hist;
}

double smooth_bin(const TH1D *hist, int bin, int halfWindow = 2) {
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

PeakFit fit_peak(TH1D *hist, const std::string &side, int peak, double lo, double hi) {
  PeakFit out;
  out.side = side;
  out.peak = peak;
  const int loBin = hist->FindBin(lo);
  const int hiBin = hist->FindBin(hi);
  int maxBin = loBin;
  double maxVal = -1.0;
  for (int bin = loBin; bin <= hiBin; ++bin) {
    const double value = smooth_bin(hist, bin, 1);
    if (value > maxVal) {
      maxVal = value;
      maxBin = bin;
    }
  }
  out.seed = hist->GetBinCenter(maxBin);
  const double widthGuess = std::max(450.0, 450.0 * std::sqrt(static_cast<double>(peak)));
  const double fitLo = std::max(0.0, out.seed - widthGuess);
  const double fitHi = std::min(12000.0, out.seed + widthGuess);
  TF1 fit((side + "_peak" + std::to_string(peak)).c_str(), "gaus(0)+pol1(3)", fitLo, fitHi);
  fit.SetParameters(std::max(1.0, maxVal), out.seed, 350.0 * std::sqrt(static_cast<double>(peak)), 1.0, 0.0);
  fit.SetParLimits(0, 0.0, std::max(10.0, maxVal * 20.0));
  fit.SetParLimits(1, lo, hi);
  fit.SetParLimits(2, 80.0, 2500.0);
  auto status = hist->Fit(&fit, "RQS0");
  out.status = static_cast<int>(status);
  out.amplitude = fit.GetParameter(0);
  out.mean = fit.GetParameter(1);
  out.sigma = std::abs(fit.GetParameter(2));
  out.chi2ndf = fit.GetNDF() > 0 ? fit.GetChisquare() / fit.GetNDF() : std::numeric_limits<double>::quiet_NaN();
  return out;
}

std::vector<PeakFit> fit_side(TH1D *hist, const std::string &side) {
  std::vector<PeakFit> out;
  out.push_back(fit_peak(hist, side, 1, 1500.0, 3600.0));
  const double one = out.back().mean;
  if (std::isfinite(one) && one > 500.0) {
    out.push_back(fit_peak(hist, side, 2, std::max(2500.0, 1.45 * one), std::min(7500.0, 2.70 * one)));
    out.push_back(fit_peak(hist, side, 3, std::max(4200.0, 2.25 * one), std::min(11000.0, 3.80 * one)));
  } else {
    out.push_back(fit_peak(hist, side, 2, 3000.0, 7000.0));
    out.push_back(fit_peak(hist, side, 3, 5000.0, 10000.0));
  }
  return out;
}

void draw_side(TH1D *data, TH1D *empty, const std::vector<PeakFit> &fits,
               const std::string &out, const std::string &side, double nominalThreshold) {
  TCanvas canvas(("c_zdc_" + side + "_multipeak").c_str(), "", 1000, 720);
  canvas.SetTicks();
  canvas.SetLogy();
  normalize(data);
  normalize(empty);
  data->SetStats(false);
  data->SetLineColor(kBlack);
  data->SetLineWidth(3);
  data->SetMinimum(1e-8);
  data->SetMaximum(std::max(1e-5, data->GetMaximum() * 6.0));
  data->GetXaxis()->SetTitle(("ZDC " + side + " sum").c_str());
  data->GetYaxis()->SetTitle("unit-normalized events");
  data->Draw("HIST");
  empty->SetLineColor(kBlue + 1);
  empty->SetLineWidth(2);
  empty->Draw("HIST same");

  TLegend legend(0.56, 0.62, 0.89, 0.89);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.AddEntry(data, "2023 data reduced tree", "l");
  legend.AddEntry(empty, "EmptyBX reduced tree", "l");
  int colors[] = {kRed + 1, kMagenta + 1, kGreen + 2};
  for (size_t i = 0; i < fits.size(); ++i) {
    if (!std::isfinite(fits[i].mean)) continue;
    TLine *line = new TLine(fits[i].mean, 1e-8, fits[i].mean, data->GetMaximum() / 1.4);
    line->SetLineColor(colors[i % 3]);
    line->SetLineStyle(2);
    line->SetLineWidth(2);
    line->Draw();
    legend.AddEntry(line, (std::to_string(fits[i].peak) + "n fitted mean").c_str(), "l");
  }
  TLine *nom = new TLine(nominalThreshold, 1e-8, nominalThreshold, data->GetMaximum() / 2.0);
  nom->SetLineColor(kOrange + 7);
  nom->SetLineWidth(2);
  nom->Draw();
  legend.AddEntry(nom, "AN nominal 1n threshold", "l");
  legend.Draw();
  stamp(canvas, ("ZDC " + side + " 1n/2n/3n peak proxy fits").c_str());
  save_canvas(canvas, out, "zdc_" + side + "_multipeak_fits");
}

void write_branch_audit(const char *dataPath, const std::string &out) {
  TFile file(dataPath, "READ");
  auto *tree = static_cast<TTree *>(file.Get("Tree"));
  std::vector<std::string> wanted = {
      "zside", "section", "channel", "zdcTs2", "zdcTs3", "zdcSOI", "zdcSOIplus1",
      "zdcHadronicChannelSOI", "zdcHadronicChannelSOIplus1", "zdcrechit", "zdcdigi"};
  std::ofstream audit(out + "/zdc_channel_branch_audit.tsv");
  audit << "branch\tavailable\tnote\n";
  for (const auto &name : wanted) {
    const bool available = tree && tree->GetBranch(name.c_str());
    audit << name << '\t' << (available ? "true" : "false") << '\t'
          << (available ? "branch present in reduced Tree" : "not present in reduced Tree; raw channel/SOI plot needs full ZDC rechit/digi forest")
          << '\n';
  }
}

void write_outputs(const char *outDir, const char *dataPath, const char *emptyPath,
                   Long64_t maxEntries, const Samples &samples,
                   const std::vector<PeakFit> &plusFits,
                   const std::vector<PeakFit> &minusFits) {
  const std::string out(outDir);
  std::ofstream table(out + "/zdc_multipeak_fit_params.tsv");
  table << "side\tpeak_n\tseed\tmean\tsigma\tamplitude\tchi2ndf\tfit_status\n";
  table << std::fixed << std::setprecision(6);
  for (const auto &fit : plusFits) {
    table << fit.side << '\t' << fit.peak << '\t' << fit.seed << '\t' << fit.mean << '\t'
          << fit.sigma << '\t' << fit.amplitude << '\t' << fit.chi2ndf << '\t' << fit.status << '\n';
  }
  for (const auto &fit : minusFits) {
    table << fit.side << '\t' << fit.peak << '\t' << fit.seed << '\t' << fit.mean << '\t'
          << fit.sigma << '\t' << fit.amplitude << '\t' << fit.chi2ndf << '\t' << fit.status << '\n';
  }

  std::ofstream summary(out + "/zdc_multipeak_summary.tsv");
  summary << "metric\tvalue\n";
  summary << "data_input\t" << dataPath << "\n";
  summary << "emptybx_input\t" << emptyPath << "\n";
  summary << "max_entries\t" << maxEntries << "\n";
  summary << "data_entries_scanned\t" << samples.dataScanned << "\n";
  summary << "emptybx_entries_scanned\t" << samples.emptyScanned << "\n";
  summary << "data_plus_values\t" << samples.plus.size() << "\n";
  summary << "data_minus_values\t" << samples.minus.size() << "\n";
  summary << "empty_plus_values\t" << samples.emptyPlus.size() << "\n";
  summary << "empty_minus_values\t" << samples.emptyMinus.size() << "\n";
  summary << "classification\ttwiki-official-reduced-tree proxy; not recreated MINIAOD forest\n";

  write_branch_audit(dataPath, out);

  std::ofstream readme(out + "/README.md");
  readme << "# ZDC Multi-Peak Fit Proxy\n\n";
  readme << "<!-- DOC_OWNER: cms-analysis-manager ZDC AN-gap producer. -->\n";
  readme << "<!-- TOKEN_NOTE: reduced-tree ZDC sum peak fits; rerun script instead of rescanning manually. -->\n\n";
  readme << "## Inputs\n\n";
  readme << "- Data input: `" << dataPath << "`\n";
  readme << "- EmptyBX input: `" << emptyPath << "`\n";
  readme << "- Max entries: `" << maxEntries << "`\n\n";
  readme << "## Classification\n\n";
  readme << "`twiki-official-reduced-tree proxy`. This uses the AN/TWiki pre-forested reduced tree because the recreated 98-forest selected skim lacks enough ZDC raw-channel information for AN appendix plots.\n\n";
  readme << "## Outputs\n\n";
  readme << "- `zdc_plus_multipeak_fits.png/pdf`\n";
  readme << "- `zdc_minus_multipeak_fits.png/pdf`\n";
  readme << "- `zdc_multipeak_fit_params.tsv`\n";
  readme << "- `zdc_channel_branch_audit.tsv`\n\n";
  readme << "## Caveats\n\n";
  readme << "- Fits are independent Gaussian-plus-linear local peak fits, not the final AN multi-component ZDC calibration model.\n";
  readme << "- Raw SOI/SOI+1 channel plots require full ZDC rechit/digi branches. The current reduced tree only supports ZDC sum plots.\n";
}

}  // namespace

void render_zdc_multipeak_fits(const char *dataPath,
                               const char *emptyBxPath,
                               const char *outDir,
                               Long64_t maxEntries = 10000000) {
  gStyle->SetOptStat(0);
  gSystem->mkdir(outDir, true);

  Samples samples;
  load_tree_sums(dataPath, maxEntries, true, samples.plus, samples.minus, samples.dataScanned);
  load_tree_sums(emptyBxPath, maxEntries, false, samples.emptyPlus, samples.emptyMinus, samples.emptyScanned);

  auto *hPlus = make_hist("hZdcPlusData", "ZDC plus data", samples.plus);
  auto *hMinus = make_hist("hZdcMinusData", "ZDC minus data", samples.minus);
  auto *hEmptyPlus = make_hist("hZdcPlusEmpty", "ZDC plus EmptyBX", samples.emptyPlus);
  auto *hEmptyMinus = make_hist("hZdcMinusEmpty", "ZDC minus EmptyBX", samples.emptyMinus);

  auto plusFits = fit_side(hPlus, "plus");
  auto minusFits = fit_side(hMinus, "minus");

  draw_side(hPlus, hEmptyPlus, plusFits, outDir, "plus", 1100.0);
  draw_side(hMinus, hEmptyMinus, minusFits, outDir, "minus", 1000.0);
  write_outputs(outDir, dataPath, emptyBxPath, maxEntries, samples, plusFits, minusFits);

  delete hPlus;
  delete hMinus;
  delete hEmptyPlus;
  delete hEmptyMinus;

  std::cout << "ZDC_MULTIPEAK_STATUS ok outdir=" << outDir << "\n";
}
