#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

#include "TLorentzVector.h"
#include "TTree.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

class D0MinimalForest : public edm::one::EDAnalyzer<> {
public:
  explicit D0MinimalForest(const edm::ParameterSet&);
  ~D0MinimalForest() override = default;

private:
  struct TrackView {
    const pat::PackedCandidate* cand = nullptr;
    int inputIndex = -1;
    bool hasTrackDetails = false;
    bool highPurity = false;
    float ptError = -1.f;
  };

  void beginJob() override;
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  void clear();
  void addCandidate(const TrackView&, const TrackView&, bool firstIsKaon);
  static float safeRapidity(const TLorentzVector&);

  edm::Service<TFileService> fs_;
  edm::EDGetTokenT<pat::PackedCandidateCollection> pfCandidateToken_;
  edm::EDGetTokenT<reco::VertexCollection> vertexToken_;

  double trackPtMin_;
  double trackEtaMax_;
  bool requireHighPurity_;
  double candidateMassMin_;
  double candidateMassMax_;
  unsigned int maxCandidates_;
  double hfEtaMin_;
  double hfEtaMax_;

  TTree* tree_ = nullptr;

  unsigned int run_ = 0;
  unsigned int lumi_ = 0;
  unsigned long long event_ = 0;
  int nPacked_ = 0;
  int nChargedTracksUsed_ = 0;
  unsigned long long nOppositeChargePairs_ = 0;
  int nD0Candidates_ = 0;
  int nVtx_ = 0;
  float bestVx_ = std::numeric_limits<float>::quiet_NaN();
  float bestVy_ = std::numeric_limits<float>::quiet_NaN();
  float bestVz_ = std::numeric_limits<float>::quiet_NaN();
  float hfEMaxPlusPacked_ = -1.f;
  float hfEMaxMinusPacked_ = -1.f;

  std::vector<int> candHypothesis_;
  std::vector<int> trk1Index_;
  std::vector<int> trk2Index_;
  std::vector<float> dMass_;
  std::vector<float> dPt_;
  std::vector<float> dEta_;
  std::vector<float> dPhi_;
  std::vector<float> dY_;

  std::vector<float> trk1Pt_;
  std::vector<float> trk1Eta_;
  std::vector<float> trk1Phi_;
  std::vector<int> trk1Charge_;
  std::vector<int> trk1PdgId_;
  std::vector<int> trk1HighPurity_;
  std::vector<int> trk1HasTrackDetails_;
  std::vector<float> trk1PtError_;

  std::vector<float> trk2Pt_;
  std::vector<float> trk2Eta_;
  std::vector<float> trk2Phi_;
  std::vector<int> trk2Charge_;
  std::vector<int> trk2PdgId_;
  std::vector<int> trk2HighPurity_;
  std::vector<int> trk2HasTrackDetails_;
  std::vector<float> trk2PtError_;
};

D0MinimalForest::D0MinimalForest(const edm::ParameterSet& iConfig)
    : pfCandidateToken_(consumes<pat::PackedCandidateCollection>(
          iConfig.getParameter<edm::InputTag>("pfCandidateSrc"))),
      vertexToken_(consumes<reco::VertexCollection>(iConfig.getParameter<edm::InputTag>("vertexSrc"))),
      trackPtMin_(iConfig.getParameter<double>("trackPtMin")),
      trackEtaMax_(iConfig.getParameter<double>("trackEtaMax")),
      requireHighPurity_(iConfig.getParameter<bool>("requireHighPurity")),
      candidateMassMin_(iConfig.getParameter<double>("candidateMassMin")),
      candidateMassMax_(iConfig.getParameter<double>("candidateMassMax")),
      maxCandidates_(iConfig.getParameter<unsigned int>("maxCandidates")),
      hfEtaMin_(iConfig.getParameter<double>("hfEtaMin")),
      hfEtaMax_(iConfig.getParameter<double>("hfEtaMax")) {}

