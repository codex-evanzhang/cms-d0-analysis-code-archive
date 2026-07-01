#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TChain.h"
#include "TF1.h"
#include "TFile.h"
#include "TH1D.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTreeReader.h"
#include "TTreeReaderArray.h"

namespace {

std::vector<std::string> split_inputs(const std::string &input) {
  std::vector<std::string> files;
  if (input.size() >= 4 && input.substr(input.size() - 4) == ".txt") {
    std::ifstream in(input);
    std::string line;
    while (std::getline(in, line)) {
      if (line.empty() || line[0] == '#') continue;
      files.push_back(line);
    }
    return files;
  }
  std::stringstream ss(input);
  std::string item;
  while (std::getline(ss, item, ',')) {
    if (!item.empty()) files.push_back(item);
  }
  return files;
}

void ensure_dir(const std::string &path) {
  gSystem->mkdir(path.c_str(), true);
}

void style_hist(TH1D *hist, int color) {
  hist->SetStats(false);
  hist->SetLineColor(color);
  hist->SetMarkerColor(color);
  hist->SetMarkerStyle(20);
  hist->SetMarkerSize(0.7);
  hist->GetXaxis()->SetTitle("m_{K#pi#pi} (GeV)");
  hist->GetYaxis()->SetTitle("Candidates / 5 MeV");
}

void stamp(const char *text) {
  TLatex latex;
  latex.SetNDC();
  latex.SetTextFont(42);
  latex.SetTextSize(0.032);
  latex.DrawLatex(0.12, 0.93, text);
}

double fit_mass(TH1D *hist, const char *fitName, std::ofstream &summary, const char *label) {
  if (!hist || hist->GetEntries() < 100) {
    summary << label << "\t" << (hist ? hist->GetEntries() : 0) << "\t-999\t-999\t-999\tlow_entries\n";
    return -999.0;
  }
  TF1 fit(fitName, "gaus(0)+pol1(3)", 1.76, 1.98);
  fit.SetParameters(std::max(1.0, hist->GetMaximum() * 0.2), 1.869, 0.014, hist->GetMaximum(), -1.0);
  fit.SetParLimits(0, 0.0, std::max(1.0, hist->GetMaximum() * 10.0));
  fit.SetParLimits(1, 1.83, 1.90);
  fit.SetParLimits(2, 0.004, 0.050);
  hist->Fit(&fit, "RQ0");
  fit.SetLineColor(kRed + 1);
  fit.Draw("same");
  const double bw = hist->GetXaxis()->GetBinWidth(1);
  const double yield = fit.GetParameter(0) * std::abs(fit.GetParameter(2)) * std::sqrt(2.0 * M_PI) / bw;
  const double chi2ndf = fit.GetNDF() > 0 ? fit.GetChisquare() / fit.GetNDF() : -1.0;
  summary << label << "\t" << hist->GetEntries() << "\t" << fit.GetParameter(1) << "\t"
          << std::abs(fit.GetParameter(2)) << "\t" << yield << "\t" << chi2ndf << "\n";
  return fit.GetParameter(1);
}

}  // namespace

