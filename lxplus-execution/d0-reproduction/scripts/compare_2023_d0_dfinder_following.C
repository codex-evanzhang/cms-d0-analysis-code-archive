#include <TCanvas.h>
#include <TFile.h>
#include <TF1.h>
#include <TH1D.h>
#include <TLegend.h>
#include <TLine.h>
#include <TROOT.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Stage {
  TString id;
  TString label;
  TString selection;
};

struct TopologyBin {
  const char* id;
  double yLow;
  double yHigh;
  double ptLow;
  double ptHigh;
  double alphaMax;
  double svpvSigMin;
  double dthetaMax;
};

const std::vector<TopologyBin>& topologyBins() {
  static const std::vector<TopologyBin> bins = {
      {"y0to1_pt2to5", 0.0, 1.0, 2.0, 5.0, 0.40, 2.5, 0.50},
      {"y0to1_pt5to8", 0.0, 1.0, 5.0, 8.0, 0.35, 3.5, 0.30},
      {"y0to1_pt8to12", 0.0, 1.0, 8.0, 12.0, 0.40, 3.5, 0.30},
      {"y1to2_pt2to5", 1.0, 2.0, 2.0, 5.0, 0.20, 2.5, 0.30},
      {"y1to2_pt5to8", 1.0, 2.0, 5.0, 8.0, 0.25, 3.5, 0.30},
      {"y1to2_pt8to12", 1.0, 2.0, 8.0, 12.0, 0.25, 3.5, 0.30},
  };
  return bins;
}

bool isOwnedOutput(const TString& path) {
  return path.BeginsWith("/afs/cern.ch/user/e/evzhang") ||
         path.BeginsWith("/afs/cern.ch/work/e/evzhang") ||
         path.BeginsWith("/eos/user/e/evzhang");
}

bool hasBranch(TTree* tree, const char* name) {
  return tree && tree->GetBranch(name);
}

TH1D* makeHist(const TString& name, const TString& title, int color) {
  auto* hist = new TH1D(name, title, 160, 1.5, 2.3);
  hist->Sumw2();
  hist->SetDirectory(nullptr);
  hist->SetLineColor(color);
  hist->SetMarkerColor(color);
  hist->SetLineWidth(2);
  hist->SetMarkerStyle(20);
  hist->SetMarkerSize(0.55);
  hist->GetXaxis()->SetTitle("m(K#pi) [GeV]");
  hist->GetYaxis()->SetTitle("Candidates");
  return hist;
}

void writeCsv(const TH1D* hist, const TString& path) {
  std::ofstream out(path.Data());
  out << "bin,low_edge,high_edge,center,count,error\n";
  for (int i = 1; i <= hist->GetNbinsX(); ++i) {
    out << i << "," << std::setprecision(8)
        << hist->GetXaxis()->GetBinLowEdge(i) << ","
        << hist->GetXaxis()->GetBinUpEdge(i) << ","
        << hist->GetXaxis()->GetBinCenter(i) << ","
        << hist->GetBinContent(i) << ","
        << hist->GetBinError(i) << "\n";
  }
}

TH1D* normalizedClone(const TH1D* hist, const TString& name, int color) {
  auto* clone = static_cast<TH1D*>(hist->Clone(name));
  clone->SetDirectory(nullptr);
  clone->SetLineColor(color);
  clone->SetMarkerColor(color);
  clone->SetLineWidth(2);
  if (clone->Integral() > 0) clone->Scale(1.0 / clone->Integral());
  clone->GetYaxis()->SetTitle("Normalized candidates");
  return clone;
}

void drawOverlay(TH1D* dfinder, TH1D* reference, const TString& stageLabel, const TString& outputStem, bool normalized) {
  gStyle->SetOptStat(0);
  TCanvas canvas("canvas", "", 950, 720);
  canvas.SetLeftMargin(0.12);
  canvas.SetRightMargin(0.04);
  canvas.SetTopMargin(0.08);
  canvas.SetBottomMargin(0.12);

  TH1D* drawDfinder = dfinder;
  TH1D* drawReference = reference;
  if (normalized) {
    drawDfinder = normalizedClone(dfinder, TString(dfinder->GetName()) + "_norm", kBlue + 2);
    drawReference = normalizedClone(reference, TString(reference->GetName()) + "_norm", kRed + 1);
  }

  double maxY = std::max(drawDfinder->GetMaximum(), drawReference->GetMaximum());
  drawDfinder->SetMaximum(maxY > 0 ? maxY * 1.28 : 1.0);
  drawDfinder->SetMinimum(0);
  drawDfinder->Draw("hist");
  drawReference->Draw("hist same");

  TLine pdg(1.86484, 0.0, 1.86484, maxY > 0 ? maxY * 1.18 : 1.0);
  pdg.SetLineColor(kGray + 2);
  pdg.SetLineStyle(2);
  pdg.SetLineWidth(2);
  pdg.Draw("same");

  TLegend legend(0.42, 0.64, 0.92, 0.88);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.AddEntry(drawDfinder, "Reproduced Dfinder on MINIAOD", "l");
  legend.AddEntry(drawReference, "Reference Dfinder skim", "l");
  legend.AddEntry(&pdg, "PDG D^{0} mass", "l");
  legend.SetHeader(stageLabel);
  legend.Draw();

  canvas.SaveAs(outputStem + (normalized ? "_normalized.png" : ".png"));
  canvas.SaveAs(outputStem + (normalized ? "_normalized.pdf" : ".pdf"));
}

