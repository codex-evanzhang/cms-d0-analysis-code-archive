#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
LABEL="${1:-d0_an_gap_push_$(date -u +%Y%m%dT%H%M%SZ)}"
RUN_DIR="$ROOT_DIR/research/cms-d0-analysis/output/an-gap-push/$LABEL"
LOG_DIR="$RUN_DIR/logs"
JOBS_TSV="$RUN_DIR/jobs.tsv"

mkdir -p "$LOG_DIR"
printf 'job\tpid\tlog\tclassification\tcommand\n' >"$JOBS_TSV"

launch_job() {
  local job="$1"
  local classification="$2"
  shift 2
  local log="$LOG_DIR/$job.log"
  setsid bash -lc "$*" >"$log" 2>&1 &
  local pid="$!"
  printf '%s\t%s\t%s\t%s\t%s\n' "$job" "$pid" "${log#$ROOT_DIR/}" "$classification" "$*" >>"$JOBS_TSV"
  printf '%s\tpid=%s\tlog=%s\n' "$job" "$pid" "${log#$ROOT_DIR/}"
}

cd "$ROOT_DIR"

launch_job \
  zdc_multipeak_twiki30m \
  twiki-official-reduced-tree \
  "MAX_EVENTS=${ZDC_MULTIPEAK_MAX_EVENTS:-30000000} research/cms-d0-analysis/scripts/run_zdc_multipeak_fits.sh zdc_multipeak_twiki30m_20260622"

launch_job \
  zdc_1n_twiki_full \
  twiki-official-reduced-tree \
  "research/cms-d0-analysis/scripts/run_zdc_1n_threshold_derivation.sh --label zdc_1n_twiki_full_20260622 --max-events ${ZDC_1N_MAX_EVENTS:--1}"

launch_job \
  hf_gap_twiki_full \
  twiki-official-reduced-tree-and-official-mc-proxy \
  "MAX_EVENTS=${HF_MAX_EVENTS:--1} research/cms-d0-analysis/scripts/run_hf_gap_derivation.sh hf_gap_twiki_full_20260622"

launch_job \
  topology_independent_sideband_full \
  twiki-official-reduced-tree-and-official-mc-proxy \
  "research/cms-d0-analysis/scripts/run_independent_sideband_topology_derivation.sh --label 20260622_gap_push --max-events ${TOPOLOGY_MAX_EVENTS:--1}"

launch_job \
  bdt_topology_98forests \
  recreated-98-forest-and-official-mc-proxy \
  "research/cms-d0-analysis/scripts/run_bdt_topology_optimization.sh --label bdt_topology_gap_push_20260622 --max-mc-events ${BDT_MAX_MC_EVENTS:-12000000} --max-signal-per-bin ${BDT_MAX_SIGNAL_PER_BIN:-10000}"

launch_job \
  mc_proxy_12m \
  official-mc-proxy-and-recreated-98-forest \
  "MAX_EVENTS=${MC_PROXY_MAX_EVENTS:-12000000} research/cms-d0-analysis/scripts/run_missing_an_mc_proxy_plots.sh missing_an_mc_proxy_gap_push_20260622_12m"

launch_job \
  official_proxy_20m10m \
  twiki-official-reduced-tree-and-official-mc-proxy \
  "MAX_DATA_EVENTS=${OFFICIAL_PROXY_MAX_DATA_EVENTS:-20000000} MAX_MC_EVENTS=${OFFICIAL_PROXY_MAX_MC_EVENTS:-10000000} research/cms-d0-analysis/scripts/run_missing_an_official_proxy_plots.sh missing_an_official_proxy_gap_push_20260622_20m10m"

"$ROOT_DIR/scripts/cms-analysisctl" ledger \
  --event d0-an-gap-push-detached-launched \
  --detail "label=$LABEL run_dir=${RUN_DIR#$ROOT_DIR/} jobs=$JOBS_TSV" \
  --source "research/cms-d0-analysis/scripts/launch_d0_an_gap_push_detached.sh" \
  --tag cms \
  --tag d0 \
  --tag an \
  --tag detached >/dev/null

printf 'run_dir=%s\n' "${RUN_DIR#$ROOT_DIR/}"
printf 'jobs=%s\n' "${JOBS_TSV#$ROOT_DIR/}"
