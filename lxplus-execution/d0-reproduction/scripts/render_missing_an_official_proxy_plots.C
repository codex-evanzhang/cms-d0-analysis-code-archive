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
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"
#include "TTreeReader.h"
#include "TTreeReaderArray.h"
#include "TTreeReaderValue.h"

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

void ensure_dir(const std::string &path) {
  gSystem->mkdir(path.c_str(), true);
}

void stamp(TCanvas &canvas, const char *text) {
  TLatex latex;
  latex.SetNDC();
  latex.SetTextFont(42);
  latex.SetTextSize(0.030);
  latex.DrawLatex(0.12, 0.93, text);
}

void save_canvas(TCanvas &canvas, const std::string &outdir, const std::string &name) {
  canvas.SaveAs((outdir + "/" + name + ".png").c_str());
  canvas.SaveAs((outdir + "/" + name + ".pdf").c_str());
}

void normalize(TH1 *hist) {
  if (hist && hist->Integral() > 0.0) hist->Scale(1.0 / hist->Integral());
}

bool has_branch(TTree *tree, const char *name) {
  return tree && tree->GetBranch(name);
}

double dca_sig(float ip, float iperr) {
  if (iperr <= 0 || !std::isfinite(ip) || !std::isfinite(iperr)) return -1.0;
  return std::abs(ip / iperr);
}

double clamp01(double x) {
  return std::max(0.0, std::min(1.0, x));
}

double prompt_fraction_fit(TH1 *data, TH1 *prompt, TH1 *nonprompt) {
  if (!data || !prompt || !nonprompt) return -1.0;
  if (data->Integral() <= 0 || prompt->Integral() <= 0 || nonprompt->Integral() <= 0) return -1.0;
  auto *d = dynamic_cast<TH1 *>(data->Clone("tmp_d_pf"));
  auto *p = dynamic_cast<TH1 *>(prompt->Clone("tmp_p_pf"));
  auto *n = dynamic_cast<TH1 *>(nonprompt->Clone("tmp_n_pf"));
  normalize(d);
  normalize(p);
  normalize(n);
  double num = 0.0;
  double den = 0.0;
  for (int i = 1; i <= d->GetNbinsX(); ++i) {
    const double dv = d->GetBinContent(i);
    const double pv = p->GetBinContent(i);
    const double nv = n->GetBinContent(i);
    const double diff = pv - nv;
    num += (dv - nv) * diff;
    den += diff * diff;
  }
  const double f = den > 0 ? clamp01(num / den) : -1.0;
  delete d;
  delete p;
  delete n;
  return f;
}

void draw_trigger_occupancy(TH1 *hist, const std::string &outdir, const std::string &name, const char *title) {
  TCanvas canvas(("c_" + name).c_str(), "", 1050, 650);
  canvas.SetTicks();
  canvas.SetGridy();
  hist->SetStats(false);
  hist->SetFillColor(kAzure - 9);
  hist->SetLineColor(kAzure + 2);
  hist->LabelsOption("v", "X");
  hist->GetYaxis()->SetTitle("Fraction of scanned events");
  hist->SetMinimum(0.0);
  hist->SetMaximum(std::max(0.05, hist->GetMaximum() * 1.25));
  hist->Draw("HIST TEXT0");
  stamp(canvas, title);
  save_canvas(canvas, outdir, name);
}

void draw_map(TH2 *hist, const std::string &outdir, const std::string &name, const char *title, double zmin = -999.0, double zmax = -999.0) {
  TCanvas canvas(("c_" + name).c_str(), "", 900, 760);
  canvas.SetRightMargin(0.16);
  canvas.SetTicks();
  hist->SetStats(false);
  if (zmin > -998.0) hist->SetMinimum(zmin);
  if (zmax > -998.0) hist->SetMaximum(zmax);
  hist->Draw("COLZ TEXT");
  stamp(canvas, title);
  save_canvas(canvas, outdir, name);
}

void draw_overlay(std::vector<TH1 *> hists, std::vector<std::string> labels, const std::string &outdir,
                  const std::string &name, const char *title, bool logy = false) {
  TCanvas canvas(("c_" + name).c_str(), "", 950, 700);
  canvas.SetTicks();
  canvas.SetLogy(logy);
  int colors[] = {kBlack, kRed + 1, kBlue + 1, kGreen + 2, kMagenta + 1, kOrange + 7, kCyan + 2};
  double ymax = 0.0;
  for (auto *hist : hists) {
    if (!hist) continue;
    normalize(hist);
    ymax = std::max(ymax, hist->GetMaximum());
  }
  TLegend legend(0.56, 0.66, 0.89, 0.89);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  bool first = true;
  for (size_t i = 0; i < hists.size(); ++i) {
    auto *hist = hists[i];
    if (!hist) continue;
    hist->SetStats(false);
    hist->SetLineColor(colors[i % 7]);
    hist->SetMarkerColor(colors[i % 7]);
    hist->SetLineWidth(i == 0 ? 3 : 2);
    hist->SetMaximum(std::max(0.01, ymax * (logy ? 8.0 : 1.35)));
    if (logy) hist->SetMinimum(1e-5);
    hist->Draw(first ? "HIST" : "HIST same");
    legend.AddEntry(hist, labels[i].c_str(), "l");
    first = false;
  }
  legend.Draw();
  stamp(canvas, title);
  save_canvas(canvas, outdir, name);
}

