# Official D0 UPC MC Model

<!-- DOC_OWNER: cms-analysis-manager official MC model summary. -->
<!-- TOKEN_NOTE: read this before querying McM/DAS or rerunning generator setup. -->

## Source of Truth

- Prompt McM request:
  `HIN-HINPbPbWinter24UPCGS-00001`.
- Nonprompt McM request:
  `HIN-HINPbPbWinter24UPCGS-00003`.
- McM API snapshots:
  `research/cms-d0-analysis/output/mcm/HIN-HINPbPbWinter24UPCGS-00001.json`
  and
  `research/cms-d0-analysis/output/mcm/HIN-HINPbPbWinter24UPCGS-00003.json`.
- Local rendered fragments:
  `work/cms-official-mc/python/PromptGNucleusToD0PhotonBeamA_5362GeV_pythia8_evtgen_cfi.py`
  and
  `work/cms-official-mc/python/NonpromptGNucleusToD0PhotonBeamA_5362GeV_pythia8_evtgen_cfi.py`.
- LXPLUS rendered fragments:
  `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/mc/official-model/python`.
- Official release:
  `CMSSW_14_1_7`, `SCRAM_ARCH=el9_amd64_gcc12`.

Useful external links:

- `https://cms-pdmv-prod.web.cern.ch/mcm/public/restapi/requests/get/HIN-HINPbPbWinter24UPCGS-00001`
- `https://cms-pdmv-prod.web.cern.ch/mcm/public/restapi/requests/get/HIN-HINPbPbWinter24UPCGS-00003`

## Prompt Signal Dataset

- Dataset:
  `/prompt-GNucleusToD0-PhotonBeamA_Bin-Pthat0_Fil-Kpi_UPC_5p36TeV_pythia8-evtgen/HINPbPbWinter24MiniAOD-NoPU_UPC_UPC_141X_mcRun3_2024_realistic_HI_v14-v2/MINIAODSIM`.
- Parent AOD:
  `/prompt-GNucleusToD0-PhotonBeamA_Bin-Pthat0_Fil-Kpi_UPC_5p36TeV_pythia8-evtgen/HINPbPbWinter24Reco-NoPU_UPC_141X_mcRun3_2024_realistic_HI_v14-v2/AODSIM`.
- McM status: `done`.
- Official event target: `50000000`.
- Official filter efficiency: `0.007 +- 0.0007`.

## Generator Definition

The official model is not generic pp charm. It is a UPC PbPb photonuclear
Pythia8+EvtGen setup:

- `comEnergy = 5362.`
- `PhotonFlux = PhotonFlux_PbPb`
- `SoftQCD:all = on`
- `PhaseSpace:pTHatMin = 0.`
- `PhotonParton:all = on`
- `MultipartonInteractions:pT0Ref = 3.0`
- `PDF:beamA2gamma = on`
- `PDF:proton2gammaSet = 0`
- `PDF:pSetB = LHAPDF6:EPPS21nlo_CT18Anlo_Pb208`
- `PDF:gammaFluxApprox2bMin = 13.272`
- `PDF:beam2gammaApprox = 2`
- `Photon:sampleQ2 = off`

EvtGen forces the D0 decay through:

- main decay table:
  `GeneratorInterface/EvtGenInterface/data/DECAY_2014_NOLONGLIFE.DEC`
- particle table:
  `GeneratorInterface/EvtGenInterface/data/evt_2014.pdl`
- user decay:
  `GeneratorInterface/ExternalDecays/data/D0_Kpi.dec`
- forced decay aliases: `myD0`, `myanti-D0`.

Filters:

- Prompt: `PythiaFilter ParticleID = 4`.
- Nonprompt: `PythiaFilter ParticleID = 5`.
- Both: `PythiaMomDauFilter ParticleID = 421`, daughters `-321, 211`,
  `MomMinPt = 0`, `-10 < eta < 10`.

## Official Production Sequence

McM sequence:

- `step = GEN,SIM`
- `eventcontent = RAWSIM`
- `datatier = GEN-SIM`
- `conditions = 141X_mcRun3_2024_realistic_HI_v14`
- `beamspot = DBrealistic`
- `scenario = HeavyIons`
- `geometry = DB:Extended`
- `era = Run3_2024_UPC`
- `nThreads = 1`

The local smoke tests use `GEN` only to validate model loading quickly. They do
not replace the official full `GEN,SIM` production chain.

## Local Validation

Helper:

