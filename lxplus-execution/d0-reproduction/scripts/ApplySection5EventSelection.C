#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TChain.h"
#include "TFile.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TLine.h"
#include "TParameter.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"
#include "TTreeReader.h"
#include "TTreeReaderArray.h"
#include "TTreeReaderValue.h"

namespace {
constexpr float ZDC_PLUS_1N_THRESHOLD = 1100.0f;
constexpr float ZDC_MINUS_1N_THRESHOLD = 1000.0f;
constexpr float HF_PLUS_GAP_THRESHOLD = 9.2f;
constexpr float HF_MINUS_GAP_THRESHOLD = 8.6f;

void Save1D(TH1D &hist, const std::string &path, double cut = std::numeric_limits<double>::quiet_NaN(),
            bool logy = false) {
  TCanvas canvas("canvas", "", 900, 700);
  if (logy) canvas.SetLogy();
  hist.SetLineWidth(2);
  hist.Draw("hist");
  if (std::isfinite(cut)) {
    TLine line(cut, 0.0, cut, hist.GetMaximum() * 1.05);
    line.SetLineColor(kRed + 1);
    line.SetLineWidth(2);
    line.Draw();
  }
  canvas.SaveAs(path.c_str());
}

void Save2D(TH2D &hist, const std::string &path) {
  TCanvas canvas("canvas", "", 900, 750);
  canvas.SetRightMargin(0.14);
  hist.Draw("colz");
  canvas.SaveAs(path.c_str());
}

float BestVertexZ(int nVtx, const std::vector<float> *zVtx, const std::vector<float> *ptSumVtx) {
  if (nVtx <= 0 || zVtx == nullptr || zVtx->empty()) return 9999.0f;
  int best = 0;
  if (ptSumVtx != nullptr && !ptSumVtx->empty()) {
    for (int i = 1; i < nVtx && i < static_cast<int>(ptSumVtx->size()); ++i) {
      if (ptSumVtx->at(i) > ptSumVtx->at(best)) best = i;
    }
  }
  if (best >= static_cast<int>(zVtx->size())) best = 0;
  return zVtx->at(best);
}

float MaxHFEnergy(const std::vector<int> &pfId, const std::vector<float> &pfEta,
                  const std::vector<float> &pfE, float etaMin, float etaMax) {
  float maxE = 0.0f;
  const size_t n = std::min({pfId.size(), pfEta.size(), pfE.size()});
  for (size_t i = 0; i < n; ++i) {
    if ((pfId[i] == 6 || pfId[i] == 7) && pfEta[i] > etaMin && pfEta[i] < etaMax) {
      maxE = std::max(maxE, pfE[i]);
    }
  }
  return maxE;
}

bool ReadInputList(const char *inputList, std::vector<std::string> &files) {
  std::ifstream input(inputList);
  if (!input.is_open()) {
    std::cerr << "Could not open input list: " << inputList << std::endl;
    return false;
  }
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    files.push_back(line);
  }
  if (files.empty()) {
    std::cerr << "Input list has no files: " << inputList << std::endl;
    return false;
  }
  return true;
}
}

