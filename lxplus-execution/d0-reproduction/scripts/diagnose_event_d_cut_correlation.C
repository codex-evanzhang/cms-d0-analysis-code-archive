#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TFile.h"
#include "TH1D.h"
#include "TSystem.h"
#include "TTree.h"
#include "TTreeReader.h"
#include "TTreeReaderArray.h"
#include "TTreeReaderValue.h"

namespace {
constexpr float kD0Mass = 1.86484f;
constexpr float kMassWindow = 0.2f;
constexpr float kZdcPlus1n = 1100.0f;
constexpr float kZdcMinus1n = 1000.0f;
constexpr float kHfPlusGap = 9.2f;
constexpr float kHfMinusGap = 8.6f;
constexpr int kStages = 8;

const char *kStageNames[kStages] = {
    "all", "pv_filter", "vz", "nVtx", "cluster_compat", "csc_halo", "zdc_xor", "hf_gap"};

struct TopologyBin {
  float absYMin;
  float absYMax;
  float ptMin;
  float ptMax;
  float alphaMax;
  float svpvSigMin;
  float dthetaMax;
};

const std::vector<TopologyBin> &Bins() {
  static const std::vector<TopologyBin> bins = {
      {0.0f, 1.0f, 2.0f, 5.0f, 0.40f, 2.5f, 0.50f},
      {0.0f, 1.0f, 5.0f, 8.0f, 0.35f, 3.5f, 0.30f},
      {0.0f, 1.0f, 8.0f, 12.0f, 0.40f, 3.5f, 0.30f},
      {1.0f, 2.0f, 2.0f, 5.0f, 0.20f, 2.5f, 0.30f},
      {1.0f, 2.0f, 5.0f, 8.0f, 0.25f, 3.5f, 0.30f},
      {1.0f, 2.0f, 8.0f, 12.0f, 0.25f, 3.5f, 0.30f},
  };
  return bins;
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

int FindBin(float pt, float y) {
  const float absY = std::fabs(y);
  const auto &bins = Bins();
  for (size_t i = 0; i < bins.size(); ++i) {
    const bool inPt = pt >= bins[i].ptMin && (pt < bins[i].ptMax || (bins[i].ptMax == 12.0f && pt <= 12.0f));
    const bool inY = absY >= bins[i].absYMin && (absY < bins[i].absYMax || (bins[i].absYMax == 2.0f && absY <= 2.0f));
    if (inPt && inY) return static_cast<int>(i);
  }
  return -1;
}

bool PassDNoMass(float dPt, float dY, float dAlpha, float dSvpvDistance, float dSvpvDisErr,
                 float dChi2Cl, float dDtheta, float trk1Pt, float trk2Pt,
                 float trk1PtErr, float trk2PtErr, float trk1Eta, float trk2Eta,
                 bool trk1HighPurity, bool trk2HighPurity) {
  if (!trk1HighPurity || !trk2HighPurity) return false;
  if (std::fabs(trk1Eta) >= 2.4f || std::fabs(trk2Eta) >= 2.4f) return false;
  if (trk1Pt <= 1.0f || trk2Pt <= 1.0f) return false;
  if (trk1Pt <= 0.0f || trk2Pt <= 0.0f) return false;
  if (trk1PtErr / trk1Pt >= 0.1f || trk2PtErr / trk2Pt >= 0.1f) return false;
  if (dPt <= 2.0f || dPt > 12.0f || std::fabs(dY) > 2.0f) return false;
  const int binIndex = FindBin(dPt, dY);
  if (binIndex < 0) return false;
  const TopologyBin &bin = Bins()[binIndex];
  if (dAlpha >= bin.alphaMax) return false;
  if (!(dSvpvDisErr > 0.0f && dSvpvDistance / dSvpvDisErr > bin.svpvSigMin)) return false;
  if (dChi2Cl <= 0.10f) return false;
  if (dDtheta >= bin.dthetaMax) return false;
  return true;
}

void SaveHist(TH1D &hist, const std::string &path) {
  TCanvas canvas("canvas", "", 900, 700);
  hist.SetLineWidth(2);
  hist.Draw("hist");
  canvas.SaveAs(path.c_str());
}
}

int diagnose_event_d_cut_correlation(const char *inputName, const char *outDir) {
  gSystem->mkdir(outDir, true);

  std::unique_ptr<TFile> input(TFile::Open(inputName, "READ"));
  if (!input || input->IsZombie()) {
    std::cerr << "Could not open " << inputName << std::endl;
    return 1;
  }

  auto *hiTree = static_cast<TTree *>(input->Get("hiEvtAnalyzer/HiTree"));
  auto *trackTree = static_cast<TTree *>(input->Get("PbPbTracks/trackTree"));
  auto *skimTree = static_cast<TTree *>(input->Get("skimanalysis/HltTree"));
  auto *metTree = static_cast<TTree *>(input->Get("l1MetFilterRecoTree/MetFilterRecoTree"));
  auto *zdcTree = static_cast<TTree *>(input->Get("zdcanalyzer/zdcrechit"));
  auto *pfTree = static_cast<TTree *>(input->Get("particleFlowAnalyser/pftree"));
  auto *dTree = static_cast<TTree *>(input->Get("Dfinder/ntDkpi"));
  if (!hiTree || !trackTree || !skimTree || !metTree || !zdcTree || !pfTree || !dTree) {
    std::cerr << "Missing required tree in " << inputName << std::endl;
    return 1;
  }

  TTreeReader hiReader(hiTree), trackReader(trackTree), skimReader(skimTree), metReader(metTree);
  TTreeReader zdcReader(zdcTree), pfReader(pfTree), dReader(dTree);

  TTreeReaderValue<unsigned int> run(hiReader, "run");
  TTreeReaderValue<unsigned int> lumi(hiReader, "lumi");
  TTreeReaderValue<unsigned long long> evt(hiReader, "evt");
  TTreeReaderValue<int> nVtx(trackReader, "nVtx");
  TTreeReaderValue<std::vector<float>> zVtx(trackReader, "zVtx");
  TTreeReaderValue<std::vector<float>> ptSumVtx(trackReader, "ptSumVtx");
  TTreeReaderValue<int> pvFilter(skimReader, "pprimaryVertexFilter");
  TTreeReaderValue<int> clusterFilter(skimReader, "pclusterCompatibilityFilter");
  TTreeReaderValue<Bool_t> cscFilter(metReader, "cscTightHalo2015Filter");
  TTreeReaderValue<float> zdcPlus(zdcReader, "sumPlus");
  TTreeReaderValue<float> zdcMinus(zdcReader, "sumMinus");
  TTreeReaderValue<std::vector<int>> pfId(pfReader, "pfId");
  TTreeReaderValue<std::vector<float>> pfEta(pfReader, "pfEta");
  TTreeReaderValue<std::vector<float>> pfE(pfReader, "pfE");

  TTreeReaderValue<int> dSize(dReader, "Dsize");
  TTreeReaderArray<float> dMass(dReader, "Dmass");
  TTreeReaderArray<float> dPt(dReader, "Dpt");
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

  Long64_t eventCounts[kStages] = {};
  Long64_t dNoMassCounts[kStages] = {};
  Long64_t dMassCounts[kStages] = {};
  Long64_t eventWithDNoMass[kStages] = {};
  Long64_t firstFailWithDNoMass[kStages] = {};
  std::string firstFailExample[kStages];

  TH1D hDNoMassAll("hDNoMassAll", "Final D cuts except mass, all events;M_{K#pi} (GeV);Candidates", 140, 1.70, 2.05);
  TH1D hDNoMassAfterOffline("hDNoMassAfterOffline", "Final D cuts except mass, after full Section 5;M_{K#pi} (GeV);Candidates", 140, 1.70, 2.05);
  TH1D hDNoMassBeforeZdc("hDNoMassBeforeZdc", "Final D cuts except mass, before ZDC/HF cuts;M_{K#pi} (GeV);Candidates", 140, 1.70, 2.05);

  Long64_t nEvents = 0;
  while (hiReader.Next()) {
    if (!trackReader.Next() || !skimReader.Next() || !metReader.Next() ||
        !zdcReader.Next() || !pfReader.Next() || !dReader.Next()) {
      std::cerr << "Friend tree entry mismatch" << std::endl;
      return 1;
    }
    ++nEvents;
    const float bestVz = BestVertexZ(*nVtx, &(*zVtx), &(*ptSumVtx));
    const bool zdcGammaN = (*zdcMinus > kZdcMinus1n && *zdcPlus < kZdcPlus1n);
    const bool zdcNgamma = (*zdcPlus > kZdcPlus1n && *zdcMinus < kZdcMinus1n);
    const float hfPlus = MaxHFEnergy(*pfId, *pfEta, *pfE, 3.0f, 5.2f);
    const float hfMinus = MaxHFEnergy(*pfId, *pfEta, *pfE, -5.2f, -3.0f);
    const bool passGap = (zdcGammaN && hfPlus < kHfPlusGap) || (zdcNgamma && hfMinus < kHfMinusGap);

    bool pass[kStages] = {};
    pass[0] = true;
    pass[1] = pass[0] && *pvFilter == 1;
    pass[2] = pass[1] && std::isfinite(bestVz) && std::fabs(bestVz) < 15.0f;
    pass[3] = pass[2] && *nVtx > 0 && *nVtx <= 3;
    pass[4] = pass[3] && *clusterFilter == 1;
    pass[5] = pass[4] && *cscFilter;
    pass[6] = pass[5] && (zdcGammaN || zdcNgamma);
    pass[7] = pass[6] && passGap;
    for (int stage = 0; stage < kStages; ++stage) {
      if (pass[stage]) ++eventCounts[stage];
    }

    bool hasDNoMass = false;
    const int size = std::min<int>(*dSize, dMass.GetSize());
    for (int i = 0; i < size; ++i) {
      const bool passNoMass = PassDNoMass(dPt[i], dY[i], dAlpha[i], dSvpvDistance[i], dSvpvDisErr[i],
                                          dChi2Cl[i], dDtheta[i], dTrk1Pt[i], dTrk2Pt[i],
                                          dTrk1PtErr[i], dTrk2PtErr[i], dTrk1Eta[i], dTrk2Eta[i],
                                          dTrk1HighPurity[i], dTrk2HighPurity[i]);
      if (!passNoMass) continue;
      hasDNoMass = true;
      hDNoMassAll.Fill(dMass[i]);
      if (pass[5]) hDNoMassBeforeZdc.Fill(dMass[i]);
      if (pass[7]) hDNoMassAfterOffline.Fill(dMass[i]);
      const bool passMass = std::fabs(dMass[i] - kD0Mass) < kMassWindow;
      for (int stage = 0; stage < kStages; ++stage) {
        if (pass[stage]) {
          ++dNoMassCounts[stage];
          if (passMass) ++dMassCounts[stage];
        }
      }
    }
    if (hasDNoMass) {
      for (int stage = 0; stage < kStages; ++stage) {
        if (pass[stage]) ++eventWithDNoMass[stage];
      }
      for (int stage = 1; stage < kStages; ++stage) {
        if (!pass[stage] && pass[stage - 1]) {
          ++firstFailWithDNoMass[stage];
          if (firstFailExample[stage].empty()) {
            firstFailExample[stage] = std::to_string(*run) + ":" + std::to_string(*lumi) + ":" +
                                      std::to_string(*evt) + ",zdcPlus=" + std::to_string(*zdcPlus) +
                                      ",zdcMinus=" + std::to_string(*zdcMinus) +
                                      ",hfPlus=" + std::to_string(hfPlus) +
                                      ",hfMinus=" + std::to_string(hfMinus);
          }
          break;
        }
      }
    }
  }

  const std::string base(outDir);
  {
    std::ofstream csv(base + "/event_d_cut_correlation.csv");
    csv << "stage,stage_name,events_passing,event_with_d_final_no_mass,d_final_no_mass_candidates,d_final_with_mass_candidates\n";
    for (int stage = 0; stage < kStages; ++stage) {
      csv << stage << "," << kStageNames[stage] << "," << eventCounts[stage] << ","
          << eventWithDNoMass[stage] << "," << dNoMassCounts[stage] << "," << dMassCounts[stage] << "\n";
    }
  }
  {
    std::ofstream csv(base + "/d_candidate_event_failure_examples.csv");
    csv << "first_failed_stage,stage_name,events_with_d_final_no_mass_first_failing_here,example\n";
    for (int stage = 1; stage < kStages; ++stage) {
      csv << stage << "," << kStageNames[stage] << "," << firstFailWithDNoMass[stage]
          << ",\"" << firstFailExample[stage] << "\"\n";
    }
  }
  SaveHist(hDNoMassAll, base + "/d_final_no_mass_all_events.png");
  SaveHist(hDNoMassBeforeZdc, base + "/d_final_no_mass_before_zdc_hf.png");
  SaveHist(hDNoMassAfterOffline, base + "/d_final_no_mass_after_full_event_selection.png");

  std::cout << "diagnosed_events=" << nEvents << "\n";
  std::cout << "output_dir=" << outDir << "\n";
  return 0;
}