```bash
scripts/cms-official-mcctl status
scripts/cms-official-mcctl smoke --flavor prompt --events 500 --step GEN
```

Prompt GEN smoke output:

- Directory:
  `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-smoke/prompt_gen_500`.
- Generated config:
  `prompt_gen_500_cfg.py`.
- Output ROOT:
  `prompt_gen_500.root`.
- cmsRun log:
  `prompt_gen_500.log`.
- Result: `6` kept events from `500` generated records.
- Observed tiny-stat filter efficiency: `6/500 = 0.012`.
- Event content includes `edm::HepMCProduct`, `GenEventInfoProduct`,
  `vector<reco::GenParticle>`, `edm::TriggerResults`, and HI gen jets.

Nonprompt initialization smoke:

- Directory:
  `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-smoke/nonprompt_gen_1`.
- Result: fragment loaded and initialized EvtGen/Pythia with the beauty filter
  (`ParticleID = 5`). One generated record was not expected to pass the very
  low official nonprompt filter efficiency.

## Detector-Level Forest Smoke

Bounded official MINIAODSIM forest checks were run through the current
CMSSW_16_1_1 foresting area with the 141X MC global tag, muon trees disabled,
and the 2023 superfilter disabled so raw detector distributions were not
preselected by the ZDC cut being studied.

BeamA:

- Dataset:
  `/prompt-GNucleusToD0-PhotonBeamA_Bin-Pthat0_Fil-Kpi_UPC_5p36TeV_pythia8-evtgen/HINPbPbWinter24MiniAOD-NoPU_UPC_UPC_141X_mcRun3_2024_realistic_HI_v14-v2/MINIAODSIM`.
- Output:
  `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-forest-smoke/prompt_miniaodsim_500_raw_detector`.

BeamB:

- Dataset:
  `/prompt-GNucleusToD0-PhotonBeamB_Bin-Pthat0_Fil-Kpi_UPC_5p36TeV_pythia8-evtgen/HINPbPbWinter24MiniAOD-NoPU_UPC_UPC_141X_mcRun3_2024_realistic_HI_v14-v2/MINIAODSIM`.
- Output:
  `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-forest-smoke/prompt_beamb_miniaodsim_500_raw_detector`.

Result:

- Both samples forested successfully for 500 events.
- Both samples have `zdcanalyzer/zdcrechit` entries, but `sumPlus = sumMinus =
  0` for every checked event. This official photonuclear MC cannot derive the
  1n ZDC threshold by itself.
- HF is filled and flips direction as expected: BeamA has lower plus-side HF
  and higher minus-side HF; BeamB flips this pattern.
- General output report synced at commit `b6faa52`; EOS output:
  `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/mc-cut-validation/official_prompt_mc_beama_beamb_500_zdc_hf`.

Reusable macros:

- `research/cms-d0-analysis/scripts/inspect_forest_detector_branches.C`
- `research/cms-d0-analysis/scripts/derive_zdc_hf_cutoffs_from_official_mc_forest.C`

## Difference from Earlier Smoke

Earlier local smoke:

- `CMSSW_16_1_1`
- Pythia8 `8.317`
- pp `HardQCD:hardccbar`
- `PhaseSpace:pTHatMin = 2.`
- D0 forced with Pythia decay switches
- no PbPb photon flux
- no nuclear PDF
- no EvtGen D0 decay card
- no prompt/nonprompt parton filter

Official model:

- `CMSSW_14_1_7`
- Pythia8 external `311-*`
- UPC PbPb photonuclear configuration
- PbPb photon flux and `EPPS21nlo_CT18Anlo_Pb208`
- EvtGen D0 Kpi forced decay
- prompt/nonprompt parton filter
- D0 daughter filter

Use the earlier smoke only to debug generator mechanics. Use the official
model for any analysis-facing MC comparison.

## Caveats

- The McM fragment contains a malformed no-op quoted line intended to bypass a
  concurrency check. The raw McM fragment is preserved verbatim under
  `work/cms-official-mc/raw`; the runnable local fragment normalizes only the
  wrapper to the standard CMS `ExternalGeneratorFilter(_generator)` form.
- The prompt `GEN` smoke validates configuration and accepted generator output,
  not detector simulation, reconstruction, foresting, or analysis weights.
- The bounded BeamA/BeamB MINIAODSIM forest smoke validates access and HF
  direction behavior, but shows ZDC response is zero in this sample.
- No CRAB task, large local production, or bulk output transfer has been
  launched.