TH2D *make_bin_map(const char *name, const char *title) {
  auto *hist = new TH2D(name, title, 3, 0, 3, 2, 0, 2);
  hist->GetXaxis()->SetBinLabel(1, "2-5");
  hist->GetXaxis()->SetBinLabel(2, "5-8");
  hist->GetXaxis()->SetBinLabel(3, "8-12");
  hist->GetYaxis()->SetBinLabel(1, "0-1");
  hist->GetYaxis()->SetBinLabel(2, "1-2");
  return hist;
}

std::pair<int, int> map_bin_coords(int ibin) {
  const int x = (ibin % 3) + 1;
  const int y = (ibin / 3) + 1;
  return {x, y};
}

void scan_data(TTree *tree, const std::string &outdir, Long64_t maxEvents) {
  TTreeReader reader(tree);
  TTreeReaderValue<Bool_t> isL1ZDCOr(reader, "isL1ZDCOr");
  TTreeReaderValue<Bool_t> isL1ZDCXORJet8(reader, "isL1ZDCXORJet8");
  TTreeReaderValue<Bool_t> isL1ZDCXORJet12(reader, "isL1ZDCXORJet12");
  TTreeReaderValue<Bool_t> isL1ZDCXORJet16(reader, "isL1ZDCXORJet16");
  TTreeReaderValue<Bool_t> isZeroBias(reader, "isZeroBias");
  TTreeReaderValue<Bool_t> selectedBkgFilter(reader, "selectedBkgFilter");
  TTreeReaderValue<Bool_t> selectedVtxFilter(reader, "selectedVtxFilter");
  TTreeReaderValue<Bool_t> clusterFilter(reader, "ClusterCompatibilityFilter");
  TTreeReaderValue<Bool_t> haloFilter(reader, "cscTightHalo2015Filter");
  TTreeReaderArray<float> Dpt(reader, "Dpt");
  TTreeReaderArray<float> Dy(reader, "Dy");
  TTreeReaderArray<float> Dmass(reader, "Dmass");
  TTreeReaderArray<float> Dip3D(reader, "Dip3D");
  TTreeReaderArray<float> Dip3derr(reader, "Dip3derr");
  TTreeReaderArray<bool> passNom(reader, "DpassCutNominal");
  TTreeReaderArray<bool> passLoose(reader, "DpassCutLoose");
  TTreeReaderArray<bool> passTrkPt(reader, "DpassCutSystDtrkPt");
  TTreeReaderArray<bool> passDsvpv(reader, "DpassCutSystDsvpvSig");
  TTreeReaderArray<bool> passAlpha(reader, "DpassCutSystDalpha");
  TTreeReaderArray<bool> passAlphaTheta(reader, "DpassCutSystDalphaDdtheta");
  TTreeReaderArray<bool> passTheta(reader, "DpassCutSystDdtheta");
  TTreeReaderArray<bool> passChi2(reader, "DpassCutSystDchi2cl");

  std::vector<std::string> triggerNames = {
      "all", "isL1ZDCOr", "isL1ZDCXORJet8", "isL1ZDCXORJet12", "isL1ZDCXORJet16", "isZeroBias",
      "selectedBkgFilter", "selectedVtxFilter", "ClusterCompatibilityFilter", "cscTightHalo2015Filter"};
  auto *hTrig = new TH1D("hTrigData", ";Trigger/filter bit;Fraction", triggerNames.size(), 0, triggerNames.size());
  for (size_t i = 0; i < triggerNames.size(); ++i) hTrig->GetXaxis()->SetBinLabel(i + 1, triggerNames[i].c_str());

  std::map<std::string, TH1D *> massAll;
  const std::vector<std::string> vars = {"nominal", "loose", "trkPt", "dsvpvSig", "alpha", "alphaDdtheta", "dtheta", "chi2cl"};
  for (const auto &v : vars) {
    massAll[v] = new TH1D(("hMass_" + v).c_str(), ";m_{K#pi} (GeV);Candidates", 70, 1.70, 2.05);
  }
  auto *massPt2Y0Nom = new TH1D("hMassPt2Y0Nom", ";m_{K#pi} (GeV);Candidates", 70, 1.70, 2.05);
  auto *massPt2Y0Trk = new TH1D("hMassPt2Y0Trk", ";m_{K#pi} (GeV);Candidates", 70, 1.70, 2.05);
  auto *massPt8Y1Nom = new TH1D("hMassPt8Y1Nom", ";m_{K#pi} (GeV);Candidates", 70, 1.70, 2.05);
  auto *massPt8Y1Bkg = new TH1D("hMassPt8Y1Bkg", ";m_{K#pi} (GeV);Candidates", 70, 1.70, 2.05);
  auto *denData = make_bin_map("hDataTrigDen", ";D^{0} p_{T} (GeV);|y|");
  auto *dataZdcOr = make_bin_map("hDataTrigZDCOr", ";D^{0} p_{T} (GeV);|y|");
  auto *dataXor8 = make_bin_map("hDataTrigXor8", ";D^{0} p_{T} (GeV);|y|");

  std::vector<TH1D *> dcaData;
  dcaData.push_back(new TH1D("hDcaDataAll", ";|DCA|/#sigma_{DCA};Normalized candidates", 60, 0, 12));
  for (const auto &bin : kBins) {
    dcaData.push_back(new TH1D((std::string("hDcaData_") + bin.label).c_str(), ";|DCA|/#sigma_{DCA};Candidates", 60, 0, 12));
  }

  std::map<std::string, std::vector<double>> yields;
  for (const auto &v : vars) yields[v] = std::vector<double>(kBins.size(), 0.0);

  Long64_t n = 0;
  while (reader.Next()) {
    if (maxEvents >= 0 && n >= maxEvents) break;
    ++n;
    hTrig->Fill("all", 1.0);
    if (*isL1ZDCOr) hTrig->Fill("isL1ZDCOr", 1.0);
    if (*isL1ZDCXORJet8) hTrig->Fill("isL1ZDCXORJet8", 1.0);
    if (*isL1ZDCXORJet12) hTrig->Fill("isL1ZDCXORJet12", 1.0);
    if (*isL1ZDCXORJet16) hTrig->Fill("isL1ZDCXORJet16", 1.0);
    if (*isZeroBias) hTrig->Fill("isZeroBias", 1.0);
    if (*selectedBkgFilter) hTrig->Fill("selectedBkgFilter", 1.0);
    if (*selectedVtxFilter) hTrig->Fill("selectedVtxFilter", 1.0);
    if (*clusterFilter) hTrig->Fill("ClusterCompatibilityFilter", 1.0);
    if (*haloFilter) hTrig->Fill("cscTightHalo2015Filter", 1.0);

    const int nd = std::min({static_cast<int>(Dpt.GetSize()), static_cast<int>(Dy.GetSize()), static_cast<int>(Dmass.GetSize()),
                             static_cast<int>(Dip3D.GetSize()), static_cast<int>(Dip3derr.GetSize()), static_cast<int>(passNom.GetSize())});
    for (int i = 0; i < nd; ++i) {
      const int ib = bin_index(Dpt[i], Dy[i]);
      const bool inMassWindow = Dmass[i] > 1.80 && Dmass[i] < 1.93;
      const double dca = dca_sig(Dip3D[i], Dip3derr[i]);
      const std::map<std::string, bool> pass = {
          {"nominal", passNom[i]},
          {"loose", i < static_cast<int>(passLoose.GetSize()) && passLoose[i]},
          {"trkPt", i < static_cast<int>(passTrkPt.GetSize()) && passTrkPt[i]},
          {"dsvpvSig", i < static_cast<int>(passDsvpv.GetSize()) && passDsvpv[i]},
          {"alpha", i < static_cast<int>(passAlpha.GetSize()) && passAlpha[i]},
          {"alphaDdtheta", i < static_cast<int>(passAlphaTheta.GetSize()) && passAlphaTheta[i]},
          {"dtheta", i < static_cast<int>(passTheta.GetSize()) && passTheta[i]},
          {"chi2cl", i < static_cast<int>(passChi2.GetSize()) && passChi2[i]},
      };
      for (const auto &kv : pass) {
        if (!kv.second) continue;
        massAll[kv.first]->Fill(Dmass[i]);
        if (ib >= 0 && inMassWindow) yields[kv.first][ib] += 1.0;
      }
      if (ib == 0 && pass.at("nominal")) massPt2Y0Nom->Fill(Dmass[i]);
      if (ib == 0 && pass.at("trkPt")) massPt2Y0Trk->Fill(Dmass[i]);
      if (ib == 5 && pass.at("nominal")) massPt8Y1Nom->Fill(Dmass[i]);
      if (ib == 5 && pass.at("chi2cl")) massPt8Y1Bkg->Fill(Dmass[i]);
      if (pass.at("nominal") && dca >= 0) {
        dcaData[0]->Fill(dca);
        if (ib >= 0) dcaData[ib + 1]->Fill(dca);
      }
      if (pass.at("nominal") && ib >= 0) {
        auto [x, y] = map_bin_coords(ib);
        denData->AddBinContent(denData->GetBin(x, y));
        if (*isL1ZDCOr) dataZdcOr->AddBinContent(dataZdcOr->GetBin(x, y));
        if (*isL1ZDCXORJet8) dataXor8->AddBinContent(dataXor8->GetBin(x, y));
      }
    }
  }

  if (n > 0) hTrig->Scale(1.0 / static_cast<double>(n));
  draw_trigger_occupancy(hTrig, outdir, "trigger_occupancy_data", "Official 2023 reduced data trigger/filter occupancy");
  draw_overlay({massAll["nominal"], massAll["trkPt"], massAll["dsvpvSig"], massAll["alpha"], massAll["dtheta"], massAll["chi2cl"]},
               {"nominal", "track-pT syst", "DsvpvSig syst", "alpha syst", "dtheta syst", "chi2cl syst"},
               outdir, "systematic_mass_variation_all", "Official data mass-shape proxy under D-cut variations");
  draw_overlay({massPt2Y0Nom, massPt2Y0Trk}, {"nominal", "track-pT variation"},
               outdir, "systematic_mass_variation_pt2to5_absy0to1", "Track-pT systematic proxy, 2<pT<5 and |y|<1");
  draw_overlay({massPt8Y1Nom, massPt8Y1Bkg}, {"nominal", "chi2cl variation"},
               outdir, "systematic_mass_variation_pt8to12_absy1to2", "Fit/topology systematic proxy, 8<pT<12 and 1<|y|<2");

  std::ofstream ytab(outdir + "/systematic_yield_variation_proxy.tsv");
  ytab << "bin\tvariation\tmass_window_candidates\trelative_to_nominal\n";
  auto *hYield = new TH1D("hYieldVariation", ";Variation;Mean relative mass-window yield", vars.size(), 0, vars.size());
  for (size_t iv = 0; iv < vars.size(); ++iv) {
    hYield->GetXaxis()->SetBinLabel(iv + 1, vars[iv].c_str());
    double sumRatio = 0.0;
    int nr = 0;
    for (size_t ib = 0; ib < kBins.size(); ++ib) {
      const double nom = yields["nominal"][ib];
      const double ratio = nom > 0 ? yields[vars[iv]][ib] / nom : -1.0;
      ytab << kBins[ib].label << "\t" << vars[iv] << "\t" << yields[vars[iv]][ib] << "\t" << ratio << "\n";
      if (ratio >= 0) {
        sumRatio += ratio;
        ++nr;
      }
    }
    if (nr > 0) hYield->SetBinContent(iv + 1, sumRatio / nr);
  }
  TCanvas cy("c_yield_variation", "", 950, 650);
  cy.SetTicks();
  cy.SetGridy();
  hYield->SetStats(false);
  hYield->SetFillColor(kOrange - 3);
  hYield->SetLineColor(kOrange + 7);
  hYield->SetMinimum(0.0);
  hYield->SetMaximum(std::max(1.5, hYield->GetMaximum() * 1.25));
  hYield->LabelsOption("v", "X");
  hYield->Draw("HIST TEXT0");
  stamp(cy, "Candidate-count proxy for D-selection systematic variations");
  save_canvas(cy, outdir, "systematic_yield_variation_proxy");

  std::ofstream trig(outdir + "/trigger_occupancy_data.tsv");
  trig << "bit\tfraction\tcount_estimate\tscanned_events\n";
  for (int i = 1; i <= hTrig->GetNbinsX(); ++i) {
    const double frac = hTrig->GetBinContent(i);
    trig << hTrig->GetXaxis()->GetBinLabel(i) << "\t" << frac << "\t" << frac * n << "\t" << n << "\n";
  }

  auto *effDataOr = dynamic_cast<TH2D *>(dataZdcOr->Clone("hDataEffOr"));
  auto *effDataXor8 = dynamic_cast<TH2D *>(dataXor8->Clone("hDataEffXor8"));
  effDataOr->Divide(denData);
  effDataXor8->Divide(denData);
  auto *corrDataXor8 = dynamic_cast<TH2D *>(effDataXor8->Clone("hDataCorrXor8"));
  for (int ix = 1; ix <= corrDataXor8->GetNbinsX(); ++ix) {
    for (int iy = 1; iy <= corrDataXor8->GetNbinsY(); ++iy) {
      const double e = corrDataXor8->GetBinContent(ix, iy);
      corrDataXor8->SetBinContent(ix, iy, e > 0 ? 1.0 / e : 0.0);
    }
  }
  draw_map(effDataOr, outdir, "trigger_efficiency_proxy_data_zdcor", "Official data candidate-level L1 ZDCOR bit fraction", 0, 1);
  draw_map(effDataXor8, outdir, "trigger_efficiency_proxy_data_zdcxorjet8", "Official data candidate-level L1 ZDCXOR+Jet8 bit fraction", 0, 1);
  draw_map(corrDataXor8, outdir, "trigger_correction_proxy_data_zdcxorjet8", "Official data reciprocal bit-fraction proxy, L1 ZDCXOR+Jet8");

  std::ofstream dtrig(outdir + "/trigger_efficiency_proxy_data.tsv");
  dtrig << "bin\tdenominator\tisL1ZDCOr\tisL1ZDCXORJet8\tcorr_xorjet8\n";
  for (size_t ib = 0; ib < kBins.size(); ++ib) {
    auto [x, y] = map_bin_coords(static_cast<int>(ib));
    const double d = denData->GetBinContent(x, y);
    const double e8 = effDataXor8->GetBinContent(x, y);
    dtrig << kBins[ib].label << "\t" << d << "\t" << effDataOr->GetBinContent(x, y) << "\t"
          << e8 << "\t" << (e8 > 0 ? 1.0 / e8 : 0.0) << "\n";
  }
}

