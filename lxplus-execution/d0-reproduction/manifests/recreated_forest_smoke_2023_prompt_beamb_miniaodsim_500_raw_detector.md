# 2023 Forest Smoke Reproduction: prompt_beamb_miniaodsim_500_raw_detector

<!-- DOC_OWNER: cms-analysis-manager forest smoke manifest. -->
<!-- TOKEN_NOTE: bounded cmsRun test only; full CRAB production needs approval. -->

## Inputs

- CMSSW source: `/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src`
- Base config: `/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src/HIForestSetupPbPbRun2025/forest_CMSSWConfig_Run3_132X_2023PbPb_Jan2024ReReco_UPC_MITHIG.py`
- Generated config: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-forest-smoke/prompt_beamb_miniaodsim_500_raw_detector/forest_smoke_cfg.py`
- ZDC mode: `hc2023`
- Global tag override: `141X_mcRun3_2024_realistic_HI_v14`
- Muons disabled: `true`
- Superfilter disabled: `true`
- MINIAOD input: `root://cms-xrd-global.cern.ch//store/mc/HINPbPbWinter24MiniAOD/prompt-GNucleusToD0-PhotonBeamB_Bin-Pthat0_Fil-Kpi_UPC_5p36TeV_pythia8-evtgen/MINIAODSIM/NoPU_UPC_UPC_141X_mcRun3_2024_realistic_HI_v14-v2/2810000/45f52738-a7e2-41c9-a01d-f7c62ccbbd98.root`
- Max events: `500`

## Outputs

- Smoke forest: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-forest-smoke/prompt_beamb_miniaodsim_500_raw_detector/HiForest_2023PbPbUPC_Jan24Reco_smoke.root`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_forest_smoke_prompt_beamb_miniaodsim_500_raw_detector_20260616T152302Z.log`

## Scope

- This validates that the forest + embedded Dfinder configuration can run from MINIAOD.
- It is not full production and is not a replacement for CRAB over HIForward0-19.
