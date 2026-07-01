import FWCore.ParameterSet.Config as cms
from Configuration.Generator.Pythia8CommonSettings_cfi import *
from GeneratorInterface.EvtGenInterface.EvtGenSetting_cff import *
from Configuration.Generator.Pythia8PhotonFluxSettings_cfi import PhotonFlux_PbPb


generator = cms.EDFilter("Pythia8GeneratorFilter",
    maxEventsToPrint = cms.untracked.int32(1),
    pythiaPylistVerbosity = cms.untracked.int32(1),
    filterEfficiency = cms.untracked.double(1.0),
    pythiaHepMCVerbosity = cms.untracked.bool(False),
    comEnergy = cms.double(5362.),
    PhotonFlux = PhotonFlux_PbPb,
    ExternalDecays = cms.PSet(
                             EvtGen130 = cms.untracked.PSet(
                                     decay_table = cms.string('GeneratorInterface/EvtGenInterface/data/DECAY_2014_NOLONGLIFE.DEC'),
                                     operates_on_particles = cms.vint32(),
                                     particle_property_file = cms.FileInPath('GeneratorInterface/EvtGenInterface/data/evt_2014.pdl'),
                                     user_decay_file = cms.vstring('GeneratorInterface/ExternalDecays/data/D0_Kpi.dec'),
                                     list_forced_decays = cms.vstring('myD0', 'myanti-D0'),
                                     convertPythiaCodes = cms.untracked.bool(False),
                             ),
                             parameterSets = cms.vstring('EvtGen130')
                     ),
    PythiaParameters = cms.PSet(
        pythia8CommonSettingsBlock,
        processParameters = cms.vstring('SoftQCD:all = on',
                                        'PhaseSpace:pTHatMin = 0.',
                                        'PhotonParton:all = on',
                                        'MultipartonInteractions:pT0Ref = 3.0',
                                        'PDF:beamA2gamma = on',# have the photon coming from beam A
                                        'PDF:proton2gammaSet = 0',
                                        'PDF:pSetB = LHAPDF6:EPPS21nlo_CT18Anlo_Pb208', # add nuclear modifications to beam B
                                        'PDF:gammaFluxApprox2bMin = 13.272',
                                        'PDF:beam2gammaApprox = 2',
                                        'Photon:sampleQ2 = off'
                                    ),
        parameterSets = cms.vstring('pythia8CommonSettings',
                                    'processParameters')
    )
)

generator.PythiaParameters.processParameters.extend(EvtGenExtraParticles)
#Bypass concurrency check
"_generator=cms.EDFilter fromGeneratorInterface.Core.ExternalGeneratorFilterimportExternalGeneratorFilter generator=ExternalGeneratorFilter(_generator)"


partonfilter = cms.EDFilter("PythiaFilter",
                                    ParticleID = cms.untracked.int32(5) #non-prompt
                                    )

D0Daufilter = cms.EDFilter("PythiaMomDauFilter",
    ParticleID = cms.untracked.int32(421),
    MomMinPt = cms.untracked.double(0.),
        MomMinEta = cms.untracked.double(-10),
        MomMaxEta = cms.untracked.double(10),
    DaughterIDs = cms.untracked.vint32(-321, 211),
    NumberDaughters = cms.untracked.int32(2),
    NumberDescendants = cms.untracked.int32(0),
)

ProductionFilterSequence = cms.Sequence(generator*partonfilter*D0Daufilter)
