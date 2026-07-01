import FWCore.ParameterSet.Config as cms

from HeavyIonsAnalysis.ZDCAnalysis.zdcrecoRun3_cfi import zdcrecoRun3
from HeavyIonsAnalysis.ZDCAnalysis.ZDCRecHitAnalyzerHC_cfi import zdcanalyzer

# 2023 Jan ReReco configs refer to this historical label. The available CMSSW
# checkout no longer ships the wrapper, but it does contain the Run 3 ZDC
# producer and the hard-code analyzer/plugin used by the old forest config.
zdcreco2023HardCode = zdcrecoRun3.clone()

zdcanalyzer.ZDCRecHitSource = cms.InputTag("zdcreco2023HardCode")
zdcanalyzer.AuxZDCRecHitSource = cms.InputTag("zdcreco2023HardCode")

zdcSequence = cms.Sequence(zdcreco2023HardCode + zdcanalyzer)
