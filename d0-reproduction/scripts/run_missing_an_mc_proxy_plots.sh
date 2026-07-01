#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
# shellcheck source=/home/ubuntu/agent-network/scripts/ledger-hook.sh
source "$ROOT_DIR/scripts/ledger-hook.sh"
ledger_hook_start script cms-d0-missing-an-mc-proxy "run_missing_an_mc_proxy_plots.sh" "cms d0 an mc lxplus root plots"

LABEL="${1:-missing_an_mc_proxy_$(date -u +%Y%m%dT%H%M%SZ)}"
MAX_EVENTS="${MAX_EVENTS:-2000000}"
REMOTE_BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
REMOTE_SCRIPT_DIR="$REMOTE_BASE/scripts"
REMOTE_SCRIPT="$REMOTE_SCRIPT_DIR/render_missing_an_mc_proxy_plots.C"
REMOTE_OUT="/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/missing-an-mc-proxy/$LABEL"
LOCAL_OUT="$ROOT_DIR/research/cms-d0-analysis/output/missing-an-mc-proxy/$LABEL"
MC_FILE="${MC_FILE:-/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root}"
DATA_SELECTED="${DATA_SELECTED:-/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/crab_98forest_apriori_20260619/section7_d_selected.root}"
LX="$ROOT_DIR/scripts/lxplusctl"

q() {
  printf '%q' "$1"
}

"$LX" ssh "mkdir -p $(q "$REMOTE_SCRIPT_DIR") $(q "$REMOTE_OUT")"
scp -q "$ROOT_DIR/research/cms-d0-analysis/scripts/render_missing_an_mc_proxy_plots.C" "lxplus-codex:$REMOTE_SCRIPT"
"$LX" ssh "set -e; source /cvmfs/cms.cern.ch/cmsset_default.sh; root -l -b -q '$(q "$REMOTE_SCRIPT")+(\"$MC_FILE\",\"$DATA_SELECTED\",\"$REMOTE_OUT\",$MAX_EVENTS)'"

mkdir -p "$LOCAL_OUT"
rsync -a --include='*.png' --include='*.pdf' --include='*.tsv' --include='*.md' --exclude='*' "lxplus-codex:$REMOTE_OUT/" "$LOCAL_OUT/"

"$ROOT_DIR/scripts/cms-analysisctl" ledger \
  --event missing-an-mc-proxy-plots-rendered \
  --detail "label=$LABEL local_out=$LOCAL_OUT remote_out=$REMOTE_OUT max_events=$MAX_EVENTS" \
  --source "research/cms-d0-analysis/scripts/run_missing_an_mc_proxy_plots.sh" \
  --tag cms \
  --tag d0 \
  --tag mc \
  --tag an \
  --tag plots >/dev/null

printf 'remote_out=%s\n' "$REMOTE_OUT"
printf 'local_out=%s\n' "$LOCAL_OUT"
