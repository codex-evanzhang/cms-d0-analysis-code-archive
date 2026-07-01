# Runs

<!-- DOC_OWNER: cms-analysis-manager human run notes. -->
<!-- TOKEN_NOTE: registry.yaml is authoritative for structured run metadata. -->

## Current Status

- Active task: reproduce a CMS D0 mass-peak analysis.
- 2026-06-18 full one-file MINIAOD chain runs end-to-end with the 2023
  offline ZDC mode: MINIAOD -> recreated HiForest/Dfinder/ZDC -> selected
  skim -> D0 mass plots. D-candidate AN cutflow matches the official forest
  from the track-pT stage onward, including `87/87` final no-mass-window
  candidates. The produced peak is still sparse because it uses one MINIAOD
  shard: `21` selected candidates and `17` plotted mass-window entries.
- 2026-06-18 ZDC backport update: the previous hardcoded `TS2-TS1` restored
  the correct event topology but was high in absolute scale. Same-event
  official/recreated slopes over 6326 matched events were `0.915746`
  (`sumPlus`) and `0.876579` (`sumMinus`). The current CMSSW_16 helper applies
  these as named 2023-offline side-scale factors on top of the restored
  hardcoded sums. A fresh 500-event validation gives official/recreated slopes
  `1.00017` and `1.00033`, with `lt OR 1500` and nominal `0nXn` agreement
  both `52/52`. Exact float equality is still not claimed; max residual side
  sum differences in that smoke were about `200 GeV`.
- 2026-06-18 medium-scale preflight:
  `output/preflight-medium-scale-20260618/README.md` now collects the
  a-priori justification checks. The medium-scale validation is justified with
  caveats: ZDC side-scale decisions close on `6326/6326` matched events, ZDC
  thresholds are AN-calibrated at plus `1100` and minus `1000` GeV and
  validated with data/EmptyBX peak/noise checks, HF blind thresholds
  are `9.1/8.6` GeV against nominal `9.2/8.6`, topology sidebands are derived
  from MC resolution and match the AN, and topology floors/pointing support
  pass in `6/6` bins. Remaining AN/specification-driven items are trigger
  mapping, daughter-track numeric cuts, final mass-fit model, and exact
  bin/scope choices.
- 2026-06-18 analysis-settings forest smoke:
  `hiforward0_analysis_settings_smoke_20260618` ran 100 2023 HIForward0
  MINIAOD events on LXPLUS with superfilter on, HC2023 corrected ZDC mode,
  muons on, and PF jets off. Output forest:
  `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-forest-2023/analysis-settings-smoke/hiforward0_analysis_settings_smoke_20260618/HiForest_2023PbPbUPC_Jan24Reco_smoke.root`.
  Schema validation passed with 11 selected forest events; `Dsize` summed to
  1533 over 7 D-candidate events; 9 events pass nominal `0nXn`. Same-event
  official shard 428 validation has zero `Dsize` mismatches and exact staged
  D-candidate cutflow closure through final no-mass-window `2/2`.
- 2026-06-18 analysis-minimal forest smoke:
  `hiforward0_minimal_analysis_smoke_20260618` ran 100 2023 HIForward0
  MINIAOD events with `--minimal-analysis`: muon output off, ZDC digi waveform
  output off, PF jets off, then ROOT-pruned to the Section 5/7 branch set.
  Final forest:
  `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-forest-2023/minimal-analysis-smoke/hiforward0_minimal_analysis_smoke_20260618/HiForest_2023PbPbUPC_Jan24Reco_smoke.root`.
  Raw pre-pruned file is `840K`; minimal file is `81K`. Minimal validator
  passed against raw values; normal selected-skim scripts completed with
  Section 5 `8` events, `78` candidates, and Section 7 `0` candidates on the
  tiny 11-entry post-superfilter smoke.
