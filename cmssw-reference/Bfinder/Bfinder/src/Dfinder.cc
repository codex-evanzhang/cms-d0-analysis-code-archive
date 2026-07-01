// vim:set ts=4 sw=4 fdm=marker et:
// Ntuplt creator for D meson related analysis.
// Maintain and contact: ta-wei wang
// Email: "tawei@mit.edu" or "ta-wei.wang@cern.ch"
#include "Bfinder/Bfinder/interface/format.h"
#include "Bfinder/Bfinder/interface/Dntuple.h"
#include "Bfinder/Bfinder/interface/utilities.h"
//
// class declaration
//

using namespace std;

class Dfinder : public edm::one::EDAnalyzer<edm::one::WatchRuns>
{//{{{
public:
  explicit Dfinder(const edm::ParameterSet&);
  ~Dfinder();
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  virtual void beginJob() ;
  virtual void analyze(const edm::Event&, const edm::EventSetup&);
  virtual void endJob() ;

  virtual void beginRun(edm::Run const&, edm::EventSetup const&);
  virtual void endRun(edm::Run const&, edm::EventSetup const&);
  virtual void beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&);
  virtual void endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&);

  virtual std::vector< std::vector< std::pair<float, int> > > GetPermu(std::vector< std::pair<float, int> > InVec);
  virtual std::vector< std::vector< std::pair<float, int> > > DelDuplicate(std::vector< std::vector< std::pair<float, int> > > InVec);

  virtual void BranchOutNTk(
                            DInfoBranches &DInfo, 
                            // edm::View<pat::PackedCandidate> input_tracks, 
                            std::vector<const reco::Track*> input_tracks,
                            reco::Vertex thePrimaryV,
                            std::vector<int> isNeededTrackIdx,
                            std::vector<int> &D_counter,
                            float *mass_window,
                            std::vector< std::pair<float, int> > TkMassCharge,
                            double tktkRes_mass,
                            double tktkRes_mass_window,
                            bool doConstrainFit,
                            bool SequentialFit,
                            int Dchannel_number,
                            int TkCombinationMethod = -1
                            );

  virtual void TkCombinationPermutation(
                                        reco::Vertex thePrimaryV,
                                        // edm::View<pat::PackedCandidate> input_tracks,
                                        std::vector<const reco::Track*> input_tracks,
                                        std::vector<int> isNeededTrackIdx,
                                        float *mass_window,
                                        std::vector< std::pair<float, int> > TkMassCharge,
                                        double tktkRes_mass,
                                        double tktkRes_mass_window,
                                        std::vector< std::vector<int> > &selectedTkhidxSet,
                                        int Dchannel_number
                                        );

  virtual void TkCombinationResFast(
                                    reco::Vertex thePrimaryV,
                                    // edm::View<pat::PackedCandidate> input_tracks,
                                    std::vector<const reco::Track*> input_tracks,
                                    std::vector<int> isNeededTrackIdx,
                                    float *mass_window,
                                    std::vector< std::pair<float, int> > TkMassCharge,
                                    double tktkRes_mass,
                                    double tktkRes_mass_window,
                                    std::vector< std::vector<int> > &selectedTkhidxSet,
                                    int Dchannel_number
                                    );

  static const reco::Track* getFromPC(const reco::Candidate& rc) {
    const pat::PackedCandidate* pp = dynamic_cast<const pat::PackedCandidate*>(&rc);
    if (pp == nullptr)
      return nullptr;
    try {
      const reco::Track* trk = &pp->pseudoTrack();
      return trk;
    } catch (edm::Exception const&) {
    }
    return nullptr;
  }
  // ----------member data ---------------------------
  edm::ESHandle<MagneticField> bField;
  edm::ESGetToken<MagneticField, IdealMagneticFieldRecord> idealMagneticFieldRecordToken_;
  edm::ParameterSet theConfig;

  std::vector<int> Dchannel_;
  edm::EDGetTokenT< std::vector<reco::GenParticle> > genLabel_;
  edm::EDGetTokenT< edm::View<pat::PackedCandidate> > trackLabel_;
  // edm::EDGetTokenT< edm::View<pat::PackedCandidate> > losttrackLabel_;
  edm::EDGetTokenT< edm::ValueMap<float> > chi2Map_;
  edm::EDGetTokenT< edm::ValueMap<reco::DeDxData> > dedxMap_;
  edm::EDGetTokenT< reco::BeamSpot > bsLabel_;
  edm::EDGetTokenT< reco::VertexCollection > pvLabel_;

  double tkPtCut_;
  double tkEtaCut_;
  std::vector<double> dCutSeparating_PtVal_;
  std::vector<double> dPtCut_;
  std::vector<double> dRapidityCut_;
  std::vector<double> VtxChiProbCut_;
  std::vector<double> svpvDistanceCut_lowptD_;
  std::vector<double> svpvDistanceCut_highptD_;
  std::vector<double> alphaCut_;
  std::vector<double> MaxDocaCut_;
  std::vector<double> tktkRes_dCutSeparating_PtVal_;
  std::vector<double> tktkRes_dPtCut_;
  std::vector<double> tktkRes_VtxChiProbCut_;
  std::vector<double> tktkRes_svpvDistanceCut_lowptD_;
  std::vector<double> tktkRes_svpvDistanceCut_highptD_;
  std::vector<double> tktkRes_svpvDistanceToSVCut_lowptD_;
  std::vector<double> tktkRes_svpvDistanceToSVCut_highptD_;
  std::vector<double> tktkRes_alphaCut_;
  std::vector<double> tktkRes_alphaToSVCut_;
  std::vector<double> ResToNonRes_PtAsym_min_;
  std::vector<double> ResToNonRes_PtAsym_max_;
  double tktkRes_masswindowCut_ = 0.0 ;
  bool makeDntuple_;
  bool doDntupleSkim_;
  bool printInfo_;
  bool readDedx_;
  bool dropUnusedTracks_;

  edm::Service<TFileService> fs;
  TTree *root;
  EvtInfoBranches     EvtInfo;
  VtxInfoBranches     VtxInfo;
  TrackInfoBranches   TrackInfo;
  DInfoBranches       DInfo;
  GenInfoBranches     GenInfo;
  CommonFuncts        Functs;
  DntupleBranches     *Dntuple;
  TTree* ntD1; 
  TTree* ntD2;
  TTree* ntD3; 
  TTree* ntD4; 
  TTree* ntD5; 
  TTree* ntD6; 
  TTree* ntD7;
  TTree* ntD8;
  TTree* ntD9;
  TTree* ntGen;

};//}}}

void Dfinder::beginJob()
{//{{{
  Dntuple = new DntupleBranches;
  root = fs->make<TTree>("root","root");
  ntD1 = fs->make<TTree>("ntDkpi","");           Dntuple->buildDBranch(ntD1, true, true);
  ntD2 = fs->make<TTree>("ntDkpipi","");         Dntuple->buildDBranch(ntD2);
  ntD3 = fs->make<TTree>("ntDkpipipi","");       Dntuple->buildDBranch(ntD3);
  ntD4 = fs->make<TTree>("ntDPhikkpi","");       Dntuple->buildDBranch(ntD4);
  ntD5 = fs->make<TTree>("ntDD0kpipi","");       Dntuple->buildDBranch(ntD5);
  ntD6 = fs->make<TTree>("ntDD0kpipipipi","");   Dntuple->buildDBranch(ntD6);
  ntD7 = fs->make<TTree>("ntBptoD0pi","");       Dntuple->buildDBranch(ntD7);
  ntD8 = fs->make<TTree>("ntLctopkpi","");       Dntuple->buildDBranch(ntD8);
  ntD9 = fs->make<TTree>("ntLctopkstoppipi",""); Dntuple->buildDBranch(ntD9);
  ntGen = fs->make<TTree>("ntGen","");           Dntuple->buildGenBranch(ntGen);
  EvtInfo.regTree(root);
  VtxInfo.regTree(root);
  TrackInfo.regTree(root);
  DInfo.regTree(root);
  GenInfo.regTree(root);
}//}}}

Dfinder::Dfinder(const edm::ParameterSet& iConfig):theConfig(iConfig)
{//{{{
  //now do what ever initialization is needed

  idealMagneticFieldRecordToken_ = esConsumes();
  // idealMagneticFieldRecordToken_ = esConsumes(edm::ESInputTag("")); // ParabolicMf

  Dchannel_ = iConfig.getParameter<std::vector<int> >("Dchannel");
  genLabel_ = consumes< std::vector<reco::GenParticle> >(iConfig.getParameter<edm::InputTag>("GenLabel"));
  trackLabel_ = consumes< edm::View<pat::PackedCandidate> >(iConfig.getParameter<edm::InputTag>("TrackLabel"));
  // losttrackLabel_ = consumes< edm::View<pat::PackedCandidate> >(iConfig.getParameter<edm::InputTag>("lostTrackLabel"));
  chi2Map_ = consumes< edm::ValueMap<float> >(iConfig.getParameter< edm::InputTag >("TrackChi2Label"));
  dedxMap_ = consumes< edm::ValueMap<reco::DeDxData> >(iConfig.getParameter<edm::InputTag>("TrackDedxLabel"));
  bsLabel_ = consumes< reco::BeamSpot >(iConfig.getParameter<edm::InputTag>("BSLabel"));
  pvLabel_ = consumes< reco::VertexCollection >(iConfig.getParameter<edm::InputTag>("PVLabel"));

  tkPtCut_ = iConfig.getParameter<double>("tkPtCut");
  tkEtaCut_ = iConfig.getParameter<double>("tkEtaCut");
  dCutSeparating_PtVal_ = iConfig.getParameter<std::vector<double> >("dCutSeparating_PtVal");
  dPtCut_ = iConfig.getParameter<std::vector<double> >("dPtCut");
  dRapidityCut_ = iConfig.getParameter<std::vector<double> >("dRapidityCut");
  VtxChiProbCut_ = iConfig.getParameter<std::vector<double> >("VtxChiProbCut");
  svpvDistanceCut_lowptD_ = iConfig.getParameter<std::vector<double> >("svpvDistanceCut_lowptD");
  svpvDistanceCut_highptD_ = iConfig.getParameter<std::vector<double> >("svpvDistanceCut_highptD");
  alphaCut_ = iConfig.getParameter<std::vector<double> >("alphaCut");
  MaxDocaCut_ = iConfig.getParameter<std::vector<double> >("MaxDocaCut");
  tktkRes_dCutSeparating_PtVal_ = iConfig.getParameter<std::vector<double> >("tktkRes_dCutSeparating_PtVal");
  tktkRes_dPtCut_ = iConfig.getParameter<std::vector<double> >("tktkRes_dPtCut");
  tktkRes_VtxChiProbCut_ = iConfig.getParameter<std::vector<double> >("tktkRes_VtxChiProbCut");
  tktkRes_svpvDistanceCut_lowptD_ = iConfig.getParameter<std::vector<double> >("tktkRes_svpvDistanceCut_lowptD");
  tktkRes_svpvDistanceCut_highptD_ = iConfig.getParameter<std::vector<double> >("tktkRes_svpvDistanceCut_highptD");
  tktkRes_svpvDistanceToSVCut_lowptD_ = iConfig.getParameter<std::vector<double> >("tktkRes_svpvDistanceToSVCut_lowptD");
  tktkRes_svpvDistanceToSVCut_highptD_ = iConfig.getParameter<std::vector<double> >("tktkRes_svpvDistanceToSVCut_highptD");
  tktkRes_alphaCut_ = iConfig.getParameter<std::vector<double> >("tktkRes_alphaCut");
  tktkRes_alphaToSVCut_ = iConfig.getParameter<std::vector<double> >("tktkRes_alphaToSVCut");
  ResToNonRes_PtAsym_min_= iConfig.getParameter<std::vector<double> >("ResToNonRes_PtAsym_min");
  ResToNonRes_PtAsym_max_= iConfig.getParameter<std::vector<double> >("ResToNonRes_PtAsym_max");
  makeDntuple_ = iConfig.getParameter<bool>("makeDntuple");
  doDntupleSkim_ = iConfig.getParameter<bool>("doDntupleSkim");
  printInfo_ = iConfig.getParameter<bool>("printInfo");
  readDedx_ = iConfig.getParameter<bool>("readDedx");
  dropUnusedTracks_ = iConfig.getParameter<bool>("dropUnusedTracks");

  if (iConfig.exists("tktkRes_masswindowCut")) { tktkRes_masswindowCut_ = iConfig.getParameter<double>("tktkRes_masswindowCut"); }
    
}

Dfinder::~Dfinder()
{//{{{
  // do anything here that needs to be done at desctruction time
  // (e.g. close files, deallocate resources etc.)
}//}}}

//
// member functions
//

