#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
# shellcheck source=/home/ubuntu/agent-network/scripts/ledger-hook.sh
source "$ROOT_DIR/scripts/ledger-hook.sh"
ledger_hook_start script cms-d0-an-proxy-plots "run_recreated_an_proxy_plots.sh" "cms d0 lxplus root plots"

LABEL="${1:-an_proxy_plots_$(date -u +%Y%m%dT%H%M%SZ)}"
REMOTE_BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
REMOTE_SCRIPT_DIR="$REMOTE_BASE/scripts"
REMOTE_SCRIPT="$REMOTE_SCRIPT_DIR/render_recreated_an_proxy_plots.C"
REMOTE_OUT="/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/an-proxy-plots/$LABEL"
LOCAL_OUT="$ROOT_DIR/research/cms-d0-analysis/output/an-proxy-plots/$LABEL"
SECTION5="${SECTION5_ROOT:-/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/crab_98forest_apriori_20260619/section5_event_selected.root}"
SECTION7="${SECTION7_ROOT:-/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-selected-skim-2023/crab_98forest_apriori_20260619/section7_d_selected.root}"
LX="$ROOT_DIR/scripts/lxplusctl"

q() {
  printf '%q' "$1"
}

"$LX" ssh "mkdir -p $(q "$REMOTE_SCRIPT_DIR") $(q "$REMOTE_OUT")"
scp -q "$ROOT_DIR/research/cms-d0-analysis/scripts/render_recreated_an_proxy_plots.C" "lxplus-codex:$REMOTE_SCRIPT"
"$LX" ssh "set -e; source /cvmfs/cms.cern.ch/cmsset_default.sh; root -l -b -q '$(q "$REMOTE_SCRIPT")+(\"$SECTION5\",\"$SECTION7\",\"$REMOTE_OUT\")'"

mkdir -p "$LOCAL_OUT"
rsync -a --include='*.png' --include='*.pdf' --include='*.tsv' --include='*.md' --exclude='*' "lxplus-codex:$REMOTE_OUT/" "$LOCAL_OUT/"

"$ROOT_DIR/scripts/cms-analysisctl" ledger \
  --event an-proxy-plots-rendered \
  --detail "label=$LABEL local_out=$LOCAL_OUT remote_out=$REMOTE_OUT" \
  --source "research/cms-d0-analysis/scripts/run_recreated_an_proxy_plots.sh" \
  --tag cms \
  --tag d0 \
  --tag plots >/dev/null

printf 'remote_out=%s\n' "$REMOTE_OUT"
printf 'local_out=%s\n' "$LOCAL_OUT"
