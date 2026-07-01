#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
LABEL="${1:-d0_an_recreation_$(date -u +%Y%m%dT%H%M%SZ)}"
OUT_BASE="$ROOT_DIR/research/cms-d0-analysis/output/an-recreation"
LOG="$OUT_BASE/${LABEL}.log"
RUNNER="$ROOT_DIR/research/cms-d0-analysis/scripts/run_d0_an_recreation_job.sh"

mkdir -p "$OUT_BASE"

setsid bash -lc '
  echo START $(date -u +%Y-%m-%dT%H:%M:%SZ)
  "$1" "$0"
  rc=$?
  echo END rc=$rc $(date -u +%Y-%m-%dT%H:%M:%SZ)
  exit $rc
' "$LABEL" "$RUNNER" >"$LOG" 2>&1 &

printf 'label=%s\n' "$LABEL"
printf 'log=%s\n' "$LOG"
printf 'pid=%s\n' "$!"
