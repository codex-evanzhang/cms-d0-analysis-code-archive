#!/usr/bin/env bash
set -euo pipefail

ROOT="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
# shellcheck source=/home/ubuntu/agent-network/scripts/ledger-hook.sh
source "$ROOT/scripts/ledger-hook.sh"
ledger_hook_start script cms-d0-from-miniaod-chain "run_from_miniaod_mass_chain.sh" "cms d0 miniaod lxplus"

LX="$ROOT/scripts/lxplusctl"
REMOTE_BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
REMOTE_FOREST_SCRIPT="$REMOTE_BASE/scripts/run_2023_forest_smoke.sh"
REMOTE_VALIDATE_SCRIPT="$REMOTE_BASE/scripts/run_recreated_vs_official_forest_validation.sh"
REMOTE_SELECTED_SCRIPT="$REMOTE_BASE/scripts/run_2023_d0_selected_skim.sh"
REMOTE_MASS_SCRIPT="$REMOTE_BASE/scripts/run_produced_skim_d0_peak.sh"

MAX_EVENTS=20000
LABEL=""
INPUT_FILE="root://xrootd-vanderbilt.sites.opensciencegrid.org//store/hidata/HIRun2023A/HIForward0/MINIAOD/16Jan2024-v1/2810000/3cb20bb5-a7b1-43a2-8025-9807f4f51e71.root"
OFFICIAL_FOREST="root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/jdlang/Run3_PbPbUPC/Forest_2023_Jan2024ReReco_2025Reforest/HIForward0/crab_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward0/260212_170406/0000/HiForest_2023PbPbUPC_Jan24Reco_428.root"
OUT_BASE="/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/from-miniaod-chain"
SKIP_FOREST=false
SKIP_VALIDATE=false

usage() {
  cat <<USAGE
Usage:
  run_from_miniaod_mass_chain.sh [options]

Options:
  --max-events N       Number of MINIAOD events to process before the forest superfilter.
  --label NAME         Reusable output label. Default is timestamped.
  --input-file PATH    MINIAOD input file or XRootD URL.
  --official PATH      Official forest shard for same-input validation.
  --out-base PATH      EOS/AFS base output directory under Evan-owned CERN space.
  --skip-forest        Reuse the forest output expected for --label.
  --skip-validate      Skip recreated-vs-official validation.

This orchestrates the practical from-scratch chain:
  MINIAOD -> recreated HiForest/Dfinder -> official forest validation
  -> Section 5/7 selected skim -> D0 mass-peak plots.

It is bounded and local-to-LXPLUS. It does not submit CRAB or launch bulk
production.
USAGE
}

q() {
  printf '%q' "$1"
}

remote() {
  "$LX" ssh "$1"
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --max-events)
      MAX_EVENTS="${2:?missing value for --max-events}"
      shift 2
      ;;
    --label)
      LABEL="${2:?missing value for --label}"
      shift 2
      ;;
    --input-file)
      INPUT_FILE="${2:?missing value for --input-file}"
      shift 2
      ;;
    --official)
      OFFICIAL_FOREST="${2:?missing value for --official}"
      shift 2
      ;;
    --out-base)
      OUT_BASE="${2:?missing value for --out-base}"
      shift 2
      ;;
    --skip-forest)
      SKIP_FOREST=true
      shift
      ;;
    --skip-validate)
      SKIP_VALIDATE=true
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      printf 'Unknown argument: %s\n' "$1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

case "$OUT_BASE" in
  /afs/cern.ch/user/e/evzhang*|/afs/cern.ch/work/e/evzhang*|/eos/user/e/evzhang*) ;;
  *)
    printf 'Refusing to write outside Evan-owned CERN paths: %s\n' "$OUT_BASE" >&2
    exit 1
    ;;
esac

if [[ -z "$LABEL" ]]; then
  LABEL="hiforward0_miniaod_${MAX_EVENTS}_fromscratch_$(date -u +%Y%m%dT%H%M%SZ)"
