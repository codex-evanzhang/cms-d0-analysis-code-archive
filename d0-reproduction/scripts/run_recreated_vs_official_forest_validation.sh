#!/usr/bin/env bash
set -euo pipefail

BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
SCRIPT_DIR="$BASE/scripts"
OUT_BASE="${D0_REPRO_OUT_BASE:-/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-vs-official-forest}"
RECREATED="${D0_REPRO_RECREATED_FOREST:-/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-forest-2023/smoke/hiforward0_miniaod_100_d0_zdc_pf_track_hcalsev/HiForest_2023PbPbUPC_Jan24Reco_smoke.root}"
CAMPAIGN_BASE="${D0_REPRO_OFFICIAL_CAMPAIGN_BASE:-root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/jdlang/Run3_PbPbUPC/Forest_2023_Jan2024ReReco_2025Reforest/HIForward0/crab_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward0/260212_170406}"
OFFICIAL="${D0_REPRO_OFFICIAL_FOREST:-}"
FIRST_FILE=1
LAST_FILE=1278
JOBS=1
LABEL="hiforward0_miniaod100"

usage() {
  cat <<USAGE
Usage:
  run_recreated_vs_official_forest_validation.sh [options]

Options:
  --recreated PATH       Recreated MINIAOD-derived HiForest.
  --official PATH        Official Dfinder HiForest shard. If omitted, search by event IDs.
  --campaign-base URL    Official campaign directory ending at the CRAB timestamp directory.
  --first-file N         First official numbered shard to scan when --official is omitted.
  --last-file N          Last official numbered shard to scan when --official is omitted.
  --jobs N               Parallel search chunks when --official is omitted.
  --label NAME           Output label.
  --out-base PATH        Output base directory.

Writes only under Evan-owned CERN paths. This is a bounded validation helper,
not a CRAB submission or bulk production script.
USAGE
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --recreated)
      RECREATED="${2:?missing value for --recreated}"
      shift 2
      ;;
    --official)
      OFFICIAL="${2:?missing value for --official}"
      shift 2
      ;;
    --campaign-base)
      CAMPAIGN_BASE="${2:?missing value for --campaign-base}"
      shift 2
      ;;
    --first-file)
      FIRST_FILE="${2:?missing value for --first-file}"
      shift 2
      ;;
    --last-file)
      LAST_FILE="${2:?missing value for --last-file}"
      shift 2
      ;;
    --label)
      LABEL="${2:?missing value for --label}"
      shift 2
      ;;
    --jobs)
      JOBS="${2:?missing value for --jobs}"
      shift 2
      ;;
    --out-base)
      OUT_BASE="${2:?missing value for --out-base}"
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

case "$OUT_BASE" in
  /afs/cern.ch/user/e/evzhang*|/afs/cern.ch/work/e/evzhang*|/eos/user/e/evzhang*) ;;
  *)
    printf 'Refusing to write outside Evan-owned CERN paths: %s\n' "$OUT_BASE" >&2
    exit 1
    ;;
esac

mkdir -p "$BASE/logs" "$BASE/manifests" "$OUT_BASE/$LABEL"
OUT_DIR="$OUT_BASE/$LABEL"
SEARCH_CSV="$OUT_DIR/official_match_search.csv"
LOG="$BASE/logs/recreated_vs_official_forest_${LABEL}_$(date -u +%Y%m%dT%H%M%SZ).log"
MANIFEST="$BASE/manifests/recreated_vs_official_forest_${LABEL}.md"
COMPARE_STATUS=0

