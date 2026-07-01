#!/usr/bin/env bash
set -euo pipefail

ROOT="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
LXPLUSCTL="$ROOT/scripts/lxplusctl"

PROJECT_DIR=""
LABEL=""
EXPECTED_FILES=1
POLL_SECONDS=600
MAX_SECONDS=43200
REQUIRE_CRAB_STATUS=0
REMOTE_OUTPUT_BASE="/eos/user/e/evzhang/codex-dplus-crab-smoke/HIForward0DplusForestSmoke"
REMOTE_PLOT_BASE="/eos/user/e/evzhang/codex-eos/outputs/dplus-peak/plots"
REMOTE_MACRO="/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak/scripts/plot_dplus_mass.C"
REMOTE_CMSSW_SRC="/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src"
LOCAL_FIG_DIR="$ROOT/repos/general-codex-output/support/figures/dplus_peak"
LOCAL_STATUS_DIR="$ROOT/research/cms-dplus-analysis/output/monitor"

usage() {
  cat <<'USAGE'
Usage:
  monitor_dplus_crab_and_plot.sh --project-dir PATH --label LABEL [options]

Options:
  --expected-files N       Plot once at least N output ROOT files are visible.
  --poll-seconds N         Sleep interval between polls. Default: 600.
  --max-seconds N          Stop after this many seconds. Default: 43200.
  --require-crab-status    Exit if crab status is unavailable. Default: continue EOS polling.
  --remote-output-base P   EOS base containing CRAB output dataset dirs.
  --remote-plot-base P     EOS base where plots should be written.
  --local-fig-dir P        Local support/ figure directory for copied plots.

This is a token-saving monitor. It does not submit CRAB. It only polls an
existing task, plots visible outputs, and copies small artifacts locally.
USAGE
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --project-dir) PROJECT_DIR="${2:?missing value}"; shift 2 ;;
    --label) LABEL="${2:?missing value}"; shift 2 ;;
    --expected-files) EXPECTED_FILES="${2:?missing value}"; shift 2 ;;
    --poll-seconds) POLL_SECONDS="${2:?missing value}"; shift 2 ;;
    --max-seconds) MAX_SECONDS="${2:?missing value}"; shift 2 ;;
    --require-crab-status) REQUIRE_CRAB_STATUS=1; shift ;;
    --remote-output-base) REMOTE_OUTPUT_BASE="${2:?missing value}"; shift 2 ;;
    --remote-plot-base) REMOTE_PLOT_BASE="${2:?missing value}"; shift 2 ;;
    --local-fig-dir) LOCAL_FIG_DIR="${2:?missing value}"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) printf 'Unknown argument: %s\n' "$1" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ -z "$PROJECT_DIR" || -z "$LABEL" ]]; then
  usage >&2
  exit 2
fi