void scan_mc(TTree *tree, const std::string &outdir, Long64_t maxEvents, bool nonpromptMode,
             std::vector<TH1D *> &dcaPrompt, std::vector<TH1D *> &dcaNonprompt) {
  TTreeReader reader(tree);
  TTreeReaderValue<Bool_t> isL1ZDCOr(reader, "isL1ZDCOr");
  TTreeReaderValue<Bool_t> isL1ZDCXORJet8(reader, "isL1ZDCXORJet8");
  TTreeReaderValue<Bool_t> isL1ZDCXORJet12(reader, "isL1ZDCXORJet12");
  TTreeReaderValue<Bool_t> isL1ZDCXORJet16(reader, "isL1ZDCXORJet16");
  TTreeReaderValue<Bool_t> isZeroBias(reader, "isZeroBias");
  TTreeReaderArray<float> Dpt(reader, "Dpt");
  TTreeReaderArray<float> Dy(reader, "Dy");
  TTreeReaderArray<float> Dip3D(reader, "Dip3D");
  TTreeReaderArray<float> Dip3derr(reader, "Dip3derr");
  TTreeReaderArray<bool> passNom(reader, "DpassCutNominal");
  TTreeReaderArray<int> Dgen(reader, "Dgen");
  TTreeReaderArray<bool> isPrompt(reader, "DisSignalCalcPrompt");
  TTreeReaderArray<bool> isFeeddown(reader, "DisSignalCalcFeeddown");

  auto *den = make_bin_map("hTrigDen", ";D^{0} p_{T} (GeV);|y|");
  auto *zdcOr = make_bin_map("hTrigZDCOr", ";D^{0} p_{T} (GeV);|y|");
  auto *xor8 = make_bin_map("hTrigXor8", ";D^{0} p_{T} (GeV);|y|");
  auto *xor12 = make_bin_map("hTrigXor12", ";D^{0} p_{T} (GeV);|y|");
  auto *xor16 = make_bin_map("hTrigXor16", ";D^{0} p_{T} (GeV);|y|");
  auto *hTrig = new TH1D(nonpromptMode ? "hTrigMcNonprompt" : "hTrigMc", ";Trigger bit;Fraction", 6, 0, 6);
  std::vector<std::string> names = {"all", "isL1ZDCOr", "isL1ZDCXORJet8", "isL1ZDCXORJet12", "isL1ZDCXORJet16", "isZeroBias"};
  for (size_t i = 0; i < names.size(); ++i) hTrig->GetXaxis()->SetBinLabel(i + 1, names[i].c_str());

  Long64_t n = 0;
  while (reader.Next()) {
    if (maxEvents >= 0 && n >= maxEvents) break;
    ++n;
    hTrig->Fill("all", 1.0);
    if (*isL1ZDCOr) hTrig->Fill("isL1ZDCOr", 1.0);
    if (*isL1ZDCXORJet8) hTrig->Fill("isL1ZDCXORJet8", 1.0);
    if (*isL1ZDCXORJet12) hTrig->Fill("isL1ZDCXORJet12", 1.0);
    if (*isL1ZDCXORJet16) hTrig->Fill("isL1ZDCXORJet16", 1.0);
    if (*isZeroBias) hTrig->Fill("isZeroBias", 1.0);
    const int nd = std::min({static_cast<int>(Dpt.GetSize()), static_cast<int>(Dy.GetSize()), static_cast<int>(passNom.GetSize()),
                             static_cast<int>(Dgen.GetSize()), static_cast<int>(Dip3D.GetSize()), static_cast<int>(Dip3derr.GetSize())});
    for (int i = 0; i < nd; ++i) {
      if (!passNom[i]) continue;
      const bool truth = Dgen[i] != 0;
      const bool prompt = i < static_cast<int>(isPrompt.GetSize()) && isPrompt[i];
      const bool feeddown = i < static_cast<int>(isFeeddown.GetSize()) && isFeeddown[i];
      if (!truth && !prompt && !feeddown) continue;
      const int ib = bin_index(Dpt[i], Dy[i]);
      const double dca = dca_sig(Dip3D[i], Dip3derr[i]);
      if (dca >= 0) {
        if (prompt && !nonpromptMode) {
          dcaPrompt[0]->Fill(dca);
          if (ib >= 0) dcaPrompt[ib + 1]->Fill(dca);
        }
        if (feeddown || nonpromptMode) {
          dcaNonprompt[0]->Fill(dca);
          if (ib >= 0) dcaNonprompt[ib + 1]->Fill(dca);
        }
      }
      if (ib < 0 || nonpromptMode) continue;
      auto [x, y] = map_bin_coords(ib);
      den->AddBinContent(den->GetBin(x, y));
      if (*isL1ZDCOr) zdcOr->AddBinContent(zdcOr->GetBin(x, y));
      if (*isL1ZDCXORJet8) xor8->AddBinContent(xor8->GetBin(x, y));
      if (*isL1ZDCXORJet12) xor12->AddBinContent(xor12->GetBin(x, y));
      if (*isL1ZDCXORJet16) xor16->AddBinContent(xor16->GetBin(x, y));
    }
  }
  if (n > 0) hTrig->Scale(1.0 / static_cast<double>(n));
  draw_trigger_occupancy(hTrig, outdir, nonpromptMode ? "trigger_occupancy_nonprompt_mc" : "trigger_occupancy_prompt_mc",
                         nonpromptMode ? "Official nonprompt MC trigger-bit occupancy" : "Official prompt MC trigger-bit occupancy");

  if (!nonpromptMode) {
    auto *effOr = dynamic_cast<TH2D *>(zdcOr->Clone("hEffOr"));
    auto *effXor8 = dynamic_cast<TH2D *>(xor8->Clone("hEffXor8"));
    auto *effXor12 = dynamic_cast<TH2D *>(xor12->Clone("hEffXor12"));
    auto *effXor16 = dynamic_cast<TH2D *>(xor16->Clone("hEffXor16"));
    effOr->Divide(den);
    effXor8->Divide(den);
    effXor12->Divide(den);
    effXor16->Divide(den);
    auto *corrXor8 = dynamic_cast<TH2D *>(effXor8->Clone("hCorrXor8"));
    for (int ix = 1; ix <= corrXor8->GetNbinsX(); ++ix) {
      for (int iy = 1; iy <= corrXor8->GetNbinsY(); ++iy) {
        const double e = corrXor8->GetBinContent(ix, iy);
        corrXor8->SetBinContent(ix, iy, e > 0 ? 1.0 / e : 0.0);
      }
    }
    draw_map(effOr, outdir, "trigger_efficiency_proxy_zdcor", "Prompt MC candidate-level L1 ZDCOR efficiency proxy", 0, 1);
    draw_map(effXor8, outdir, "trigger_efficiency_proxy_zdcxorjet8", "Prompt MC candidate-level L1 ZDCXOR+Jet8 efficiency proxy", 0, 1);
    draw_map(effXor12, outdir, "trigger_efficiency_proxy_zdcxorjet12", "Prompt MC candidate-level L1 ZDCXOR+Jet12 efficiency proxy", 0, 1);
    draw_map(effXor16, outdir, "trigger_efficiency_proxy_zdcxorjet16", "Prompt MC candidate-level L1 ZDCXOR+Jet16 efficiency proxy", 0, 1);
    draw_map(corrXor8, outdir, "trigger_correction_proxy_zdcxorjet8", "Reciprocal trigger-efficiency proxy, L1 ZDCXOR+Jet8");

    std::ofstream trig(outdir + "/trigger_efficiency_proxy.tsv");
    trig << "bin\tdenominator\tisL1ZDCOr\tisL1ZDCXORJet8\tisL1ZDCXORJet12\tisL1ZDCXORJet16\tcorr_xorjet8\n";
    for (size_t ib = 0; ib < kBins.size(); ++ib) {
      auto [x, y] = map_bin_coords(static_cast<int>(ib));
      const double d = den->GetBinContent(x, y);
      const double e8 = effXor8->GetBinContent(x, y);
      trig << kBins[ib].label << "\t" << d << "\t" << effOr->GetBinContent(x, y) << "\t" << e8 << "\t"
           << effXor12->GetBinContent(x, y) << "\t" << effXor16->GetBinContent(x, y) << "\t"
           << (e8 > 0 ? 1.0 / e8 : 0.0) << "\n";
    }
  }
}

