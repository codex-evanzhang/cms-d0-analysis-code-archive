#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TChain.h"
#include "TFile.h"
#include "TH1D.h"
#include "TLine.h"
#include "TParameter.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"

namespace {
constexpr float D0_PDG_MASS = 1.86484f;
constexpr float D0_MASS_WINDOW = 0.2f;

struct DCutBin {
  const char *label;
  float absYMin;
  float absYMax;
  float ptMin;
  float ptMax;
  float alphaMax;
  float svpvSigMin;
  float vertexProbMin;
  float dthetaMax;
};

const std::vector<DCutBin> &CutBins() {
  static const std::vector<DCutBin> bins = {
      {"absy0to1_pt2to5", 0.0f, 1.0f, 2.0f, 5.0f, 0.4f, 2.5f, 0.1f, 0.5f},
      {"absy0to1_pt5to8", 0.0f, 1.0f, 5.0f, 8.0f, 0.35f, 3.5f, 0.1f, 0.3f},
      {"absy0to1_pt8to12", 0.0f, 1.0f, 8.0f, 12.0f, 0.4f, 3.5f, 0.1f, 0.3f},
      {"absy1to2_pt2to5", 1.0f, 2.0f, 2.0f, 5.0f, 0.2f, 2.5f, 0.1f, 0.3f},
      {"absy1to2_pt5to8", 1.0f, 2.0f, 5.0f, 8.0f, 0.25f, 3.5f, 0.1f, 0.3f},
      {"absy1to2_pt8to12", 1.0f, 2.0f, 8.0f, 12.0f, 0.25f, 3.5f, 0.1f, 0.3f},
  };
  return bins;
}

int FindCutBin(float pt, float y) {
  const float absY = std::fabs(y);
  const auto &bins = CutBins();
  for (size_t i = 0; i < bins.size(); ++i) {
    const bool inPt = (pt >= bins[i].ptMin) &&
                      (pt < bins[i].ptMax || (bins[i].ptMax == 12.0f && pt <= bins[i].ptMax));
    const bool inY = (absY >= bins[i].absYMin) &&
                     (absY < bins[i].absYMax || (bins[i].absYMax == 2.0f && absY <= bins[i].absYMax));
    if (inPt && inY) return static_cast<int>(i);
  }
  return -1;
}

void Save1D(TH1D &hist, const std::string &path, const std::vector<double> &cuts = {}, bool logy = false) {
  TCanvas canvas("canvas", "", 900, 700);
  if (logy) canvas.SetLogy();
  hist.SetLineWidth(2);
  hist.Draw("hist");
  for (double cut : cuts) {
    if (!std::isfinite(cut)) continue;
    TLine line(cut, 0.0, cut, hist.GetMaximum() * 1.05);
    line.SetLineColor(kRed + 1);
    line.SetLineWidth(2);
    line.Draw();
  }
  canvas.SaveAs(path.c_str());
}

void WriteAndPlotBinned(const std::vector<TH1D *> &hists, const std::string &plotDir,
                        const std::string &prefix, const std::vector<double> &cuts,
                        bool logy = false) {
  for (size_t i = 0; i < hists.size(); ++i) {
    hists[i]->Write();
    Save1D(*hists[i], plotDir + "/" + prefix + "_" + CutBins()[i].label + ".png", {cuts[i]}, logy);
  }
}
}

