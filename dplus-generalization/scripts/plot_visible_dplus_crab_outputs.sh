#!/usr/bin/env bash
set -euo pipefail

ROOT="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
# shellcheck source=/home/ubuntu/agent-network/scripts/ledger-hook.sh
source "$ROOT/scripts/ledger-hook.sh"

LXPLUSCTL="$ROOT/scripts/lxplusctl"

CRAB_OUTPUT_LABEL="dplus_medium_100files_20260622_overnight"
PLOT_LABEL=""
MIN_FILES=1
MAX_FILES=0
REMOTE_OUTPUT_BASE="/eos/user/e/evzhang/codex-dplus-crab-smoke/HIForward0DplusForestSmoke"
REMOTE_PLOT_BASE="/eos/user/e/evzhang/codex-eos/outputs/dplus-peak/plots"
REMOTE_MACRO="/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak/scripts/plot_dplus_mass.C"
REMOTE_CMSSW_SRC="/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src"
LOCAL_FIG_DIR="$ROOT/repos/general-codex-output/support/figures/dplus_peak"
LOCAL_STATUS_DIR="$ROOT/research/cms-dplus-analysis/output/monitor"

usage() {
  cat <<'USAGE'
Usage:
  plot_visible_dplus_crab_outputs.sh [options]

Options:
  --crab-output-label LABEL  CRAB output label to find under EOS.
  --plot-label LABEL         Independent plot/checkpoint label.
  --min-files N              Require at least N visible ROOT files. Default: 1.
  --max-files N              Use only the first N visible files. Default: all.
  --remote-output-base PATH  EOS base containing CRAB output dataset dirs.
  --remote-plot-base PATH    EOS base where the preliminary plot is written.
  --local-fig-dir PATH       Local figure directory for copied artifacts.

This creates a checkpoint D+ Kpipi mass plot from the CRAB outputs currently
visible on EOS. It does not submit, resubmit, cancel, or mutate CRAB jobs.
USAGE
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --crab-output-label) CRAB_OUTPUT_LABEL="${2:?missing value}"; shift 2 ;;
    --plot-label) PLOT_LABEL="${2:?missing value}"; shift 2 ;;
    --min-files) MIN_FILES="${2:?missing value}"; shift 2 ;;
    --max-files) MAX_FILES="${2:?missing value}"; shift 2 ;;
    --remote-output-base) REMOTE_OUTPUT_BASE="${2:?missing value}"; shift 2 ;;
    --remote-plot-base) REMOTE_PLOT_BASE="${2:?missing value}"; shift 2 ;;
    --local-fig-dir) LOCAL_FIG_DIR="${2:?missing value}"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) printf 'Unknown argument: %s\n' "$1" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ -z "$PLOT_LABEL" ]]; then
  PLOT_LABEL="${CRAB_OUTPUT_LABEL}_prelim_$(date -u +%Y%m%dT%H%M%SZ)"
fi

case "$CRAB_OUTPUT_LABEL" in
  *[!A-Za-z0-9._-]*|"") printf 'Unsafe CRAB output label: %s\n' "$CRAB_OUTPUT_LABEL" >&2; exit 2 ;;
esac
case "$PLOT_LABEL" in
  *[!A-Za-z0-9._-]*|"") printf 'Unsafe plot label: %s\n' "$PLOT_LABEL" >&2; exit 2 ;;
