# D0MinimalForest

Minimal CMSSW analyzer for testing a from-MINIAOD D0 analysis path in the
modern `CMSSW_16_1_1` foresting area.

## Scope

The analyzer consumes `packedPFCandidates` and
`offlineSlimmedPrimaryVertices` from MINIAOD. It writes a compact ROOT tree
with event identifiers, vertex information, a packed-candidate HF energy-gap
proxy, and opposite-charge Kpi/piK invariant-mass candidates.

This is not a replacement for Dfinder yet. It deliberately does not claim a
secondary-vertex fit, decay-length significance, pointing angle, or kaon/pion
PID. Generic CMS packed candidates do not provide reliable K/pi identification
for this analysis; both mass hypotheses are therefore saved for each
opposite-charge pair that passes the configured mass window.

## Output Tree

The TFileService tree is `d0MinimalForest/d0minimal`.

Important branches:

- `run`, `lumi`, `event`
- `nPacked`, `nChargedTracksUsed`, `nOppositeChargePairs`, `nD0Candidates`
- `nVtx`, `bestVx`, `bestVy`, `bestVz`
- `hfEMaxPlusPacked`, `hfEMaxMinusPacked`
- `dMass`, `dPt`, `dEta`, `dPhi`, `dY`
- `candHypothesis`: `0` means track 1 is assigned kaon mass; `1` means track
  2 is assigned kaon mass
- daughter branches `trk{1,2}Pt`, `trk{1,2}Eta`, `trk{1,2}Phi`,
  `trk{1,2}Charge`, `trk{1,2}PdgId`, `trk{1,2}HighPurity`,
  `trk{1,2}HasTrackDetails`, `trk{1,2}PtError`

## Smoke Test

From the CMSSW source directory:

```bash
cd /afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src
cmsenv
scram b -j4
mkdir -p /eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/minimal-forest-smoke
cmsRun EvanAnalysis/D0MinimalForest/test/runD0MinimalForest_cfg.py \
  inputFiles=root://eoscms.cern.ch//store/hidata/HIRun2026A/HIForward0/MINIAOD/PromptReco-v1/000/404/474/00000/126fe461-c885-48bb-bca8-63f64da479cb.root \
  maxEvents=10 \
  tfileServiceFile=/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/minimal-forest-smoke/d0_minimal_forest_2026_10.root
```

## Next Validation Steps

1. Run on a small 2023 MINIAOD sample once an input file is selected.
2. Compare `dMass` spectra against the Dfinder skim-level mass peak.
3. Add trigger/filter bits needed for Section 5 event selection.
4. Add ZDC rechit/digi branches by reusing the validated Run 3 ZDC modules.
5. Add a vertex-fit/topology stage or explicitly delegate that part to
   Dfinder until an independent implementation is validated.
