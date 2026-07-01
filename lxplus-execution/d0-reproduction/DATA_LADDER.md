# D0 Reproduction Data Ladder

<!-- DOC_OWNER: compact data-product dependency ladder. -->
<!-- TOKEN_NOTE: keep this as a short map; detailed manifests belong in manifests/. -->

This file tracks the top-down dependency ladder. Each product starts as a
reference artifact, then should be replaced by a reproduced version.

## Level 0: Published/Note-Level Products

Purpose:

- Human-facing plots, tables, and final numerical results.
- Used to define the target behavior and validate the final reproduction.

Status:

- Needs inventory from analysis note and paper draft.

## Level 1: Final Plot/Fit Inputs

Purpose:

- Histograms, fit workspaces, ROOT trees, correction tables, and summary tables
  directly used to make Level 0 products.

Status:

- First top-down mass-peak product created from the combined 2023 D0 reference
  skim. This validates the basic branch/cut/plot path before upstream
  reproduction.
- Output manifest:
  `manifests/first_mass_peak_2023.md`
- EOS outputs:
  `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/first-mass-peak-2023`

## Level 2: Reduced Candidate Outputs

Purpose:

- Analysis-specific selected D0 candidate ntuples or histograms.
- Should be reproduced, because analogous products are needed for D+, Ds+, or
  D* extensions.

Known context:

- Prior local context used selected-D ROOT trees with branches like `Dmass`,
  `Dpt`, and `Dy` for mass-peak checks. Those are validation context, not the
  final reproduction start.
- The first 2023 mass-peak check used `Dmass`, `Dpt`, `Dy`, `DpassCut23PAS`,
  and `DpassCutNominal` from the reference skim. These branches now need to be
  traced back through the D0 skim/reducer step.

## Level 3: D0 Skims

Purpose:

- Channel-specific skims produced from forest/MINIAOD inputs.
- Must be regenerated for a true reproduction.

Known context:

- CMS HiForest TWiki lists Jan 2024 ReReco D0 skim references and official data
  inputs. These references should be used for validation while rebuilding.

## Level 4: Forest/MINIAOD Inputs

Purpose:

- Least-reduced practical inputs for this reproduction, subject to available
  CMS tools and documentation.

Known context:

- Jan 2024 ReReco target context from HiForest2025PbPbHF TWiki:
  `/HIForward[0-19]/HIRun2023A-16Jan2024-v1/MINIAOD`
- Golden JSON context:
  `/eos/user/c/cmsdqm/www/CAF/certification/Collisions23HI/Cert_Collisions2023HI_374288_375823_Good_ZDC_Golden.json`

## Level 5: Certification, Triggers, and Conditions

Purpose:

- Run/lumi certification, triggers, detector conditions, calibrations, and CMS
  software setup.

Status:

- Must be confirmed from CMS documentation/TWiki before production-scale work.
