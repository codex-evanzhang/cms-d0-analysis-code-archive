#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
# shellcheck source=/home/ubuntu/agent-network/scripts/ledger-hook.sh
source "$ROOT_DIR/scripts/ledger-hook.sh"
ledger_hook_start script cms-d0-bdt-topology "run_bdt_topology_optimization.sh" "cms d0 bdt lxplus"

LX="$ROOT_DIR/scripts/lxplusctl"
LABEL="bdt_topology_$(date -u +%Y%m%dT%H%M%SZ)"
REMOTE_BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
REMOTE_SCRIPT_DIR="$REMOTE_BASE/scripts"
REMOTE_SCRIPT="$REMOTE_SCRIPT_DIR/run_bdt_topology_optimization.py"
REMOTE_OUT_BASE="/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/bdt-topology"
DATA_DIR="/eos/user/e/evzhang/codex-d0-crab-smoke/HIForward0D0ForestSmoke/d0_minimal_raw_100files_hiforward0_20260618/260618_223033/0000"
MC_FILE="/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root"
MAX_MC_EVENTS=8000000
MAX_SIGNAL_PER_BIN=5000
MAX_DATA_FILES=-1
REUSE_FLAT=false
LOCAL_OUT_BASE="$ROOT_DIR/research/cms-d0-analysis/output/bdt-topology"

usage() {
  cat <<USAGE
Usage:
  run_bdt_topology_optimization.sh [options]

Options:
  --label NAME              Output label.
  --data-dir PATH           Recreated forest directory on EOS.
  --mc PATH                 Official flat prompt D0 MC ROOT file.
  --out-base PATH           Remote Evan-owned output base.
  --max-mc-events N         Max MC tree entries to scan. Default: $MAX_MC_EVENTS.
  --max-signal-per-bin N    Stop filling MC signal after this many per bin. Default: $MAX_SIGNAL_PER_BIN.
  --max-data-files N        Limit recreated forest files. Default: all.
  --reuse-flat              Reuse existing bdt_training_flat.root in output dir.

This trains one ROOT/TMVA BDT per pT/|y| bin using MC signal and data
sidebands, then applies the frozen thresholds to the recreated forest data.
USAGE
}

q() {
  printf '%q' "$1"
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --label)
      LABEL="${2:?missing label}"
      shift 2
      ;;
    --data-dir)
      DATA_DIR="${2:?missing data dir}"
      shift 2
      ;;
    --mc)
      MC_FILE="${2:?missing MC file}"
      shift 2
      ;;
    --out-base)
      REMOTE_OUT_BASE="${2:?missing out base}"
      shift 2
      ;;
    --max-mc-events)
      MAX_MC_EVENTS="${2:?missing max mc events}"
      shift 2
      ;;
    --max-signal-per-bin)
      MAX_SIGNAL_PER_BIN="${2:?missing max signal per bin}"
      shift 2
      ;;
    --max-data-files)
      MAX_DATA_FILES="${2:?missing max data files}"
      shift 2
      ;;
    --reuse-flat)
      REUSE_FLAT=true
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

case "$REMOTE_OUT_BASE" in
  /afs/cern.ch/user/e/evzhang*|/afs/cern.ch/work/e/evzhang*|/eos/user/e/evzhang*) ;;
  *)
    printf 'Refusing to write outside Evan-owned CERN paths: %s\n' "$REMOTE_OUT_BASE" >&2
    exit 1
    ;;
esac

REMOTE_OUT="$REMOTE_OUT_BASE/$LABEL"
REMOTE_LIST="$REMOTE_OUT/data_files.txt"
LOCAL_OUT="$LOCAL_OUT_BASE/$LABEL"

"$LX" ssh "mkdir -p $(q "$REMOTE_SCRIPT_DIR") $(q "$REMOTE_OUT")"
scp -q "$ROOT_DIR/research/cms-d0-analysis/scripts/run_bdt_topology_optimization.py" "lxplus-codex:$REMOTE_SCRIPT"
"$LX" ssh "chmod 755 $(q "$REMOTE_SCRIPT")"

"$LX" ssh "find $(q "$DATA_DIR") -maxdepth 1 -type f -name '*.root' | sort > $(q "$REMOTE_LIST") && wc -l $(q "$REMOTE_LIST")"

REUSE_ARG=""
if [[ "$REUSE_FLAT" == "true" ]]; then
  REUSE_ARG="--reuse-flat"
fi

"$LX" ssh "set -e; cd $(q "$REMOTE_OUT"); python3 $(q "$REMOTE_SCRIPT") --mc $(q "$MC_FILE") --data-list $(q "$REMOTE_LIST") --outdir $(q "$REMOTE_OUT") --max-mc-events $(q "$MAX_MC_EVENTS") --max-signal-per-bin $(q "$MAX_SIGNAL_PER_BIN") --max-data-files $(q "$MAX_DATA_FILES") $REUSE_ARG"

mkdir -p "$LOCAL_OUT"
rsync -a \
  --include='README.md' \
  --include='metadata.json' \
  --include='*.tsv' \
  --include='plots/' \
  --include='plots/*.png' \
  --include='plots/*.pdf' \
  --exclude='*' \
  "lxplus-codex:$REMOTE_OUT/" \
  "$LOCAL_OUT/"

printf 'remote_out=%s\n' "$REMOTE_OUT"
printf 'local_out=%s\n' "$LOCAL_OUT"

"$ROOT_DIR/scripts/cms-analysisctl" ledger \
  --event bdt-topology-optimization \
  --detail "label=$LABEL remote_out=$REMOTE_OUT data_dir=$DATA_DIR max_mc_events=$MAX_MC_EVENTS max_signal_per_bin=$MAX_SIGNAL_PER_BIN max_data_files=$MAX_DATA_FILES" \
  --source "research/cms-d0-analysis/scripts/run_bdt_topology_optimization.sh" \
  --tag bdt \
  --tag topology \
  --tag d0 >/dev/null