fi

FOREST_OUT="$OUT_BASE/recreated-forest"
VALIDATION_OUT="$OUT_BASE/validation"
FOREST_ROOT="$FOREST_OUT/$LABEL/HiForest_2023PbPbUPC_Jan24Reco_smoke.root"
FOREST_INPUT_DIR="$OUT_BASE/forest-input-dir/$LABEL"
STAGED_FOREST="$FOREST_INPUT_DIR/recreated_from_miniaod.root"
SELECTED_LABEL="${LABEL}_selected"
SELECTED_OUT="$OUT_BASE/selected-skim"
SECTION7_ROOT="$SELECTED_OUT/$SELECTED_LABEL/section7_d_selected.root"
MASS_OUT="$OUT_BASE/mass-peak/$SELECTED_LABEL"

printf 'label=%s\n' "$LABEL"
printf 'max_events=%s\n' "$MAX_EVENTS"
printf 'input_file=%s\n' "$INPUT_FILE"
printf 'official_forest=%s\n' "$OFFICIAL_FOREST"
printf 'forest_root=%s\n' "$FOREST_ROOT"

if [[ "$SKIP_FOREST" == "false" ]]; then
  remote "$(q "$REMOTE_FOREST_SCRIPT") --max-events $(q "$MAX_EVENTS") --input-file $(q "$INPUT_FILE") --label $(q "$LABEL") --outdir $(q "$FOREST_OUT")"
else
  printf 'Skipping forest production; expecting existing forest_root=%s\n' "$FOREST_ROOT"
fi

remote "test -s $(q "$FOREST_ROOT") && ls -lh $(q "$FOREST_ROOT")"

VALIDATION_STATUS="skipped"
if [[ "$SKIP_VALIDATE" == "false" ]]; then
  if remote "$(q "$REMOTE_VALIDATE_SCRIPT") --recreated $(q "$FOREST_ROOT") --official $(q "$OFFICIAL_FOREST") --label $(q "${LABEL}_vs_official") --out-base $(q "$VALIDATION_OUT")"; then
    VALIDATION_STATUS="ok"
  else
    VALIDATION_STATUS="mismatch-or-error"
    printf 'Validation reported a mismatch or error; continuing to mass-peak production for diagnostic output.\n' >&2
  fi
fi

remote "mkdir -p $(q "$FOREST_INPUT_DIR") && cp -f $(q "$FOREST_ROOT") $(q "$STAGED_FOREST") && ls -lh $(q "$STAGED_FOREST")"

remote "D0_REPRO_HIFORWARD0_FOREST_DIR=$(q "$FOREST_INPUT_DIR") $(q "$REMOTE_SELECTED_SCRIPT") --max-files 1 --label $(q "$SELECTED_LABEL") --outdir $(q "$SELECTED_OUT")"

remote "test -s $(q "$SECTION7_ROOT") && ls -lh $(q "$SECTION7_ROOT")"

remote "$(q "$REMOTE_MASS_SCRIPT") --input $(q "$SECTION7_ROOT") --outdir $(q "$MASS_OUT")"

printf '\nfrom_miniaod_chain_summary:\n'
printf '  label: %s\n' "$LABEL"
printf '  validation_status: %s\n' "$VALIDATION_STATUS"
printf '  forest_root: %s\n' "$FOREST_ROOT"
printf '  selected_skim: %s\n' "$SECTION7_ROOT"
printf '  mass_output: %s\n' "$MASS_OUT"

"$ROOT/scripts/cms-analysisctl" ledger \
  --event from-miniaod-mass-chain \
  --detail "label=$LABEL validation=$VALIDATION_STATUS max_events=$MAX_EVENTS mass_out=$MASS_OUT" \
  --source "research/cms-d0-analysis/scripts/run_from_miniaod_mass_chain.sh" \
  --tag miniaod \
  --tag d0 \
  --tag validation >/dev/null
