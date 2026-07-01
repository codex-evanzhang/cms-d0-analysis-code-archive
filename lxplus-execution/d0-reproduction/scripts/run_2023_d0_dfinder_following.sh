#!/usr/bin/env bash
set -euo pipefail

BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
CMSSW_SRC="${D0_REPRO_CMSSW_SRC:-/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src}"
OUTROOT="${D0_REPRO_DFINDER_OUTROOT:-/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/dfinder-following-2023}"
REFERENCE_DFINDER="${D0_REPRO_REFERENCE_DFINDER:-/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root}"
INPUT_URL="${D0_REPRO_INPUT_URL:-root://xrootd-vanderbilt.sites.opensciencegrid.org//store/hidata/HIRun2023A/HIForward0/MINIAOD/16Jan2024-v1/2810000/3cb20bb5-a7b1-43a2-8025-9807f4f51e71.root}"
MAX_EVENTS="${MAX_EVENTS:-1000}"
MAX_REFERENCE_ENTRIES="${MAX_REFERENCE_ENTRIES:-1000000}"
LABEL="${LABEL:-hiforward0_2023_dfinder_${MAX_EVENTS}}"
RUN_COMPARE=1

usage() {
  cat <<USAGE
Usage:
  run_2023_d0_dfinder_following.sh [--smoke] [--max-events N] [--max-reference-entries N] [--input-url URL] [--label NAME] [--outroot PATH] [--no-compare]

Runs the real Bfinder/Dfinder plugin on 2023 UPC MINIAOD with the same D0-only
settings used by the 2023 forest config, then compares the output to the
reference Dfinder skim at successive analysis-selection stages.
USAGE
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --smoke)
      MAX_EVENTS=100
      MAX_REFERENCE_ENTRIES=200000
      LABEL="smoke_100"
      shift
      ;;
    --max-events)
      MAX_EVENTS="${2:?missing value for --max-events}"
      LABEL="hiforward0_2023_dfinder_${MAX_EVENTS}"
      shift 2
      ;;
    --max-reference-entries)
      MAX_REFERENCE_ENTRIES="${2:?missing value for --max-reference-entries}"
      shift 2
      ;;
    --input-url)
      INPUT_URL="${2:?missing value for --input-url}"
      shift 2
      ;;
    --label)
      LABEL="${2:?missing value for --label}"
      shift 2
      ;;
    --outroot)
      OUTROOT="${2:?missing value for --outroot}"
      shift 2
      ;;
    --no-compare)
      RUN_COMPARE=0
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      printf 'Unknown argument: %s\n' "$1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

for path in "$OUTROOT" "$BASE"; do
  case "$path" in
    /afs/cern.ch/user/e/evzhang*|/afs/cern.ch/work/e/evzhang*|/eos/user/e/evzhang*)
      ;;
    *)
      printf 'Refusing to write outside Evan-owned CERN paths: %s\n' "$path" >&2
      exit 1
      ;;
  esac
done

CFG="$BASE/scripts/run_2023_d0_dfinder_only_cfg.py"
COMPARE_MACRO="$BASE/scripts/compare_2023_d0_dfinder_following.C"
RUN_DIR="$OUTROOT/$LABEL"
DFINDER_OUTPUT="$RUN_DIR/d0_dfinder_only.root"
STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
LOCAL_OUTPUT="$BASE/scratch/${LABEL}_${STAMP}_d0_dfinder_only.root"
LOG="$BASE/logs/run_2023_d0_dfinder_following_${LABEL}_${STAMP}.log"
MANIFEST="$BASE/manifests/dfinder_following_2023_${LABEL}.md"

mkdir -p "$BASE/logs" "$BASE/manifests" "$BASE/scripts" "$BASE/scratch" "$RUN_DIR"

source /cvmfs/cms.cern.ch/cmsset_default.sh
cd "$CMSSW_SRC"
eval "$(scram runtime -sh)"

