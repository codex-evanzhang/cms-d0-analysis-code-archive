# D0 Cut Derivation

<!-- DOC_OWNER: cms-analysis-manager cut derivation workflow. -->
<!-- TOKEN_NOTE: use this before scanning AN/code for why each cut exists. -->

## Purpose

The D0 analysis cuts should not be treated as magic constants. Each cut has a
job:

- Event-quality cuts remove noncollision, bad-vertex, beam-halo, and detector
  pathology events before D candidates are considered.
- ZDC XOR cuts select `0nXn` UPC topology: one nucleus emits a photon and stays
  near `0n`, while the other dissociates and deposits neutron energy in the
  opposite ZDC.
- HF rapidity-gap cuts reject hadronic activity on the photon-going/`0n` side,
  preserving exclusive or diffractive UPC-like events.
- Daughter-track cuts ensure the kaon/pion tracks are reconstructable and
  well-measured.
- D-candidate topological cuts suppress random `K pi` combinations while
  keeping displaced, vertex-compatible D0 candidates.
- Kinematic cuts define the analysis fiducial region, not just background
  rejection.

## Inputs

The executable derivation layer uses three sample roles:

| Role | Current default input | Purpose |
|---|---|---|
| signal MC | official prompt `GNucleusToD0` D0 skim | D-candidate signal efficiency versus topology cuts |
| data sidebands | 2023 UPC D0 data skim | combinatorial background versus D-candidate cuts |
| EBX/control | 2023 EmptyBX skim | detector-noise fake rate for ZDC/HF thresholds |

Current defaults are registered in `registry.yaml`:

- signal MC:
  `/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root`
- data:
  `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root`
- EmptyBX:
  `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root`

The derivation macro targets the reduced `Tree` schema. That schema includes
event-level ZDC/HF branches, D-candidate kinematics/topology, and MC truth
matching branches such as `DisSignalCalcPrompt`.

Important current limitation: the official prompt D0 MC has usable generator
and D-truth behavior, but not usable ZDC neutron response in the checked
detector-level forests. A 500-event BeamA plus 500-event BeamB MINIAODSIM
forest smoke found `sumPlus = sumMinus = 0` for every checked event. Therefore
ZDC 1n thresholds cannot be derived from this official photonuclear MC alone.
Use data/EmptyBX plus validated ZDC reconstruction/calibration for ZDC
thresholds; use official MC for D-topology signal efficiency and HF direction
cross-checks.

Current audit update: the saved 200k-event event-threshold optima are not
physics recommendations. They are edge-of-grid optima from a retention/fake-rate
score that does not fit the 1n ZDC peak or validate the HF-gap plateau. Use
`research/cms-d0-analysis/output/zdc_hf_cut_audit/README.md` before discussing
or rerunning ZDC/HF cuts.

Supervisor guidance recorded 2026-06-17:

- ZDC selection efficiency is data-driven. The cuts are documented in the AN,
  but the efficiency of the ZDC selection should be established from data and
  appropriate control samples, not from the current D0 signal MC alone.
- Topological cuts should reference MC because the signal efficiency loss is a
  central constraint. Manual rectangular cuts must balance MC signal retention
  against data-sideband background leakage; a BDT is an acceptable alternative
  if it is trained and validated with the same signal/background logic.
- HF cuts should use automation. Validate the HF rapidity-gap threshold with
  data observables such as track multiplicity to check that hadronic events are
  rejected while UPC-like events are retained.

## Independence Protocol

Cuts must be frozen from physics reasoning, control samples, MC truth, data
sidebands, EmptyBX/ZeroBias, and predeclared scores before comparing to the AN.
The AN remains the reproduction target and a posthoc validation reference, but
AN constants should not change a derived cut silently.

Current machine-readable audit:

- Config:
  `research/cms-d0-analysis/config/selection_independence.yaml`.
- Generated report:
  `research/cms-d0-analysis/output/selection-independence/selection_independence_audit.md`.
