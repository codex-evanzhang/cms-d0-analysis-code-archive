# Upstream Recreation Plan For The 2023 D0 Analysis

<!-- DOC_OWNER: cms-analysis-manager upstream dependency plan. -->
<!-- TOKEN_NOTE: compact map from current analysis products back to MINIAOD. -->

## Goal

Replace every analysis-produced input that was still being used in the current
reproduction with a reproducible upstream step. Existing reference forests and
skims remain validation targets, not nominal inputs.

## Current Dependency Classification

| Dependency | Currently used how | Would not exist before analysis? | Recreation path |
|---|---|---:|---|
| Wang combined/per-PD D0 skims | Validation and earlier top-down plotting | Yes | Rebuild from forests; compare counts, schema, cutflow, mass spectra. |
| Vanderbilt HIForward0 forests | Input to selected-skim reproduction | Probably yes as processed production | Recreate from official MINIAOD with the forest config and Dfinder enabled. |
| `Dfinder/ntDkpi` branches | Source of D0 candidates | Yes, analysis/channel candidate production | Recreate inside the foresting step from packed PF candidates and vertices. |
| Section 5 event selection | Recreated with `ApplySection5EventSelection.C` | Yes, analysis-specific | Keep as scripted reducer, but validate thresholds and cutflow against note/reference. |
| Section 7 D selection | Recreated with `ApplySection7DSelection.C` | Yes, analysis-specific | Keep as scripted reducer, validate per-bin cut table and selected mass spectra. |
| ZDC/HF thresholds | Used in Section 5 | Yes, analysis-specific calibration/choice | Trace to AN/table/control samples; reproduce threshold derivation or document as fixed analysis input. |
| D0 rectangular cuts | Used in Section 7 | Yes, optimized analysis choice | Trace to AN Table 7; reproduce cutflow and later justify/optimize if extending channels. |
| Golden JSON/run range | Certification input | No, central CMS/data-quality input | Treat as external official input; record exact path and revision. |
| MINIAOD datasets | Least-reduced practical CMS input | No, central CMS input | Use official DAS datasets as starting point. |

## Earliest Practical Inputs

- Dataset family:
  `/HIForward[0-19]/HIRun2023A-16Jan2024-v1/MINIAOD`
- Run range:
  `374804-375746`
- Certification JSON:
  `/eos/user/c/cmsdqm/www/CAF/certification/Collisions23HI/Cert_Collisions2023HI_374288_375823_Good_ZDC_Golden.json`
- Forest config:
  `/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src/HIForestSetupPbPbRun2025/forest_CMSSWConfig_Run3_132X_2023PbPb_Jan2024ReReco_UPC_MITHIG.py`
- CRAB config template:
  `/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src/HIForestSetupPbPbRun2025/forest_CRABConfig_132X_2023PbPb_Jan2024ReReco_UPC_MITHIG.py`

## Source-Code Anchors

- `HIForestSetupPbPbRun2025` remote:
  `git@github.com:jdlang/HIForestSetupPbPbRun2025.git`
- `HIForestSetupPbPbRun2025` checked commit:
  `90900d08a2305fcd1eda1727506528e76eaa700c`
- `Bfinder` remote:
  `git@github.com:boundino/Bfinder.git`
- `Bfinder` checked commit:
  `e22c60dd5165d58e7d68f90ad37c57ed4aa3b4b6`

## Dfinder Configuration To Reproduce

The 2023 UPC forest config embeds Dfinder:

- `runOnMC = False`
- Vertex label: `offlineSlimmedPrimaryVertices`
- Track label: `packedPFCandidates`
- Track chi2 label: `packedPFCandidateTrackChi2`
- Gen label placeholder: `prunedGenParticles`
- Pre-fit track cuts: `tkPtCut = 0.2`, `tkEtaCut = 2.4`
- Enabled D channels:
  - `K+ pi- : D0bar`
  - `K- pi+ : D0`
- Disabled channels in this D0 pass:
  - `D+`, `D-`, four-track D0, `Ds`, `D*`, `B`, `Lambda_c`
- Loose Dfinder cuts applied to all channels:
  - `dPtCut = 0`
  - `VtxChiProbCut = 0.05`
  - `svpvDistanceCut = 0.5`
  - `alphaCut = 4`

This means the next upstream goal is not to write Dfinder from scratch, but to
recreate the exact CMSSW foresting job that runs the Bfinder/Dfinder package on
MINIAOD.

## Reproduction Chain

### Stage A: Forest Smoke Test

Purpose: prove the local CMSSW environment can recreate a small forest from
MINIAOD with Dfinder enabled.

Script:

```bash
/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_forest_smoke.sh --max-events 100
```

Expected validation:

- Output contains `Dfinder/ntDkpi`.
- Output contains event trees needed by the selected skim:
  `hiEvtAnalyzer/HiTree`, `PbPbTracks/trackTree`, `skimanalysis/HltTree`,
  `l1MetFilterRecoTree/MetFilterRecoTree`, `zdcanalyzer/zdcrechit`,
  `particleFlowAnalyser/pftree`.
