# D0 Reproduction Log

<!-- DOC_OWNER: chronological human-readable analysis log. -->
<!-- TOKEN_NOTE: append concise entries here; put bulky command output in logs/. -->

## 2026-06-15

- Created the top-down reproduction scaffold for the CMS D0 analysis on LXPLUS.
- Adopted Evan's strategy: start with polished products, reproduce final plots,
  then replace each upstream dependency until the chain reaches inputs that
  existed before the analysis-specific work.
- Set policy that reference skims/plots/tables are validation artifacts, not
  permanent starting inputs.
- No heavy analysis job, CRAB submission, bulk transfer, or destructive action
  has been run from this workspace.
- Ran the first top-down 2023 D0 mass-peak validation from the combined Jan
  2024 ReReco D0 reference skim.
  - Script:
    `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_mass_peak.sh`
  - Input:
    `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root`
  - Output:
    `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/first-mass-peak-2023`
  - Selection used for the main plot:
    `Dpt>2 && Dpt<12 && abs(Dy)<2 && DpassCut23PAS`
  - Processed `54386911` tree entries.
  - `DpassCut23PAS` selected values: `58623`; histogram integral in
    `1.70 < m(Kpi) < 2.05` GeV: `22403`; simple Gaussian+linear fit mean:
    `1.86400522698` GeV; sigma: `0.0128314303572` GeV.
  - `DpassCutNominal` selected values: `126349`; histogram integral:
    `48846`; simple Gaussian+linear fit mean: `1.86407468739` GeV; sigma:
    `0.0129692065378` GeV.
  - Caveat: this is a reference-skim validation rung, not a final full-chain
    reproduced analysis result.
- Recreated the first practical 2023 HIForward0 selected-skim pass from forest
  files using the analysis-shaped Section 5 and Section 7 macros.
  - Wrapper:
    `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_d0_selected_skim.sh`
  - Forest source:
    `/eos/user/e/evzhang/codex-eos/datasets/vanderbilt_HIForward0_251227_162520_0000_0006`
  - Input files: `27`; input forest events: `9597325`.
  - Section 5 output:
    `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/hiForward0_27files_selected_first_pass/section5_event_selected.root`
  - Section 5 selected events: `5769934`; Section 5 candidates:
    `9843575`.
  - Section 7 output:
    `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/hiForward0_27files_selected_first_pass/section7_d_selected.root`
  - Section 7 selected D candidates: `1348`.
  - Plots:
    `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/hiForward0_27files_selected_first_pass/plots`
  - Manifest:
    `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/manifests/recreated_selected_skim_2023_hiForward0_27files_selected_first_pass.md`
  - Caveat: this is an analysis-shaped selected skim, not yet a branch-for-
    branch clone of the Wang reference `Tree` skim.
- Attempted an event-vector micro-skim prototype:
  `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_d0_skim_recreation.sh`.
  A one-event diagnostic succeeded, but raw vector copying was too slow for
  full-sample production, so the practical path was switched to the selected-
  skim pipeline above.
- Wrote the upstream plan for replacing analysis-produced inputs with a chain
  starting from official MINIAOD:
  `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/UPSTREAM_RECREATION_PLAN.md`.
- Added bounded forest-smoke helper:
  `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_forest_smoke.sh`.
- Ran a 10-event forest smoke attempt from the 2023 HIForward0 MINIAOD test
  file. It failed before event processing because the current `CMSSW_16_1_1`
  area cannot import
  `HeavyIonsAnalysis.ZDCAnalysis.ZDCAnalyzersHC2023_cff`.
  - Log:
    `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_forest_smoke_hiforward0_miniaod_10_20260615T211301Z.log`
  - Output directory:
    `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-forest-2023/smoke/hiforward0_miniaod_10`
  - Current interpretation: the first upstream blocker is exact CMSSW/ZDC
    package reproducibility. The current area has `ZDCAnalyzersPbPb_cff.py`
    and `zdcSequencePbPb`, but the 2023 config expects
    `ZDCAnalyzersHC2023_cff.py` and `zdcSequence`. Do not silently substitute
    these in nominal production until validated.

Open questions before first analysis step:

- Which final plot or table should be reproduced first?
- Which analysis note/paper draft versions should be treated as the reference?
- Which polished ROOT/table input is the cleanest Level 1 starting artifact?

## Cut Recreation Audit: 2026-06-15T21:45:24Z

