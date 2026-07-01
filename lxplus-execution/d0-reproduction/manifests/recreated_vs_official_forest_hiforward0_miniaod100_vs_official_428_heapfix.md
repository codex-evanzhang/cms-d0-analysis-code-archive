# Recreated vs Official Forest Validation: hiforward0_miniaod100_vs_official_428_heapfix

<!-- DOC_OWNER: cms-analysis-manager forest validation manifest. -->
<!-- TOKEN_NOTE: summary only; detailed outputs are in the EOS output directory. -->

## Inputs

- Recreated forest: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-forest-2023/smoke/hiforward0_miniaod_100_d0_zdc_pf_track_hcalsev/HiForest_2023PbPbUPC_Jan24Reco_smoke.root`
- Official forest: `root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/jdlang/Run3_PbPbUPC/Forest_2023_Jan2024ReReco_2025Reforest/HIForward0/crab_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward0/260212_170406/0000/HiForest_2023PbPbUPC_Jan24Reco_428.root`
- Official campaign base: `root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/jdlang/Run3_PbPbUPC/Forest_2023_Jan2024ReReco_2025Reforest/HIForward0/crab_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward0/260212_170406`

## Outputs

- Output directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-vs-official-forest/hiforward0_miniaod100_vs_official_428_heapfix`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/recreated_vs_official_forest_hiforward0_miniaod100_vs_official_428_heapfix_20260616T134411Z.log`
- Matcher CSV: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-vs-official-forest/hiforward0_miniaod100_vs_official_428_heapfix/official_match_search.csv`
- Comparator manifest: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-vs-official-forest/hiforward0_miniaod100_vs_official_428_heapfix/manifest.md`

## Stage Counts

```csv
stage,label,recreated_candidates,official_candidates,delta
stage00_all,"all candidates",1486,1486,0
stage01_mass,"|m-mD0|<0.2",733,733,0
stage02_high_purity,"track high purity",733,733,0
stage03_track_eta,"track |eta|<2.4",733,733,0
stage04_track_pt,"track pT>1",24,24,0
stage05_rel_pt_err,"track rel pT err<0.1",24,24,0
stage06_dpt,"2<D pT<=12",17,17,0
stage07_dy,"|D y|<=2",17,17,0
stage08_alpha,"binned alpha",3,3,0
stage09_svpv_sig,"binned SVPV sig",2,2,0
stage10_sv_prob,"SV prob>0.1",2,2,0
stage11_dtheta,"binned dtheta",2,2,0
final_no_mass_window,"final cuts except mass window",2,2,0
```

## Status

- Comparator exit status: `1`
