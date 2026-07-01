# Upstream Code Provenance

The reproduction used upstream CMS software in addition to the code archived
here. This repository does not vendor the full upstream trees.

## CMSSW / Foresting

Recorded primary foresting area on LXPLUS:

```text
/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src
```

This area contains the CMSSW packages used for HiForest/Dfinder-style
production. The archive includes local wrappers/configs that invoked this
environment, especially:

- `d0-reproduction/scripts/run_2023_forest_smoke.sh`
- `d0-reproduction/scripts/run_2023_forest_crab_smoke.sh`
- `d0-reproduction/scripts/setup_cmssw132x_forest_area.sh`
- `d0-reproduction/config/ZDCAnalyzersHC2023_cff.py`
- `dplus-generalization/scripts/run_2023_dplus_forest_smoke.sh`
- `dplus-generalization/scripts/run_2023_dplus_forest_crab_smoke.sh`

## LXPLUS Execution Copies

Many scripts were synced to and executed from:

```text
/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts
```

Those files are represented here by their local source-of-truth copies under
`d0-reproduction/scripts/`. A live LXPLUS refresh requires current CERN
interactive authentication.

## EOS Outputs

Analysis outputs were written under Evan-owned EOS areas such as:

```text
/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction
```

EOS outputs are data products and are intentionally excluded from this public
repo.

## Official MC Fragments

Official D0 UPC MC fragments rendered from stored McM snapshots are included in:

```text
official-mc-fragments/
```

These are code/config fragments, not generated event data.