TString csvSafeLabel(TString label) {
  label.ReplaceAll("\"", "\"\"");
  return label;
}

void drawCutflow(const std::vector<Stage>& stages,
                 const std::vector<double>& dfinderCounts,
                 const std::vector<double>& referenceCounts,
                 const TString& outputStem) {
  TCanvas canvas("cutflow_canvas", "", 1280, 760);
  canvas.SetLeftMargin(0.11);
  canvas.SetRightMargin(0.04);
  canvas.SetBottomMargin(0.30);
  canvas.SetLogy();

  TH1D hD("h_cutflow_dfinder", "Selection-stage candidate counts", stages.size(), 0.5, stages.size() + 0.5);
  TH1D hR("h_cutflow_reference", "Selection-stage candidate counts", stages.size(), 0.5, stages.size() + 0.5);
  hD.SetDirectory(nullptr);
  hR.SetDirectory(nullptr);
  hD.SetLineColor(kBlue + 2);
  hD.SetMarkerColor(kBlue + 2);
  hD.SetMarkerStyle(20);
  hD.SetLineWidth(2);
  hR.SetLineColor(kRed + 1);
  hR.SetMarkerColor(kRed + 1);
  hR.SetMarkerStyle(21);
  hR.SetLineWidth(2);

  for (std::size_t i = 0; i < stages.size(); ++i) {
    hD.GetXaxis()->SetBinLabel(i + 1, stages[i].label);
    hR.GetXaxis()->SetBinLabel(i + 1, stages[i].label);
    hD.SetBinContent(i + 1, std::max(0.5, dfinderCounts[i]));
    hR.SetBinContent(i + 1, std::max(0.5, referenceCounts[i]));
  }
  hD.GetYaxis()->SetTitle("Candidates");
  hD.SetMinimum(0.5);
  hD.SetMaximum(std::max(hD.GetMaximum(), hR.GetMaximum()) * 5.0);
  hD.LabelsOption("v", "X");
  hD.Draw("hist");
  hR.Draw("hist same");

  TLegend legend(0.48, 0.74, 0.92, 0.88);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.AddEntry(&hD, "Reproduced Dfinder", "l");
  legend.AddEntry(&hR, "Reference skim", "l");
  legend.Draw();

  canvas.SaveAs(outputStem + ".png");
  canvas.SaveAs(outputStem + ".pdf");
}

double fillHist(TTree* tree, TH1D* hist, const TString& selection, Long64_t maxEntries = 0) {
  gROOT->cd();
  TH1D* rootHist = static_cast<TH1D*>(hist->Clone(hist->GetName()));
  rootHist->SetDirectory(gROOT);
  rootHist->Reset();
  TString draw = "Dmass>>";
  draw += rootHist->GetName();
  const Long64_t nentries = maxEntries > 0 ? std::min(maxEntries, tree->GetEntries()) : tree->GetEntries();
  const double selected = tree->Draw(draw, selection, "goff", nentries);
  hist->Add(rootHist);
  rootHist->Delete();
  return selected;
}

double countDraw(TTree* tree, const TString& expression, const TString& selection, Long64_t maxEntries = 0) {
  const Long64_t nentries = maxEntries > 0 ? std::min(maxEntries, tree->GetEntries()) : tree->GetEntries();
  return tree->Draw(expression, selection, "goff", nentries);
}

TString binKinematicCut(const TopologyBin& bin) {
  return TString::Format("(abs(Dy)>=%g && abs(Dy)<%g && Dpt>=%g && Dpt<%g)",
                         bin.yLow, bin.yHigh, bin.ptLow, bin.ptHigh);
}

