# Pythia MC Setup for the D0 Analysis

<!-- DOC_OWNER: cms-analysis-manager Pythia/MC generation notes. -->
<!-- TOKEN_NOTE: use this before rereading generator twikis or CMSSW fragments. -->

## Current Working Setup

- CMSSW area: `/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src`.
- Pythia from CMSSW external tools: Pythia8 `8.317`, path under `/cvmfs/cms.cern.ch/el9_amd64_gcc13/external/pythia8/317-*`.
- LHAPDF from CMSSW external tools: `6.4.0`.
- Local package created in CMSSW:
  `/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src/EvanAnalysis/D0PythiaMC`.
- Durable scripts/templates:
  `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_pythia_d0_smoke.sh`
  and
  `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/mc/pythia/`.

## Official Model Status

The analysis-facing MC model has now been matched to the CMS McM requests.
Use `MC_OFFICIAL_MODEL.md` for the compact official definition.

- Prompt request: `HIN-HINPbPbWinter24UPCGS-00001`.
- Nonprompt request: `HIN-HINPbPbWinter24UPCGS-00003`.
- Official release: `CMSSW_14_1_7`, `SCRAM_ARCH=el9_amd64_gcc12`.
- Local helper: `scripts/cms-official-mcctl`.
- Local fragments: `work/cms-official-mc/python/`.
- LXPLUS fragments:
  `/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/mc/official-model/python`.
- Prompt 500-event `GEN` smoke completed with 6 kept events:
  `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/official-mc-smoke/prompt_gen_500`.

The older `CMSSW_16_1_1` Pythia smoke remains useful only for mechanics.
It is not the official UPC PbPb MC model.

## What Works Now

Run:

```bash
/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/run_pythia_d0_smoke.sh \
  --standalone-events 100 \
  --cmssw-events 10 \
  --outdir /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/pythia-d0-smoke/cmssw_gen_smoke_100_10
```

Outputs:

- Standalone Pythia summary:
  `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/pythia-d0-smoke/cmssw_gen_smoke_100_10/standalone_pythia_d0_summary.csv`.
- CMSSW generated config:
  `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/pythia-d0-smoke/cmssw_gen_smoke_100_10/d0_pythia_gen_cfg.py`.
- CMSSW GEN ROOT:
  `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/pythia-d0-smoke/cmssw_gen_smoke_100_10/d0_pythia_gen.root`.
- Event content dump:
  `/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/pythia-d0-smoke/cmssw_gen_smoke_100_10/d0_pythia_gen_event_content.txt`.

Smoke result from the successful run:

```text
standalone events: 100 accepted
D0 or anti-D0 total: 113
D0/anti-D0 -> K pi: 113
D0/anti-D0 with 2<pT<12 and |y|<2: 38
CMSSW GEN events: 10
CMSSW output size: 526011 bytes
```

The CMSSW output contains `GenEventInfoProduct`, `HepMCProduct`,
`reco::GenParticle`, gen jets, and gen MET products.

## Generator Fragment

Current fragment:

```python
HardQCD:hardccbar = on
PhaseSpace:pTHatMin = 2.
421:onMode = off
421:onIfMatch = -321 211
421:onIfMatch = 321 -211
```

This is a proton-proton charm smoke sample at `sqrt(s)=5.362 TeV`, using CP5
settings through CMSSW. It forces D0/anti-D0 to K pi. This is not yet the final
UPC PbPb signal model.

## Interpretation

- The current setup proves that Pythia8 works in both standalone and CMSSW/EDM
  modes on LXPLUS.
- It gives a fast generator-level source for debugging D0 kinematics, daughter
  decay handling, and future Dfinder/forest matching.
- It is not yet a validated physics MC sample for the D0 UPC measurement.

## Still Needed for Analysis-Grade MC

1. Add MC truth branches needed by the D0 analysis: prompt/nonprompt label,
   generated D0 kinematics, daughter matching, event weights, pThat, and
   cross-section metadata.
2. Validate the official prompt MC against the existing official MINIAOD/forest
   sample at generator truth and Dfinder input levels.
3. Run GEN -> SIM -> DIGI/RECO/MINIAOD only after the generator-level fragment
   is validated and Evan approves larger production.
4. Extend the same McM-matching workflow to D+, Ds+, or D* only after the D0
   prompt/nonprompt setup is stable.

## Useful References

- CMS WorkBook generator intro:
  `https://twiki.cern.ch/twiki/bin/view/CMSPublic/WorkBookGenIntro`
- CMS WorkBook generation page:
  `https://twiki.cern.ch/twiki/bin/view/CMSPublic/WorkBookGeneration`
- CMS cmsDriver guide:
  `https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideCmsDriver`
- Pythia8 manual:
  `https://pythia.org/latest-manual/Welcome.html`
- CMS generator fragments moved from GitHub to CERN GitLab in 2025:
  `https://github.com/cms-sw/genproductions`