void draw_dca_and_prompt_fraction(const std::string &outdir, const std::vector<TH1D *> &data,
                                  const std::vector<TH1D *> &prompt, const std::vector<TH1D *> &nonprompt) {
  draw_overlay({data[0], prompt[0], nonprompt[0]}, {"official data", "prompt MC", "nonprompt/feeddown MC"},
               outdir, "dca_template_all", "DCA-significance templates for prompt-fraction proxy", true);

  auto *hFrac = new TH1D("hPromptFraction", ";Kinematic bin;Prompt fraction proxy", kBins.size(), 0, kBins.size());
  std::ofstream out(outdir + "/prompt_fraction_proxy_by_bin.tsv");
  out << "bin\tdata_entries\tprompt_entries\tnonprompt_entries\tprompt_fraction_proxy\n";
  for (size_t ib = 0; ib < kBins.size(); ++ib) {
    const double f = prompt_fraction_fit(data[ib + 1], prompt[ib + 1], nonprompt[ib + 1]);
    hFrac->GetXaxis()->SetBinLabel(ib + 1, kBins[ib].label);
    if (f >= 0) hFrac->SetBinContent(ib + 1, f);
    out << kBins[ib].label << "\t" << data[ib + 1]->Integral() << "\t" << prompt[ib + 1]->Integral() << "\t"
        << nonprompt[ib + 1]->Integral() << "\t" << f << "\n";
  }
  TCanvas canvas("c_prompt_fraction", "", 1050, 650);
  canvas.SetTicks();
  canvas.SetGridy();
  hFrac->SetStats(false);
  hFrac->SetFillColor(kGreen - 7);
  hFrac->SetLineColor(kGreen + 3);
  hFrac->SetMinimum(0.0);
  hFrac->SetMaximum(1.05);
  hFrac->LabelsOption("v", "X");
  hFrac->Draw("HIST TEXT0");
  stamp(canvas, "Template-fit prompt-fraction proxy; not final DCA systematic");
  save_canvas(canvas, outdir, "prompt_fraction_proxy_by_bin");
}

}  // namespace

