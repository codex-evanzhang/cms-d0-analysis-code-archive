# Produced-Skim D0 Peak: 2023 HIForward0

<!-- DOC_OWNER: cms-analysis-manager produced SelectedD skim plotting. -->
<!-- TOKEN_NOTE: the output manifest and CSVs contain the reusable numeric summary. -->

## Inputs

- Produced skim: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/crab_98forest_apriori_20260619/section7_d_selected.root`
- Tree: `SelectedD`
- Plot region: `1.70 < Dmass < 2.05`, `2 < Dpt < 12`, `abs(Dy) < 2`

## Outputs

- Output directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/produced-skim-d0-peak-2023/crab_98forest_apriori_20260619`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_produced_skim_d0_peak_20260619T160325Z.log`

## Numeric Manifest

```text
result: 2023 D0 peak from reproduced SelectedD skim
input: /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/crab_98forest_apriori_20260619/section7_d_selected.root
tree: SelectedD
total_tree_entries: 3143
processed_tree_entries: 3143
max_entries_argument: 0
mass_range_gev: 1.70,2.05
bins: 140
bin_width_gev: 0.0025
selection_note: SelectedD already contains the recreated first-pass D0 selection; this plot reapplies only 2<Dpt<12 and |Dy|<2.

h_selected_d0_mass_2pt12_absy2
  cut: Dmass>1.70 && Dmass<2.05 && Dpt>2 && Dpt<12 && abs(Dy)<2
  tree_draw_selected_values: 2678
  histogram_entries: 2678
  histogram_integral: 2678
  max_bin_center: 1.86375
  max_bin_count: 46

simple_fit_status: 0
simple_fit_model: gaus(0)+pol1(3) over 1.78-1.95 GeV
simple_fit_mean: 1.86235561253
simple_fit_sigma: 0.012887818345
simple_fit_chi2: 86.276856681
simple_fit_ndf: 63

h_pt0_y0
  cut: Dmass>1.70 && Dmass<2.05 && Dpt>2 && Dpt<5 && Dy>-2 && Dy<-1
  tree_draw_selected_values: 435
  histogram_entries: 435
  histogram_integral: 435
  max_bin_center: 1.70375
  max_bin_count: 8

h_pt0_y1
  cut: Dmass>1.70 && Dmass<2.05 && Dpt>2 && Dpt<5 && Dy>-1 && Dy<0
  tree_draw_selected_values: 395
  histogram_entries: 395
  histogram_integral: 395
  max_bin_center: 1.71125
  max_bin_count: 9

h_pt0_y2
  cut: Dmass>1.70 && Dmass<2.05 && Dpt>2 && Dpt<5 && Dy>0 && Dy<1
  tree_draw_selected_values: 471
  histogram_entries: 471
  histogram_integral: 471
  max_bin_center: 1.86375
  max_bin_count: 12

h_pt0_y3
  cut: Dmass>1.70 && Dmass<2.05 && Dpt>2 && Dpt<5 && Dy>1 && Dy<2
  tree_draw_selected_values: 412
  histogram_entries: 412
  histogram_integral: 412
  max_bin_center: 1.71625
  max_bin_count: 8

h_pt1_y0
  cut: Dmass>1.70 && Dmass<2.05 && Dpt>5 && Dpt<8 && Dy>-2 && Dy<-1
  tree_draw_selected_values: 136
  histogram_entries: 136
  histogram_integral: 136
  max_bin_center: 1.85625
  max_bin_count: 5

h_pt1_y1
  cut: Dmass>1.70 && Dmass<2.05 && Dpt>5 && Dpt<8 && Dy>-1 && Dy<0
  tree_draw_selected_values: 179
  histogram_entries: 179
  histogram_integral: 179
  max_bin_center: 1.86625
  max_bin_count: 9

h_pt1_y2
  cut: Dmass>1.70 && Dmass<2.05 && Dpt>5 && Dpt<8 && Dy>0 && Dy<1
  tree_draw_selected_values: 190
  histogram_entries: 190
  histogram_integral: 190
  max_bin_center: 1.85375
  max_bin_count: 6

h_pt1_y3
  cut: Dmass>1.70 && Dmass<2.05 && Dpt>5 && Dpt<8 && Dy>1 && Dy<2
  tree_draw_selected_values: 134
  histogram_entries: 134
  histogram_integral: 134
  max_bin_center: 1.70375
  max_bin_count: 5

h_pt2_y0
  cut: Dmass>1.70 && Dmass<2.05 && Dpt>8 && Dpt<12 && Dy>-2 && Dy<-1
  tree_draw_selected_values: 56
  histogram_entries: 56
  histogram_integral: 56
  max_bin_center: 1.70375
  max_bin_count: 3

h_pt2_y1
  cut: Dmass>1.70 && Dmass<2.05 && Dpt>8 && Dpt<12 && Dy>-1 && Dy<0
  tree_draw_selected_values: 101
  histogram_entries: 101
  histogram_integral: 101
  max_bin_center: 1.86125
  max_bin_count: 7

h_pt2_y2
  cut: Dmass>1.70 && Dmass<2.05 && Dpt>8 && Dpt<12 && Dy>0 && Dy<1
  tree_draw_selected_values: 100
  histogram_entries: 100
  histogram_integral: 100
  max_bin_center: 1.78125
  max_bin_count: 4

h_pt2_y3
  cut: Dmass>1.70 && Dmass<2.05 && Dpt>8 && Dpt<12 && Dy>1 && Dy<2
  tree_draw_selected_values: 69
  histogram_entries: 69
  histogram_integral: 69
  max_bin_center: 1.86375
```

## pT-y Counts

```csv
pt_low,pt_high,y_low,y_high,entries,max_bin_center,max_bin_count
2,5,-2,-1,435,1.70375,8
2,5,-1,0,395,1.71125,9
2,5,0,1,471,1.86375,12
2,5,1,2,412,1.71625,8
5,8,-2,-1,136,1.85625,5
5,8,-1,0,179,1.86625,9
5,8,0,1,190,1.85375,6
5,8,1,2,134,1.70375,5
8,12,-2,-1,56,1.70375,3
8,12,-1,0,101,1.86125,7
8,12,0,1,100,1.78125,4
8,12,1,2,69,1.86375,4
```