{
  printf 'started_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf 'recreated=%s\n' "$RECREATED"
  printf 'out_dir=%s\n' "$OUT_DIR"

  if [[ -z "$OFFICIAL" ]]; then
    printf 'Searching official shards %s..%s with jobs=%s\n' "$FIRST_FILE" "$LAST_FILE" "$JOBS"
    if (( JOBS <= 1 )); then
      root -l -b -q "$SCRIPT_DIR/find_official_forest_events.C(\"$RECREATED\",\"$CAMPAIGN_BASE\",$FIRST_FILE,$LAST_FILE,\"$SEARCH_CSV\")"
    else
      SEARCH_DIR="$OUT_DIR/official_match_chunks"
      mkdir -p "$SEARCH_DIR"
      rm -f "$SEARCH_DIR"/matches_*.csv "$SEARCH_DIR"/matches_*.log
      total=$((LAST_FILE - FIRST_FILE + 1))
      chunk=$(((total + JOBS - 1) / JOBS))
      pids=()
      for ((job = 0; job < JOBS; job++)); do
        start=$((FIRST_FILE + job * chunk))
        end=$((start + chunk - 1))
        if (( start > LAST_FILE )); then
          continue
        fi
        if (( end > LAST_FILE )); then
          end="$LAST_FILE"
        fi
        chunk_csv="$SEARCH_DIR/matches_${start}_${end}.csv"
        chunk_log="$SEARCH_DIR/matches_${start}_${end}.log"
        printf 'launch_search_chunk=%s-%s\n' "$start" "$end"
        (
          root -l -b -q "$SCRIPT_DIR/find_official_forest_events.C(\"$RECREATED\",\"$CAMPAIGN_BASE\",$start,$end,\"$chunk_csv\")"
        ) >"$chunk_log" 2>&1 &
        pids+=("$!")
      done
      search_status=0
      for pid in "${pids[@]}"; do
        if ! wait "$pid"; then
          search_status=1
        fi
      done
      {
        printf 'file_number,file_url,matched,run_lumi_rows,matched_keys\n'
        awk 'FNR>1' "$SEARCH_DIR"/matches_*.csv 2>/dev/null || true
      } > "$SEARCH_CSV"
      printf 'parallel_search_status=%s\n' "$search_status"
      grep -h 'MATCH file=' "$SEARCH_DIR"/matches_*.log 2>/dev/null || true
      grep -h 'BEST file=' "$SEARCH_DIR"/matches_*.log 2>/dev/null || true
    fi
    OFFICIAL="$(
      awk -F, 'NR>1 && $3>best {best=$3; url=$2} END{if (url) print url}' "$SEARCH_CSV"
    )"
    if [[ -z "$OFFICIAL" ]]; then
      printf 'No official forest shard matched events in %s\n' "$RECREATED" >&2
      exit 1
    fi
  fi

  printf 'official=%s\n' "$OFFICIAL"
  set +e
  root -l -b -q "$SCRIPT_DIR/compare_two_d0_forests_same_events.C(\"$RECREATED\",\"$OFFICIAL\",\"$OUT_DIR\")"
  COMPARE_STATUS="$?"
  set -e
  printf 'compare_status=%s\n' "$COMPARE_STATUS"
  printf 'finished_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} 2>&1 | tee "$LOG"
COMPARE_STATUS="$(awk -F= '/^compare_status=/{status=$2} END{print status+0}' "$LOG")"

{
  printf '# Recreated vs Official Forest Validation: %s\n\n' "$LABEL"
  printf '<!-- DOC_OWNER: cms-analysis-manager forest validation manifest. -->\n'
  printf '<!-- TOKEN_NOTE: summary only; detailed outputs are in the EOS output directory. -->\n\n'
  printf '## Inputs\n\n'
  printf -- '- Recreated forest: `%s`\n' "$RECREATED"
  printf -- '- Official forest: `%s`\n' "$OFFICIAL"
  printf -- '- Official campaign base: `%s`\n\n' "$CAMPAIGN_BASE"
  printf '## Outputs\n\n'
  printf -- '- Output directory: `%s`\n' "$OUT_DIR"
  printf -- '- Log: `%s`\n' "$LOG"
  printf -- '- Matcher CSV: `%s`\n' "$SEARCH_CSV"
  printf -- '- Comparator manifest: `%s/manifest.md`\n\n' "$OUT_DIR"
  if [[ -f "$OUT_DIR/stage_counts.csv" ]]; then
    printf '## Stage Counts\n\n'
    printf '```csv\n'
    sed -n '1,20p' "$OUT_DIR/stage_counts.csv"
    printf '```\n'
  fi
  printf '\n## Status\n\n'
  printf -- '- Comparator exit status: `%s`\n' "$COMPARE_STATUS"
} > "$MANIFEST"

{
  printf '%s\t%s\t%s\t%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "recreated-vs-official-forest" "$LABEL" "$OUT_DIR"
} >> "$BASE/logs/action-log.tsv"

printf 'manifest=%s\n' "$MANIFEST"
printf 'log=%s\n' "$LOG"
printf 'out_dir=%s\n' "$OUT_DIR"
exit "$COMPARE_STATUS"
