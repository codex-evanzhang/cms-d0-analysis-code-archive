# Commands

<!-- DOC_OWNER: cms-analysis-manager reproducible command recipes. -->
<!-- TOKEN_NOTE: promote repeated command sequences into scripts/cms-analysisctl subcommands. -->

Use local controller helpers first:

```bash
/home/ubuntu/agent-network/scripts/cms-analysisctl status
/home/ubuntu/agent-network/scripts/cms-analysisctl ledger
/home/ubuntu/agent-network/scripts/cms-analysisctl gallery
/home/ubuntu/agent-network/scripts/cms-analysisctl summary
```

Regenerate the current AN-shaped D0 reproduction draft, promote it to
`general-codex-output`, and push/check Overleaf:

```bash
/home/ubuntu/agent-network/research/cms-d0-analysis/scripts/run_full_d0_an_recreation_workflow.sh d0_an_full_LABEL
```

Detached form:

```bash
/home/ubuntu/agent-network/research/cms-d0-analysis/scripts/run_full_d0_an_recreation_workflow.sh --detached d0_an_full_LABEL
```

If LXPLUS is flaky but a current MC proxy bundle already exists locally, skip
only that stage:

```bash
SKIP_MC_PROXY=true /home/ubuntu/agent-network/research/cms-d0-analysis/scripts/run_full_d0_an_recreation_workflow.sh d0_an_full_LABEL
```

If the official-reference proxy bundle already exists locally, skip only that
stage:

```bash
SKIP_OFFICIAL_PROXY=true /home/ubuntu/agent-network/research/cms-d0-analysis/scripts/run_full_d0_an_recreation_workflow.sh d0_an_full_LABEL
```

This wrapper checks LXPLUS and VOMS, regenerates recreated-data proxy plots
from the 98-forest selected outputs, regenerates prompt-MC/data proxy plots
for missing AN targets, regenerates official-reference trigger/DCA/systematic
proxy plots for AN slots that need branches absent from the 98-forest selected
skim, rebuilds the LaTeX draft from durable artifact paths, and syncs the result
to Overleaf.

Regenerate only the prompt-MC/data proxy plots:

```bash
MAX_EVENTS=2000000 /home/ubuntu/agent-network/research/cms-d0-analysis/scripts/run_missing_an_mc_proxy_plots.sh missing_an_mc_proxy_LABEL
```

Regenerate official-reference proxy plots for AN slots that need branches absent
from the 98-forest selected skim, including trigger-bit occupancy,
candidate-level trigger-efficiency proxies, DCA prompt-fraction templates, and
D-selection systematic variation panels:

```bash
MAX_DATA_EVENTS=5000000 MAX_MC_EVENTS=5000000 \
  /home/ubuntu/agent-network/research/cms-d0-analysis/scripts/run_missing_an_official_proxy_plots.sh \
  missing_an_official_proxy_LABEL
```

These plots are useful AN-recreation scaffolding, but they must stay labeled as
official-reference/proxy outputs unless the same observables are regenerated
from the reproduced MINIAOD-derived chain.

Regenerate official MC sample provenance tables from stored McM snapshots:

```bash
/home/ubuntu/agent-network/research/cms-d0-analysis/scripts/build_official_mc_sample_tables.py
```

Launch the standard detached AN gap-push jobs, with one log per job and an
input-class manifest:

```bash
/home/ubuntu/agent-network/research/cms-d0-analysis/scripts/launch_d0_an_gap_push_detached.sh d0_an_gap_push_LABEL
```

This currently launches ZDC multipeak fits, ZDC 1n threshold refits, HF gap
derivation, independent-sideband topology, BDT topology, MC proxy plots, and
official-reference proxy plots. It writes:

```text
research/cms-d0-analysis/output/an-gap-push/d0_an_gap_push_LABEL/jobs.tsv
research/cms-d0-analysis/output/an-gap-push/d0_an_gap_push_LABEL/logs/
```

