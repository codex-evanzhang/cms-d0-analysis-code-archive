#!/usr/bin/env bash
set -euo pipefail

CMSSW_SRC=/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src
SCRIPT_DIR=/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts
OUTROOT=/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/minimal-forest-2023-compare
REFERENCE_DFINDER=/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root

DATASET=${DATASET:-/HIForward0/HIRun2023A-16Jan2024-v1/MINIAOD}
MAX_EVENTS=${MAX_EVENTS:-1000}
MAX_REFERENCE_ENTRIES=${MAX_REFERENCE_ENTRIES:-0}
LABEL=${LABEL:-hiforward0_2023_miniaod_${MAX_EVENTS}}
INPUT_LFN=${INPUT_LFN:-}
INPUT_URL=${INPUT_URL:-}

source /cvmfs/cms.cern.ch/cmsset_default.sh

if [[ -z "$INPUT_URL" ]]; then
  if ! voms-proxy-info -exists -valid 0:10 >/dev/null 2>&1; then
    echo "ERROR: no valid CMS VOMS proxy. Set INPUT_URL directly or run the one-shot secure workflow 'CERN CMS proxy' first for DAS lookup." >&2
    exit 2
  fi
  if [[ -z "$INPUT_LFN" ]]; then
    INPUT_LFN=$(dasgoclient --query="file dataset=${DATASET}" | sed -n '1p')
  fi
  if [[ -z "$INPUT_LFN" ]]; then
    echo "ERROR: DAS returned no files for ${DATASET}" >&2
    exit 2
  fi
  INPUT_URL="root://eoscms.cern.ch/${INPUT_LFN}"
fi

OUTDIR="${OUTROOT}/${LABEL}"
MINIMAL_OUTPUT="${OUTDIR}/d0_minimal_forest.root"
mkdir -p "$OUTDIR"

cd "$CMSSW_SRC"
eval "$(scram runtime -sh)"

cmsRun EvanAnalysis/D0MinimalForest/test/runD0MinimalForest_cfg.py \
  inputFiles="$INPUT_URL" \
  maxEvents="$MAX_EVENTS" \
  tfileServiceFile="$MINIMAL_OUTPUT" \
  trackPtMin=0.2 \
  trackEtaMax=2.4 \
  requireHighPurity=False \
  candidateMassMin=1.5 \
  candidateMassMax=2.3 \
  maxCandidates=200000

root -l -b -q "${SCRIPT_DIR}/compare_d0_minimal_to_dfinder.C+(\"${MINIMAL_OUTPUT}\",\"${REFERENCE_DFINDER}\",\"${OUTDIR}\",\"${INPUT_URL}\",${MAX_REFERENCE_ENTRIES})"

cat > "${OUTDIR}/run_manifest.md" <<MANIFEST
# 2023 Minimal Forest Run

- Dataset: \`${DATASET}\`
- Input LFN: \`${INPUT_LFN:-not-set-explicit-url-used}\`
- Input URL: \`${INPUT_URL}\`
- Max events: \`${MAX_EVENTS}\`
- Minimal forest output: \`${MINIMAL_OUTPUT}\`
- Comparison output directory: \`${OUTDIR}\`
- Dfinder reference: \`${REFERENCE_DFINDER}\`

MANIFEST

echo "$OUTDIR"
