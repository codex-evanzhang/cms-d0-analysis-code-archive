#!/usr/bin/env bash
set -euo pipefail

CMSSW_SRC=/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src
OUTDIR=/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/minimal-forest-smoke
DEFAULT_INPUT='root://eoscms.cern.ch//store/hidata/HIRun2026A/HIForward0/MINIAOD/PromptReco-v1/000/404/474/00000/126fe461-c885-48bb-bca8-63f64da479cb.root'

input_file=${1:-$DEFAULT_INPUT}
max_events=${2:-10}
label=${3:-smoke}
output_file=${OUTDIR}/d0_minimal_forest_${label}_${max_events}.root

source /cvmfs/cms.cern.ch/cmsset_default.sh
cd "$CMSSW_SRC"
eval "$(scram runtime -sh)"
mkdir -p "$OUTDIR"

cmsRun EvanAnalysis/D0MinimalForest/test/runD0MinimalForest_cfg.py \
  inputFiles="$input_file" \
  maxEvents="$max_events" \
  tfileServiceFile="$output_file"

python3 - "$output_file" <<'PY'
import sys
import ROOT

path = sys.argv[1]
root_file = ROOT.TFile.Open(path)
if not root_file or root_file.IsZombie():
    print(f"OPEN_FAILED {path}")
    sys.exit(2)
tree = root_file.Get("d0MinimalForest/d0minimal")
if not tree:
    print("TREE_MISSING")
    sys.exit(2)

total_tracks = 0
total_cands = 0
events_with_tracks = 0
events_with_cands = 0
for event in tree:
    n_tracks = int(event.nChargedTracksUsed)
    n_cands = int(event.nD0Candidates)
    total_tracks += n_tracks
    total_cands += n_cands
    events_with_tracks += int(n_tracks > 0)
    events_with_cands += int(n_cands > 0)

print(f"output={path}")
print(
    f"entries={tree.GetEntries()} totalTracks={total_tracks} "
    f"eventsWithTracks={events_with_tracks} totalCands={total_cands} "
    f"eventsWithCands={events_with_cands}"
)
PY
