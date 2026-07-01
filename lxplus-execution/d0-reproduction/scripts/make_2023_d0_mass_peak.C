#include <TCanvas.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TF1.h>
#include <TGaxis.h>
#include <TH1D.h>
#include <TLegend.h>
#include <TLine.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

const char* kDefaultInput =
    "/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/"
    "Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root";

const char* kDefaultOutputDir =
    "/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/first-mass-peak-2023";

bool isOwnedWritePath(const TString& path) {
  return path.BeginsWith("/afs/cern.ch/user/e/evzhang") ||
         path.BeginsWith("/afs/cern.ch/work/e/evzhang") ||
         path.BeginsWith("/eos/user/e/evzhang");
}

void writeHistogramCsv(const TH1D* hist, const TString& path) {
  std::ofstream out(path.Data());
  out << "bin,low_edge,high_edge,center,count,error\n";
  for (int bin = 1; bin <= hist->GetNbinsX(); ++bin) {
    out << bin << "," << std::setprecision(8) << hist->GetXaxis()->GetBinLowEdge(bin) << ","
        << hist->GetXaxis()->GetBinUpEdge(bin) << "," << hist->GetXaxis()->GetBinCenter(bin)
        << "," << hist->GetBinContent(bin) << "," << hist->GetBinError(bin) << "\n";
  }
}

TH1D* drawMassHistogram(TTree* tree,
                        const TString& histName,
                        const TString& cut,
                        Long64_t nEntries,
                        int color,
                        std::ofstream& manifest) {
  constexpr int kBins = 140;
  constexpr double kLo = 1.70;
  constexpr double kHi = 2.05;

  gROOT->cd();
  auto* hist = new TH1D(histName, "", kBins, kLo, kHi);
  hist->Sumw2();
  TString drawExpr = TString::Format("Dmass>>+%s", histName.Data());
  Long64_t selected = tree->Draw(drawExpr, cut, "goff", nEntries);
  if (!hist) {
    std::cerr << "ERROR: failed to retrieve histogram " << histName << "\n";
    return nullptr;
  }

  hist->SetDirectory(nullptr);
  hist->Sumw2(false);
  hist->SetLineColor(color);
  hist->SetMarkerColor(color);
  hist->SetMarkerStyle(20);
  hist->SetMarkerSize(0.75);
  hist->SetLineWidth(2);
  hist->SetTitle("");
  hist->GetXaxis()->SetTitle("m(K#pi) [GeV]");
  hist->GetYaxis()->SetTitle("Candidates / 2.5 MeV");

  manifest << histName << "\n";
  manifest << "  cut: " << cut << "\n";
  manifest << "  tree_draw_selected_values: " << selected << "\n";
  manifest << "  histogram_entries: " << std::setprecision(12) << hist->GetEntries() << "\n";
  manifest << "  histogram_integral: " << hist->Integral() << "\n";
  manifest << "  max_bin_center: " << hist->GetXaxis()->GetBinCenter(hist->GetMaximumBin()) << "\n";
  manifest << "  max_bin_count: " << hist->GetBinContent(hist->GetMaximumBin()) << "\n\n";

  return hist;
}

void saveCanvas(TH1D* hist,
                const TString& outputStem,
                bool fitSelected,
                std::ofstream& manifest) {
  gStyle->SetOptStat(0);
  TGaxis::SetMaxDigits(3);

  TCanvas canvas("canvas", "canvas", 900, 700);
  canvas.SetLeftMargin(0.12);
  canvas.SetRightMargin(0.04);
  canvas.SetTopMargin(0.08);
  canvas.SetBottomMargin(0.12);

  hist->Draw("E1");

  TF1* fit = nullptr;
  if (fitSelected && hist->Integral() > 0) {
    fit = new TF1("signal_plus_linear_background", "gaus(0)+pol1(3)", 1.78, 1.95);
    fit->SetParameters(hist->GetMaximum(), 1.865, 0.020, hist->GetMinimum(), 0.0);
    fit->SetParLimits(1, 1.82, 1.90);
    fit->SetParLimits(2, 0.004, 0.080);
    int status = hist->Fit(fit, "RQ0");
    fit->SetLineColor(kRed + 1);
    fit->SetLineWidth(2);
    fit->Draw("same");
    manifest << "simple_fit_status: " << status << "\n";
    manifest << "simple_fit_model: gaus(0)+pol1(3) over 1.78-1.95 GeV\n";
    manifest << "simple_fit_mean: " << fit->GetParameter(1) << "\n";
    manifest << "simple_fit_sigma: " << fit->GetParameter(2) << "\n";
    manifest << "simple_fit_chi2: " << fit->GetChisquare() << "\n";
    manifest << "simple_fit_ndf: " << fit->GetNDF() << "\n\n";
  }

  TLine pdgMass(1.86484, 0.0, 1.86484, hist->GetMaximum() * 1.08);
  pdgMass.SetLineColor(kGreen + 2);
  pdgMass.SetLineStyle(2);
  pdgMass.SetLineWidth(2);
  pdgMass.Draw("same");

  TLegend legend(0.50, 0.70, 0.92, 0.88);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.AddEntry(hist, "2023 data candidates", "lep");
  if (fit) {
    legend.AddEntry(fit, "simple Gaussian + linear fit", "l");
  }
  legend.AddEntry(&pdgMass, "PDG D^{0} mass", "l");
  legend.Draw();

  canvas.SaveAs(outputStem + ".png");
  canvas.SaveAs(outputStem + ".pdf");
}

}  // namespace

