#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TDirectory.h"
#include "TF1.h"
#include "TFile.h"
#include "TGraphErrors.h"
#include "TH1.h"
#include "TH2.h"
#include "TKey.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLine.h"
#include "TStyle.h"
#include "TTree.h"
#include "TSystem.h"

namespace {

struct BinDef {
  const char *label;
  double ptMin;
  double ptMax;
  double absYMin;
  double absYMax;
};

const std::vector<BinDef> kBins = {
    {"absy0to1_pt2to5", 2.0, 5.0, 0.0, 1.0},
    {"absy0to1_pt5to8", 5.0, 8.0, 0.0, 1.0},
    {"absy0to1_pt8to12", 8.0, 12.0, 0.0, 1.0},
    {"absy1to2_pt2to5", 2.0, 5.0, 1.0, 2.0},
    {"absy1to2_pt5to8", 5.0, 8.0, 1.0, 2.0},
    {"absy1to2_pt8to12", 8.0, 12.0, 1.0, 2.0},
};

void ensure_dir(const std::string &path) {
  gSystem->mkdir(path.c_str(), true);
}

void stamp(TCanvas &canvas, const char *text = "Recreated 2023 D^{0} workflow") {
  TLatex latex;
  latex.SetNDC();
  latex.SetTextSize(0.032);
  latex.SetTextFont(42);
  latex.DrawLatex(0.12, 0.93, text);
}

void save_canvas(TCanvas &canvas, const std::string &outdir, const std::string &name) {
  canvas.SaveAs((outdir + "/" + name + ".png").c_str());
  canvas.SaveAs((outdir + "/" + name + ".pdf").c_str());
}

void draw_hist(TFile &file, const char *histName, const std::string &outdir, const std::string &name,
               const char *xTitle = "", bool logy = false) {
  auto *hist = dynamic_cast<TH1 *>(file.Get(histName));
  if (!hist) {
    std::cerr << "missing histogram " << histName << "\n";
    return;
  }
  TCanvas canvas(("c_" + name).c_str(), "", 900, 700);
  canvas.SetTicks();
  canvas.SetLogy(logy);
  hist->SetLineColor(kBlue + 1);
  hist->SetMarkerColor(kBlue + 1);
  hist->SetMarkerStyle(20);
  hist->SetStats(false);
  if (std::string(xTitle).size()) hist->GetXaxis()->SetTitle(xTitle);
  hist->GetYaxis()->SetTitle("Entries");
  hist->Draw(hist->InheritsFrom("TH2") ? "COLZ" : "HIST E");
  stamp(canvas);
  save_canvas(canvas, outdir, name);
}

void draw_cutflow(TFile &file, const char *histName, const std::string &outdir, const std::string &name) {
  auto *hist = dynamic_cast<TH1 *>(file.Get(histName));
  if (!hist) return;
  TCanvas canvas(("c_" + name).c_str(), "", 1000, 700);
  canvas.SetTicks();
  canvas.SetGridy();
  hist->SetStats(false);
  hist->SetFillColor(kAzure - 9);
  hist->SetLineColor(kAzure + 2);
  hist->LabelsOption("v", "X");
  hist->GetYaxis()->SetTitle("Entries");
  hist->Draw("HIST TEXT0");
  stamp(canvas);
  save_canvas(canvas, outdir, name);
}

void draw_grid(TFile &file, const std::string &prefix, const std::string &outdir, const std::string &name,
               const char *xTitle, bool logy = false) {
  TCanvas canvas(("c_" + name).c_str(), "", 1500, 900);
  canvas.Divide(3, 2, 0.001, 0.001);
  for (size_t i = 0; i < kBins.size(); ++i) {
    canvas.cd(static_cast<int>(i + 1));
    gPad->SetTicks();
    gPad->SetLogy(logy);
    std::string histName = prefix + "_" + kBins[i].label;
    auto *hist = dynamic_cast<TH1 *>(file.Get(histName.c_str()));
    if (!hist) continue;
    hist->SetStats(false);
    hist->SetLineColor(kBlue + 1);
    hist->SetMarkerColor(kBlue + 1);
    hist->GetXaxis()->SetTitle(xTitle);
    hist->GetYaxis()->SetTitle("Candidates");
    hist->Draw("HIST");
    TLatex label;
    label.SetNDC();
    label.SetTextSize(0.05);
    label.DrawLatex(0.16, 0.82, kBins[i].label);
  }
  canvas.cd();
  stamp(canvas);
  save_canvas(canvas, outdir, name);
}

double bin_width(TH1 *hist) {
  return hist ? hist->GetXaxis()->GetBinWidth(1) : 0.0;
}

void draw_mass_fits(TTree *tree, const std::string &outdir) {
  if (!tree) return;
  std::ofstream summary(outdir + "/mass_fit_summary.tsv");
  summary << "bin\tentries\tmean\tsigma\tyield_gaus\tchi2ndf\tfit_status\n";

  TCanvas allCanvas("c_mass_fit_all", "", 900, 700);
  auto *hAll = new TH1D("hAllMassFit", "Selected D^{0};m_{K#pi} (GeV);Candidates / 5 MeV", 70, 1.70, 2.05);
  tree->Draw("Dmass>>hAllMassFit", "Dpt>2 && Dpt<12 && abs(Dy)<2", "goff");
  hAll->SetStats(false);
  hAll->SetMarkerStyle(20);
  hAll->SetMarkerColor(kBlack);
  hAll->SetLineColor(kBlack);
  TF1 fAll("fAll", "gaus(0)+pol1(3)", 1.75, 1.98);
  fAll.SetParameters(hAll->GetMaximum(), 1.865, 0.018, 1.0, 0.0);
  fAll.SetParLimits(0, 0.0, std::max(1.0, hAll->GetMaximum() * 20.0));
  fAll.SetParLimits(1, 1.82, 1.90);
  fAll.SetParLimits(2, 0.005, 0.060);
  hAll->Fit(&fAll, "RQ0");
  hAll->Draw("E");
  fAll.SetLineColor(kRed + 1);
  fAll.Draw("same");
  stamp(allCanvas);
  save_canvas(allCanvas, outdir, "mass_fit_all_selected");
  summary << "all\t" << hAll->GetEntries() << "\t" << fAll.GetParameter(1) << "\t" << std::abs(fAll.GetParameter(2))
          << "\t" << fAll.GetParameter(0) * std::abs(fAll.GetParameter(2)) * std::sqrt(2.0 * M_PI) / bin_width(hAll)
          << "\t" << (fAll.GetNDF() > 0 ? fAll.GetChisquare() / fAll.GetNDF() : -1) << "\tok\n";

  TCanvas grid("c_mass_fit_grid", "", 1500, 900);
  grid.Divide(3, 2, 0.001, 0.001);
  TCanvas pullGrid("c_mass_fit_pull_grid", "", 1500, 900);
  pullGrid.Divide(3, 2, 0.001, 0.001);
  for (size_t i = 0; i < kBins.size(); ++i) {
    grid.cd(static_cast<int>(i + 1));
    gPad->SetTicks();
    const auto &bin = kBins[i];
    std::string hname = std::string("hMassFit_") + bin.label;
    auto *hist = new TH1D(hname.c_str(), ";m_{K#pi} (GeV);Candidates / 5 MeV", 70, 1.70, 2.05);
    std::string cut = Form("Dpt>%g && Dpt<%g && abs(Dy)>%g && abs(Dy)<%g", bin.ptMin, bin.ptMax, bin.absYMin, bin.absYMax);
    tree->Draw((std::string("Dmass>>") + hname).c_str(), cut.c_str(), "goff");
    hist->SetStats(false);
    hist->SetMarkerStyle(20);
    hist->SetMarkerSize(0.7);
    hist->SetLineColor(kBlack);
    hist->Draw("E");
    TF1 fit((std::string("f_") + bin.label).c_str(), "gaus(0)+pol1(3)", 1.75, 1.98);
    fit.SetParameters(std::max(1.0, hist->GetMaximum()), 1.865, 0.018, 1.0, 0.0);
    fit.SetParLimits(0, 0.0, std::max(1.0, hist->GetMaximum() * 20.0));
    fit.SetParLimits(1, 1.82, 1.90);
    fit.SetParLimits(2, 0.005, 0.060);
    auto *pull = new TH1D((std::string("hPull_") + bin.label).c_str(), ";m_{K#pi} (GeV);Pull", 70, 1.70, 2.05);
    if (hist->GetEntries() >= 25) {
      hist->Fit(&fit, "RQ0");
      fit.SetLineColor(kRed + 1);
      fit.Draw("same");
      summary << bin.label << "\t" << hist->GetEntries() << "\t" << fit.GetParameter(1) << "\t"
              << std::abs(fit.GetParameter(2)) << "\t"
              << fit.GetParameter(0) * std::abs(fit.GetParameter(2)) * std::sqrt(2.0 * M_PI) / bin_width(hist)
              << "\t" << (fit.GetNDF() > 0 ? fit.GetChisquare() / fit.GetNDF() : -1) << "\tok\n";
      for (int ib = 1; ib <= hist->GetNbinsX(); ++ib) {
        const double x = hist->GetBinCenter(ib);
        if (x < 1.75 || x > 1.98) continue;
        const double obs = hist->GetBinContent(ib);
        const double exp = fit.Eval(x);
        const double err = obs > 0 ? std::sqrt(obs) : 1.0;
        pull->SetBinContent(ib, (obs - exp) / err);
      }
    } else {
      summary << bin.label << "\t" << hist->GetEntries() << "\t-999\t-999\t-999\t-999\tskipped_low_entries\n";
    }
    TLatex label;
    label.SetNDC();
    label.SetTextSize(0.055);
    label.DrawLatex(0.16, 0.82, bin.label);
    pullGrid.cd(static_cast<int>(i + 1));
    gPad->SetTicks();
    gPad->SetGridy();
    pull->SetStats(false);
    pull->SetLineColor(kBlue + 1);
    pull->SetMarkerColor(kBlue + 1);
    pull->SetMarkerStyle(20);
    pull->SetMinimum(-5.0);
    pull->SetMaximum(5.0);
    pull->Draw("E");
    TLine zero(1.70, 0.0, 2.05, 0.0);
    zero.SetLineStyle(2);
    zero.Draw("same");
    label.DrawLatex(0.16, 0.82, bin.label);
  }
  grid.cd();
  stamp(grid);
  save_canvas(grid, outdir, "mass_fit_grid_selected");
  pullGrid.cd();
  stamp(pullGrid, "Fit pulls; bins with low statistics are left empty");
  save_canvas(pullGrid, outdir, "mass_fit_pull_grid_selected");
}

void write_manifest(const std::string &outdir) {
  std::ofstream manifest(outdir + "/README.md");
  manifest << "# Recreated AN Proxy Plots\n\n";
  manifest << "<!-- DOC_OWNER: cms-analysis-manager generated ROOT plot bundle. -->\n";
  manifest << "<!-- TOKEN_NOTE: generated by render_recreated_an_proxy_plots.C. -->\n\n";
  manifest << "These plots are generated from the reproduced 98-forest Section 5/7 selected outputs. ";
  manifest << "They are intended to populate AN-style plot slots where current reproduced data exist. ";
  manifest << "They are not final cross-section, efficiency, or theory plots.\n";
}

}  // namespace