void D0MinimalForest::beginJob() {
  tree_ = fs_->make<TTree>("d0minimal", "minimal D0 candidate forest from packed MINIAOD candidates");

  tree_->Branch("run", &run_, "run/i");
  tree_->Branch("lumi", &lumi_, "lumi/i");
  tree_->Branch("event", &event_, "event/l");
  tree_->Branch("nPacked", &nPacked_, "nPacked/I");
  tree_->Branch("nChargedTracksUsed", &nChargedTracksUsed_, "nChargedTracksUsed/I");
  tree_->Branch("nOppositeChargePairs", &nOppositeChargePairs_, "nOppositeChargePairs/l");
  tree_->Branch("nD0Candidates", &nD0Candidates_, "nD0Candidates/I");
  tree_->Branch("nVtx", &nVtx_, "nVtx/I");
  tree_->Branch("bestVx", &bestVx_, "bestVx/F");
  tree_->Branch("bestVy", &bestVy_, "bestVy/F");
  tree_->Branch("bestVz", &bestVz_, "bestVz/F");
  tree_->Branch("hfEMaxPlusPacked", &hfEMaxPlusPacked_, "hfEMaxPlusPacked/F");
  tree_->Branch("hfEMaxMinusPacked", &hfEMaxMinusPacked_, "hfEMaxMinusPacked/F");

  tree_->Branch("candHypothesis", &candHypothesis_);
  tree_->Branch("trk1Index", &trk1Index_);
  tree_->Branch("trk2Index", &trk2Index_);
  tree_->Branch("dMass", &dMass_);
  tree_->Branch("dPt", &dPt_);
  tree_->Branch("dEta", &dEta_);
  tree_->Branch("dPhi", &dPhi_);
  tree_->Branch("dY", &dY_);

  tree_->Branch("trk1Pt", &trk1Pt_);
  tree_->Branch("trk1Eta", &trk1Eta_);
  tree_->Branch("trk1Phi", &trk1Phi_);
  tree_->Branch("trk1Charge", &trk1Charge_);
  tree_->Branch("trk1PdgId", &trk1PdgId_);
  tree_->Branch("trk1HighPurity", &trk1HighPurity_);
  tree_->Branch("trk1HasTrackDetails", &trk1HasTrackDetails_);
  tree_->Branch("trk1PtError", &trk1PtError_);

  tree_->Branch("trk2Pt", &trk2Pt_);
  tree_->Branch("trk2Eta", &trk2Eta_);
  tree_->Branch("trk2Phi", &trk2Phi_);
  tree_->Branch("trk2Charge", &trk2Charge_);
  tree_->Branch("trk2PdgId", &trk2PdgId_);
  tree_->Branch("trk2HighPurity", &trk2HighPurity_);
  tree_->Branch("trk2HasTrackDetails", &trk2HasTrackDetails_);
  tree_->Branch("trk2PtError", &trk2PtError_);
}

void D0MinimalForest::analyze(const edm::Event& iEvent, const edm::EventSetup&) {
  clear();

  run_ = iEvent.id().run();
  lumi_ = iEvent.id().luminosityBlock();
  event_ = iEvent.id().event();

  edm::Handle<reco::VertexCollection> vertices;
  iEvent.getByToken(vertexToken_, vertices);
  if (vertices.isValid()) {
    nVtx_ = static_cast<int>(vertices->size());
    if (!vertices->empty()) {
      bestVx_ = vertices->front().x();
      bestVy_ = vertices->front().y();
      bestVz_ = vertices->front().z();
    }
  }

  edm::Handle<pat::PackedCandidateCollection> pfCandidates;
  iEvent.getByToken(pfCandidateToken_, pfCandidates);
  if (!pfCandidates.isValid()) {
    tree_->Fill();
    return;
  }

  nPacked_ = static_cast<int>(pfCandidates->size());
  std::vector<TrackView> tracks;
  tracks.reserve(pfCandidates->size());

  for (std::size_t i = 0; i < pfCandidates->size(); ++i) {
    const auto& cand = pfCandidates->at(i);
    const float eta = cand.eta();
    const float absEta = std::abs(eta);

    if (absEta > hfEtaMin_ && absEta < hfEtaMax_) {
      if (eta > 0.f) {
        hfEMaxPlusPacked_ = std::max(hfEMaxPlusPacked_, static_cast<float>(cand.energy()));
      } else {
        hfEMaxMinusPacked_ = std::max(hfEMaxMinusPacked_, static_cast<float>(cand.energy()));
      }
    }

    if (cand.charge() == 0)
      continue;
    if (cand.pt() < trackPtMin_)
      continue;
    if (std::abs(cand.eta()) > trackEtaMax_)
      continue;

    TrackView view;
    view.cand = &cand;
    view.inputIndex = static_cast<int>(i);
    view.hasTrackDetails = cand.hasTrackDetails();
    view.highPurity = view.hasTrackDetails && cand.trackHighPurity();
    if (requireHighPurity_ && !view.highPurity)
      continue;
    if (view.hasTrackDetails && cand.bestTrack() != nullptr) {
      view.ptError = cand.bestTrack()->ptError();
    }
    tracks.push_back(view);
  }

  nChargedTracksUsed_ = static_cast<int>(tracks.size());

  for (std::size_t i = 0; i < tracks.size(); ++i) {
    for (std::size_t j = i + 1; j < tracks.size(); ++j) {
      if (tracks[i].cand->charge() * tracks[j].cand->charge() >= 0)
        continue;

      ++nOppositeChargePairs_;
      addCandidate(tracks[i], tracks[j], true);
      addCandidate(tracks[i], tracks[j], false);
      if (static_cast<unsigned int>(nD0Candidates_) >= maxCandidates_)
        break;
    }
    if (static_cast<unsigned int>(nD0Candidates_) >= maxCandidates_)
      break;
  }

  tree_->Fill();
}

