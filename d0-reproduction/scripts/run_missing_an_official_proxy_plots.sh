#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
# shellcheck source=/home/ubuntu/agent-network/scripts/ledger-hook.sh
source "$ROOT_DIR/scripts/ledger-hook.sh"
ledger_hook_start script cms-d0-missing-an-official-proxy "run_missing_an_official_proxy_plots.sh" "cms d0 an official lxplus root plots"

LABEL="${1:-missing_an_official_proxy_$(date -u +%Y%m%dT%H%M%SZ)}"
MAX_DATA_EVENTS="${MAX_DATA_EVENTS:-5000000}"
MAX_MC_EVENTS="${MAX_MC_EVENTS:-5000000}"
REMOTE_BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
REMOTE_SCRIPT_DIR="$REMOTE_BASE/scripts"
REMOTE_SCRIPT="$REMOTE_SCRIPT_DIR/render_missing_an_official_proxy_plots.C"
REMOTE_OUT="/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/missing-an-official-proxy/$LABEL"
LOCAL_OUT="$ROOT_DIR/research/cms-d0-analysis/output/missing-an-official-proxy/$LABEL"
DATA_FILE="${DATA_FILE:-/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root}"
PROMPT_MC_FILE="${PROMPT_MC_FILE:-/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root}"
NONPROMPT_MC_FILE="${NONPROMPT_MC_FILE:-/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/260203/Dzero_260203_HiForest_260120_nonprompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_Dpt1_PF0p1.root}"
LX="$ROOT_DIR/scripts/lxplusctl"

q() {
  printf '%q' "$1"
}

"$LX" ssh "mkdir -p $(q "$REMOTE_SCRIPT_DIR") $(q "$REMOTE_OUT")"
scp -q "$ROOT_DIR/research/cms-d0-analysis/scripts/render_missing_an_official_proxy_plots.C" "lxplus-codex:$REMOTE_SCRIPT"
"$LX" ssh "set -e; source /cvmfs/cms.cern.ch/cmsset_default.sh; root -l -b -q '$(q "$REMOTE_SCRIPT")+(\"$DATA_FILE\",\"$PROMPT_MC_FILE\",\"$NONPROMPT_MC_FILE\",\"$REMOTE_OUT\",$MAX_DATA_EVENTS,$MAX_MC_EVENTS)'"

mkdir -p "$LOCAL_OUT"
rsync -a --include='*.png' --include='*.pdf' --include='*.tsv' --include='*.md' --exclude='*' "lxplus-codex:$REMOTE_OUT/" "$LOCAL_OUT/"

"$ROOT_DIR/scripts/cms-analysisctl" ledger \
  --event missing-an-official-proxy-plots-rendered \
  --detail "label=$LABEL local_out=$LOCAL_OUT remote_out=$REMOTE_OUT max_data_events=$MAX_DATA_EVENTS max_mc_events=$MAX_MC_EVENTS" \
  --source "research/cms-d0-analysis/scripts/run_missing_an_official_proxy_plots.sh" \
  --tag cms \
  --tag d0 \
  --tag an \
  --tag official-proxy \
  --tag plots >/dev/null

printf 'remote_out=%s\n' "$REMOTE_OUT"
printf 'local_out=%s\n' "$LOCAL_OUT"
