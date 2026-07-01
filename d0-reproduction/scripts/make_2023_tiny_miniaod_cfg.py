import os

import FWCore.ParameterSet.Config as cms


INPUT_FILE = os.environ.get(
    "D0_TINY_MINIAOD_INPUT",
    "root://xrootd-vanderbilt.sites.opensciencegrid.org//store/hidata/HIRun2023A/HIForward0/MINIAOD/16Jan2024-v1/2810000/3cb20bb5-a7b1-43a2-8025-9807f4f51e71.root",
)
OUTPUT_FILE = os.environ.get(
    "D0_TINY_MINIAOD_OUTPUT",
    "file:/eos/user/e/evzhang/codex-d0-crab-smoke/tiny-input/hiforward0_2023_miniaod_first100.root",
)
MAX_EVENTS = int(os.environ.get("D0_TINY_MINIAOD_MAX_EVENTS", "100"))

process = cms.Process("D0TinyMINIAOD")

process.source = cms.Source(
    "PoolSource",
    fileNames=cms.untracked.vstring(INPUT_FILE),
    duplicateCheckMode=cms.untracked.string("noDuplicateCheck"),
)

process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(MAX_EVENTS))

process.out = cms.OutputModule(
    "PoolOutputModule",
    fileName=cms.untracked.string(OUTPUT_FILE),
    outputCommands=cms.untracked.vstring("keep *"),
)

process.end = cms.EndPath(process.out)