int ApplySection5EventSelection(const char *inputList = "inputs/all_files.txt",
                                const char *outputName = "outputs/section5_event_selected.root",
                                const char *plotDir = "plots",
                                int maxFiles = -1,
                                Long64_t maxEvents = -1) {
  gStyle->SetOptStat(1110);
  gSystem->mkdir("outputs", true);
  gSystem->mkdir(plotDir, true);
  const std::string eventPlotDir = std::string(plotDir) + "/event_selection";
  gSystem->mkdir(eventPlotDir.c_str(), true);

  std::vector<std::string> files;
  if (!ReadInputList(inputList, files)) return 1;
  if (maxFiles > 0 && maxFiles < static_cast<int>(files.size())) {
    files.resize(maxFiles);
  }

  TChain hi("hiEvtAnalyzer/HiTree");
  TChain track("PbPbTracks/trackTree");
  TChain skim("skimanalysis/HltTree");
  TChain met("l1MetFilterRecoTree/MetFilterRecoTree");
  TChain zdc("zdcanalyzer/zdcrechit");
  TChain pf("particleFlowAnalyser/pftree");
  TChain d("Dfinder/ntDkpi");
  for (const auto &file : files) {
    hi.Add(file.c_str());
    track.Add(file.c_str());
    skim.Add(file.c_str());
    met.Add(file.c_str());
    zdc.Add(file.c_str());
    pf.Add(file.c_str());
    d.Add(file.c_str());
  }

  const Long64_t entries = hi.GetEntries();
  if (entries <= 0 || track.GetEntries() != entries || skim.GetEntries() != entries ||
      met.GetEntries() != entries || zdc.GetEntries() != entries || pf.GetEntries() != entries ||
      d.GetEntries() != entries) {
    std::cerr << "Input trees are empty or have mismatched entry counts." << std::endl;
    std::cerr << "hi=" << entries << " track=" << track.GetEntries()
              << " skim=" << skim.GetEntries() << " met=" << met.GetEntries()
              << " zdc=" << zdc.GetEntries() << " pf=" << pf.GetEntries()
              << " d=" << d.GetEntries() << std::endl;
    return 1;
  }

  TTreeReader hiReader(&hi);
  TTreeReader trackReader(&track);
  TTreeReader skimReader(&skim);
  TTreeReader metReader(&met);
  TTreeReader zdcReader(&zdc);
  TTreeReader pfReader(&pf);
  TTreeReader dReader(&d);

  TTreeReaderValue<unsigned int> run(hiReader, "run");
  TTreeReaderValue<unsigned int> lumi(hiReader, "lumi");
  TTreeReaderValue<unsigned long long> evt(hiReader, "evt");
  TTreeReaderValue<int> nVtx(trackReader, "nVtx");
  TTreeReaderValue<std::vector<float>> zVtx(trackReader, "zVtx");
  TTreeReaderValue<std::vector<float>> ptSumVtx(trackReader, "ptSumVtx");
  TTreeReaderValue<int> pprimaryVertexFilter(skimReader, "pprimaryVertexFilter");
  TTreeReaderValue<int> pclusterCompatibilityFilter(skimReader, "pclusterCompatibilityFilter");
  TTreeReaderValue<Bool_t> cscTightHalo2015Filter(metReader, "cscTightHalo2015Filter");
  TTreeReaderValue<float> zdcSumPlus(zdcReader, "sumPlus");
  TTreeReaderValue<float> zdcSumMinus(zdcReader, "sumMinus");
  TTreeReaderValue<std::vector<int>> pfId(pfReader, "pfId");
  TTreeReaderValue<std::vector<float>> pfEta(pfReader, "pfEta");
  TTreeReaderValue<std::vector<float>> pfE(pfReader, "pfE");

  TTreeReaderValue<int> dRunNo(dReader, "RunNo");
  TTreeReaderValue<int> dLumiNo(dReader, "LumiNo");
  TTreeReaderValue<int> dEvtNo(dReader, "EvtNo");
  TTreeReaderValue<int> dSize(dReader, "Dsize");
  TTreeReaderArray<int> dType(dReader, "Dtype");
  TTreeReaderArray<float> dMass(dReader, "Dmass");
  TTreeReaderArray<float> dPt(dReader, "Dpt");
  TTreeReaderArray<float> dEta(dReader, "Deta");
  TTreeReaderArray<float> dPhi(dReader, "Dphi");
  TTreeReaderArray<float> dY(dReader, "Dy");
  TTreeReaderArray<float> dChi2Cl(dReader, "Dchi2cl");
  TTreeReaderArray<float> dDtheta(dReader, "Ddtheta");
  TTreeReaderArray<float> dAlpha(dReader, "Dalpha");
  TTreeReaderArray<float> dSvpvDistance(dReader, "DsvpvDistance");
  TTreeReaderArray<float> dSvpvDisErr(dReader, "DsvpvDisErr");
  TTreeReaderArray<float> dTrk1Pt(dReader, "Dtrk1Pt");
  TTreeReaderArray<float> dTrk2Pt(dReader, "Dtrk2Pt");
  TTreeReaderArray<float> dTrk1PtErr(dReader, "Dtrk1PtErr");
  TTreeReaderArray<float> dTrk2PtErr(dReader, "Dtrk2PtErr");
  TTreeReaderArray<float> dTrk1Eta(dReader, "Dtrk1Eta");
  TTreeReaderArray<float> dTrk2Eta(dReader, "Dtrk2Eta");
  TTreeReaderArray<Bool_t> dTrk1HighPurity(dReader, "Dtrk1highPurity");
  TTreeReaderArray<Bool_t> dTrk2HighPurity(dReader, "Dtrk2highPurity");

  TFile output(outputName, "RECREATE");
  TTree selectedEvents("Events", "Events passing section 5 offline selections");
  TTree candidates("Candidates", "Dfinder candidates in events passing section 5 offline selections");

  int outRun = 0, outLumi = 0, outNVtx = 0, outDirection = 0;
  unsigned long long outEvent = 0;
  float outBestVz = 0.0f, outZDCPlus = 0.0f, outZDCMinus = 0.0f;
  float outHFEMaxPlus = 0.0f, outHFEMaxMinus = 0.0f, outHFEMaxGapSide = 0.0f;
  int outPVFilter = 0, outClusterCompatibility = 0, outCSCFilter = 0;
  int outZDCGammaN = 0, outZDCNgamma = 0;
  int outNDCandidates = 0;
  selectedEvents.Branch("run", &outRun, "run/I");
  selectedEvents.Branch("lumi", &outLumi, "lumi/I");
  selectedEvents.Branch("event", &outEvent, "event/l");
  selectedEvents.Branch("nVtx", &outNVtx, "nVtx/I");
  selectedEvents.Branch("bestVz", &outBestVz, "bestVz/F");
  selectedEvents.Branch("pprimaryVertexFilter", &outPVFilter, "pprimaryVertexFilter/I");
  selectedEvents.Branch("pclusterCompatibilityFilter", &outClusterCompatibility, "pclusterCompatibilityFilter/I");
  selectedEvents.Branch("cscTightHalo2015Filter", &outCSCFilter, "cscTightHalo2015Filter/I");
  selectedEvents.Branch("ZDCsumPlus", &outZDCPlus, "ZDCsumPlus/F");
  selectedEvents.Branch("ZDCsumMinus", &outZDCMinus, "ZDCsumMinus/F");
  selectedEvents.Branch("HFEMaxPlus", &outHFEMaxPlus, "HFEMaxPlus/F");
  selectedEvents.Branch("HFEMaxMinus", &outHFEMaxMinus, "HFEMaxMinus/F");
  selectedEvents.Branch("HFEMaxGapSide", &outHFEMaxGapSide, "HFEMaxGapSide/F");
  selectedEvents.Branch("ZDCgammaN", &outZDCGammaN, "ZDCgammaN/I");
  selectedEvents.Branch("ZDCNgamma", &outZDCNgamma, "ZDCNgamma/I");
  selectedEvents.Branch("direction", &outDirection, "direction/I");
  selectedEvents.Branch("nDfinderCandidates", &outNDCandidates, "nDfinderCandidates/I");

  int cRun = 0, cLumi = 0, cNVtx = 0, cDirection = 0, cDtype = 0;
  unsigned long long cEvent = 0;
  float cBestVz = 0.0f, cZDCPlus = 0.0f, cZDCMinus = 0.0f, cHFEMaxPlus = 0.0f, cHFEMaxMinus = 0.0f;
  float cDmass = 0.0f, cDpt = 0.0f, cDeta = 0.0f, cDphi = 0.0f, cDy = 0.0f, cDchi2cl = 0.0f;
  float cDalpha = 0.0f, cDdtheta = 0.0f, cDsvpvDistance = 0.0f, cDsvpvDisErr = 0.0f, cDsvpvSig = 0.0f;
  float cDtrk1Pt = 0.0f, cDtrk2Pt = 0.0f, cDtrk1PtErr = 0.0f, cDtrk2PtErr = 0.0f;
  float cDtrk1Eta = 0.0f, cDtrk2Eta = 0.0f;
  int cDtrk1HighPurity = 0, cDtrk2HighPurity = 0;
  candidates.Branch("run", &cRun, "run/I");
  candidates.Branch("lumi", &cLumi, "lumi/I");
  candidates.Branch("event", &cEvent, "event/l");
  candidates.Branch("nVtx", &cNVtx, "nVtx/I");
  candidates.Branch("bestVz", &cBestVz, "bestVz/F");
  candidates.Branch("ZDCsumPlus", &cZDCPlus, "ZDCsumPlus/F");
  candidates.Branch("ZDCsumMinus", &cZDCMinus, "ZDCsumMinus/F");
  candidates.Branch("HFEMaxPlus", &cHFEMaxPlus, "HFEMaxPlus/F");
  candidates.Branch("HFEMaxMinus", &cHFEMaxMinus, "HFEMaxMinus/F");
  candidates.Branch("direction", &cDirection, "direction/I");
  candidates.Branch("Dtype", &cDtype, "Dtype/I");
  candidates.Branch("Dmass", &cDmass, "Dmass/F");
  candidates.Branch("Dpt", &cDpt, "Dpt/F");
  candidates.Branch("Deta", &cDeta, "Deta/F");
  candidates.Branch("Dphi", &cDphi, "Dphi/F");
  candidates.Branch("Dy", &cDy, "Dy/F");
  candidates.Branch("Dchi2cl", &cDchi2cl, "Dchi2cl/F");
  candidates.Branch("Dalpha", &cDalpha, "Dalpha/F");
  candidates.Branch("Ddtheta", &cDdtheta, "Ddtheta/F");
  candidates.Branch("DsvpvDistance", &cDsvpvDistance, "DsvpvDistance/F");
  candidates.Branch("DsvpvDisErr", &cDsvpvDisErr, "DsvpvDisErr/F");
  candidates.Branch("DsvpvSig", &cDsvpvSig, "DsvpvSig/F");
  candidates.Branch("Dtrk1Pt", &cDtrk1Pt, "Dtrk1Pt/F");
  candidates.Branch("Dtrk2Pt", &cDtrk2Pt, "Dtrk2Pt/F");
  candidates.Branch("Dtrk1PtErr", &cDtrk1PtErr, "Dtrk1PtErr/F");
  candidates.Branch("Dtrk2PtErr", &cDtrk2PtErr, "Dtrk2PtErr/F");
  candidates.Branch("Dtrk1Eta", &cDtrk1Eta, "Dtrk1Eta/F");
  candidates.Branch("Dtrk2Eta", &cDtrk2Eta, "Dtrk2Eta/F");
  candidates.Branch("Dtrk1highPurity", &cDtrk1HighPurity, "Dtrk1highPurity/I");
  candidates.Branch("Dtrk2highPurity", &cDtrk2HighPurity, "Dtrk2highPurity/I");

  TH1D hCutFlow("hEventCutFlow", "Event cut flow;Cut stage;Events", 8, 0, 8);
  const char *cutLabels[] = {"All", "PV filter", "|v_{z}|<15", "nVtx<=3", "Cluster comp.", "CSC halo", "ZDC XOR", "HF gap"};
  for (int i = 1; i <= 8; ++i) hCutFlow.GetXaxis()->SetBinLabel(i, cutLabels[i - 1]);
  TH1D hPVFilter("hPVFilter_before_cut", "Primary vertex filter before cut;filter value;Events", 3, -0.5, 2.5);
  TH1D hBestVz("hBestVz_before_cut", "Best primary-vertex z before cut;v_{z} (cm);Events", 120, -30, 30);
  TH1D hNVtx("hNVtx_before_cut", "Vertex multiplicity before cut;nVtx;Events", 20, -0.5, 19.5);
  TH1D hCluster("hClusterCompatibility_before_cut", "Cluster compatibility filter before cut;filter value;Events", 3, -0.5, 2.5);
  TH1D hCSC("hCSCTightHalo2015_before_cut", "CSC tight halo 2015 filter before cut;filter value;Events", 3, -0.5, 2.5);
  TH1D hZDCPlus("hZDCPlus_before_cut", "ZDC Plus energy before XOR cut;ZDC Plus energy (GeV);Events", 160, -1000, 15000);
  TH1D hZDCMinus("hZDCMinus_before_cut", "ZDC Minus energy before XOR cut;ZDC Minus energy (GeV);Events", 160, -1000, 15000);
  TH2D hZDC2D("hZDCPlusMinus_before_cut", "ZDC energies before XOR cut;ZDC Plus energy (GeV);ZDC Minus energy (GeV)", 120, -1000, 15000, 120, -1000, 15000);
  TH1D hHFPlus("hHFEMaxPlus_before_gap_cut", "HF Plus E_{max} before gap cut;E_{max}^{HF+} (GeV);Events", 140, 0, 70);
  TH1D hHFMinus("hHFEMaxMinus_before_gap_cut", "HF Minus E_{max} before gap cut;E_{max}^{HF-} (GeV);Events", 140, 0, 70);
  TH1D hHFGapside("hHFEMaxGapSide_before_gap_cut", "0n-side HF E_{max} before gap cut;E_{max}^{gap side} (GeV);Events", 140, 0, 70);

  Long64_t nEvents = 0;
  Long64_t nSelectedEvents = 0;
  Long64_t nCandidates = 0;
  Long64_t nSelectedEventCandidates = 0;

  while (hiReader.Next()) {
    if (maxEvents >= 0 && nEvents >= maxEvents) break;
    if (!trackReader.Next() || !skimReader.Next() || !metReader.Next() ||
        !zdcReader.Next() || !pfReader.Next() || !dReader.Next()) {
      std::cerr << "A friend tree ended before hiEvtAnalyzer/HiTree." << std::endl;
      return 1;
    }

    ++nEvents;
    if (nEvents % 100000 == 0) std::cout << "Section 5 processed " << nEvents << " / " << entries << " events" << std::endl;
    hCutFlow.Fill(0.5);

    const float bestVz = BestVertexZ(*nVtx, &(*zVtx), &(*ptSumVtx));
    hPVFilter.Fill(*pprimaryVertexFilter);
    if (*pprimaryVertexFilter != 1) continue;
    hCutFlow.Fill(1.5);

    hBestVz.Fill(bestVz);
    if (!std::isfinite(bestVz) || std::fabs(bestVz) >= 15.0f) continue;
    hCutFlow.Fill(2.5);

    hNVtx.Fill(*nVtx);
    if (*nVtx <= 0 || *nVtx > 3) continue;
    hCutFlow.Fill(3.5);

    hCluster.Fill(*pclusterCompatibilityFilter);
    if (*pclusterCompatibilityFilter != 1) continue;
    hCutFlow.Fill(4.5);

    hCSC.Fill(*cscTightHalo2015Filter ? 1 : 0);
    if (!*cscTightHalo2015Filter) continue;
    hCutFlow.Fill(5.5);

    const bool zdcGammaN = (*zdcSumMinus > ZDC_MINUS_1N_THRESHOLD && *zdcSumPlus < ZDC_PLUS_1N_THRESHOLD);
    const bool zdcNgamma = (*zdcSumPlus > ZDC_PLUS_1N_THRESHOLD && *zdcSumMinus < ZDC_MINUS_1N_THRESHOLD);
    hZDCPlus.Fill(*zdcSumPlus);
    hZDCMinus.Fill(*zdcSumMinus);
    hZDC2D.Fill(*zdcSumPlus, *zdcSumMinus);
    if (!zdcGammaN && !zdcNgamma) continue;
    hCutFlow.Fill(6.5);

    const float hfPlus = MaxHFEnergy(*pfId, *pfEta, *pfE, 3.0f, 5.2f);
    const float hfMinus = MaxHFEnergy(*pfId, *pfEta, *pfE, -5.2f, -3.0f);
    const float hfGapSide = zdcGammaN ? hfPlus : hfMinus;
    const bool passGap = (zdcGammaN && hfPlus < HF_PLUS_GAP_THRESHOLD) ||
                         (zdcNgamma && hfMinus < HF_MINUS_GAP_THRESHOLD);
    hHFPlus.Fill(hfPlus);
    hHFMinus.Fill(hfMinus);
    hHFGapside.Fill(hfGapSide);
    if (!passGap) continue;
    hCutFlow.Fill(7.5);

    ++nSelectedEvents;
    outRun = *run;
    outLumi = *lumi;
    outEvent = *evt;
    outNVtx = *nVtx;
    outBestVz = bestVz;
    outPVFilter = *pprimaryVertexFilter;
    outClusterCompatibility = *pclusterCompatibilityFilter;
    outCSCFilter = *cscTightHalo2015Filter ? 1 : 0;
    outZDCPlus = *zdcSumPlus;
    outZDCMinus = *zdcSumMinus;
    outHFEMaxPlus = hfPlus;
    outHFEMaxMinus = hfMinus;
    outHFEMaxGapSide = hfGapSide;
    outZDCGammaN = zdcGammaN ? 1 : 0;
    outZDCNgamma = zdcNgamma ? 1 : 0;
    outDirection = zdcGammaN ? 1 : -1;
    outNDCandidates = *dSize;
    selectedEvents.Fill();

    nCandidates += *dSize;
    const int size = std::min<int>(*dSize, dMass.GetSize());
    for (int i = 0; i < size; ++i) {
      cRun = *run;
      cLumi = *lumi;
      cEvent = *evt;
      cNVtx = *nVtx;
      cBestVz = bestVz;
      cZDCPlus = *zdcSumPlus;
      cZDCMinus = *zdcSumMinus;
      cHFEMaxPlus = hfPlus;
      cHFEMaxMinus = hfMinus;
      cDirection = outDirection;
      cDtype = dType[i];
      cDmass = dMass[i];
      cDpt = dPt[i];
      cDeta = dEta[i];
      cDphi = dPhi[i];
      cDy = dY[i];
      cDchi2cl = dChi2Cl[i];
      cDalpha = dAlpha[i];
      cDdtheta = dDtheta[i];
      cDsvpvDistance = dSvpvDistance[i];
      cDsvpvDisErr = dSvpvDisErr[i];
      cDsvpvSig = (dSvpvDisErr[i] > 0.0f) ? dSvpvDistance[i] / dSvpvDisErr[i] : -999.0f;
      cDtrk1Pt = dTrk1Pt[i];
      cDtrk2Pt = dTrk2Pt[i];
      cDtrk1PtErr = dTrk1PtErr[i];
      cDtrk2PtErr = dTrk2PtErr[i];
      cDtrk1Eta = dTrk1Eta[i];
      cDtrk2Eta = dTrk2Eta[i];
      cDtrk1HighPurity = dTrk1HighPurity[i] ? 1 : 0;
      cDtrk2HighPurity = dTrk2HighPurity[i] ? 1 : 0;
      candidates.Fill();
      ++nSelectedEventCandidates;
    }
  }

  output.cd();
  TParameter<int>("nInputFiles", static_cast<int>(files.size())).Write();
  TParameter<Long64_t>("nInputEvents", nEvents).Write();
  TParameter<Long64_t>("nSelectedEvents", nSelectedEvents).Write();
  TParameter<Long64_t>("nDfinderCandidatesInSelectedEvents", nSelectedEventCandidates).Write();
  hCutFlow.Write();
  hPVFilter.Write();
  hBestVz.Write();
  hNVtx.Write();
  hCluster.Write();
  hCSC.Write();
  hZDCPlus.Write();
  hZDCMinus.Write();
  hZDC2D.Write();
  hHFPlus.Write();
  hHFMinus.Write();
  hHFGapside.Write();
  selectedEvents.Write();
  candidates.Write();
  output.Close();

  Save1D(hCutFlow, eventPlotDir + "/00_event_cutflow.png", std::numeric_limits<double>::quiet_NaN(), true);
  Save1D(hPVFilter, eventPlotDir + "/01_primary_vertex_filter.png", 0.5);
  Save1D(hBestVz, eventPlotDir + "/02_best_vz.png", 15.0);
  Save1D(hNVtx, eventPlotDir + "/03_nVtx.png", 3.0);
  Save1D(hCluster, eventPlotDir + "/04_cluster_compatibility_filter.png", 0.5);
  Save1D(hCSC, eventPlotDir + "/05_csc_tight_halo_filter.png", 0.5);
  Save1D(hZDCPlus, eventPlotDir + "/06_zdc_plus_energy.png", ZDC_PLUS_1N_THRESHOLD, true);
  Save1D(hZDCMinus, eventPlotDir + "/07_zdc_minus_energy.png", ZDC_MINUS_1N_THRESHOLD, true);
  Save2D(hZDC2D, eventPlotDir + "/08_zdc_plus_vs_minus.png");
  Save1D(hHFPlus, eventPlotDir + "/09_hf_plus_emax.png", HF_PLUS_GAP_THRESHOLD, true);
  Save1D(hHFMinus, eventPlotDir + "/10_hf_minus_emax.png", HF_MINUS_GAP_THRESHOLD, true);
  Save1D(hHFGapside, eventPlotDir + "/11_hf_gap_side_emax.png", std::numeric_limits<double>::quiet_NaN(), true);

  std::cout << "Wrote " << outputName << std::endl;
  std::cout << "Section 5 summary: files=" << files.size()
            << " input events=" << nEvents
            << " selected events=" << nSelectedEvents
            << " candidates in selected events=" << nSelectedEventCandidates << std::endl;
  return 0;
}
