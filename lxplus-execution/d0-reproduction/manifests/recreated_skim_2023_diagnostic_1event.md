# Recreated 2023 D0 Skim: diagnostic_1event

<!-- DOC_OWNER: cms-analysis-manager skim reproduction manifest. -->
<!-- TOKEN_NOTE: concise status; detailed command output is in logs/. -->

## Inputs

- Forest source: `/eos/user/e/evzhang/codex-eos/datasets/vanderbilt_HIForward0_251227_162520_0000_0006`
- Input list: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/manifests/hiForward0_local_forest_files.txt`
- Input file count in list: `27`
- max_files argument: `1`
- max_events argument: `1`

- compute_hf argument: `0`

## Outputs

- Recreated skim ROOT: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-skim-2023/diagnostic_1event/recreated_d0_micro_skim_HIForward0.root`
- Plots/summary directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-skim-2023/diagnostic_1event/plots`
- Run log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_d0_skim_recreation_diagnostic_1event_20260615T195200Z.log`

## Reducer Scope

- Reads aligned trees: `hiEvtAnalyzer/HiTree`, `PbPbTracks/trackTree`, `skimanalysis/HltTree`, `l1MetFilterRecoTree/MetFilterRecoTree`, `zdcanalyzer/zdcrechit`, `particleFlowAnalyser/pftree`, `Dfinder/ntDkpi`.
- Reads `hltanalysis/HltTree` by `(run,lumi,event)` lookup because it is not entry-aligned in the checked forest batch.
- Copies event metadata, ZDC/HF quantities, key Dfinder branches, and computes `DpassCut23PAS` from the AN Table 7 rectangular cuts.

## Current Caveats

- This is a first-pass analysis skim subset, not yet a bit-for-bit clone of the Wang reference skim.
- `DpassCutNominal` is temporarily set equal to recreated `DpassCut23PAS` until the exact nominal-cut definition is recovered.
- `clusComp_quality` and `nTrackInAcceptanceHP` are placeholders in this first pass.
- `HFEMaxPlus`, `HFEMaxMinus`, and derived gap flags are placeholders unless `--compute-hf` was used.
- HLT trigger branches are filled only when the separate `hltanalysis/HltTree` contains a matching event key.

## Summary Metrics

```tsv
metric	value
input_files	1
chain_events	368603
processed_events	1
hlt_rows_read	7532
hlt_unique_events	7532
hlt_duplicate_rows	0
hlt_matched_events	1
hlt_zdc_or_events	0
input_dfinder_candidates	0
copied_dfinder_candidates	0
pass_cut23_pas_candidates	0
dsize_truncated_by_reader	0
compute_hf	0
output_root	/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-skim-2023/diagnostic_1event/recreated_d0_micro_skim_HIForward0.root
```
