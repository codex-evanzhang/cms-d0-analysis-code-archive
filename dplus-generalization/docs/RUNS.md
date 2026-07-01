# D+ Run Log

<!-- DOC_OWNER: cms-analysis-manager concise D+ run state. -->
<!-- TOKEN_NOTE: update after each meaningful smoke, CRAB, skim, or plot checkpoint. -->

## 2026-06-22

- Task started as a separate D+ peak search, not a D0 AN reproduction step.
- Existing D0 98-forest outputs and official reference forest both contain
  `Dfinder/ntDkpipi`, but the checked files have zero entries because the base
  forest config has D+/D- Kpipi channels disabled.
- Base config finding:
  `process.Dfinder.Dchannel = cms.vint32(1,1,0,0,...)`; D+ requires setting
  the third and fourth entries to `1`.
- First D+ smoke attempts showed `Unmatched input parameter vector size`; the
  override had only 14 Dchannel entries while this Dfinder build expects 18.
  Fixed the override to keep all channels present while enabling only D+/D-.
- Corrected local 100-event no-superfilter smoke:
  `/eos/user/e/evzhang/codex-eos/outputs/dplus-peak/recreated-forest-2023/smoke/hiforward0_dplus_miniaod_100_nosuperfilter_fixed_20260622/HiForest_2023PbPbUPC_Jan24Reco_smoke.root`.
  Result: `Dfinder/ntDkpipi` has 100 event entries.
- Corrected local 100-event normal UPC-filtered smoke:
  `/eos/user/e/evzhang/codex-eos/outputs/dplus-peak/recreated-forest-2023/smoke/hiforward0_dplus_miniaod_100_fixed_20260622/HiForest_2023PbPbUPC_Jan24Reco_smoke.root`.
  Result: 11 filtered event entries and 8,130 D+ candidates before diagnostic
  mass/kinematic cuts.
- First smoke mass diagnostic:
  `/eos/user/e/evzhang/codex-eos/outputs/dplus-peak/plots/smoke_100_fixed_20260622/`.
  Result: 8,111 raw Kpipi candidates, 1,284 candidates with
  `2 < Dpt < 20, |Dy| < 2`, and 34 after the loose topology diagnostic. This
  validates branch production and plotting, but is not a credible peak claim.
- Submitted one-job CRAB smoke on the physically bounded 100-event MINIAOD file:
  task `260622_232410:evzhang_crab_dplus_minimal_tiny_crab_smoke_20260622_2323`,
  project
  `/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak/crab-smoke/crab-projects/crab_dplus_minimal_tiny_crab_smoke_20260622_2323`.
  Status after submission: queued on command `SUBMIT`.
- The tiny CRAB smoke failed before cmsRun physics because CRAB resolved
  `/store/user/evzhang/...` through `/eos/cms/store/user/...`, while the tiny
  file is on CERNBOX at `/eos/user/e/evzhang/...`. Canceled the task to avoid
  useless retries.
- Submitted replacement one-real-MINIAOD-file CRAB smoke:
  task `260622_233514:evzhang_crab_dplus_onefile_crab_smoke_20260622_2338`,
  project
  `/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak/crab-smoke/crab-projects/crab_dplus_onefile_crab_smoke_20260622_2338`.
  Input LFN:
  `/store/hidata/HIRun2023A/HIForward0/MINIAOD/16Jan2024-v1/2810000/0640e99d-84f5-49fd-a012-48be69252e99.root`.

## 2026-06-23

- One-file CRAB smoke completed and produced D+ forest output. The chain
  submitted the 100-file medium run:
  `260623_062056:evzhang_crab_dplus_medium_100files_20260622_overnight`.
- Medium CRAB project:
  `/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak/crab-smoke/crab-projects/crab_dplus_medium_100files_20260622_overnight`.
- Added `plot_visible_dplus_crab_outputs.sh` to build checkpoint mass plots
  from ROOT outputs currently visible on EOS without mutating CRAB jobs.
- Optimized `plot_dplus_mass.C` from repeated `TChain::Draw` calls to a
  one-pass candidate-array loop and added loose/medium/tight topology
  diagnostics.
- One-file medium checkpoint:
  `dplus_medium_prelim1_toposcan_20260623T1052Z`.
  It used 1 of 34 visible ROOT outputs and produced 142,768 loose candidates,
  2,246 medium candidates, and 71 tight candidates. No credible visible
  D+ peak claim; loose is smooth background and medium statistics are low.
- Four-file medium checkpoint:
  `dplus_medium_prelim4_readable_20260623T1100Z`.
  It used 4 of 34 visible ROOT outputs and produced 615,362 loose candidates,
  9,072 medium candidates, and 202 tight candidates. The medium diagnostic fit
  returned mean 1.87381 GeV, sigma 0.00649 GeV, yield-like 82.8, chi2/ndf
  0.74, but the standalone plot is still fluctuation-dominated rather than a
  clean peak.
- Current honest status: D+ branch production and medium CRAB output are
  validated. The preliminary 4-file checkpoint is suggestive near the nominal
  D+ mass but not enough to claim a credible peak. The 100-file monitor should
  give the first meaningful peak test.
- The original full-output monitor stopped once the VOMS proxy expired because
  `crab status` could no longer query CRIC. Patched
  `monitor_dplus_crab_and_plot.sh` so CRAB-status failure is nonfatal by
  default; it now keeps polling EOS visible outputs and will plot after 100
  ROOT files are visible.
- Resumed the 100-file EOS-output monitor with `setsid` as local PID `750147`.
  First resumed poll saw 37/100 visible ROOT outputs and continued despite the
  expired VOMS proxy.
- After VOMS refresh, CRAB status showed 30 finished, 9 transferring, and 61
  running jobs; 39 ROOT outputs were visible on EOS.
- Ran a 20-file checkpoint:
  `dplus_medium_prelim20_toposcan_20260623T1105Z`.
  It used 20/39 visible outputs and produced 2,680,780 loose candidates,
  38,122 medium candidates, and 932 tight candidates. The medium topology view
  still looked background-dominated, but the tight topology view produced a
  visible D+ peak near the nominal mass. Fit summary: tight mean 1.87190 GeV,
  sigma 0.01486 GeV, yield-like 133.7, chi2/ndf 0.97.
- Current honest status: a credible preliminary D+ -> Kpipi peak is visible
  with the tight diagnostic topology cuts in the 20-file medium checkpoint.
  This is not a final analysis result because the cuts are diagnostic and not
  yet optimized/validated for efficiency or bias.
- Ran a 59-file checkpoint from all ROOT outputs visible at the user's request:
  `dplus_medium_prelim59_toposcan_20260623T1230Z`.
  It used 59 visible outputs and produced 8,497,790 loose candidates, 118,117
  medium candidates, and 2,570 tight candidates. The tight topology plot shows
  a clear D+ peak. Fit summary: medium mean 1.87164 GeV, sigma 0.00803 GeV,
  yield-like 478.2; tight mean 1.87153 GeV, sigma 0.01142 GeV, yield-like
  231.1; both fits have chi2/ndf below 1.