- 2026-06-18 CRAB tiny-input minimal smoke:
  `260618_215454:evzhang_crab_d0_minimal_tiny_crab_smoke_20260618_2155`
  ran one CRAB job at `T2_CH_CERN` from the physically bounded 100-event
  MINIAOD file
  `/eos/user/e/evzhang/codex-d0-crab-smoke/tiny-input/hiforward0_2023_miniaod_first100.root`.
  This avoids the prior CRAB `maxEvents` ambiguity by setting `maxEvents=-1`
  on an already-trimmed input. The FJR reports `EventsRead=100`; runtime was
  `0:02:23`, max memory `1006 MB`, and output appeared at
  `/eos/user/e/evzhang/codex-d0-crab-smoke/HIForward0D0ForestSmoke/d0_minimal_tiny_crab_smoke_20260618_2155/260618_215454/0000/HiForest_2023PbPbUPC_Jan24Reco_smoke_1.root`.
  Post-run pruning produced
  `HiForest_2023PbPbUPC_Jan24Reco_smoke_1_minimal.root` with 11 post-superfilter
  entries and the minimal validator passed. CRAB status was still reporting
  `transferring` when the readable output/log tarball were already present.
- 2026-06-18 CRAB 100-file HIForward0 run submitted:
  `260618_223033:evzhang_crab_d0_minimal_raw_100files_hiforward0_20260618`.
  Input manifest:
  `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/manifests/d0_minimal_raw_100files_hiforward0_20260618_inputs.txt`.
  Local copy and provenance note:
  `research/cms-d0-analysis/output/crab-100file-hiforward0-20260618/`.
  Config/manifest directory:
  `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/crab-smoke/d0_minimal_raw_100files_hiforward0_20260618`.
  CRAB project:
  `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/crab-smoke/crab-projects/crab_d0_minimal_raw_100files_hiforward0_20260618`.
  Settings: 100 HIForward0 Jan2024 MINIAOD files, raw CRAB output,
  `--minimal-analysis`, `maxEvents=-1`, `FileBased`, `unitsPerJob=1`,
  `T2_CH_CERN`, output LFN `/store/user/evzhang/codex-d0-crab-smoke`.
  Immediate post-submit status was `WAITING on command SUBMIT`; no completion
  wait was performed.
  Status check on 2026-06-19: `crab status --proxy /tmp/x509up_u189459`
  reports server `SUBMITTED`, scheduler `FAILED`, with 98/100 jobs finished
  and jobs 97/98 failed with exit code `50662`. Verbose CRAB error says the
  failed jobs were not retried because of excessive worker-node disk usage.
  EOS contains 98 ROOT outputs in
  `/eos/user/e/evzhang/codex-d0-crab-smoke/HIForward0D0ForestSmoke/d0_minimal_raw_100files_hiforward0_20260618/260618_223033/0000`,
  size about `340G`; missing output job IDs are `97` and `98`.
- 2026-06-16 same-event validation supersedes the earlier mixed-sample
  Dfinder comparison. On one official HIForward0 forest file, all 8,607 forest
  events match the reference skim event keys; no-mass-window selection gives
  140 candidates in both forest formula and reference formula.
- Prior context: seven 2026 runs `510, 511, 523, 527, 529, 543, 549` and
  a 2025-vs-2026 comparison.
- The prior comparison should inform setup, but the new reproduction should be
  scoped from the reference analysis, datasets, code, and expected plots.

## Run Notes

| Run | Year | Status | Notes |
| --- | --- | --- | --- |
| 510 | 2026 | prior context | Seven-run comparison input |
| 511 | 2026 | prior context | Seven-run comparison input |
| 523 | 2026 | prior context | Seven-run comparison input |
| 527 | 2026 | prior context | Seven-run comparison input |
| 529 | 2026 | prior context | Seven-run comparison input |
| 543 | 2026 | prior context | Seven-run comparison input |
| 549 | 2026 | prior context | Seven-run comparison input |