int ApplySection7DSelection(const char *inputName = "outputs/section5_event_selected.root",
                            const char *outputName = "outputs/section7_d_selected.root",
                            const char *plotDir = "plots",
                            Long64_t maxCandidates = -1) {
  gStyle->SetOptStat(1110);
  gSystem->mkdir("outputs", true);
  gSystem->mkdir(plotDir, true);
  const std::string dPlotDir = std::string(plotDir) + "/d_selection";
  gSystem->mkdir(dPlotDir.c_str(), true);

  TChain chain("Candidates");
  chain.Add(inputName);
  if (chain.GetEntries() <= 0) {
    std::cerr << "No Candidates entries found in " << inputName << std::endl;
    return 1;
  }

  TTreeReader reader(&chain);
  TTreeReaderValue<int> run(reader, "run");
  TTreeReaderValue<int> lumi(reader, "lumi");
  TTreeReaderValue<unsigned long long> event(reader, "event");
  TTreeReaderValue<int> nVtx(reader, "nVtx");
  TTreeReaderValue<float> bestVz(reader, "bestVz");
  TTreeReaderValue<float> zdcSumPlus(reader, "ZDCsumPlus");
  TTreeReaderValue<float> zdcSumMinus(reader, "ZDCsumMinus");
  TTreeReaderValue<float> hfEMaxPlus(reader, "HFEMaxPlus");
  TTreeReaderValue<float> hfEMaxMinus(reader, "HFEMaxMinus");
  TTreeReaderValue<int> direction(reader, "direction");
  TTreeReaderValue<int> dType(reader, "Dtype");
  TTreeReaderValue<float> dMass(reader, "Dmass");
  TTreeReaderValue<float> dPt(reader, "Dpt");
  TTreeReaderValue<float> dEta(reader, "Deta");
  TTreeReaderValue<float> dPhi(reader, "Dphi");
  TTreeReaderValue<float> dY(reader, "Dy");
  TTreeReaderValue<float> dChi2Cl(reader, "Dchi2cl");
  TTreeReaderValue<float> dAlpha(reader, "Dalpha");
  TTreeReaderValue<float> dDtheta(reader, "Ddtheta");
  TTreeReaderValue<float> dSvpvDistance(reader, "DsvpvDistance");
  TTreeReaderValue<float> dSvpvDisErr(reader, "DsvpvDisErr");
  TTreeReaderValue<float> dSvpvSig(reader, "DsvpvSig");
  TTreeReaderValue<float> dTrk1Pt(reader, "Dtrk1Pt");
  TTreeReaderValue<float> dTrk2Pt(reader, "Dtrk2Pt");
  TTreeReaderValue<float> dTrk1PtErr(reader, "Dtrk1PtErr");
  TTreeReaderValue<float> dTrk2PtErr(reader, "Dtrk2PtErr");
  TTreeReaderValue<float> dTrk1Eta(reader, "Dtrk1Eta");
  TTreeReaderValue<float> dTrk2Eta(reader, "Dtrk2Eta");
  TTreeReaderValue<int> dTrk1HighPurity(reader, "Dtrk1highPurity");
  TTreeReaderValue<int> dTrk2HighPurity(reader, "Dtrk2highPurity");

  TFile output(outputName, "RECREATE");
  TTree selected("SelectedD", "D0 candidates passing section 5 event cuts and section 7 D cuts");

  int oRun = 0, oLumi = 0, oNVtx = 0, oDirection = 0, oDtype = 0, oCutBin = -1;
  unsigned long long oEvent = 0;
  float oBestVz = 0.0f, oZDCPlus = 0.0f, oZDCMinus = 0.0f, oHFPlus = 0.0f, oHFMinus = 0.0f;
  float oDmass = 0.0f, oDpt = 0.0f, oDeta = 0.0f, oDphi = 0.0f, oDy = 0.0f, oDchi2cl = 0.0f;
  float oDalpha = 0.0f, oDdtheta = 0.0f, oDsvpvDistance = 0.0f, oDsvpvDisErr = 0.0f, oDsvpvSig = 0.0f;
  float oDtrk1Pt = 0.0f, oDtrk2Pt = 0.0f, oDtrk1PtErr = 0.0f, oDtrk2PtErr = 0.0f;
  float oDtrk1Eta = 0.0f, oDtrk2Eta = 0.0f, oDtrk1RelPtErr = 0.0f, oDtrk2RelPtErr = 0.0f;
  int oDtrk1HighPurity = 0, oDtrk2HighPurity = 0;
  selected.Branch("run", &oRun, "run/I");
  selected.Branch("lumi", &oLumi, "lumi/I");
  selected.Branch("event", &oEvent, "event/l");
  selected.Branch("nVtx", &oNVtx, "nVtx/I");
  selected.Branch("bestVz", &oBestVz, "bestVz/F");
  selected.Branch("ZDCsumPlus", &oZDCPlus, "ZDCsumPlus/F");
  selected.Branch("ZDCsumMinus", &oZDCMinus, "ZDCsumMinus/F");
  selected.Branch("HFEMaxPlus", &oHFPlus, "HFEMaxPlus/F");
  selected.Branch("HFEMaxMinus", &oHFMinus, "HFEMaxMinus/F");
  selected.Branch("direction", &oDirection, "direction/I");
  selected.Branch("cutBin", &oCutBin, "cutBin/I");
  selected.Branch("Dtype", &oDtype, "Dtype/I");
  selected.Branch("Dmass", &oDmass, "Dmass/F");
  selected.Branch("Dpt", &oDpt, "Dpt/F");
  selected.Branch("Deta", &oDeta, "Deta/F");
  selected.Branch("Dphi", &oDphi, "Dphi/F");
  selected.Branch("Dy", &oDy, "Dy/F");
  selected.Branch("Dchi2cl", &oDchi2cl, "Dchi2cl/F");
  selected.Branch("Dalpha", &oDalpha, "Dalpha/F");
  selected.Branch("Ddtheta", &oDdtheta, "Ddtheta/F");
  selected.Branch("DsvpvDistance", &oDsvpvDistance, "DsvpvDistance/F");
  selected.Branch("DsvpvDisErr", &oDsvpvDisErr, "DsvpvDisErr/F");
  selected.Branch("DsvpvSig", &oDsvpvSig, "DsvpvSig/F");
  selected.Branch("Dtrk1Pt", &oDtrk1Pt, "Dtrk1Pt/F");
  selected.Branch("Dtrk2Pt", &oDtrk2Pt, "Dtrk2Pt/F");
  selected.Branch("Dtrk1PtErr", &oDtrk1PtErr, "Dtrk1PtErr/F");
  selected.Branch("Dtrk2PtErr", &oDtrk2PtErr, "Dtrk2PtErr/F");
  selected.Branch("Dtrk1RelPtErr", &oDtrk1RelPtErr, "Dtrk1RelPtErr/F");
  selected.Branch("Dtrk2RelPtErr", &oDtrk2RelPtErr, "Dtrk2RelPtErr/F");
  selected.Branch("Dtrk1Eta", &oDtrk1Eta, "Dtrk1Eta/F");
  selected.Branch("Dtrk2Eta", &oDtrk2Eta, "Dtrk2Eta/F");
  selected.Branch("Dtrk1highPurity", &oDtrk1HighPurity, "Dtrk1highPurity/I");
  selected.Branch("Dtrk2highPurity", &oDtrk2HighPurity, "Dtrk2highPurity/I");

  TH1D hCutFlow("hDCutFlow", "D-candidate cut flow;Cut stage;Candidates", 12, 0, 12);
  const char *cutLabels[] = {"All", "Mass window", "Trk high purity", "|trk #eta|<2.4",
                             "trk p_{T}>1", "trk p_{T} err/p_{T}<0.1", "2<D p_{T}<12",
                             "|D y|<2", "D #alpha", "D L_{3D}/#sigma", "SV prob.", "D #Delta#theta"};
  for (int i = 1; i <= 12; ++i) hCutFlow.GetXaxis()->SetBinLabel(i, cutLabels[i - 1]);

  TH1D hMassBefore("hDmass_before_cut", "D^{0} mass before mass-window cut;M_{K#pi} (GeV);Candidates", 160, 1.5, 2.3);
  TH1D hTrk1HPBefore("hDtrk1HighPurity_before_cut", "Daughter 1 highPurity before cut;highPurity;Candidates", 3, -0.5, 2.5);
  TH1D hTrk2HPBefore("hDtrk2HighPurity_before_cut", "Daughter 2 highPurity before cut;highPurity;Candidates", 3, -0.5, 2.5);
  TH1D hTrk1EtaBefore("hDtrk1Eta_before_cut", "Daughter 1 #eta before cut;#eta;Candidates", 120, -3.5, 3.5);
  TH1D hTrk2EtaBefore("hDtrk2Eta_before_cut", "Daughter 2 #eta before cut;#eta;Candidates", 120, -3.5, 3.5);
  TH1D hTrk1PtBefore("hDtrk1Pt_before_cut", "Daughter 1 p_{T} before cut;p_{T} (GeV);Candidates", 120, 0, 12);
  TH1D hTrk2PtBefore("hDtrk2Pt_before_cut", "Daughter 2 p_{T} before cut;p_{T} (GeV);Candidates", 120, 0, 12);
  TH1D hTrk1RelErrBefore("hDtrk1RelPtErr_before_cut", "Daughter 1 p_{T} error/p_{T} before cut;p_{T} error/p_{T};Candidates", 100, 0, 0.5);
  TH1D hTrk2RelErrBefore("hDtrk2RelPtErr_before_cut", "Daughter 2 p_{T} error/p_{T} before cut;p_{T} error/p_{T};Candidates", 100, 0, 0.5);
  TH1D hDptBefore("hDpt_before_cut", "D^{0} p_{T} before analysis-range cut;p_{T} (GeV);Candidates", 140, 0, 14);
  TH1D hDyBefore("hDy_before_cut", "D^{0} y before analysis-range cut;y;Candidates", 120, -3, 3);
  TH1D hMassSelected("hDmass_selected", "Selected D^{0} candidates;M_{K#pi} (GeV);Candidates", 160, 1.5, 2.3);
  TH1D hMassSelectedZoom("hDmass_selected_zoom", "Selected D^{0} candidates;M_{K#pi} (GeV);Candidates", 120, 1.75, 1.98);

  std::vector<TH1D *> hAlpha, hSvpvSig, hChi2Cl, hDtheta;
  std::vector<double> alphaCuts, svpvCuts, chi2Cuts, dthetaCuts;
  for (const auto &bin : CutBins()) {
    hAlpha.push_back(new TH1D((std::string("hDalpha_before_cut_") + bin.label).c_str(),
                              (std::string("D #alpha before cut, ") + bin.label + ";#alpha;Candidates").c_str(),
                              100, 0, 1.5));
    hSvpvSig.push_back(new TH1D((std::string("hDsvpvSig_before_cut_") + bin.label).c_str(),
                                (std::string("D L_{3D}/#sigma before cut, ") + bin.label + ";L_{3D}/#sigma;Candidates").c_str(),
                                120, 0, 12));
    hChi2Cl.push_back(new TH1D((std::string("hDchi2cl_before_cut_") + bin.label).c_str(),
                               (std::string("SV probability before cut, ") + bin.label + ";SV probability;Candidates").c_str(),
                               100, 0, 1));
    hDtheta.push_back(new TH1D((std::string("hDdtheta_before_cut_") + bin.label).c_str(),
                               (std::string("D #Delta#theta before cut, ") + bin.label + ";#Delta#theta;Candidates").c_str(),
                               100, 0, 1.2));
    alphaCuts.push_back(bin.alphaMax);
    svpvCuts.push_back(bin.svpvSigMin);
    chi2Cuts.push_back(bin.vertexProbMin);
    dthetaCuts.push_back(bin.dthetaMax);
  }

  Long64_t nInputCandidates = 0;
  Long64_t nSelected = 0;

  while (reader.Next()) {
    if (maxCandidates >= 0 && nInputCandidates >= maxCandidates) break;
    ++nInputCandidates;
    if (nInputCandidates % 500000 == 0) {
      std::cout << "Section 7 processed " << nInputCandidates << " candidates, selected " << nSelected << std::endl;
    }
    hCutFlow.Fill(0.5);

    hMassBefore.Fill(*dMass);
    if (std::fabs(*dMass - D0_PDG_MASS) >= D0_MASS_WINDOW) continue;
    hCutFlow.Fill(1.5);

    hTrk1HPBefore.Fill(*dTrk1HighPurity);
    hTrk2HPBefore.Fill(*dTrk2HighPurity);
    if (*dTrk1HighPurity != 1 || *dTrk2HighPurity != 1) continue;
    hCutFlow.Fill(2.5);

    hTrk1EtaBefore.Fill(*dTrk1Eta);
    hTrk2EtaBefore.Fill(*dTrk2Eta);
    if (std::fabs(*dTrk1Eta) >= 2.4f || std::fabs(*dTrk2Eta) >= 2.4f) continue;
    hCutFlow.Fill(3.5);

    hTrk1PtBefore.Fill(*dTrk1Pt);
    hTrk2PtBefore.Fill(*dTrk2Pt);
    if (*dTrk1Pt <= 1.0f || *dTrk2Pt <= 1.0f) continue;
    hCutFlow.Fill(4.5);

    const float relErr1 = (*dTrk1Pt > 0.0f) ? *dTrk1PtErr / *dTrk1Pt : 999.0f;
    const float relErr2 = (*dTrk2Pt > 0.0f) ? *dTrk2PtErr / *dTrk2Pt : 999.0f;
    hTrk1RelErrBefore.Fill(relErr1);
    hTrk2RelErrBefore.Fill(relErr2);
    if (relErr1 >= 0.1f || relErr2 >= 0.1f) continue;
    hCutFlow.Fill(5.5);

    hDptBefore.Fill(*dPt);
    if (*dPt <= 2.0f || *dPt > 12.0f) continue;
    hCutFlow.Fill(6.5);

    hDyBefore.Fill(*dY);
    if (std::fabs(*dY) > 2.0f) continue;
    hCutFlow.Fill(7.5);

    const int binIndex = FindCutBin(*dPt, *dY);
    if (binIndex < 0) continue;
    const DCutBin &bin = CutBins()[binIndex];

    hAlpha[binIndex]->Fill(*dAlpha);
    if (*dAlpha >= bin.alphaMax) continue;
    hCutFlow.Fill(8.5);

    hSvpvSig[binIndex]->Fill(*dSvpvSig);
    if (*dSvpvSig <= bin.svpvSigMin) continue;
    hCutFlow.Fill(9.5);

    hChi2Cl[binIndex]->Fill(*dChi2Cl);
    if (*dChi2Cl <= bin.vertexProbMin) continue;
    hCutFlow.Fill(10.5);

    hDtheta[binIndex]->Fill(*dDtheta);
    if (*dDtheta >= bin.dthetaMax) continue;
    hCutFlow.Fill(11.5);

    oRun = *run;
    oLumi = *lumi;
    oEvent = *event;
    oNVtx = *nVtx;
    oBestVz = *bestVz;
    oZDCPlus = *zdcSumPlus;
    oZDCMinus = *zdcSumMinus;
    oHFPlus = *hfEMaxPlus;
    oHFMinus = *hfEMaxMinus;
    oDirection = *direction;
    oCutBin = binIndex;
    oDtype = *dType;
    oDmass = *dMass;
    oDpt = *dPt;
    oDeta = *dEta;
    oDphi = *dPhi;
    oDy = *dY;
    oDchi2cl = *dChi2Cl;
    oDalpha = *dAlpha;
    oDdtheta = *dDtheta;
    oDsvpvDistance = *dSvpvDistance;
    oDsvpvDisErr = *dSvpvDisErr;
    oDsvpvSig = *dSvpvSig;
    oDtrk1Pt = *dTrk1Pt;
    oDtrk2Pt = *dTrk2Pt;
    oDtrk1PtErr = *dTrk1PtErr;
    oDtrk2PtErr = *dTrk2PtErr;
    oDtrk1RelPtErr = relErr1;
    oDtrk2RelPtErr = relErr2;
    oDtrk1Eta = *dTrk1Eta;
    oDtrk2Eta = *dTrk2Eta;
    oDtrk1HighPurity = *dTrk1HighPurity;
    oDtrk2HighPurity = *dTrk2HighPurity;
    selected.Fill();
    hMassSelected.Fill(*dMass);
    hMassSelectedZoom.Fill(*dMass);
    ++nSelected;
  }

  output.cd();
  TParameter<Long64_t>("nInputCandidates", nInputCandidates).Write();
  TParameter<Long64_t>("nSelectedCandidates", nSelected).Write();
  hCutFlow.Write();
  hMassBefore.Write();
  hTrk1HPBefore.Write();
  hTrk2HPBefore.Write();
  hTrk1EtaBefore.Write();
  hTrk2EtaBefore.Write();
  hTrk1PtBefore.Write();
  hTrk2PtBefore.Write();
  hTrk1RelErrBefore.Write();
  hTrk2RelErrBefore.Write();
  hDptBefore.Write();
  hDyBefore.Write();
  hMassSelected.Write();
  hMassSelectedZoom.Write();
  WriteAndPlotBinned(hAlpha, dPlotDir, "12_Dalpha", alphaCuts);
  WriteAndPlotBinned(hSvpvSig, dPlotDir, "13_DsvpvSig", svpvCuts, true);
  WriteAndPlotBinned(hChi2Cl, dPlotDir, "14_Dchi2cl", chi2Cuts);
  WriteAndPlotBinned(hDtheta, dPlotDir, "15_Ddtheta", dthetaCuts);
  selected.Write();
  output.Close();

  Save1D(hCutFlow, dPlotDir + "/00_d_cutflow.png", {}, true);
  Save1D(hMassBefore, dPlotDir + "/01_Dmass.png",
         {D0_PDG_MASS - D0_MASS_WINDOW, D0_PDG_MASS + D0_MASS_WINDOW}, true);
  Save1D(hTrk1HPBefore, dPlotDir + "/02_Dtrk1HighPurity.png", {0.5});
  Save1D(hTrk2HPBefore, dPlotDir + "/03_Dtrk2HighPurity.png", {0.5});
  Save1D(hTrk1EtaBefore, dPlotDir + "/04_Dtrk1Eta.png", {-2.4, 2.4});
  Save1D(hTrk2EtaBefore, dPlotDir + "/05_Dtrk2Eta.png", {-2.4, 2.4});
  Save1D(hTrk1PtBefore, dPlotDir + "/06_Dtrk1Pt.png", {1.0}, true);
  Save1D(hTrk2PtBefore, dPlotDir + "/07_Dtrk2Pt.png", {1.0}, true);
  Save1D(hTrk1RelErrBefore, dPlotDir + "/08_Dtrk1RelPtErr.png", {0.1}, true);
  Save1D(hTrk2RelErrBefore, dPlotDir + "/09_Dtrk2RelPtErr.png", {0.1}, true);
  Save1D(hDptBefore, dPlotDir + "/10_Dpt.png", {2.0, 12.0}, true);
  Save1D(hDyBefore, dPlotDir + "/11_Dy.png", {-2.0, 2.0});
  Save1D(hMassSelected, dPlotDir + "/16_Dmass_selected.png",
         {D0_PDG_MASS - D0_MASS_WINDOW, D0_PDG_MASS + D0_MASS_WINDOW}, true);
  Save1D(hMassSelectedZoom, dPlotDir + "/17_Dmass_selected_zoom.png", {D0_PDG_MASS}, false);

  std::cout << "Wrote " << outputName << std::endl;
  std::cout << "Section 7 summary: input candidates=" << nInputCandidates
            << " selected candidates=" << nSelected << std::endl;
  return 0;
}