int plot_dplus_mass(const char *inputSpec,
                    const char *outdir = "/eos/user/e/evzhang/codex-eos/outputs/dplus-peak/plots/latest",
                    const char *label = "dplus_mass") {
  gStyle->SetOptStat(0);
  ensure_dir(outdir);

  TChain chain("Dfinder/ntDkpipi");
  const auto files = split_inputs(inputSpec ? inputSpec : "");
  for (const auto &file : files) {
    chain.Add(file.c_str());
  }
  if (chain.GetEntries() <= 0) {
    std::cerr << "No Dfinder/ntDkpipi entries found for input: " << inputSpec << "\n";
    return 1;
  }
  chain.SetBranchStatus("*", 0);
  for (const char *branch : {"Dmass", "Dpt", "Dy", "Dtrk1Pt", "Dtrk2Pt", "Dtrk3Pt",
                             "Dchi2cl", "DsvpvDisErr", "DsvpvDistance", "Dalpha"}) {
    chain.SetBranchStatus(branch, 1);
  }

  auto *hRaw = new TH1D("hDplusRaw", ";m_{K#pi#pi} (GeV);Candidates / 5 MeV", 80, 1.67, 2.07);
  auto *hKin = new TH1D("hDplusKin", ";m_{K#pi#pi} (GeV);Candidates / 5 MeV", 80, 1.67, 2.07);
  auto *hLoose = new TH1D("hDplusLooseTopo", ";m_{K#pi#pi} (GeV);Candidates / 5 MeV", 80, 1.67, 2.07);
  auto *hMedium = new TH1D("hDplusMediumTopo", ";m_{K#pi#pi} (GeV);Candidates / 5 MeV", 80, 1.67, 2.07);
  auto *hTight = new TH1D("hDplusTightTopo", ";m_{K#pi#pi} (GeV);Candidates / 5 MeV", 80, 1.67, 2.07);
  style_hist(hRaw, kGray + 2);
  style_hist(hKin, kBlue + 1);
  style_hist(hLoose, kBlack);
  style_hist(hMedium, kGreen + 2);
  style_hist(hTight, kMagenta + 2);

  const std::string rawCut = "Dmass>1.67 && Dmass<2.07";
  const std::string kinCut = rawCut + " && Dpt>2 && Dpt<20 && abs(Dy)<2";
  const std::string looseCut = kinCut +
      " && Dtrk1Pt>0.2 && Dtrk2Pt>0.2 && Dtrk3Pt>0.2"
      " && Dchi2cl>0.05 && DsvpvDisErr>0 && DsvpvDistance/DsvpvDisErr>1.5"
      " && Dalpha<0.2";
  const std::string mediumCut = kinCut +
      " && Dtrk1Pt>0.5 && Dtrk2Pt>0.5 && Dtrk3Pt>0.5"
      " && Dchi2cl>0.08 && DsvpvDisErr>0 && DsvpvDistance/DsvpvDisErr>3.0"
      " && Dalpha<0.12";
  const std::string tightCut = kinCut +
      " && Dpt>3 && Dtrk1Pt>0.7 && Dtrk2Pt>0.7 && Dtrk3Pt>0.7"
      " && Dchi2cl>0.12 && DsvpvDisErr>0 && DsvpvDistance/DsvpvDisErr>5.0"
      " && Dalpha<0.08";

  TTreeReader reader(&chain);
  TTreeReaderArray<Float_t> Dmass(reader, "Dmass");
  TTreeReaderArray<Float_t> Dpt(reader, "Dpt");
  TTreeReaderArray<Float_t> Dy(reader, "Dy");
  TTreeReaderArray<Float_t> Dtrk1Pt(reader, "Dtrk1Pt");
  TTreeReaderArray<Float_t> Dtrk2Pt(reader, "Dtrk2Pt");
  TTreeReaderArray<Float_t> Dtrk3Pt(reader, "Dtrk3Pt");
  TTreeReaderArray<Float_t> Dchi2cl(reader, "Dchi2cl");
  TTreeReaderArray<Float_t> DsvpvDisErr(reader, "DsvpvDisErr");
  TTreeReaderArray<Float_t> DsvpvDistance(reader, "DsvpvDistance");
  TTreeReaderArray<Float_t> Dalpha(reader, "Dalpha");

  while (reader.Next()) {
    const auto nCand = std::min({Dmass.GetSize(), Dpt.GetSize(), Dy.GetSize(), Dtrk1Pt.GetSize(),
                                 Dtrk2Pt.GetSize(), Dtrk3Pt.GetSize(), Dchi2cl.GetSize(),
                                 DsvpvDisErr.GetSize(), DsvpvDistance.GetSize(), Dalpha.GetSize()});
    for (size_t i = 0; i < nCand; ++i) {
      const float mass = Dmass[i];
      if (!(mass > 1.67 && mass < 2.07)) continue;
      hRaw->Fill(mass);

      const bool passKin = Dpt[i] > 2.0 && Dpt[i] < 20.0 && std::abs(Dy[i]) < 2.0;
      if (!passKin) continue;
      hKin->Fill(mass);

      const bool passLoose =
          Dtrk1Pt[i] > 0.2 && Dtrk2Pt[i] > 0.2 && Dtrk3Pt[i] > 0.2 &&
          Dchi2cl[i] > 0.05 && DsvpvDisErr[i] > 0.0 &&
          DsvpvDistance[i] / DsvpvDisErr[i] > 1.5 && Dalpha[i] < 0.2;
      if (passLoose) hLoose->Fill(mass);

      const bool passMedium =
          Dtrk1Pt[i] > 0.5 && Dtrk2Pt[i] > 0.5 && Dtrk3Pt[i] > 0.5 &&
          Dchi2cl[i] > 0.08 && DsvpvDisErr[i] > 0.0 &&
          DsvpvDistance[i] / DsvpvDisErr[i] > 3.0 && Dalpha[i] < 0.12;
      if (passMedium) hMedium->Fill(mass);

      const bool passTight =
          Dpt[i] > 3.0 && Dtrk1Pt[i] > 0.7 && Dtrk2Pt[i] > 0.7 && Dtrk3Pt[i] > 0.7 &&
          Dchi2cl[i] > 0.12 && DsvpvDisErr[i] > 0.0 &&
          DsvpvDistance[i] / DsvpvDisErr[i] > 5.0 && Dalpha[i] < 0.08;
      if (passTight) hTight->Fill(mass);
    }
  }

  std::ofstream summary(std::string(outdir) + "/dplus_mass_summary.tsv");
  summary << "selection\tentries\tfit_mean_GeV\tfit_sigma_GeV\tfit_yield_like\tchi2ndf\n";

  TCanvas c("c_dplus_mass", "", 950, 720);
  c.SetTicks();
  hRaw->SetMinimum(0.0);
  hRaw->Draw("HIST");
  hKin->Draw("HIST same");
  hLoose->Draw("E same");
  fit_mass(hLoose, "fDplusLooseTopo", summary, "loose_topology");

  TLegend leg(0.54, 0.70, 0.88, 0.88);
  leg.SetBorderSize(0);
  leg.SetFillStyle(0);
  leg.AddEntry(hRaw, "Raw Dfinder K#pi#pi", "l");
  leg.AddEntry(hKin, "2 < p_{T} < 20, |y| < 2", "l");
  leg.AddEntry(hLoose, "Loose topology diagnostic", "lep");
  leg.Draw();
  stamp("2023 D^{+} #rightarrow K#pi#pi peak-search diagnostic");
  c.SaveAs((std::string(outdir) + "/" + label + ".png").c_str());
  c.SaveAs((std::string(outdir) + "/" + label + ".pdf").c_str());

  TCanvas cLoose("c_dplus_loose", "", 950, 720);
  cLoose.SetTicks();
  hLoose->Draw("E");
  fit_mass(hLoose, "fDplusLooseTopoStandalone", summary, "loose_topology_standalone");
  stamp("Loose D^{+} topology diagnostic");
  cLoose.SaveAs((std::string(outdir) + "/" + label + "_loose_topology.png").c_str());
  cLoose.SaveAs((std::string(outdir) + "/" + label + "_loose_topology.pdf").c_str());

  TCanvas cScan("c_dplus_scan", "", 950, 720);
  cScan.SetTicks();
  hLoose->SetMinimum(0.0);
  hLoose->Draw("E");
  hMedium->Draw("E same");
  hTight->Draw("E same");
  fit_mass(hMedium, "fDplusMediumTopo", summary, "medium_topology");
  fit_mass(hTight, "fDplusTightTopo", summary, "tight_topology");
  TLegend legScan(0.54, 0.72, 0.88, 0.88);
  legScan.SetBorderSize(0);
  legScan.SetFillStyle(0);
  legScan.AddEntry(hLoose, "Loose topology", "lep");
  legScan.AddEntry(hMedium, "Medium topology", "lep");
  legScan.AddEntry(hTight, "Tight topology", "lep");
  legScan.Draw();
  stamp("D^{+} topology scan diagnostic");
  cScan.SaveAs((std::string(outdir) + "/" + label + "_topology_scan.png").c_str());
  cScan.SaveAs((std::string(outdir) + "/" + label + "_topology_scan.pdf").c_str());

  TCanvas cMedium("c_dplus_medium", "", 950, 720);
  cMedium.SetTicks();
  hMedium->Draw("E");
  fit_mass(hMedium, "fDplusMediumTopoStandalone", summary, "medium_topology_standalone");
  stamp("Medium D^{+} topology diagnostic");
  cMedium.SaveAs((std::string(outdir) + "/" + label + "_medium_topology.png").c_str());
  cMedium.SaveAs((std::string(outdir) + "/" + label + "_medium_topology.pdf").c_str());

  TCanvas cTight("c_dplus_tight", "", 950, 720);
  cTight.SetTicks();
  hTight->Draw("E");
  fit_mass(hTight, "fDplusTightTopoStandalone", summary, "tight_topology_standalone");
  stamp("Tight D^{+} topology diagnostic");
  cTight.SaveAs((std::string(outdir) + "/" + label + "_tight_topology.png").c_str());
  cTight.SaveAs((std::string(outdir) + "/" + label + "_tight_topology.pdf").c_str());

  TFile fout((std::string(outdir) + "/" + label + ".root").c_str(), "RECREATE");
  hRaw->Write();
  hKin->Write();
  hLoose->Write();
  hMedium->Write();
  hTight->Write();
  fout.Close();

  std::ofstream readme(std::string(outdir) + "/README.md");
  readme << "# D+ Kpipi mass diagnostic\n\n";
  readme << "Input spec: `" << inputSpec << "`\n\n";
  readme << "Tree: `Dfinder/ntDkpipi`\n\n";
  readme << "Selections:\n";
  readme << "- raw: `" << rawCut << "`\n";
  readme << "- kinematic: `" << kinCut << "`\n";
  readme << "- loose topology: `" << looseCut << "`\n";
  readme << "- medium topology: `" << mediumCut << "`\n";
  readme << "- tight topology: `" << tightCut << "`\n";
  readme << "\nThese are diagnostic cuts for first peak finding, not final D+ analysis cuts.\n";

  std::cout << "entries=" << chain.GetEntries() << "\n";
  std::cout << "raw_candidates=" << hRaw->GetEntries() << "\n";
  std::cout << "kin_candidates=" << hKin->GetEntries() << "\n";
  std::cout << "loose_candidates=" << hLoose->GetEntries() << "\n";
  std::cout << "medium_candidates=" << hMedium->GetEntries() << "\n";
  std::cout << "tight_candidates=" << hTight->GetEntries() << "\n";
  std::cout << "outdir=" << outdir << "\n";
  return 0;
}
