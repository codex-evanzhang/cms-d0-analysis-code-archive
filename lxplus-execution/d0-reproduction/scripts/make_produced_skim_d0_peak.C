#include <TCanvas.h>
#include <TFile.h>
#include <TF1.h>
#include <TGaxis.h>
#include <TH1D.h>
#include <TLegend.h>
#include <TLine.h>
#include <TPad.h>
#include <TROOT.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>

#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

const char* kDefaultInput =
    "/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/"
    "recreated-selected-skim-2023/hiForward0_27files_selected_first_pass/section7_d_selected.root";

const char* kDefaultOutputDir =
    "/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/"
    "produced-skim-d0-peak-2023/hiForward0_27files_selected_first_pass";

bool isOwnedWritePath(const TString& path) {
  return path.BeginsWith("/afs/cern.ch/user/e/evzhang") ||
         path.BeginsWith("/afs/cern.ch/work/e/evzhang") ||
         path.BeginsWith("/eos/user/e/evzhang");
}

void writeHistogramCsv(const TH1D* hist, const TString& path) {
  std::ofstream out(path.Data());
  out << "bin,low_edge,high_edge,center,count,error\n";
  for (int bin = 1; bin <= hist->GetNbinsX(); ++bin) {
    out << bin << "," << std::setprecision(8)
        << hist->GetXaxis()->GetBinLowEdge(bin) << ","
        << hist->GetXaxis()->GetBinUpEdge(bin) << ","
        << hist->GetXaxis()->GetBinCenter(bin) << ","
        << hist->GetBinContent(bin) << ","
        << hist->GetBinError(bin) << "\n";
  }
}

TH1D* makeMassHist(TTree* tree,
                   const TString& name,
                   const TString& cut,
                   Long64_t nEntries,
                   int color,
                   std::ofstream& manifest) {
  constexpr int kBins = 140;
  constexpr double kLo = 1.70;
  constexpr double kHi = 2.05;

  gROOT->cd();
  auto* hist = new TH1D(name, "", kBins, kLo, kHi);
  hist->Sumw2();
  const TString drawExpr = TString::Format("Dmass>>+%s", name.Data());
  const Long64_t selected = tree->Draw(drawExpr, cut, "goff", nEntries);

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

  manifest << name << "\n";
  manifest << "  cut: " << cut << "\n";
  manifest << "  tree_draw_selected_values: " << selected << "\n";
  manifest << "  histogram_entries: " << std::setprecision(12) << hist->GetEntries() << "\n";
  manifest << "  histogram_integral: " << hist->Integral() << "\n";
  manifest << "  max_bin_center: " << hist->GetXaxis()->GetBinCenter(hist->GetMaximumBin()) << "\n";
  manifest << "  max_bin_count: " << hist->GetBinContent(hist->GetMaximumBin()) << "\n\n";

  return hist;
}

TF1* fitAndDraw(TH1D* hist, std::ofstream& manifest) {
  TF1* fit = nullptr;
  if (hist->Integral() > 0) {
    fit = new TF1("signal_plus_linear_background", "gaus(0)+pol1(3)", 1.78, 1.95);
    fit->SetParameters(hist->GetMaximum(), 1.865, 0.020, 0.0, 0.0);
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
  return fit;
}

void saveMainPlot(TH1D* hist, const TString& outputStem, std::ofstream& manifest) {
  gStyle->SetOptStat(0);
  TGaxis::SetMaxDigits(3);

  TCanvas canvas("produced_skim_mass", "produced_skim_mass", 900, 700);
  canvas.SetLeftMargin(0.12);
  canvas.SetRightMargin(0.04);
  canvas.SetTopMargin(0.08);
  canvas.SetBottomMargin(0.12);

  hist->Draw("E1");
  TF1* fit = fitAndDraw(hist, manifest);

  TLine pdgMass(1.86484, 0.0, 1.86484, hist->GetMaximum() * 1.10);
  pdgMass.SetLineColor(kGreen + 2);
  pdgMass.SetLineStyle(2);
  pdgMass.SetLineWidth(2);
  pdgMass.Draw("same");

  TLegend legend(0.48, 0.70, 0.92, 0.88);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.AddEntry(hist, "recreated SelectedD skim", "lep");
  if (fit) {
    legend.AddEntry(fit, "Gaussian + linear fit", "l");
  }
  legend.AddEntry(&pdgMass, "PDG D^{0} mass", "l");
  legend.Draw();

  canvas.SaveAs(outputStem + ".png");
  canvas.SaveAs(outputStem + ".pdf");
}

void savePtYGrid(const std::vector<TH1D*>& hists, const TString& outputStem) {
  TCanvas canvas("pt_y_grid", "pt_y_grid", 1500, 1000);
  canvas.Divide(4, 3, 0.001, 0.001);
  for (int i = 0; i < static_cast<int>(hists.size()); ++i) {
    canvas.cd(i + 1);
    gPad->SetLeftMargin(0.14);
    gPad->SetRightMargin(0.04);
    gPad->SetTopMargin(0.10);
    gPad->SetBottomMargin(0.13);
    hists[i]->Draw("hist");
    TLine pdgMass(1.86484, 0.0, 1.86484, hists[i]->GetMaximum() * 1.08);
    pdgMass.SetLineColor(kGreen + 2);
    pdgMass.SetLineStyle(2);
    pdgMass.Draw("same");
  }
  canvas.SaveAs(outputStem + ".png");
  canvas.SaveAs(outputStem + ".pdf");
}

}  // namespace