// ------------ method called for each event  ------------
void Dfinder::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  //checking input parameter size
  if((Dchannel_.size() != dCutSeparating_PtVal_.size())
     || (Dchannel_.size() != dPtCut_.size())
     || (Dchannel_.size() != dRapidityCut_.size())
     || (Dchannel_.size() != VtxChiProbCut_.size())
     || (Dchannel_.size() != svpvDistanceCut_lowptD_.size())
     || (Dchannel_.size() != svpvDistanceCut_highptD_.size())
     || (Dchannel_.size() != alphaCut_.size())
     || (Dchannel_.size() != MaxDocaCut_.size())
     || (Dchannel_.size() != tktkRes_dCutSeparating_PtVal_.size())
     || (Dchannel_.size() != tktkRes_dPtCut_.size())
     || (Dchannel_.size() != tktkRes_VtxChiProbCut_.size())
     || (Dchannel_.size() != tktkRes_svpvDistanceCut_lowptD_.size())
     || (Dchannel_.size() != tktkRes_svpvDistanceCut_highptD_.size())
     || (Dchannel_.size() != tktkRes_svpvDistanceToSVCut_lowptD_.size())
     || (Dchannel_.size() != tktkRes_svpvDistanceToSVCut_highptD_.size())
     || (Dchannel_.size() != tktkRes_alphaCut_.size())
     || (Dchannel_.size() != tktkRes_alphaToSVCut_.size())
     || (Dchannel_.size() != ResToNonRes_PtAsym_min_.size())
     || (Dchannel_.size() != ResToNonRes_PtAsym_max_.size())
     ){
    std::cout<<"Unmatched input parameter vector size, EXIT"<<std::endl;
    return;
  }

  //std::cout << "*************************\nReconstructing event number: " << iEvent.id() << "\n";
  using namespace edm;
  using namespace reco;
  //ESHandle<MagneticField> bField;
  bField = iSetup.getHandle(idealMagneticFieldRecordToken_);

  // Change used muon and track collections
  auto tks = iEvent.getHandle( trackLabel_ ); // edm::Handle< edm::View<pat::PackedCandidate> >
  // auto losttks = iEvent.getHandle(losttrackLabel_);
  auto chi2Handle = iEvent.getHandle(chi2Map_); // edm::Handle<edm::ValueMap<float>>
  auto dedxHandle = iEvent.getHandle(dedxMap_); // edm::Handle<edm::ValueMap<reco::DeDxData>>

  edm::Handle<std::vector<reco::GenParticle>> gens;
  if (!iEvent.isRealData()) {
    iEvent.getByToken(genLabel_, gens);
  }

  //CLEAN all memory
  memset(&EvtInfo     ,0x00,sizeof(EvtInfo)   );
  memset(&VtxInfo     ,0x00,sizeof(VtxInfo)   );
  memset(&TrackInfo   ,0x00,sizeof(TrackInfo) );
  memset(&DInfo       ,0x00,sizeof(DInfo)    );
  memset(&GenInfo     ,0x00,sizeof(GenInfo)   );

  // EvtInfo section{{{
  EvtInfo.RunNo   = iEvent.id().run();
  EvtInfo.EvtNo   = iEvent.id().event();
  EvtInfo.BxNo    = iEvent.bunchCrossing();
  EvtInfo.LumiNo  = iEvent.luminosityBlock();
  EvtInfo.Orbit   = iEvent.orbitNumber();
  EvtInfo.McFlag  = !iEvent.isRealData();

  // Handle primary vertex properties
  Vertex thePrimaryV;
  math::XYZPoint RefVtx;
  //get beamspot information
  Vertex theBeamSpotV;
  reco::BeamSpot beamSpot;
  edm::Handle<reco::BeamSpot> beamSpotHandle;
  iEvent.getByToken(bsLabel_, beamSpotHandle);
  if (beamSpotHandle.isValid()){
    beamSpot = *beamSpotHandle;
    theBeamSpotV = Vertex(beamSpot.position(), beamSpot.covariance3D());
    EvtInfo.BSx             = beamSpot.x0();
    EvtInfo.BSy             = beamSpot.y0();
    EvtInfo.BSz             = beamSpot.z0();
    EvtInfo.BSxErr          = beamSpot.x0Error();
    EvtInfo.BSyErr          = beamSpot.y0Error();
    EvtInfo.BSzErr          = beamSpot.z0Error();
    EvtInfo.BSdxdz          = beamSpot.dxdz();
    EvtInfo.BSdydz          = beamSpot.dydz();
    EvtInfo.BSdxdzErr       = beamSpot.dxdzError();
    EvtInfo.BSdydzErr       = beamSpot.dydzError();
    EvtInfo.BSWidthX        = beamSpot.BeamWidthX();
    EvtInfo.BSWidthXErr     = beamSpot.BeamWidthXError();
    EvtInfo.BSWidthY        = beamSpot.BeamWidthY();
    EvtInfo.BSWidthYErr     = beamSpot.BeamWidthYError();
  } else {
    std::cout<< "No beam spot available from EventSetup \n";
  }

  //get vertex informationa
  edm::Handle<reco::VertexCollection> VertexHandle;
  iEvent.getByToken(pvLabel_, VertexHandle);

  double PVBS_Pt_Max = -100.;
  reco::Vertex PVtx_BS;
  if( VertexHandle.isValid() && !VertexHandle.failedToGet() && VertexHandle->size() > 0) {
    //const vector<reco::Vertex> VerticesBS = *VertexHandle;
    for(std::vector<reco::Vertex>::const_iterator it_vtx = VertexHandle->begin();it_vtx != VertexHandle->end(); it_vtx++ ) {
      if (VtxInfo.Size>=MAX_Vertices) {
        std::cout << "PVBS " << VtxInfo.Size << std::endl;
        fprintf(stderr,"ERROR: number of  Vertices exceeds the size of array.\n");
        break;//exit(0);
      }
      VtxInfo.isValid[VtxInfo.Size] = it_vtx->isValid();
      VtxInfo.isFake[VtxInfo.Size] = it_vtx->isFake();
      VtxInfo.Ndof[VtxInfo.Size] = it_vtx->ndof();
      VtxInfo.NormalizedChi2[VtxInfo.Size] = it_vtx->normalizedChi2();
      VtxInfo.x[VtxInfo.Size] = it_vtx->x(); 
      VtxInfo.y[VtxInfo.Size] = it_vtx->y();
      VtxInfo.z[VtxInfo.Size] = it_vtx->z();
      VtxInfo.Pt_Sum[VtxInfo.Size] = 0.;
      VtxInfo.Pt_Sum2[VtxInfo.Size] = 0.;
      //if its hiSelectedVertex, then there will be only one vertex and will have no associated tracks
      if(int(VertexHandle->end()-VertexHandle->begin())==1){
        thePrimaryV = *it_vtx;
        VtxInfo.Size++;
        break;
      }
      for (reco::Vertex::trackRef_iterator it = it_vtx->tracks_begin(); it != it_vtx->tracks_end(); it++) {
        VtxInfo.Pt_Sum[VtxInfo.Size] += (*it)->pt();
        VtxInfo.Pt_Sum2[VtxInfo.Size] += ((*it)->pt() * (*it)->pt());
      }
      if( VtxInfo.Pt_Sum[VtxInfo.Size] >= PVBS_Pt_Max ){
        PVBS_Pt_Max = VtxInfo.Pt_Sum[VtxInfo.Size];
        thePrimaryV = *it_vtx;
      }            
      VtxInfo.Size++;
    }
  }else{ 
    thePrimaryV = Vertex(beamSpot.position(), beamSpot.covariance3D());
 } 
  RefVtx = thePrimaryV.position();

  EvtInfo.PVx     = thePrimaryV.position().x();
  EvtInfo.PVy     = thePrimaryV.position().y();
  EvtInfo.PVz     = thePrimaryV.position().z();
  EvtInfo.PVxE    = thePrimaryV.xError();
  EvtInfo.PVyE    = thePrimaryV.yError();
  EvtInfo.PVzE    = thePrimaryV.zError();
  EvtInfo.PVnchi2 = thePrimaryV.normalizedChi2();
  EvtInfo.PVchi2  = thePrimaryV.chi2();

  // Double check size=0.
  TrackInfo.size  = 0;
  DInfo.size      = 0;
  GenInfo.size    = 0;

  std::vector<int> D_counter;
  for(unsigned int i = 0; i < Dchannel_.size(); i++){
    D_counter.push_back(0);
  }

  auto input_pc_tracks = *tks; // edm::View<pat::PackedCandidate>
  std::vector<const reco::Track*> input_tracks;
  for(auto tk_it=tks->begin(); tk_it != tks->end(); tk_it++){
      // static const reco::Track* getFromPC(const reco::Candidate& rc) {
    auto rt = getFromPC((*tk_it));
    input_tracks.push_back(rt);
  }
  // auto input_losttracks = *losttks;
  // input_tracks.insert(input_tracks.end(), input_losttracks.begin(), input_losttracks.end());
  try {
    // const reco::GenParticle* genMuonPtr[MAX_MUON];
    // // memset(genMuonPtr,0x00,MAX_MUON);
    // memset(genMuonPtr,0x00,MAX_MUON*sizeof(genMuonPtr[0]));
    // const reco::GenParticle* genTrackPtr[MAX_TRACK];
    int genTrackPtr[MAX_TRACK];
    // memset(genTrackPtr,0x00,MAX_GEN);
    // memset(genTrackPtr,0x00,MAX_TRACK*sizeof(genTrackPtr[0]));
    //standard check for validity of input data

    if (input_tracks.size() > 0) {
      if (printInfo_) std::cout << "Got " << input_tracks.size() << " tracks" << std::endl;
      //Preselect tracks{{{
      std::vector<bool> isNeededTrack;// Are the tracks redundant?
      std::vector<int> isNeededTrackIdx;
      int PassedTrk = 0;
      for(auto tk_it_it=input_tracks.begin(); // edm::View<pat::PackedCandidate>::const_iterator tk_it
          tk_it_it != input_tracks.end(); tk_it_it++){
        if(PassedTrk >= MAX_TRACK){
          fprintf(stderr,"ERROR: number of tracks exceeds the size of array.\n");
          break;
        }
        auto tk_it = (*tk_it_it); // edm::View<pat::PackedCandidate>
        isNeededTrack.push_back(false);
        if (!tk_it) continue;
        // if(!tk_it->hasTrackDetails()) continue;
        if (abs(tk_it->charge()) != 1) continue;
        if (tk_it->pt()<tkPtCut_) continue;
        if (fabs(tk_it->eta())>tkEtaCut_) continue;
        if( !(tk_it->/*pseudoTrack().*/quality(reco::TrackBase::qualityByName("highPurity")))) continue;
        // if( std::abs( tk_it->/*pseudoTrack().*/ptError() ) / tk_it->/*pseudoTrack().*/pt() >= 0.1 ) continue;
        // if( tk_it->/*pseudoTrack().*/numberOfValidHits() < 11 ) continue;

        isNeededTrack[tk_it_it-input_tracks.begin()] = true;
        isNeededTrackIdx.push_back(tk_it_it-input_tracks.begin());
        PassedTrk++;
      }//end of track preselection}}}
      //printf("-----*****DEBUG:End of track preselection.\n");
      if(printInfo_) std::cout<<"PassedTrk: "<<PassedTrk<<std::endl;

      // DInfo section{{{
      //////////////////////////////////////////////////////////////////////////
      // RECONSTRUCTION: K+pi-
      //////////////////////////////////////////////////////////////////////////
      float d0_mass_window[2] = {D0_MASS-0.4,D0_MASS+0.4};

      if(Dchannel_[0] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(KAON_MASS, 0);
        std::pair<float, int> tk2 = std::make_pair(-PION_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        for(unsigned int i = 0; i < PermuVec.size(); i++){
          Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, d0_mass_window, PermuVec[i], -1, -1, false, false, 1, 0);
        }
        // Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, d0_mass_window, InVec, -1, -1, false, false, 1, 1);
      }
      //////////////////////////////////////////////////////////////////////////
      // RECONSTRUCTION: K-pi+
      //////////////////////////////////////////////////////////////////////////
      if(Dchannel_[1] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(-KAON_MASS, 0);
        std::pair<float, int> tk2 = std::make_pair(PION_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        for(unsigned int i = 0; i < PermuVec.size(); i++){
          Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, d0_mass_window, PermuVec[i], -1, -1, false, false, 2, 0);
        }
        // Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, d0_mass_window, InVec, -1, -1, false, false, 2, 1);
      }
      //////////////////////////////////////////////////////////////////////////
      // RECONSTRUCTION: K-pi+pi+
      //////////////////////////////////////////////////////////////////////////
      float dplus_mass_window[2] = {DPLUS_MASS-0.2,DPLUS_MASS+0.2};
      if(Dchannel_[2] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(-KAON_MASS, 0);
        std::pair<float, int> tk2 = std::make_pair(PION_MASS, 0);
        std::pair<float, int> tk3 = std::make_pair(PION_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        InVec.push_back(tk3);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        for(unsigned int i = 0; i < PermuVec.size(); i++){
          Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, dplus_mass_window, PermuVec[i], -1, -1, false, false, 3, 0);
        }
        //Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, dplus_mass_window, InVec, -1, -1, false, false, 3, 1);
      }
      //////////////////////////////////////////////////////////////////////////
      // RECONSTRUCTION: K+pi-pi-
      //////////////////////////////////////////////////////////////////////////
      if(Dchannel_[3] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(KAON_MASS, 0);
        std::pair<float, int> tk2 = std::make_pair(-PION_MASS, 0);
        std::pair<float, int> tk3 = std::make_pair(-PION_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        InVec.push_back(tk3);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        for(unsigned int i = 0; i < PermuVec.size(); i++){
          Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, dplus_mass_window, PermuVec[i], -1, -1, false, false, 4, 0);
        }
        //Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, dplus_mass_window, InVec, -1, -1, false, false, 4, 1);
      }
      //////////////////////////////////////////////////////////////////////////
      // RECONSTRUCTION: K-pi-pi+pi+
      //////////////////////////////////////////////////////////////////////////
      if(Dchannel_[4] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(-KAON_MASS, 0);
        std::pair<float, int> tk2 = std::make_pair(-PION_MASS, 0);
        std::pair<float, int> tk3 = std::make_pair(PION_MASS, 0);
        std::pair<float, int> tk4 = std::make_pair(PION_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        InVec.push_back(tk3);
        InVec.push_back(tk4);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        for(unsigned int i = 0; i < PermuVec.size(); i++){
          Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, d0_mass_window, PermuVec[i], -1, -1, false, false, 5, 0);
        }
        //Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, d0_mass_window, InVec, -1, -1, false, false, 5, 1);
      }
      //////////////////////////////////////////////////////////////////////////
      // RECONSTRUCTION: K+pi+pi-pi-
      //////////////////////////////////////////////////////////////////////////
      if(Dchannel_[5] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(KAON_MASS, 0);
        std::pair<float, int> tk2 = std::make_pair(PION_MASS, 0);
        std::pair<float, int> tk3 = std::make_pair(-PION_MASS, 0);
        std::pair<float, int> tk4 = std::make_pair(-PION_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        InVec.push_back(tk3);
        InVec.push_back(tk4);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        for(unsigned int i = 0; i < PermuVec.size(); i++){
          Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, d0_mass_window, PermuVec[i], -1, -1, false, false, 6, 0);
        }
        //Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, d0_mass_window, InVec, -1, -1, false, false, 6, 1);
      }
      //////////////////////////////////////////////////////////////////////////
      // RECONSTRUCTION: K+K-(Phi)pi+
      //////////////////////////////////////////////////////////////////////////
      float dsubs_mass_window[2] = {DSUBS_MASS-0.2,DSUBS_MASS+0.2};
      if(Dchannel_[6] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(KAON_MASS, 1);
        std::pair<float, int> tk2 = std::make_pair(-KAON_MASS, 1);
        std::pair<float, int> tk3 = std::make_pair(PION_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        InVec.push_back(tk3);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        double Phi_masswindows=0.1;
        if(tktkRes_masswindowCut_>0.0001 && tktkRes_masswindowCut_<2 ) { Phi_masswindows=tktkRes_masswindowCut_;}
        //for(unsigned int i = 0; i < PermuVec.size(); i++){
        //    Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, dsubs_mass_window, PermuVec[i], PHI_MASS, 0.1, false, false, 7, 0);
        //}
        Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, dsubs_mass_window, InVec, PHI_MASS, Phi_masswindows , false, false, 7, 1);
      }
      //////////////////////////////////////////////////////////////////////////
      // RECONSTRUCTION: K+K-(Phi)pi-
      //////////////////////////////////////////////////////////////////////////
      if(Dchannel_[7] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(-KAON_MASS, 1); // original is k+,k- pi- , true for pp, HIMB567, changed to k-k+pi- to have same order of charge relation with another Ds channel
        std::pair<float, int> tk2 = std::make_pair(KAON_MASS, 1);
        std::pair<float, int> tk3 = std::make_pair(-PION_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        InVec.push_back(tk3);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        double Phi_masswindows=0.1;
        if(tktkRes_masswindowCut_>0.0001 && tktkRes_masswindowCut_<2) { Phi_masswindows=tktkRes_masswindowCut_;}
        //for(unsigned int i = 0; i < PermuVec.size(); i++){
        //    Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, dsubs_mass_window, PermuVec[i], PHI_MASS, 0.1, false, false, 8, 0);
        //}
        Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, dsubs_mass_window, InVec, PHI_MASS, Phi_masswindows, false, false, 8, 1);
      }
      //////////////////////////////////////////////////////////////////////////
      // RECONSTRUCTION: D0(K-pi+)pi+
      //////////////////////////////////////////////////////////////////////////
      float dstar_mass_window[2] = {DSTAR_MASS-0.2,DSTAR_MASS+0.2};
      if(Dchannel_[8] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(-KAON_MASS, 1);
        std::pair<float, int> tk2 = std::make_pair(PION_MASS, 1);
        std::pair<float, int> tk3 = std::make_pair(PION_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        InVec.push_back(tk3);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        //for(unsigned int i = 0; i < PermuVec.size(); i++){
        //    Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, dstar_mass_window, PermuVec[i], D0_MASS, 0.1, false, true, 9, 0);
        //}
        Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, dstar_mass_window, InVec, D0_MASS, 0.1, false, true, 9, 1);
      }
      //////////////////////////////////////////////////////////////////////////
      // RECONSTRUCTION: D0bar(K+pi-)pi-
      //////////////////////////////////////////////////////////////////////////
      if(Dchannel_[9] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(KAON_MASS, 1);
        std::pair<float, int> tk2 = std::make_pair(-PION_MASS, 1);
        std::pair<float, int> tk3 = std::make_pair(-PION_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        InVec.push_back(tk3);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        //for(unsigned int i = 0; i < PermuVec.size(); i++){
        //    Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, dstar_mass_window, PermuVec[i], D0_MASS, 0.1, false, true, 10, 0);
        //}
        Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, dstar_mass_window, InVec, D0_MASS, 0.1, false, true, 10, 1);
      }

      //////////////////////////////////////////////////////////////////////////
      // RECONSTRUCTION: D0(K-pi-pi+pi+)pi+
      //////////////////////////////////////////////////////////////////////////
      if(Dchannel_[10] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(-KAON_MASS, 1);
        std::pair<float, int> tk2 = std::make_pair(-PION_MASS, 1);
        std::pair<float, int> tk3 = std::make_pair(PION_MASS, 1);
        std::pair<float, int> tk4 = std::make_pair(PION_MASS, 1);
        std::pair<float, int> tk5 = std::make_pair(PION_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        InVec.push_back(tk3);
        InVec.push_back(tk4);
        InVec.push_back(tk5);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        //for(unsigned int i = 0; i < PermuVec.size(); i++){
        //    Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, dstar_mass_window, PermuVec[i], D0_MASS, 0.1, false, true, 11, 0);
        //}
        Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, dstar_mass_window, InVec, D0_MASS, 0.1, false, true, 11, 1);
      }

      //////////////////////////////////////////////////////////////////////////
      // RECONSTRUCTION: D0bar(K+pi+pi-pi-)pi-
      //////////////////////////////////////////////////////////////////////////
      if(Dchannel_[11] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(KAON_MASS, 1);
        std::pair<float, int> tk2 = std::make_pair(PION_MASS, 1);
        std::pair<float, int> tk3 = std::make_pair(-PION_MASS, 1);
        std::pair<float, int> tk4 = std::make_pair(-PION_MASS, 1);
        std::pair<float, int> tk5 = std::make_pair(-PION_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        InVec.push_back(tk3);
        InVec.push_back(tk4);
        InVec.push_back(tk5);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        //for(unsigned int i = 0; i < PermuVec.size(); i++){
        //    Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, dstar_mass_window, PermuVec[i], D0_MASS, 0.1, false, true, 12, 0);
        //}
        Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, dstar_mass_window, InVec, D0_MASS, 0.1, false, true, 12, 1);
      }
      //////////////////////////////////////////////////////////////////////////
      // RECONSTRUCTION: D0bar(K+pi-)pi+
      //////////////////////////////////////////////////////////////////////////
      //float bplus_mass_window[2] = {BPLUS_MASS-0.2,BPLUS_MASS+0.2};
      //                    float bplus_mass_window[2] = {4.5, 6.5};
      float bplus_mass_window[2] = {4.9, 6.1};
      //                    float bplus_mass_window[2] = {4.9, 5.6};
      if(Dchannel_[12] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(KAON_MASS, 1);
        std::pair<float, int> tk2 = std::make_pair(-PION_MASS, 1);
        std::pair<float, int> tk3 = std::make_pair(PION_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        InVec.push_back(tk3);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, bplus_mass_window, InVec, D0_MASS, 0.04, false, true, 13, 1);
      }
      //////////////////////////////////////////////////////////////////////////
      // RECONSTRUCTION: D0(K-pi+)pi-
      //////////////////////////////////////////////////////////////////////////
      if(Dchannel_[13] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(-KAON_MASS, 1);
        std::pair<float, int> tk2 = std::make_pair(PION_MASS, 1);
        std::pair<float, int> tk3 = std::make_pair(-PION_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        InVec.push_back(tk3);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, bplus_mass_window, InVec, D0_MASS, 0.04, false, true, 14, 1);
      }

      ///////////////////////////////////////////////////////////////////////////
      //RECONSTRUCTION: pi+p+k-(for lambda_C)
      ///////////////////////////////////////////////////////////////////////////
      float lambdaC_mass_window[2] = {LAMBDAC_MASS-0.3,LAMBDAC_MASS+0.3};
      if(Dchannel_[14] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(+PION_MASS, 0);
        std::pair<float, int> tk2 = std::make_pair(+PROTON_MASS, 0);
        std::pair<float, int> tk3 = std::make_pair(-KAON_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        InVec.push_back(tk3);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        for(unsigned int i = 0; i < PermuVec.size(); i++){
          Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, lambdaC_mass_window, PermuVec[i], -1, -1, false, false, 15, 0);
        }    
      }    
      ///////////////////////////////////////////////////////////////////////////
      //RECONSTRUCTION: pi-pbar-k+(for anti_lambda_C)
      ///////////////////////////////////////////////////////////////////////////
      if(Dchannel_[15] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(-PION_MASS, 0);
        std::pair<float, int> tk2 = std::make_pair(-PROTON_MASS, 0);
        std::pair<float, int> tk3 = std::make_pair(+KAON_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        InVec.push_back(tk3);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        for(unsigned int i = 0; i < PermuVec.size(); i++){
          Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, lambdaC_mass_window, PermuVec[i], -1, -1, false, false, 16, 0);
        }    
      }    
      //////////////////////////////////////////////////////////////////////////
      // RECONSTRUCTION: pi+pi-(Kshort)p+ (for lambda_C)
      //////////////////////////////////////////////////////////////////////////
      if(Dchannel_[16] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(PION_MASS, 1);
        std::pair<float, int> tk2 = std::make_pair(-PION_MASS, 1);
        std::pair<float, int> tk3 = std::make_pair(PROTON_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        InVec.push_back(tk3);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        double Ks_masswindows = 0.1;
        if(tktkRes_masswindowCut_ > 0.0001 && tktkRes_masswindowCut_ < 2) { Ks_masswindows = tktkRes_masswindowCut_; }
        Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, lambdaC_mass_window, InVec, KSHORT_MASS, Ks_masswindows , false, true, 17, 1);
      }
      //////////////////////////////////////////////////////////////////////////
      // RECONSTRUCTION: pi+pi-(Kshort)pbar- (for anti-lambda_C)
      //////////////////////////////////////////////////////////////////////////
      if(Dchannel_[17] == 1){
        std::vector< std::vector< std::pair<float, int> > > PermuVec;
        std::vector< std::pair<float, int> > InVec;
        std::pair<float, int> tk1 = std::make_pair(-PION_MASS, 1);
        std::pair<float, int> tk2 = std::make_pair(PION_MASS, 1);
        std::pair<float, int> tk3 = std::make_pair(-PROTON_MASS, 0);
        InVec.push_back(tk1);
        InVec.push_back(tk2);
        InVec.push_back(tk3);
        PermuVec = GetPermu(InVec);
        PermuVec = DelDuplicate(PermuVec);
        double Ks_masswindows = 0.1;
        if(tktkRes_masswindowCut_ > 0.0001 && tktkRes_masswindowCut_ < 2) { Ks_masswindows = tktkRes_masswindowCut_; }
        Dfinder::BranchOutNTk( DInfo, input_tracks, thePrimaryV, isNeededTrackIdx, D_counter, lambdaC_mass_window, InVec, KSHORT_MASS, Ks_masswindows, false, true, 18, 1);
      }


      if(printInfo_){
        printf("D_counter: ");
        for(unsigned int i = 0; i < Dchannel_.size(); i++){
          printf("%d/", D_counter[i]);
        }
        printf("\n");
      }//}}}
      //printf("-----*****DEBUG:End of DInfo.\n");

      for(auto tk_it_it=input_tracks.begin();
          tk_it_it != input_tracks.end() ; tk_it_it++){
        auto tk_it = (*tk_it_it);
        int tk_hindex = int(tk_it_it - input_tracks.begin());
        if(tk_hindex>=int(isNeededTrack.size())) break;
        if (isNeededTrack[tk_hindex]==false) continue;

        //Create list of relative xb candidates for later filling
        std::vector<int> listOfRelativeDCand1;//1~nXb
        std::vector<int> listOfRelativeDCand2;//1~nXb
        std::vector<int> listOfRelativeDCand3;//1~nXb
        std::vector<int> listOfRelativeDCand4;//1~nXb
        std::vector<int> listOfRelativeDCand5;//1~nXb
        std::vector<int> listOfRelativeDResCand1;//1~nXb
        std::vector<int> listOfRelativeDResCand2;//1~nXb
        std::vector<int> listOfRelativeDResCand3;//1~nXb
        std::vector<int> listOfRelativeDResCand4;//1~nXb
        for(int d=0; d < DInfo.size; d++){
          if(DInfo.rftk1_index[d] == tk_hindex){
            listOfRelativeDCand1.push_back(d+1);
          }
          if(DInfo.rftk2_index[d] == tk_hindex){
            listOfRelativeDCand2.push_back(d+1);
          }
          if(DInfo.rftk3_index[d] == tk_hindex){
            listOfRelativeDCand3.push_back(d+1);
          }
          if(DInfo.rftk4_index[d] == tk_hindex){
            listOfRelativeDCand4.push_back(d+1);
          }
          if(DInfo.rftk5_index[d] == tk_hindex){
            listOfRelativeDCand5.push_back(d+1);
          }

          if(DInfo.tktkRes_rftk1_index[d] == tk_hindex){
            listOfRelativeDResCand1.push_back(d+1);
          }
          if(DInfo.tktkRes_rftk2_index[d] == tk_hindex){
            listOfRelativeDResCand2.push_back(d+1);
          }
          if(DInfo.tktkRes_rftk3_index[d] == tk_hindex){
            listOfRelativeDResCand3.push_back(d+1);
          }
          if(DInfo.tktkRes_rftk4_index[d] == tk_hindex){
            listOfRelativeDResCand4.push_back(d+1);
          }
        }

        // drop unused tracks
        if(dropUnusedTracks_ && listOfRelativeDCand1.size() == 0 && listOfRelativeDCand2.size() == 0 && listOfRelativeDCand3.size() == 0 && listOfRelativeDCand4.size() == 0 && listOfRelativeDCand5.size() == 0 &&
           listOfRelativeDResCand1.size() == 0 && listOfRelativeDResCand2.size() == 0 && listOfRelativeDResCand3.size() == 0 && listOfRelativeDResCand4.size() == 0) continue;

                    
        TrackInfo.index          [TrackInfo.size] = TrackInfo.size;
        TrackInfo.handle_index   [TrackInfo.size] = tk_hindex;
        TrackInfo.charge         [TrackInfo.size] = tk_it->charge();
        TrackInfo.pt             [TrackInfo.size] = tk_it->pt();
        TrackInfo.eta            [TrackInfo.size] = tk_it->eta();
        TrackInfo.phi            [TrackInfo.size] = tk_it->phi();
        TrackInfo.ptErr          [TrackInfo.size] = tk_it->/*pseudoTrack().*/ptError();
        TrackInfo.etaErr         [TrackInfo.size] = tk_it->/*pseudoTrack().*/etaError();
        TrackInfo.phiErr         [TrackInfo.size] = tk_it->/*pseudoTrack().*/phiError();
        // TrackInfo.p              [TrackInfo.size] = tk_it->p();
        TrackInfo.striphit       [TrackInfo.size] = tk_it->/*pseudoTrack().*/hitPattern().numberOfValidStripHits();
        TrackInfo.pixelhit       [TrackInfo.size] = tk_it->/*pseudoTrack().*/hitPattern().numberOfValidPixelHits();
        TrackInfo.nStripLayer    [TrackInfo.size] = tk_it->/*pseudoTrack().*/hitPattern().stripLayersWithMeasurement();
        TrackInfo.nPixelLayer    [TrackInfo.size] = tk_it->/*pseudoTrack().*/hitPattern().pixelLayersWithMeasurement();
        TrackInfo.fpbarrelhit    [TrackInfo.size] = tk_it->/*pseudoTrack().*/hitPattern().hasValidHitInPixelLayer(PixelSubdetector::PixelBarrel,1);
        TrackInfo.fpendcaphit    [TrackInfo.size] = tk_it->/*pseudoTrack().*/hitPattern().hasValidHitInPixelLayer(PixelSubdetector::PixelEndcap,1);
        TrackInfo.chi2           [TrackInfo.size] = tk_it->/*pseudoTrack().*/chi2();            
        if (chi2Handle.isValid() && !chi2Handle.failedToGet())
          TrackInfo.chi2           [TrackInfo.size] = (float)((*chi2Handle)[ tks->ptrAt( tk_hindex ) ]) * tk_it->/*pseudoTrack().*/ndof();
        TrackInfo.ndf            [TrackInfo.size] = tk_it->/*pseudoTrack().*/ndof();
        TrackInfo.d0             [TrackInfo.size] = tk_it->/*pseudoTrack().*/d0();
        TrackInfo.d0error        [TrackInfo.size] = tk_it->/*pseudoTrack().*/d0Error();
        TrackInfo.dz             [TrackInfo.size] = tk_it->/*pseudoTrack().*/dz();
        TrackInfo.dzerror        [TrackInfo.size] = tk_it->/*pseudoTrack().*/dzError();
        TrackInfo.dxy            [TrackInfo.size] = tk_it->/*pseudoTrack().*/dxy();
        TrackInfo.dxyerror       [TrackInfo.size] = tk_it->/*pseudoTrack().*/dxyError();
        TrackInfo.dxy1           [TrackInfo.size] = tk_it->/*pseudoTrack().*/dxy(RefVtx);
        TrackInfo.dxyerror1      [TrackInfo.size] = TMath::Sqrt(tk_it->/*pseudoTrack().*/dxyError()*tk_it->/*pseudoTrack().*/dxyError() + thePrimaryV.xError()*thePrimaryV.yError());
        TrackInfo.dz1            [TrackInfo.size] = tk_it->/*pseudoTrack().*/dz(RefVtx);
        TrackInfo.dzerror1       [TrackInfo.size] = TMath::Sqrt(tk_it->/*pseudoTrack().*/dzError()*tk_it->/*pseudoTrack().*/dzError() + thePrimaryV.zError()*thePrimaryV.zError());
        TrackInfo.highPurity     [TrackInfo.size] = tk_it->/*pseudoTrack().*/quality(reco::TrackBase::qualityByName("highPurity"));
        TrackInfo.geninfo_index  [TrackInfo.size] = -1; // initialize for later use
        TrackInfo.geninfo_pdgId  [TrackInfo.size] = -1; // initialize for later use
        // TrackInfo.trkMVAVal      [TrackInfo.size] = (*mvaoutput)[tk_it->track()];
        TrackInfo.trkAlgo        [TrackInfo.size] = tk_it->/*pseudoTrack().*/algo();
        TrackInfo.originalTrkAlgo[TrackInfo.size] = tk_it->/*pseudoTrack().*/originalAlgo();
        TrackInfo.dedx           [TrackInfo.size] = -1;
        if (dedxHandle.isValid() && !dedxHandle.failedToGet())
          TrackInfo.dedx           [TrackInfo.size] = ((*dedxHandle)[ tks->ptrAt( tk_hindex ) ]).dEdx();

        // Gen-match
        // https://github.com/cms-sw/cmssw/blob/CMSSW_11_2_X/CommonTools/UtilAlgos/interface/MatchByDRDPt.h
        if (!iEvent.isRealData())
          {
            // std::cout<<tk_it->genParticle()<<std::endl;
            genTrackPtr[TrackInfo.size] = -1;
            float currentdR = 1.e+10;//, currentptRel=0.;
            for(std::vector<reco::GenParticle>::const_iterator it_gen=gens->begin();
                it_gen != gens->end(); it_gen++)
              {
                int abspdg = abs(int(it_gen->pdgId()));
                if (it_gen->status() != 1) continue;
                if (abspdg != 211 && 
                    abspdg != 321 && 
                    abspdg != 2212) continue;
                if (tk_it->charge() != it_gen->charge()) continue;
                float ptRel = fabs(tk_it->pt() - it_gen->pt()) / tk_it->pt();
                if(ptRel >= 0.2) continue; //
                float deta = tk_it->eta() - it_gen->eta();
                float dphi = std::abs(tk_it->phi() - it_gen->phi());
                if(dphi > float(M_PI))
                  dphi -= float(2 * M_PI);
                float dR = sqrt(deta*deta + dphi*dphi);
                if(dR >= 0.02) continue; //
                if(dR < currentdR) {
                  genTrackPtr[TrackInfo.size] = it_gen - gens->begin();
                  currentdR = dR;
                }
              }
            // genTrackPtr[TrackInfo.size] = tk_it->genParticle();
          }
        // <--------------

        // Fill the same list for DInfo // original idx is Handle_idx changed to the TrackInfo idx 
        for(unsigned int iCands=0; iCands < listOfRelativeDCand1.size(); iCands++){
          DInfo.rftk1_index[listOfRelativeDCand1[iCands]-1] = TrackInfo.size;
        }
        for(unsigned int iCands=0; iCands < listOfRelativeDCand2.size(); iCands++){
          DInfo.rftk2_index[listOfRelativeDCand2[iCands]-1] = TrackInfo.size;
        }
        for(unsigned int iCands=0; iCands < listOfRelativeDCand3.size(); iCands++){
          DInfo.rftk3_index[listOfRelativeDCand3[iCands]-1] = TrackInfo.size;
        }
        for(unsigned int iCands=0; iCands < listOfRelativeDCand4.size(); iCands++){
          DInfo.rftk4_index[listOfRelativeDCand4[iCands]-1] = TrackInfo.size;
        }
        for(unsigned int iCands=0; iCands < listOfRelativeDCand5.size(); iCands++){
          DInfo.rftk5_index[listOfRelativeDCand5[iCands]-1] = TrackInfo.size;
        }

        for(unsigned int iCands=0; iCands < listOfRelativeDResCand1.size(); iCands++){
          DInfo.tktkRes_rftk1_index[listOfRelativeDResCand1[iCands]-1] = TrackInfo.size;
        }
        for(unsigned int iCands=0; iCands < listOfRelativeDResCand2.size(); iCands++){
          DInfo.tktkRes_rftk2_index[listOfRelativeDResCand2[iCands]-1] = TrackInfo.size;
        }
        for(unsigned int iCands=0; iCands < listOfRelativeDResCand3.size(); iCands++){
          DInfo.tktkRes_rftk3_index[listOfRelativeDResCand3[iCands]-1] = TrackInfo.size;
        }
        for(unsigned int iCands=0; iCands < listOfRelativeDResCand4.size(); iCands++){
          DInfo.tktkRes_rftk4_index[listOfRelativeDResCand4[iCands]-1] = TrackInfo.size;
        }
        TrackInfo.size++;
      }//end of TrackInfo}}}
      //printf("-----*****DEBUG:End of TrackInfo.\n");
    }//has nTracks>1
    else {
      if (printInfo_) std::cout << "There's no track: " << iEvent.id() << std::endl;
    }
        
    // GenInfo section{{{
    if (!iEvent.isRealData()) {

      std::vector<const reco::Candidate *> sel_cands;

      //fill gen PV, parton, gluon, etc can also be used
      GenInfo.genPVx = -99;
      GenInfo.genPVy = -99;
      GenInfo.genPVz = -99;
      for(std::vector<reco::GenParticle>::const_iterator it_gen=gens->begin();
          it_gen != gens->end(); it_gen++){

        //pt 0 particle should be from beam. Apart from proton and neutron, they are also pointing to PV actually
        if( !(it_gen->pt() > 0) ) continue;

        //take the vertex of first produced particle, should be enough. Better to check 2-3 particles if want
        GenInfo.genPVx = it_gen->vx();
        GenInfo.genPVy = it_gen->vy();
        GenInfo.genPVz = it_gen->vz();
        break;
      }
      //end fill gen PV

      for(auto it_gen=gens->begin(); // std::vector<reco::GenParticle>::const_iterator
          it_gen != gens->end(); it_gen++){
        if (it_gen->status() > 2 && it_gen->status() != 8) continue;//only status 1(final state), 2(decayed), 8(simulated)
        if(GenInfo.size >= MAX_GEN){
          fprintf(stderr,"ERROR: number of gens exceeds the size of array.\n");
          break;;
        }

        bool isGenSignal = false;
        //save target intermediate state particle
        if (
            abs(int(it_gen->pdgId()/100) % 100) == 4  ||//c menson
            abs(int(it_gen->pdgId()/100) % 100) == 5  ||//b menson
            abs(it_gen->pdgId()) == 511 ||//B_0
            abs(it_gen->pdgId()) == 521 ||//B_+-
            abs(it_gen->pdgId()) == 531 ||//B_s
            abs(it_gen->pdgId()) == 130 ||//KL
            abs(it_gen->pdgId()) == 4122 ||//lamadac
            //abs(it_gen->pdgId()) == 311 ||//K0
            // abs(it_gen->pdgId()) == 321 ||//K+
            // abs(it_gen->pdgId()) == 310 ||//KS
            //abs(it_gen->pdgId()) == 313 ||//K*0(892)
            //abs(it_gen->pdgId()) == 323 ||//K*+-(892)
            //abs(it_gen->pdgId()) == 333 ||//phi(1020)  //# might need to add this
            it_gen->pdgId() == 443      ||//Jpsi
            it_gen->pdgId() == 100443   ||//Psi(2S)
            it_gen->pdgId() == 553      ||//Upsilon
            it_gen->pdgId() == 100553     //Upsilon(2S)
            || abs(it_gen->pdgId())   == 9010221 //f0(980) // intermediate state for Ds kkpi
            || abs(it_gen->pdgId())   == 10221 //f0(1370) 
            || abs(it_gen->pdgId())   == 10331 //f0(1710) 
            || abs(it_gen->pdgId())   == 10331 //f0(1710) 
            || abs(it_gen->pdgId())   == 10311 //K*1430 neutral 
            
            //# check here by rui
            ) isGenSignal = true;//b, c, s mesons

        if (abs(it_gen->pdgId()) == 13) isGenSignal = true;//all mu

        if (
            abs(int(it_gen->pdgId()/100) % 100) == 3  ||//s menson  //# ex k+k-, also phi
            abs(it_gen->pdgId()) == 111 || //pi0
            abs(it_gen->pdgId()) == 211 ||//pi+
            abs(it_gen->pdgId()) == 2212  ||//proton
            abs(it_gen->pdgId()) == 321 || //K+ actually, I DO NOT need to add this,s meson is in the list
            abs(it_gen->pdgId()) == 3124 || //lambda(1520)0
            abs(it_gen->pdgId()) == 2224 || //delta++
            abs(it_gen->pdgId()) == 22
            //# check here by Rui
            ) {
          reco::GenParticle _deRef = (*it_gen);
          reco::Candidate* Myself = dynamic_cast<reco::Candidate*>(&_deRef);
          //std::cout<<Myself->pdgId()<<"-----------"<<std::endl;
          isGenSignal = (Functs.GetAncestor(Myself, 5) | Functs.GetAncestor(Myself, 4)| Functs.GetAncestor(Myself, 41));
          //isGenSignal = (Functs.GetAncestor(Myself, 41));
        }//all pi and K from b or c meson or c bayron //need to check here.!!!

        if( !isGenSignal && abs(it_gen->pdgId()) > 400 ){ //should be OK to require the PID > 400
          reco::GenParticle _deRef = (*it_gen);
          reco::Candidate* Myself = dynamic_cast<reco::Candidate*>(&_deRef);
          isGenSignal = Functs.GetDescendant(Myself, 4);
        }//other particles (with pid > 400) have D meson in descendant chain

        if (!isGenSignal) continue;

        for(int trackIdx = 0; trackIdx < TrackInfo.size; trackIdx++){ //# saving gen-track ref index 
          // hope for match by pat::GenericParticle
          if (genTrackPtr[trackIdx] < 0 ) continue;
          if (int(it_gen - gens->begin()) == genTrackPtr[trackIdx]) {
            TrackInfo.geninfo_index[trackIdx] = GenInfo.size;
            TrackInfo.geninfo_pdgId[trackIdx] = it_gen->pdgId();
            // break;
          }
        }

        GenInfo.index[GenInfo.size]         = GenInfo.size;
        GenInfo.handle_index[GenInfo.size]  = it_gen-gens->begin();
        GenInfo.pt[GenInfo.size]            = it_gen->pt();
        GenInfo.eta[GenInfo.size]           = it_gen->eta();
        GenInfo.phi[GenInfo.size]           = it_gen->phi();
        GenInfo.mass[GenInfo.size]          = it_gen->mass();
        GenInfo.pdgId[GenInfo.size]         = it_gen->pdgId();
        GenInfo.status[GenInfo.size]        = it_gen->status();
        GenInfo.collisionId[GenInfo.size]   = it_gen->collisionId();
        // GenInfo.collisionId[GenInfo.size]   = -1;
        GenInfo.nMo[GenInfo.size]           = it_gen->numberOfMothers();
        GenInfo.nDa[GenInfo.size]           = it_gen->numberOfDaughters();
        GenInfo.vtxX[GenInfo.size]          = it_gen->vx(); //it should be the production vx of the particle, better to double check
        GenInfo.vtxY[GenInfo.size]          = it_gen->vy();
        GenInfo.vtxZ[GenInfo.size]          = it_gen->vz();
        GenInfo.mo1[GenInfo.size]           = -1;//To be matched later.
        GenInfo.mo2[GenInfo.size]           = -1;
        GenInfo.da1[GenInfo.size]           = -1;
        GenInfo.da2[GenInfo.size]           = -1;
        GenInfo.da3[GenInfo.size]           = -1;
        GenInfo.da4[GenInfo.size]           = -1;
        GenInfo.size++;
        sel_cands.push_back(&*it_gen);
      }
      //printf("-----*****DEBUG:End of gens loop.\n");

      int geninfo_idx = 0;
      for(std::vector<const reco::Candidate *>::iterator sCands = sel_cands.begin();
          sCands != sel_cands.end(); sCands++){
        geninfo_idx = int(sCands-sel_cands.begin());
        for(int nGenMo = 0; nGenMo < std::min(2,int((*sCands)->numberOfMothers())); nGenMo++){
          //if((*sCands)->numberOfMothers()==1){
          for(std::vector<const reco::Candidate *>::iterator mCands = sel_cands.begin();
              mCands != sel_cands.end(); mCands++){
            if((*sCands)->mother(nGenMo) == *mCands){
              //if((*sCands)->mother(0) == *mCands){
              if(nGenMo == 0) GenInfo.mo1[geninfo_idx] = int(mCands-sel_cands.begin());
              if(nGenMo == 1) GenInfo.mo2[geninfo_idx] = int(mCands-sel_cands.begin());
            }
          }
        }
        for(int nGenDa = 0; nGenDa < std::min(4,int((*sCands)->numberOfDaughters())); nGenDa++){
          for(std::vector<const reco::Candidate *>::iterator mCands = sel_cands.begin();
              mCands != sel_cands.end(); mCands++){
            if((*sCands)->daughter(nGenDa) == *mCands){
              if(nGenDa == 0) GenInfo.da1[geninfo_idx] = int(mCands-sel_cands.begin());
              if(nGenDa == 1) GenInfo.da2[geninfo_idx] = int(mCands-sel_cands.begin());
              if(nGenDa == 2) GenInfo.da3[geninfo_idx] = int(mCands-sel_cands.begin());
              if(nGenDa == 3) GenInfo.da4[geninfo_idx] = int(mCands-sel_cands.begin());
            }
          }
        }
      }
      //printf("-----*****DEBUG:End of IndexToIgen\n");
    }//isRealData}}}
    //printf("-----*****DEBUG:End of GenInfo.\n");
    //std::cout<<"Start to fill!\n";

  } // try
  catch (std::exception & err) {
    std::cout  << "Exception during event number: " << iEvent.id()
               << "\n" << err.what() << "\n";
  } // catch 
  root->Fill();
  //std::cout<<"filled!\n";

  //Made a Dntuple on the fly   
  if(makeDntuple_){
    int isDchannel[18];
    for(int ichannel=0; ichannel<18; ichannel++)
      { isDchannel[ichannel] = Dchannel_[ichannel]; }
    bool REAL = iEvent.isRealData();
    bool fillZeroCandEvt = true;
    int Dtypesize[9]={0,0,0,0,0,0,0,0,0};
    Dntuple->makeDNtuple(isDchannel, Dtypesize, REAL, fillZeroCandEvt, doDntupleSkim_, &EvtInfo, &VtxInfo, &TrackInfo, &DInfo, &GenInfo, ntD1, ntD2, ntD3, ntD4, ntD5, ntD6, ntD7, ntD8, ntD9);
    if(!REAL) Dntuple->fillDGenTree(ntGen, &GenInfo);
  }

}