- Label: `hiForward0_27files_selected_first_pass`
- Section 5 input: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/hiForward0_27files_selected_first_pass/section5_event_selected.root`
- Section 7 input: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/hiForward0_27files_selected_first_pass/section7_d_selected.root`
- Output directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-recreation-audits/hiForward0_27files_selected_first_pass`
- Scope: audit current nominal cuts and threshold scans; not final EBX/MC derivation.

## Cut Derivation: 2026-06-16T12:40:30Z

- Label: `smoke_5k`
- Signal MC: `/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root`
- Data sideband sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root`
- Control sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root`
- Max events per sample: `5000`
- Output directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/smoke_5k`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_cut_derivation_smoke_5k_20260616T123959Z.log`
- Scope: MC/data/EBX cut derivation scan; bounded runs are provisional.

## Cut Derivation: 2026-06-16T12:42:58Z

- Label: `smoke_5k_v2`
- Signal MC: `/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root`
- Data sideband sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root`
- Control sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root`
- Max events per sample: `5000`
- Output directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/smoke_5k_v2`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_cut_derivation_smoke_5k_v2_20260616T124223Z.log`
- Scope: MC/data/EBX cut derivation scan; bounded runs are provisional.

## Cut Derivation: 2026-06-16T12:48:16Z

- Label: `smoke_5k_cached`
- Signal MC: `/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root`
- Data sideband sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root`
- Control sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root`
- Max events per sample: `5000`
- Output directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/smoke_5k_cached`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_cut_derivation_smoke_5k_cached_20260616T124807Z.log`
- Scope: MC/data/EBX cut derivation scan; bounded runs are provisional.

## Cut Derivation: 2026-06-16T12:48:28Z

- Label: `prompt_mc_data_sideband_emptybx_200k`
- Signal MC: `/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root`
- Data sideband sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root`
- Control sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root`
- Max events per sample: `200000`
- Output directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/prompt_mc_data_sideband_emptybx_200k`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_cut_derivation_prompt_mc_data_sideband_emptybx_200k_20260616T124823Z.log`
- Scope: MC/data/EBX cut derivation scan; bounded runs are provisional.

## Cut Derivation: 2026-06-16T12:49:29Z

- Label: `smoke_5k_data_eventfix`
- Signal MC: `/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root`
- Data sideband sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root`
- Control sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root`
- Max events per sample: `5000`
- Output directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/smoke_5k_data_eventfix`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_cut_derivation_smoke_5k_data_eventfix_20260616T124921Z.log`
- Scope: MC/data/EBX cut derivation scan; bounded runs are provisional.

## Cut Derivation: 2026-06-16T12:49:51Z

- Label: `prompt_mc_data_sideband_emptybx_200k`
- Signal MC: `/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root`
- Data sideband sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root`
- Control sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root`
- Max events per sample: `200000`
- Output directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/prompt_mc_data_sideband_emptybx_200k`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_cut_derivation_prompt_mc_data_sideband_emptybx_200k_20260616T124946Z.log`
- Scope: MC/data/EBX cut derivation scan; bounded runs are provisional.

## MINIAOD Forest Smoke: 2026-06-16T13:12:40Z

- Label: `hiforward0_miniaod_100_d0_zdc_pf_track_hcalsev`
- Input MINIAOD: `root://xrootd-vanderbilt.sites.opensciencegrid.org//store/hidata/HIRun2023A/HIForward0/MINIAOD/16Jan2024-v1/2810000/3cb20bb5-a7b1-43a2-8025-9807f4f51e71.root`
- Output forest: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-forest-2023/smoke/hiforward0_miniaod_100_d0_zdc_pf_track_hcalsev/HiForest_2023PbPbUPC_Jan24Reco_smoke.root`
- Runner: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_2023_forest_smoke.sh`
- Schema validator: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/validate_recreated_forest_schema.C`
- Result: core schema passes for Dfinder, ZDC rechit, PF/HF, skim, HLT, event, and track trees.
- Counts from 100 input events: `10` selected forest entries, `7` entries with nonzero `Dsize`, `1533` total Dfinder Kpi candidates.
- ZDC foresting fix: generated configs use `ZDCAnalyzersPbPb_cff`/`zdcSequencePbPb`/`zdcrecoRun3` plus `hcalRecAlgoESProd_cfi`; the old `ZDCAnalyzersHC2023_cff` path is not present in this checkout.
- Smoke caveats: PF jet tree disabled, empty `inst_lumi.json` staged, and no full CRAB/HIForward production launched.

## Cut Derivation: 2026-06-16T18:52:03Z

- Label: `prompt_mc_data_sideband_emptybx_1m_20260616`
- Signal MC: `/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root`
- Data sideband sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root`
- Control sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root`
- Max events per sample: `1000000`
- Output directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/prompt_mc_data_sideband_emptybx_1m_20260616`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_cut_derivation_prompt_mc_data_sideband_emptybx_1m_20260616_20260616T185140Z.log`
- Scope: MC/data/EBX cut derivation scan; bounded runs are provisional.

## Cut Derivation: 2026-06-16T18:55:39Z

- Label: `prompt_mc_data_sideband_emptybx_5m_20260616`
- Signal MC: `/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root`
- Data sideband sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root`
- Control sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root`
- Max events per sample: `5000000`
- Output directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/prompt_mc_data_sideband_emptybx_5m_20260616`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_cut_derivation_prompt_mc_data_sideband_emptybx_5m_20260616_20260616T185409Z.log`
- Scope: MC/data/EBX cut derivation scan; bounded runs are provisional.

## Cut Derivation: 2026-06-17T22:36:57Z

- Label: `topocuts_rect_full_20260617`
- Signal MC: `/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root`
- Data sideband sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root`
- Control sample: `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root`
- Max events per sample: `-1`
- Output directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/topocuts_rect_full_20260617`
- Log: `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/logs/run_2023_cut_derivation_topocuts_rect_full_20260617_20260617T222016Z.log`
- Scope: MC/data/EBX cut derivation scan; bounded runs are provisional.

## Cut Recreation Audit: 2026-06-18T20:59:07Z

- Label: `crab_cern_twofile_medium_apriori_20260618`
- Section 5 input: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/crab_cern_twofile_medium_apriori_20260618/section5_event_selected.root`
- Section 7 input: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/crab_cern_twofile_medium_apriori_20260618/section7_d_selected.root`
- Output directory: `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-recreation-audits/crab_cern_twofile_medium_apriori_20260618`
- Scope: audit current nominal cuts and threshold scans; not final EBX/MC derivation.
