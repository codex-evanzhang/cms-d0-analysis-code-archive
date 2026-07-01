#include <TCanvas.h>
#include <TChain.h>
#include <TFile.h>
#include <TH1D.h>
#include <TParameter.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr float kD0PdgMass = 1.86484f;
constexpr float kD0MassWindow = 0.2f;
constexpr float kZdcPlus1nThreshold = 1100.0f;
constexpr float kZdcMinus1nThreshold = 1000.0f;
constexpr float kHfPlusGapThreshold = 9.2f;
constexpr float kHfMinusGapThreshold = 8.6f;

struct EventKey {
  UInt_t run = 0;
  UInt_t lumi = 0;
  ULong64_t event = 0;

  bool operator==(const EventKey& other) const {
    return run == other.run && lumi == other.lumi && event == other.event;
  }
};

struct EventKeyHash {
  std::size_t operator()(const EventKey& key) const {
    std::size_t h = std::hash<ULong64_t>{}(key.event);
    h ^= static_cast<std::size_t>(key.run) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    h ^= static_cast<std::size_t>(key.lumi) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    return h;
  }
};

struct HltBits {
  int zdcOrMin400Max10000 = 0;
  int zdcOrMax400Pixel = 0;
  int zdcOrMax10000 = 0;
  int zdcXorJet8 = 0;
  int zdcXorJet12 = 0;
  int zdcXorJet16 = 0;
  int zeroBiasMin400Max10000 = 0;
  int zeroBiasMax400Pixel = 0;
  int zeroBiasMax10000 = 0;
};

struct Vertex {
  float x = 9999.0f;
  float y = 9999.0f;
  float z = 9999.0f;
  float xErr = 9999.0f;
  float yErr = 9999.0f;
  float zErr = 9999.0f;
};

struct DCutBin {
  float absYMin;
  float absYMax;
  float ptMin;
  float ptMax;
  float alphaMax;
  float svpvSigMin;
  float vertexProbMin;
  float dthetaMax;
};

const std::vector<DCutBin>& CutBins() {
  static const std::vector<DCutBin> bins = {
      {0.0f, 1.0f, 2.0f, 5.0f, 0.4f, 2.5f, 0.1f, 0.5f},
      {0.0f, 1.0f, 5.0f, 8.0f, 0.35f, 3.5f, 0.1f, 0.3f},
      {0.0f, 1.0f, 8.0f, 12.0f, 0.4f, 3.5f, 0.1f, 0.3f},
      {1.0f, 2.0f, 2.0f, 5.0f, 0.2f, 2.5f, 0.1f, 0.3f},
      {1.0f, 2.0f, 5.0f, 8.0f, 0.25f, 3.5f, 0.1f, 0.3f},
      {1.0f, 2.0f, 8.0f, 12.0f, 0.25f, 3.5f, 0.1f, 0.3f},
  };
  return bins;
}

bool IsOwnedWritePath(const TString& path) {
  return path.BeginsWith("/afs/cern.ch/user/e/evzhang") ||
         path.BeginsWith("/afs/cern.ch/work/e/evzhang") ||
         path.BeginsWith("/eos/user/e/evzhang");
}

bool ReadInputList(const char* inputList, std::vector<std::string>& files) {
  std::ifstream input(inputList);
  if (!input.is_open()) {
    std::cerr << "ERROR: could not open input list " << inputList << "\n";
    return false;
  }
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    files.push_back(line);
  }
  if (files.empty()) {
    std::cerr << "ERROR: input list has no files " << inputList << "\n";
    return false;
  }
  return true;
}

Vertex BestVertex(int nVtx,
                  const std::vector<float>& xVtx,
                  const std::vector<float>& yVtx,
                  const std::vector<float>& zVtx,
                  const std::vector<float>& xErrVtx,
                  const std::vector<float>& yErrVtx,
                  const std::vector<float>& zErrVtx,
                  const std::vector<float>& ptSumVtx) {
  Vertex out;
  if (nVtx <= 0 || zVtx.empty()) return out;

  int best = 0;
  for (int i = 1; i < nVtx && i < static_cast<int>(ptSumVtx.size()); ++i) {
    if (ptSumVtx[i] > ptSumVtx[best]) best = i;
  }
  if (best >= static_cast<int>(zVtx.size())) best = 0;

  auto get = [best](const std::vector<float>& values) {
    return (best < static_cast<int>(values.size())) ? values[best] : 9999.0f;
  };
  out.x = get(xVtx);
  out.y = get(yVtx);
  out.z = get(zVtx);
  out.xErr = get(xErrVtx);
  out.yErr = get(yErrVtx);
  out.zErr = get(zErrVtx);
  return out;
}

float MaxHFEnergy(const std::vector<int>& pfId,
                  const std::vector<float>& pfEta,
                  const std::vector<float>& pfE,
                  float etaMin,
                  float etaMax) {
  float maxE = 0.0f;
  const size_t n = std::min({pfId.size(), pfEta.size(), pfE.size()});
  for (size_t i = 0; i < n; ++i) {
    if ((pfId[i] == 6 || pfId[i] == 7) && pfEta[i] > etaMin && pfEta[i] < etaMax) {
      maxE = std::max(maxE, pfE[i]);
    }
  }
  return maxE;
}

int FindCutBin(float pt, float y) {
  const float absY = std::fabs(y);
  const auto& bins = CutBins();
  for (size_t i = 0; i < bins.size(); ++i) {
    const bool inPt = (pt >= bins[i].ptMin) &&
                      (pt < bins[i].ptMax || (bins[i].ptMax == 12.0f && pt <= bins[i].ptMax));
    const bool inY = (absY >= bins[i].absYMin) &&
                     (absY < bins[i].absYMax || (bins[i].absYMax == 2.0f && absY <= bins[i].absYMax));
    if (inPt && inY) return static_cast<int>(i);
  }
  return -1;
}