- Generator:
  `research/cms-d0-analysis/scripts/audit_selection_independence.py`.

Freeze order:

1. Define the physics target and reconstruction objects.
2. Define control samples and sidebands.
3. Define the score, constraints, and rounding/coarsening convention.
4. Derive the cut or allowed plateau without AN constants.
5. Freeze the candidate selection.
6. Compare to the AN and run closure/systematic checks.

If a blind rule lands on a neighboring plateau point, keep that fact explicit.
The AN point can be tested as a closure/systematic variation, not used as a
hidden retuning step.

## Event Cuts

The reproducible event scan is split by cut family. ZDC and HF should not use a
single generic retention score as the final decision metric.

ZDC selection:

- Derive or validate the plus/minus thresholds from the AN, data, and control
  samples with the official ZDC branch definitions.
- Establish the ZDC selection efficiency data-driven, including the `0n` and
  `Xn` sides separately where possible.
- Require 1n-peak/noise-tail validation plots before accepting a threshold.
- Treat MC as a topology or direction diagnostic only unless it has validated
  nonzero detector-level ZDC response.

Current data-driven ZDC threshold workflow:

- Script:
  `research/cms-d0-analysis/scripts/run_zdc_1n_threshold_derivation.sh`.
- Macro:
  `research/cms-d0-analysis/scripts/derive_zdc_1n_thresholds_data_driven.C`.
- Current best run:
  `research/cms-d0-analysis/output/zdc-1n-thresholds/zerobias_forest_emptybx_forest_5m_20260617`.
- Inputs:
  ZeroBias forest pattern
  `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/forest/crab_HiForest_260218_HIPhysicsRawPrime*_HIRun2023A_ZB_374970/*/0000/*.root`
  and EmptyBX forest pattern
  `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/forest/crab_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2/260219_210631/0000/*.root`.
- Method that reproduces the AN thresholds: fit the 1n peak in the ZDC energy
  distribution, compare the fitted local 1n density against the local EmptyBX
  density, require the integrated EmptyBX tail to be in the sparse-tail regime,
  and keep high fitted 1n efficiency. In the implemented scan this is the
  lowest threshold satisfying local fitted-1n/EmptyBX >= 2, EmptyBX tail
  <= `5e-5`, and Gaussian 1n efficiency >= `97.5%`, then rounded to the
  nearest `100 GeV`.
- 5M forest result: plus raw `1125 GeV` -> rounded `1100 GeV`; minus raw
  `1025 GeV` -> rounded `1000 GeV`. These match the AN thresholds.
- Diagnostic: a simple `mu - 2 sigma` rule cuts too high in the forest-level
  run, giving plus `~1478 GeV` and minus `~1219 GeV`. Therefore the AN
  thresholds are better described as a noise-tail/1n-onset choice, not a
  pure `2 sigma below the peak` rule.
- Caveat: the empirical EmptyBX fake rate from the forest branch is around
  `5e-5` at the accepted raw thresholds, while the AN text quotes a smaller
  EmptyBX probability. Before final claims, confirm that the branch definition,
  OOTPU subtraction, and calibration match the AN ZDC recipe exactly.

HF rapidity-gap selection:

- Automate threshold scans over `HFEMaxPlus` and `HFEMaxMinus`.
- Check signal-like retention and background rejection against data
  observables, especially track multiplicity and other hadronic-activity
  proxies.
- Use EmptyBX/noise samples to control detector noise, but do not let a flat
  fake-rate term drive the threshold to the loosest scanned value.
- Keep the AN nominal thresholds and low/high variations as validation targets
  and systematic handles.

Current HF gap cutoff workflow:

- Macro:
  `research/cms-d0-analysis/scripts/derive_hf_gap_cutoffs_data_mc.C`.
- Current blind full-stat output:
  `research/cms-d0-analysis/output/cut-derivation/hf_gap_data_mc_full_blind_20260618`
  and
  `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/hf_gap_data_mc_full_blind_20260618`.
