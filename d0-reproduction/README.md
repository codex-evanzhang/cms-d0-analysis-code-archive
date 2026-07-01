# D0 Reproduction Code

This folder contains the D0 analysis-reproduction code copied from the local
working area.

## `scripts/`

Main categories:

- `run_*`: shell runners for local/LXPLUS/CRAB-style workflows.
- `derive_*`: cut-derivation macros and scripts for ZDC, HF, topology, and
  sideband studies.
- `render_*`, `build_*`, `summarize_*`: plotting, report-building, and compact
  summary generation.
- `compare_*`, `validate_*`, `inspect_*`: validation against official/reference
  forests, schema checks, and branch diagnostics.
- `make_*`: one-off but reusable plot/config producers.

## `config/`

Small configuration files used by the reproduction. The most important are:

- `zdc_selection.yaml`: ZDC selection recipe and AN threshold convention.
- `selection_independence.yaml`: audit structure for whether selections are
  independent, partially independent, or AN-specified.
- `ZDCAnalyzersHC2023_cff.py`: 2023 ZDC analyzer config shim used in foresting
  validation attempts.

## `docs/`

Copied workflow notes and command recipes. These are included because many
scripts require CERN-side paths or CMSSW environments that are easier to
understand with the run notes.

Most useful starting points:

- `README.md`: compact project orientation.
- `COMMANDS.md`: command recipes used during reproduction.
- `RUNS.md`: run/checkpoint log.
- `CUT_DERIVATION.md`: ZDC/HF/topology cut-derivation notes.
- `registry.yaml`: machine-readable D0 sample/tool/workflow registry.
- `ANALYSIS_ROADMAP.md` and `CMS_ANALYSIS_PLAN.md`: higher-level reproduction
  roadmap.