// ------------ method called once each job just after ending the event loop  ------------{{{
void Dfinder::endJob()
{
  delete Dntuple;
}

// ------------ method called when starting to processes a run  ------------
void Dfinder::beginRun(edm::Run const&, edm::EventSetup const&)
{
}

// ------------ method called when ending the processing of a run  ------------
void Dfinder::endRun(edm::Run const&, edm::EventSetup const&)
{
}

// ------------ method called when starting to processes a luminosity block  ------------
void Dfinder::beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}

// ------------ method called when ending the processing of a luminosity block  ------------
void Dfinder::endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void Dfinder::fillDescriptions(edm::ConfigurationDescriptions& descriptions)
{
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}//}}}

//Functions{{{
std::vector< std::vector< std::pair<float, int> > > Dfinder::GetPermu(std::vector< std::pair<float, int> > InVec){
  if(InVec.size() == 1){
    std::vector< std::vector< std::pair<float, int> > > OneEntryVec;
    OneEntryVec.push_back(InVec);
    return OneEntryVec;
  }
  std::vector< std::vector< std::pair<float, int> > > NPermu;
  for(unsigned int i = 0; i < InVec.size(); i++){
    std::vector< std::pair<float, int> > copy;
    copy = InVec;
    copy.erase(copy.begin()+i);
    std::vector< std::vector< std::pair<float, int> > > Nminus1Permu;
    Nminus1Permu = GetPermu(copy);
    for(unsigned int j = 0; j < Nminus1Permu.size(); j++){
      Nminus1Permu[j].push_back(InVec[i]);
    }
    NPermu.insert(NPermu.end(), Nminus1Permu.begin(), Nminus1Permu.end());
  }
  return NPermu;
}