void D0MinimalForest::addCandidate(const TrackView& first, const TrackView& second, bool firstIsKaon) {
  if (static_cast<unsigned int>(nD0Candidates_) >= maxCandidates_)
    return;

  constexpr double pionMass = 0.13957039;
  constexpr double kaonMass = 0.493677;

  TLorentzVector v1;
  TLorentzVector v2;
  v1.SetPtEtaPhiM(first.cand->pt(), first.cand->eta(), first.cand->phi(), firstIsKaon ? kaonMass : pionMass);
  v2.SetPtEtaPhiM(second.cand->pt(), second.cand->eta(), second.cand->phi(), firstIsKaon ? pionMass : kaonMass);
  const TLorentzVector d0 = v1 + v2;
  const float mass = d0.M();
  if (mass < candidateMassMin_ || mass > candidateMassMax_)
    return;

  candHypothesis_.push_back(firstIsKaon ? 0 : 1);
  trk1Index_.push_back(first.inputIndex);
  trk2Index_.push_back(second.inputIndex);
  dMass_.push_back(mass);
  dPt_.push_back(d0.Pt());
  dEta_.push_back(d0.Eta());
  dPhi_.push_back(d0.Phi());
  dY_.push_back(safeRapidity(d0));

  trk1Pt_.push_back(first.cand->pt());
  trk1Eta_.push_back(first.cand->eta());
  trk1Phi_.push_back(first.cand->phi());
  trk1Charge_.push_back(first.cand->charge());
  trk1PdgId_.push_back(first.cand->pdgId());
  trk1HighPurity_.push_back(first.highPurity ? 1 : 0);
  trk1HasTrackDetails_.push_back(first.hasTrackDetails ? 1 : 0);
  trk1PtError_.push_back(first.ptError);

  trk2Pt_.push_back(second.cand->pt());
  trk2Eta_.push_back(second.cand->eta());
  trk2Phi_.push_back(second.cand->phi());
  trk2Charge_.push_back(second.cand->charge());
  trk2PdgId_.push_back(second.cand->pdgId());
  trk2HighPurity_.push_back(second.highPurity ? 1 : 0);
  trk2HasTrackDetails_.push_back(second.hasTrackDetails ? 1 : 0);
  trk2PtError_.push_back(second.ptError);

  ++nD0Candidates_;
}

float D0MinimalForest::safeRapidity(const TLorentzVector& v) {
  const double denom = v.E() - v.Pz();
  const double numer = v.E() + v.Pz();
  if (denom <= 0.0 || numer <= 0.0)
    return std::numeric_limits<float>::quiet_NaN();
  return 0.5f * std::log(numer / denom);
}

void D0MinimalForest::clear() {
  run_ = 0;
  lumi_ = 0;
  event_ = 0;
  nPacked_ = 0;
  nChargedTracksUsed_ = 0;
  nOppositeChargePairs_ = 0;
  nD0Candidates_ = 0;
  nVtx_ = 0;
  bestVx_ = std::numeric_limits<float>::quiet_NaN();
  bestVy_ = std::numeric_limits<float>::quiet_NaN();
  bestVz_ = std::numeric_limits<float>::quiet_NaN();
  hfEMaxPlusPacked_ = -1.f;
  hfEMaxMinusPacked_ = -1.f;

  candHypothesis_.clear();
  trk1Index_.clear();
  trk2Index_.clear();
  dMass_.clear();
  dPt_.clear();
  dEta_.clear();
  dPhi_.clear();
  dY_.clear();

  trk1Pt_.clear();
  trk1Eta_.clear();
  trk1Phi_.clear();
  trk1Charge_.clear();
  trk1PdgId_.clear();
  trk1HighPurity_.clear();
  trk1HasTrackDetails_.clear();
  trk1PtError_.clear();

  trk2Pt_.clear();
  trk2Eta_.clear();
  trk2Phi_.clear();
  trk2Charge_.clear();
  trk2PdgId_.clear();
  trk2HighPurity_.clear();
  trk2HasTrackDetails_.clear();
  trk2PtError_.clear();
}

DEFINE_FWK_MODULE(D0MinimalForest);
