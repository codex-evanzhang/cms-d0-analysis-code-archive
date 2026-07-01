# D+ Generalization Code

This folder contains the scripts and notes used for the D+ to Kpipi peak test.

This was not a full D+ analysis. It was a generalization check: use the D0
forest/skim/plotting experience to enable Dfinder `ntDkpipi`-style output and
find a credible preliminary D+ mass peak.

Main scripts:

- `run_2023_dplus_forest_smoke.sh`: bounded local D+ foresting smoke test.
- `run_2023_dplus_forest_crab_smoke.sh`: CRAB smoke/medium-run helper for D+
  foresting.
- `minimize_2023_dplus_forest.C`: prune D+ forests to analysis-relevant
  branches.
- `plot_dplus_mass.C`: diagnostic D+ mass spectrum plotter.
- `monitor_dplus_crab_and_plot.sh`: monitor finished CRAB outputs and run
  preliminary peak plots.