std::vector< std::vector< std::pair<float, int> > > Dfinder::DelDuplicate(std::vector< std::vector< std::pair<float, int> > > InVec){
  std::vector< std::vector< std::pair<float, int> > > CleanedVec;
  for(unsigned int i = 0; i < InVec.size(); i++){
    bool IsDuplicate = false;
    for(unsigned int j = 0; j < CleanedVec.size(); j++){
      bool ADuplicate = true;
      for(unsigned int k = 0; k < InVec[i].size(); k++){
        if(InVec[i][k] != CleanedVec[j][k]) {
          ADuplicate = false;
          break;
        }
      }
      if(ADuplicate) {
        IsDuplicate = ADuplicate;
        break;
      }
    }
    if(!IsDuplicate) CleanedVec.push_back(InVec[i]);
  }
  return CleanedVec;
}

void Dfinder::TkCombinationPermutation(
                                       reco::Vertex thePrimaryV,
                                       // edm::View<pat::PackedCandidate> input_tracks, 
                                       std::vector<const reco::Track*> input_tracks, 
                                       std::vector<int> isNeededTrackIdx,
                                       float *mass_window,
                                       std::vector< std::pair<float, int> > TkMassCharge,
                                       double tktkRes_mass,
                                       double tktkRes_mass_window,
                                       std::vector< std::vector<int> > &selectedTkhidxSet,
                                       int Dchannel_number
                                       ){
  int tk1_hindex = -1;
  int tk2_hindex = -1;
  int tk3_hindex = -1;
  int tk4_hindex = -1;
  int tk5_hindex = -1;

  std::vector<int> selectedTkhidx;
  TLorentzVector v4_D, v4_Res, v4_NonRes;//unfitted D, resonance, non-resonance track 4 vectors sum 
  TLorentzVector v4_tk1, v4_tk2, v4_tk3, v4_tk4, v4_tk5;// all tracks
  TLorentzVector v4_Restk1, v4_Restk2, v4_Restk3, v4_Restk4, v4_Restk5;// resonance tracks
  TLorentzVector v4_NonRestk1, v4_NonRestk2, v4_NonRestk3, v4_NonRestk4, v4_NonRestk5;// non-resonance tracks, i.e., all - resonance tracks
  //for(edm::View<pat::PackedCandidate>::const_iterator tk_it1=input_tracks.begin();
  //        tk_it1 != input_tracks.end() ; tk_it1++){
  for(int tk1idx = 0; tk1idx < (int)isNeededTrackIdx.size(); tk1idx++){
    v4_D.Clear(); v4_Res.Clear();
    //tk1_hindex = int(tk_it1 - input_tracks.begin());
    //if(tk1_hindex>=int(isNeededTrack.size())) break;
    //if(!isNeededTrack[tk1_hindex]) continue;
    //if(tk_it1->charge()*TkMassCharge[0].first<0) continue;
    tk1_hindex = isNeededTrackIdx[tk1idx];
    if(input_tracks[tk1_hindex]->charge()*TkMassCharge[0].first<0) continue;
    v4_tk1.SetXYZM(input_tracks[tk1_hindex]->px(),input_tracks[tk1_hindex]->py(),input_tracks[tk1_hindex]->pz(),fabs(TkMassCharge[0].first));
    v4_D.SetPxPyPzE((v4_tk1.Px()), (v4_tk1.Py()), (v4_tk1.Pz()), (v4_tk1.E()));
    if(TkMassCharge[0].second==1){
      v4_Restk1.SetXYZM(input_tracks[tk1_hindex]->px(),input_tracks[tk1_hindex]->py(),input_tracks[tk1_hindex]->pz(),fabs(TkMassCharge[0].first));
      v4_NonRestk1.SetXYZM(0,0,0,0);
    }
    else{    
      v4_Restk1.SetXYZM(0,0,0,0);
      v4_NonRestk1.SetXYZM(input_tracks[tk1_hindex]->px(),input_tracks[tk1_hindex]->py(),input_tracks[tk1_hindex]->pz(),fabs(TkMassCharge[0].first));
    }
    v4_Res.SetPxPyPzE(v4_Restk1.Px(), v4_Restk1.Py(), v4_Restk1.Pz(), v4_Restk1.E());
    v4_NonRes.SetPxPyPzE(v4_NonRestk1.Px(), v4_NonRestk1.Py(), v4_NonRestk1.Pz(), v4_NonRestk1.E());

    //for(edm::View<pat::PackedCandidate>::const_iterator tk_it2=tk_it1+1;
    //        tk_it2 != input_tracks.end() ; tk_it2++){
    for(int tk2idx = tk1idx+1; tk2idx < (int)isNeededTrackIdx.size(); tk2idx++){
      //tk2_hindex = int(tk_it2 - input_tracks.begin());
      //if(tk2_hindex>=int(isNeededTrack.size())) break;
      //if(!isNeededTrack[tk2_hindex]) continue;
      //if(tk_it2->charge()*TkMassCharge[1].first<0) continue;
      tk2_hindex = isNeededTrackIdx[tk2idx];
      if(input_tracks[tk2_hindex]->charge()*TkMassCharge[1].first<0) continue;
      if(tk2_hindex==tk1_hindex) continue;
      v4_tk2.SetXYZM(input_tracks[tk2_hindex]->px(),input_tracks[tk2_hindex]->py(),input_tracks[tk2_hindex]->pz(),fabs(TkMassCharge[1].first));
      v4_D.SetPxPyPzE((v4_tk1.Px() + v4_tk2.Px()), (v4_tk1.Py() + v4_tk2.Py()), (v4_tk1.Pz() + v4_tk2.Pz()), (v4_tk1.E() + v4_tk2.E()));
      if(TkMassCharge[1].second==1){
        v4_Restk2.SetXYZM(input_tracks[tk2_hindex]->px(),input_tracks[tk2_hindex]->py(),input_tracks[tk2_hindex]->pz(),fabs(TkMassCharge[1].first));
        v4_NonRestk2.SetXYZM(0,0,0,0);
      }
      else{ 
        v4_Restk2.SetXYZM(0,0,0,0);
        v4_NonRestk2.SetXYZM(input_tracks[tk2_hindex]->px(),input_tracks[tk2_hindex]->py(),input_tracks[tk2_hindex]->pz(),fabs(TkMassCharge[1].first));
      }
      v4_Res.SetPxPyPzE((v4_Restk1.Px() + v4_Restk2.Px()), (v4_Restk1.Py() + v4_Restk2.Py()), (v4_Restk1.Pz() + v4_Restk2.Pz()), (v4_Restk1.E() + v4_Restk2.E()));
      v4_NonRes.SetPxPyPzE((v4_NonRestk1.Px() + v4_NonRestk2.Px()), (v4_NonRestk1.Py() + v4_NonRestk2.Py()), (v4_NonRestk1.Pz() + v4_NonRestk2.Pz()), (v4_NonRestk1.E() + v4_NonRestk2.E()));
      if(TkMassCharge.size()==2){
        //cut mass window before fit
        if(tktkRes_mass > 0) {
          if (fabs(v4_Res.Mag()-tktkRes_mass) > tktkRes_mass_window) continue;
          if (v4_Res.Pt() < tktkRes_dPtCut_[Dchannel_number-1]) continue;
          //if (fabs(v4_Res.Eta()) > tktkRes_dEtaCut_[Dchannel_number-1]) continue;
        }
        if(v4_D.Mag()<mass_window[0] || v4_D.Mag()>mass_window[1]) continue;
        if(v4_D.Pt() < dPtCut_[Dchannel_number-1])continue;
        //if(fabs(v4_D.Eta()) > dEtaCut_[Dchannel_number-1])continue;
        if(fabs(v4_D.Rapidity()) > dRapidityCut_[Dchannel_number-1])continue;
        selectedTkhidx.push_back(tk1_hindex);
        selectedTkhidx.push_back(tk2_hindex);
        selectedTkhidxSet.push_back(selectedTkhidx);
        selectedTkhidx.clear();
        continue;
      }
      for(int tk3idx = tk2idx+1; tk3idx < (int)isNeededTrackIdx.size(); tk3idx++){
        tk3_hindex = isNeededTrackIdx[tk3idx];
        if(input_tracks[tk3_hindex]->charge()*TkMassCharge[2].first<0) continue;
        if(tk3_hindex==tk1_hindex) continue;
        if(tk3_hindex==tk2_hindex) continue;
        v4_tk3.SetXYZM(input_tracks[tk3_hindex]->px(),input_tracks[tk3_hindex]->py(),input_tracks[tk3_hindex]->pz(),fabs(TkMassCharge[2].first));
        v4_D = v4_tk1 + v4_tk2 + v4_tk3;
        v4_D.SetPxPyPzE((v4_tk1.Px() + v4_tk2.Px() + v4_tk3.Px()), (v4_tk1.Py() + v4_tk2.Py() + v4_tk3.Py()), (v4_tk1.Pz() + v4_tk2.Pz() + v4_tk3.Pz()), (v4_tk1.E() + v4_tk2.E() + v4_tk3.E()));
        if(TkMassCharge[2].second==1){ 
          v4_Restk3.SetXYZM(input_tracks[tk3_hindex]->px(),input_tracks[tk3_hindex]->py(),input_tracks[tk3_hindex]->pz(),fabs(TkMassCharge[2].first));
          v4_NonRestk3.SetXYZM(0,0,0,0);
        }
        else{ 
          v4_Restk3.SetXYZM(0,0,0,0);
          v4_NonRestk3.SetXYZM(input_tracks[tk3_hindex]->p(),input_tracks[tk3_hindex]->py(),input_tracks[tk3_hindex]->pz(),fabs(TkMassCharge[2].first));
        }
        v4_Res.SetPxPyPzE((v4_Restk1.Px() + v4_Restk2.Px() + v4_Restk3.Px()), (v4_Restk1.Py() + v4_Restk2.Py() + v4_Restk3.Py()), (v4_Restk1.Pz() + v4_Restk2.Pz() + v4_Restk3.Pz()), (v4_Restk1.E() + v4_Restk2.E() + v4_Restk3.E()));
        v4_NonRes.SetPxPyPzE((v4_NonRestk1.Px() + v4_NonRestk2.Px() + v4_NonRestk3.Px()), (v4_NonRestk1.Py() + v4_NonRestk2.Py() + v4_NonRestk3.Py()), (v4_NonRestk1.Pz() + v4_NonRestk2.Pz() + v4_NonRestk3.Pz()), (v4_NonRestk1.E() + v4_NonRestk2.E() + v4_NonRestk3.E()));
        if(TkMassCharge.size()==3){
          if(tktkRes_mass > 0) {
            if (fabs(v4_Res.Mag()-tktkRes_mass) > tktkRes_mass_window) continue;
            if (v4_Res.Pt() < tktkRes_dPtCut_[Dchannel_number-1]) continue;
            //if (fabs(v4_Res.Eta()) > tktkRes_dEtaCut_[Dchannel_number-1]) continue;
            if((v4_Res.Pt()-v4_NonRes.Pt())/(v4_Res.Pt()+v4_NonRes.Pt()) < ResToNonRes_PtAsym_min_[Dchannel_number-1] || (v4_Res.Pt()-v4_NonRes.Pt())/(v4_Res.Pt()+v4_NonRes.Pt()) > ResToNonRes_PtAsym_max_[Dchannel_number-1]) continue;
          }
          if(v4_D.Mag()<mass_window[0] || v4_D.Mag()>mass_window[1]) continue;
          if(v4_D.Pt() < dPtCut_[Dchannel_number-1])continue;
          //if(fabs(v4_D.Eta()) > dEtaCut_[Dchannel_number-1])continue;
          if(fabs(v4_D.Rapidity()) > dRapidityCut_[Dchannel_number-1])continue;
          selectedTkhidx.push_back(tk1_hindex);
          selectedTkhidx.push_back(tk2_hindex);
          selectedTkhidx.push_back(tk3_hindex);
          selectedTkhidxSet.push_back(selectedTkhidx);
          selectedTkhidx.clear();
          continue;
        }
        //for(edm::View<pat::PackedCandidate>::const_iterator tk_it4=tk_it3+1;
        //        tk_it4 != input_tracks.end() ; tk_it4++){
        for(int tk4idx = tk3idx+1; tk4idx < (int)isNeededTrackIdx.size(); tk4idx++){
          //tk4_hindex = int(tk_it4 - input_tracks.begin());
          //if(tk4_hindex>=int(isNeededTrack.size())) break;
          //if(!isNeededTrack[tk4_hindex]) continue;
          //if(tk_it4->charge()*TkMassCharge[3].first<0) continue;
          tk4_hindex = isNeededTrackIdx[tk4idx];
          if(input_tracks[tk4_hindex]->charge()*TkMassCharge[3].first<0) continue;
          if(tk4_hindex==tk1_hindex) continue;
          if(tk4_hindex==tk2_hindex) continue;
          if(tk4_hindex==tk3_hindex) continue;
          v4_tk4.SetXYZM(input_tracks[tk4_hindex]->px(),input_tracks[tk4_hindex]->py(),input_tracks[tk4_hindex]->pz(),fabs(TkMassCharge[3].first));
          v4_D.SetPxPyPzE((v4_tk1.Px() + v4_tk2.Px() + v4_tk3.Px() + v4_tk4.Px()), (v4_tk1.Py() + v4_tk2.Py() + v4_tk3.Py() + v4_tk4.Py()), (v4_tk1.Pz() + v4_tk2.Pz() + v4_tk3.Pz() + v4_tk4.Pz()), (v4_tk1.E() + v4_tk2.E() + v4_tk3.E() + v4_tk4.E()));
          if(TkMassCharge[3].second==1){
            v4_Restk4.SetXYZM(input_tracks[tk4_hindex]->px(),input_tracks[tk4_hindex]->py(),input_tracks[tk4_hindex]->pz(),fabs(TkMassCharge[3].first));
            v4_NonRestk4.SetXYZM(0,0,0,0);
          }
          else{ 
            v4_Restk4.SetXYZM(0,0,0,0);
            v4_NonRestk4.SetXYZM(input_tracks[tk4_hindex]->px(),input_tracks[tk4_hindex]->py(),input_tracks[tk4_hindex]->pz(),fabs(TkMassCharge[3].first));
          }
          v4_Res.SetPxPyPzE((v4_Restk1.Px() + v4_Restk2.Px() + v4_Restk3.Px() + v4_Restk4.Px()), (v4_Restk1.Py() + v4_Restk2.Py() + v4_Restk3.Py() + v4_Restk4.Py()), (v4_Restk1.Pz() + v4_Restk2.Pz() + v4_Restk3.Pz() + v4_Restk4.Pz()), (v4_Restk1.E() + v4_Restk2.E() + v4_Restk3.E() + v4_Restk4.E()));
          v4_NonRes.SetPxPyPzE((v4_NonRestk1.Px() + v4_NonRestk2.Px() + v4_NonRestk3.Px() + v4_NonRestk4.Px()), (v4_NonRestk1.Py() + v4_NonRestk2.Py() + v4_NonRestk3.Py() + v4_NonRestk4.Py()), (v4_NonRestk1.Pz() + v4_NonRestk2.Pz() + v4_NonRestk3.Pz() + v4_NonRestk4.Pz()), (v4_NonRestk1.E() + v4_NonRestk2.E() + v4_NonRestk3.E() + v4_NonRestk4.E()));
          if(TkMassCharge.size()==4){
            if(tktkRes_mass > 0) {
              if (fabs(v4_Res.Mag()-tktkRes_mass) > tktkRes_mass_window) continue;
              if (v4_Res.Pt() < tktkRes_dPtCut_[Dchannel_number-1]) continue;
              //if (fabs(v4_Res.Eta()) > tktkRes_dEtaCut_[Dchannel_number-1]) continue;
              if((v4_Res.Pt()-v4_NonRes.Pt())/(v4_Res.Pt()+v4_NonRes.Pt()) < ResToNonRes_PtAsym_min_[Dchannel_number-1] || (v4_Res.Pt()-v4_NonRes.Pt())/(v4_Res.Pt()+v4_NonRes.Pt()) > ResToNonRes_PtAsym_max_[Dchannel_number-1]) continue;
            }
            if(v4_D.Mag()<mass_window[0] || v4_D.Mag()>mass_window[1]) continue;
            if(v4_D.Pt() < dPtCut_[Dchannel_number-1])continue;
            //if(fabs(v4_D.Eta()) > dEtaCut_[Dchannel_number-1])continue;
            if(fabs(v4_D.Rapidity()) > dRapidityCut_[Dchannel_number-1])continue;
            selectedTkhidx.push_back(tk1_hindex);
            selectedTkhidx.push_back(tk2_hindex);
            selectedTkhidx.push_back(tk3_hindex);
            selectedTkhidx.push_back(tk4_hindex);
            selectedTkhidxSet.push_back(selectedTkhidx);
            selectedTkhidx.clear();
            continue;
          }
          //for(edm::View<pat::PackedCandidate>::const_iterator tk_it5=tk_it4+1;
          //        tk_it5 != input_tracks.end() ; tk_it5++){
          for(int tk5idx = tk4idx+1; tk5idx < (int)isNeededTrackIdx.size(); tk5idx++){
            //tk5_hindex = int(tk_it5 - input_tracks.begin());
            //if(tk5_hindex>=int(isNeededTrack.size())) break;
            //if(!isNeededTrack[tk5_hindex]) continue;
            //if(tk_it5->charge()*TkMassCharge[4].first<0) continue;
            tk5_hindex = isNeededTrackIdx[tk5idx];
            if(input_tracks[tk5_hindex]->charge()*TkMassCharge[4].first<0) continue;
            if(tk5_hindex==tk1_hindex) continue;
            if(tk5_hindex==tk2_hindex) continue;
            if(tk5_hindex==tk3_hindex) continue;
            if(tk5_hindex==tk4_hindex) continue;
            v4_tk5.SetXYZM(input_tracks[tk5_hindex]->px(),input_tracks[tk5_hindex]->py(),input_tracks[tk5_hindex]->pz(),fabs(TkMassCharge[4].first));
            v4_D.SetPxPyPzE((v4_tk1.Px() + v4_tk2.Px() + v4_tk3.Px() + v4_tk4.Px() + v4_tk5.Px()), (v4_tk1.Py() + v4_tk2.Py() + v4_tk3.Py() + v4_tk4.Py() + v4_tk5.Py()), (v4_tk1.Pz() + v4_tk2.Pz() + v4_tk3.Pz() + v4_tk4.Pz() + v4_tk5.Pz()), (v4_tk1.E() + v4_tk2.E() + v4_tk3.E() + v4_tk4.E() + v4_tk5.E()));
            if(TkMassCharge[4].second==1){
              v4_Restk5.SetXYZM(input_tracks[tk5_hindex]->px(),input_tracks[tk5_hindex]->py(),input_tracks[tk5_hindex]->pz(),fabs(TkMassCharge[4].first));
              v4_NonRestk5.SetXYZM(0,0,0,0);
            }
            else{ 
              v4_Restk5.SetXYZM(0,0,0,0);
              v4_NonRestk5.SetXYZM(input_tracks[tk5_hindex]->px(),input_tracks[tk5_hindex]->py(),input_tracks[tk5_hindex]->pz(),fabs(TkMassCharge[4].first));
            }
            v4_Res.SetPxPyPzE((v4_Restk1.Px() + v4_Restk2.Px() + v4_Restk3.Px() + v4_Restk4.Px() + v4_Restk5.Px()), (v4_Restk1.Py() + v4_Restk2.Py() + v4_Restk3.Py() + v4_Restk4.Py() + v4_Restk5.Py()), (v4_Restk1.Pz() + v4_Restk2.Pz() + v4_Restk3.Pz() + v4_Restk4.Pz() + v4_Restk5.Pz()), (v4_Restk1.E() + v4_Restk2.E() + v4_Restk3.E() + v4_Restk4.E() + v4_Restk5.E()));
            v4_NonRes.SetPxPyPzE((v4_NonRestk1.Px() + v4_NonRestk2.Px() + v4_NonRestk3.Px() + v4_NonRestk4.Px() + v4_NonRestk5.Px()), (v4_NonRestk1.Py() + v4_NonRestk2.Py() + v4_NonRestk3.Py() + v4_NonRestk4.Py() + v4_NonRestk5.Py()), (v4_NonRestk1.Pz() + v4_NonRestk2.Pz() + v4_NonRestk3.Pz() + v4_NonRestk4.Pz() + v4_NonRestk5.Pz()), (v4_NonRestk1.E() + v4_NonRestk2.E() + v4_NonRestk3.E() + v4_NonRestk4.E() + v4_NonRestk5.E()));
            if(TkMassCharge.size()==5){
              if(tktkRes_mass > 0) {
                if (fabs(v4_Res.Mag()-tktkRes_mass) > tktkRes_mass_window) continue;
                if (v4_Res.Pt() < tktkRes_dPtCut_[Dchannel_number-1]) continue;
                //if (fabs(v4_Res.Eta()) > tktkRes_dEtaCut_[Dchannel_number-1]) continue;
                if((v4_Res.Pt()-v4_NonRes.Pt())/(v4_Res.Pt()+v4_NonRes.Pt()) < ResToNonRes_PtAsym_min_[Dchannel_number-1] || (v4_Res.Pt()-v4_NonRes.Pt())/(v4_Res.Pt()+v4_NonRes.Pt()) > ResToNonRes_PtAsym_max_[Dchannel_number-1]) continue;
              }
              if(v4_D.Mag()<mass_window[0] || v4_D.Mag()>mass_window[1]) continue;
              if(v4_D.Pt() < dPtCut_[Dchannel_number-1])continue;
              //if(fabs(v4_D.Eta()) > dEtaCut_[Dchannel_number-1])continue;
              if(fabs(v4_D.Rapidity()) > dRapidityCut_[Dchannel_number-1])continue;
              selectedTkhidx.push_back(tk1_hindex);
              selectedTkhidx.push_back(tk2_hindex);
              selectedTkhidx.push_back(tk3_hindex);
              selectedTkhidx.push_back(tk4_hindex);
              selectedTkhidx.push_back(tk5_hindex);
              selectedTkhidxSet.push_back(selectedTkhidx);
              selectedTkhidx.clear();
              continue;
            }
          }
        }
      }
    }
  }
  // std::cout<<"TkCombinationPermutation, selectedTkhidxSet.size: "<<selectedTkhidxSet.size()<<std::endl;
  return;
}

