# CMS D+ Mass Peak Search

<!-- DOC_OWNER: cms-analysis-manager D+ task workspace. -->
<!-- TOKEN_NOTE: separate from D0 AN reproduction; use this file and RUNS.md before scanning logs. -->

Goal: find a credible \(D^+ \to K\pi\pi\) mass peak in 2023 UPC PbPb data.

This task is intentionally separate from the D0 AN reproduction. It may reuse
the D0 foresting, ZDC/HF, CRAB, and Overleaf plumbing, but outputs and logs
belong under the D+ workspace and the CERN-side `dplus-peak` area.

Default CERN write roots:

- AFS/work: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak`
- EOS output: `/eos/user/e/evzhang/codex-eos/outputs/dplus-peak`
- CRAB output LFN: `/store/user/evzhang/codex-dplus-crab-smoke`

Current strategy:

1. Enable Dfinder D+/D- Kpipi channels in the 2023 UPC forest config.
2. Run true bounded local cmsRun smoke tests before CRAB.
3. Inspect `Dfinder/ntDkpipi` schema and entries.
4. Build a lightweight D+ mass-peak reducer/plotter.
5. Scale to a medium run only after smoke tests confirm the tree is populated.
