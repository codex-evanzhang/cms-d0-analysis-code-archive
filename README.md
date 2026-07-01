# CMS D0 Analysis Code Archive

Public code archive for the Codex-assisted CMS D0 analysis reproduction work.

This repository is not a CMS data release and is not a self-contained physics
analysis result. It collects the reusable code, configs, wrappers, report
builders, and command recipes that were used while reproducing the D0 analysis
workflow and testing the D+ generalization path.

## Contents

- `d0-reproduction/scripts/`: ROOT macros, Python scripts, shell runners,
  CRAB helpers, plotting, cut derivation, validation, and report builders for
  the D0 reproduction.
- `d0-reproduction/config/`: small configuration files used by the D0
  reproduction, including ZDC and selection-independence configs.
- `d0-reproduction/docs/`: local workflow notes and command recipes copied
  from the working analysis area.
- `dplus-generalization/`: scripts and notes used for the D+ to Kpipi peak
  generalization test.
- `official-mc-fragments/`: official D0 UPC MC fragments rendered from stored
  McM snapshots for reproducibility of the MC setup.
- `reporting/`: TeX/report/slide-generation sources used to present the
  reproduction outputs.
- `orchestration/`: selected local helper scripts used to run LXPLUS, CMS, and
  Git/Overleaf workflows from the agent host.
- `manifests/`: generated and hand-written inventories explaining where code
  came from and how it was used.

## What Is Excluded

This public repository intentionally excludes:

- CMS ROOT files, MINIAOD, HiForest, skims, CRAB outputs, and derived data.
- Credentials, tokens, passphrases, private keys, cookies, and local secrets.
- Large rendered plot/output directories.
- Full upstream CMSSW/Dfinder/HiForest source trees.

For upstream CMSSW/Dfinder code, this archive keeps the wrappers, configs, and
recorded provenance needed to find or recreate the execution environment rather
than vendoring the full external source tree.

## Main Workflow Covered

The principal reproduction path was:

```text
2023 MINIAOD
  -> recreated/minimal HiForest with Dfinder/ZDC/HF branches
  -> event-level skim
  -> D0 candidate skim
  -> cut derivation and validation
  -> D0 mass peak and AN-style diagnostic plots
  -> reports / Overleaf / slides
```

The D+ path is included as a separate generalization test:

```text
2023 MINIAOD / Dfinder foresting with ntDkpipi
  -> D+ candidate mass spectrum
  -> preliminary Kpipi peak diagnostics
```

## Rebuilding The Manifest

After changing the archive, regenerate the code manifest:

```bash
python3 tools/build_code_manifest.py
```

The manifest records relative path, file size, SHA256, and category for each
archived code/config/documentation file.
