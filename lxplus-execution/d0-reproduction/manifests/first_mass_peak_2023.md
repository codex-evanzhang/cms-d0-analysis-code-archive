# First 2023 D0 Mass-Peak Validation

<!-- DOC_OWNER: cms-analysis-manager skim-level first reproduction product. -->
<!-- TOKEN_NOTE: use this instead of opening ROOT logs unless debugging. -->

Status: first-pass done on 2026-06-15.

## Purpose

Create the first top-down \(D^0 \to K\pi\) invariant-mass peak from 2023 data
using the most polished available reference skim. This validates the basic
data path, branches, selections, plotting, and fit sanity before rebuilding
the upstream skim/forest/MINIAOD chain.

This is not yet a final full-chain reproduced result.

## Input

Reference skim:

`/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root`

Tree: `Tree`

Processed entries: `54386911`

Relevant branches checked from the skim:

- `Dmass`
- `Dpt`
- `Dy`
- `DpassCut23PAS`
- `DpassCutNominal`

## Command

```bash
/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_mass_peak.sh
```

The wrapper ran:

```bash
root -l -b -q "/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/make_2023_d0_mass_peak.C(...,0)"
```

Run log:

`/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_mass_peak_20260615T192309Z.log`

## Outputs

Output directory:

`/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/first-mass-peak-2023`

Key files:

- `d0_mass_2023_dpasscut23pas_2pt12_absy2.png`
- `d0_mass_2023_dpasscut23pas_2pt12_absy2.pdf`
- `d0_mass_2023_dpasscut23pas_2pt12_absy2.csv`
- `d0_mass_2023_dpasscutnominal_2pt12_absy2.png`
- `d0_mass_2023_dpasscutnominal_2pt12_absy2.pdf`
- `d0_mass_2023_dpasscutnominal_2pt12_absy2.csv`
- `d0_mass_2023_selection_overlay.png`
- `d0_mass_2023_selection_overlay.pdf`
- `d0_mass_2023_histograms.root`
- `mass_peak_2023_manifest.txt`

## Main Selection

Main mass-peak plot:

```text
Dpt>2 && Dpt<12 && abs(Dy)<2 && DpassCut23PAS
```

Mass histogram:

- Range: `1.70 < m(Kpi) < 2.05` GeV
- Bins: `140`
- Bin width: `0.0025` GeV

## Numerical Sanity Checks

Kinematic-only selection:

- Cut: `Dpt>2 && Dpt<12 && abs(Dy)<2`
- Selected values: `185428`
- Histogram integral in mass range: `73651`
- Max bin center: `1.86625` GeV

`DpassCut23PAS` selection:

- Selected values: `58623`
- Histogram integral in mass range: `22403`
- Max bin center: `1.86125` GeV
- Simple fit status: `0`
- Simple fit model: Gaussian + linear background over `1.78-1.95` GeV
- Fit mean: `1.86400522698` GeV
- Fit sigma: `0.0128314303572` GeV
- Fit chi2/ndf: `46.8997149989 / 63`

`DpassCutNominal` selection:

- Selected values: `126349`
- Histogram integral in mass range: `48846`
- Max bin center: `1.86625` GeV
- Simple fit status: `0`
- Fit mean: `1.86407468739` GeV
- Fit sigma: `0.0129692065378` GeV
- Fit chi2/ndf: `59.8464567304 / 63`

## Validation Status

Verified:

- The input file exists and is readable on LXPLUS.
- The expected `Tree` and mass/pT/y/pass-cut branches exist.
- The full combined skim was processed without ROOT errors.
- The output plot has a visible \(D^0\)-like peak near the PDG mass.
- The simple fit converged for both selected variants.

Limitations:

- This used the existing combined D0 reference skim, so it validates only the
  top-down plotting rung.
- The skim/reducer, foresting, trigger, correction, and normalization steps
  still need reproduction before any final measurement claim.
- The simple fit is a sanity check only, not the final signal-extraction model.

## Next Step

Compare this mass-peak shape and selection choice to the analysis-note target
plot, then reproduce the selected-candidate/skim production feeding this plot
from upstream forest or MINIAOD inputs.
