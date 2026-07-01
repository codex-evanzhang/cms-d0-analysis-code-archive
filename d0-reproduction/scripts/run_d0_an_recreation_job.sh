#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
# shellcheck source=/home/ubuntu/agent-network/scripts/ledger-hook.sh
source "$ROOT_DIR/scripts/ledger-hook.sh"
ledger_hook_start script cms-d0-an-recreation "run_d0_an_recreation_job.sh" "cms d0 an latex"

LABEL="${1:-d0_an_recreation_$(date -u +%Y%m%dT%H%M%SZ)}"
OUT_BASE="$ROOT_DIR/research/cms-d0-analysis/output/an-recreation"
OUT_DIR="$OUT_BASE/$LABEL"
SCRIPT="$ROOT_DIR/research/cms-d0-analysis/scripts/build_d0_an_recreation.py"

mkdir -p "$OUT_DIR"

python3 "$SCRIPT" \
  --outdir "$OUT_DIR" \
  --auto-latest-proxy \
  --auto-latest-mc-proxy \
  --auto-latest-official-proxy \
  --auto-latest-normalization \
  --auto-latest-theory \
  --auto-latest-pthat \
  --auto-latest-yield-fit \
  --auto-latest-zdc-multipeak \
  --auto-latest-zdc-1n \
  --auto-latest-hf-gap \
  --auto-latest-bdt \
  --compile

"$ROOT_DIR/scripts/cms-analysisctl" ledger \
  --event d0-an-recreation-draft \
  --detail "label=$LABEL out_dir=$OUT_DIR" \
  --source "research/cms-d0-analysis/scripts/run_d0_an_recreation_job.sh" \
  --tag cms \
  --tag d0 \
  --tag an \
  --tag latex >/dev/null

printf 'out_dir=%s\n' "$OUT_DIR"
printf 'pdf=%s\n' "$OUT_DIR/main.pdf"
printf 'coverage=%s\n' "$OUT_DIR/coverage_matrix.tsv"
