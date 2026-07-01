#!/usr/bin/env bash
set -euo pipefail

BASE="/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction"
EOS_BASE="/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction"
SIGNAL_MC="/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root"
DATA="/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root"
CONTROL="/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root"
LABEL="prompt_mc_data_sideband_emptybx_200k"
MAX_EVENTS=200000
OUTDIR=""

usage() {
  cat <<USAGE
Usage:
  run_2023_cut_derivation.sh [options]

Options:
  --signal-mc FILE   Signal MC skim Tree input.
  --data FILE        Data skim Tree input for sideband background.
  --control FILE     EBX/control skim Tree input for detector-noise fake rate.
  --label LABEL      Output label.
  --max-events N     Max entries per sample; use -1 for all entries.
  --outdir DIR       Output directory.

Default mode is a bounded smoke derivation, not final cut freezing.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --signal-mc)
      SIGNAL_MC="$2"
      shift 2
      ;;
    --data)
      DATA="$2"
      shift 2
      ;;
    --control)
      CONTROL="$2"
      shift 2
      ;;
    --label)
      LABEL="$2"
      shift 2
      ;;
    --max-events)
      MAX_EVENTS="$2"
      shift 2
      ;;
    --outdir)
      OUTDIR="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if [[ -z "$OUTDIR" ]]; then
  OUTDIR="$EOS_BASE/cut-derivation/$LABEL"
fi

for input in "$SIGNAL_MC" "$DATA" "$CONTROL"; do
  if [[ ! -f "$input" ]]; then
    echo "Missing input: $input" >&2
    exit 1
  fi
done

mkdir -p "$OUTDIR" "$BASE/logs"
cp "$BASE/CUT_DERIVATION.md" "$OUTDIR/CUT_DERIVATION.md" || true

LOG="$BASE/logs/run_2023_cut_derivation_${LABEL}_$(date -u +%Y%m%dT%H%M%SZ).log"
root -l -b -q "$BASE/scripts/derive_2023_d0_cuts.C+(\"$SIGNAL_MC\",\"$DATA\",\"$CONTROL\",\"$OUTDIR\",$MAX_EVENTS)" 2>&1 | tee "$LOG"

{
  printf '\n## Cut Derivation: %s\n\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf -- '- Label: `%s`\n' "$LABEL"
  printf -- '- Signal MC: `%s`\n' "$SIGNAL_MC"
  printf -- '- Data sideband sample: `%s`\n' "$DATA"
  printf -- '- Control sample: `%s`\n' "$CONTROL"
  printf -- '- Max events per sample: `%s`\n' "$MAX_EVENTS"
  printf -- '- Output directory: `%s`\n' "$OUTDIR"
  printf -- '- Log: `%s`\n' "$LOG"
  printf -- '- Scope: MC/data/EBX cut derivation scan; bounded runs are provisional.\n'
} >> "$BASE/LOG.md"

printf 'Cut derivation complete: %s\n' "$OUTDIR"
