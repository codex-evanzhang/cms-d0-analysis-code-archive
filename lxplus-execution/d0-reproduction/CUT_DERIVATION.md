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

Important current limitation: the official prompt D0 MC skim has usable D truth
matching, but its `ZDCsumPlus/Minus` values are not usable for event-threshold
derivation in the checked skim (`-9999` in the smoke scan). Therefore the
pipeline derives ZDC/HF event thresholds from data retention versus EBX fake
rate, while D-topology cuts use MC signal efficiency versus data sidebands.

## Event Cuts

The reproducible event scan evaluates data retention against EBX fake rate:

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
- EBX/control data estimate the false-positive rate.
- Signal MC would be used here only if a future sample includes usable ZDC/HF
  response.

The reduced `Tree` does not expose every upstream forest branch used in the
from-forest event selection, especially the full vertex-z and CSC-halo pieces.
Those remain part of the upstream selection reproduction, while this layer
derives thresholds available in the skim-level schema.

## D-Candidate Cuts

Baseline before topology optimization:

- `1.70 < Dmass < 2.05` GeV fit range;
- `2 < Dpt <= 12` GeV;
- `|Dy| <= 2`;
- daughter `pT > 1` GeV;
- daughter `|eta| < 2.4`;
- daughter `pT error / pT < 0.1`.

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
data sidebands, currently:

- low sideband: `1.70 < Dmass < 1.80` GeV;
- high sideband: `1.93 < Dmass < 2.05` GeV.

The current ranking score is

```text
signal_efficiency / sqrt(background_fraction + 1 / background_baseline_count)
```

This is a normalized discovery-style score for comparing cut points without
claiming an absolute yield. It should be replaced or supplemented by the final
analysis likelihood/yield uncertainty once normalization, efficiencies, and
systematics are fully reproduced.

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

## Review Rules

- Do not freeze final cuts from a small smoke scan.
- Require MC signal efficiency, data sideband stability, and EBX fake-rate
  checks before treating any cut as analysis-grade.
- Keep nominal AN cuts as validation targets, not hidden priors.
- If the scan optimum disagrees strongly with the nominal value, inspect the
  distribution and branch definition before changing the analysis cut.