Build raw-yield/final-normalization scaffolding without claiming a final cross
section:

```bash
.venv-plotting/bin/python \
  /home/ubuntu/agent-network/research/cms-d0-analysis/scripts/build_normalization_scaffold.py \
  --label normalization_scaffold_LABEL
```

Build FONLL/theory-comparison scaffolding without claiming final theory curves:

```bash
.venv-plotting/bin/python \
  /home/ubuntu/agent-network/research/cms-d0-analysis/scripts/build_theory_comparison_scaffold.py \
  --label theory_scaffold_LABEL
```

Build private-MC pThat stitching scaffolding without claiming a stitched MC
result:

```bash
.venv-plotting/bin/python \
  /home/ubuntu/agent-network/research/cms-d0-analysis/scripts/build_pthat_stitching_scaffold.py \
  --label pthat_stitching_scaffold_LABEL
```

Build raw-yield mass-fit QA plots and tables without claiming a final corrected
signal-extraction result:

```bash
.venv-plotting/bin/python \
  /home/ubuntu/agent-network/research/cms-d0-analysis/scripts/build_final_yield_fit_summary.py \
  --label final_yield_fit_summary_LABEL
```

Run the TWiki official reduced-tree ZDC 1n/2n/3n peak-fit proxy and raw-channel
branch audit:

```bash
MAX_EVENTS=30000000 \
  /home/ubuntu/agent-network/research/cms-d0-analysis/scripts/run_zdc_multipeak_fits.sh \
  zdc_multipeak_LABEL
```

Run the HF gap derivation through the reusable wrapper:

```bash
MAX_EVENTS=-1 \
  /home/ubuntu/agent-network/research/cms-d0-analysis/scripts/run_hf_gap_derivation.sh \
  hf_gap_LABEL
```

Generate the requested 2026 signed-rapidity and pT-binned mass plots, then
rewrite the reusable general output project:

```bash
/home/ubuntu/agent-network/research/cms-d0-analysis/scripts/make_2026_pt_y_mass_plots.py --sync
```

Plot generation, plot script edits, gallery updates, and plot construction
details route through `cms-plotting-producer`.

Reviewed task:

```bash
/home/ubuntu/agent-network/scripts/run-cms-reviewed-task.sh /home/ubuntu/agent-network/work/tasks/TASK.md LABEL
```

Recreate a bounded 2023 MINIAOD-to-HiForest smoke with Dfinder and ZDC, then
validate the core schema:

```bash
/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_forest_smoke.sh \
  --max-events 100 \
  --label hiforward0_miniaod_100_d0_zdc_pf_track_hcalsev

cd /afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts
root -l -b -q 'validate_recreated_forest_schema.C("/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-forest-2023/smoke/hiforward0_miniaod_100_d0_zdc_pf_track_hcalsev/HiForest_2023PbPbUPC_Jan24Reco_smoke.root")'
```

Compare a recreated MINIAOD forest to the official Dfinder forest shard with
the same event keys:

```bash
/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_recreated_vs_official_forest_validation.sh \
  --recreated /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-forest-2023/smoke/hiforward0_miniaod_100_d0_zdc_hc2023_filtersource/HiForest_2023PbPbUPC_Jan24Reco_smoke.root \
  --official root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/jdlang/Run3_PbPbUPC/Forest_2023_Jan2024ReReco_2025Reforest/HIForward0/crab_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward0/260212_170406/0000/HiForest_2023PbPbUPC_Jan24Reco_428.root \
  --label hiforward0_miniaod100_hc2023_vs_official_428
```

Current result: Dfinder candidate counts, staged cutflow, and D0 mass peak
match exactly on the 10 overlapping events. The unresolved mismatch is ZDC
event filtering/calibration: the recreated forest keeps one extra event,
`375055:61:10988872`, and official ZDC sums differ strongly from the available
local ZDC modes. Recover the official Run3 ZDC backport/PSet before claiming
full event-filter reproduction.

