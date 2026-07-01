import FWCore.ParameterSet.Config as cms

d0MinimalForest = cms.EDAnalyzer(
    "D0MinimalForest",
    pfCandidateSrc=cms.InputTag("packedPFCandidates"),
    vertexSrc=cms.InputTag("offlineSlimmedPrimaryVertices"),
    trackPtMin=cms.double(0.2),
    trackEtaMax=cms.double(2.4),
    requireHighPurity=cms.bool(False),
    candidateMassMin=cms.double(1.6),
    candidateMassMax=cms.double(2.4),
    maxCandidates=cms.uint32(200000),
    hfEtaMin=cms.double(3.0),
    hfEtaMax=cms.double(5.2),
)
