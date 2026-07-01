# CMS Heavy-Flavor Analysis Plan

<!-- DOC_OWNER: cms-analysis-manager living summer-analysis plan. -->
<!-- TOKEN_NOTE: update this file instead of rediscovering analysis structure. Keep facts marked by provenance. -->

Status: draft v0, created before the official analysis note/paper draft is ingested.

Scope:

- First reproduce the existing 2023 \(D^0\) measurement.
- Then extend the workflow to a new charm-hadron channel, such as \(D^+\) or
  \(D_s^+\), after the \(D^0\) reproduction is validated.

Core anti-hallucination rule:

- No final physics claim unless it is traceable to a data file, code path,
  command output, analysis-note section, paper-draft section, or explicitly
  stated assumption.

## Source-Handling Policy

Analysis notes, paper drafts, private code, and internal plots should be treated
as controlled source material.

- Use the analysis note as a specification and provenance source.
- Do not copy large chunks of note text into public or reusable output buffers.
- Do not reuse private plots as final outputs unless explicitly requested.
- Reproduce plots from data/code whenever possible.
- Reproduce analysis-specific intermediate products as well as final plots. In
  particular, existing skims and merged ROOT outputs are validation references,
  not nominal starting inputs, because they may encode particle-specific
  selections and dropped branches.
- For the D0 reproduction, regenerate the D0 skim/reducer output from the
  least-reduced practical input, then compare against the existing D0 skims.
  This is the portability test for later \(D^+\), \(D_s^+\), and \(D^*\)
  channels.
- If exact methods or code are required, cite the source internally and rewrite
  the workflow as reproducible local scripts.
- Mark every inherited assumption as `from-note`, `from-code`, `from-paper`,
  `from-supervisor`, or `unverified`.

## Phase 0: Intake And Registry Setup

Goal: build the source of truth before producing physics outputs.

Inputs to collect:

- Analysis note and paper draft locations.
- Official 2023 \(D^0\) reproduction target.
- Data sample paths and run list.
- MC sample paths and production details.
- Trigger, skim, and event-filter definitions.
- Luminosity or event-normalization information.
- Existing code repository or CMSSW area.
- Official plot/table list and expected tolerances.

Artifacts to create:

- `registry.yaml`: machine-readable sample/run/output registry.
- `DATASETS.md`: human-readable dataset notes.
- `RUNS.md`: run-period and run-quality notes.
- `COMMANDS.md`: reproducible commands.
- `CHECKLISTS.md`: review and validation checklists.
- `SUPERVISOR_SUMMARY.md`: compact status buffer.
- `logs/analysis-ledger.jsonl`: append-only local action log.

Open questions:

- What is the canonical analysis note path?
- What paper draft should be reviewed?
- What is the collision system and energy for the summer measurement?
- What exact 2023 \(D^0\) observable is the reproduction target?
- Which new hadron is the preferred extension target?

## Phase 1: Conceptual Map

Goal: express the analysis as a dependency graph before coding.

Map these components:

- Dataset provenance.
- MC provenance.
- Event selection.
- Track and vertex selection.
- \(D^0\) candidate reconstruction.
- Signal extraction from invariant-mass distributions.
- Acceptance and efficiency corrections.
- Prompt/nonprompt treatment, if relevant.
- Cross-section definition and normalization.
- Systematic uncertainty model.
- Model comparison and external references.
- Final tables and plots.

Output:

- A short conceptual map with each node labeled by input, output, owner script,
  and validation checks.

## Phase 2: Pseudocode Implementation

Goal: make the workflow unambiguous before running large jobs.

For each analysis step, define:

- Inputs.
- Outputs.
- Selection cuts.
- Branch names.
- Histogram binning.
- Fit model.
- Correction formula.
- Normalization formula.
- Validation checks.
- Failure modes.

Example skeleton:

```text
load data and MC samples
apply certified run/event selection
build charm-hadron candidates
fill candidate mass histograms in analysis bins
fit signal plus background model
extract raw yields
derive acceptance times efficiency from MC
apply prompt/nonprompt correction if required
compute corrected yields or differential cross sections
evaluate systematic variations
compare to official reproduction target
compare to models only after internal validation
```

## Phase 3: Reproduce Existing 2023 \(D^0\) Measurement

Goal: validate the full pipeline on a known result before measuring anything
new.

Required gates:

- Input files match the official analysis.
- Analysis-specific intermediate products are reproduced, including foresting
  where analysis-owned and the D0 skim/reducer step.
- Existing skims are matched as references by event count, candidate count,
  branch schema, cutflow, and basic mass spectra.
- Cutflow matches the official analysis within agreed tolerance.
- Raw mass spectra match expected shapes and yields.
- Fit results reproduce official yields within agreed tolerance.
- MC correction factors match official factors within agreed tolerance.
- Final observable reproduces the official result within agreed tolerance.

Reproduction table template:

| Observable | Official | Reproduced | Difference | Status |
| --- | ---: | ---: | ---: | --- |
| raw yield | TBD | TBD | TBD | pending |
| \(A\epsilon\) | TBD | TBD | TBD | pending |
| corrected yield | TBD | TBD | TBD | pending |
| cross section | TBD | TBD | TBD | pending |

Do not proceed to the new-hadron measurement until the reproduction target is
either passed or the discrepancy is documented and accepted.

## Phase 4: New-Hadron Extension

Goal: reuse the validated structure for a new charm-hadron channel.

Candidate channels:

- \(D^+\): likely simpler first extension.
- \(D_s^+\): likely more difficult because of larger backgrounds and
  intermediate-resonance structure.

For each new channel, define:

- Decay channel.
- Daughter track selection.
- Candidate topology and vertex selection.
- Mass window and fit model.
- MC truth matching.
- Efficiency correction.
- Feed-down or prompt/nonprompt treatment.
- Systematic variations.
- Cross-section or yield-ratio observable.

Do not assume \(D^0\) cuts transfer unchanged. Each inherited cut must be
justified by note, MC closure, or data-driven validation.

## Cross-Check Pipeline

Every important output should pass these roles:

- Producer: creates plots/tables/scripts.
- Data checker: verifies samples, paths, branches, event counts, and cutflows.
- Code checker: verifies script logic, binning, normalization, and reproducibility.
- Physics checker: challenges interpretation and formulas.
- Reviewer: separates verified facts from assumptions and uncertainties.

For each output, record:

- Input files.
- Tree and branch names.
- Cuts.
- Binning.
- Fit model.
- Correction formula.
- Normalization.
- Script path.
- Git commit or file timestamp.
- Known limitations.

## Evidence Labels

Use these labels in notes and summaries:

- `verified`: checked directly in data/code/output.
- `from-note`: taken from analysis note.
- `from-paper`: taken from paper draft or publication.
- `from-code`: taken from existing analysis code.
- `from-supervisor`: stated by supervisor/collaborator.
- `assumption`: used provisionally and needs confirmation.
- `hypothesis`: plausible explanation, not established.
- `blocked`: needs missing input or clarification.

## Immediate Next Steps

1. Use `ANALYSIS_ROADMAP.md` as the section-indexed build path from the
   uploaded analysis note.
2. Extract the official sample list, run list, selections, and target plots
   into compact machine-readable registries.
3. Build the conceptual map from the roadmap.
4. Write pseudocode for the \(D^0\) reproduction.
5. Define reproduction tolerances with Evan/supervisor.