Run the current practical full one-file chain from MINIAOD to mass plots:

```bash
/home/ubuntu/agent-network/research/cms-d0-analysis/scripts/run_from_miniaod_mass_chain.sh \
  --max-events -1 \
  --label hiforward0_miniaod_fullfile_zdc_offline2023_chain_20260618
```

Current result for the 2026-06-18 label: recreated forest `3.5G`, `36760`
forest events, `19477` Section 5 selected events, `127032` candidates in those
events, `21` Section 7 selected candidates, and `17` plotted mass-window
candidates. Validation against official shard 428 has `6326` matched events.
The D-candidate staged cutflow agrees from track-pT onward, including `87/87`
final no-mass-window candidates. ZDC decisions are close but not exact:
nominal `0nXn` agreement is `6314/6326`, and `lt OR 1500` agreement is
`6318/6326`; exact rechit/digi sums do not match. The one file is a chain
validation and sparse diagnostic peak, not final statistics.

Older pre-2023-offline-ZDC full-chain label retained for comparison:

```bash
/home/ubuntu/agent-network/research/cms-d0-analysis/scripts/run_from_miniaod_mass_chain.sh \
  --max-events -1 \
  --label hiforward0_miniaod_fullfile_fromscratch_chain_20260616
```

Current result for that label: recreated forest `3.5G`, `36760` recreated
forest events, `11909` Section 5 selected events, `65347` candidates in those
events, `10` Section 7 selected candidates, and `8` plotted mass-window
candidates. Validation against official shard 428 has `6326` matched events;
from track-pT onward the staged candidate cutflow agrees exactly, including
`87/87` final no-mass-window candidates. The comparator returns nonzero because
of tiny numerical candidate differences and two `Dsize` mismatches. The one
file is a chain validation, not enough statistics for a final peak; use
batch/CRAB-style multi-file production next.

Derive provisional D0 cuts from prompt MC, 2023 data sidebands, and EmptyBX
control on LXPLUS:

```bash
/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_cut_derivation.sh \
  --label prompt_mc_data_sideband_emptybx_200k \
  --max-events 200000
```

Output:

```text
/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/prompt_mc_data_sideband_emptybx_200k
```

Do not freeze final cuts from bounded scans. Use them to validate the pipeline,
compare against nominal cuts, and decide which full-statistics scans are worth
running.

Run the current full-stat HF gap cutoff derivation:

```bash
scp research/cms-d0-analysis/scripts/derive_hf_gap_cutoffs_data_mc.C \
  lxplus-codex:/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/derive_hf_gap_cutoffs_data_mc.C

scripts/lxplusctl ssh 'set -e; source /cvmfs/cms.cern.ch/cmsset_default.sh; OUT=/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/hf_gap_data_mc_full_blind_20260618; mkdir -p "$OUT"; cd /afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts; root -l -b -q "derive_hf_gap_cutoffs_data_mc.C++(\"/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root\",\"/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root\",\"/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root\",\"$OUT\",-1)"'
```

Current blind result: `HFEMaxPlus < 9.1 GeV` and `HFEMaxMinus < 8.6 GeV`.
The AN-compatible `HFEMaxPlus < 9.2 GeV` point is a posthoc plateau/closure
point, not a derivation input.
Local output:
`research/cms-d0-analysis/output/cut-derivation/hf_gap_data_mc_full_blind_20260618`.

Regenerate the selection-independence audit:

```bash
research/cms-d0-analysis/scripts/audit_selection_independence.py
```

Run the full-stat rectangular topology derivation with the AN Section 7.1
sideband definition:

```bash
OUT=/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/topocuts_rect_an_sideband_full_20260617
mkdir -p "$OUT"
root -l -b -q \
  '/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/derive_2023_d0_topology_cuts_an_sideband.C+("/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root","/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root","/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root","'"$OUT"'",-1)'
```

Postprocess locally:

