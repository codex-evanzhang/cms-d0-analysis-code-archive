from CRABClient.UserUtilities import config

config = config()
config.General.requestName = "d0_forest_crab_smoke_cern_compute_20260618_1810"
config.General.workArea = "/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/crab-smoke/crab-projects"
config.General.transferOutputs = True
config.General.transferLogs = True

config.JobType.pluginName = "Analysis"
config.JobType.psetName = "/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/crab-smoke/d0_forest_crab_smoke_cern_compute_20260618_1810/forest_crab_cfg.py"
config.JobType.outputFiles = ["HiForest_2023PbPbUPC_Jan24Reco_smoke.root"]
config.JobType.inputFiles = ["/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/crab-smoke/d0_forest_crab_smoke_cern_compute_20260618_1810/inst_lumi.json"]
config.JobType.allowUndistributedCMSSW = True
config.JobType.maxMemoryMB = 2500
config.JobType.maxJobRuntimeMin = 1250

config.Data.outputPrimaryDataset = "HIForward0D0ForestSmoke"
config.Data.userInputFiles = [
    "/store/hidata/HIRun2023A/HIForward0/MINIAOD/16Jan2024-v1/2810000/3cb20bb5-a7b1-43a2-8025-9807f4f51e71.root",
    "/store/hidata/HIRun2023A/HIForward0/MINIAOD/16Jan2024-v1/2810000/0640e99d-84f5-49fd-a012-48be69252e99.root",
]
config.Data.splitting = "FileBased"
config.Data.unitsPerJob = 1
config.Data.publication = False
config.Data.outputDatasetTag = "d0_forest_crab_smoke_cern_compute_20260618_1810"
config.Data.outLFNDirBase = "/store/user/evzhang/codex-d0-crab-smoke"
config.Data.ignoreLocality = True

config.Site.storageSite = "T3_CH_CERNBOX"
config.Site.whitelist = ["T2_CH_CERN"]
