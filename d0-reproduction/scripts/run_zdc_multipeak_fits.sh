#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
# shellcheck source=/home/ubuntu/agent-network/scripts/ledger-hook.sh
source "$ROOT_DIR/scripts/ledger-hook.sh"
ledger_hook_start script cms-d0-zdc-multipeak-fits "run_zdc_multipeak_fits.sh" "cms d0 zdc lxplus root plots"

LABEL="${1:-zdc_multipeak_$(date -u +%Y%m%dT%H%M%SZ)}"
MAX_EVENTS="${MAX_EVENTS:-10000000}"
DATA_FILE="${DATA_FILE:-/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root}"
EMPTYBX_FILE="${EMPTYBX_FILE:-/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root}"
REMOTE_BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
REMOTE_SCRIPT_DIR="$REMOTE_BASE/scripts"
REMOTE_SCRIPT="$REMOTE_SCRIPT_DIR/render_zdc_multipeak_fits.C"
REMOTE_OUT="/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/zdc-multipeak-fits/$LABEL"
LOCAL_OUT="$ROOT_DIR/research/cms-d0-analysis/output/zdc-multipeak-fits/$LABEL"
LX="$ROOT_DIR/scripts/lxplusctl"

q() {
  printf '%q' "$1"
}

"$LX" ssh "mkdir -p $(q "$REMOTE_SCRIPT_DIR") $(q "$REMOTE_OUT")"
scp -q "$ROOT_DIR/research/cms-d0-analysis/scripts/render_zdc_multipeak_fits.C" "lxplus-codex:$REMOTE_SCRIPT"
"$LX" ssh "set -e; source /cvmfs/cms.cern.ch/cmsset_default.sh; root -l -b -q '$(q "$REMOTE_SCRIPT")+(\"$DATA_FILE\",\"$EMPTYBX_FILE\",\"$REMOTE_OUT\",$MAX_EVENTS)'"

mkdir -p "$LOCAL_OUT"
rsync -a --include='*.png' --include='*.pdf' --include='*.tsv' --include='*.md' --exclude='*' "lxplus-codex:$REMOTE_OUT/" "$LOCAL_OUT/"

"$ROOT_DIR/scripts/cms-analysisctl" ledger \
  --event zdc-multipeak-fits-rendered \
  --detail "label=$LABEL local_out=$LOCAL_OUT remote_out=$REMOTE_OUT max_events=$MAX_EVENTS" \
  --source "research/cms-d0-analysis/scripts/run_zdc_multipeak_fits.sh" \
  --tag cms \
  --tag d0 \
  --tag zdc \
  --tag plots >/dev/null

printf 'remote_out=%s\n' "$REMOTE_OUT"
printf 'local_out=%s\n' "$LOCAL_OUT"
