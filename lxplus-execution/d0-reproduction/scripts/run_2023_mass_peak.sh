#!/usr/bin/env bash
set -euo pipefail

BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
INPUT="${D0_REPRO_2023_INPUT:-/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root}"
OUTDIR="${D0_REPRO_2023_OUTDIR:-/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/first-mass-peak-2023}"
MAX_ENTRIES=0

usage() {
  cat <<USAGE
Usage:
  run_2023_mass_peak.sh [--max-entries N] [--outdir PATH]

This uses the 2023 Jan 2024 ReReco combined D0 reference skim for the first
top-down mass-peak validation rung. It does not claim a final full-chain result.
USAGE
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --max-entries)
      MAX_ENTRIES="${2:?missing value for --max-entries}"
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

mkdir -p "$BASE/logs" "$OUTDIR"
LOG="$BASE/logs/run_2023_mass_peak_$(date -u +%Y%m%dT%H%M%SZ).log"

{
  printf 'started_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf 'host=%s\n' "$(hostname -f 2>/dev/null || hostname)"
  printf 'input=%s\n' "$INPUT"
  printf 'outdir=%s\n' "$OUTDIR"
  printf 'max_entries=%s\n' "$MAX_ENTRIES"
  root -l -b -q "$BASE/scripts/make_2023_d0_mass_peak.C(\"$INPUT\",\"$OUTDIR\",$MAX_ENTRIES)"
  printf 'finished_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} 2>&1 | tee "$LOG"

printf 'log=%s\n' "$LOG"