```bash
.venv-plotting/bin/python \
  research/cms-d0-analysis/scripts/summarize_topological_cut_scan.py \
  research/cms-d0-analysis/output/cut-derivation/topocuts_rect_an_sideband_full_20260617
```

Current result: the blind score optimum does not reproduce the AN cuts because
it prefers looser `DsvpvSig` and `Dchi2cl` cuts. The AN-guided rectangular
method does reproduce all six AN bins: fix `DsvpvSig > 2.5` for `2 < pT < 5`,
`DsvpvSig > 3.5` for higher `pT`, and `Dchi2cl > 0.1`, then accept
`Dalpha`/`Ddtheta` choices on the constrained score plateau. Main local output:
`research/cms-d0-analysis/output/cut-derivation/topocuts_rect_an_sideband_full_20260617`.

Run the MC-background discrepancy check, using truth-matched MC prompt D0 as
signal and non-truth-matched MC candidates in the same AN mass sidebands as
background:

```bash
OUT=/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/topocuts_mc_background_full_20260617
mkdir -p "$OUT"
root -l -b -q \
  '/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/derive_2023_d0_topology_cuts_mc_background.C+("/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root","unused","unused","'"$OUT"'",-1)'
```

Postprocess locally:

```bash
.venv-plotting/bin/python \
  research/cms-d0-analysis/scripts/summarize_topological_cut_scan.py \
  research/cms-d0-analysis/output/cut-derivation/topocuts_mc_background_full_20260617 \
  --background-label 'non-truth-matched MC candidates in the AN mass sidebands' \
  --background-axis-label 'nonmatched MC sideband fraction passing'
```

Current result: this does not fix the raw-score discrepancy. The raw optimum
still chooses looser vertex-displacement and vertex-fit cuts. With the same
AN-guided floors, the nominal cuts remain on the constrained plateau in all
six bins. Main local output:
`research/cms-d0-analysis/output/cut-derivation/topocuts_mc_background_full_20260617`.

Derive the `DsvpvSig` and `Dchi2cl` rectangular quality floors without using
the AN floor values as inputs:

```bash
.venv-plotting/bin/python \
  research/cms-d0-analysis/scripts/derive_topological_floor_justification.py
```

Current result: using loose `Dalpha`/`Ddtheta`, the script first chooses the
lowest `Dchi2cl` floor that gives measurable nonmatched-MC background
rejection with small MC signal loss, then chooses the lowest `DsvpvSig` floor
that gives at least one-third nonmatched-MC background rejection, visible
data-sideband rejection, and majority MC signal retention. This reproduces the
AN floors in all six bins. Main output:
`research/cms-d0-analysis/output/cut-derivation/topological_floor_justification_20260618`.

Justify the `Dalpha` and `Ddtheta` pointing cuts after fixing the independently
derived floors:

```bash
.venv-plotting/bin/python \
  research/cms-d0-analysis/scripts/derive_topological_pointing_justification.py
```

Current result: the exact AN `Dalpha`/`Ddtheta` rows are accepted by a
predeclared signal-efficient, background-stable plateau rule in all six bins.
This proves plateau compatibility, not uniqueness. Main output:
`research/cms-d0-analysis/output/cut-derivation/topological_pointing_justification_20260618`.

Run the independent-sideband topology derivation, then rerun floors and
pointing from the derived sideband:

```bash
/home/ubuntu/agent-network/research/cms-d0-analysis/scripts/run_independent_sideband_topology_derivation.sh \
  --max-events -1 \
  --label 20260618
```

Current result: the MC-resolution rule derives
`0.07 < |Dmass - 1.86484| < 0.12 GeV`, matching the AN sideband posthoc. The
same floor rule matches the AN `DsvpvSig`/`Dchi2cl` floors in all six bins,
and the exact AN `Dalpha`/`Ddtheta` rows are accepted by the independent
plateau rule in all six bins. The new `pointing_rederived_representative.tsv`
contains one AN-free representative row per bin; it matches the exact AN
pointing row in `1/6` bins, so use it as a robustness/closure object rather
than claiming the AN pointing values are uniquely determined.