void make_2023_d0_mass_peak(const char* inputPath = kDefaultInput,
                            const char* outputDir = kDefaultOutputDir,
                            Long64_t maxEntries = 0) {
  TString outDir(outputDir);
  if (!isOwnedWritePath(outDir)) {
    std::cerr << "ERROR: refusing to write outside Evan-owned CERN paths: " << outDir << "\n";
    return;
  }
  gSystem->mkdir(outDir, true);

  std::unique_ptr<TFile> input(TFile::Open(inputPath, "READ"));
  if (!input || input->IsZombie()) {
    std::cerr << "ERROR: failed to open input " << inputPath << "\n";
    return;
  }

  auto* tree = dynamic_cast<TTree*>(input->Get("Tree"));
  if (!tree) {
    std::cerr << "ERROR: input has no TTree named Tree\n";
    return;
  }

  const Long64_t totalEntries = tree->GetEntries();
  const Long64_t nEntries = (maxEntries > 0 && maxEntries < totalEntries) ? maxEntries : totalEntries;

  tree->SetBranchStatus("*", 0);
  for (const char* branch : {"Dmass", "Dpt", "Dy", "DpassCut23PAS", "DpassCutNominal"}) {
    tree->SetBranchStatus(branch, 1);
  }

  TString manifestPath = outDir + "/mass_peak_2023_manifest.txt";
  std::ofstream manifest(manifestPath.Data());
  manifest << "result: first 2023 D0 mass peak from reference skim\n";
  manifest << "input: " << inputPath << "\n";
  manifest << "tree: Tree\n";
  manifest << "total_tree_entries: " << totalEntries << "\n";
  manifest << "processed_tree_entries: " << nEntries << "\n";
  manifest << "max_entries_argument: " << maxEntries << "\n";
  manifest << "mass_range_gev: 1.70,2.05\n";
  manifest << "bins: 140\n";
  manifest << "bin_width_gev: 0.0025\n";
  manifest << "note: reference skim is used only for first top-down validation; it is not the final reproduction starting point.\n\n";

  TString kinematicCut = "Dpt>2 && Dpt<12 && abs(Dy)<2";
  TString selectedCut = kinematicCut + " && DpassCut23PAS";
  TString nominalCut = kinematicCut + " && DpassCutNominal";

  TH1D* hKinematic = drawMassHistogram(tree, "h_mass_kinematic_2pt12_absy2",
                                       kinematicCut, nEntries, kGray + 2, manifest);
  TH1D* hSelected = drawMassHistogram(tree, "h_mass_dpasscut23pas_2pt12_absy2",
                                      selectedCut, nEntries, kBlue + 2, manifest);
  TH1D* hNominal = drawMassHistogram(tree, "h_mass_dpasscutnominal_2pt12_absy2",
                                     nominalCut, nEntries, kMagenta + 2, manifest);
  if (!hKinematic || !hSelected || !hNominal) {
    std::cerr << "ERROR: histogram creation failed\n";
    return;
  }

  writeHistogramCsv(hKinematic, outDir + "/d0_mass_2023_kinematic_2pt12_absy2.csv");
  writeHistogramCsv(hSelected, outDir + "/d0_mass_2023_dpasscut23pas_2pt12_absy2.csv");
  writeHistogramCsv(hNominal, outDir + "/d0_mass_2023_dpasscutnominal_2pt12_absy2.csv");

  saveCanvas(hSelected, outDir + "/d0_mass_2023_dpasscut23pas_2pt12_absy2", true, manifest);
  saveCanvas(hNominal, outDir + "/d0_mass_2023_dpasscutnominal_2pt12_absy2", true, manifest);

  TCanvas overlay("overlay", "overlay", 900, 700);
  overlay.SetLeftMargin(0.12);
  overlay.SetRightMargin(0.04);
  overlay.SetTopMargin(0.08);
  overlay.SetBottomMargin(0.12);
  const double maxY = std::max({hKinematic->GetMaximum(), hSelected->GetMaximum(), hNominal->GetMaximum()});
  hKinematic->SetMaximum(maxY * 1.25);
  hKinematic->GetYaxis()->SetTitle("Candidates / 2.5 MeV");
  hKinematic->Draw("hist");
  hSelected->Draw("E1 same");
  hNominal->Draw("E1 same");
  TLine pdgMass(1.86484, 0.0, 1.86484, maxY * 1.15);
  pdgMass.SetLineColor(kGreen + 2);
  pdgMass.SetLineStyle(2);
  pdgMass.SetLineWidth(2);
  pdgMass.Draw("same");
  TLegend legend(0.50, 0.66, 0.92, 0.88);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.AddEntry(hKinematic, "2 < p_{T} < 12, |y| < 2", "l");
  legend.AddEntry(hSelected, "+ DpassCut23PAS", "lep");
  legend.AddEntry(hNominal, "+ DpassCutNominal", "lep");
  legend.AddEntry(&pdgMass, "PDG D^{0} mass", "l");
  legend.Draw();
  overlay.SaveAs(outDir + "/d0_mass_2023_selection_overlay.png");
  overlay.SaveAs(outDir + "/d0_mass_2023_selection_overlay.pdf");

  TFile outputRoot(outDir + "/d0_mass_2023_histograms.root", "RECREATE");
  hKinematic->Write();
  hSelected->Write();
  hNominal->Write();
  outputRoot.Close();

  manifest << "outputs:\n";
  manifest << "  d0_mass_2023_dpasscut23pas_2pt12_absy2.png/pdf/csv\n";
  manifest << "  d0_mass_2023_dpasscutnominal_2pt12_absy2.png/pdf/csv\n";
  manifest << "  d0_mass_2023_selection_overlay.png/pdf\n";
  manifest << "  d0_mass_2023_histograms.root\n";
  manifest.close();

  std::cout << "Wrote first 2023 D0 mass-peak outputs to " << outDir << "\n";
  std::cout << "Manifest: " << manifestPath << "\n";
}