{
  printf 'started_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf 'host=%s\n' "$(hostname -f 2>/dev/null || hostname)"
  printf 'cmssw_src=%s\n' "$CMSSW_SRC"
  printf 'config=%s\n' "$CFG"
  printf 'input_url=%s\n' "$INPUT_URL"
  printf 'max_events=%s\n' "$MAX_EVENTS"
  printf 'max_reference_entries=%s\n' "$MAX_REFERENCE_ENTRIES"
  printf 'dfinder_output=%s\n' "$DFINDER_OUTPUT"
  printf 'local_output=%s\n' "$LOCAL_OUTPUT"
  cmsRun "$CFG" inputFiles="$INPUT_URL" maxEvents="$MAX_EVENTS" outputFile="$LOCAL_OUTPUT"
  ACTUAL_LOCAL_OUTPUT="$LOCAL_OUTPUT"
  if [[ ! -f "$ACTUAL_LOCAL_OUTPUT" ]]; then
    GENERATED_OUTPUT="${LOCAL_OUTPUT%.root}_numEvent${MAX_EVENTS}.root"
    if [[ -f "$GENERATED_OUTPUT" ]]; then
      ACTUAL_LOCAL_OUTPUT="$GENERATED_OUTPUT"
      printf 'cmssw_output_name=%s\n' "$ACTUAL_LOCAL_OUTPUT"
    else
      printf 'ERROR: expected local Dfinder output not found: %s\n' "$LOCAL_OUTPUT" >&2
      exit 3
    fi
  fi
  cp -f "$ACTUAL_LOCAL_OUTPUT" "$DFINDER_OUTPUT"
  printf 'copied_output=%s -> %s\n' "$ACTUAL_LOCAL_OUTPUT" "$DFINDER_OUTPUT"
  if [[ "$RUN_COMPARE" -eq 1 ]]; then
    root -l -b -q "$COMPARE_MACRO(\"$DFINDER_OUTPUT\",\"$REFERENCE_DFINDER\",\"$RUN_DIR\",\"$INPUT_URL\",${MAX_REFERENCE_ENTRIES})"
  fi
  printf 'finished_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} 2>&1 | tee "$LOG"

{
  printf '# 2023 Dfinder-Following D0 Run: %s\n\n' "$LABEL"
  printf '<!-- DOC_OWNER: cms-analysis-manager Dfinder-following reproduction manifest. -->\n'
  printf '<!-- TOKEN_NOTE: detailed stdout is in logs; plots and cutflow are in the run directory. -->\n\n'
  printf '## Inputs\n\n'
  printf -- '- MINIAOD input: `%s`\n' "$INPUT_URL"
  printf -- '- Max events: `%s`\n' "$MAX_EVENTS"
  printf -- '- Reference entries scanned per comparison stage: `%s`\n' "$MAX_REFERENCE_ENTRIES"
  printf -- '- Reference Dfinder skim: `%s`\n\n' "$REFERENCE_DFINDER"
  printf '## Dfinder Configuration\n\n'
  printf -- '- CMSSW source: `%s`\n' "$CMSSW_SRC"
  printf -- '- Config: `%s`\n' "$CFG"
  printf -- '- Track label: `packedPFCandidates`\n'
  printf -- '- Track chi2 label: `packedPFCandidateTrackChi2`\n'
  printf -- '- Vertex label: `offlineSlimmedPrimaryVertices`\n'
  printf -- '- Beam spot label: `offlineBeamSpot`\n'
  printf -- '- D channels: `K+pi-` and `K-pi+` only\n'
  printf -- '- Dfinder preselection: track pT > 0.2 GeV, |eta| < 2.4, VtxChiProb > 0.05, svpvDistance/svpvDisErr > 0.5, alpha < 4\n\n'
  printf '## Outputs\n\n'
  printf -- '- Dfinder output: `%s`\n' "$DFINDER_OUTPUT"
  printf -- '- Local scratch output: `%s`\n' "$LOCAL_OUTPUT"
  printf -- '- Run directory: `%s`\n' "$RUN_DIR"
  printf -- '- Run log: `%s`\n\n' "$LOG"
  if [[ -f "$RUN_DIR/dfinder_following_manifest.md" ]]; then
    printf '## Comparison Summary\n\n'
    sed -n '/## Counts/,$p' "$RUN_DIR/dfinder_following_manifest.md"
  fi
} > "$MANIFEST"

{
  printf '%s\t%s\t%s\t%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "dfinder-following" "$LABEL" "$DFINDER_OUTPUT"
} >> "$BASE/logs/action-log.tsv"

printf 'manifest=%s\n' "$MANIFEST"
printf 'log=%s\n' "$LOG"
printf 'run_dir=%s\n' "$RUN_DIR"
printf 'dfinder_output=%s\n' "$DFINDER_OUTPUT"