Main outputs:

```text
research/cms-d0-analysis/output/cut-derivation/topology_sideband_derivation_20260618
research/cms-d0-analysis/output/cut-derivation/topocuts_rect_independent_sideband_full_20260618
research/cms-d0-analysis/output/cut-derivation/topocuts_mc_background_independent_sideband_full_20260618
research/cms-d0-analysis/output/cut-derivation/topological_floor_independent_sideband_20260618
research/cms-d0-analysis/output/cut-derivation/topological_pointing_independent_sideband_20260618
```

Run the post-derivation neighboring-plateau closure on the 2023 data mass
peak/yield diagnostic:

```bash
research/cms-d0-analysis/scripts/run_topological_pointing_closure.sh --max-events -1
```

Current result: nearby accepted plateau rows pass diagnostic
sideband-subtracted and MC-efficiency-corrected yield closure in all six bins.
The largest corrected-yield shift is `12.3%` in `absy1to2_pt2to5`; the largest
corrected-yield z-distance is `1.08` in `absy0to1_pt2to5`. This strengthens the
AN pointing-row justification, but it is still not the final AN mass-fit model.
Main output:
`research/cms-d0-analysis/output/cut-derivation/topological_pointing_closure_20260618`.

Run detector-level official BeamA/BeamB MC forest smokes for raw ZDC/HF checks:

```bash
/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_forest_smoke.sh \
  --max-events 500 \
  --input-file root://cms-xrd-global.cern.ch//store/mc/HINPbPbWinter24MiniAOD/prompt-GNucleusToD0-PhotonBeamA_Bin-Pthat0_Fil-Kpi_UPC_5p36TeV_pythia8-evtgen/MINIAODSIM/NoPU_UPC_UPC_141X_mcRun3_2024_realistic_HI_v14-v2/120000/db419421-6011-4b1b-b074-fe8247bc69d4.root \
  --label prompt_miniaodsim_500_raw_detector \
  --outdir /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-forest-smoke \
  --global-tag 141X_mcRun3_2024_realistic_HI_v14 \
  --no-muons \
  --no-superfilter

/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_forest_smoke.sh \
  --max-events 500 \
  --input-file root://cms-xrd-global.cern.ch//store/mc/HINPbPbWinter24MiniAOD/prompt-GNucleusToD0-PhotonBeamB_Bin-Pthat0_Fil-Kpi_UPC_5p36TeV_pythia8-evtgen/MINIAODSIM/NoPU_UPC_UPC_141X_mcRun3_2024_realistic_HI_v14-v2/2810000/45f52738-a7e2-41c9-a01d-f7c62ccbbd98.root \
  --label prompt_beamb_miniaodsim_500_raw_detector \
  --outdir /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-forest-smoke \
  --global-tag 141X_mcRun3_2024_realistic_HI_v14 \
  --no-muons \
  --no-superfilter
```

Inspect detector branches or reproduce the BeamA/BeamB cutoff diagnostic:

```bash
cd /afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts
root -l -b -q 'inspect_forest_detector_branches.C("/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-forest-smoke/prompt_miniaodsim_500_raw_detector/HiForest_2023PbPbUPC_Jan24Reco_smoke.root")'

root -l -b -q -e '.L derive_zdc_hf_cutoffs_from_official_mc_forest.C' \
  -e 'derive_zdc_hf_cutoffs_from_official_mc_beams("/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-forest-smoke/prompt_miniaodsim_500_raw_detector/HiForest_2023PbPbUPC_Jan24Reco_smoke.root","/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-forest-smoke/prompt_beamb_miniaodsim_500_raw_detector/HiForest_2023PbPbUPC_Jan24Reco_smoke.root","/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root","/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root","/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/mc-cut-validation/official_prompt_mc_beama_beamb_500_zdc_hf",200000)'
```

