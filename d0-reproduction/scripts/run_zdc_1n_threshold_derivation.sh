#!/usr/bin/env bash
set -euo pipefail

ROOT="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
# shellcheck source=/home/ubuntu/agent-network/scripts/ledger-hook.sh
source "$ROOT/scripts/ledger-hook.sh"
ledger_hook_start script zdc-1n-threshold-derivation "run_zdc_1n_threshold_derivation.sh" "cms d0 zdc lxplus"

SSH_HOST="${LXPLUS_SSH_HOST:-lxplus-codex}"
LABEL="data_emptybx_5m_$(date -u +%Y%m%dT%H%M%SZ)"
MAX_EVENTS=5000000
DATA_PATH="/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root"
EMPTYBX_PATH="/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root"
REMOTE_SCRIPT_DIR="/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts"
REMOTE_OUT_BASE="/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/zdc-1n-thresholds"
LOCAL_OUT_BASE="$ROOT/research/cms-d0-analysis/output/zdc-1n-thresholds"

usage() {
  cat <<USAGE
Usage: run_zdc_1n_threshold_derivation.sh [options]

Options:
  --label LABEL
  --max-events N
  --data PATH
  --emptybx PATH
  -h, --help
USAGE
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --label)
      LABEL="${2:?missing value for --label}"
      shift 2
      ;;
    --max-events)
      MAX_EVENTS="${2:?missing value for --max-events}"
      shift 2
      ;;
    --data)
      DATA_PATH="${2:?missing value for --data}"
      shift 2
      ;;
    --emptybx)
      EMPTYBX_PATH="${2:?missing value for --emptybx}"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      printf 'Unknown option: %s\n' "$1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

MACRO="$ROOT/research/cms-d0-analysis/scripts/derive_zdc_1n_thresholds_data_driven.C"
REMOTE_MACRO="$REMOTE_SCRIPT_DIR/derive_zdc_1n_thresholds_data_driven.C"
REMOTE_OUT="$REMOTE_OUT_BASE/$LABEL"
LOCAL_OUT="$LOCAL_OUT_BASE/$LABEL"

mkdir -p "$LOCAL_OUT_BASE"

printf 'Copying macro to LXPLUS...\n'
scp -q "$MACRO" "$SSH_HOST:$REMOTE_MACRO"

printf 'Running ZDC 1n threshold derivation on LXPLUS...\n'
ssh "$SSH_HOST" "
  set -euo pipefail
  source /cvmfs/cms.cern.ch/cmsset_default.sh
  mkdir -p '$REMOTE_OUT'
  cd '$REMOTE_SCRIPT_DIR'
  root -l -b -q 'derive_zdc_1n_thresholds_data_driven.C(\"$DATA_PATH\",\"$EMPTYBX_PATH\",\"$REMOTE_OUT\",$MAX_EVENTS)'
"

printf 'Copying outputs back locally...\n'
mkdir -p "$LOCAL_OUT"
scp -q "$SSH_HOST:$REMOTE_OUT/"'README.md' "$LOCAL_OUT/" || true
scp -q "$SSH_HOST:$REMOTE_OUT/"'*.tsv' "$LOCAL_OUT/" || true
scp -q "$SSH_HOST:$REMOTE_OUT/"'*.png' "$LOCAL_OUT/" || true
scp -q "$SSH_HOST:$REMOTE_OUT/"'*.pdf' "$LOCAL_OUT/" || true

"$ROOT/scripts/cms-analysisctl" ledger \
  --event zdc-1n-threshold-derivation \
  --detail "label=$LABEL max_events=$MAX_EVENTS remote=$REMOTE_OUT local=${LOCAL_OUT#$ROOT/}" \
  --source "$MACRO" \
  --tag zdc \
  --tag cuts \
  --tag lxplus >/dev/null

printf 'Remote output: %s\n' "$REMOTE_OUT"
printf 'Local output: %s\n' "${LOCAL_OUT#$ROOT/}"