void Dfinder::TkCombinationResFast(
                                   reco::Vertex thePrimaryV,
                                   // edm::View<pat::PackedCandidate> input_tracks, 
                                   std::vector<const reco::Track*> input_tracks, 
                                   std::vector<int> isNeededTrackIdx,
                                   float *mass_window,
                                   std::vector< std::pair<float, int> > TkMassCharge,
                                   double tktkRes_mass,
                                   double tktkRes_mass_window,
                                   std::vector< std::vector<int> > &selectedTkhidxSet,
                                   int Dchannel_number
                                   ){
  int tk1_hindex = -1;
  int tk2_hindex = -1;
  int tk3_hindex = -1;
  int tk4_hindex = -1;
  int tk5_hindex = -1;

  std::vector<int> selectedTkhidx;
  TLorentzVector v4_D, v4_Res, v4_NonRes;//unfitted D, resonance, non-resonance track 4 vectors sum 
  TLorentzVector v4_tk1, v4_tk2, v4_tk3, v4_tk4, v4_tk5;// all tracks
  TLorentzVector v4_Restk1, v4_Restk2, v4_Restk3, v4_Restk4, v4_Restk5;// resonance tracks
  TLorentzVector v4_NonRestk1, v4_NonRestk2, v4_NonRestk3, v4_NonRestk4, v4_NonRestk5;// non-resonance tracks, i.e., all - resonance tracks

  for(int tk1idx = 0; tk1idx < (int)isNeededTrackIdx.size(); tk1idx++){
    v4_D.Clear(); v4_Res.Clear();
    tk1_hindex = isNeededTrackIdx[tk1idx];
    // cout<<"\ntk1idx = "<<tk1idx<<" ,tk1_hindex"<<tk1_hindex<<endl;
    if(input_tracks[tk1_hindex]->charge()*TkMassCharge[0].first<0) continue;
    v4_tk1.SetXYZM(input_tracks[tk1_hindex]->px(),input_tracks[tk1_hindex]->py(),input_tracks[tk1_hindex]->pz(),fabs(TkMassCharge[0].first));
    v4_D.SetPxPyPzE((v4_tk1.Px()), (v4_tk1.Py()), (v4_tk1.Pz()), (v4_tk1.E()));
    if(TkMassCharge[0].second==1){
      v4_Restk1.SetXYZM(input_tracks[tk1_hindex]->px(),input_tracks[tk1_hindex]->py(),input_tracks[tk1_hindex]->pz(),fabs(TkMassCharge[0].first));
      v4_NonRestk1.SetXYZM(0,0,0,0);
    }
    else{ 
      v4_Restk1.SetXYZM(0,0,0,0);
      v4_NonRestk1.SetXYZM(input_tracks[tk1_hindex]->px(),input_tracks[tk1_hindex]->py(),input_tracks[tk1_hindex]->pz(),fabs(TkMassCharge[0].first));
    }
    v4_Res.SetPxPyPzE(v4_Restk1.Px(), v4_Restk1.Py(), v4_Restk1.Pz(), v4_Restk1.E());
    v4_NonRes.SetPxPyPzE(v4_NonRestk1.Px(), v4_NonRestk1.Py(), v4_NonRestk1.Pz(), v4_NonRestk1.E());
    for(int tk2idx = ((TkMassCharge[1]==TkMassCharge[0]) ? tk1idx+1:0); tk2idx < (int)isNeededTrackIdx.size(); tk2idx++){
      tk2_hindex = isNeededTrackIdx[tk2idx];
      // cout<<"tk2idx = "<<tk2idx<<" ,tk2_hindex"<<tk2_hindex<<endl;
      if(input_tracks[tk2_hindex]->charge()*TkMassCharge[1].first<0) continue;
      if(tk2_hindex==tk1_hindex) continue;
      v4_tk2.SetXYZM(input_tracks[tk2_hindex]->px(),input_tracks[tk2_hindex]->py(),input_tracks[tk2_hindex]->pz(),fabs(TkMassCharge[1].first));
      v4_D.SetPxPyPzE((v4_tk1.Px() + v4_tk2.Px()), (v4_tk1.Py() + v4_tk2.Py()), (v4_tk1.Pz() + v4_tk2.Pz()), (v4_tk1.E() + v4_tk2.E()));
      if(TkMassCharge[1].second==1){ 
        v4_Restk2.SetXYZM(input_tracks[tk2_hindex]->px(),input_tracks[tk2_hindex]->py(),input_tracks[tk2_hindex]->pz(),fabs(TkMassCharge[1].first));
        v4_NonRestk2.SetXYZM(0,0,0,0);
      }
      else{
        v4_Restk2.SetXYZM(0,0,0,0);
        v4_NonRestk2.SetXYZM(input_tracks[tk2_hindex]->px(),input_tracks[tk2_hindex]->py(),input_tracks[tk2_hindex]->pz(),fabs(TkMassCharge[1].first));
      }
      v4_Res.SetPxPyPzE((v4_Restk1.Px() + v4_Restk2.Px()), (v4_Restk1.Py() + v4_Restk2.Py()), (v4_Restk1.Pz() + v4_Restk2.Pz()), (v4_Restk1.E() + v4_Restk2.E()));
      v4_NonRes.SetPxPyPzE((v4_NonRestk1.Px() + v4_NonRestk2.Px()), (v4_NonRestk1.Py() + v4_NonRestk2.Py()), (v4_NonRestk1.Pz() + v4_NonRestk2.Pz()), (v4_NonRestk1.E() + v4_NonRestk2.E()));
      if(TkMassCharge.size()>2)
        if(TkMassCharge[1].second == 1 && TkMassCharge[2].second == 0 ){
          if(tktkRes_mass > 0) {
            if (fabs(v4_Res.Mag()-tktkRes_mass) > tktkRes_mass_window) continue;
            if (v4_Res.Pt() < tktkRes_dPtCut_[Dchannel_number-1]) continue;
            //if (fabs(v4_Res.Eta()) > tktkRes_dEtaCut_[Dchannel_number-1]) continue;
          }
        }
      if(TkMassCharge.size()==2){
        //cut mass window before fit
        if(v4_D.Mag()<mass_window[0] || v4_D.Mag()>mass_window[1]) continue;
        if(v4_D.Pt() < dPtCut_[Dchannel_number-1])continue;
        //if(fabs(v4_D.Eta()) > dEtaCut_[Dchannel_number-1])continue;
        if(fabs(v4_D.Rapidity()) > dRapidityCut_[Dchannel_number-1])continue;
        selectedTkhidx.push_back(tk1_hindex);
        selectedTkhidx.push_back(tk2_hindex);
        selectedTkhidxSet.push_back(selectedTkhidx);
        selectedTkhidx.clear();
        continue;
      }
      for(int tk3idx = ((TkMassCharge[2]==TkMassCharge[1]) ? tk2idx+1:0); tk3idx < (int)isNeededTrackIdx.size(); tk3idx++){
        tk3_hindex = isNeededTrackIdx[tk3idx];
        // cout<<"tk3idx = "<<tk3idx<<" ,tk3_hindex"<<tk3_hindex<<endl;
        if(input_tracks[tk3_hindex]->charge()*TkMassCharge[2].first<0) continue;
        if(tk3_hindex==tk1_hindex) continue;
        if(tk3_hindex==tk2_hindex) continue;

        v4_tk3.SetXYZM(input_tracks[tk3_hindex]->px(),input_tracks[tk3_hindex]->py(),input_tracks[tk3_hindex]->pz(),fabs(TkMassCharge[2].first));
        v4_D.SetPxPyPzE((v4_tk1.Px() + v4_tk2.Px() + v4_tk3.Px()), (v4_tk1.Py() + v4_tk2.Py() + v4_tk3.Py()), (v4_tk1.Pz() + v4_tk2.Pz() + v4_tk3.Pz()), (v4_tk1.E() + v4_tk2.E() + v4_tk3.E()));
        if(TkMassCharge[2].second==1){ 
          v4_Restk3.SetXYZM(input_tracks[tk3_hindex]->px(),input_tracks[tk3_hindex]->py(),input_tracks[tk3_hindex]->pz(),fabs(TkMassCharge[2].first));
          v4_NonRestk3.SetXYZM(0,0,0,0);
        }
        else{ 
          v4_Restk3.SetXYZM(0,0,0,0);
          v4_NonRestk3.SetXYZM(input_tracks[tk3_hindex]->px(),input_tracks[tk3_hindex]->py(),input_tracks[tk3_hindex]->pz(),fabs(TkMassCharge[2].first));
        }
        v4_Res.SetPxPyPzE((v4_Restk1.Px() + v4_Restk2.Px() + v4_Restk3.Px()), (v4_Restk1.Py() + v4_Restk2.Py() + v4_Restk3.Py()), (v4_Restk1.Pz() + v4_Restk2.Pz() + v4_Restk3.Pz()), (v4_Restk1.E() + v4_Restk2.E() + v4_Restk3.E()));
        v4_NonRes.SetPxPyPzE((v4_NonRestk1.Px() + v4_NonRestk2.Px() + v4_NonRestk3.Px()), (v4_NonRestk1.Py() + v4_NonRestk2.Py() + v4_NonRestk3.Py()), (v4_NonRestk1.Pz() + v4_NonRestk2.Pz() + v4_NonRestk3.Pz()), (v4_NonRestk1.E() + v4_NonRestk2.E() + v4_NonRestk3.E()));
        if(TkMassCharge.size()>3)
          if(TkMassCharge[2].second == 1 && TkMassCharge[3].second == 0 ){
            if(tktkRes_mass > 0) {
              if (fabs(v4_Res.Mag()-tktkRes_mass) > tktkRes_mass_window) continue;
              if (v4_Res.Pt() < tktkRes_dPtCut_[Dchannel_number-1]) continue;
              //if (fabs(v4_Res.Eta()) > tktkRes_dEtaCut_[Dchannel_number-1]) continue;
            }
          }
        if(TkMassCharge.size()==3){
          if(tktkRes_mass > 0)
            if((v4_Res.Pt()-v4_NonRes.Pt())/(v4_Res.Pt()+v4_NonRes.Pt()) < ResToNonRes_PtAsym_min_[Dchannel_number-1] || (v4_Res.Pt()-v4_NonRes.Pt())/(v4_Res.Pt()+v4_NonRes.Pt()) > ResToNonRes_PtAsym_max_[Dchannel_number-1]) continue;
          if(v4_D.Mag()<mass_window[0] || v4_D.Mag()>mass_window[1]) continue;
          if(v4_D.Pt() < dPtCut_[Dchannel_number-1])continue;
          //if(fabs(v4_D.Eta()) > dEtaCut_[Dchannel_number-1])continue;
          if(fabs(v4_D.Rapidity()) > dRapidityCut_[Dchannel_number-1])continue;

          selectedTkhidx.push_back(tk1_hindex);
          selectedTkhidx.push_back(tk2_hindex);
          selectedTkhidx.push_back(tk3_hindex);
          selectedTkhidxSet.push_back(selectedTkhidx);
          selectedTkhidx.clear();
          // cout<<"pushed here\n\n"<<endl;
          continue;
        }
        for(int tk4idx = ((TkMassCharge[3]==TkMassCharge[2]) ? tk3idx+1:0); tk4idx < (int)isNeededTrackIdx.size(); tk4idx++){
          tk4_hindex = isNeededTrackIdx[tk4idx];
          if(input_tracks[tk4_hindex]->charge()*TkMassCharge[3].first<0) continue;
          if(tk4_hindex==tk1_hindex) continue;
          if(tk4_hindex==tk2_hindex) continue;
          if(tk4_hindex==tk3_hindex) continue;
          v4_tk4.SetXYZM(input_tracks[tk4_hindex]->px(),input_tracks[tk4_hindex]->py(),input_tracks[tk4_hindex]->pz(),fabs(TkMassCharge[3].first));
          v4_D.SetPxPyPzE((v4_tk1.Px() + v4_tk2.Px() + v4_tk3.Px() + v4_tk4.Px()), (v4_tk1.Py() + v4_tk2.Py() + v4_tk3.Py() + v4_tk4.Py()), (v4_tk1.Pz() + v4_tk2.Pz() + v4_tk3.Pz() + v4_tk4.Pz()), (v4_tk1.E() + v4_tk2.E() + v4_tk3.E() + v4_tk4.E()));
          if(TkMassCharge[3].second==1){ 
            v4_Restk4.SetXYZM(input_tracks[tk4_hindex]->px(),input_tracks[tk4_hindex]->py(),input_tracks[tk4_hindex]->pz(),fabs(TkMassCharge[3].first));
            v4_NonRestk4.SetXYZM(0,0,0,0);
          }
          else{ 
            v4_Restk4.SetXYZM(0,0,0,0);
            v4_NonRestk4.SetXYZM(input_tracks[tk4_hindex]->px(),input_tracks[tk4_hindex]->py(),input_tracks[tk4_hindex]->pz(),fabs(TkMassCharge[3].first));
          }
          v4_Res.SetPxPyPzE((v4_Restk1.Px() + v4_Restk2.Px() + v4_Restk3.Px() + v4_Restk4.Px()), (v4_Restk1.Py() + v4_Restk2.Py() + v4_Restk3.Py() + v4_Restk4.Py()), (v4_Restk1.Pz() + v4_Restk2.Pz() + v4_Restk3.Pz() + v4_Restk4.Pz()), (v4_Restk1.E() + v4_Restk2.E() + v4_Restk3.E() + v4_Restk4.E()));
          v4_NonRes.SetPxPyPzE((v4_NonRestk1.Px() + v4_NonRestk2.Px() + v4_NonRestk3.Px() + v4_NonRestk4.Px()), (v4_NonRestk1.Py() + v4_NonRestk2.Py() + v4_NonRestk3.Py() + v4_NonRestk4.Py()), (v4_NonRestk1.Pz() + v4_NonRestk2.Pz() + v4_NonRestk3.Pz() + v4_NonRestk4.Pz()), (v4_NonRestk1.E() + v4_NonRestk2.E() + v4_NonRestk3.E() + v4_NonRestk4.E()));
          if(TkMassCharge.size()>4)
            if(TkMassCharge[3].second == 1 && TkMassCharge[4].second == 0 ){
              if(tktkRes_mass > 0) {
                if (fabs(v4_Res.Mag()-tktkRes_mass) > tktkRes_mass_window) continue;
                if (v4_Res.Pt() < tktkRes_dPtCut_[Dchannel_number-1]) continue;
                //if (fabs(v4_Res.Eta()) > tktkRes_dEtaCut_[Dchannel_number-1]) continue;
              }
            }
          if(TkMassCharge.size()==4){
            if(tktkRes_mass > 0)
              if((v4_Res.Pt()-v4_NonRes.Pt())/(v4_Res.Pt()+v4_NonRes.Pt()) < ResToNonRes_PtAsym_min_[Dchannel_number-1] || (v4_Res.Pt()-v4_NonRes.Pt())/(v4_Res.Pt()+v4_NonRes.Pt()) > ResToNonRes_PtAsym_max_[Dchannel_number-1]) continue;
            if(v4_D.Mag()<mass_window[0] || v4_D.Mag()>mass_window[1]) continue;
            if(v4_D.Pt() < dPtCut_[Dchannel_number-1])continue;
            //if(fabs(v4_D.Eta()) > dEtaCut_[Dchannel_number-1])continue;
            if(fabs(v4_D.Rapidity()) > dRapidityCut_[Dchannel_number-1])continue;
            selectedTkhidx.push_back(tk1_hindex);
            selectedTkhidx.push_back(tk2_hindex);
            selectedTkhidx.push_back(tk3_hindex);
            selectedTkhidx.push_back(tk4_hindex);
            selectedTkhidxSet.push_back(selectedTkhidx);
            selectedTkhidx.clear();
            continue;
          }
          for(int tk5idx = ((TkMassCharge[4]==TkMassCharge[3]) ? tk4idx+1:0); tk5idx < (int)isNeededTrackIdx.size(); tk5idx++){
            tk5_hindex = isNeededTrackIdx[tk5idx];
            if(input_tracks[tk5_hindex]->charge()*TkMassCharge[4].first<0) continue;
            if(tk5_hindex==tk1_hindex) continue;
            if(tk5_hindex==tk2_hindex) continue;
            if(tk5_hindex==tk3_hindex) continue;
            if(tk5_hindex==tk4_hindex) continue;
            v4_tk5.SetXYZM(input_tracks[tk5_hindex]->px(),input_tracks[tk5_hindex]->py(),input_tracks[tk5_hindex]->pz(),fabs(TkMassCharge[4].first));
            v4_D.SetPxPyPzE((v4_tk1.Px() + v4_tk2.Px() + v4_tk3.Px() + v4_tk4.Px() + v4_tk5.Px()), (v4_tk1.Py() + v4_tk2.Py() + v4_tk3.Py() + v4_tk4.Py() + v4_tk5.Py()), (v4_tk1.Pz() + v4_tk2.Pz() + v4_tk3.Pz() + v4_tk4.Pz() + v4_tk5.Pz()), (v4_tk1.E() + v4_tk2.E() + v4_tk3.E() + v4_tk4.E() + v4_tk5.E()));
            if(TkMassCharge[4].second==1){ 
              v4_Restk5.SetXYZM(input_tracks[tk5_hindex]->px(),input_tracks[tk5_hindex]->py(),input_tracks[tk5_hindex]->pz(),fabs(TkMassCharge[4].first));
              v4_NonRestk5.SetXYZM(0,0,0,0);
            }
            else{ 
              v4_Restk5.SetXYZM(0,0,0,0);
              v4_NonRestk5.SetXYZM(input_tracks[tk5_hindex]->px(),input_tracks[tk5_hindex]->py(),input_tracks[tk5_hindex]->pz(),fabs(TkMassCharge[4].first));
            }
            v4_Res.SetPxPyPzE((v4_Restk1.Px() + v4_Restk2.Px() + v4_Restk3.Px() + v4_Restk4.Px() + v4_Restk5.Px()), (v4_Restk1.Py() + v4_Restk2.Py() + v4_Restk3.Py() + v4_Restk4.Py() + v4_Restk5.Py()), (v4_Restk1.Pz() + v4_Restk2.Pz() + v4_Restk3.Pz() + v4_Restk4.Pz() + v4_Restk5.Pz()), (v4_Restk1.E() + v4_Restk2.E() + v4_Restk3.E() + v4_Restk4.E() + v4_Restk5.E()));
            v4_NonRes.SetPxPyPzE((v4_NonRestk1.Px() + v4_NonRestk2.Px() + v4_NonRestk3.Px() + v4_NonRestk4.Px() + v4_NonRestk5.Px()), (v4_NonRestk1.Py() + v4_NonRestk2.Py() + v4_NonRestk3.Py() + v4_NonRestk4.Py() + v4_NonRestk5.Py()), (v4_NonRestk1.Pz() + v4_NonRestk2.Pz() + v4_NonRestk3.Pz() + v4_NonRestk4.Pz() + v4_NonRestk5.Pz()), (v4_NonRestk1.E() + v4_NonRestk2.E() + v4_NonRestk3.E() + v4_NonRestk4.E() + v4_NonRestk5.E()));
            if(TkMassCharge.size()==5){
              if(tktkRes_mass > 0) {
                if (fabs(v4_Res.Mag()-tktkRes_mass) > tktkRes_mass_window) continue;
                if (v4_Res.Pt() < tktkRes_dPtCut_[Dchannel_number-1]) continue;
                //if (fabs(v4_Res.Eta()) > tktkRes_dEtaCut_[Dchannel_number-1]) continue;
                if((v4_Res.Pt()-v4_NonRes.Pt())/(v4_Res.Pt()+v4_NonRes.Pt()) < ResToNonRes_PtAsym_min_[Dchannel_number-1] || (v4_Res.Pt()-v4_NonRes.Pt())/(v4_Res.Pt()+v4_NonRes.Pt()) > ResToNonRes_PtAsym_max_[Dchannel_number-1]) continue;
              }
              if(v4_D.Mag()<mass_window[0] || v4_D.Mag()>mass_window[1]) continue;
              if(v4_D.Pt() < dPtCut_[Dchannel_number-1])continue;
              //if(fabs(v4_D.Eta()) > dEtaCut_[Dchannel_number-1])continue;
              if(fabs(v4_D.Rapidity()) > dRapidityCut_[Dchannel_number-1])continue;
              selectedTkhidx.push_back(tk1_hindex);
              selectedTkhidx.push_back(tk2_hindex);
              selectedTkhidx.push_back(tk3_hindex);
              selectedTkhidx.push_back(tk4_hindex);
              selectedTkhidx.push_back(tk5_hindex);
              selectedTkhidxSet.push_back(selectedTkhidx);
              selectedTkhidx.clear();
              continue;
            }
          }
        }
      }
    }
  }
  //    std::cout<<"TkCombinationResFast, selectedTkhidxSet.size: "<<selectedTkhidxSet.size()<<std::endl;
  return;
}
//}}}