- Event counts are consistent across aligned trees.
- A tiny selected-skim run can consume the smoke forest.

Current status:

- A 10-event smoke attempt was run on 2026-06-15:

```bash
/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_forest_smoke.sh --max-events 10 --label hiforward0_miniaod_10
```

- It failed during Python config loading, before event processing:

```text
ModuleNotFoundError: No module named 'HeavyIonsAnalysis.ZDCAnalysis.ZDCAnalyzersHC2023_cff'
```

- The current `CMSSW_16_1_1` area contains
  `HeavyIonsAnalysis/ZDCAnalysis/python/ZDCAnalyzersPbPb_cff.py`, not
  `ZDCAnalyzersHC2023_cff.py`.
- `ZDCAnalyzersPbPb_cff.py` defines `zdcSequencePbPb`, while the 2023 forest
  config currently loads `ZDCAnalyzersHC2023_cff` and later adds
  `process.zdcSequence`.

Interpretation:

- The first upstream blocker is environment/config reproducibility, not Dfinder.
- The 2023 forest config says it expects `CMSSW_13_2_10+`,
  `forest_CMSSW_13_2_X`, and `Dfinder_14XX_miniAOD`; the current available
  area is `CMSSW_16_1_1`.
- Before any full production, recover the exact intended forest release or
  validate an explicit ZDC config migration.

Next checks:

1. Find the release or commit that provided
   `ZDCAnalyzersHC2023_cff.py`.
2. If unavailable, test a generated diagnostic config that replaces the load
   with `ZDCAnalyzersPbPb_cff.py` and maps `zdcSequencePbPb` to the expected
   sequence name. This must be marked as a migration test, not the nominal
   reproduction, until ZDC output is validated against the reference forest.
3. After the forest smoke runs, compare tree names and ZDC branch content
   against the existing reference forest before submitting CRAB.

### Stage B: Forest Production For HIForward0

Purpose: replace the copied Vanderbilt HIForward0 forest files.

Method:

1. Generate a CRAB config from
   `forest_CRABConfig_132X_2023PbPb_Jan2024ReReco_UPC_MITHIG.py` with
   `pd = 'HIForward0'`.
2. Use:
   - input dataset `/HIForward0/HIRun2023A-16Jan2024-v1/MINIAOD`
   - Golden JSON above
   - run range `374804-375746`
   - `LumiBased` splitting, `unitsPerJob = 20`
3. Submit only after Evan approves CRAB/full production.
4. Write outputs under Evan-owned EOS, preferably:
   `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-forest-2023/HIForward0`

Validation against existing HIForward0 forests:

- Number of output files/jobs.
- Total forest entries.
- Tree list/schema.
- Dfinder candidate multiplicity distributions.
- D mass before Section 7 cuts.
- ZDC/PF/HF diagnostic distributions.

### Stage C: Forest Production For HIForward0-19

Purpose: replace all per-PD processed forests.

Repeat Stage B for `HIForward0` through `HIForward19`. This is a bulk CRAB
campaign and requires explicit approval before submission.

### Stage D: Recreate Selected Skims From Reproduced Forests

Use the already working selected-skim script, pointing `D0_REPRO_HIFORWARD0_FOREST_DIR`
or an equivalent input-list variable to the newly reproduced forest directory:

```bash
D0_REPRO_HIFORWARD0_FOREST_DIR=/path/to/reproduced/HIForward0 \
/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_d0_selected_skim.sh --label reproduced_HIForward0_selected
```

Validation:

- Compare Section 5 cutflow to the selected-skim first pass.
- Compare Section 7 selected count and D mass shape.
- Compare against Wang per-PD reference skim where branch definitions overlap.

### Stage E: Recreate Wang-Style Tree Or Stop Using It

The exact Wang `Tree` format is still not reproduced. Two acceptable paths:

1. Rebuild a branch-compatible `Tree` reducer once branch definitions are
   understood.
2. Stop depending on that exact format and use the independently reproduced
   `SelectedD` pipeline as the analysis carrier, as long as all downstream
   quantities are validated.

## Current Stop Gates

- Do not submit CRAB jobs without Evan approval.
- Do not run all HIForward0-19 production without Evan approval.
- Do not treat the current selected-skim result as final until compared against
  the reference skim/counts for matching samples.
- Do not claim ZDC/HF threshold derivation is reproduced until the threshold
  source/control-sample derivation is found.
- Do not silently swap `ZDCAnalyzersHC2023_cff` for `ZDCAnalyzersPbPb_cff` in
  nominal production. That is only a diagnostic migration hypothesis until
  validated.

## Next Concrete Command

Run the bounded forest smoke test:

```bash
/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_forest_smoke.sh --max-events 100
```

If it succeeds, validate the smoke forest schema and run the selected-skim
pipeline on that smoke output. If it fails, fix the CMSSW environment/config
before any CRAB campaign.
