# 2023 Forest Smoke Reproduction: prompt_miniaodsim_20_nomutree

<!-- DOC_OWNER: cms-analysis-manager forest smoke manifest. -->
<!-- TOKEN_NOTE: bounded cmsRun test only; full CRAB production needs approval. -->

## Inputs

- CMSSW source: `/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src`
- Base config: `/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src/HIForestSetupPbPbRun2025/forest_CMSSWConfig_Run3_132X_2023PbPb_Jan2024ReReco_UPC_MITHIG.py`
- Generated config: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-forest-smoke/prompt_miniaodsim_20_nomutree/forest_smoke_cfg.py`
- ZDC mode: `hc2023`
- Global tag override: `141X_mcRun3_2024_realistic_HI_v14`
- Muons disabled: `true`
- MINIAOD input: `root://cms-xrd-global.cern.ch//store/mc/HINPbPbWinter24MiniAOD/prompt-GNucleusToD0-PhotonBeamA_Bin-Pthat0_Fil-Kpi_UPC_5p36TeV_pythia8-evtgen/MINIAODSIM/NoPU_UPC_UPC_141X_mcRun3_2024_realistic_HI_v14-v2/120000/db419421-6011-4b1b-b074-fe8247bc69d4.root`
- Max events: `20`

## Outputs

- Smoke forest: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-forest-smoke/prompt_miniaodsim_20_nomutree/HiForest_2023PbPbUPC_Jan24Reco_smoke.root`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_forest_smoke_prompt_miniaodsim_20_nomutree_20260616T151549Z.log`

## Scope

- This validates that the forest + embedded Dfinder configuration can run from MINIAOD.
- It is not full production and is not a replacement for CRAB over HIForward0-19.
