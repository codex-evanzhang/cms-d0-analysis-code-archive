#!/usr/bin/env bash
set -euo pipefail

BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
DATASET_DIR="${D0_REPRO_HIFORWARD0_FOREST_DIR:-/eos/user/e/evzhang/codex-eos/datasets/vanderbilt_HIForward0_251227_162520_0000_0006}"
OUTDIR="${D0_REPRO_SELECTED_SKIM_OUTDIR:-/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023}"
INPUT_LIST="$BASE/manifests/hiForward0_local_forest_files.txt"
MAX_EVENTS=-1
MAX_FILES=-1
MAX_CANDIDATES=-1
LABEL="hiForward0_selected_first_pass"

usage() {
  cat <<USAGE
Usage:
  run_2023_d0_selected_skim.sh [--smoke] [--max-events N] [--max-files N] [--max-candidates N] [--label NAME] [--outdir PATH]

Recreates the analysis-shaped selected D0 skim from current-schema forests:
  1. Section 5 event selection from forest trees.
  2. Section 7 D-candidate selection from the recreated Section 5 output.

This is the practical skim-reproduction path for analysis validation. The older
event-vector micro-skim prototype remains documented but is too slow for full
sample production without further optimization.
USAGE
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --smoke)
      MAX_EVENTS=50000
      MAX_FILES=1
      MAX_CANDIDATES=-1
      LABEL="smoke_batch001_50k"
      shift
      ;;
    --max-events)
      MAX_EVENTS="${2:?missing value for --max-events}"
      shift 2
      ;;
    --max-files)
      MAX_FILES="${2:?missing value for --max-files}"
      shift 2
      ;;
    --max-candidates)
      MAX_CANDIDATES="${2:?missing value for --max-candidates}"
      shift 2
      ;;
    --label)
      LABEL="${2:?missing value for --label}"
      shift 2
      ;;
    --outdir)
      OUTDIR="${2:?missing value for --outdir}"
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

case "$OUTDIR" in
  /afs/cern.ch/user/e/evzhang*|/afs/cern.ch/work/e/evzhang*|/eos/user/e/evzhang*)
    ;;
  *)
    printf 'Refusing to write outside Evan-owned CERN paths: %s\n' "$OUTDIR" >&2
    exit 1
    ;;
esac

RUN_DIR="$OUTDIR/$LABEL"
PLOT_DIR="$RUN_DIR/plots"
SECTION5_ROOT="$RUN_DIR/section5_event_selected.root"
SECTION7_ROOT="$RUN_DIR/section7_d_selected.root"
LOG="$BASE/logs/run_2023_d0_selected_skim_${LABEL}_$(date -u +%Y%m%dT%H%M%SZ).log"
MANIFEST="$BASE/manifests/recreated_selected_skim_2023_${LABEL}.md"

mkdir -p "$BASE/logs" "$BASE/manifests" "$BASE/scripts" "$RUN_DIR" "$PLOT_DIR"
find "$DATASET_DIR" -maxdepth 1 -type f -name '*.root' | sort > "$INPUT_LIST"

SECTION5_MACRO="$BASE/scripts/ApplySection5EventSelection.C"
SECTION7_MACRO="$BASE/scripts/ApplySection7DSelection.C"
if [[ ! -f "$SECTION5_MACRO" ]]; then
  cp /afs/cern.ch/user/e/evzhang/RootAnalysis/MITanalysisTest/ApplySection5EventSelection.C "$SECTION5_MACRO"
fi
if [[ ! -f "$SECTION7_MACRO" ]]; then
  cp /afs/cern.ch/user/e/evzhang/RootAnalysis/MITanalysisTest/ApplySection7DSelection.C "$SECTION7_MACRO"
fi

{
  printf 'started_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf 'host=%s\n' "$(hostname -f 2>/dev/null || hostname)"
  printf 'dataset_dir=%s\n' "$DATASET_DIR"
  printf 'input_list=%s\n' "$INPUT_LIST"
  printf 'input_file_count=%s\n' "$(wc -l < "$INPUT_LIST")"
  printf 'max_files=%s\n' "$MAX_FILES"
  printf 'max_events=%s\n' "$MAX_EVENTS"
  printf 'max_candidates=%s\n' "$MAX_CANDIDATES"
  printf 'section5_root=%s\n' "$SECTION5_ROOT"
  printf 'section7_root=%s\n' "$SECTION7_ROOT"
  printf 'plot_dir=%s\n' "$PLOT_DIR"
  root -l -b -q "$SECTION5_MACRO(\"$INPUT_LIST\",\"$SECTION5_ROOT\",\"$PLOT_DIR\",$MAX_FILES,$MAX_EVENTS)"
  root -l -b -q "$SECTION7_MACRO(\"$SECTION5_ROOT\",\"$SECTION7_ROOT\",\"$PLOT_DIR\",$MAX_CANDIDATES)"
  printf 'finished_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} 2>&1 | tee "$LOG"

{
  printf '# Recreated 2023 D0 Selected Skim: %s\n\n' "$LABEL"
  printf '<!-- DOC_OWNER: cms-analysis-manager selected-skim reproduction manifest. -->\n'
  printf '<!-- TOKEN_NOTE: concise status; detailed command output is in logs/. -->\n\n'
  printf '## Inputs\n\n'
  printf -- '- Forest source: `%s`\n' "$DATASET_DIR"
  printf -- '- Input list: `%s`\n' "$INPUT_LIST"
  printf -- '- Input file count in list: `%s`\n' "$(wc -l < "$INPUT_LIST")"
  printf -- '- max_files argument: `%s`\n' "$MAX_FILES"
  printf -- '- max_events argument: `%s`\n' "$MAX_EVENTS"
  printf -- '- max_candidates argument: `%s`\n\n' "$MAX_CANDIDATES"
  printf '## Scripts\n\n'
  printf -- '- Section 5 macro copy: `%s`\n' "$SECTION5_MACRO"
  printf -- '- Section 7 macro copy: `%s`\n\n' "$SECTION7_MACRO"
  printf '## Outputs\n\n'
  printf -- '- Section 5 recreated event/candidate skim: `%s`\n' "$SECTION5_ROOT"
  printf -- '- Section 7 recreated selected D skim: `%s`\n' "$SECTION7_ROOT"
  printf -- '- Plots: `%s`\n' "$PLOT_DIR"
  printf -- '- Run log: `%s`\n\n' "$LOG"
  printf '## Scope\n\n'
  printf -- '- This recreates the analysis-shaped selected skim from forests using documented Section 5 event cuts and Section 7 D-candidate cuts.\n'
  printf -- '- It is not yet a full branch-for-branch clone of the Wang `Tree` reference skim.\n'
  printf -- '- Existing reference skims remain validation references only.\n\n'
  printf '## Extracted Run Summary\n\n'
  printf '```text\n'
  grep -E 'Section 5 summary|Section 7 summary|Wrote ' "$LOG" || true
  printf '```\n'
} > "$MANIFEST"

{
  printf '%s\t%s\t%s\t%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "recreate-selected-skim" "$LABEL" "$SECTION7_ROOT"
} >> "$BASE/logs/action-log.tsv"

printf 'manifest=%s\n' "$MANIFEST"
printf 'log=%s\n' "$LOG"
printf 'section5_root=%s\n' "$SECTION5_ROOT"
printf 'section7_root=%s\n' "$SECTION7_ROOT"
