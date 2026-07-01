# Top-Down D0 Analysis Reproduction Plan

<!-- DOC_OWNER: cms-analysis-manager top-down reproduction policy. -->
<!-- TOKEN_NOTE: this is the compact task contract; update it when the analysis strategy changes. -->

## Objective

Reproduce the CMS D0 analysis as if the original analysis-specific products had
not already been produced. Existing polished artifacts can be used to validate
the reproduction, but the final chain should recreate every analysis-specific
step from the least-reduced practical inputs.

## Strategy

Work top-down first, then replace each dependency with a reproduced upstream
product.

1. Final polished outputs
   - Identify the final plots, tables, fit products, and note/paper numbers.
   - Recreate them once using the most polished available reference inputs.
   - This verifies plotting conventions, bin definitions, axes, labels, and
     fit model behavior before upstream complexity is introduced.

2. Final reduced analysis inputs
   - Locate the ROOT files or tables that feed the final plots.
   - Recreate final plots from those files with independent scripts.
   - Record exact branches, histograms, selections, weights, and fit ranges.

3. Analysis skims and candidate ntuples
   - Reproduce the D0 candidate selection outputs used by the final reduced
     inputs.
   - Check event counts, candidate counts, branch schemas, and invariant-mass
     spectra against reference skims.
   - Treat these skims as products to reproduce, not as permanent starting
     inputs.

4. Forest/MINIAOD inputs and certification
   - Confirm official dataset names, run/lumi certification JSONs, trigger
     menus, and forest production instructions from CMS documentation/TWiki.
   - Recreate the analysis candidate skims from the least-reduced practical
     inputs available to Evan.

5. Corrections and cross-section construction
   - Reproduce efficiency, acceptance, signal extraction, normalization, and
     systematic uncertainty machinery.
   - Keep correction derivations separable from final plotting.

6. Extension readiness
   - After the D0 chain is reproducible, identify which steps are reusable for
     D+, Ds+, or D* and which are channel-specific.

## Step Gate Rules

Each step is complete only when it has:

- A script or command recipe stored in this workspace.
- An input manifest.
- An output manifest.
- A validation comparison against the appropriate reference product.
- A log entry with date, command, status, and open questions.
- A reviewer/checker note for nontrivial physics or statistical claims.

## Stop Conditions

Stop and update Evan before continuing if any of the following happen:

- Required data location, branch meaning, trigger, skim definition, correction,
  fit model, or normalization is ambiguous.
- A CMS TWiki/documentation instruction conflicts with existing local code.
- A command requires CRAB submission, bulk dataset transfer, proxy renewal with
  sensitive credentials, public service exposure, or large compute commitment.
- A result disagrees with the reference and the discrepancy is not understood.
- A script would write outside Evan-owned CERN directories.

## Immediate Next Step

Start with a read-only inventory of final polished products and reference
analysis inputs. Then choose the first final plot to reproduce from the most
polished available ROOT input.
