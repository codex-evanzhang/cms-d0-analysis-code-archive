# Recreated 2023 D0 Selected Skim: crab_98forest_apriori_20260619

<!-- DOC_OWNER: cms-analysis-manager selected-skim reproduction manifest. -->
<!-- TOKEN_NOTE: concise status; detailed command output is in logs/. -->

## Inputs

- Forest source: `/eos/user/e/evzhang/codex-d0-crab-smoke/HIForward0D0ForestSmoke/d0_minimal_raw_100files_hiforward0_20260618/260618_223033/0000`
- Input list: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/manifests/hiForward0_local_forest_files.txt`
- Input file count in list: `98`
- max_files argument: `-1`
- max_events argument: `-1`
- max_candidates argument: `-1`

## Scripts

- Section 5 macro copy: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/ApplySection5EventSelection.C`
- Section 7 macro copy: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/ApplySection7DSelection.C`

## Outputs

- Section 5 recreated event/candidate skim: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/crab_98forest_apriori_20260619/section5_event_selected.root`
- Section 7 recreated selected D skim: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/crab_98forest_apriori_20260619/section7_d_selected.root`
- Plots: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/crab_98forest_apriori_20260619/plots`
- Run log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_d0_selected_skim_crab_98forest_apriori_20260619_20260619T152939Z.log`

## Scope

- This recreates the analysis-shaped selected skim from forests using documented Section 5 event cuts and Section 7 D-candidate cuts.
- It is not yet a full branch-for-branch clone of the Wang `Tree` reference skim.
- Existing reference skims remain validation references only.

## Extracted Run Summary

```text
Wrote /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/crab_98forest_apriori_20260619/section5_event_selected.root
Section 5 summary: files=98 input events=4110752 selected events=2003495 candidates in selected events=13885225
Wrote /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/crab_98forest_apriori_20260619/section7_d_selected.root
Section 7 summary: input candidates=13885225 selected candidates=3143
```
