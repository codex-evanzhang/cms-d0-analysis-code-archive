import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing("analysis")
options.outputFile = "/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/minimal-forest-smoke/d0_minimal_forest.root"
options.register(
    "tfileServiceFile",
    "",
    VarParsing.multiplicity.singleton,
    VarParsing.varType.string,
    "Exact TFileService output path. If empty, options.outputFile is used.",
)
options.register(
    "trackPtMin",
    0.2,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.float,
    "Minimum daughter packed-candidate pT.",
)
options.register(
    "trackEtaMax",
    2.4,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.float,
    "Maximum absolute daughter packed-candidate eta.",
)
options.register(
    "requireHighPurity",
    False,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.bool,
    "Require packed candidates with track details to pass high-purity track quality.",
)
options.register(
    "candidateMassMin",
    1.6,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.float,
    "Minimum saved Kpi/piK invariant mass.",
)
options.register(
    "candidateMassMax",
    2.4,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.float,
    "Maximum saved Kpi/piK invariant mass.",
)
options.register(
    "maxCandidates",
    200000,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.int,
    "Maximum saved candidates per event.",
)
options.parseArguments()

process = cms.Process("D0MINI")
process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 100

process.source = cms.Source(
    "PoolSource",
    duplicateCheckMode=cms.untracked.string("noDuplicateCheck"),
    fileNames=cms.untracked.vstring(options.inputFiles),
)

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(options.maxEvents))

process.TFileService = cms.Service(
    "TFileService",
    fileName=cms.string(options.tfileServiceFile or options.outputFile),
)

from EvanAnalysis.D0MinimalForest.d0MinimalForest_cfi import d0MinimalForest

process.d0MinimalForest = d0MinimalForest.clone(
    trackPtMin=options.trackPtMin,
    trackEtaMax=options.trackEtaMax,
    requireHighPurity=options.requireHighPurity,
    candidateMassMin=options.candidateMassMin,
    candidateMassMax=options.candidateMassMax,
    maxCandidates=options.maxCandidates,
)

process.p = cms.Path(process.d0MinimalForest)
