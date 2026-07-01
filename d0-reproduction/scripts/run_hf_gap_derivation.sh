#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
# shellcheck source=/home/ubuntu/agent-network/scripts/ledger-hook.sh
source "$ROOT_DIR/scripts/ledger-hook.sh"
ledger_hook_start script cms-d0-hf-gap-derivation "run_hf_gap_derivation.sh" "cms d0 hf lxplus root cuts plots"

LABEL="${1:-hf_gap_$(date -u +%Y%m%dT%H%M%SZ)}"
MAX_EVENTS="${MAX_EVENTS:--1}"
PROMPT_MC_FILE="${PROMPT_MC_FILE:-/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root}"
DATA_FILE="${DATA_FILE:-/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root}"
EMPTYBX_FILE="${EMPTYBX_FILE:-/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root}"
REMOTE_BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
REMOTE_SCRIPT_DIR="$REMOTE_BASE/scripts"
REMOTE_SCRIPT="$REMOTE_SCRIPT_DIR/derive_hf_gap_cutoffs_data_mc.C"
REMOTE_OUT="/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/$LABEL"
LOCAL_OUT="$ROOT_DIR/research/cms-d0-analysis/output/cut-derivation/$LABEL"
LX="$ROOT_DIR/scripts/lxplusctl"

q() {
  printf '%q' "$1"
}

"$LX" ssh "mkdir -p $(q "$REMOTE_SCRIPT_DIR") $(q "$REMOTE_OUT")"
scp -q "$ROOT_DIR/research/cms-d0-analysis/scripts/derive_hf_gap_cutoffs_data_mc.C" "lxplus-codex:$REMOTE_SCRIPT"
"$LX" ssh "set -e; source /cvmfs/cms.cern.ch/cmsset_default.sh; cd $(q "$REMOTE_SCRIPT_DIR"); root -l -b -q 'derive_hf_gap_cutoffs_data_mc.C++(\"$PROMPT_MC_FILE\",\"$DATA_FILE\",\"$EMPTYBX_FILE\",\"$REMOTE_OUT\",$MAX_EVENTS)'"

mkdir -p "$LOCAL_OUT"
rsync -a --include='*.png' --include='*.pdf' --include='*.tsv' --include='*.md' --exclude='*' "lxplus-codex:$REMOTE_OUT/" "$LOCAL_OUT/"

"$ROOT_DIR/scripts/cms-analysisctl" ledger \
  --event hf-gap-derivation-rendered \
  --detail "label=$LABEL local_out=$LOCAL_OUT remote_out=$REMOTE_OUT max_events=$MAX_EVENTS" \
  --source "research/cms-d0-analysis/scripts/run_hf_gap_derivation.sh" \
  --tag cms \
  --tag d0 \
  --tag hf \
  --tag cuts \
  --tag plots >/dev/null

printf 'remote_out=%s\n' "$REMOTE_OUT"
printf 'local_out=%s\n' "$LOCAL_OUT"
