# Recreated 2023 D0 Selected Skim: hiforward0_miniaod_fullfile_fromscratch_chain_20260616_selected

<!-- DOC_OWNER: cms-analysis-manager selected-skim reproduction manifest. -->
<!-- TOKEN_NOTE: concise status; detailed command output is in logs/. -->

## Inputs

- Forest source: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/from-miniaod-chain/forest-input-dir/hiforward0_miniaod_fullfile_fromscratch_chain_20260616`
- Input list: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/manifests/hiForward0_local_forest_files.txt`
- Input file count in list: `1`
- max_files argument: `1`
- max_events argument: `-1`
- max_candidates argument: `-1`

## Scripts

- Section 5 macro copy: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/ApplySection5EventSelection.C`
- Section 7 macro copy: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/ApplySection7DSelection.C`

## Outputs

- Section 5 recreated event/candidate skim: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/from-miniaod-chain/selected-skim/hiforward0_miniaod_fullfile_fromscratch_chain_20260616_selected/section5_event_selected.root`
- Section 7 recreated selected D skim: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/from-miniaod-chain/selected-skim/hiforward0_miniaod_fullfile_fromscratch_chain_20260616_selected/section7_d_selected.root`
- Plots: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/from-miniaod-chain/selected-skim/hiforward0_miniaod_fullfile_fromscratch_chain_20260616_selected/plots`
- Run log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_d0_selected_skim_hiforward0_miniaod_fullfile_fromscratch_chain_20260616_selected_20260616T165136Z.log`

## Scope

- This recreates the analysis-shaped selected skim from forests using documented Section 5 event cuts and Section 7 D-candidate cuts.
- It is not yet a full branch-for-branch clone of the Wang `Tree` reference skim.
- Existing reference skims remain validation references only.

## Extracted Run Summary

```text
Wrote /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/from-miniaod-chain/selected-skim/hiforward0_miniaod_fullfile_fromscratch_chain_20260616_selected/section5_event_selected.root
Section 5 summary: files=1 input events=36760 selected events=11909 candidates in selected events=65347
Wrote /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/from-miniaod-chain/selected-skim/hiforward0_miniaod_fullfile_fromscratch_chain_20260616_selected/section7_d_selected.root
Section 7 summary: input candidates=65347 selected candidates=10
```
