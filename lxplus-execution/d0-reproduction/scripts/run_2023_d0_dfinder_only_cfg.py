import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing
from Configuration.Eras.Era_Run3_2023_UPC_cff import Run3_2023_UPC


options = VarParsing("analysis")
options.outputFile = "d0_dfinder_only.root"
options.inputFiles = [
    "root://xrootd-vanderbilt.sites.opensciencegrid.org//store/hidata/HIRun2023A/HIForward0/MINIAOD/16Jan2024-v1/2810000/3cb20bb5-a7b1-43a2-8025-9807f4f51e71.root"
]
options.maxEvents = 1000
options.parseArguments()

process = cms.Process("DfinderOnly", Run3_2023_UPC)

process.load("Configuration.Geometry.GeometryDB_cff")
process.load("Configuration.StandardSequences.Services_cff")
process.load("Configuration.StandardSequences.MagneticField_38T_cff")
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
process.load("FWCore.MessageService.MessageLogger_cfi")

from Configuration.AlCa.GlobalTag import GlobalTag

process.GlobalTag = GlobalTag(process.GlobalTag, "132X_dataRun3_Prompt_HI_LowPtPhotonReg_v2", "")

process.source = cms.Source(
    "PoolSource",
    duplicateCheckMode=cms.untracked.string("noDuplicateCheck"),
    fileNames=cms.untracked.vstring(options.inputFiles),
)
process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(options.maxEvents))
process.options = cms.untracked.PSet(
    numberOfThreads=cms.untracked.uint32(1),
    numberOfStreams=cms.untracked.uint32(0),
    wantSummary=cms.untracked.bool(True),
)
process.TFileService = cms.Service("TFileService", fileName=cms.string(options.outputFile))

from Bfinder.finderMaker.finderMaker_75X_cff import finderMaker_75X, setCutForAllChannelsDfinder

finderMaker_75X(
    process,
    runOnMC=False,
    VtxLabel="offlineSlimmedPrimaryVertices",
    TrkLabel="packedPFCandidates",
    TrkChi2Label="packedPFCandidateTrackChi2",
    GenParticleLabel="prunedGenParticles",
)

process.Dfinder.tkPtCut = cms.double(0.2)
process.Dfinder.tkEtaCut = cms.double(2.4)
process.Dfinder.Dchannel = cms.vint32(
    1,  # K+ pi- : D0bar
    1,  # K- pi+ : D0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
)
setCutForAllChannelsDfinder(
    process,
    dPtCut=0.0,
    VtxChiProbCut=0.05,
    svpvDistanceCut=0.5,
    alphaCut=4.0,
)
process.Dfinder.dPtCut = cms.vdouble(
    0.0, 0.0,  # K pi D0/D0bar
    1.0, 1.0,  # D+
    0.0, 0.0,
    0.0, 0.0,
    0.0, 0.0,
    0.0, 0.0,
    0.0, 0.0,
    0.0, 0.0,
    2.0, 2.0,
)
process.Dfinder.printInfo = cms.bool(False)
process.Dfinder.dropUnusedTracks = cms.bool(True)
process.Dfinder.readDedx = cms.bool(False)

process.dfinder = cms.Path(process.DfinderSequence)
process.MessageLogger.cerr.FwkReport.reportEvery = 100