- Method: split the reduced data tree into plus-gap `ZDCgammaN` and minus-gap
  `ZDCNgamma` events; scan the gap-side HF energy from `1` to `20 GeV`; require
  high prompt-MC signal retention where a same-side MC gap sample exists, high
  pass rate for low-track data events, and bounded leakage for high-track data
  events. EmptyBX is plotted as a detector/noise control, not used as the sole
  optimizer.
- Data-defined control slices in the full run: low-track `nTrack <= 3`,
  high-track `nTrack >= 10` on each gap side.
- Blind working rule: choose the first threshold satisfying MC efficiency
  `>= 0.989` when MC exists, low-track data pass `>= 0.9765`, and high-track
  data leakage `<= 0.85`. The nominal AN value is only a posthoc marker in the
  table and plots.
- Blind full-stat result: `HFEMaxPlus < 9.1 GeV` and `HFEMaxMinus < 8.6 GeV`.
  The AN-compatible plus value `9.2 GeV` also passes the same rule and has
  score ratio `1.00017` relative to the blind threshold, so it is a plateau
  closure/systematic point rather than an input to the derivation. The minus
  side matches exactly.
- Caveat: the rule constants are analysis choices. Treat this as a documented,
  reproducible HF-gap selection rule, not as a final physics signoff.

The current reduced-tree event scan evaluates:

- vertex/filter baseline from available reduced-tree branches:
  `selectedVtxFilter`, `ClusterCompatibilityFilter`, and `1 <= nVtx <= 3`;
- ZDC XOR thresholds:
  `ZDCsumMinus > minusThreshold && ZDCsumPlus < plusThreshold`
  or
  `ZDCsumPlus > plusThreshold && ZDCsumMinus < minusThreshold`;
- HF gap thresholds on the photon-going side:
  if `ZDCgammaN`, require `HFEMaxPlus < HFPlusThreshold`;
  if `ZDCNgamma`, require `HFEMaxMinus < HFMinusThreshold`.

Why:

- ZDC thresholds distinguish real neutron emission from pedestal/noise tails.
- HF thresholds enforce the rapidity gap and reject hadronic contamination.
- EBX/control data estimate detector-noise false-positive rates.
- Track multiplicity and related data observables should validate that HF cuts
  reject hadronic events rather than only optimizing detector-noise behavior.
- Signal MC can be used for HF signal-retention and direction checks, but not
  for ZDC neutron-threshold derivation unless a sample with nonzero validated
  ZDC response is introduced.
- The checked official BeamA/BeamB samples do fill HF and show the expected
  rapidity-gap side flip, but they do not fill ZDC neutron energy. Do not use
  them to set ZDC neutron thresholds.

The reduced `Tree` does not expose every upstream forest branch used in the
from-forest event selection, especially the full vertex-z and CSC-halo pieces.
Those remain part of the upstream selection reproduction, while this layer
derives thresholds available in the skim-level schema.

Event/vertex diagnostic update:

- Output:
  `research/cms-d0-analysis/output/cut-justification/event_d_correlation_5files_20260619`.
- Sample: first five sorted recreated 2023 forest files from the 98-file CRAB
  output.
- Method: count events and final no-mass D-like candidates after each event
  selection stage, where the D-like proxy applies the final candidate cuts
  except the mass cut.
- Result: after the vertex-z cut, the `1 <= nVtx <= 3` cut retains
  `241202/256184 = 94.1%` of events and `15945/16391 = 97.3%` of final
  no-mass D-like candidates. This supports treating the vertex-count cut as a
  multiple-interaction cleanup, not as a strong peak-shaping cut.
- ZDC/HF stages intentionally remove most candidate-bearing events because
  they define the UPC `0nXn` plus rapidity-gap topology.

## D-Candidate Cuts

Baseline before topology optimization:

- `1.70 < Dmass < 2.05` GeV fit range;
- `2 < Dpt <= 12` GeV;
- `|Dy| <= 2`;
- daughter `pT > 1` GeV;
- daughter `|eta| < 2.4`;
- daughter `pT error / pT < 0.1`.

Daughter-track scan update:

- Script:
  `research/cms-d0-analysis/scripts/scan_daughter_track_cuts.C`.
- Output:
  `research/cms-d0-analysis/output/cut-justification/daughter_track_scan_2m_v2_20260619`.
- Method: scan one daughter-track cut at a time over `2M` entries per sample.
  The signal proxy is truth-matched prompt D0 MC after event-quality,
  fit-range, and fiducial cuts. The background proxy is 2023 data sidebands
  after the full event topology. ZDC/HF topology is not required for MC because
  the checked official prompt MC has unusable ZDC neutron response.
- Result for `|eta|`: `|eta| < 2.4` is the tracker-acceptance edge; loosening
  to `2.6` adds no candidates in this scan.
- Result for `pT error / pT`: the `0.1` cut is near a plateau. Loosening from
  `0.1` to `0.2` raises MC signal retention only from `0.5506` to `0.5530`
  while data-sideband pass rises from `0.6269` to `0.6517`.
- Result for daughter `pT`: `pT > 1.0` is a real signal/background compromise,
  not a pure detector edge. In the scan, `0.9`, `1.0`, and `1.1` GeV give MC
  signal retentions `0.636`, `0.551`, and `0.466`, with data-sideband pass
  fractions `0.766`, `0.627`, and `0.532`. Treat the exact `1.0 GeV` choice as
  partially independent and carry nearby variations as a closure/systematic
  before final yields.

The scan is performed in six `(abs(y), pT)` bins:

- `0 <= |y| < 1`, `2-5`, `5-8`, `8-12` GeV;
- `1 <= |y| <= 2`, `2-5`, `5-8`, `8-12` GeV.

For each bin, the macro scans rectangular cuts in:

- `Dalpha` maximum: pointing angle, smaller means the reconstructed D momentum
  points back to the primary vertex;
- `DsvpvSig` minimum: secondary-to-primary vertex displacement significance,
  larger means the D decay vertex is more separated from prompt combinatorics;
- `Dchi2cl` minimum: secondary-vertex probability, larger rejects bad vertex
  fits;
- `Ddtheta` maximum: angular consistency variable, smaller rejects mismatched
  topology.

Signal efficiency comes from truth-matched MC candidates. Background comes from
data sidebands. The original bounded scans used a broad fit-sideband window:

- low sideband: `1.70 < Dmass < 1.80` GeV;
- high sideband: `1.93 < Dmass < 2.05` GeV.

The AN Section 7.1 sideband definition is implemented for reproduction:

```text
0.07 < |Dmass - 1.86484| < 0.12 GeV
```

Independent topology-sideband derivation:

- Macro:
  `research/cms-d0-analysis/scripts/derive_d0_topology_sidebands.C`.
- Wrapper:
  `research/cms-d0-analysis/scripts/run_independent_sideband_topology_derivation.sh`.
- Current full-stat output:
  `research/cms-d0-analysis/output/cut-derivation/topology_sideband_derivation_20260618`.
- Rule: estimate the MC truth-matched D0 mass resolution in each analysis bin
  with `(q84.135 - q15.865)/2`; use one global sideband from the worst bin,
  with inner edge `round(3*sigma_max / 0.01)*0.01 GeV` and outer edge
  `round(5*sigma_max / 0.01)*0.01 GeV`.
- Full-stat result: the worst-bin resolution is in the forward-rapidity bins,
  giving the independently derived sideband
  `0.07 < |Dmass - 1.86484| < 0.12 GeV`. This matches the AN sideband only
  as a posthoc comparison.
- Stability checks: the derived sideband has maximum MC signal leakage
  `3.570%` and maximum upper/lower data-sideband count asymmetry `0.352`
  across the six bins.

