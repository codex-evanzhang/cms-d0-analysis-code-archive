# 2026 PbPb
# CMSSW_16_1_X
# ZeroBias

from CRABClient.UserUtilities import config
from CRABClient.UserUtilities import getUsername
username = getUsername()

###############################################################################
# INPUT/OUTPUT SETTINGS

pd = '404549'
jobTag = 'PbPb_privateReco_' + pd
cmsswConfig = 'forest_CMSSWConfig_Run3_161X_2026PbPb_DATA.py'

isOnDAS = False

# These are ignored when isOnDAS = False
input = ''
inputDatabase = 'global'

# Use this file list instead
inputFilelist = 'filelist_privateReco.txt'

# Use to output to Vanderbilt
# output = '/store/user/' + username + '/HiForest/2026PbPbUPC_PromptReco/'
# outputServer = 'T2_US_Vanderbilt'

# Use to output to CERN EOS
output = '/store/group/phys_heavyions/' + username + '/Run3_2026PbPb/'
outputServer = 'T2_CH_CERN'

###############################################################################

config = config()

config.General.requestName = jobTag
config.General.workArea = 'CrabWorkArea'
config.General.transferOutputs = True

config.JobType.psetName = cmsswConfig
config.JobType.pluginName = 'Analysis'
config.JobType.maxMemoryMB = 2500
config.JobType.pyCfgParams = ['noprint']
config.JobType.allowUndistributedCMSSW = True
config.JobType.inputFiles = [ # Added by Evan
    'inst_lumi.json'
]

if isOnDAS :
    config.Data.inputDataset = input
    config.Data.inputDBS = inputDatabase
    config.Data.lumiMask = '/eos/user/c/cmsdqm/www/CAF/certification/Collisions25pO/pO_golden.json'
    config.Data.splitting = 'EventAwareLumiBased'
    config.Data.unitsPerJob = 10000
    config.Data.totalUnits = -1
else :
    config.Data.outputPrimaryDataset = jobTag
    config.Data.userInputFiles = open(inputFilelist).readlines()
    config.Data.splitting = 'FileBased'
    config.Data.unitsPerJob = 1
    config.Data.totalUnits = -1

config.Data.outLFNDirBase = output
config.Data.publication = False
config.Data.allowNonValidInputDataset = True

config.Site.storageSite = outputServer
config.Site.whitelist = ['T2_US_Vanderbilt', 'T2_CH_CERN']
