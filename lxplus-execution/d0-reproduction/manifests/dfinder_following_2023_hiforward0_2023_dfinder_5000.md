# 2023 Dfinder-Following D0 Run: hiforward0_2023_dfinder_5000

<!-- DOC_OWNER: cms-analysis-manager Dfinder-following reproduction manifest. -->
<!-- TOKEN_NOTE: detailed stdout is in logs; plots and cutflow are in the run directory. -->

## Inputs

- MINIAOD input: `root://xrootd-vanderbilt.sites.opensciencegrid.org//store/hidata/HIRun2023A/HIForward0/MINIAOD/16Jan2024-v1/2810000/3cb20bb5-a7b1-43a2-8025-9807f4f51e71.root`
- Max events: `5000`
- Reference entries scanned per comparison stage: `2000000`
- Reference Dfinder skim: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root`

## Dfinder Configuration

- CMSSW source: `/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src`
- Config: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_d0_dfinder_only_cfg.py`
- Track label: `packedPFCandidates`
- Track chi2 label: `packedPFCandidateTrackChi2`
- Vertex label: `offlineSlimmedPrimaryVertices`
- Beam spot label: `offlineBeamSpot`
- D channels: `K+pi-` and `K-pi+` only
- Dfinder preselection: track pT > 0.2 GeV, |eta| < 2.4, VtxChiProb > 0.05, svpvDistance < 0.5, alpha < 4

## Outputs

- Dfinder output: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/dfinder-following-2023/hiforward0_2023_dfinder_5000/d0_dfinder_only.root`
- Local scratch output: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scratch/hiforward0_2023_dfinder_5000_20260615T232942Z_d0_dfinder_only.root`
- Run directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/dfinder-following-2023/hiforward0_2023_dfinder_5000`
- Run log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_d0_dfinder_following_hiforward0_2023_dfinder_5000_20260615T232942Z.log`

## Comparison Summary

## Counts

- `stage0_raw` Dfinder output: reproduced `198972`, reference `11250`
- `stage1_kin` 2<pT<12, |y|<2: reproduced `17421`, reference `5984`
- `stage2_track` track quality: reproduced `3278`, reference `3907`
- `stage3_topology` topology selected: reproduced `30`, reference `1846`

## Peak Fits

- `reproduced_topology_selected` fit status: `3`, mean: `1.8672441 GeV`, sigma: `0.00084816027 GeV`
- `reference_topology_selected` fit status: `0`, mean: `1.863945 GeV`, sigma: `0.019665854 GeV`

## Selection Notes

- Stage 0 is the Dfinder Kpi ntuple output mass window.
- Stage 1 adds the analysis pT/y region: `2 < Dpt < 12`, `|Dy| < 2`.
- Stage 2 adds daughter-track pT/eta/relative-pT-error cuts when those branches exist.
- Stage 3 recomputes the pT/|y|-binned topology cuts from D chi2 probability, decay-length significance, alpha, and dtheta when branches exist; otherwise the reference `DpassCut23PAS` flag is used.
- The reproduced sample is limited by the requested MINIAOD event count; absolute yields are not expected to match the full reference skim until the same event set is processed.