Current result: official BeamA/BeamB MC fills HF and flips direction as
expected, but `zdcanalyzer/zdcrechit` has `sumPlus=sumMinus=0` for all checked
events. Use this MC for HF-direction diagnostics, not for ZDC neutron-threshold
derivation.

Run a bounded ZDC reconstruction setting matrix on official BeamA MINIAODSIM:

```bash
/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_forest_smoke.sh \
  --max-events 50 \
  --input-file root://cms-xrd-global.cern.ch//store/mc/HINPbPbWinter24MiniAOD/prompt-GNucleusToD0-PhotonBeamA_Bin-Pthat0_Fil-Kpi_UPC_5p36TeV_pythia8-evtgen/MINIAODSIM/NoPU_UPC_UPC_141X_mcRun3_2024_realistic_HI_v14-v2/120000/db419421-6011-4b1b-b074-fe8247bc69d4.root \
  --label beama_50_default \
  --outdir /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/zdc-setting-matrix/official_prompt_beama_50_20260616 \
  --global-tag 141X_mcRun3_2024_realistic_HI_v14 \
  --zdc-variant default \
  --no-muons \
  --no-superfilter
```

Current matrix result: `default`, `pbpb-uncorrected`, and `pbpb-force-soi3`
all have `50` ZDC digi tree entries but `0` nonempty ZDC digi events and `0`
nonzero rechit events. `edmDumpEventContent` shows only `hcalDigis:ZDC`, and
a one-event `HcalDigiDump` prints no ZDC/QIE10 frames. This means the checked
official BeamA MINIAODSIM has no usable ZDC digi content; SOI/OOT-pileup
reconstruction settings cannot recover the one-neutron peak from this sample.
Summary table:
`research/cms-d0-analysis/output/zdc_variant_summary_beama_50_20260616.tsv`.

Audit the ZDC/HF cut-input logic from the saved scan tables:

```bash
/home/ubuntu/agent-network/research/cms-d0-analysis/scripts/audit_zdc_hf_cut_inputs.py
```

Current audit result: the saved event-threshold best points, `ZDC 1500/1500`
and `HF 15/15`, are edge-of-grid artifacts from a retention/EmptyBX score. Do
not use them as D0 analysis cuts. ZDC/HF cuts need detector-level official
MC/data/EmptyBX distributions, not GEN-only MC.

Run the ZDC 1n threshold validation from ZeroBias and EmptyBX forests. The
nominal cut is the AN calibrated recipe, `ZDCsignal = HADsum + 0.1*EMsum`, with
plus `1100 GeV` and minus `1000 GeV` one-neutron thresholds. This validation
checks fitted one-neutron efficiency and EmptyBX noise tails; it does not
replace the nominal AN threshold definition:

```bash
/home/ubuntu/agent-network/research/cms-d0-analysis/scripts/run_zdc_1n_threshold_derivation.sh \
  --label zerobias_forest_emptybx_forest_5m_20260617 \
  --max-events 5000000 \
  --data '/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/forest/crab_HiForest_260218_HIPhysicsRawPrime*_HIRun2023A_ZB_374970/*/0000/*.root' \
  --emptybx '/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/forest/crab_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2/260219_210631/0000/*.root'
```

Current result: the validation rule chooses the lowest rounded 100 GeV threshold
with local fitted-1n/EmptyBX >= 2, integrated EmptyBX tail <= `5e-5`, and fitted
1n efficiency >= `97.5%`. On the 5M forest run this brackets the AN values,
giving plus raw `1125 -> 1100 GeV` and minus raw `1025 -> 1000 GeV`. The simple
`mu-2sigma` diagnostic cuts too high and should not be used as the nominal
threshold rule.

LXPLUS actions should start read-only. Mutating CERN-side commands, CRAB
submission, proxy handling, destructive cleanup, and bulk transfer require
controller review and Evan's approval.
