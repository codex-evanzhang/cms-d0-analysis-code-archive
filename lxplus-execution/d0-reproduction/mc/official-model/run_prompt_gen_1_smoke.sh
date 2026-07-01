#!/usr/bin/env bash
set -euo pipefail
source /cvmfs/cms.cern.ch/cmsset_default.sh
export SCRAM_ARCH=el9_amd64_gcc12
mkdir -p /afs/cern.ch/work/e/evzhang/codex-work/cmssw
cd /afs/cern.ch/work/e/evzhang/codex-work/cmssw
if [[ ! -d CMSSW_14_1_7/src ]]; then
  scram p CMSSW CMSSW_14_1_7
fi
cd CMSSW_14_1_7/src
eval "$(scramv1 runtime -sh)"
mkdir -p /afs/cern.ch/work/e/evzhang/codex-work/cmssw/CMSSW_14_1_7/src/EvanAnalysis/D0OfficialMC/python
touch /afs/cern.ch/work/e/evzhang/codex-work/cmssw/CMSSW_14_1_7/src/EvanAnalysis/D0OfficialMC/__init__.py /afs/cern.ch/work/e/evzhang/codex-work/cmssw/CMSSW_14_1_7/src/EvanAnalysis/D0OfficialMC/python/__init__.py
cp /afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/mc/official-model/python/PromptGNucleusToD0PhotonBeamA_5362GeV_pythia8_evtgen_cfi.py /afs/cern.ch/work/e/evzhang/codex-work/cmssw/CMSSW_14_1_7/src/EvanAnalysis/D0OfficialMC/python/PromptGNucleusToD0PhotonBeamA_5362GeV_pythia8_evtgen_cfi.py
scram b -j 2
mkdir -p /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-smoke/prompt_gen_1
cd /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-smoke/prompt_gen_1
cmsDriver.py EvanAnalysis/D0OfficialMC/PromptGNucleusToD0PhotonBeamA_5362GeV_pythia8_evtgen_cfi.py \
  --python_filename prompt_gen_1_cfg.py \
  --fileout file:prompt_gen_1.root \
  --mc \
  --eventcontent RAWSIM \
  --datatier GEN-SIM \
  --conditions 141X_mcRun3_2024_realistic_HI_v14 \
  --beamspot DBrealistic \
  --step GEN \
  --scenario HeavyIons \
  --geometry DB:Extended \
  --era Run3_2024_UPC \
  --nThreads 1 \
  --no_exec \
  -n 1
cmsRun prompt_gen_1_cfg.py > prompt_gen_1.log 2>&1
edmEventSize -v prompt_gen_1.root > prompt_gen_1_event_size.txt 2>&1 || true
printf 'output_dir=%s\n' /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-smoke/prompt_gen_1
printf 'config=%s\n' /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-smoke/prompt_gen_1/prompt_gen_1_cfg.py
printf 'root=%s\n' /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-smoke/prompt_gen_1/prompt_gen_1.root
printf 'log=%s\n' /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-smoke/prompt_gen_1/prompt_gen_1.log
