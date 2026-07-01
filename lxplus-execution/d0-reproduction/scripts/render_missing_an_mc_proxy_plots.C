#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1.h"
#include "TH2.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLine.h"
#include "TProfile.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"
#include "TTreeReader.h"
#include "TTreeReaderArray.h"
#include "TTreeReaderValue.h"

namespace {

constexpr double D0_MASS = 1.86484;

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

bool has_branch(TTree *tree, const char *name) {
  return tree && tree->GetBranch(name);
}

void ensure_dir(const std::string &path) {
  gSystem->mkdir(path.c_str(), true);
}

void stamp(TCanvas &canvas, const char *text = "Recreated D^{0} AN proxy") {
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

void normalize(TH1 *hist) {
  if (hist && hist->Integral() > 0) hist->Scale(1.0 / hist->Integral());
}

int bin_index(double pt, double y) {
  const double ay = std::abs(y);
  for (size_t i = 0; i < kBins.size(); ++i) {
    const auto &b = kBins[i];
    const bool inPt = pt >= b.ptMin && (pt < b.ptMax || (b.ptMax == 12.0 && pt <= b.ptMax));
    const bool inY = ay >= b.absYMin && (ay < b.absYMax || (b.absYMax == 2.0 && ay <= b.absYMax));
    if (inPt && inY) return static_cast<int>(i);
  }
  return -1;
}

bool pass_an_rectangular(double pt, double y, double alpha, double svpvSig, double chi2cl, double dtheta) {
  const int ibin = bin_index(pt, y);
  if (ibin < 0) return false;
  const bool lowPt = pt < 5.0;
  double alphaCut = 0.40;
  double dthetaCut = 0.30;
  if (std::abs(y) < 1.0 && pt >= 5.0 && pt < 8.0) alphaCut = 0.35;
  if (std::abs(y) >= 1.0) alphaCut = (pt < 5.0 ? 0.20 : 0.25);
  if (std::abs(y) < 1.0 && pt < 5.0) dthetaCut = 0.50;
  return alpha < alphaCut && svpvSig > (lowPt ? 2.5 : 3.5) && chi2cl > 0.10 && dtheta < dthetaCut;
}

void draw_overlay(TH1 *data, TH1 *mc, const std::string &outdir, const std::string &name, const char *xTitle,
                  bool logy = false) {
  if (!data || !mc) return;
  normalize(data);
  normalize(mc);
  TCanvas canvas(("c_" + name).c_str(), "", 900, 700);
  canvas.SetTicks();
  canvas.SetLogy(logy);
  data->SetStats(false);
  mc->SetStats(false);
  data->SetLineColor(kBlack);
  data->SetMarkerColor(kBlack);
  data->SetMarkerStyle(20);
  mc->SetLineColor(kRed + 1);
  mc->SetLineWidth(2);
  data->GetXaxis()->SetTitle(xTitle);
  data->GetYaxis()->SetTitle("Normalized candidates");
  const double ymax = std::max(data->GetMaximum(), mc->GetMaximum()) * (logy ? 5.0 : 1.35);
  data->SetMaximum(std::max(ymax, 0.01));
  if (logy) data->SetMinimum(1e-5);
  data->Draw("E");
  mc->Draw("HIST same");
  TLegend legend(0.58, 0.76, 0.88, 0.89);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.AddEntry(data, "2023 recreated data", "lep");
  legend.AddEntry(mc, "prompt MC truth-match", "l");
  legend.Draw();
  stamp(canvas);
  save_canvas(canvas, outdir, name);
}

void draw_hist2(TH2 *hist, const std::string &outdir, const std::string &name, const char *stampText) {
  if (!hist) return;
  TCanvas canvas(("c_" + name).c_str(), "", 900, 760);
  canvas.SetRightMargin(0.16);
  canvas.SetTicks();
  hist->SetStats(false);
  hist->Draw("COLZ TEXT");
  stamp(canvas, stampText);
  save_canvas(canvas, outdir, name);
}

void draw_projection(TH1 *h1, TH1 *h2, const std::string &outdir, const std::string &name, const char *xTitle,
                     const char *l1, const char *l2) {
  if (!h1 || !h2) return;
  normalize(h1);
  normalize(h2);
  TCanvas canvas(("c_" + name).c_str(), "", 900, 700);
  canvas.SetTicks();
  h1->SetStats(false);
  h2->SetStats(false);
  h1->SetLineColor(kBlue + 1);
  h1->SetLineWidth(2);
  h2->SetLineColor(kRed + 1);
  h2->SetLineWidth(2);
  h1->GetXaxis()->SetTitle(xTitle);
  h1->GetYaxis()->SetTitle("Normalized entries");
  h1->SetMaximum(std::max(h1->GetMaximum(), h2->GetMaximum()) * 1.25);
  h1->Draw("HIST");
  h2->Draw("HIST same");
  TLegend legend(0.58, 0.76, 0.88, 0.89);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.AddEntry(h1, l1, "l");
  legend.AddEntry(h2, l2, "l");
  legend.Draw();
  stamp(canvas);
  save_canvas(canvas, outdir, name);
}

void render_mc_proxy(const char *mcPath, const char *dataSelectedPath, const std::string &outdir,
                     Long64_t maxEvents) {
  TFile mcFile(mcPath, "READ");
  TFile dataFile(dataSelectedPath, "READ");
  auto *mcTree = dynamic_cast<TTree *>(mcFile.Get("Tree"));
  auto *dataTree = dynamic_cast<TTree *>(dataFile.Get("SelectedD"));
  if (!mcTree || !dataTree) {
    std::cerr << "ERROR missing Tree or SelectedD inputs\n";
    return;
  }

  auto *hProcess = new TH1D("hProcess", ";PYTHIA ProcessID;Events", 100, 0, 1000);
  auto *hGenPtY = new TH2D("hGenPtY", ";D^{0}_{gen} p_{T} (GeV);|y_{gen}|", 3, 2, 12, 2, 0, 2);
  hGenPtY->GetXaxis()->SetBinLabel(1, "2-5");
  hGenPtY->GetXaxis()->SetBinLabel(2, "5-8");
  hGenPtY->GetXaxis()->SetBinLabel(3, "8-12");
  hGenPtY->GetYaxis()->SetBinLabel(1, "0-1");
  hGenPtY->GetYaxis()->SetBinLabel(2, "1-2");
  auto *hRecoPtY = dynamic_cast<TH2D *>(hGenPtY->Clone("hRecoPtY"));
  hRecoPtY->Reset();
  hRecoPtY->SetTitle(";D^{0}_{reco} p_{T} (GeV);|y_{reco}|");
  auto *hEffPtY = dynamic_cast<TH2D *>(hGenPtY->Clone("hEffPtY"));
  hEffPtY->Reset();
  hEffPtY->SetTitle(";D^{0} p_{T} (GeV);|y|");

  auto *hGenPt = new TH1D("hGenPt", ";D^{0} p_{T} (GeV);Entries", 50, 0, 20);
  auto *hRecoPt = new TH1D("hRecoPt", ";D^{0} p_{T} (GeV);Entries", 50, 0, 20);
  auto *hGenY = new TH1D("hGenY", ";D^{0} y;Entries", 48, -2.4, 2.4);
  auto *hRecoY = new TH1D("hRecoY", ";D^{0} y;Entries", 48, -2.4, 2.4);
  auto *hDataPt = new TH1D("hDataPt", ";D^{0} p_{T} (GeV);Entries", 50, 0, 20);
  auto *hDataY = new TH1D("hDataY", ";D^{0} y;Entries", 48, -2.4, 2.4);
  auto *hWeight = dynamic_cast<TH2D *>(hGenPtY->Clone("hDataOverMcWeight"));
  hWeight->Reset();
  hWeight->SetTitle(";D^{0} p_{T} bin;|y| bin");
  auto *hMcReweightedPt = new TH1D("hMcReweightedPt", ";D^{0} p_{T} (GeV);Entries", 50, 0, 20);
  auto *hMcReweightedY = new TH1D("hMcReweightedY", ";D^{0} y;Entries", 48, -2.4, 2.4);

  std::map<std::string, TH1 *> dataH;
  std::map<std::string, TH1 *> mcH;
  const std::vector<std::tuple<std::string, std::string, int, double, double>> vars = {
      {"alpha", "D #alpha", 60, 0.0, 1.2},
      {"svpvSig", "L_{3D}/#sigma", 60, 0.0, 20.0},
      {"svpvSig2D", "L_{2D}/#sigma", 60, 0.0, 20.0},
      {"chi2cl", "SV fit probability", 50, 0.0, 1.0},
      {"dtheta", "#Delta#theta", 60, 0.0, 1.2},
      {"trk1pt", "daughter 1 p_{T} (GeV)", 60, 0.0, 8.0},
      {"trk2pt", "daughter 2 p_{T} (GeV)", 60, 0.0, 8.0},
      {"relpt1", "daughter 1 #sigma(p_{T})/p_{T}", 50, 0.0, 0.12},
      {"relpt2", "daughter 2 #sigma(p_{T})/p_{T}", 50, 0.0, 0.12},
  };
  for (const auto &item : vars) {
    const std::string key = std::get<0>(item);
    const std::string title = ";" + std::get<1>(item) + ";Candidates";
    dataH[key] = new TH1D(("hData_" + key).c_str(), title.c_str(), std::get<2>(item), std::get<3>(item), std::get<4>(item));
    mcH[key] = new TH1D(("hMc_" + key).c_str(), title.c_str(), std::get<2>(item), std::get<3>(item), std::get<4>(item));
  }

  TTreeReader mcReader(mcTree);
  TTreeReaderValue<Int_t> processID(mcReader, "ProcessID");
  TTreeReaderArray<float> gpt(mcReader, "Gpt");
  TTreeReaderArray<float> gy(mcReader, "Gy");
  TTreeReaderArray<float> dpt(mcReader, "Dpt");
  TTreeReaderArray<float> dy(mcReader, "Dy");
  TTreeReaderArray<float> dmass(mcReader, "Dmass");
  TTreeReaderArray<float> dalpha(mcReader, "Dalpha");
  TTreeReaderArray<float> dsv(mcReader, "DsvpvDistance");
  TTreeReaderArray<float> dsverr(mcReader, "DsvpvDisErr");
  TTreeReaderArray<float> dsv2(mcReader, "DsvpvDistance_2D");
  TTreeReaderArray<float> dsverr2(mcReader, "DsvpvDisErr_2D");
  TTreeReaderArray<float> dchi(mcReader, "Dchi2cl");
  TTreeReaderArray<float> ddtheta(mcReader, "Ddtheta");
  TTreeReaderArray<float> dtrk1pt(mcReader, "Dtrk1Pt");
  TTreeReaderArray<float> dtrk2pt(mcReader, "Dtrk2Pt");
  TTreeReaderArray<float> dtrk1err(mcReader, "Dtrk1PtErr");
  TTreeReaderArray<float> dtrk2err(mcReader, "Dtrk2PtErr");
  TTreeReaderArray<int> dgen(mcReader, "Dgen");

  Long64_t entry = 0;
  while (mcReader.Next()) {
    if (maxEvents >= 0 && entry >= maxEvents) break;
    ++entry;
    hProcess->Fill(*processID);
    for (int i = 0; i < static_cast<int>(gpt.GetSize()) && i < static_cast<int>(gy.GetSize()); ++i) {
      const double pt = gpt[i];
      const double y = gy[i];
      hGenPt->Fill(pt);
      hGenY->Fill(y);
      const int ib = bin_index(pt, y);
      if (ib >= 0) hGenPtY->Fill(pt, std::abs(y));
    }
    const int nd = std::min({static_cast<int>(dpt.GetSize()), static_cast<int>(dy.GetSize()), static_cast<int>(dmass.GetSize()),
                             static_cast<int>(dalpha.GetSize()), static_cast<int>(dsv.GetSize()), static_cast<int>(dsverr.GetSize()),
                             static_cast<int>(dchi.GetSize()), static_cast<int>(ddtheta.GetSize()), static_cast<int>(dgen.GetSize())});
    for (int i = 0; i < nd; ++i) {
      if (dgen[i] == 0) continue;
      const double pt = dpt[i];
      const double y = dy[i];
      const double svsig = dsverr[i] > 0 ? dsv[i] / dsverr[i] : -1;
      const double svsig2 = i < static_cast<int>(dsv2.GetSize()) && i < static_cast<int>(dsverr2.GetSize()) && dsverr2[i] > 0 ? dsv2[i] / dsverr2[i] : -1;
      if (!pass_an_rectangular(pt, y, dalpha[i], svsig, dchi[i], ddtheta[i])) continue;
      hRecoPt->Fill(pt);
      hRecoY->Fill(y);
      const int ib = bin_index(pt, y);
      if (ib >= 0) hRecoPtY->Fill(pt, std::abs(y));
      mcH["alpha"]->Fill(dalpha[i]);
      mcH["svpvSig"]->Fill(svsig);
      if (svsig2 >= 0) mcH["svpvSig2D"]->Fill(svsig2);
      mcH["chi2cl"]->Fill(dchi[i]);
      mcH["dtheta"]->Fill(ddtheta[i]);
      mcH["trk1pt"]->Fill(dtrk1pt[i]);
      mcH["trk2pt"]->Fill(dtrk2pt[i]);
      if (dtrk1pt[i] > 0) mcH["relpt1"]->Fill(dtrk1err[i] / dtrk1pt[i]);
      if (dtrk2pt[i] > 0) mcH["relpt2"]->Fill(dtrk2err[i] / dtrk2pt[i]);
    }
  }

  Float_t Dmass, Dpt, Dy, Dalpha, Ddtheta, DsvpvSig, Dchi2cl, Dtrk1Pt, Dtrk2Pt, Dtrk1RelPtErr, Dtrk2RelPtErr;
  dataTree->SetBranchAddress("Dmass", &Dmass);
  dataTree->SetBranchAddress("Dpt", &Dpt);
  dataTree->SetBranchAddress("Dy", &Dy);
  dataTree->SetBranchAddress("Dalpha", &Dalpha);
  dataTree->SetBranchAddress("Ddtheta", &Ddtheta);
  dataTree->SetBranchAddress("DsvpvSig", &DsvpvSig);
  dataTree->SetBranchAddress("Dchi2cl", &Dchi2cl);
  dataTree->SetBranchAddress("Dtrk1Pt", &Dtrk1Pt);
  dataTree->SetBranchAddress("Dtrk2Pt", &Dtrk2Pt);
  dataTree->SetBranchAddress("Dtrk1RelPtErr", &Dtrk1RelPtErr);
  dataTree->SetBranchAddress("Dtrk2RelPtErr", &Dtrk2RelPtErr);
  const Long64_t ndata = dataTree->GetEntries();
  for (Long64_t i = 0; i < ndata; ++i) {
    dataTree->GetEntry(i);
    hDataPt->Fill(Dpt);
    hDataY->Fill(Dy);
    const int ib = bin_index(Dpt, Dy);
    if (ib >= 0) hWeight->Fill(Dpt, std::abs(Dy));
    dataH["alpha"]->Fill(Dalpha);
    dataH["svpvSig"]->Fill(DsvpvSig);
    dataH["chi2cl"]->Fill(Dchi2cl);
    dataH["dtheta"]->Fill(Ddtheta);
    dataH["trk1pt"]->Fill(Dtrk1Pt);
    dataH["trk2pt"]->Fill(Dtrk2Pt);
    dataH["relpt1"]->Fill(Dtrk1RelPtErr);
    dataH["relpt2"]->Fill(Dtrk2RelPtErr);
  }

  hEffPtY->Divide(hRecoPtY, hGenPtY, 1.0, 1.0, "B");
  if (hWeight->Integral() > 0) hWeight->Scale(1.0 / hWeight->Integral());
  auto *hMcRecoPtYNorm = dynamic_cast<TH2D *>(hRecoPtY->Clone("hMcRecoPtYNorm"));
  if (hMcRecoPtYNorm->Integral() > 0) hMcRecoPtYNorm->Scale(1.0 / hMcRecoPtYNorm->Integral());
  hWeight->Divide(hMcRecoPtYNorm);
  for (int ix = 1; ix <= hWeight->GetNbinsX(); ++ix) {
    for (int iy = 1; iy <= hWeight->GetNbinsY(); ++iy) {
      if (!std::isfinite(hWeight->GetBinContent(ix, iy))) hWeight->SetBinContent(ix, iy, 0.0);
    }
  }

  TTreeReader rwReader(mcTree);
  TTreeReaderArray<float> rwDpt(rwReader, "Dpt");
  TTreeReaderArray<float> rwDy(rwReader, "Dy");
  TTreeReaderArray<int> rwDgen(rwReader, "Dgen");
  Long64_t rwEntry = 0;
  while (rwReader.Next()) {
    if (maxEvents >= 0 && rwEntry >= maxEvents) break;
    ++rwEntry;
    const int nd = std::min({static_cast<int>(rwDpt.GetSize()), static_cast<int>(rwDy.GetSize()), static_cast<int>(rwDgen.GetSize())});
    for (int i = 0; i < nd; ++i) {
      if (rwDgen[i] == 0) continue;
      const int ib = bin_index(rwDpt[i], rwDy[i]);
      if (ib < 0) continue;
      const double w = hWeight->GetBinContent(hWeight->GetXaxis()->FindFixBin(rwDpt[i]), hWeight->GetYaxis()->FindFixBin(std::abs(rwDy[i])));
      hMcReweightedPt->Fill(rwDpt[i], w);
      hMcReweightedY->Fill(rwDy[i], w);
    }
  }

  TCanvas cproc("c_process", "", 900, 700);
  cproc.SetLogy();
  hProcess->SetStats(false);
  hProcess->SetLineColor(kBlue + 1);
  hProcess->Draw("HIST");
  stamp(cproc, "Official prompt MC process-ID distribution");
  save_canvas(cproc, outdir, "mc_process_id_distribution");

  draw_hist2(hGenPtY, outdir, "mc_gen_d0_pt_y_map", "Official prompt MC generated D^{0}");
  draw_hist2(hRecoPtY, outdir, "mc_reco_truth_d0_pt_y_map", "Truth-matched D^{0} after AN rectangular cuts");
  draw_hist2(hEffPtY, outdir, "mc_acceptance_efficiency_proxy_map", "Reco/gen proxy, not final efficiency");
  draw_projection(hGenPt, hRecoPt, outdir, "mc_gen_vs_reco_d0_pt", "D^{0} p_{T} (GeV)", "generated", "truth-matched reco");
  draw_projection(hGenY, hRecoY, outdir, "mc_gen_vs_reco_d0_y", "D^{0} y", "generated", "truth-matched reco");
  draw_hist2(hWeight, outdir, "data_over_mc_reweighting_proxy_map", "Data/MC selected-shape proxy weights");
  draw_projection(hRecoPt, hMcReweightedPt, outdir, "mc_pt_before_after_proxy_reweight", "D^{0} p_{T} (GeV)", "MC selected", "MC reweighted");
  draw_projection(hRecoY, hMcReweightedY, outdir, "mc_y_before_after_proxy_reweight", "D^{0} y", "MC selected", "MC reweighted");
  draw_projection(hDataPt, hRecoPt, outdir, "data_vs_mc_selected_d0_pt", "D^{0} p_{T} (GeV)", "data selected", "MC truth selected");
  draw_projection(hDataY, hRecoY, outdir, "data_vs_mc_selected_d0_y", "D^{0} y", "data selected", "MC truth selected");

  for (const auto &item : vars) {
    const std::string key = std::get<0>(item);
    draw_overlay(dataH[key], mcH[key], outdir, "data_vs_mc_" + key, std::get<1>(item).c_str(), key == "svpvSig" || key == "svpvSig2D");
  }

  std::ofstream summary(outdir + "/mc_proxy_summary.tsv");
  summary << "metric\tvalue\n";
  summary << "mc_events_scanned\t" << entry << "\n";
  summary << "data_selected_candidates\t" << ndata << "\n";
  summary << "gen_bin_entries\t" << hGenPtY->Integral() << "\n";
  summary << "truth_reco_bin_entries\t" << hRecoPtY->Integral() << "\n";
  summary << "note\tproxy plots use official prompt MC plus recreated 98-forest selected data; not final corrected efficiencies\n";

  std::ofstream readme(outdir + "/README.md");
  readme << "# Missing AN MC/Data Proxy Plots\n\n";
  readme << "<!-- DOC_OWNER: cms-analysis-manager generated ROOT plot bundle. -->\n";
  readme << "<!-- TOKEN_NOTE: generated by render_missing_an_mc_proxy_plots.C. -->\n\n";
  readme << "Inputs:\n\n";
  readme << "- MC: `" << mcPath << "`\n";
  readme << "- Data selected D tree: `" << dataSelectedPath << "`\n\n";
  readme << "These plots fill AN-like targets for MC process IDs, D0 efficiency proxy maps, ";
  readme << "MC/data topology overlays, and simple selected-shape reweighting diagnostics. ";
  readme << "They are validation/proxy plots, not final cross-section corrections.\n";
}

}  // namespace

int render_missing_an_mc_proxy_plots(
    const char *mcPath = "/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root",
    const char *dataSelectedPath = "/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/crab_98forest_apriori_20260619/section7_d_selected.root",
    const char *outdir = "/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/missing-an-mc-proxy/latest",
    Long64_t maxEvents = 2000000) {
  gStyle->SetOptStat(0);
  std::string out(outdir);
  ensure_dir(out);
  render_mc_proxy(mcPath, dataSelectedPath, out, maxEvents);
  std::cout << "MISSING_AN_MC_PROXY_STATUS ok outdir=" << out << "\n";
  return 0;
}
