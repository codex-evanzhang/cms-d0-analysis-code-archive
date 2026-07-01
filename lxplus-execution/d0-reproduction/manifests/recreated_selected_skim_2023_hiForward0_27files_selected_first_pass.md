# Recreated 2023 D0 Selected Skim: hiForward0_27files_selected_first_pass

<!-- DOC_OWNER: cms-analysis-manager selected-skim reproduction manifest. -->
<!-- TOKEN_NOTE: concise status; detailed command output is in logs/. -->

## Inputs

- Forest source: `/eos/user/e/evzhang/codex-eos/datasets/vanderbilt_HIForward0_251227_162520_0000_0006`
- Input list: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/manifests/hiForward0_local_forest_files.txt`
- Input file count in list: `27`
- max_files argument: `-1`
- max_events argument: `-1`
- max_candidates argument: `-1`

## Scripts

- Section 5 macro copy: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/ApplySection5EventSelection.C`
- Section 7 macro copy: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/ApplySection7DSelection.C`

## Outputs

- Section 5 recreated event/candidate skim: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/hiForward0_27files_selected_first_pass/section5_event_selected.root`
- Section 7 recreated selected D skim: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/hiForward0_27files_selected_first_pass/section7_d_selected.root`
- Plots: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/hiForward0_27files_selected_first_pass/plots`
- Run log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_d0_selected_skim_hiForward0_27files_selected_first_pass_20260615T195748Z.log`

## Scope

- This recreates the analysis-shaped selected skim from forests using documented Section 5 event cuts and Section 7 D-candidate cuts.
- It is not yet a full branch-for-branch clone of the Wang `Tree` reference skim.
- Existing reference skims remain validation references only.

## Extracted Run Summary

```text
Wrote /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/hiForward0_27files_selected_first_pass/section5_event_selected.root
Section 5 summary: files=27 input events=9597325 selected events=5769934 candidates in selected events=9843575
Wrote /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/hiForward0_27files_selected_first_pass/section7_d_selected.root
Section 7 summary: input candidates=9843575 selected candidates=1348
```
