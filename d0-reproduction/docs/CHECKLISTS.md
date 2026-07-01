# CMS D0 Analysis Checklists

<!-- DOC_OWNER: cms-analysis-manager validation checklist. -->
<!-- TOKEN_NOTE: use this as the reviewer prompt source; do not expand into long prose. -->

## Mass Peak Comparison

- Same observable and units.
- Same event/candidate selection.
- Same D0 decay channel and charge treatment.
- Same trigger and run/lumi filtering.
- Same binning and fit range.
- Same normalization convention.
- Same signal model.
- Same background model.
- Same treatment of sidebands.
- Same uncertainty display.
- Peak position, width, yield, residuals, and ratio plots reviewed.
- Differences are labeled as statistical, selection, reconstruction, or unknown.

## Code

- Script inputs are explicit.
- Output filenames include dataset/run/selection/fit labels.
- Repeated commands are wrapped in scripts.
- Random seeds, fit ranges, and config constants are recorded.
- Logs are saved under `logs/` or `output/`.

## Data

- Input file list recorded.
- File counts and sizes checked.
- Branch/histogram names verified before interpretation.
- Empty or failed files flagged.
- 2025 and 2026 samples are not accidentally mixed.

## Plot Construction

- Plot owner is `cms-plotting-producer` unless Primary Codex overrides.
- Plot inputs, cuts, branch names, binning, labels, output paths, and
  regeneration command are recorded.
- Supervisor-facing plots are saved as PNG and PDF when practical.
- Binned plot sets include metadata and bin-count summaries when practical.
- No spectra, fit values, or visual conclusions are inferred without data.

## Math/Statistics

- Fit likelihood/objective is identified.
- Parameter constraints are explicit.
- Uncertainty convention is stated.
- Pulls/residuals are inspected.
- Claims do not exceed the evidence.

## Supervisor-Facing Output

- Fewer words, more plots.
- Every plot has a one-line takeaway.
- Caveats are explicit.
- Open decisions are separated from results.
- Next action is concrete.
