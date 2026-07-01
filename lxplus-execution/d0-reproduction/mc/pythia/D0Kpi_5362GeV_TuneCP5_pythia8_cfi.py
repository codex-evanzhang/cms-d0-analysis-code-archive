import FWCore.ParameterSet.Config as cms

from Configuration.Generator.Pythia8CommonSettings_cfi import *
from Configuration.Generator.MCTunes2017.PythiaCP5Settings_cfi import *


generator = cms.EDFilter(
    "Pythia8ConcurrentGeneratorFilter",
    pythiaPylistVerbosity=cms.untracked.int32(0),
    pythiaHepMCVerbosity=cms.untracked.bool(False),
    maxEventsToPrint=cms.untracked.int32(0),
    filterEfficiency=cms.untracked.double(1.0),
    comEnergy=cms.double(5362.0),
    PythiaParameters=cms.PSet(
        pythia8CommonSettingsBlock,
        pythia8CP5SettingsBlock,
        processParameters=cms.vstring(
            "HardQCD:hardccbar = on",
            "PhaseSpace:pTHatMin = 2.",
            "421:onMode = off",
            "421:onIfMatch = -321 211",
            "421:onIfMatch = 321 -211",
        ),
        parameterSets=cms.vstring(
            "pythia8CommonSettings",
            "pythia8CP5Settings",
            "processParameters",
        ),
    ),
)

ProductionFilterSequence = cms.Sequence(generator)
