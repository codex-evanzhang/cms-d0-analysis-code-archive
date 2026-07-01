# 2023 D+ Forest CRAB Smoke: dplus_minimal_tiny_crab_smoke_20260622_2323

<!-- DOC_OWNER: cms-analysis-manager D+ CRAB smoke manifest. -->
<!-- TOKEN_NOTE: tiny two-file CRAB task; full production still needs separate approval. -->

- CMSSW source: `/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src`
- Template local-smoke config: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-forest-2023/analysis-settings-smoke/hiforward0_analysis_settings_smoke_20260618/forest_smoke_cfg.py`
- CRAB PSet: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak/crab-smoke/dplus_minimal_tiny_crab_smoke_20260622_2323/forest_crab_cfg.py`
- CRAB config: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak/crab-smoke/dplus_minimal_tiny_crab_smoke_20260622_2323/crabConfig.py`
- Packaged lumi JSON: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak/crab-smoke/dplus_minimal_tiny_crab_smoke_20260622_2323/inst_lumi.json`
- Storage site: `T3_CH_CERNBOX`
- Output LFN base: `/store/user/evzhang/codex-dplus-crab-smoke`
- Physical EOS target: `/eos/user/e/evzhang/...` via CERNBOX
- Compute whitelist: `["T2_US_Vanderbilt", "T2_CH_CERN"]`
- Max CRAB memory/runtime: `2500 MB`, `1250 min`
- Max events per CRAB job: `100`
- Minimal-analysis toggles: `true`
- Splitting: `FileBased`, `unitsPerJob = 1`
- Input files: `1`

  - `/store/user/evzhang/codex-d0-crab-smoke/tiny-input/hiforward0_2023_miniaod_first100.root`