//BranchOutNTk{{{
void Dfinder::BranchOutNTk(//input 2~4 tracks
                           DInfoBranches &DInfo, 
                           std::vector<const reco::Track*> input_tracks, 
                           reco::Vertex thePrimaryV,
                           std::vector<int> isNeededTrackIdx,
                           std::vector<int> &D_counter,
                           float *mass_window,
                           std::vector< std::pair<float, int> > TkMassCharge,
                           double tktkRes_mass,
                           double tktkRes_mass_window,
                           bool doConstrainFit,
                           bool SequentialFit,
                           int Dchannel_number,
                           int TkCombinationMethod
                           ){
  if(Dchannel_number > (int)Dchannel_.size()){ printf("Exceeding defined # of channel, exit"); return;}

  std::vector< std::vector<int> > selectedTkhidxSet;
  if( TkCombinationMethod == 0 ) 
    TkCombinationPermutation( thePrimaryV, input_tracks, isNeededTrackIdx, mass_window, TkMassCharge, tktkRes_mass, tktkRes_mass_window, selectedTkhidxSet, Dchannel_number);
  else if( TkCombinationMethod == 1 )
    TkCombinationResFast( thePrimaryV, input_tracks, isNeededTrackIdx, mass_window, TkMassCharge, tktkRes_mass, tktkRes_mass_window, selectedTkhidxSet, Dchannel_number);
  else
    { printf("unknown method of track combination, exit"); return;}
  //    std::cout<<"selectedTkhidxSet.size: "<<selectedTkhidxSet.size()<<std::endl;

  float chi = 0.;
  float ndf = 0.;

  //particle factory: produce transient tracks
  KinematicParticleFactoryFromTransientTrack pFactory;
  VirtualKinematicParticleFactory vFactory;
  //fitter for D
  KinematicParticleVertexFitter   tktk_fitter;
  RefCountedKinematicTree         tktk_VFT;
  RefCountedKinematicParticle     tktk_VFP;
  RefCountedKinematicVertex       tktk_VFPvtx;
  //constrain fit fitter
  KinematicConstrainedVertexFitter kcv_tktk_fitter;
  //fitter for Res
  KinematicParticleVertexFitter   tktkRes_fitter;
  RefCountedKinematicTree         tktkRes_VFT;
  RefCountedKinematicParticle     tktkRes_VFP;
  RefCountedKinematicVertex       tktkRes_VFPvtx;

  const MagneticField *field = bField.product();
  AnalyticalImpactPointExtrapolator extrapolator(field);
  TrajectoryStateOnSurface tsos;
  
  TLorentzVector v4_tk;
  // std::vector<TLorentzVector> tktk_4vecs;//fitted tks
  TLorentzVector tktk_4vec;//fitted D
  TLorentzVector unfitted_tktk_4vec;//unfitted D
  // std::vector<TLorentzVector> tktkRes_4vecs;//fitted Res tks
  TLorentzVector tktkRes_4vec;//fitted Res
  TLorentzVector unfitted_tktkRes_4vec;//unfitted Res
  std::vector<RefCountedKinematicParticle> tktk_candidate;//input tracks to D fitter
  std::vector<RefCountedKinematicParticle> tktkRes_candidate;//input tracks to Res fitter
  std::vector<RefCountedKinematicParticle> tktkCands;//output tracks from D fitter
  std::vector<RefCountedKinematicParticle> tktkResCands;//output tracks from Res fitter
  TLorentzVector temp_vec;//for temporary usage

  for(int i = 0; i < int(selectedTkhidxSet.size()); i++){
    if (DInfo.size >= MAX_XB) break;
    
    //clear before using
    v4_tk.Clear();
    // tktk_4vecs.clear();
    tktk_4vec.Clear();
    unfitted_tktk_4vec.Clear();
    // tktkRes_4vecs.clear();
    tktkRes_4vec.Clear();
    unfitted_tktkRes_4vec.Clear();
    tktk_candidate.clear();
    tktkRes_candidate.clear();
    tktkCands.clear();
    tktkResCands.clear();
    unfitted_tktk_4vec.SetPxPyPzE(0., 0., 0., 0.);
    unfitted_tktkRes_4vec.SetPxPyPzE(0., 0., 0., 0.);

    //push back the Res tracks as first tracks
    ParticleMass tk_mass;
    //keep track of the push_back track index
    std::vector<int> pushbackTrkIdx;
    std::vector<int> pushbackResTrkIdx;
    std::vector<float> pushbackTrkMassHypo;
    std::vector<float> pushbackResTrkMassHypo;
    float tk_sigma;
    for(int p = 0; p < int(selectedTkhidxSet[0].size()); p++){       // all Set has same size, could also use [i] as the index for selectedTkhidxSet[i] 
      temp_vec.SetXYZM(input_tracks[selectedTkhidxSet[i][p]]->px(), input_tracks[selectedTkhidxSet[i][p]]->py(), input_tracks[selectedTkhidxSet[i][p]]->pz(), fabs(TkMassCharge[p].first));
      unfitted_tktk_4vec += temp_vec;
      if(TkMassCharge[p].second==0) continue; // push resonace duaghter particle first here, other particle later
      reco::TransientTrack tkTT(*(input_tracks[selectedTkhidxSet[i][p]])/*.pseudoTrack()*/, &(*bField) );
      if (!tkTT.isValid()) continue;
      tk_mass = fabs(TkMassCharge[p].first);
      tk_sigma = Functs.getParticleSigma(tk_mass);
      if(!SequentialFit){
        tktk_candidate.push_back(pFactory.particle(tkTT,tk_mass,chi,ndf,tk_sigma));
        pushbackTrkIdx.push_back(selectedTkhidxSet[i][p]);
        pushbackTrkMassHypo.push_back(tk_mass);
      }
      if(tktkRes_mass>0){
        unfitted_tktkRes_4vec += temp_vec;
        tktkRes_candidate.push_back(pFactory.particle(tkTT,tk_mass,chi,ndf,tk_sigma));
        pushbackResTrkIdx.push_back(selectedTkhidxSet[i][p]);
        pushbackResTrkMassHypo.push_back(tk_mass);
      }
    }

    //if there is tktk Res, check its fit validity also
    if(tktkRes_mass>0){
      tktkRes_VFT = tktkRes_fitter.fit(tktkRes_candidate);
      if(!tktkRes_VFT->isValid()) continue;
      tktkRes_VFP   = tktkRes_VFT->currentParticle();
      tktkRes_VFPvtx = tktkRes_VFT->currentDecayVertex();
      double chi2_prob_tktkRes = TMath::Prob(tktkRes_VFPvtx->chiSquared(),tktkRes_VFPvtx->degreesOfFreedom());

      if(chi2_prob_tktkRes < tktkRes_VtxChiProbCut_[Dchannel_number-1]) continue;

      if(SequentialFit){
        float tktkchi = tktkRes_VFPvtx->chiSquared();
        float tktkndf = tktkRes_VFPvtx->degreesOfFreedom();
        tktk_candidate.push_back(vFactory.particle(tktkRes_VFP->currentState(),tktkchi,tktkndf,tktkRes_VFP));
        //tktk_candidate.push_back(tktkRes_VFP);
        pushbackTrkIdx.push_back(-1);//means its a resonance particle
        pushbackTrkMassHypo.push_back(-1);//means its a resonance particle
      }
    }
    //push back the other tracks
    for(int p = 0; p < int(selectedTkhidxSet[0].size()); p++){        
      if(TkMassCharge[p].second==1) continue;
      reco::TransientTrack tkTT(*(input_tracks[selectedTkhidxSet[i][p]])/*.pseudoTrack()*/, &(*bField) );
      if (!tkTT.isValid()) continue;
      tk_mass = fabs(TkMassCharge[p].first);
      tk_sigma = Functs.getParticleSigma(tk_mass);
      tktk_candidate.push_back(pFactory.particle(tkTT,tk_mass,chi,ndf,tk_sigma));
      pushbackTrkIdx.push_back(selectedTkhidxSet[i][p]);
      pushbackTrkMassHypo.push_back(tk_mass);
    }

    double MaximumDoca = Functs.getMaxDoca(tktk_candidate); // Max of all tracks pairs MinimumDistance
    if (MaximumDoca > MaxDocaCut_[Dchannel_number-1]) continue;

    if(tktkRes_mass>0){
      if(doConstrainFit){
        ParticleMass tktkResMass = tktkRes_mass;
        MultiTrackKinematicConstraint *tktkResConstraint = new TwoTrackMassKinematicConstraint(tktkResMass);
        tktk_VFT = kcv_tktk_fitter.fit(tktk_candidate, tktkResConstraint);
        delete tktkResConstraint;
      }
      else tktk_VFT = tktk_fitter.fit(tktk_candidate);
    }
    else{
      tktk_VFT = tktk_fitter.fit(tktk_candidate); // this line same as previous one?
    }

    // std::cout<<"=> "<<i<<std::endl;

    if(!tktk_VFT->isValid()) continue;

    tktk_VFT->movePointerToTheTop(); // KinematicTree.cc , make the Tree accessible, pointer to particle, and daughters
    tktk_VFP   = tktk_VFT->currentParticle();
    tktk_VFPvtx = tktk_VFT->currentDecayVertex();
    if (!tktk_VFPvtx->vertexIsValid()) continue;

    double chi2_prob_tktk = TMath::Prob(tktk_VFPvtx->chiSquared(),tktk_VFPvtx->degreesOfFreedom()); 
    if(chi2_prob_tktk < VtxChiProbCut_[Dchannel_number-1]) continue;

    tktkCands  = tktk_VFT->finalStateParticles(); // these determine the order of output - rftk

    //cut mass window after fit
    //if (tktk_VFP->currentState().mass()<mass_window[0] || tktk_VFP->currentState().mass()>mass_window[1]) continue;

    // for(unsigned int k = 0; k < tktkCands.size(); k++){
    //   v4_tk.SetPxPyPzE(tktkCands[k]->currentState().kinematicParameters().momentum().x(),
    //                    tktkCands[k]->currentState().kinematicParameters().momentum().y(),
    //                    tktkCands[k]->currentState().kinematicParameters().momentum().z(),
    //                    tktkCands[k]->currentState().kinematicParameters().energy());
    //   tktk_4vecs.push_back(v4_tk);
    //   v4_tk.Clear();
    // }

    tktk_4vec.SetPxPyPzE(tktk_VFP->currentState().kinematicParameters().momentum().x(),
                         tktk_VFP->currentState().kinematicParameters().momentum().y(),
                         tktk_VFP->currentState().kinematicParameters().momentum().z(),
                         tktk_VFP->currentState().kinematicParameters().energy());

    //tktkRes fit info
    if(tktkRes_mass>0){
      tktkResCands = tktkRes_VFT->finalStateParticles();

      // for(unsigned int k = 0; k < tktkResCands.size(); k++){
      //   v4_tk.SetPxPyPzE(tktkResCands[k]->currentState().kinematicParameters().momentum().x(),
      //                    tktkResCands[k]->currentState().kinematicParameters().momentum().y(),
      //                    tktkResCands[k]->currentState().kinematicParameters().momentum().z(),
      //                    tktkResCands[k]->currentState().kinematicParameters().energy());
      //   tktkRes_4vecs.push_back(v4_tk);
      //   v4_tk.Clear();
      // }

      tktkRes_4vec.SetPxPyPzE(tktkRes_VFP->currentState().kinematicParameters().momentum().x(),
                              tktkRes_VFP->currentState().kinematicParameters().momentum().y(),
                              tktkRes_VFP->currentState().kinematicParameters().momentum().z(),
                              tktkRes_VFP->currentState().kinematicParameters().energy());

      DInfo.tktkRes_unfitted_mass[DInfo.size]   = unfitted_tktkRes_4vec.Mag();
      DInfo.tktkRes_unfitted_pt[DInfo.size]     = unfitted_tktkRes_4vec.Pt();
      DInfo.tktkRes_mass[DInfo.size]            = tktkRes_4vec.Mag();
      DInfo.tktkRes_pt[DInfo.size]              = tktkRes_4vec.Pt();
      DInfo.tktkRes_eta[DInfo.size]             = tktkRes_4vec.Eta();
      DInfo.tktkRes_phi[DInfo.size]             = tktkRes_4vec.Phi();
      DInfo.tktkRes_vtxX[DInfo.size]            = tktkRes_VFPvtx->position().x();
      DInfo.tktkRes_vtxY[DInfo.size]            = tktkRes_VFPvtx->position().y();
      DInfo.tktkRes_vtxZ[DInfo.size]            = tktkRes_VFPvtx->position().z();
      DInfo.tktkRes_vtxXErr[DInfo.size]         = tktkRes_VFPvtx->error().cxx();
      DInfo.tktkRes_vtxYErr[DInfo.size]         = tktkRes_VFPvtx->error().cyy();
      DInfo.tktkRes_vtxZErr[DInfo.size]         = tktkRes_VFPvtx->error().czz();
      DInfo.tktkRes_vtxYXErr[DInfo.size]        = tktkRes_VFPvtx->error().cyx();
      DInfo.tktkRes_vtxZXErr[DInfo.size]        = tktkRes_VFPvtx->error().czx();
      DInfo.tktkRes_vtxZYErr[DInfo.size]        = tktkRes_VFPvtx->error().czy();
      DInfo.tktkRes_vtxdof[DInfo.size]          = tktkRes_VFPvtx->degreesOfFreedom();
      DInfo.tktkRes_vtxchi2[DInfo.size]         = tktkRes_VFPvtx->chiSquared();

      TVector3 Res_svpvVec;
      Res_svpvVec.SetXYZ(DInfo.tktkRes_vtxX[DInfo.size]-EvtInfo.PVx, DInfo.tktkRes_vtxY[DInfo.size]-EvtInfo.PVy, DInfo.tktkRes_vtxZ[DInfo.size]-EvtInfo.PVz);
      TVector3 Res_dVec;
      //Res_dVec.SetPtEtaPhi(DInfo.tktkRes_pt[DInfo.size], DInfo.tktkRes_eta[DInfo.size], DInfo.tktkRes_phi[DInfo.size]);
      Res_dVec.SetXYZ(tktkRes_4vec.Px(), tktkRes_4vec.Py(), tktkRes_4vec.Pz());
      DInfo.tktkRes_alpha[DInfo.size] = Res_svpvVec.Angle(Res_dVec);
      if( DInfo.tktkRes_alpha[DInfo.size] > tktkRes_alphaCut_[Dchannel_number-1]) continue;

      TVector3 Res_svTosvVec;
      Res_svTosvVec.SetXYZ(DInfo.tktkRes_vtxX[DInfo.size]-tktk_VFPvtx->position().x(), DInfo.tktkRes_vtxY[DInfo.size]-tktk_VFPvtx->position().y(), DInfo.tktkRes_vtxZ[DInfo.size]-tktk_VFPvtx->position().z());
      DInfo.tktkRes_alphaToSV[DInfo.size] = Res_svTosvVec.Angle(Res_dVec);
      if( DInfo.tktkRes_alphaToSV[DInfo.size] > tktkRes_alphaToSVCut_[Dchannel_number-1]) continue;

      VertexDistance3D Res_a3d;
      DInfo.tktkRes_svpvDistance[DInfo.size] = Res_a3d.distance(thePrimaryV,tktkRes_VFPvtx->vertexState()).value();
      DInfo.tktkRes_svpvDisErr[DInfo.size] = Res_a3d.distance(thePrimaryV,tktkRes_VFPvtx->vertexState()).error();
      if(DInfo.tktkRes_pt[DInfo.size] <= tktkRes_dCutSeparating_PtVal_[Dchannel_number-1] && (DInfo.tktkRes_svpvDistance[DInfo.size]/DInfo.tktkRes_svpvDisErr[DInfo.size]) < tktkRes_svpvDistanceCut_lowptD_[Dchannel_number-1]) continue;
      else if( DInfo.tktkRes_pt[DInfo.size] > tktkRes_dCutSeparating_PtVal_[Dchannel_number-1] && (DInfo.tktkRes_svpvDistance[DInfo.size]/DInfo.tktkRes_svpvDisErr[DInfo.size]) < tktkRes_svpvDistanceCut_highptD_[Dchannel_number-1]) continue;

      DInfo.tktkRes_svpvDistanceToSV[DInfo.size] = Res_a3d.distance(tktk_VFPvtx->vertexState(),tktkRes_VFPvtx->vertexState()).value();
      DInfo.tktkRes_svpvDisErrToSV[DInfo.size] = Res_a3d.distance(tktk_VFPvtx->vertexState(),tktkRes_VFPvtx->vertexState()).error();
      if(DInfo.tktkRes_pt[DInfo.size] <= tktkRes_dCutSeparating_PtVal_[Dchannel_number-1] && (DInfo.tktkRes_svpvDistanceToSV[DInfo.size]/DInfo.tktkRes_svpvDisErrToSV[DInfo.size]) < tktkRes_svpvDistanceToSVCut_lowptD_[Dchannel_number-1]) continue;
      else if( DInfo.tktkRes_pt[DInfo.size] > tktkRes_dCutSeparating_PtVal_[Dchannel_number-1] && (DInfo.tktkRes_svpvDistanceToSV[DInfo.size]/DInfo.tktkRes_svpvDisErrToSV[DInfo.size]) < tktkRes_svpvDistanceToSVCut_highptD_[Dchannel_number-1]) continue;

      //index initialization to -2
      DInfo.tktkRes_rftk1_index[DInfo.size]     = -2;
      DInfo.tktkRes_rftk2_index[DInfo.size]     = -2;
      DInfo.tktkRes_rftk3_index[DInfo.size]     = -2;
      DInfo.tktkRes_rftk4_index[DInfo.size]     = -2;
      DInfo.tktkRes_rftk1_MassHypo[DInfo.size]     = -2;
      DInfo.tktkRes_rftk2_MassHypo[DInfo.size]     = -2;
      DInfo.tktkRes_rftk3_MassHypo[DInfo.size]     = -2;
      DInfo.tktkRes_rftk4_MassHypo[DInfo.size]     = -2;

      // DInfo.tktkRes_rftk1_mass[DInfo.size]      = tktkRes_4vecs[0].Mag();
      // DInfo.tktkRes_rftk1_pt[DInfo.size]        = tktkRes_4vecs[0].Pt();
      // DInfo.tktkRes_rftk1_eta[DInfo.size]       = tktkRes_4vecs[0].Eta();
      // DInfo.tktkRes_rftk1_phi[DInfo.size]       = tktkRes_4vecs[0].Phi();
      DInfo.tktkRes_rftk1_index[DInfo.size]     = pushbackResTrkIdx[0];
      DInfo.tktkRes_rftk1_MassHypo[DInfo.size]     = pushbackResTrkMassHypo[0];
      // DInfo.tktkRes_rftk2_mass[DInfo.size]      = tktkRes_4vecs[1].Mag();
      // DInfo.tktkRes_rftk2_pt[DInfo.size]        = tktkRes_4vecs[1].Pt();
      // DInfo.tktkRes_rftk2_eta[DInfo.size]       = tktkRes_4vecs[1].Eta();
      // DInfo.tktkRes_rftk2_phi[DInfo.size]       = tktkRes_4vecs[1].Phi();
      DInfo.tktkRes_rftk2_index[DInfo.size]     = pushbackResTrkIdx[1];
      DInfo.tktkRes_rftk2_MassHypo[DInfo.size]     = pushbackResTrkMassHypo[1];
      if(tktkResCands.size()>2){
        // DInfo.tktkRes_rftk3_mass[DInfo.size]      = tktkRes_4vecs[2].Mag();
        // DInfo.tktkRes_rftk3_pt[DInfo.size]        = tktkRes_4vecs[2].Pt();
        // DInfo.tktkRes_rftk3_eta[DInfo.size]       = tktkRes_4vecs[2].Eta();
        // DInfo.tktkRes_rftk3_phi[DInfo.size]       = tktkRes_4vecs[2].Phi();
        DInfo.tktkRes_rftk3_index[DInfo.size]     = pushbackResTrkIdx[2];
        DInfo.tktkRes_rftk3_MassHypo[DInfo.size]     = pushbackResTrkMassHypo[2];
      }
      if(tktkResCands.size()>3){
        // DInfo.tktkRes_rftk4_mass[DInfo.size]      = tktkRes_4vecs[3].Mag();
        // DInfo.tktkRes_rftk4_pt[DInfo.size]        = tktkRes_4vecs[3].Pt();
        // DInfo.tktkRes_rftk4_eta[DInfo.size]       = tktkRes_4vecs[3].Eta();
        // DInfo.tktkRes_rftk4_phi[DInfo.size]       = tktkRes_4vecs[3].Phi();
        DInfo.tktkRes_rftk4_index[DInfo.size]     = pushbackResTrkIdx[3];
        DInfo.tktkRes_rftk4_MassHypo[DInfo.size]     = pushbackResTrkMassHypo[3];
      }
    }

    //fit info
    DInfo.index[DInfo.size]           = DInfo.size;
    DInfo.unfitted_mass[DInfo.size]   = unfitted_tktk_4vec.Mag();
    DInfo.unfitted_pt[DInfo.size]     = unfitted_tktk_4vec.Pt();
    DInfo.mass[DInfo.size]            = tktk_4vec.Mag();
    DInfo.pt[DInfo.size]              = tktk_4vec.Pt();
    DInfo.eta[DInfo.size]             = tktk_4vec.Eta();
    DInfo.phi[DInfo.size]             = tktk_4vec.Phi();
    DInfo.px[DInfo.size]              = tktk_4vec.Px();
    DInfo.py[DInfo.size]              = tktk_4vec.Py();
    DInfo.pz[DInfo.size]              = tktk_4vec.Pz();
    DInfo.MaxDoca[DInfo.size]         = MaximumDoca;

    VertexDistance3D a3d;
    //https://github.com/cms-sw/cmssw/blob/CMSSW_7_5_0/RecoVertex/VertexTools/src/VertexDistance3D.cc
    DInfo.svpvDistance[DInfo.size] = a3d.distance(thePrimaryV,tktk_VFPvtx->vertexState()).value(); 
    DInfo.svpvDisErr[DInfo.size] = a3d.distance(thePrimaryV,tktk_VFPvtx->vertexState()).error();
    if( DInfo.pt[DInfo.size] <= dCutSeparating_PtVal_[Dchannel_number-1] && (DInfo.svpvDistance[DInfo.size]/DInfo.svpvDisErr[DInfo.size]) < svpvDistanceCut_lowptD_[Dchannel_number-1]) continue;
    else if( DInfo.pt[DInfo.size] > dCutSeparating_PtVal_[Dchannel_number-1] && (DInfo.svpvDistance[DInfo.size]/DInfo.svpvDisErr[DInfo.size]) < svpvDistanceCut_highptD_[Dchannel_number-1]) continue;

    tsos = extrapolator.extrapolate(tktk_VFP->currentState().freeTrajectoryState(),
                                    RecoVertex::convertPos(thePrimaryV.position()));
    //std::pair<bool, Measurement1D> cur3DIP;
    Measurement1D cur3DIP;
    
    GlobalPoint refPoint          = tsos.globalPosition();
    GlobalError refPointErr       = tsos.cartesianError().position();
    GlobalPoint vertexPosition    = RecoVertex::convertPos(thePrimaryV.position());
    GlobalError vertexPositionErr = RecoVertex::convertError(thePrimaryV.error());
    cur3DIP =  (a3d.distance(VertexState(vertexPosition,vertexPositionErr), VertexState(refPoint, refPointErr))); //IPTools::absoluteImpactParameter3D(tsos, thePrimaryV, a3d);

    DInfo.ip3d[DInfo.size]            = cur3DIP.value();
    DInfo.ip3derr[DInfo.size]         = cur3DIP.error();
    
    reco::Vertex::Point vp1(thePrimaryV.position().x(), thePrimaryV.position().y(), 0.);
    reco::Vertex::Point vp2(tktk_VFPvtx->vertexState().position().x(), tktk_VFPvtx->vertexState().position().y(), 0.);
    ROOT::Math::SVector<double, 6> sv1(thePrimaryV.covariance(0,0), thePrimaryV.covariance(0,1), thePrimaryV.covariance(1,1), 0., 0., 0.);
    ROOT::Math::SVector<double, 6> sv2(tktk_VFPvtx->vertexState().error().cxx(), tktk_VFPvtx->vertexState().error().cyx(), tktk_VFPvtx->vertexState().error().cyy(), 0., 0., 0.);
    reco::Vertex::Error ve1(sv1);
    reco::Vertex::Error ve2(sv2);
    reco::Vertex v1(vp1, ve1);
    reco::Vertex v2(vp2, ve2);
    DInfo.svpvDistance_2D[DInfo.size] = a3d.distance(v1, v2).value();
    DInfo.svpvDisErr_2D[DInfo.size] = a3d.distance(v1, v2).error();

    DInfo.vtxX[DInfo.size]            = tktk_VFPvtx->position().x();
    DInfo.vtxY[DInfo.size]            = tktk_VFPvtx->position().y();
    DInfo.vtxZ[DInfo.size]            = tktk_VFPvtx->position().z();
    DInfo.vtxXErr[DInfo.size]         = tktk_VFPvtx->error().cxx();
    DInfo.vtxYErr[DInfo.size]         = tktk_VFPvtx->error().cyy();
    DInfo.vtxZErr[DInfo.size]         = tktk_VFPvtx->error().czz();
    DInfo.vtxYXErr[DInfo.size]        = tktk_VFPvtx->error().cyx();
    DInfo.vtxZXErr[DInfo.size]        = tktk_VFPvtx->error().czx();
    DInfo.vtxZYErr[DInfo.size]        = tktk_VFPvtx->error().czy();
    DInfo.vtxdof[DInfo.size]          = tktk_VFPvtx->degreesOfFreedom();
    DInfo.vtxchi2[DInfo.size]         = tktk_VFPvtx->chiSquared();

    TVector3 svpvVec;
    svpvVec.SetXYZ(DInfo.vtxX[DInfo.size]-EvtInfo.PVx, DInfo.vtxY[DInfo.size]-EvtInfo.PVy, DInfo.vtxZ[DInfo.size]-EvtInfo.PVz);
    TVector3 dVec;
    dVec.SetXYZ(DInfo.px[DInfo.size], DInfo.py[DInfo.size], DInfo.pz[DInfo.size]);
    DInfo.alpha[DInfo.size] = svpvVec.Angle(dVec);
    if( DInfo.alpha[DInfo.size] > alphaCut_[Dchannel_number-1]) continue;

    // DInfo.rftk1_mass[DInfo.size]      = tktk_4vecs[0].Mag();
    // DInfo.rftk1_pt[DInfo.size]        = tktk_4vecs[0].Pt();
    // DInfo.rftk1_eta[DInfo.size]       = tktk_4vecs[0].Eta();
    // DInfo.rftk1_phi[DInfo.size]       = tktk_4vecs[0].Phi();
    // DInfo.rftk2_mass[DInfo.size]      = tktk_4vecs[1].Mag();
    // DInfo.rftk2_pt[DInfo.size]        = tktk_4vecs[1].Pt();
    // DInfo.rftk2_eta[DInfo.size]       = tktk_4vecs[1].Eta();
    // DInfo.rftk2_phi[DInfo.size]       = tktk_4vecs[1].Phi();

    //Index initialize to -2
    DInfo.rftk1_index[DInfo.size] = -2;
    DInfo.rftk2_index[DInfo.size] = -2;
    DInfo.rftk3_index[DInfo.size] = -2;
    DInfo.rftk4_index[DInfo.size] = -2;
    DInfo.rftk5_index[DInfo.size] = -2;
    DInfo.rftk1_MassHypo[DInfo.size] = -2;
    DInfo.rftk2_MassHypo[DInfo.size] = -2;
    DInfo.rftk3_MassHypo[DInfo.size] = -2;
    DInfo.rftk4_MassHypo[DInfo.size] = -2;
    DInfo.rftk5_MassHypo[DInfo.size] = -2;

    DInfo.rftk1_index[DInfo.size]     = pushbackTrkIdx[0];
    DInfo.rftk2_index[DInfo.size]     = pushbackTrkIdx[1];
    DInfo.rftk1_MassHypo[DInfo.size] = pushbackTrkMassHypo[0];
    DInfo.rftk2_MassHypo[DInfo.size] = pushbackTrkMassHypo[1];

    //document the mass hypothesis
    //If its a Res particle, save it as D0
    if( DInfo.rftk1_index[DInfo.size] == -1) DInfo.rftk1_MassHypo[DInfo.size] = D0_MASS;

    if(tktkCands.size()>2){
      // DInfo.rftk3_mass[DInfo.size]  = tktk_4vecs[2].Mag();
      // DInfo.rftk3_pt[DInfo.size]    = tktk_4vecs[2].Pt();
      // DInfo.rftk3_eta[DInfo.size]   = tktk_4vecs[2].Eta();
      // DInfo.rftk3_phi[DInfo.size]   = tktk_4vecs[2].Phi();
      DInfo.rftk3_index[DInfo.size] = pushbackTrkIdx[2];
      DInfo.rftk3_MassHypo[DInfo.size] = pushbackTrkMassHypo[2];
    }
    if(tktkCands.size()>3){
      // DInfo.rftk4_mass[DInfo.size]  = tktk_4vecs[3].Mag();
      // DInfo.rftk4_pt[DInfo.size]    = tktk_4vecs[3].Pt();
      // DInfo.rftk4_eta[DInfo.size]   = tktk_4vecs[3].Eta();
      // DInfo.rftk4_phi[DInfo.size]   = tktk_4vecs[3].Phi();
      DInfo.rftk4_index[DInfo.size] = pushbackTrkIdx[3];
      DInfo.rftk4_MassHypo[DInfo.size] = pushbackTrkMassHypo[3];
    }
    if(tktkCands.size()>4){
      // DInfo.rftk5_mass[DInfo.size]  = tktk_4vecs[4].Mag();
      // DInfo.rftk5_pt[DInfo.size]    = tktk_4vecs[4].Pt();
      // DInfo.rftk5_eta[DInfo.size]   = tktk_4vecs[4].Eta();
      // DInfo.rftk5_phi[DInfo.size]   = tktk_4vecs[4].Phi();
      DInfo.rftk5_index[DInfo.size] = pushbackTrkIdx[4];
      DInfo.rftk5_MassHypo[DInfo.size] = pushbackTrkMassHypo[4];
    }

    DInfo.type[DInfo.size] = Dchannel_number;
    D_counter[Dchannel_number-1]++;

    tktk_candidate.clear();
    tktkCands.clear();

    // Use the trick of swapping with an empty vector to free the memory
    //decltype(tktk_4vecs)().swap(tktk_4vecs);
    //decltype(tktkRes_4vecs)().swap(tktkRes_4vecs);    
    //decltype(tktk_candidate)().swap(tktk_candidate);   
    //decltype(tktkRes_candidate)().swap(tktkRes_candidate);
    //decltype(tktkCands)().swap(tktkCands);        
    //decltype(tktkResCands)().swap(tktkResCands);     

    DInfo.size++;
  }
}
//}}}

//define this as a plug-in
DEFINE_FWK_MODULE(Dfinder);
