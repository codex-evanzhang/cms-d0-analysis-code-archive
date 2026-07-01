# 2023 Forest Smoke Reproduction: hiforward0_miniaod_10_d0_zdc_pf_track_hcalsev2

<!-- DOC_OWNER: cms-analysis-manager forest smoke manifest. -->
<!-- TOKEN_NOTE: bounded cmsRun test only; full CRAB production needs approval. -->

## Inputs

- CMSSW source: `/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src`
- Base config: `/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src/HIForestSetupPbPbRun2025/forest_CMSSWConfig_Run3_132X_2023PbPb_Jan2024ReReco_UPC_MITHIG.py`
- Generated config: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-forest-2023/smoke/hiforward0_miniaod_10_d0_zdc_pf_track_hcalsev2/forest_smoke_cfg.py`
- MINIAOD input: `root://xrootd-vanderbilt.sites.opensciencegrid.org//store/hidata/HIRun2023A/HIForward0/MINIAOD/16Jan2024-v1/2810000/3cb20bb5-a7b1-43a2-8025-9807f4f51e71.root`
- Max events: `10`

## Outputs

- Smoke forest: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-forest-2023/smoke/hiforward0_miniaod_10_d0_zdc_pf_track_hcalsev2/HiForest_2023PbPbUPC_Jan24Reco_smoke.root`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_forest_smoke_hiforward0_miniaod_10_d0_zdc_pf_track_hcalsev2_20260616T130844Z.log`

## Scope

- This validates that the forest + embedded Dfinder configuration can run from MINIAOD.
- It is not full production and is not a replacement for CRAB over HIForward0-19.