int render_missing_an_official_proxy_plots(
    const char *dataPath = "/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root",
    const char *promptMcPath = "/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root",
    const char *nonpromptMcPath = "/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/260203/Dzero_260203_HiForest_260120_nonprompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_Dpt1_PF0p1.root",
    const char *outdir = "/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/missing-an-official-proxy/latest",
    Long64_t maxDataEvents = 5000000,
    Long64_t maxMcEvents = 5000000) {
  gStyle->SetOptStat(0);
  std::string out(outdir);
  ensure_dir(out);

  TFile dataFile(dataPath, "READ");
  TFile promptFile(promptMcPath, "READ");
  TFile nonpromptFile(nonpromptMcPath, "READ");
  auto *dataTree = dynamic_cast<TTree *>(dataFile.Get("Tree"));
  auto *promptTree = dynamic_cast<TTree *>(promptFile.Get("Tree"));
  auto *nonpromptTree = dynamic_cast<TTree *>(nonpromptFile.Get("Tree"));
  if (!dataTree || !promptTree) {
    std::cerr << "ERROR missing required Tree in data or prompt MC input\n";
    return 2;
  }

  const std::vector<const char *> requiredData = {"DpassCutNominal", "DpassCutSystDtrkPt", "DpassCutSystDsvpvSig", "Dip3D", "Dip3derr"};
  for (const char *name : requiredData) {
    if (!has_branch(dataTree, name)) {
      std::cerr << "ERROR data tree missing branch " << name << "\n";
      return 3;
    }
  }

  std::vector<TH1D *> dcaData;
  std::vector<TH1D *> dcaPrompt;
  std::vector<TH1D *> dcaNonprompt;
  dcaData.push_back(new TH1D("hDcaDataForFitAll", ";|DCA|/#sigma_{DCA};Candidates", 60, 0, 12));
  dcaPrompt.push_back(new TH1D("hDcaPromptAll", ";|DCA|/#sigma_{DCA};Candidates", 60, 0, 12));
  dcaNonprompt.push_back(new TH1D("hDcaNonpromptAll", ";|DCA|/#sigma_{DCA};Candidates", 60, 0, 12));
  for (const auto &bin : kBins) {
    dcaData.push_back(new TH1D((std::string("hDcaDataFit_") + bin.label).c_str(), ";|DCA|/#sigma_{DCA};Candidates", 60, 0, 12));
    dcaPrompt.push_back(new TH1D((std::string("hDcaPrompt_") + bin.label).c_str(), ";|DCA|/#sigma_{DCA};Candidates", 60, 0, 12));
    dcaNonprompt.push_back(new TH1D((std::string("hDcaNonprompt_") + bin.label).c_str(), ";|DCA|/#sigma_{DCA};Candidates", 60, 0, 12));
  }

  scan_data(dataTree, out, maxDataEvents);

  TFile dataDcaFile(dataPath, "READ");
  auto *dataDcaTree = dynamic_cast<TTree *>(dataDcaFile.Get("Tree"));
  if (dataDcaTree) {
    TTreeReader r(dataDcaTree);
    TTreeReaderArray<float> Dpt(r, "Dpt");
    TTreeReaderArray<float> Dy(r, "Dy");
    TTreeReaderArray<float> Dip3D(r, "Dip3D");
    TTreeReaderArray<float> Dip3derr(r, "Dip3derr");
    TTreeReaderArray<bool> passNom(r, "DpassCutNominal");
    Long64_t n = 0;
    while (r.Next()) {
      if (maxDataEvents >= 0 && n >= maxDataEvents) break;
      ++n;
      const int nd = std::min({static_cast<int>(Dpt.GetSize()), static_cast<int>(Dy.GetSize()), static_cast<int>(Dip3D.GetSize()),
                               static_cast<int>(Dip3derr.GetSize()), static_cast<int>(passNom.GetSize())});
      for (int i = 0; i < nd; ++i) {
        if (!passNom[i]) continue;
        const double dca = dca_sig(Dip3D[i], Dip3derr[i]);
        if (dca < 0) continue;
        const int ib = bin_index(Dpt[i], Dy[i]);
        dcaData[0]->Fill(dca);
        if (ib >= 0) dcaData[ib + 1]->Fill(dca);
      }
    }
  }

  scan_mc(promptTree, out, maxMcEvents, false, dcaPrompt, dcaNonprompt);
  if (nonpromptTree && has_branch(nonpromptTree, "DpassCutNominal") && has_branch(nonpromptTree, "Dgen")) {
    scan_mc(nonpromptTree, out, maxMcEvents, true, dcaPrompt, dcaNonprompt);
  }
  draw_dca_and_prompt_fraction(out, dcaData, dcaPrompt, dcaNonprompt);

  std::ofstream readme(out + "/README.md");
  readme << "# Missing AN Official-Reference Proxy Plots\n\n";
  readme << "<!-- DOC_OWNER: cms-analysis-manager generated ROOT plot bundle. -->\n";
  readme << "<!-- TOKEN_NOTE: generated by render_missing_an_official_proxy_plots.C. -->\n\n";
  readme << "Inputs:\n\n";
  readme << "- Official/pre-forested 2023 data tree: `" << dataPath << "`\n";
  readme << "- Official prompt MC tree: `" << promptMcPath << "`\n";
  readme << "- Official nonprompt/feeddown MC tree, when readable: `" << nonpromptMcPath << "`\n\n";
  readme << "These outputs are reference/proxy plots for AN slots that the 98 recreated selected skim cannot support by itself: trigger-bit occupancy, ";
  readme << "candidate-level trigger efficiency proxies, DCA prompt-fraction templates, and systematic pass-flag mass-shape checks. They are not final corrected cross sections or final systematics.\n";

  std::ofstream summary(out + "/official_proxy_summary.tsv");
  summary << "metric\tvalue\n";
  summary << "max_data_events\t" << maxDataEvents << "\n";
  summary << "max_mc_events\t" << maxMcEvents << "\n";
  summary << "data_input\t" << dataPath << "\n";
  summary << "prompt_mc_input\t" << promptMcPath << "\n";
  summary << "nonprompt_mc_input\t" << nonpromptMcPath << "\n";
  summary << "classification\tproxy; official/pre-forested inputs used where recreated 98 selected skim lacks branches\n";

  std::cout << "MISSING_AN_OFFICIAL_PROXY_STATUS ok outdir=" << out << "\n";
  return 0;
}
