# Recreated vs Official Forest Validation: hiforward0_miniaod_20000_fromscratch_chain_20260616_v2_vs_official

<!-- DOC_OWNER: cms-analysis-manager forest validation manifest. -->
<!-- TOKEN_NOTE: summary only; detailed outputs are in the EOS output directory. -->

## Inputs

- Recreated forest: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/from-miniaod-chain/recreated-forest/hiforward0_miniaod_20000_fromscratch_chain_20260616_v2/HiForest_2023PbPbUPC_Jan24Reco_smoke.root`
- Official forest: `root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/jdlang/Run3_PbPbUPC/Forest_2023_Jan2024ReReco_2025Reforest/HIForward0/crab_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward0/260212_170406/0000/HiForest_2023PbPbUPC_Jan24Reco_428.root`
- Official campaign base: `root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/jdlang/Run3_PbPbUPC/Forest_2023_Jan2024ReReco_2025Reforest/HIForward0/crab_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward0/260212_170406`

## Outputs

- Output directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/from-miniaod-chain/validation/hiforward0_miniaod_20000_fromscratch_chain_20260616_v2_vs_official`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/recreated_vs_official_forest_hiforward0_miniaod_20000_fromscratch_chain_20260616_v2_vs_official_20260616T160513Z.log`
- Matcher CSV: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/from-miniaod-chain/validation/hiforward0_miniaod_20000_fromscratch_chain_20260616_v2_vs_official/official_match_search.csv`
- Comparator manifest: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/from-miniaod-chain/validation/hiforward0_miniaod_20000_fromscratch_chain_20260616_v2_vs_official/manifest.md`

## Stage Counts

```csv
stage,label,recreated_candidates,official_candidates,delta
stage00_all,"all candidates",27988,27988,0
stage01_mass,"|m-mD0|<0.2",13749,13749,0
stage02_high_purity,"track high purity",13749,13749,0
stage03_track_eta,"track |eta|<2.4",13749,13749,0
stage04_track_pt,"track pT>1",384,384,0
stage05_rel_pt_err,"track rel pT err<0.1",382,382,0
stage06_dpt,"2<D pT<=12",284,284,0
stage07_dy,"|D y|<=2",280,280,0
stage08_alpha,"binned alpha",40,40,0
stage09_svpv_sig,"binned SVPV sig",4,4,0
stage10_sv_prob,"SV prob>0.1",4,4,0
stage11_dtheta,"binned dtheta",2,2,0
final_no_mass_window,"final cuts except mass window",3,3,0
```

## Status

- Comparator exit status: `1`
