#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
# shellcheck source=/home/ubuntu/agent-network/scripts/ledger-hook.sh
source "$ROOT_DIR/scripts/ledger-hook.sh"

SCRIPT_PATH="$(realpath "$0")"
WORKFLOW_BASE="$ROOT_DIR/research/cms-d0-analysis/output/an-recreation-workflows"

if [[ "${1:-}" == "--detached" ]]; then
  shift
  LABEL="${1:-d0_an_full_$(date -u +%Y%m%dT%H%M%SZ)}"
  LOG_DIR="$WORKFLOW_BASE/$LABEL"
  mkdir -p "$LOG_DIR"
  setsid bash "$SCRIPT_PATH" "$LABEL" >"$LOG_DIR/workflow.log" 2>&1 &
  printf 'label=%s\n' "$LABEL"
  printf 'log=%s\n' "$LOG_DIR/workflow.log"
  printf 'pid=%s\n' "$!"
  exit 0
fi

ledger_hook_start script cms-d0-full-an-recreation "run_full_d0_an_recreation_workflow.sh" "cms d0 an overleaf lxplus root"

LABEL="${1:-d0_an_full_$(date -u +%Y%m%dT%H%M%SZ)}"
LOG_DIR="$WORKFLOW_BASE/$LABEL"
SUMMARY="$LOG_DIR/summary.txt"
mkdir -p "$LOG_DIR"

LX="$ROOT_DIR/scripts/lxplusctl"
PROXY_RUNNER="$ROOT_DIR/research/cms-d0-analysis/scripts/run_recreated_an_proxy_plots.sh"
MC_PROXY_RUNNER="$ROOT_DIR/research/cms-d0-analysis/scripts/run_missing_an_mc_proxy_plots.sh"
OFFICIAL_PROXY_RUNNER="$ROOT_DIR/research/cms-d0-analysis/scripts/run_missing_an_official_proxy_plots.sh"
AN_RUNNER="$ROOT_DIR/research/cms-d0-analysis/scripts/run_d0_an_recreation_job.sh"
PROMOTER="$ROOT_DIR/research/cms-d0-analysis/scripts/promote_an_recreation_to_general_output.sh"

log_step() {
  printf '[%s] %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$*" | tee -a "$SUMMARY"
}

run_logged() {
  local name="$1"
  shift
  log_step "START $name"
  "$@" >"$LOG_DIR/$name.log" 2>&1
  log_step "END $name"
}

log_step "workflow_label=$LABEL"
run_logged lxplus_test "$LX" test
run_logged voms_test "$LX" ssh "source /cvmfs/cms.cern.ch/cmsset_default.sh; voms-proxy-info -exists -valid 0:10; printf 'timeleft='; voms-proxy-info -timeleft"

PROXY_LABEL="an_proxy_plots_${LABEL}"
MC_PROXY_LABEL="missing_an_mc_proxy_${LABEL}"
OFFICIAL_PROXY_LABEL="missing_an_official_proxy_${LABEL}"
AN_LABEL="d0_an_recreation_${LABEL}"

run_logged proxy_plots "$PROXY_RUNNER" "$PROXY_LABEL"
if [[ "${SKIP_MC_PROXY:-false}" == "true" ]]; then
  log_step "SKIP mc_proxy_plots; builder will use latest existing local MC proxy bundle"
else
  run_logged mc_proxy_plots "$MC_PROXY_RUNNER" "$MC_PROXY_LABEL"
fi
if [[ "${SKIP_OFFICIAL_PROXY:-false}" == "true" ]]; then
  log_step "SKIP official_proxy_plots; builder will use latest existing local official proxy bundle if present"
else
  run_logged official_proxy_plots "$OFFICIAL_PROXY_RUNNER" "$OFFICIAL_PROXY_LABEL"
fi
run_logged an_recreation "$AN_RUNNER" "$AN_LABEL"

AN_DIR="$ROOT_DIR/research/cms-d0-analysis/output/an-recreation/$AN_LABEL"
run_logged promote_overleaf "$PROMOTER" "$AN_DIR" "Recreate D0 AN draft from reproduced data"
run_logged overleaf_check "$ROOT_DIR/scripts/project-registry.py" overleaf-check general-output

"$ROOT_DIR/scripts/cms-analysisctl" ledger \
  --event d0-full-an-recreation-workflow \
  --detail "label=$LABEL an_dir=$AN_DIR log_dir=$LOG_DIR" \
  --source "research/cms-d0-analysis/scripts/run_full_d0_an_recreation_workflow.sh" \
  --tag cms \
  --tag d0 \
  --tag an \
  --tag overleaf >/dev/null

log_step "AN_DIR=$AN_DIR"
log_step "PDF=$AN_DIR/main.pdf"
log_step "OVERLEAF_REPO=$ROOT_DIR/repos/general-codex-output"
log_step "DONE"

printf 'label=%s\n' "$LABEL"
printf 'log_dir=%s\n' "$LOG_DIR"
printf 'summary=%s\n' "$SUMMARY"
printf 'an_dir=%s\n' "$AN_DIR"