TString binnedRequirementCut(const TString& requirement) {
  TString out = "(";
  bool first = true;
  for (const auto& bin : topologyBins()) {
    if (!first) out += "||";
    first = false;
    out += "(" + binKinematicCut(bin) + "&&" + requirement + ")";
  }
  out += ")";
  return out;
}

TString alphaCut() {
  TString out = "(";
  bool first = true;
  for (const auto& bin : topologyBins()) {
    if (!first) out += "||";
    first = false;
    out += TString::Format("(%s&&Dalpha<%g)", binKinematicCut(bin).Data(), bin.alphaMax);
  }
  out += ")";
  return out;
}

TString svpvSigCut() {
  TString out = "(";
  bool first = true;
  for (const auto& bin : topologyBins()) {
    if (!first) out += "||";
    first = false;
    out += TString::Format("(%s&&DsvpvDisErr>0&&DsvpvDistance/DsvpvDisErr>%g)",
                           binKinematicCut(bin).Data(), bin.svpvSigMin);
  }
  out += ")";
  return out;
}

TString dthetaCut() {
  TString out = "(";
  bool first = true;
  for (const auto& bin : topologyBins()) {
    if (!first) out += "||";
    first = false;
    out += TString::Format("(%s&&Ddtheta<%g)", binKinematicCut(bin).Data(), bin.dthetaMax);
  }
  out += ")";
  return out;
}

void requireBranch(TTree* tree, const char* name, const char* treeName) {
  if (!hasBranch(tree, name)) {
    throw std::runtime_error(TString::Format("missing required branch %s in %s", name, treeName).Data());
  }
}

void validateBranches(TTree* tree, const char* treeName, bool requireHighPurity) {
  std::vector<const char*> required = {
      "Dmass", "Dpt", "Dy", "Dtrk1Eta", "Dtrk2Eta", "Dtrk1Pt", "Dtrk2Pt",
      "Dtrk1PtErr", "Dtrk2PtErr", "Dalpha", "DsvpvDistance", "DsvpvDisErr",
      "Dchi2cl", "Ddtheta",
  };
  if (requireHighPurity) {
    required.push_back("Dtrk1highPurity");
    required.push_back("Dtrk2highPurity");
  }
  for (const char* name : required) requireBranch(tree, name, treeName);
}

std::vector<Stage> buildSection7Stages(bool includeHighPurity) {
  const TString all = "(Dmass==Dmass)";
  const TString mass = all + " && (abs(Dmass-1.86484)<0.2)";
  const TString hp = includeHighPurity ? mass + " && (Dtrk1highPurity==1 && Dtrk2highPurity==1)" : mass;
  const TString eta = hp + " && (abs(Dtrk1Eta)<2.4 && abs(Dtrk2Eta)<2.4)";
  const TString trkpt = eta + " && (Dtrk1Pt>1.0 && Dtrk2Pt>1.0)";
  const TString relerr = trkpt + " && (Dtrk1PtErr/Dtrk1Pt<0.1 && Dtrk2PtErr/Dtrk2Pt<0.1)";
  const TString dpt = relerr + " && (Dpt>2.0 && Dpt<=12.0)";
  const TString dy = dpt + " && (abs(Dy)<=2.0)";
  const TString alpha = dy + " && " + alphaCut();
  const TString svpv = alpha + " && " + svpvSigCut();
  const TString svprob = svpv + " && (Dchi2cl>0.10)";
  const TString dtheta = svprob + " && " + dthetaCut();

  return {
      {"stage00_all_candidates", "all candidates", all},
      {"stage01_mass_window", "|m-mD0|<0.2", mass},
      {"stage02_high_purity", includeHighPurity ? "track high purity" : "track high purity (not retained)", hp},
      {"stage03_track_eta", "track |eta|<2.4", eta},
      {"stage04_track_pt", "track pT>1", trkpt},
      {"stage05_track_rel_pt_err", "track rel pT err<0.1", relerr},
      {"stage06_dpt", "2<D pT<=12", dpt},
      {"stage07_dy", "|D y|<=2", dy},
      {"stage08_alpha", "binned alpha", alpha},
      {"stage09_svpv_sig", "binned SVPV sig", svpv},
      {"stage10_sv_prob", "SV prob>0.1", svprob},
      {"stage11_dtheta", "binned dtheta", dtheta},
  };
}

