#!/usr/bin/env bash
set -euo pipefail

ROOT="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
LXPLUSCTL="$ROOT/scripts/lxplusctl"
MONITOR="$ROOT/research/cms-dplus-analysis/scripts/monitor_dplus_crab_and_plot.sh"

SMOKE_PROJECT_DIR=""
SMOKE_LABEL=""
MEDIUM_LABEL="dplus_medium_100files_$(date -u +%Y%m%d_%H%M%S)"
INPUT_LIST="/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/manifests/d0_minimal_raw_100files_hiforward0_20260618_inputs.txt"
EXPECTED_MEDIUM_FILES=100
POLL_SECONDS=600
MAX_SECONDS=43200
REMOTE_RUNNER="/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak/scripts/run_2023_dplus_forest_crab_smoke.sh"
LOCAL_STATUS_DIR="$ROOT/research/cms-dplus-analysis/output/monitor"

usage() {
  cat <<'USAGE'
Usage:
  launch_dplus_medium_after_smoke.sh --smoke-project-dir PATH --smoke-label LABEL [options]

Waits for an already-submitted one-file D+ CRAB smoke to produce output. If it
does, submits the 100-file D+ medium CRAB run and monitors/plots the output.
If the smoke monitor fails, medium submission is not attempted.
USAGE
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --smoke-project-dir) SMOKE_PROJECT_DIR="${2:?missing value}"; shift 2 ;;
    --smoke-label) SMOKE_LABEL="${2:?missing value}"; shift 2 ;;
    --medium-label) MEDIUM_LABEL="${2:?missing value}"; shift 2 ;;
    --input-list) INPUT_LIST="${2:?missing value}"; shift 2 ;;
    --expected-medium-files) EXPECTED_MEDIUM_FILES="${2:?missing value}"; shift 2 ;;
    --poll-seconds) POLL_SECONDS="${2:?missing value}"; shift 2 ;;
    --max-seconds) MAX_SECONDS="${2:?missing value}"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) printf 'Unknown argument: %s\n' "$1" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ -z "$SMOKE_PROJECT_DIR" || -z "$SMOKE_LABEL" ]]; then
  usage >&2
  exit 2
fi

mkdir -p "$LOCAL_STATUS_DIR"
CHAIN_ID="${MEDIUM_LABEL}_chain_$(date -u +%Y%m%dT%H%M%SZ)"
CHAIN_LOG="$LOCAL_STATUS_DIR/${CHAIN_ID}.log"
CHAIN_STATUS="$LOCAL_STATUS_DIR/${CHAIN_ID}.md"

log() {
  printf '%s %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$*" | tee -a "$CHAIN_LOG"
}

write_status() {
  {
    printf '# D+ Medium Chain\n\n'
    printf -- '- Smoke label: `%s`\n' "$SMOKE_LABEL"
    printf -- '- Medium label: `%s`\n' "$MEDIUM_LABEL"
    printf -- '- Input list: `%s`\n' "$INPUT_LIST"
    printf -- '- Updated UTC: %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    printf -- '- State: `%s`\n' "$1"
    if [[ "${2:-}" ]]; then
      printf -- '- Detail: %s\n' "$2"
    fi
  } > "$CHAIN_STATUS"
}

remote_q() {
  printf '%q' "$1"
}

write_status running "waiting for smoke output"
log "waiting for smoke output"
"$MONITOR" \
  --project-dir "$SMOKE_PROJECT_DIR" \
  --label "$SMOKE_LABEL" \
  --expected-files 1 \
  --poll-seconds "$POLL_SECONDS" \
  --max-seconds "$MAX_SECONDS" >> "$CHAIN_LOG" 2>&1

write_status running "smoke output found; submitting medium run"
log "smoke output found; submitting medium run $MEDIUM_LABEL"
SUBMIT_LOG="$LOCAL_STATUS_DIR/${MEDIUM_LABEL}_submit.log"
input_q="$(remote_q "$INPUT_LIST")"
runner_q="$(remote_q "$REMOTE_RUNNER")"
medium_q="$(remote_q "$MEDIUM_LABEL")"
"$LXPLUSCTL" ssh "$runner_q --label $medium_q --minimal-analysis --input-list $input_q" | tee "$SUBMIT_LOG" >> "$CHAIN_LOG"

MEDIUM_PROJECT_DIR="$(awk -F': ' '/Project dir:/ {print $2}' "$SUBMIT_LOG" | tail -n 1)"
if [[ -z "$MEDIUM_PROJECT_DIR" ]]; then
  MEDIUM_PROJECT_DIR="/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak/crab-smoke/crab-projects/crab_${MEDIUM_LABEL}"
fi

write_status running "medium submitted: $MEDIUM_PROJECT_DIR"
log "medium project dir: $MEDIUM_PROJECT_DIR"
"$MONITOR" \
  --project-dir "$MEDIUM_PROJECT_DIR" \
  --label "$MEDIUM_LABEL" \
  --expected-files "$EXPECTED_MEDIUM_FILES" \
  --poll-seconds "$POLL_SECONDS" \
  --max-seconds "$MAX_SECONDS" >> "$CHAIN_LOG" 2>&1

write_status completed "medium plots copied by monitor"
log "completed"
