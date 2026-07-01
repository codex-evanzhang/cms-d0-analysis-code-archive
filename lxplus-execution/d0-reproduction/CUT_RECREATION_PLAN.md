# Cut Recreation Plan For The 2023 D0 Analysis

<!-- DOC_OWNER: cms-analysis-manager cut recreation plan. -->
<!-- TOKEN_NOTE: compact map from copied cut constants to reproducible derivation steps. -->

## Goal

Move the analysis-specific event and D-candidate cuts from copied constants to
reproducible derivation workflows. The nominal AN values remain targets for
validation, not unexplained inputs.

## Current Nominal Cuts

### Event Selection

The current selected-skim macro applies:

| Stage | Nominal cut | Current branch/input |
|---|---|---|
| Primary-vertex filter | `pprimaryVertexFilter == 1` | `skimanalysis/HltTree` |
| Vertex z | best primary vertex `abs(vz) < 15 cm` | `PbPbTracks/trackTree` |
| Vertex multiplicity | `1 <= nVtx <= 3` in code | `PbPbTracks/trackTree` |
| Cluster compatibility | `pclusterCompatibilityFilter == 1` | `skimanalysis/HltTree` |
| Halo rejection | `cscTightHalo2015Filter == true` | `l1MetFilterRecoTree/MetFilterRecoTree` |
| ZDC XOR | exactly one side above 1n threshold | `zdcanalyzer/zdcrechit` |
| ZDC plus threshold | `ZDCsumPlus > 1100 GeV` for Xn | `sumPlus` |
| ZDC minus threshold | `ZDCsumMinus > 1000 GeV` for Xn | `sumMinus` |
| HF rapidity gap | photon-going / 0n side has no large HF PF candidate | `particleFlowAnalyser/pftree` |
| HF plus gap threshold | `HF+ Emax < 9.2 GeV` | max PF energy, `3.0 < eta < 5.2` |
| HF minus gap threshold | `HF- Emax < 8.6 GeV` | max PF energy, `-5.2 < eta < -3.0` |

Note: the AN text extraction says "requiring nVtx==3", while the code uses
`1 <= nVtx <= 3`. Treat this as an explicit validation item.

### D-Candidate Selection

The current selected-skim macro applies:

| Stage | Nominal cut |
|---|---|
| Mass pre-window | `abs(Dmass - 1.86484) < 0.2 GeV` |
| Daughter quality | both tracks high-purity |
| Daughter acceptance | both tracks `abs(eta) < 2.4` |
| Daughter pT | both tracks `pT > 1.0 GeV` |
| Daughter pT resolution | both tracks `pT error / pT < 0.1` |
| D kinematics | `2 < Dpt <= 12 GeV`, `abs(Dy) <= 2` |

Topological cuts from AN Table 7:

| abs(y) | pT (GeV) | alpha max | DsvpvSig min | vertex probability min | dtheta max |
|---:|---:|---:|---:|---:|---:|
| 0-1 | 2-5 | 0.40 | 2.5 | 0.10 | 0.50 |
| 0-1 | 5-8 | 0.35 | 3.5 | 0.10 | 0.30 |
| 0-1 | 8-12 | 0.40 | 3.5 | 0.10 | 0.30 |
| 1-2 | 2-5 | 0.20 | 2.5 | 0.10 | 0.30 |
| 1-2 | 5-8 | 0.25 | 3.5 | 0.10 | 0.30 |
| 1-2 | 8-12 | 0.25 | 3.5 | 0.10 | 0.30 |

## Recreation Strategy

### 1. Provenance Extraction

Record each cut with:

- AN section/table/source line.
- Current branch name and producer tree.
- Current code location.
- Whether the cut is central CMS/reconstruction, analysis-optimized, or
  detector-calibration driven.

### 2. ZDC Threshold Recreation

Inputs needed:

- Recreated forests containing the ZDC rechit/sum information.
- Empty-bunch crossing data to measure false 0n-to-Xn noise.
- HIForward or ZeroBias events to fit the 1n, 2n, 3n peaks.

Procedure:

1. Validate or rebuild the offline ZDC signal as `HADsum + 0.1 * EMsum` after
   offline `TS2 - TS1` out-of-time pileup subtraction.
2. Fit the 1n, 2n, and 3n peaks separately for plus and minus sides.
3. Measure the EBX noise tail above candidate thresholds.
4. Choose the lowest threshold that keeps EBX false-positive probability below
   the AN target level while preserving the Xn peak integral at the percent
   level.
5. Validate that the result is consistent with the nominal 1100/1000 GeV.

### 3. HF Gap Threshold Recreation

Inputs needed:

- EBX data for the HF `Emax` noise distribution.
- Direct and resolved photonuclear MC for signal efficiency.
- Data yield or cross-section scans versus threshold.

Procedure:

1. Build `Emax` on each HF side as the largest PF candidate energy in
   `3.0 < abs(eta) < 5.2`.
2. Confirm EBX noise is mostly below 8 GeV.
3. Scan thresholds, including 5.5, 8.6, 9.2, and 15 GeV.
4. Require MC direct/resolved efficiencies to be on the plateau.
5. Require corrected data yield or cross section to be on the plateau.
6. Choose side-specific nominal thresholds consistent with the plateau:
   9.2 GeV for HF plus and 8.6 GeV for HF minus.
7. Keep 5.5 and 15 GeV as systematic variations unless the reproduced scan
   suggests a different prescription.

### 4. D-Candidate Cut Recreation

Inputs needed:

- Candidate trees after event selection.
- Signal MC with matched generated D0 where available.
- Data sidebands to model combinatorial background.

Procedure:

1. Keep Table 6 track-quality requirements as baseline reconstruction cuts.
2. In each `(abs(y), pT)` bin, scan rectangular cuts in:
   `Dalpha`, `DsvpvSig`, `Dchi2cl`, and `Ddtheta`.
3. Estimate signal using mass fits or sideband subtraction.
4. Estimate efficiency using signal MC.
5. Optimize a fixed objective such as expected `S/sqrt(S+B)` or final-yield
   uncertainty, with guardrails against large direct/resolved or data/MC bias.
6. Compare the optimum region to the nominal Table 7 values.
7. Freeze the final table only after cutflow, mass spectra, and efficiency
   checks are reproduced.

## Immediate Implementation

The first executable layer is an audit/scan harness:

- Write nominal cut tables.
- Dump current Section 5 and Section 7 cutflows.
- Scan ZDC XOR event counts versus candidate ZDC thresholds.
- Scan HF pass fractions versus candidate HF thresholds.
- Record all outputs under the D0 reproduction workspace.

This does not replace the full derivation from EBX/MC/MINIAOD. It makes the
current copied cuts auditable and gives the next scripts stable inputs.

## Open Validation Items

- Resolve the `nVtx` wording/code mismatch.
- Recover or register the EBX data used for ZDC/HF noise studies.
- Recover or register the direct/resolved MC samples used for HF and D-cut
  efficiency scans.
- Reproduce the ZDC signal construction directly after the foresting blocker is
  fixed.
- Replace data-sideband-only D-cut scans with MC-backed optimization once the
  MC inputs are registered.
