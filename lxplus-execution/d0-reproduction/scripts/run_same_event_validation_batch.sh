#!/usr/bin/env bash
set -euo pipefail

BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
OUTROOT="${D0_REPRO_SAME_EVENT_OUTROOT:-/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/same-event-validation}"
PREFIX="${D0_REPRO_FOREST_PREFIX:-root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/jdlang/Run3_PbPbUPC/Forest_2023_Jan2024ReReco_2025Reforest/HIForward0/crab_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward0/260212_170406/0000}"
REFERENCE="${D0_REPRO_REFERENCE_SKIM:-/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward0_Drej-pasor_Trig-4.root}"
FILES="2 3 4"
MAX_REFERENCE_ENTRIES="${MAX_REFERENCE_ENTRIES:-80000}"
LABEL_SUFFIX="${LABEL_SUFFIX:-first80k_massspec}"

usage() {
  cat <<USAGE
Usage:
  run_same_event_validation_batch.sh [--files "2 3 4"] [--max-reference-entries N] [--label-suffix NAME]

Run same-event validation over multiple official 2023 HIForward0 forest files
against the HIForward0 D0 reference skim and write a summary CSV.
USAGE
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --files)
      FILES="${2:?missing value for --files}"
      shift 2
      ;;
    --max-reference-entries)
      MAX_REFERENCE_ENTRIES="${2:?missing value for --max-reference-entries}"
      shift 2
      ;;
    --label-suffix)
      LABEL_SUFFIX="${2:?missing value for --label-suffix}"
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

if command -v voms-proxy-info >/dev/null 2>&1; then
  timeleft="$(voms-proxy-info -timeleft 2>/dev/null || printf 0)"
  if [[ "${timeleft:-0}" -le 0 ]]; then
    printf 'No valid CMS VOMS proxy is available; refresh with voms-proxy-init -voms cms -rfc first.\n' >&2
    exit 3
  fi
fi

mkdir -p "$BASE/logs" "$BASE/manifests" "$OUTROOT"
BATCH_ID="hiforward0_batch_${LABEL_SUFFIX}_$(date -u +%Y%m%dT%H%M%SZ)"
SUMMARY="$OUTROOT/${BATCH_ID}_summary.csv"
LOG="$BASE/logs/same_event_validation_batch_${BATCH_ID}.log"
MANIFEST="$BASE/manifests/same_event_validation_batch_${BATCH_ID}.md"

printf 'file,label,status,matched_events,forest_nomass,reference_nomass,reference_dpass_dptdy,forest_stage11,reference_stage11,run_dir\n' > "$SUMMARY"

{
  printf 'started_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf 'files=%s\n' "$FILES"
  printf 'max_reference_entries=%s\n' "$MAX_REFERENCE_ENTRIES"
  printf 'reference=%s\n' "$REFERENCE"
  for file_no in $FILES; do
    forest="${PREFIX}/HiForest_2023PbPbUPC_Jan24Reco_${file_no}.root"
    label="hiforward0_forest${file_no}_vs_reference_${LABEL_SUFFIX}"
    run_dir="$OUTROOT/$label"
    printf '\n=== validating file %s ===\n' "$file_no"
    printf 'forest=%s\n' "$forest"
    status=ok
    if ! LABEL="$label" \
      D0_REPRO_FOREST_URL="$forest" \
      D0_REPRO_REFERENCE_SKIM="$REFERENCE" \
      MAX_REFERENCE_ENTRIES="$MAX_REFERENCE_ENTRIES" \
      "$BASE/scripts/run_same_event_validation.sh"; then
      status=failed
    fi

    matched_events=""
    forest_nomass=""
    reference_nomass=""
    reference_dpass=""
    forest_stage11=""
    reference_stage11=""

    if [[ -f "$run_dir/event_overlap.csv" ]]; then
      matched_events="$(awk -F, '$1=="matched_events" {print $2}' "$run_dir/event_overlap.csv")"
    fi
    if [[ -f "$run_dir/mass_spectrum_selection_counts.csv" ]]; then
      forest_nomass="$(awk -F, '$1=="forest_formula_no_mass_window" {print $2}' "$run_dir/mass_spectrum_selection_counts.csv")"
      reference_nomass="$(awk -F, '$1=="reference_formula_no_mass_window" {print $2}' "$run_dir/mass_spectrum_selection_counts.csv")"
      reference_dpass="$(awk -F, '$1=="reference_DpassCut23PAS_and_DptDy" {print $2}' "$run_dir/mass_spectrum_selection_counts.csv")"
    fi
    if [[ -f "$run_dir/stage_counts_same_events.csv" ]]; then
      forest_stage11="$(awk -F, '$1=="stage11_dtheta" {print $3}' "$run_dir/stage_counts_same_events.csv")"
      reference_stage11="$(awk -F, '$1=="stage11_dtheta" {print $4}' "$run_dir/stage_counts_same_events.csv")"
    fi
    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
      "$file_no" "$label" "$status" "$matched_events" "$forest_nomass" "$reference_nomass" \
      "$reference_dpass" "$forest_stage11" "$reference_stage11" "$run_dir" >> "$SUMMARY"
  done
  printf '\nfinished_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} 2>&1 | tee "$LOG"

{
  printf '# Same-Event Validation Batch: %s\n\n' "$BATCH_ID"
  printf '<!-- DOC_OWNER: cms-analysis-manager multi-forest validation summary. -->\n'
  printf '<!-- TOKEN_NOTE: use the CSV summary before reading per-file logs. -->\n\n'
  printf '## Inputs\n\n'
  printf -- '- Files: `%s`\n' "$FILES"
  printf -- '- Reference skim: `%s`\n' "$REFERENCE"
  printf -- '- Max reference entries per file: `%s`\n\n' "$MAX_REFERENCE_ENTRIES"
  printf '## Summary\n\n'
  printf '```csv\n'
  cat "$SUMMARY"
  printf '```\n\n'
  printf '## Durable Paths\n\n'
  printf -- '- Summary CSV: `%s`\n' "$SUMMARY"
  printf -- '- Log: `%s`\n' "$LOG"
} > "$MANIFEST"

{
  printf '%s\t%s\t%s\t%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "same-event-validation-batch" "$BATCH_ID" "$SUMMARY"
} >> "$BASE/logs/action-log.tsv"

printf 'manifest=%s\n' "$MANIFEST"
printf 'log=%s\n' "$LOG"
printf 'summary=%s\n' "$SUMMARY"
