from CRABClient.UserUtilities import config

config = config()
config.General.requestName = "dplus_minimal_tiny_crab_smoke_20260622_2323"
config.General.workArea = "/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak/crab-smoke/crab-projects"
config.General.transferOutputs = True
config.General.transferLogs = True

config.JobType.pluginName = "Analysis"
config.JobType.psetName = "/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak/crab-smoke/dplus_minimal_tiny_crab_smoke_20260622_2323/forest_crab_cfg.py"
config.JobType.outputFiles = ["HiForest_2023PbPbUPC_Jan24Reco_smoke.root"]
config.JobType.inputFiles = ["/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak/crab-smoke/dplus_minimal_tiny_crab_smoke_20260622_2323/inst_lumi.json"]
config.JobType.allowUndistributedCMSSW = True
config.JobType.maxMemoryMB = 2500
config.JobType.maxJobRuntimeMin = 1250

config.Data.outputPrimaryDataset = "HIForward0DplusForestSmoke"
config.Data.userInputFiles = [
    "/store/user/evzhang/codex-d0-crab-smoke/tiny-input/hiforward0_2023_miniaod_first100.root",
]
config.Data.splitting = "FileBased"
config.Data.unitsPerJob = 1
config.Data.publication = False
config.Data.outputDatasetTag = "dplus_minimal_tiny_crab_smoke_20260622_2323"
config.Data.outLFNDirBase = "/store/user/evzhang/codex-dplus-crab-smoke"
config.Data.ignoreLocality = True

config.Site.storageSite = "T3_CH_CERNBOX"
config.Site.whitelist = ["T2_US_Vanderbilt", "T2_CH_CERN"]