void fitIfUseful(TH1D* hist, std::ofstream& manifest, const TString& label) {
  if (hist->Integral() < 200) {
    manifest << "- `" << label << "` fit skipped: fewer than 200 entries.\n";
    return;
  }
  TF1 fit("d0_peak_fit", "gaus(0)+pol1(3)", 1.78, 1.95);
  fit.SetParameters(hist->GetMaximum(), 1.865, 0.015, 1.0, 0.0);
  int status = hist->Fit(&fit, "QNR");
  const double sigma = fit.GetParameter(2);
  if (status != 0 || sigma <= 0.0 || sigma > 0.2) {
    manifest << "- `" << label << "` fit diagnostic rejected: status `" << status
             << "`, sigma `" << std::setprecision(8) << sigma << " GeV`.\n";
    return;
  }
  manifest << "- `" << label << "` diagnostic fit status: `" << status << "`, mean: `"
           << std::setprecision(8) << fit.GetParameter(1) << " GeV`, sigma: `"
           << sigma << " GeV`\n";
}

}  // namespace

int compare_2023_d0_dfinder_following(const char* dfinderPath,
                                      const char* referencePath,
                                      const char* outputDir,
                                      const char* inputLabel = "",
                                      Long64_t maxReferenceEntries = 0) {
  TString outDir(outputDir);
  if (!isOwnedOutput(outDir)) {
    std::cerr << "ERROR: refusing to write outside Evan-owned paths: " << outDir << "\n";
    return 2;
  }
  gSystem->mkdir(outDir, true);

  TFile dfinderFile(dfinderPath, "READ");
  if (dfinderFile.IsZombie()) {
    std::cerr << "ERROR: failed to open Dfinder output " << dfinderPath << "\n";
    return 2;
  }
  auto* dfinderTree = dynamic_cast<TTree*>(dfinderFile.Get("Dfinder/ntDkpi"));
  if (!dfinderTree) {
    std::cerr << "ERROR: missing Dfinder/ntDkpi in " << dfinderPath << "\n";
    return 2;
  }

  TFile referenceFile(referencePath, "READ");
  if (referenceFile.IsZombie()) {
    std::cerr << "ERROR: failed to open reference " << referencePath << "\n";
    return 2;
  }
  auto* referenceTree = dynamic_cast<TTree*>(referenceFile.Get("Tree"));
  if (!referenceTree) {
    std::cerr << "ERROR: missing Tree in reference " << referencePath << "\n";
    return 2;
  }

  try {
    validateBranches(dfinderTree, "Dfinder/ntDkpi", true);
    validateBranches(referenceTree, "reference Tree", false);
  } catch (const std::runtime_error& err) {
    std::cerr << "ERROR: " << err.what() << "\n";
    return 2;
  }

  auto dfinderStages = buildSection7Stages(true);
  auto referenceStages = buildSection7Stages(false);
  std::vector<double> dfinderCounts;
  std::vector<double> referenceCounts;
  std::vector<TH1D*> dfinderHists;
  std::vector<TH1D*> referenceHists;

  TFile outRoot(outDir + "/dfinder_following_stage_histograms.root", "RECREATE");
  for (std::size_t i = 0; i < dfinderStages.size(); ++i) {
    auto* hD = makeHist("h_dfinder_" + dfinderStages[i].id, dfinderStages[i].label, kBlue + 2);
    auto* hR = makeHist("h_reference_" + referenceStages[i].id, referenceStages[i].label, kRed + 1);
    dfinderCounts.push_back(fillHist(dfinderTree, hD, dfinderStages[i].selection));
    referenceCounts.push_back(fillHist(referenceTree, hR, referenceStages[i].selection, maxReferenceEntries));
    outRoot.cd();
    hD->Write();
    hR->Write();
    writeCsv(hD, outDir + "/" + TString(hD->GetName()) + ".csv");
    writeCsv(hR, outDir + "/" + TString(hR->GetName()) + ".csv");
    drawOverlay(hD, hR, dfinderStages[i].label, outDir + "/" + dfinderStages[i].id + "_mass_overlay", false);
    drawOverlay(hD, hR, dfinderStages[i].label, outDir + "/" + dfinderStages[i].id + "_mass_overlay", true);
    dfinderHists.push_back(hD);
    referenceHists.push_back(hR);
  }
  drawCutflow(dfinderStages, dfinderCounts, referenceCounts, outDir + "/dfinder_following_cutflow");
  outRoot.Close();

  std::ofstream counts((outDir + "/stage_counts.csv").Data());
  counts << "stage,label,dfinder_candidates,reference_candidates,dfinder_selection,reference_selection\n";
  for (std::size_t i = 0; i < dfinderStages.size(); ++i) {
    counts << dfinderStages[i].id << ",\""
           << csvSafeLabel(dfinderStages[i].label) << "\","
           << std::setprecision(12) << dfinderCounts[i] << ","
           << referenceCounts[i] << ",\""
           << csvSafeLabel(dfinderStages[i].selection) << "\",\""
           << csvSafeLabel(referenceStages[i].selection) << "\"\n";
  }
  counts.close();

  const double dfinderDtype12 = hasBranch(dfinderTree, "Dtype")
      ? countDraw(dfinderTree, "Dtype", "(Dmass==Dmass)&&(Dtype==1||Dtype==2)")
      : -1.0;
  const double dfinderAll = dfinderCounts.empty() ? 0.0 : dfinderCounts.front();
  const double dfinderMass = dfinderCounts.size() > 1 ? dfinderCounts[1] : 0.0;
  const double dfinderHighPurity = dfinderCounts.size() > 2 ? dfinderCounts[2] : 0.0;

  std::ofstream manifest((outDir + "/dfinder_following_manifest.md").Data());
  manifest << "# Dfinder-Following 2023 D0 Comparison\n\n";
  manifest << "This is an exact Section-7-style validation of the Dfinder Kpi tree against the reference selected skim.\n\n";
  manifest << "- Reproduced Dfinder output: `" << dfinderPath << "`\n";
  manifest << "- Reference Dfinder skim: `" << referencePath << "`\n";
  manifest << "- MINIAOD input label: `" << inputLabel << "`\n";
  manifest << "- Reproduced Dfinder tree entries: `" << dfinderTree->GetEntries() << "`\n";
  manifest << "- Reference tree entries: `" << referenceTree->GetEntries() << "`\n\n";
  manifest << "- Reference entries scanned per stage: `"
           << (maxReferenceEntries > 0 ? std::min(maxReferenceEntries, referenceTree->GetEntries()) : referenceTree->GetEntries())
           << "`\n\n";
  manifest << "## Producer Schema Checks\n\n";
  manifest << "- Required analysis branches exist in both `Dfinder/ntDkpi` and the reference `Tree`.\n";
  manifest << "- The reproduced Dfinder tree retains `Dtrk1highPurity` and `Dtrk2highPurity`; the reduced reference tree does not, so the high-purity stage is a no-op on the reference side.\n";
  manifest << "- `ntDkpi` entries are event-level rows; D candidates are represented by D-array branches.\n";
  if (dfinderDtype12 >= 0.0) {
    manifest << "- Reproduced candidates with Dtype 1 or 2: `" << std::setprecision(12) << dfinderDtype12
             << "` out of all candidates `" << dfinderAll << "`.\n";
  } else {
    manifest << "- Dtype branch is absent, so D0/D0bar channel identity was not counted.\n";
  }
  manifest << "- High-purity branch check after the mass window: mass-window count `"
           << dfinderMass << "`, high-purity count `" << dfinderHighPurity
           << "`. These are expected to match for Dfinder outputs because high purity is applied before candidate construction.\n\n";
  manifest << "## Counts\n\n";
  for (std::size_t i = 0; i < dfinderStages.size(); ++i) {
    manifest << "- `" << dfinderStages[i].id << "` " << dfinderStages[i].label
             << ": reproduced `" << std::setprecision(12) << dfinderCounts[i]
             << "`, reference `" << referenceCounts[i] << "`\n";
  }
  manifest << "\n## Peak Fits\n\n";
  fitIfUseful(dfinderHists.back(), manifest, "reproduced_topology_selected");
  fitIfUseful(referenceHists.back(), manifest, "reference_topology_selected");
  manifest << "\n## Selection Notes\n\n";
  manifest << "- The validation follows the order in `ApplySection7DSelection.C`: mass window, high purity, daughter-track eta, daughter-track pT, daughter-track relative pT error, D pT, D rapidity, binned alpha, binned decay-length significance, secondary-vertex probability, and binned dtheta.\n";
  manifest << "- The count table uses ROOT's selected-candidate count. The mass-overlay histograms are drawn over `1.5 < m(Kpi) < 2.3 GeV`, so the stage-0 plotted integral can be lower than the stage-0 table count.\n";
  manifest << "- The binned topology thresholds are copied into this macro explicitly so both trees are tested with the same formulas.\n";
  manifest << "- The reproduced Dfinder output is produced from the 2023 MINIAOD using the CMSSW Dfinder analyzer, not by copying branches from the reference skim.\n";
  manifest << "- This does not yet validate the upstream 2023 ZDC/event-filter chain; the old `zdcreco2023HardCode` module is not present in the inspected modern CMSSW release.\n";
  manifest << "- The reproduced sample is limited by the requested MINIAOD event count; absolute yields are not expected to match the full reference skim until the same event set is processed.\n";
  manifest.close();

  std::cout << outDir << "/dfinder_following_manifest.md\n";
  return 0;
}