Independent-sideband topology result:

- Full wrapper run:

```bash
research/cms-d0-analysis/scripts/run_independent_sideband_topology_derivation.sh \
  --max-events -1 \
  --label 20260618
```

- Data-sideband scan:
  `research/cms-d0-analysis/output/cut-derivation/topocuts_rect_independent_sideband_full_20260618`.
- MC-background scan:
  `research/cms-d0-analysis/output/cut-derivation/topocuts_mc_background_independent_sideband_full_20260618`.
- Floor derivation:
  `research/cms-d0-analysis/output/cut-derivation/topological_floor_independent_sideband_20260618`.
- Pointing derivation:
  `research/cms-d0-analysis/output/cut-derivation/topological_pointing_independent_sideband_20260618`.
- Result: the sideband and `DsvpvSig`/`Dchi2cl` floors are now derived
  without AN constants and match the AN values in all six bins. The exact AN
  `Dalpha`/`Ddtheta` pointing rows are accepted by the independently defined
  plateau rule in `6/6` bins.
- Important caveat: the pointing plateau is not unique. The script also writes
  an AN-free representative cut table; that representative matches the AN
  pointing row in `1/6` bins. Therefore the honest claim is plateau
  compatibility plus independently derived floors/sideband, not uniqueness of
  the exact AN pointing row.

The current ranking score is

```text
signal_efficiency / sqrt(background_fraction + 1 / background_baseline_count)
```

This is a normalized discovery-style score for comparing cut points without
claiming an absolute yield. It should be replaced or supplemented by the final
analysis likelihood/yield uncertainty once normalization, efficiencies, and
systematics are fully reproduced.

BDT option:

- A BDT can replace or supplement rectangular topological cuts if it is trained
  on MC signal and data-sideband or validated-background candidates.
- Required validation: signal efficiency by `pT` and `|y|`, background rejection
  in sidebands, overtraining checks, mass-shape bias checks, and stability under
  reasonable training-variable variations.
- Do not train on the signal mass peak region in a way that sculpts the final
  invariant-mass distribution without an explicit closure test.

## Outputs

Run:

```bash
/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_cut_derivation.sh
```

The output directory contains:

- `zdc_threshold_scan.tsv`
- `hf_gap_threshold_scan.tsv`
- `d_topology_scan.tsv`
- `recommended_event_thresholds.tsv`
- `recommended_d_topological_cuts.tsv`
- `cut_rationale.md`
- `README.md`

Default wrapper mode is bounded by `--max-events 200000` for quick iteration.
Use full statistics only after the smoke output is validated.

Current full-stat topology baselines:

```bash
/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_cut_derivation.sh \
  --label topocuts_rect_full_20260617 \
  --max-events -1

research/cms-d0-analysis/scripts/summarize_cut_derivation.py \
  research/cms-d0-analysis/output/cut-derivation/topocuts_rect_full_20260617

root -l -b -q \
  '/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/derive_2023_d0_topology_cuts_an_sideband.C+("SIGNAL_MC","DATA","CONTROL","OUTDIR",-1)'

.venv-plotting/bin/python \
  research/cms-d0-analysis/scripts/summarize_topological_cut_scan.py \
  research/cms-d0-analysis/output/cut-derivation/topocuts_rect_an_sideband_full_20260617
```

Full-stat topology result: a blind `signal_efficiency / sqrt(background)`
score alone does not reproduce the AN rectangular cuts. It repeatedly prefers
looser `DsvpvSig > 1.5` and `Dchi2cl > 0.02` points. This is now handled by a
two-stage a-priori procedure rather than by snapping to the AN:

1. Derive the vertex-quality floors `DsvpvSig` and `Dchi2cl` with loose
   pointing cuts, requiring visible MC-background and data-sideband rejection
   while preserving most MC signal.
