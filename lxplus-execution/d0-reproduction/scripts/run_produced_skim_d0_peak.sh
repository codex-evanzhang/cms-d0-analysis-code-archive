#!/usr/bin/env bash
set -euo pipefail

BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
INPUT="${D0_REPRO_PRODUCED_SKIM_INPUT:-/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/hiForward0_27files_selected_first_pass/section7_d_selected.root}"
OUTDIR="${D0_REPRO_PRODUCED_SKIM_OUTDIR:-/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/produced-skim-d0-peak-2023/hiForward0_27files_selected_first_pass}"
MAX_ENTRIES=0

usage() {
  cat <<USAGE
Usage:
  run_produced_skim_d0_peak.sh [--input FILE] [--outdir PATH] [--max-entries N]

Create D0 mass-peak plots from the reproduced SelectedD skim. The skim already
contains the recreated first-pass D0 selection; this wrapper only reapplies the
standard 2<Dpt<12 and |Dy|<2 plotting region.
USAGE
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --input)
      INPUT="${2:?missing value for --input}"
      shift 2
      ;;
    --outdir)
      OUTDIR="${2:?missing value for --outdir}"
      shift 2
      ;;
    --max-entries)
      MAX_ENTRIES="${2:?missing value for --max-entries}"
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

for path in "$BASE" "$OUTDIR"; do
  case "$path" in
    /afs/cern.ch/user/e/evzhang*|/afs/cern.ch/work/e/evzhang*|/eos/user/e/evzhang*)
      ;;
    *)
      printf 'Refusing to write outside Evan-owned CERN paths: %s\n' "$path" >&2
      exit 1
      ;;
  esac
done

mkdir -p "$BASE/logs" "$BASE/manifests" "$OUTDIR"
LOG="$BASE/logs/run_produced_skim_d0_peak_$(date -u +%Y%m%dT%H%M%SZ).log"
MANIFEST="$BASE/manifests/produced_skim_d0_peak_2023.md"

{
  printf 'started_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf 'host=%s\n' "$(hostname -f 2>/dev/null || hostname)"
  printf 'input=%s\n' "$INPUT"
  printf 'outdir=%s\n' "$OUTDIR"
  printf 'max_entries=%s\n' "$MAX_ENTRIES"
  root -l -b -q "$BASE/scripts/make_produced_skim_d0_peak.C(\"$INPUT\",\"$OUTDIR\",$MAX_ENTRIES)"
  printf 'finished_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} 2>&1 | tee "$LOG"

{
  printf '# Produced-Skim D0 Peak: 2023 HIForward0\n\n'
  printf '<!-- DOC_OWNER: cms-analysis-manager produced SelectedD skim plotting. -->\n'
  printf '<!-- TOKEN_NOTE: the output manifest and CSVs contain the reusable numeric summary. -->\n\n'
  printf '## Inputs\n\n'
  printf -- '- Produced skim: `%s`\n' "$INPUT"
  printf -- '- Tree: `SelectedD`\n'
  printf -- '- Plot region: `1.70 < Dmass < 2.05`, `2 < Dpt < 12`, `abs(Dy) < 2`\n\n'
  printf '## Outputs\n\n'
  printf -- '- Output directory: `%s`\n' "$OUTDIR"
  printf -- '- Log: `%s`\n\n' "$LOG"
  if [[ -f "$OUTDIR/produced_skim_d0_peak_manifest.txt" ]]; then
    printf '## Numeric Manifest\n\n'
    printf '```text\n'
    sed -n '1,120p' "$OUTDIR/produced_skim_d0_peak_manifest.txt"
    printf '```\n\n'
  fi
  if [[ -f "$OUTDIR/pt_y_bin_counts.csv" ]]; then
    printf '## pT-y Counts\n\n'
    printf '```csv\n'
    cat "$OUTDIR/pt_y_bin_counts.csv"
    printf '```\n'
  fi
} > "$MANIFEST"

{
  printf '%s\t%s\t%s\t%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "produced-skim-d0-peak" "hiForward0_27files_selected_first_pass" "$OUTDIR"
} >> "$BASE/logs/action-log.tsv"

printf 'manifest=%s\n' "$MANIFEST"
printf 'log=%s\n' "$LOG"
printf 'outdir=%s\n' "$OUTDIR"
