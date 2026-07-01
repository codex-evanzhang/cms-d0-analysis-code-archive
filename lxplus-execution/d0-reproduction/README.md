# CMS D0 Reproduction Workspace

<!-- DOC_OWNER: CERN-side control surface for D0 reproduction. -->
<!-- TOKEN_NOTE: start here; this file points to the compact plan, registry, and log. -->

This LXPLUS workspace is for reproducing Evan Zhang's CMS D0 production
analysis in a top-down, auditable way.

Canonical workspace:

`/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction`

Large outputs should go under:

`/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction`

## Policy

- All write actions for this project must stay inside Evan-owned CERN paths:
  `/afs/cern.ch/user/e/evzhang`, `/afs/cern.ch/work/e/evzhang`, or
  `/eos/user/e/evzhang`.
- Other CERN dataset paths may be read for analysis input discovery and
  validation, but should not be modified.
- Existing skims, merged ROOT files, plots, and tables are validation
  references. They are not the final starting point for a full reproduction.
- The reproduction should work backward from polished products to upstream
  inputs: final plots, final fit artifacts, analysis tables, reduced ntuples,
  D0 candidate skims, forest/MINIAOD inputs, then data certification.
- Stop and report when a required input, selection definition, calibration,
  correction, or tool instruction is missing or ambiguous.

## Files

- `TOP_DOWN_REPRODUCTION_PLAN.md`: step gates and stop conditions.
- `DATA_LADDER.md`: current view of products from polished outputs back to raw
  practical inputs.
- `STEP_REGISTRY.tsv`: compact status table for each reproduction step.
- `LOG.md`: human-readable action log and decisions.
- `scripts/d0-reproctl`: lightweight status/log/doctor helper.

## Standard Commands

Run from LXPLUS:

```bash
cd /afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction
./scripts/d0-reproctl status
./scripts/d0-reproctl doctor
./scripts/d0-reproctl log "short action description"
```

Use scripts for repeated work. Keep command lines and outputs needed for
reproducibility in `logs/`.