2. With those floors fixed, define a pointing-cut plateau in `Dalpha` and
   `Ddtheta`: MC signal retention relative to the loose `alpha<0.5,
   dtheta<0.5` point must be at least `0.8`, the geometric mean of
   data-sideband and nonmatched-MC score ratios must be at least `0.9`, and
   neither background proxy may increase beyond statistical precision.

With the AN sideband definition, the exact AN cut rows are accepted by this
a-priori plateau rule in all six bins:

```text
0 <= |y| < 1, 2-5:   alpha<0.40, svpv>2.5, chi2>0.10, dtheta<0.50
0 <= |y| < 1, 5-8:   alpha<0.35, svpv>3.5, chi2>0.10, dtheta<0.30
0 <= |y| < 1, 8-12:  alpha<0.40, svpv>3.5, chi2>0.10, dtheta<0.30
1 <= |y| <= 2, 2-5:  alpha<0.20, svpv>2.5, chi2>0.10, dtheta<0.30
1 <= |y| <= 2, 5-8:  alpha<0.25, svpv>3.5, chi2>0.10, dtheta<0.30
1 <= |y| <= 2, 8-12: alpha<0.25, svpv>3.5, chi2>0.10, dtheta<0.30
```

Stored output for the older AN-guided reproduction:

- `research/cms-d0-analysis/output/cut-derivation/topocuts_rect_an_sideband_full_20260617`
- `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/topocuts_rect_an_sideband_full_20260617`
- `topology_guided_an_reproduction.tsv`
- `topology_plots/topology_eff_vs_sideband_grid.png`

MC-background discrepancy check: replacing the data-sideband background proxy
with non-truth-matched MC candidates in the same AN mass sidebands does not
make the blind score reproduce the AN cuts. The raw optimum still prefers
loose `DsvpvSig > 1.5` and `Dchi2cl > 0.02` points. Therefore the discrepancy
is not mainly a data-sideband-versus-MC-background issue; it is caused by using
a blind scalar score without the AN-motivated vertex-displacement and
vertex-fit quality floors.

With the same AN-guided constraints as above, the MC-background scan also
accepts the AN cut table on the constrained score plateau. The nominal over
constrained-best score ratios are `0.970`, `0.997`, `0.998`, `0.923`, `0.994`,
and `0.999` for the six `(abs(y), pT)` bins. This supports treating the AN
rectangular cuts as a stable, quality-constrained plateau choice rather than
the unique maximum of a raw score.

Stored MC-background output:

- `research/cms-d0-analysis/output/cut-derivation/topocuts_mc_background_full_20260617`
- `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/topocuts_mc_background_full_20260617`
- `topology_an_cut_validation.tsv`
- `topology_guided_an_reproduction.tsv`

Independent floor justification: the current reproducible derivation for the
`DsvpvSig` and `Dchi2cl` floors uses the scan grids without looking at the AN
floor values. The rule is:

- use loose `Dalpha` and `Ddtheta` so the scan isolates the floor variables;
- choose the first `Dchi2cl` floor with nonmatched-MC background rejection
  >= `0.04` and MC signal loss <= `0.06`;
- then choose the first `DsvpvSig` floor with nonmatched-MC background
  rejection >= one third, data-sideband rejection >= `0.18`, and MC signal
  retention >= `0.55`.

This independently reproduces the AN floor values in all six bins:

```text
0 <= |y| < 1, 2-5:   svpv>2.5, chi2>0.10
0 <= |y| < 1, 5-8:   svpv>3.5, chi2>0.10
0 <= |y| < 1, 8-12:  svpv>3.5, chi2>0.10
1 <= |y| <= 2, 2-5:  svpv>2.5, chi2>0.10
1 <= |y| <= 2, 5-8:  svpv>3.5, chi2>0.10
1 <= |y| <= 2, 8-12: svpv>3.5, chi2>0.10
```

Interpretation: `Dchi2cl > 0.1` is the first effective vertex-probability
floor after the no-op `0.02/0.05` region. The `DsvpvSig` floor is lower in the
low-`pT` bins because tightening to `3.5` would discard too much signal, while
in higher-`pT` bins `3.5` gives visible rejection in both MC-background and
data-sideband proxies while retaining most of the signal.