bool PassCut23PAS(float mass,
                  float pt,
                  float y,
                  float chi2cl,
                  float alpha,
                  float dtheta,
                  float svpvDistance,
                  float svpvDisErr,
                  float trk1Pt,
                  float trk2Pt,
                  float trk1PtErr,
                  float trk2PtErr,
                  float trk1Eta,
                  float trk2Eta,
                  bool trk1HighPurity,
                  bool trk2HighPurity) {
  if (std::fabs(mass - kD0PdgMass) >= kD0MassWindow) return false;
  if (!trk1HighPurity || !trk2HighPurity) return false;
  if (std::fabs(trk1Eta) >= 2.4f || std::fabs(trk2Eta) >= 2.4f) return false;
  if (trk1Pt <= 1.0f || trk2Pt <= 1.0f) return false;
  if (trk1Pt <= 0.0f || trk2Pt <= 0.0f) return false;
  if (trk1PtErr / trk1Pt >= 0.1f || trk2PtErr / trk2Pt >= 0.1f) return false;

  const int cutBin = FindCutBin(pt, y);
  if (cutBin < 0) return false;
  const auto& bin = CutBins()[cutBin];
  const float svpvSig = (svpvDisErr > 0.0f) ? svpvDistance / svpvDisErr : -999.0f;
  return alpha < bin.alphaMax && svpvSig > bin.svpvSigMin &&
         chi2cl > bin.vertexProbMin && dtheta < bin.dthetaMax;
}

void SaveHistogram(TH1D& hist, const TString& path) {
  TCanvas canvas("canvas", "", 900, 700);
  hist.SetLineWidth(2);
  hist.Draw("hist");
  canvas.SaveAs(path);
}

std::unordered_map<EventKey, HltBits, EventKeyHash> BuildHltMap(TChain& hlt,
                                                                 Long64_t& rowsRead,
                                                                 Long64_t& duplicateRows) {
  std::unordered_map<EventKey, HltBits, EventKeyHash> out;
  rowsRead = 0;
  duplicateRows = 0;
  if (hlt.GetEntries() <= 0) return out;

  TTreeReader reader(&hlt);
  TTreeReaderValue<Int_t> run(reader, "Run");
  TTreeReaderValue<Int_t> lumi(reader, "LumiBlock");
  TTreeReaderValue<ULong64_t> event(reader, "Event");

  TTreeReaderValue<Int_t> zdcOrMin400Max10000(reader, "HLT_HIUPC_ZDC1nOR_MinPixelCluster400_MaxPixelCluster10000_v16");
  TTreeReaderValue<Int_t> zdcOrMax400Pixel(reader, "HLT_HIUPC_ZDC1nOR_SinglePixelTrackLowPt_MaxPixelCluster400_v15");
  TTreeReaderValue<Int_t> zdcOrMax10000(reader, "HLT_HIUPC_ZDC1nOR_SinglePixelTrack_MaxPixelTrack_v16");
  TTreeReaderValue<Int_t> zdcXorJet8(reader, "HLT_HIUPC_SingleJet8_ZDC1nXOR_MaxPixelCluster10000_v4");
  TTreeReaderValue<Int_t> zdcXorJet12(reader, "HLT_HIUPC_SingleJet12_ZDC1nXOR_MaxPixelCluster10000_v4");
  TTreeReaderValue<Int_t> zdcXorJet16(reader, "HLT_HIUPC_SingleJet16_ZDC1nXOR_MaxPixelCluster10000_v4");
  TTreeReaderValue<Int_t> zeroBiasMin400Max10000(reader, "HLT_HIUPC_ZeroBias_MinPixelCluster400_MaxPixelCluster10000_v16");
  TTreeReaderValue<Int_t> zeroBiasMax400Pixel(reader, "HLT_HIUPC_ZeroBias_SinglePixelTrackLowPt_MaxPixelCluster400_v15");
  TTreeReaderValue<Int_t> zeroBiasMax10000(reader, "HLT_HIUPC_ZeroBias_SinglePixelTrack_MaxPixelTrack_v16");

  while (reader.Next()) {
    ++rowsRead;
    EventKey key{static_cast<UInt_t>(*run), static_cast<UInt_t>(*lumi), *event};
    HltBits bits;
    bits.zdcOrMin400Max10000 = (*zdcOrMin400Max10000 != 0) ? 1 : 0;
    bits.zdcOrMax400Pixel = (*zdcOrMax400Pixel != 0) ? 1 : 0;
    bits.zdcOrMax10000 = (*zdcOrMax10000 != 0) ? 1 : 0;
    bits.zdcXorJet8 = (*zdcXorJet8 != 0) ? 1 : 0;
    bits.zdcXorJet12 = (*zdcXorJet12 != 0) ? 1 : 0;
    bits.zdcXorJet16 = (*zdcXorJet16 != 0) ? 1 : 0;
    bits.zeroBiasMin400Max10000 = (*zeroBiasMin400Max10000 != 0) ? 1 : 0;
    bits.zeroBiasMax400Pixel = (*zeroBiasMax400Pixel != 0) ? 1 : 0;
    bits.zeroBiasMax10000 = (*zeroBiasMax10000 != 0) ? 1 : 0;

    auto it = out.find(key);
    if (it == out.end()) {
      out.emplace(key, bits);
    } else {
      ++duplicateRows;
      it->second.zdcOrMin400Max10000 |= bits.zdcOrMin400Max10000;
      it->second.zdcOrMax400Pixel |= bits.zdcOrMax400Pixel;
      it->second.zdcOrMax10000 |= bits.zdcOrMax10000;
      it->second.zdcXorJet8 |= bits.zdcXorJet8;
      it->second.zdcXorJet12 |= bits.zdcXorJet12;
      it->second.zdcXorJet16 |= bits.zdcXorJet16;
      it->second.zeroBiasMin400Max10000 |= bits.zeroBiasMin400Max10000;
      it->second.zeroBiasMax400Pixel |= bits.zeroBiasMax400Pixel;
      it->second.zeroBiasMax10000 |= bits.zeroBiasMax10000;
    }
  }
  return out;
}

}  // namespace