case "$PROJECT_DIR" in
  /afs/cern.ch/work/e/evzhang/*|/afs/cern.ch/user/e/evzhang/*) ;;
  *) printf 'Refusing non-Evan CRAB project dir: %s\n' "$PROJECT_DIR" >&2; exit 1 ;;
esac

case "$REMOTE_OUTPUT_BASE" in
  /eos/user/e/evzhang/*) ;;
  *) printf 'Refusing non-Evan EOS output base: %s\n' "$REMOTE_OUTPUT_BASE" >&2; exit 1 ;;
esac

mkdir -p "$LOCAL_FIG_DIR" "$LOCAL_STATUS_DIR"
RUN_ID="${LABEL}_$(date -u +%Y%m%dT%H%M%SZ)"
LOG="$LOCAL_STATUS_DIR/${RUN_ID}.log"
STATUS="$LOCAL_STATUS_DIR/${RUN_ID}.md"
PLOT_LABEL="dplus_mass_${LABEL}"
REMOTE_PLOT_DIR="$REMOTE_PLOT_BASE/$LABEL"
REMOTE_INPUT_LIST="$REMOTE_PLOT_DIR/input_files.txt"

remote_q() {
  printf '%q' "$1"
}

log() {
  printf '%s %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$*" | tee -a "$LOG"
}

write_status() {
  {
    printf '# D+ CRAB Monitor\n\n'
    printf -- '- Label: `%s`\n' "$LABEL"
    printf -- '- Project dir: `%s`\n' "$PROJECT_DIR"
    printf -- '- Expected files: `%s`\n' "$EXPECTED_FILES"
    printf -- '- Remote plot dir: `%s`\n' "$REMOTE_PLOT_DIR"
    printf -- '- Updated UTC: %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    printf -- '- State: `%s`\n' "$1"
    if [[ "${2:-}" ]]; then
      printf -- '- Detail: %s\n' "$2"
    fi
  } > "$STATUS"
}

crab_status() {
  local project_q
  project_q="$(remote_q "$PROJECT_DIR")"
  "$LXPLUSCTL" ssh "cd $(remote_q "$REMOTE_CMSSW_SRC") && eval \"\$(scramv1 runtime -sh)\" && PROXY_PATH=\$(voms-proxy-info -path 2>/dev/null || true) && test -n \"\$PROXY_PATH\" && crab status --proxy \"\$PROXY_PATH\" -d $project_q"
}

count_outputs() {
  local base_q label_q
  base_q="$(remote_q "$REMOTE_OUTPUT_BASE")"
  label_q="$(remote_q "$LABEL")"
  "$LXPLUSCTL" ssh "find $base_q -path \"*$label_q*\" -name 'HiForest_*.root' -type f 2>/dev/null | wc -l"
}

make_remote_input_list() {
  local base_q label_q input_q plot_q
  base_q="$(remote_q "$REMOTE_OUTPUT_BASE")"
  label_q="$(remote_q "$LABEL")"
  input_q="$(remote_q "$REMOTE_INPUT_LIST")"
  plot_q="$(remote_q "$REMOTE_PLOT_DIR")"
  "$LXPLUSCTL" ssh "mkdir -p $plot_q && find $base_q -path \"*$label_q*\" -name 'HiForest_*.root' -type f 2>/dev/null | sort > $input_q && wc -l $input_q"
}

run_remote_plot() {
  local input_q plot_q macro_q plot_label_q
  input_q="$(remote_q "$REMOTE_INPUT_LIST")"
  plot_q="$(remote_q "$REMOTE_PLOT_DIR")"
  macro_q="$(remote_q "$REMOTE_MACRO")"
  plot_label_q="$(remote_q "$PLOT_LABEL")"
  "$LXPLUSCTL" ssh "root -l -b -q '$macro_q(\"$REMOTE_INPUT_LIST\",\"$REMOTE_PLOT_DIR\",\"$PLOT_LABEL\")'"
}

copy_artifacts() {
  scp "lxplus-codex:$REMOTE_PLOT_DIR/$PLOT_LABEL.png" "$LOCAL_FIG_DIR/" || true
  scp "lxplus-codex:$REMOTE_PLOT_DIR/$PLOT_LABEL"_loose_topology.png "$LOCAL_FIG_DIR/" || true
  scp "lxplus-codex:$REMOTE_PLOT_DIR/$PLOT_LABEL"_topology_scan.png "$LOCAL_FIG_DIR/" || true
  scp "lxplus-codex:$REMOTE_PLOT_DIR/$PLOT_LABEL"_medium_topology.png "$LOCAL_FIG_DIR/" || true
  scp "lxplus-codex:$REMOTE_PLOT_DIR/$PLOT_LABEL"_tight_topology.png "$LOCAL_FIG_DIR/" || true
  scp "lxplus-codex:$REMOTE_PLOT_DIR/dplus_mass_summary.tsv" "$LOCAL_FIG_DIR/${LABEL}_dplus_mass_summary.tsv" || true
  scp "lxplus-codex:$REMOTE_PLOT_DIR/README.md" "$LOCAL_FIG_DIR/${LABEL}_README.md" || true
}

write_status running "monitor started"
log "monitor started for $LABEL"
start_epoch="$(date +%s)"

while true; do
  now_epoch="$(date +%s)"
  if (( now_epoch - start_epoch > MAX_SECONDS )); then
    write_status timed-out "max runtime reached before plotting"
    log "timed out"
    exit 2
  fi

  log "polling CRAB status"
  status_tmp="$(mktemp)"
  if ! crab_status > "$status_tmp" 2>&1; then
    cat "$status_tmp" >> "$LOG" || true
    rm -f "$status_tmp"
    if (( REQUIRE_CRAB_STATUS )); then
      write_status blocked "crab status failed, likely expired VOMS/proxy or CRAB service issue"
      log "crab status failed"
      exit 3
    fi
    log "crab status failed; continuing with EOS visible-output polling"
  else
    cat "$status_tmp" >> "$LOG"
    if grep -Eq 'Jobs status:[[:space:]]+failed[[:space:]]+100\.0%' "$status_tmp"; then
      rm -f "$status_tmp"
      write_status failed "CRAB reports all jobs failed before expected outputs appeared"
      log "all CRAB jobs failed"
      exit 4
    fi
    rm -f "$status_tmp"
  fi

  visible="$(count_outputs | tr -dc '0-9')"
  visible="${visible:-0}"
  log "visible ROOT outputs: $visible / $EXPECTED_FILES"
  write_status running "visible ROOT outputs: $visible / $EXPECTED_FILES"

  if (( visible >= EXPECTED_FILES )); then
    log "expected outputs visible; building input list"
    make_remote_input_list >> "$LOG" 2>&1
    log "running remote D+ plot macro"
    run_remote_plot >> "$LOG" 2>&1
    log "copying plot artifacts locally"
    copy_artifacts >> "$LOG" 2>&1
    write_status completed "plots copied to $LOCAL_FIG_DIR"
    log "completed"
    exit 0
  fi

  sleep "$POLL_SECONDS"
done
