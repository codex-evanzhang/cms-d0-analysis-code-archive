#!/usr/bin/env bash
set -euo pipefail

ROOT="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
LOCAL_OUT_REL="research/cms-d0-analysis/output/cut-derivation/topological_pointing_closure_20260618"
LOCAL_OUT="$ROOT/$LOCAL_OUT_REL"
REMOTE_ROOT="/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction"
REMOTE_SCRIPT_DIR="$REMOTE_ROOT/scripts"
REMOTE_OUT="/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation/topological_pointing_closure_20260618"
DATA_PATH="/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root"
MAX_EVENTS="-1"

usage() {
  cat <<USAGE
Usage: $0 [--max-events N]

Runs the post-derivation Dalpha/Ddtheta neighboring-plateau closure on LXPLUS.
The local output is:
  $LOCAL_OUT_REL
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --max-events)
      MAX_EVENTS="${2:?missing value for --max-events}"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      usage >&2
      exit 2
      ;;
  esac
done

cd "$ROOT"
mkdir -p "$LOCAL_OUT"

.venv-plotting/bin/python \
  research/cms-d0-analysis/scripts/derive_topological_pointing_closure.py \
  --prepare-only \
  --outdir "$LOCAL_OUT"

scripts/lxplusctl ssh "mkdir -p '$REMOTE_SCRIPT_DIR' '$REMOTE_OUT'"
scp \
  research/cms-d0-analysis/scripts/check_topological_pointing_closure.C \
  "$LOCAL_OUT/closure_points.tsv" \
  lxplus-codex:"$REMOTE_SCRIPT_DIR/"

scripts/lxplusctl ssh \
  "set -e; source /cvmfs/cms.cern.ch/cmsset_default.sh; cd '$REMOTE_SCRIPT_DIR'; root -l -b -q 'check_topological_pointing_closure.C++(\"$DATA_PATH\",\"$REMOTE_SCRIPT_DIR/closure_points.tsv\",\"$REMOTE_OUT\",$MAX_EVENTS)'"

scp \
  lxplus-codex:"$REMOTE_OUT/README.md" \
  lxplus-codex:"$REMOTE_OUT/closure_yields.tsv" \
  lxplus-codex:"$REMOTE_OUT/closure_mass_hist.tsv" \
  "$LOCAL_OUT/"

.venv-plotting/bin/python \
  research/cms-d0-analysis/scripts/derive_topological_pointing_closure.py \
  --summarize-only \
  --outdir "$LOCAL_OUT"

echo "Local closure output: $LOCAL_OUT_REL"