esac
case "$REMOTE_OUTPUT_BASE" in
  /eos/user/e/evzhang/*) ;;
  *) printf 'Refusing non-Evan EOS output base: %s\n' "$REMOTE_OUTPUT_BASE" >&2; exit 1 ;;
esac
case "$REMOTE_PLOT_BASE" in
  /eos/user/e/evzhang/*) ;;
  *) printf 'Refusing non-Evan EOS plot base: %s\n' "$REMOTE_PLOT_BASE" >&2; exit 1 ;;
esac

ledger_hook_start script plot-visible-dplus-crab-outputs \
  "plot_visible_dplus_crab_outputs.sh --crab-output-label $CRAB_OUTPUT_LABEL --plot-label $PLOT_LABEL" \
  "cms dplus crab plot lxplus"

mkdir -p "$LOCAL_FIG_DIR" "$LOCAL_STATUS_DIR"

MASS_LABEL="dplus_mass_${PLOT_LABEL}"
REMOTE_PLOT_DIR="$REMOTE_PLOT_BASE/$PLOT_LABEL"
REMOTE_INPUT_LIST="$REMOTE_PLOT_DIR/input_files.txt"
LOCAL_LOG="$LOCAL_STATUS_DIR/${PLOT_LABEL}.log"

remote_q() {
  printf '%q' "$1"
}

log() {
  printf '%s %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$*" | tee -a "$LOCAL_LOG"
}

log "building current visible-file list for $CRAB_OUTPUT_LABEL"

count_line="$("$LXPLUSCTL" ssh "mkdir -p $(remote_q "$REMOTE_PLOT_DIR") && find $(remote_q "$REMOTE_OUTPUT_BASE") -path \"*$(remote_q "$CRAB_OUTPUT_LABEL")*\" -name 'HiForest_*.root' -type f 2>/dev/null | sort > $(remote_q "$REMOTE_INPUT_LIST") && wc -l $(remote_q "$REMOTE_INPUT_LIST")")"
VISIBLE_FILES="$(printf '%s\n' "$count_line" | awk 'END {print $1}')"
VISIBLE_FILES="${VISIBLE_FILES:-0}"
log "visible ROOT outputs: $VISIBLE_FILES"

if (( VISIBLE_FILES < MIN_FILES )); then
  log "not enough visible outputs; required $MIN_FILES"
  exit 3
fi

USED_FILES="$VISIBLE_FILES"
if (( MAX_FILES > 0 && VISIBLE_FILES > MAX_FILES )); then
  "$LXPLUSCTL" ssh "tmp=\$(mktemp) && head -n $MAX_FILES $(remote_q "$REMOTE_INPUT_LIST") > \"\$tmp\" && mv \"\$tmp\" $(remote_q "$REMOTE_INPUT_LIST") && wc -l $(remote_q "$REMOTE_INPUT_LIST")" >> "$LOCAL_LOG"
  USED_FILES="$MAX_FILES"
  log "restricted checkpoint input list to first $USED_FILES visible files"
fi

log "running remote D+ mass macro"
"$LXPLUSCTL" ssh "cd $(remote_q "$REMOTE_CMSSW_SRC") && eval \"\$(scramv1 runtime -sh)\" && root -l -b -q '$(remote_q "$REMOTE_MACRO")(\"$REMOTE_INPUT_LIST\",\"$REMOTE_PLOT_DIR\",\"$MASS_LABEL\")'" | tee -a "$LOCAL_LOG"

log "copying preliminary artifacts locally"
scp "lxplus-codex:$REMOTE_PLOT_DIR/$MASS_LABEL.png" "$LOCAL_FIG_DIR/" >> "$LOCAL_LOG" 2>&1 || true
scp "lxplus-codex:$REMOTE_PLOT_DIR/$MASS_LABEL.pdf" "$LOCAL_FIG_DIR/" >> "$LOCAL_LOG" 2>&1 || true
scp "lxplus-codex:$REMOTE_PLOT_DIR/${MASS_LABEL}_loose_topology.png" "$LOCAL_FIG_DIR/" >> "$LOCAL_LOG" 2>&1 || true
scp "lxplus-codex:$REMOTE_PLOT_DIR/${MASS_LABEL}_loose_topology.pdf" "$LOCAL_FIG_DIR/" >> "$LOCAL_LOG" 2>&1 || true
scp "lxplus-codex:$REMOTE_PLOT_DIR/${MASS_LABEL}_topology_scan.png" "$LOCAL_FIG_DIR/" >> "$LOCAL_LOG" 2>&1 || true
scp "lxplus-codex:$REMOTE_PLOT_DIR/${MASS_LABEL}_topology_scan.pdf" "$LOCAL_FIG_DIR/" >> "$LOCAL_LOG" 2>&1 || true
scp "lxplus-codex:$REMOTE_PLOT_DIR/${MASS_LABEL}_medium_topology.png" "$LOCAL_FIG_DIR/" >> "$LOCAL_LOG" 2>&1 || true
scp "lxplus-codex:$REMOTE_PLOT_DIR/${MASS_LABEL}_medium_topology.pdf" "$LOCAL_FIG_DIR/" >> "$LOCAL_LOG" 2>&1 || true
scp "lxplus-codex:$REMOTE_PLOT_DIR/${MASS_LABEL}_tight_topology.png" "$LOCAL_FIG_DIR/" >> "$LOCAL_LOG" 2>&1 || true
scp "lxplus-codex:$REMOTE_PLOT_DIR/${MASS_LABEL}_tight_topology.pdf" "$LOCAL_FIG_DIR/" >> "$LOCAL_LOG" 2>&1 || true
scp "lxplus-codex:$REMOTE_PLOT_DIR/dplus_mass_summary.tsv" "$LOCAL_FIG_DIR/${PLOT_LABEL}_dplus_mass_summary.tsv" >> "$LOCAL_LOG" 2>&1 || true
scp "lxplus-codex:$REMOTE_PLOT_DIR/README.md" "$LOCAL_FIG_DIR/${PLOT_LABEL}_README.md" >> "$LOCAL_LOG" 2>&1 || true

{
  printf '# D+ Visible CRAB Output Checkpoint\n\n'
  printf -- '- Updated UTC: `%s`\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf -- '- CRAB output label: `%s`\n' "$CRAB_OUTPUT_LABEL"
  printf -- '- Plot label: `%s`\n' "$PLOT_LABEL"
  printf -- '- Visible ROOT files: `%s`\n' "$VISIBLE_FILES"
  printf -- '- Used ROOT files: `%s`\n' "$USED_FILES"
  printf -- '- Remote input list: `%s`\n' "$REMOTE_INPUT_LIST"
  printf -- '- Remote plot dir: `%s`\n' "$REMOTE_PLOT_DIR"
  printf -- '- Local figure dir: `%s`\n' "$LOCAL_FIG_DIR"
} > "$LOCAL_STATUS_DIR/${PLOT_LABEL}.md"

log "completed preliminary plot"
printf 'plot_label=%s\nvisible_files=%s\nused_files=%s\nremote_plot_dir=%s\nlocal_fig_dir=%s\n' \
  "$PLOT_LABEL" "$VISIBLE_FILES" "$USED_FILES" "$REMOTE_PLOT_DIR" "$LOCAL_FIG_DIR"
