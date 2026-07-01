# Datasets

<!-- DOC_OWNER: cms-analysis-manager dataset/path notes. -->
<!-- TOKEN_NOTE: keep full file listings out of this file; store generated inventories under output/. -->

Known prior-context LXPLUS paths:

```text
/afs/cern.ch/user/e/evzhang/RootAnalysis/MITanalysisTest/510&511&523&527&529&543&549
/afs/cern.ch/user/e/evzhang/RootAnalysis/MITanalysisTest/2025Data
/afs/cern.ch/user/e/evzhang/RootAnalysis/MITanalysisTest/comparison_2025_vs_2026
```

## HiForest TWiki Inventory

Source:

- `https://twiki.cern.ch/twiki/bin/view/CMS/HiForest2025PbPbHF`
- Relevant anchor: `#2023_PbPb_UPC_Jan_2024_ReReco_Go`
- TWiki revision observed: r67, 2026-06-10
- Extracted inventory:
  `research/cms-d0-analysis/output/hiforest2025pbpbhf_datasets.md`
- Machine-readable inventory:
  `research/cms-d0-analysis/output/hiforest2025pbpbhf_datasets.yaml`
- Extractor:
  `research/cms-d0-analysis/scripts/extract_hiforest_twiki_datasets.py`

## Primary 2023 Reproduction Data

Use the Jan 2024 ReReco sample as the primary reproduction target unless Evan or
the supervisor explicitly switches the target to the newer Feb 2025 ReReco.

Reproduction policy:

- The reproduced analysis should start from the least-reduced official inputs
  available with stable provenance, preferably MINIAOD or the documented forest
  production inputs.
- Existing skims and merged ROOT outputs are reference artifacts for validation.
  They should not be the nominal starting point for final reproduced numbers.
- The D0 skim/reducer step itself should be reproduced. The regenerated skim
  should be compared against the existing skims by event count, candidate count,
  branch schema, cutflow, and basic mass spectra.
- This matters for future \(D^+\), \(D_s^+\), and \(D^*\) analyses: a
  particle-specific D0 skim may have already dropped events, candidates, tracks,
  or soft pions needed for other charm-hadron channels.

- Run range: `374804-375746`
- Golden JSON:
  `/eos/user/c/cmsdqm/www/CAF/certification/Collisions23HI/Cert_Collisions2023HI_374288_375823_Good_ZDC_Golden.json`
- MINIAOD pattern:
  `/HIForward[0-19]/HIRun2023A-16Jan2024-v1/MINIAOD`
- DAS count checked on 2026-06-18:
  `6784` MINIAOD files total across HIForward0-19, with `9.84952545e9`
  events and `21.868 TB` input size. HIForward0 alone has `319` files.
- Combined D0 skim:
  `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root`
- Per-PD D0 skim pattern:
  `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward{0..19}_Drej-pasor_Trig-4.root`
- Forest directory pattern listed on the TWiki:
  `root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/jdlang/Run3_PbPbUPC/Forest_2023_Jan2024ReReco_2025Reforest/HIForward[0-19]/crab_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward[0-19]/...`

Validation status on 2026-06-14:

- Golden JSON exists on LXPLUS.
- Combined D0 skim exists on CERN EOS.
- Per-PD D0 skim count on CERN EOS: 20 files.
- These skim files are validated as existing references only; they remain to be
  reproduced from less-reduced inputs.
- Vanderbilt XRootD forest listing is not yet validated: `xrdfs` returned
  `Auth failed: No protocols left to try`.

## Control Samples

EmptyBX:

- Skim:
  `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root`
- Forests:
  `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/forest/crab_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2/260219_210631/0000`
- MINIAOD:
  `/HIEmptyBX/HIRun2023A-PromptReco-v2/MINIAOD`

ZeroBias:

- Skim:
  `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIPhysicsRawPrime0-5_HIRun2023A_ZB_374970.root`
- Forests:
  `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/forest/crab_HiForest_260218_HIPhysicsRawPrime*_HIRun2023A_ZB_374970/*/0000`
- MINIAOD:
  `/HIPhysicsRawPrime[0-5]/HIRun2023A-PromptReco-v2/MINIAOD`

Validation status on 2026-06-14:

- EmptyBX skim exists on CERN EOS.
- ZeroBias skim exists on CERN EOS.

## Initial MC Input

TWiki-listed prompt `gamma N -> D0` signal MC:

- Skim:
  `/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root`
- Forest directory:
  `root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/wangj/prompt-GNucleusToD0-PhotonBeamA_Bin-Pthat0_Fil-Kpi_UPC_5p36TeV_pythia8-evtgen/crab_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2/260218_200449`
- MINIAOD:
  `/prompt-GNucleusToD0-PhotonBeamA_Bin-Pthat0_Fil-Kpi_UPC_5p36TeV_pythia8-evtgen/HINPbPbWinter24MiniAOD-NoPU_UPC_UPC_141X_mcRun3_2024_realistic_HI_v14-v2/MINIAODSIM`

Validation status on 2026-06-14:

- MC skim exists on CERN EOS.
- The MC skim is a reference artifact; final MC efficiency/correction work
  should reproduce or independently validate the foresting, candidate
  production, matching, and skim/reduction steps.
- Vanderbilt XRootD MC forest listing is not yet validated because `xrdfs`
  returned an auth error.

## Selection Metadata From TWiki

Jan 2024 ReReco production context:

- `CMSSW_13_2_15`
- Forest 13_2_X
- Dfinder 14XX
- D0 channel: `Dpt > 0.0`
- Track cuts listed: `Trkpt > 0.2 GeV`, `Trketa < 2.4`
- Particle-flow settings: min track pt `0.0`, max track eta `6.0`

Jan 2024 ReReco filter summary:

- Forest filters include `pPrimaryVertexFilter`, `pZDCEnergyFilter0nOr`, and HLT.
- `pZDCEnergyFilter0nOr`: `sumPlus <= 1500 || sumMinus <= 1500`
- Overview event filter: `hltZDCOr && PVfilter && pZDCEnergyFilter0nOr`
- Skim HLT flag: `isZDCOr`
- Forest D0 preselection includes:
  `Dpt > 0`, `Trkpt > 0.1 GeV`,
  `Dchi2cl > 0.05`,
  `DsvpvDistance/DsvpvErr > 0.5`, and `Dalpha < 4`.
- Skim D0 selection is labeled `pasor`; exact cut implementation should be
  checked from the linked code before final claims.

## Open Metadata To Fill In

- Reference analysis source.
- Luminosity/normalization assumptions.
- D0 candidate observable definitions.
- Expected output histograms and fit products.
- Full forest file counts after XRootD/proxy access is fixed.
- Reproducible commands/configs for forest production and the D0 skim/reducer
  step.
- Whether the final reproduction should use Jan 2024 ReReco, Feb 2025 ReReco,
  or both as a cross-check.
