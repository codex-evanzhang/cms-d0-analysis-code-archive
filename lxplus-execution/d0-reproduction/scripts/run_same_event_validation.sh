#!/usr/bin/env bash
set -euo pipefail

BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
CMSSW_SRC="${D0_REPRO_CMSSW_SRC:-/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src}"
OUTROOT="${D0_REPRO_SAME_EVENT_OUTROOT:-/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/same-event-validation}"

FOREST_URL="${D0_REPRO_FOREST_URL:-root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/jdlang/Run3_PbPbUPC/Forest_2023_Jan2024ReReco_2025Reforest/HIForward0/crab_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward0/260212_170406/0000/HiForest_2023PbPbUPC_Jan24Reco_1.root}"
REFERENCE_SKIM="${D0_REPRO_REFERENCE_SKIM:-/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward0_Drej-pasor_Trig-4.root}"
LABEL="${LABEL:-hiforward0_forest1_vs_reference}"
MAX_REFERENCE_ENTRIES="${MAX_REFERENCE_ENTRIES:-0}"
MAX_MISMATCH_ROWS="${MAX_MISMATCH_ROWS:-200}"

usage() {
  cat <<USAGE
Usage:
  run_same_event_validation.sh [--forest URL] [--reference FILE] [--label NAME] [--max-reference-entries N]

Compares an official 2023 PbPb UPC forest Dfinder tree to the reduced D0
reference skim using identical (run,lumi,event) keys.
USAGE
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --forest)
      FOREST_URL="${2:?missing value for --forest}"
      shift 2
      ;;
    --reference)
      REFERENCE_SKIM="${2:?missing value for --reference}"
      shift 2
      ;;
    --label)
      LABEL="${2:?missing value for --label}"
      shift 2
      ;;
    --max-reference-entries)
      MAX_REFERENCE_ENTRIES="${2:?missing value for --max-reference-entries}"
      shift 2
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

for path in "$BASE" "$OUTROOT"; do
  case "$path" in
    /afs/cern.ch/user/e/evzhang*|/afs/cern.ch/work/e/evzhang*|/eos/user/e/evzhang*)
      ;;
    *)
      printf 'Refusing to write outside Evan-owned CERN paths: %s\n' "$path" >&2
      exit 1
      ;;
  esac
done

SCRIPT="$BASE/scripts/compare_forest_reference_same_events.C"
RUN_DIR="$OUTROOT/$LABEL"
LOG="$BASE/logs/same_event_validation_${LABEL}_$(date -u +%Y%m%dT%H%M%SZ).log"
MANIFEST="$BASE/manifests/same_event_validation_${LABEL}.md"

mkdir -p "$BASE/logs" "$BASE/manifests" "$BASE/scripts" "$RUN_DIR"

source /cvmfs/cms.cern.ch/cmsset_default.sh
cd "$CMSSW_SRC"
eval "$(scram runtime -sh)"

{
  printf 'started_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf 'host=%s\n' "$(hostname -f 2>/dev/null || hostname)"
  printf 'cmssw_src=%s\n' "$CMSSW_SRC"
  printf 'macro=%s\n' "$SCRIPT"
  printf 'forest_url=%s\n' "$FOREST_URL"
  printf 'reference_skim=%s\n' "$REFERENCE_SKIM"
  printf 'output_dir=%s\n' "$RUN_DIR"
  printf 'max_reference_entries=%s\n' "$MAX_REFERENCE_ENTRIES"
  root -l -b -q "$SCRIPT+(\"$FOREST_URL\",\"$REFERENCE_SKIM\",\"$RUN_DIR\",${MAX_REFERENCE_ENTRIES},${MAX_MISMATCH_ROWS})"
  printf 'finished_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} 2>&1 | tee "$LOG"

{
  printf '# Same-Event D0 Validation: %s\n\n' "$LABEL"
  printf '<!-- DOC_OWNER: cms-analysis-manager exact forest/reference validation. -->\n'
  printf '<!-- TOKEN_NOTE: detailed stdout is in the log; CSVs in the run directory are the durable diagnostics. -->\n\n'
  printf '## Inputs\n\n'
  printf -- '- Forest: `%s`\n' "$FOREST_URL"
  printf -- '- Reference skim: `%s`\n' "$REFERENCE_SKIM"
  printf -- '- Reference entries scanned: `%s`\n\n' "$MAX_REFERENCE_ENTRIES"
  printf '## Outputs\n\n'
  printf -- '- Run directory: `%s`\n' "$RUN_DIR"
  printf -- '- Log: `%s`\n\n' "$LOG"
  if [[ -f "$RUN_DIR/event_overlap.csv" ]]; then
    printf '## Event Overlap\n\n'
    printf '```csv\n'
    cat "$RUN_DIR/event_overlap.csv"
    printf '```\n\n'
  fi
  if [[ -f "$RUN_DIR/stage_counts_same_events.csv" ]]; then
    printf '## Stage Counts\n\n'
    printf '```csv\n'
    cat "$RUN_DIR/stage_counts_same_events.csv"
    printf '```\n'
  fi
} > "$MANIFEST"

{
  printf '%s\t%s\t%s\t%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "same-event-validation" "$LABEL" "$RUN_DIR"
} >> "$BASE/logs/action-log.tsv"

printf 'manifest=%s\n' "$MANIFEST"
printf 'log=%s\n' "$LOG"
printf 'run_dir=%s\n' "$RUN_DIR"
