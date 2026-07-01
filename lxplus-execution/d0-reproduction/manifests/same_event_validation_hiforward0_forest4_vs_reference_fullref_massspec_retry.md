# Same-Event D0 Validation: hiforward0_forest4_vs_reference_fullref_massspec_retry

<!-- DOC_OWNER: cms-analysis-manager exact forest/reference validation. -->
<!-- TOKEN_NOTE: detailed stdout is in the log; CSVs in the run directory are the durable diagnostics. -->

## Inputs

- Forest: `root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/jdlang/Run3_PbPbUPC/Forest_2023_Jan2024ReReco_2025Reforest/HIForward0/crab_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward0/260212_170406/0000/HiForest_2023PbPbUPC_Jan24Reco_4.root`
- Reference skim: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward0_Drej-pasor_Trig-4.root`
- Reference entries scanned: `0`

## Outputs

- Run directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/same-event-validation/hiforward0_forest4_vs_reference_fullref_massspec_retry`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/same_event_validation_hiforward0_forest4_vs_reference_fullref_massspec_retry_20260616T114939Z.log`

## Event Overlap

```csv
metric,value
forest_entries,7772
forest_unique_events,7772
forest_total_candidates,416868
forest_max_dsize,13564
reference_entries_total,9784506
reference_entries_scanned,9784506
reference_rows_with_d_gt_0,266960
reference_total_candidates_scanned,849561
matched_events,7772
missing_reference_events,0
duplicate_matched_reference_keys,0
dsize_matched_events,4388
dsize_mismatched_events,3384
same_index_compared_candidates,3
same_index_perfect_candidates,3
reference_flag_formula_mismatches,71
```

## Stage Counts

```csv
stage,label,forest_count,reference_formula_count,reference_flag_and_formula_stage_count,reference_flag_final_count,same_index_stage_mismatch_count
stage00_all,"all candidates",416868,613,117,0,0
stage01_mass,"|m-mD0|<0.2",204154,308,54,0,0
stage02_high_purity,"track high purity",204154,308,54,0,0
stage03_track_eta,"track |eta|<2.4",204154,308,54,0,0
stage04_track_pt,"track pT>1",5310,142,54,0,0
stage05_rel_pt_err,"track rel pT err<0.1",5260,138,54,0,0
stage06_dpt,"2<D pT<=12",3753,116,46,0,0
stage07_dy,"|D y|<=2",3651,116,46,0,0
stage08_alpha,"binned alpha",468,81,46,0,0
stage09_svpv_sig,"binned SVPV sig",64,56,46,0,0
stage10_sv_prob,"SV prob>0.1",60,52,46,0,0
stage11_dtheta,"binned dtheta",46,46,46,117,0
```