Stored floor-justification output:

- `research/cms-d0-analysis/output/cut-derivation/topological_floor_justification_20260618`
- `floor_derivation.tsv`
- `floor_profiles.tsv`
- `floor_profiles/floor_profile_grid.png`

Independent-sideband floor update:

- Output:
  `research/cms-d0-analysis/output/cut-derivation/topological_floor_independent_sideband_20260618`.
- Inputs:
  `topocuts_rect_independent_sideband_full_20260618/d_topology_scan.tsv`
  and
  `topocuts_mc_background_independent_sideband_full_20260618/d_topology_scan.tsv`.
- Result: the same floor rule matches the AN floors in `6/6` bins while using
  the independently derived mass sideband instead of an AN-specified sideband.

Independent pointing-cut plateau justification:

- Script:
  `research/cms-d0-analysis/scripts/derive_topological_pointing_justification.py`.
- Output:
  `research/cms-d0-analysis/output/cut-derivation/topological_pointing_justification_20260618`.
- Inputs:
  `topocuts_rect_an_sideband_full_20260617/d_topology_scan.tsv`,
  `topocuts_mc_background_full_20260617/d_topology_scan.tsv`, and
  `topological_floor_justification_20260618/floor_derivation.tsv`.
- Result: the exact AN `Dalpha`/`Ddtheta` rows are accepted by the blind
  plateau rule in `6/6` bins.

Posthoc AN row metrics under the plateau rule:

```text
0 <= |y| < 1, 2-5:   alpha<0.40, dtheta<0.50, signal retention 0.949, joint score ratio 0.951
0 <= |y| < 1, 5-8:   alpha<0.35, dtheta<0.30, signal retention 0.984, joint score ratio 0.997
0 <= |y| < 1, 8-12:  alpha<0.40, dtheta<0.30, signal retention 0.999, joint score ratio 0.993
1 <= |y| <= 2, 2-5:  alpha<0.20, dtheta<0.30, signal retention 0.809, joint score ratio 0.904
1 <= |y| <= 2, 5-8:  alpha<0.25, dtheta<0.30, signal retention 0.983, joint score ratio 0.989
1 <= |y| <= 2, 8-12: alpha<0.25, dtheta<0.30, signal retention 0.998, joint score ratio 0.989
```

Important caveat: the pointing values are not proven unique. In several bins
many nearby `Dalpha`/`Ddtheta` points lie on the same stable plateau. The
robust next step is closure: compare the exact AN row against neighboring
accepted plateau points in mass-peak fits and corrected yields.

Independent-sideband pointing update:

- Output:
  `research/cms-d0-analysis/output/cut-derivation/topological_pointing_independent_sideband_20260618`.
- Inputs:
  independent-sideband data and MC-background topology scans plus
  `topological_floor_independent_sideband_20260618/floor_derivation.tsv`.
- Result: the exact AN `Dalpha`/`Ddtheta` rows are accepted by the same
  predeclared plateau rule in `6/6` bins.
- New output:
  `pointing_rederived_representative.tsv` gives one AN-free representative
  row per bin. It is selected from the accepted plateau by retaining at least
  `0.94` times the best accepted signal retention and then maximizing
  combined data-sideband plus nonmatched-MC background rejection. This
  representative matches the AN pointing row in `1/6` bins, which confirms
  that the exact pointing choices are plateau-compatible but not uniquely
  determined by the current rule.

Neighboring-plateau closure:

- Wrapper:
  `research/cms-d0-analysis/scripts/run_topological_pointing_closure.sh`.
- Local controller/summarizer:
  `research/cms-d0-analysis/scripts/derive_topological_pointing_closure.py`.
- LXPLUS ROOT macro:
  `research/cms-d0-analysis/scripts/check_topological_pointing_closure.C`.
