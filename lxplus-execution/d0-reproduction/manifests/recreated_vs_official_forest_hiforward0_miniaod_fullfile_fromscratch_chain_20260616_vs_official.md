# Recreated vs Official Forest Validation: hiforward0_miniaod_fullfile_fromscratch_chain_20260616_vs_official

<!-- DOC_OWNER: cms-analysis-manager forest validation manifest. -->
<!-- TOKEN_NOTE: summary only; detailed outputs are in the EOS output directory. -->

## Inputs

- Recreated forest: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/from-miniaod-chain/recreated-forest/hiforward0_miniaod_fullfile_fromscratch_chain_20260616/HiForest_2023PbPbUPC_Jan24Reco_smoke.root`
- Official forest: `root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/jdlang/Run3_PbPbUPC/Forest_2023_Jan2024ReReco_2025Reforest/HIForward0/crab_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward0/260212_170406/0000/HiForest_2023PbPbUPC_Jan24Reco_428.root`
- Official campaign base: `root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/jdlang/Run3_PbPbUPC/Forest_2023_Jan2024ReReco_2025Reforest/HIForward0/crab_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward0/260212_170406`

## Outputs

- Output directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/from-miniaod-chain/validation/hiforward0_miniaod_fullfile_fromscratch_chain_20260616_vs_official`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/recreated_vs_official_forest_hiforward0_miniaod_fullfile_fromscratch_chain_20260616_vs_official_20260616T164946Z.log`
- Matcher CSV: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/from-miniaod-chain/validation/hiforward0_miniaod_fullfile_fromscratch_chain_20260616_vs_official/official_match_search.csv`
- Comparator manifest: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/from-miniaod-chain/validation/hiforward0_miniaod_fullfile_fromscratch_chain_20260616_vs_official/manifest.md`

## Stage Counts

```csv
stage,label,recreated_candidates,official_candidates,delta
stage00_all,"all candidates",448512,448511,1
stage01_mass,"|m-mD0|<0.2",219275,219273,2
stage02_high_purity,"track high purity",219275,219273,2
stage03_track_eta,"track |eta|<2.4",219275,219273,2
stage04_track_pt,"track pT>1",5529,5529,0
stage05_rel_pt_err,"track rel pT err<0.1",5457,5457,0
stage06_dpt,"2<D pT<=12",3998,3998,0
stage07_dy,"|D y|<=2",3887,3887,0
stage08_alpha,"binned alpha",473,473,0
stage09_svpv_sig,"binned SVPV sig",51,51,0
stage10_sv_prob,"SV prob>0.1",46,46,0
stage11_dtheta,"binned dtheta",35,35,0
final_no_mass_window,"final cuts except mass window",87,87,0
```

## Status

- Comparator exit status: `1`