int render_recreated_an_proxy_plots(
    const char *section5Path = "/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/crab_98forest_apriori_20260619/section5_event_selected.root",
    const char *section7Path = "/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/crab_98forest_apriori_20260619/section7_d_selected.root",
    const char *outdir = "/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/an-proxy-plots/latest") {
  gStyle->SetOptStat(0);
  gStyle->SetOptFit(0);
  std::string out(outdir);
  ensure_dir(out);

  TFile section5(section5Path, "READ");
  TFile section7(section7Path, "READ");
  if (section5.IsZombie() || section7.IsZombie()) {
    std::cerr << "ERROR failed to open input ROOT files\n";
    return 2;
  }

  draw_cutflow(section5, "hEventCutFlow", out, "event_cutflow_section5");
  draw_hist(section5, "hBestVz_before_cut", out, "pv_z_before_cut", "best vertex z (cm)", true);
  draw_hist(section5, "hNVtx_before_cut", out, "vertex_multiplicity_before_cut", "n_{vtx}", true);
  draw_hist(section5, "hZDCPlus_before_cut", out, "zdc_plus_before_xor", "ZDC plus energy (GeV)", true);
  draw_hist(section5, "hZDCMinus_before_cut", out, "zdc_minus_before_xor", "ZDC minus energy (GeV)", true);
  draw_hist(section5, "hZDCPlusMinus_before_cut", out, "zdc_plus_minus_correlation", "ZDC plus energy (GeV)", false);
  draw_hist(section5, "hHFEMaxPlus_before_gap_cut", out, "hf_plus_emax_before_gap", "HF plus E_{max} (GeV)", true);
  draw_hist(section5, "hHFEMaxMinus_before_gap_cut", out, "hf_minus_emax_before_gap", "HF minus E_{max} (GeV)", true);
  draw_hist(section5, "hHFEMaxGapSide_before_gap_cut", out, "hf_gap_side_emax_before_gap", "0n-side HF E_{max} (GeV)", true);

  draw_cutflow(section7, "hDCutFlow", out, "d_candidate_cutflow_section7");
  draw_hist(section7, "hDmass_before_cut", out, "d_mass_before_d_cuts", "m_{K#pi} (GeV)", true);
  draw_hist(section7, "hDmass_selected", out, "d_mass_selected_recreated", "m_{K#pi} (GeV)", false);
  draw_hist(section7, "hDtrk1Pt_before_cut", out, "daughter1_pt_before_cut", "track 1 p_{T} (GeV)", true);
  draw_hist(section7, "hDtrk2Pt_before_cut", out, "daughter2_pt_before_cut", "track 2 p_{T} (GeV)", true);
  draw_hist(section7, "hDtrk1RelPtErr_before_cut", out, "daughter1_rel_pt_err_before_cut", "track 1 #sigma(p_{T})/p_{T}", true);
  draw_hist(section7, "hDtrk2RelPtErr_before_cut", out, "daughter2_rel_pt_err_before_cut", "track 2 #sigma(p_{T})/p_{T}", true);
  draw_hist(section7, "hDpt_before_cut", out, "d_pt_before_cut", "D^{0} p_{T} (GeV)", true);
  draw_hist(section7, "hDy_before_cut", out, "d_y_before_cut", "D^{0} y", true);

  draw_grid(section7, "hDalpha_before_cut", out, "topology_alpha_grid_before_cut", "D #alpha", true);
  draw_grid(section7, "hDsvpvSig_before_cut", out, "topology_svpv_sig_grid_before_cut", "L_{3D}/#sigma", true);
  draw_grid(section7, "hDchi2cl_before_cut", out, "topology_chi2cl_grid_before_cut", "SV fit probability", true);
  draw_grid(section7, "hDdtheta_before_cut", out, "topology_dtheta_grid_before_cut", "#Delta#theta", true);

  auto *tree = dynamic_cast<TTree *>(section7.Get("SelectedD"));
  draw_mass_fits(tree, out);
  write_manifest(out);
  std::cout << "AN_PROXY_PLOTS_STATUS ok outdir=" << out << "\n";
  return 0;
}