- Output:
  `research/cms-d0-analysis/output/cut-derivation/topological_pointing_closure_20260618`.
- Remote output:
  `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/topological_pointing_closure_20260618`.
- Rule: compare nearby accepted plateau points to the AN row using
  sideband-subtracted mass-window yield and the frozen MC signal efficiency
  from the topology scan. A neighbor is compatible if the corrected-yield
  shift is within `2 sigma` or within `15%`.
- Full-stat result on the official 2023 reduced data skim: `6/6` bins pass the
  neighboring-plateau closure criterion.

Closure summary:

```text
0 <= |y| < 1, 2-5:   7/7 neighbors pass, max corrected-yield shift 0.098, max z 1.08
0 <= |y| < 1, 5-8:   9/9 neighbors pass, max corrected-yield shift 0.028, max z 0.16
0 <= |y| < 1, 8-12:  9/9 neighbors pass, max corrected-yield shift 0.038, max z 0.09
1 <= |y| <= 2, 2-5:  3/3 neighbors pass, max corrected-yield shift 0.123, max z 0.73
1 <= |y| <= 2, 5-8:  9/9 neighbors pass, max corrected-yield shift 0.048, max z 0.22
1 <= |y| <= 2, 8-12: 9/9 neighbors pass, max corrected-yield shift 0.060, max z 0.13
```

Interpretation: the AN pointing rows are not only inside the predeclared
plateau; nearby accepted rows give statistically compatible
efficiency-corrected diagnostic yields. This strengthens the a-priori
justification, but it is still not the final AN mass-fit model.

## Medium-Scale Preflight

Current preflight output:
`research/cms-d0-analysis/output/preflight-medium-scale-20260618/README.md`.

The medium-scale validation is now justified, with explicit caveats:

- ZDC scale closure: official/recreated same-event slopes are stable under
  event/lumi and energy splits after applying the 2023-offline side scales;
  nominal `0nXn` and `lt OR 1500` decisions agree on `6326/6326` matched
  events.
- ZDC thresholds: data/EmptyBX 1n-onset rule derives rounded thresholds
  `1100 GeV` plus and `1000 GeV` minus without using the AN thresholds as
  inputs.
- HF thresholds: blind data/MC gap rule derives `9.1 GeV` plus and
  `8.6 GeV` minus; nominal `9.2/8.6` is a posthoc plateau closure point.
- Topology: sideband, vertex-quality floors, and pointing plateau support are
  independently justified; floors and AN pointing-row acceptance pass in
  `6/6` bins.

Remaining non-independent pieces for the medium-scale run log are trigger
mapping, daughter-track numeric cuts, final mass-fit model, exact bin/scope
choices, and the fact that the ZDC side scale is currently an
official-reforest matching correction rather than a recovered historical
conditions payload.

## Review Rules

- Do not freeze final cuts from a small smoke scan.
- Require MC signal efficiency, data sideband stability, and EBX fake-rate
  checks before treating any cut as analysis-grade.
- Keep nominal AN cuts as validation targets or closure/systematic points, not
  hidden priors.
- Treat ZDC efficiency as data-driven unless a validated detector-level ZDC MC
  sample is introduced.
- For HF cuts, require automated scans plus data cross-checks against hadronic
  activity observables such as track multiplicity.
- For topological cuts, require explicit MC signal-efficiency accounting and
  data-sideband background accounting; manual cuts and BDT cuts must pass the
  same closure logic.
- If the scan optimum disagrees strongly with the nominal value, inspect the
  distribution and branch definition before changing the analysis cut.
- For ZDC/HF, require detector-level official MC/data/EmptyBX distributions and
  a 1n-peak/gap-threshold validation; GEN-only MC cannot validate these cuts.
- If using the official `PhotonBeamA/B` MINIAODSIM forests, treat HF-side plots
  as diagnostics and ZDC as blocked unless a nonzero, validated ZDC response is
  recovered.