int recreate_2023_d0_micro_skim(const char* inputList,
                                const char* outputPath,
                                const char* plotDir,
                                Long64_t maxEvents = -1,
                                int maxFiles = -1,
                                int computeHF = 0) {
  if (!IsOwnedWritePath(TString(outputPath)) || !IsOwnedWritePath(TString(plotDir))) {
    std::cerr << "ERROR: refusing to write outside Evan-owned CERN paths\n";
    return 1;
  }

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
  TChain hlt("hltanalysis/HltTree");

  for (const auto& file : files) {
    hi.Add(file.c_str());
    track.Add(file.c_str());
    skim.Add(file.c_str());
    met.Add(file.c_str());
    zdc.Add(file.c_str());
    pf.Add(file.c_str());
    d.Add(file.c_str());
    hlt.Add(file.c_str());
  }

  const Long64_t entries = hi.GetEntries();
  if (entries <= 0 || track.GetEntries() != entries || skim.GetEntries() != entries ||
      met.GetEntries() != entries || zdc.GetEntries() != entries ||
      d.GetEntries() != entries || (computeHF && pf.GetEntries() != entries)) {
    std::cerr << "ERROR: aligned forest trees are empty or have mismatched entry counts\n";
    std::cerr << "hi=" << entries << " track=" << track.GetEntries()
              << " skim=" << skim.GetEntries() << " met=" << met.GetEntries()
              << " zdc=" << zdc.GetEntries() << " pf=" << pf.GetEntries()
              << " d=" << d.GetEntries() << " hlt=" << hlt.GetEntries() << "\n";
    return 1;
  }

  std::cout << "Building HLT lookup from " << hlt.GetEntries() << " rows\n";
  Long64_t hltRowsRead = 0;
  Long64_t hltDuplicateRows = 0;
  auto hltMap = BuildHltMap(hlt, hltRowsRead, hltDuplicateRows);
  std::cout << "Built HLT lookup: rows=" << hltRowsRead
            << " unique=" << hltMap.size()
            << " duplicates=" << hltDuplicateRows << "\n";

  gSystem->mkdir(gSystem->DirName(outputPath), true);
  gSystem->mkdir(plotDir, true);
  gStyle->SetOptStat(1110);

  TTreeReader hiReader(&hi);
  TTreeReader trackReader(&track);
  TTreeReader skimReader(&skim);
  TTreeReader metReader(&met);
  TTreeReader zdcReader(&zdc);
  TTreeReader pfReader(&pf);
  TTreeReader dReader(&d);

  TTreeReaderValue<UInt_t> run(hiReader, "run");
  TTreeReaderValue<UInt_t> lumi(hiReader, "lumi");
  TTreeReaderValue<ULong64_t> evt(hiReader, "evt");
  TTreeReaderValue<Float_t> vx(hiReader, "vx");
  TTreeReaderValue<Float_t> vy(hiReader, "vy");
  TTreeReaderValue<Float_t> vz(hiReader, "vz");
  TTreeReaderValue<Int_t> clusCompNPixHits(hiReader, "clusComp_nPixHits");

  TTreeReaderValue<Int_t> nVtx(trackReader, "nVtx");
  TTreeReaderValue<std::vector<float>> xVtx(trackReader, "xVtx");
  TTreeReaderValue<std::vector<float>> yVtx(trackReader, "yVtx");
  TTreeReaderValue<std::vector<float>> zVtx(trackReader, "zVtx");
  TTreeReaderValue<std::vector<float>> xErrVtx(trackReader, "xErrVtx");
  TTreeReaderValue<std::vector<float>> yErrVtx(trackReader, "yErrVtx");
  TTreeReaderValue<std::vector<float>> zErrVtx(trackReader, "zErrVtx");
  TTreeReaderValue<std::vector<float>> ptSumVtx(trackReader, "ptSumVtx");

  TTreeReaderValue<Int_t> pprimaryVertexFilter(skimReader, "pprimaryVertexFilter");
  TTreeReaderValue<Int_t> pclusterCompatibilityFilter(skimReader, "pclusterCompatibilityFilter");
  TTreeReaderValue<Int_t> pzdcEnergyFilter0nOr(skimReader, "pzdcEnergyFilter0nOr");
  TTreeReaderValue<Int_t> pzdcEnergyFilter1nOr(skimReader, "pzdcEnergyFilter1nOr");
  TTreeReaderValue<Int_t> pzdcEnergyFilterXOr(skimReader, "pzdcEnergyFilterXOr");
  TTreeReaderValue<Bool_t> cscTightHalo2015Filter(metReader, "cscTightHalo2015Filter");
  TTreeReaderValue<Float_t> zdcSumPlus(zdcReader, "sumPlus");
  TTreeReaderValue<Float_t> zdcSumMinus(zdcReader, "sumMinus");

  TTreeReaderValue<std::vector<int>> pfId(pfReader, "pfId");
  TTreeReaderValue<std::vector<float>> pfEta(pfReader, "pfEta");
  TTreeReaderValue<std::vector<float>> pfE(pfReader, "pfE");

  TTreeReaderValue<Int_t> dSize(dReader, "Dsize");
  TTreeReaderArray<Int_t> dType(dReader, "Dtype");
  TTreeReaderArray<Float_t> dMass(dReader, "Dmass");
  TTreeReaderArray<Float_t> dPt(dReader, "Dpt");
  TTreeReaderArray<Float_t> dEta(dReader, "Deta");
  TTreeReaderArray<Float_t> dPhi(dReader, "Dphi");
  TTreeReaderArray<Float_t> dY(dReader, "Dy");
  TTreeReaderArray<Float_t> dChi2Cl(dReader, "Dchi2cl");
  TTreeReaderArray<Float_t> dDtheta(dReader, "Ddtheta");
  TTreeReaderArray<Float_t> dAlpha(dReader, "Dalpha");
  TTreeReaderArray<Float_t> dSvpvDistance(dReader, "DsvpvDistance");
  TTreeReaderArray<Float_t> dSvpvDisErr(dReader, "DsvpvDisErr");
  TTreeReaderArray<Float_t> dSvpvDistance2D(dReader, "DsvpvDistance_2D");
  TTreeReaderArray<Float_t> dSvpvDisErr2D(dReader, "DsvpvDisErr_2D");
  TTreeReaderArray<Float_t> dIp3D(dReader, "Dip3D.Dip3d");
  TTreeReaderArray<Float_t> dIp3DErr(dReader, "Dip3derr");
  TTreeReaderArray<Float_t> dTrk1Pt(dReader, "Dtrk1Pt");
  TTreeReaderArray<Float_t> dTrk2Pt(dReader, "Dtrk2Pt");
  TTreeReaderArray<Float_t> dTrk1PtErr(dReader, "Dtrk1PtErr");
  TTreeReaderArray<Float_t> dTrk2PtErr(dReader, "Dtrk2PtErr");
  TTreeReaderArray<Float_t> dTrk1Eta(dReader, "Dtrk1Eta");
  TTreeReaderArray<Float_t> dTrk2Eta(dReader, "Dtrk2Eta");
  TTreeReaderArray<Float_t> dTrk1MassHypo(dReader, "Dtrk1MassHypo");
  TTreeReaderArray<Float_t> dTrk2MassHypo(dReader, "Dtrk2MassHypo");
  TTreeReaderArray<Float_t> dTrk1PixelHit(dReader, "Dtrk1PixelHit");
  TTreeReaderArray<Float_t> dTrk2PixelHit(dReader, "Dtrk2PixelHit");
  TTreeReaderArray<Float_t> dTrk1StripHit(dReader, "Dtrk1StripHit");
  TTreeReaderArray<Float_t> dTrk2StripHit(dReader, "Dtrk2StripHit");
  TTreeReaderArray<Float_t> dTrk1P(dReader, "Dtrk1P");
  TTreeReaderArray<Float_t> dTrk2P(dReader, "Dtrk2P");
  TTreeReaderArray<Bool_t> dTrk1HighPurity(dReader, "Dtrk1highPurity");
  TTreeReaderArray<Bool_t> dTrk2HighPurity(dReader, "Dtrk2highPurity");

  TFile output(outputPath, "RECREATE");
  TTree tree("Tree", "Tree for UPC Dzero analysis (Codex recreated first-pass subset)");

  UInt_t oRun = 0;
  ULong64_t oEvent = 0;
  UInt_t oLumi = 0;
  Int_t oProcessID = 0;
  Int_t oClusCompNPixHits = 0;
  Float_t oClusCompQuality = -999.0f;
  Float_t oVX = 0.0f, oVY = 0.0f, oVZ = 0.0f;
  Float_t oVXError = 0.0f, oVYError = 0.0f, oVZError = 0.0f;
  Int_t oNVtx = 0;
  Int_t oIsL1ZDCOr = 0;
  Int_t oIsL1ZDCOrMin400Max10000 = 0;
  Int_t oIsL1ZDCOrMax400Pixel = 0;
  Int_t oIsL1ZDCOrMax10000 = 0;
  Int_t oIsL1ZDCXORJet8 = 0;
  Int_t oIsL1ZDCXORJet12 = 0;
  Int_t oIsL1ZDCXORJet16 = 0;
  Int_t oIsZeroBias = 0;
  Int_t oIsZeroBiasMin400Max10000 = 0;
  Int_t oIsZeroBiasMax400Pixel = 0;
  Int_t oIsZeroBiasMax10000 = 0;
  Int_t oSelectedBkgFilter = 0;
  Int_t oSelectedVtxFilter = 0;
  Int_t oClusterCompatibilityFilter = 0;
  Int_t oCscTightHalo2015Filter = 0;
  Int_t oPprimaryVertexFilter = 0;
  Int_t oPzdcEnergyFilter0nOr = 0;
  Int_t oPzdcEnergyFilter1nOr = 0;
  Int_t oPzdcEnergyFilterXOr = 0;
  Int_t oZDCgammaN = 0;
  Int_t oZDCNgamma = 0;
  Int_t oGapgammaN = 0;
  Int_t oGapNgamma = 0;
  Int_t oGammaN = 0;
  Int_t oNgamma = 0;
  Float_t oZDCsumPlus = 0.0f;
  Float_t oZDCsumMinus = 0.0f;
  Float_t oHFEMaxPlus = 0.0f;
  Float_t oHFEMaxMinus = 0.0f;
  Int_t oNTrackInAcceptanceHP = -1;
  Int_t oDsize = 0;

  std::vector<int> vDtype;
  std::vector<float> vDpt, vDeta, vDphi, vDy, vDmass;
  std::vector<float> vDchi2cl, vDdtheta, vDalpha;
  std::vector<float> vDsvpvDistance, vDsvpvDisErr, vDsvpvDistance2D, vDsvpvDisErr2D;
  std::vector<float> vDip3D, vDip3derr;
  std::vector<float> vDtrk1Pt, vDtrk2Pt, vDtrk1PtErr, vDtrk2PtErr, vDtrk1Eta, vDtrk2Eta;
  std::vector<float> vDtrk1MassHypo, vDtrk2MassHypo, vDtrk1P, vDtrk2P;
  std::vector<float> vDtrk1PixelHit, vDtrk2PixelHit, vDtrk1StripHit, vDtrk2StripHit;
  std::vector<int> vDtrk1highPurity, vDtrk2highPurity;
  std::vector<int> vDpassCut23PAS, vDpassCutNominal;

  tree.Branch("Run", &oRun, "Run/i");
  tree.Branch("Event", &oEvent, "Event/l");
  tree.Branch("Lumi", &oLumi, "Lumi/i");
  tree.Branch("ProcessID", &oProcessID, "ProcessID/I");
  tree.Branch("clusComp_nPixHits", &oClusCompNPixHits, "clusComp_nPixHits/I");
  tree.Branch("clusComp_quality", &oClusCompQuality, "clusComp_quality/F");
  tree.Branch("VX", &oVX, "VX/F");
  tree.Branch("VY", &oVY, "VY/F");
  tree.Branch("VZ", &oVZ, "VZ/F");
  tree.Branch("VXError", &oVXError, "VXError/F");
  tree.Branch("VYError", &oVYError, "VYError/F");
  tree.Branch("VZError", &oVZError, "VZError/F");
  tree.Branch("nVtx", &oNVtx, "nVtx/I");
  tree.Branch("isL1ZDCOr", &oIsL1ZDCOr, "isL1ZDCOr/I");
  tree.Branch("isL1ZDCOr_Min400_Max10000", &oIsL1ZDCOrMin400Max10000, "isL1ZDCOr_Min400_Max10000/I");
  tree.Branch("isL1ZDCOr_Max400_Pixel", &oIsL1ZDCOrMax400Pixel, "isL1ZDCOr_Max400_Pixel/I");
  tree.Branch("isL1ZDCOr_Max10000", &oIsL1ZDCOrMax10000, "isL1ZDCOr_Max10000/I");
  tree.Branch("isL1ZDCXORJet8", &oIsL1ZDCXORJet8, "isL1ZDCXORJet8/I");
  tree.Branch("isL1ZDCXORJet12", &oIsL1ZDCXORJet12, "isL1ZDCXORJet12/I");
  tree.Branch("isL1ZDCXORJet16", &oIsL1ZDCXORJet16, "isL1ZDCXORJet16/I");
  tree.Branch("isZeroBias", &oIsZeroBias, "isZeroBias/I");
  tree.Branch("isZeroBias_Min400_Max10000", &oIsZeroBiasMin400Max10000, "isZeroBias_Min400_Max10000/I");
  tree.Branch("isZeroBias_Max400_Pixel", &oIsZeroBiasMax400Pixel, "isZeroBias_Max400_Pixel/I");
  tree.Branch("isZeroBias_Max10000", &oIsZeroBiasMax10000, "isZeroBias_Max10000/I");
  tree.Branch("selectedBkgFilter", &oSelectedBkgFilter, "selectedBkgFilter/I");
  tree.Branch("selectedVtxFilter", &oSelectedVtxFilter, "selectedVtxFilter/I");
  tree.Branch("ClusterCompatibilityFilter", &oClusterCompatibilityFilter, "ClusterCompatibilityFilter/I");
  tree.Branch("cscTightHalo2015Filter", &oCscTightHalo2015Filter, "cscTightHalo2015Filter/I");
  tree.Branch("pprimaryVertexFilter", &oPprimaryVertexFilter, "pprimaryVertexFilter/I");
  tree.Branch("pzdcEnergyFilter0nOr", &oPzdcEnergyFilter0nOr, "pzdcEnergyFilter0nOr/I");
  tree.Branch("pzdcEnergyFilter1nOr", &oPzdcEnergyFilter1nOr, "pzdcEnergyFilter1nOr/I");
  tree.Branch("pzdcEnergyFilterXOr", &oPzdcEnergyFilterXOr, "pzdcEnergyFilterXOr/I");
  tree.Branch("ZDCgammaN", &oZDCgammaN, "ZDCgammaN/I");
  tree.Branch("ZDCNgamma", &oZDCNgamma, "ZDCNgamma/I");
  tree.Branch("gapgammaN", &oGapgammaN, "gapgammaN/I");
  tree.Branch("gapNgamma", &oGapNgamma, "gapNgamma/I");
  tree.Branch("gammaN", &oGammaN, "gammaN/I");
  tree.Branch("Ngamma", &oNgamma, "Ngamma/I");
  tree.Branch("ZDCsumPlus", &oZDCsumPlus, "ZDCsumPlus/F");
  tree.Branch("ZDCsumMinus", &oZDCsumMinus, "ZDCsumMinus/F");
  tree.Branch("HFEMaxPlus", &oHFEMaxPlus, "HFEMaxPlus/F");
  tree.Branch("HFEMaxMinus", &oHFEMaxMinus, "HFEMaxMinus/F");
  tree.Branch("nTrackInAcceptanceHP", &oNTrackInAcceptanceHP, "nTrackInAcceptanceHP/I");
  tree.Branch("Dsize", &oDsize, "Dsize/I");
  tree.Branch("Dtype", &vDtype);
  tree.Branch("Dpt", &vDpt);
  tree.Branch("Deta", &vDeta);
  tree.Branch("Dphi", &vDphi);
  tree.Branch("Dy", &vDy);
  tree.Branch("Dmass", &vDmass);
  tree.Branch("Dchi2cl", &vDchi2cl);
  tree.Branch("Ddtheta", &vDdtheta);
  tree.Branch("Dalpha", &vDalpha);
  tree.Branch("DsvpvDistance", &vDsvpvDistance);
  tree.Branch("DsvpvDisErr", &vDsvpvDisErr);
  tree.Branch("DsvpvDistance_2D", &vDsvpvDistance2D);
  tree.Branch("DsvpvDisErr_2D", &vDsvpvDisErr2D);
  tree.Branch("Dip3D", &vDip3D);
  tree.Branch("Dip3derr", &vDip3derr);
  tree.Branch("Dtrk1Pt", &vDtrk1Pt);
  tree.Branch("Dtrk2Pt", &vDtrk2Pt);
  tree.Branch("Dtrk1PtErr", &vDtrk1PtErr);
  tree.Branch("Dtrk2PtErr", &vDtrk2PtErr);
  tree.Branch("Dtrk1Eta", &vDtrk1Eta);
  tree.Branch("Dtrk2Eta", &vDtrk2Eta);
  tree.Branch("Dtrk1MassHypo", &vDtrk1MassHypo);
  tree.Branch("Dtrk2MassHypo", &vDtrk2MassHypo);
  tree.Branch("Dtrk1PixelHit", &vDtrk1PixelHit);
  tree.Branch("Dtrk2PixelHit", &vDtrk2PixelHit);
  tree.Branch("Dtrk1StripHit", &vDtrk1StripHit);
  tree.Branch("Dtrk2StripHit", &vDtrk2StripHit);
  tree.Branch("Dtrk1P", &vDtrk1P);
  tree.Branch("Dtrk2P", &vDtrk2P);
  tree.Branch("Dtrk1highPurity", &vDtrk1highPurity);
  tree.Branch("Dtrk2highPurity", &vDtrk2highPurity);
  tree.Branch("DpassCut23PAS", &vDpassCut23PAS);
  tree.Branch("DpassCutNominal", &vDpassCutNominal);

  TH1D hDsize("hDsize", "Dfinder candidate multiplicity;Dsize;Events", 120, 0, 120);
  TH1D hMassAll("hDmass_all_candidates", "All copied Dfinder candidates;m(K#pi) [GeV];Candidates", 160, 1.5, 2.3);
  TH1D hMassPass("hDmass_passCut23PAS", "Candidates passing recreated DpassCut23PAS;m(K#pi) [GeV];Candidates", 160, 1.5, 2.3);
  TH1D hEventFlags("hEventFlagCounts", "Event flags;Flag;Events", 9, 0, 9);
  const char* flagLabels[] = {"HLT-map", "L1ZDCOr", "PV", "|vz|<15", "nVtx<=3", "Bkg", "ZDC XOR", "Gap", "gammaN/Ngamma"};
  for (int i = 1; i <= 9; ++i) hEventFlags.GetXaxis()->SetBinLabel(i, flagLabels[i - 1]);

  Long64_t nEvents = 0;
  Long64_t nHltMatched = 0;
  Long64_t nHltZdcOr = 0;
  Long64_t nInputCandidates = 0;
  Long64_t nCopiedCandidates = 0;
  Long64_t nPassCut23 = 0;
  Long64_t nDsizeTruncatedByReader = 0;

  std::cout << "Starting event loop: entries=" << entries
            << " maxEvents=" << maxEvents
            << " computeHF=" << computeHF << "\n";
  while (hiReader.Next()) {
    if (maxEvents >= 0 && nEvents >= maxEvents) break;
    if (!trackReader.Next() || !skimReader.Next() || !metReader.Next() ||
        !zdcReader.Next() || !dReader.Next() || (computeHF && !pfReader.Next())) {
      std::cerr << "ERROR: aligned tree ended early at event " << nEvents << "\n";
      return 1;
    }
    ++nEvents;
    if (nEvents == 1 || nEvents % 100000 == 0) {
      std::cout << "Processed " << nEvents << " / " << entries << " events\n";
    }

    const EventKey key{*run, *lumi, *evt};
    const auto hltIt = hltMap.find(key);
    const HltBits bits = (hltIt != hltMap.end()) ? hltIt->second : HltBits{};
    if (hltIt != hltMap.end()) {
      ++nHltMatched;
      hEventFlags.Fill(0.5);
    }

    const Vertex bestVertex = BestVertex(*nVtx, *xVtx, *yVtx, *zVtx, *xErrVtx, *yErrVtx, *zErrVtx, *ptSumVtx);
    const float bestVz = std::isfinite(bestVertex.z) && bestVertex.z != 9999.0f ? bestVertex.z : *vz;
    const bool passPv = (*pprimaryVertexFilter == 1);
    const bool passVz = std::isfinite(bestVz) && std::fabs(bestVz) < 15.0f;
    const bool passNVtx = (*nVtx > 0 && *nVtx <= 3);
    const bool passBkg = (*pclusterCompatibilityFilter == 1) && (*cscTightHalo2015Filter);
    const bool zdcGammaN = (*zdcSumMinus > kZdcMinus1nThreshold && *zdcSumPlus < kZdcPlus1nThreshold);
    const bool zdcNgamma = (*zdcSumPlus > kZdcPlus1nThreshold && *zdcSumMinus < kZdcMinus1nThreshold);
    const bool passZdcXor = zdcGammaN || zdcNgamma;
    const float hfPlus = computeHF ? MaxHFEnergy(*pfId, *pfEta, *pfE, 3.0f, 5.2f) : -1.0f;
    const float hfMinus = computeHF ? MaxHFEnergy(*pfId, *pfEta, *pfE, -5.2f, -3.0f) : -1.0f;
    const bool gapGammaN = computeHF && zdcGammaN && hfPlus < kHfPlusGapThreshold;
    const bool gapNgamma = computeHF && zdcNgamma && hfMinus < kHfMinusGapThreshold;
    const bool passGap = gapGammaN || gapNgamma;

    oRun = *run;
    oEvent = *evt;
    oLumi = *lumi;
    oProcessID = 0;
    oClusCompNPixHits = *clusCompNPixHits;
    oClusCompQuality = -999.0f;  // TODO: derive from clusComp arrays if the AN uses it numerically.
    oVX = bestVertex.x != 9999.0f ? bestVertex.x : *vx;
    oVY = bestVertex.y != 9999.0f ? bestVertex.y : *vy;
    oVZ = bestVz;
    oVXError = bestVertex.xErr;
    oVYError = bestVertex.yErr;
    oVZError = bestVertex.zErr;
    oNVtx = *nVtx;
    oIsL1ZDCOrMin400Max10000 = bits.zdcOrMin400Max10000;
    oIsL1ZDCOrMax400Pixel = bits.zdcOrMax400Pixel;
    oIsL1ZDCOrMax10000 = bits.zdcOrMax10000;
    oIsL1ZDCOr = bits.zdcOrMin400Max10000 || bits.zdcOrMax400Pixel || bits.zdcOrMax10000;
    oIsL1ZDCXORJet8 = bits.zdcXorJet8;
    oIsL1ZDCXORJet12 = bits.zdcXorJet12;
    oIsL1ZDCXORJet16 = bits.zdcXorJet16;
    oIsZeroBiasMin400Max10000 = bits.zeroBiasMin400Max10000;
    oIsZeroBiasMax400Pixel = bits.zeroBiasMax400Pixel;
    oIsZeroBiasMax10000 = bits.zeroBiasMax10000;
    oIsZeroBias = bits.zeroBiasMin400Max10000 || bits.zeroBiasMax400Pixel || bits.zeroBiasMax10000;
    oSelectedBkgFilter = passBkg ? 1 : 0;
    oSelectedVtxFilter = (passPv && passVz && passNVtx) ? 1 : 0;
    oClusterCompatibilityFilter = *pclusterCompatibilityFilter;
    oCscTightHalo2015Filter = *cscTightHalo2015Filter ? 1 : 0;
    oPprimaryVertexFilter = *pprimaryVertexFilter;
    oPzdcEnergyFilter0nOr = *pzdcEnergyFilter0nOr;
    oPzdcEnergyFilter1nOr = *pzdcEnergyFilter1nOr;
    oPzdcEnergyFilterXOr = *pzdcEnergyFilterXOr;
    oZDCgammaN = zdcGammaN ? 1 : 0;
    oZDCNgamma = zdcNgamma ? 1 : 0;
    oGapgammaN = gapGammaN ? 1 : 0;
    oGapNgamma = gapNgamma ? 1 : 0;
    oGammaN = (passPv && passVz && passNVtx && passBkg && gapGammaN) ? 1 : 0;
    oNgamma = (passPv && passVz && passNVtx && passBkg && gapNgamma) ? 1 : 0;
    oZDCsumPlus = *zdcSumPlus;
    oZDCsumMinus = *zdcSumMinus;
    oHFEMaxPlus = hfPlus;
    oHFEMaxMinus = hfMinus;
    oNTrackInAcceptanceHP = -1;  // TODO: add once track-array branch choice is validated.

    if (oIsL1ZDCOr) {
      ++nHltZdcOr;
      hEventFlags.Fill(1.5);
    }
    if (passPv) hEventFlags.Fill(2.5);
    if (passVz) hEventFlags.Fill(3.5);
    if (passNVtx) hEventFlags.Fill(4.5);
    if (passBkg) hEventFlags.Fill(5.5);
    if (passZdcXor) hEventFlags.Fill(6.5);
    if (passGap) hEventFlags.Fill(7.5);
    if (oGammaN || oNgamma) hEventFlags.Fill(8.5);

    vDtype.clear();
    vDpt.clear();
    vDeta.clear();
    vDphi.clear();
    vDy.clear();
    vDmass.clear();
    vDchi2cl.clear();
    vDdtheta.clear();
    vDalpha.clear();
    vDsvpvDistance.clear();
    vDsvpvDisErr.clear();
    vDsvpvDistance2D.clear();
    vDsvpvDisErr2D.clear();
    vDip3D.clear();
    vDip3derr.clear();
    vDtrk1Pt.clear();
    vDtrk2Pt.clear();
    vDtrk1PtErr.clear();
    vDtrk2PtErr.clear();
    vDtrk1Eta.clear();
    vDtrk2Eta.clear();
    vDtrk1MassHypo.clear();
    vDtrk2MassHypo.clear();
    vDtrk1P.clear();
    vDtrk2P.clear();
    vDtrk1PixelHit.clear();
    vDtrk2PixelHit.clear();
    vDtrk1StripHit.clear();
    vDtrk2StripHit.clear();
    vDtrk1highPurity.clear();
    vDtrk2highPurity.clear();
    vDpassCut23PAS.clear();
    vDpassCutNominal.clear();

    nInputCandidates += *dSize;
    const int nAvailable = std::min<int>(*dSize, dMass.GetSize());
    if (nAvailable < *dSize) ++nDsizeTruncatedByReader;
    hDsize.Fill(std::min(*dSize, 119));

    for (int i = 0; i < nAvailable; ++i) {
      const bool pass23 = PassCut23PAS(dMass[i], dPt[i], dY[i], dChi2Cl[i], dAlpha[i], dDtheta[i],
                                       dSvpvDistance[i], dSvpvDisErr[i], dTrk1Pt[i], dTrk2Pt[i],
                                       dTrk1PtErr[i], dTrk2PtErr[i], dTrk1Eta[i], dTrk2Eta[i],
                                       dTrk1HighPurity[i], dTrk2HighPurity[i]);
      vDtype.push_back(dType[i]);
      vDpt.push_back(dPt[i]);
      vDeta.push_back(dEta[i]);
      vDphi.push_back(dPhi[i]);
      vDy.push_back(dY[i]);
      vDmass.push_back(dMass[i]);
      vDchi2cl.push_back(dChi2Cl[i]);
      vDdtheta.push_back(dDtheta[i]);
      vDalpha.push_back(dAlpha[i]);
      vDsvpvDistance.push_back(dSvpvDistance[i]);
      vDsvpvDisErr.push_back(dSvpvDisErr[i]);
      vDsvpvDistance2D.push_back(dSvpvDistance2D[i]);
      vDsvpvDisErr2D.push_back(dSvpvDisErr2D[i]);
      vDip3D.push_back(dIp3D[i]);
      vDip3derr.push_back(dIp3DErr[i]);
      vDtrk1Pt.push_back(dTrk1Pt[i]);
      vDtrk2Pt.push_back(dTrk2Pt[i]);
      vDtrk1PtErr.push_back(dTrk1PtErr[i]);
      vDtrk2PtErr.push_back(dTrk2PtErr[i]);
      vDtrk1Eta.push_back(dTrk1Eta[i]);
      vDtrk2Eta.push_back(dTrk2Eta[i]);
      vDtrk1MassHypo.push_back(dTrk1MassHypo[i]);
      vDtrk2MassHypo.push_back(dTrk2MassHypo[i]);
      vDtrk1PixelHit.push_back(dTrk1PixelHit[i]);
      vDtrk2PixelHit.push_back(dTrk2PixelHit[i]);
      vDtrk1StripHit.push_back(dTrk1StripHit[i]);
      vDtrk2StripHit.push_back(dTrk2StripHit[i]);
      vDtrk1P.push_back(dTrk1P[i]);
      vDtrk2P.push_back(dTrk2P[i]);
      vDtrk1highPurity.push_back(dTrk1HighPurity[i] ? 1 : 0);
      vDtrk2highPurity.push_back(dTrk2HighPurity[i] ? 1 : 0);
      vDpassCut23PAS.push_back(pass23 ? 1 : 0);
      vDpassCutNominal.push_back(pass23 ? 1 : 0);  // TODO: replace after nominal-cut definition is located.

      hMassAll.Fill(dMass[i]);
      if (pass23) {
        ++nPassCut23;
        hMassPass.Fill(dMass[i]);
      }
    }
    oDsize = static_cast<Int_t>(vDmass.size());
    nCopiedCandidates += oDsize;
    tree.Fill();
  }

  std::cout << "Event loop done; writing output\n";
  output.cd();
  tree.Write();
  hDsize.Write();
  hMassAll.Write();
  hMassPass.Write();
  hEventFlags.Write();
  TParameter<int>("nInputFiles", static_cast<int>(files.size())).Write();
  TParameter<Long64_t>("nInputEventsInChains", entries).Write();
  TParameter<Long64_t>("nProcessedEvents", nEvents).Write();
  TParameter<Long64_t>("nHltRowsRead", hltRowsRead).Write();
  TParameter<Long64_t>("nHltUniqueEvents", static_cast<Long64_t>(hltMap.size())).Write();
  TParameter<Long64_t>("nHltDuplicateRows", hltDuplicateRows).Write();
  TParameter<Long64_t>("nHltMatchedEvents", nHltMatched).Write();
  TParameter<Long64_t>("nHltZdcOrEvents", nHltZdcOr).Write();
  TParameter<Long64_t>("nInputDfinderCandidates", nInputCandidates).Write();
  TParameter<Long64_t>("nCopiedDfinderCandidates", nCopiedCandidates).Write();
  TParameter<Long64_t>("nPassCut23PAS", nPassCut23).Write();
  TParameter<Long64_t>("nDsizeTruncatedByReader", nDsizeTruncatedByReader).Write();
  TParameter<int>("computeHF", computeHF).Write();
  output.Close();
  std::cout << "ROOT file closed\n";

  SaveHistogram(hDsize, TString(plotDir) + "/dsize.png");
  SaveHistogram(hMassAll, TString(plotDir) + "/dmass_all_candidates.png");
  SaveHistogram(hMassPass, TString(plotDir) + "/dmass_passCut23PAS.png");
  SaveHistogram(hEventFlags, TString(plotDir) + "/event_flag_counts.png");

  std::ofstream summary((TString(plotDir) + "/recreated_skim_summary.tsv").Data());
  summary << "metric\tvalue\n";
  summary << "input_files\t" << files.size() << "\n";
  summary << "chain_events\t" << entries << "\n";
  summary << "processed_events\t" << nEvents << "\n";
  summary << "hlt_rows_read\t" << hltRowsRead << "\n";
  summary << "hlt_unique_events\t" << hltMap.size() << "\n";
  summary << "hlt_duplicate_rows\t" << hltDuplicateRows << "\n";
  summary << "hlt_matched_events\t" << nHltMatched << "\n";
  summary << "hlt_zdc_or_events\t" << nHltZdcOr << "\n";
  summary << "input_dfinder_candidates\t" << nInputCandidates << "\n";
  summary << "copied_dfinder_candidates\t" << nCopiedCandidates << "\n";
  summary << "pass_cut23_pas_candidates\t" << nPassCut23 << "\n";
  summary << "dsize_truncated_by_reader\t" << nDsizeTruncatedByReader << "\n";
  summary << "compute_hf\t" << computeHF << "\n";
  summary << "output_root\t" << outputPath << "\n";
  summary.close();

  std::cout << "Wrote recreated skim: " << outputPath << "\n";
  std::cout << "Processed events: " << nEvents << "\n";
  std::cout << "Copied Dfinder candidates: " << nCopiedCandidates << "\n";
  std::cout << "DpassCut23PAS candidates: " << nPassCut23 << "\n";
  std::cout << "Summary: " << TString(plotDir) + "/recreated_skim_summary.tsv" << "\n";
  return 0;
}