void make_produced_skim_d0_peak(const char* inputPath = kDefaultInput,
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

  auto* tree = dynamic_cast<TTree*>(input->Get("SelectedD"));
  if (!tree) {
    std::cerr << "ERROR: input has no TTree named SelectedD\n";
    return;
  }

  const Long64_t totalEntries = tree->GetEntries();
  const Long64_t nEntries = (maxEntries > 0 && maxEntries < totalEntries) ? maxEntries : totalEntries;

  tree->SetBranchStatus("*", 0);
  for (const char* branch : {"Dmass", "Dpt", "Dy"}) {
    tree->SetBranchStatus(branch, 1);
  }

  TString manifestPath = outDir + "/produced_skim_d0_peak_manifest.txt";
  std::ofstream manifest(manifestPath.Data());
  manifest << "result: 2023 D0 peak from reproduced SelectedD skim\n";
  manifest << "input: " << inputPath << "\n";
  manifest << "tree: SelectedD\n";
  manifest << "total_tree_entries: " << totalEntries << "\n";
  manifest << "processed_tree_entries: " << nEntries << "\n";
  manifest << "max_entries_argument: " << maxEntries << "\n";
  manifest << "mass_range_gev: 1.70,2.05\n";
  manifest << "bins: 140\n";
  manifest << "bin_width_gev: 0.0025\n";
  manifest << "selection_note: SelectedD already contains the recreated first-pass D0 selection; this plot reapplies only 2<Dpt<12 and |Dy|<2.\n\n";

  const TString standardCut = "Dmass>1.70 && Dmass<2.05 && Dpt>2 && Dpt<12 && abs(Dy)<2";
  TH1D* hAll = makeMassHist(tree, "h_selected_d0_mass_2pt12_absy2", standardCut, nEntries, kBlue + 2, manifest);
  saveMainPlot(hAll, outDir + "/d0_mass_produced_skim_selected", manifest);
  writeHistogramCsv(hAll, outDir + "/d0_mass_produced_skim_selected.csv");

  const std::array<double, 4> ptEdges = {2.0, 5.0, 8.0, 12.0};
  const std::array<double, 5> yEdges = {-2.0, -1.0, 0.0, 1.0, 2.0};
  std::ofstream counts((outDir + "/pt_y_bin_counts.csv").Data());
  counts << "pt_low,pt_high,y_low,y_high,entries,max_bin_center,max_bin_count\n";

  std::vector<TH1D*> gridHists;
  for (int ipt = 0; ipt < 3; ++ipt) {
    for (int iy = 0; iy < 4; ++iy) {
      TString name = TString::Format("h_pt%d_y%d", ipt, iy);
      TString title = TString::Format("%.0f<p_{T}<%.0f, %.0f<y<%.0f",
                                      ptEdges[ipt], ptEdges[ipt + 1], yEdges[iy], yEdges[iy + 1]);
      TString cut = TString::Format("Dmass>1.70 && Dmass<2.05 && Dpt>%.6g && Dpt<%.6g && Dy>%.6g && Dy<%.6g",
                                    ptEdges[ipt], ptEdges[ipt + 1], yEdges[iy], yEdges[iy + 1]);
      TH1D* hist = makeMassHist(tree, name, cut, nEntries, kBlue + 2, manifest);
      hist->SetTitle(title);
      gridHists.push_back(hist);
      counts << ptEdges[ipt] << "," << ptEdges[ipt + 1] << ","
             << yEdges[iy] << "," << yEdges[iy + 1] << ","
             << hist->Integral() << ","
             << hist->GetXaxis()->GetBinCenter(hist->GetMaximumBin()) << ","
             << hist->GetBinContent(hist->GetMaximumBin()) << "\n";
    }
  }
  counts.close();
  savePtYGrid(gridHists, outDir + "/d0_mass_produced_skim_pt_y_grid");

  TFile outputRoot(outDir + "/d0_mass_produced_skim_histograms.root", "RECREATE");
  hAll->Write();
  for (TH1D* hist : gridHists) {
    hist->Write();
  }
  outputRoot.Close();

  manifest << "outputs:\n";
  manifest << "  d0_mass_produced_skim_selected.png/pdf/csv\n";
  manifest << "  d0_mass_produced_skim_pt_y_grid.png/pdf\n";
  manifest << "  pt_y_bin_counts.csv\n";
  manifest << "  d0_mass_produced_skim_histograms.root\n";
  manifest.close();

  std::cout << "Wrote produced-skim D0 mass-peak outputs to " << outDir << "\n";
  std::cout << "Manifest: " << manifestPath << "\n";
}
