#!/usr/bin/env bash
set -euo pipefail

BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
DATASET_DIR="${D0_REPRO_HIFORWARD0_FOREST_DIR:-/eos/user/e/evzhang/codex-eos/datasets/vanderbilt_HIForward0_251227_162520_0000_0006}"
OUTDIR="${D0_REPRO_SKIM_OUTDIR:-/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-skim-2023}"
INPUT_LIST="$BASE/manifests/hiForward0_local_forest_files.txt"
MAX_EVENTS=-1
MAX_FILES=-1
COMPUTE_HF=0
LABEL="hiForward0_first_pass"

usage() {
  cat <<USAGE
Usage:
  run_2023_d0_skim_recreation.sh [--smoke] [--max-events N] [--max-files N] [--label NAME] [--outdir PATH]

Creates a first-pass recreated D0 analysis skim from current-schema 2023
HIForward0 forest files. Existing D0 skims are validation references only.
USAGE
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --smoke)
      MAX_EVENTS=1000
      MAX_FILES=1
      LABEL="smoke_batch001_1k"
      shift
      ;;
    --compute-hf)
      COMPUTE_HF=1
      shift
      ;;
    --max-events)
      MAX_EVENTS="${2:?missing value for --max-events}"
      shift 2
      ;;
    --max-files)
      MAX_FILES="${2:?missing value for --max-files}"
      shift 2
      ;;
    --label)
      LABEL="${2:?missing value for --label}"
      shift 2
      ;;
    --outdir)
      OUTDIR="${2:?missing value for --outdir}"
      shift 2
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

case "$OUTDIR" in
  /afs/cern.ch/user/e/evzhang*|/afs/cern.ch/work/e/evzhang*|/eos/user/e/evzhang*)
    ;;
  *)
    printf 'Refusing to write outside Evan-owned CERN paths: %s\n' "$OUTDIR" >&2
    exit 1
    ;;
esac

mkdir -p "$BASE/logs" "$BASE/manifests" "$OUTDIR/$LABEL" "$OUTDIR/$LABEL/plots"
find "$DATASET_DIR" -maxdepth 1 -type f -name '*.root' | sort > "$INPUT_LIST"

OUTPUT_ROOT="$OUTDIR/$LABEL/recreated_d0_micro_skim_HIForward0.root"
PLOT_DIR="$OUTDIR/$LABEL/plots"
LOG="$BASE/logs/run_2023_d0_skim_recreation_${LABEL}_$(date -u +%Y%m%dT%H%M%SZ).log"
MANIFEST="$BASE/manifests/recreated_skim_2023_${LABEL}.md"

{
  printf 'started_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf 'host=%s\n' "$(hostname -f 2>/dev/null || hostname)"
  printf 'dataset_dir=%s\n' "$DATASET_DIR"
  printf 'input_list=%s\n' "$INPUT_LIST"
  printf 'input_file_count=%s\n' "$(wc -l < "$INPUT_LIST")"
  printf 'output_root=%s\n' "$OUTPUT_ROOT"
  printf 'plot_dir=%s\n' "$PLOT_DIR"
  printf 'max_events=%s\n' "$MAX_EVENTS"
  printf 'max_files=%s\n' "$MAX_FILES"
  printf 'compute_hf=%s\n' "$COMPUTE_HF"
  root -l -b -q "$BASE/scripts/recreate_2023_d0_micro_skim.C+(\"$INPUT_LIST\",\"$OUTPUT_ROOT\",\"$PLOT_DIR\",$MAX_EVENTS,$MAX_FILES,$COMPUTE_HF)"
  printf 'finished_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} 2>&1 | tee "$LOG"

{
  printf '# Recreated 2023 D0 Skim: %s\n\n' "$LABEL"
  printf '<!-- DOC_OWNER: cms-analysis-manager skim reproduction manifest. -->\n'
  printf '<!-- TOKEN_NOTE: concise status; detailed command output is in logs/. -->\n\n'
  printf '## Inputs\n\n'
  printf -- '- Forest source: `%s`\n' "$DATASET_DIR"
  printf -- '- Input list: `%s`\n' "$INPUT_LIST"
  printf -- '- Input file count in list: `%s`\n' "$(wc -l < "$INPUT_LIST")"
  printf -- '- max_files argument: `%s`\n' "$MAX_FILES"
  printf -- '- max_events argument: `%s`\n\n' "$MAX_EVENTS"
  printf -- '- compute_hf argument: `%s`\n\n' "$COMPUTE_HF"
  printf '## Outputs\n\n'
  printf -- '- Recreated skim ROOT: `%s`\n' "$OUTPUT_ROOT"
  printf -- '- Plots/summary directory: `%s`\n' "$PLOT_DIR"
  printf -- '- Run log: `%s`\n\n' "$LOG"
  printf '## Reducer Scope\n\n'
  printf -- '- Reads aligned trees: `hiEvtAnalyzer/HiTree`, `PbPbTracks/trackTree`, `skimanalysis/HltTree`, `l1MetFilterRecoTree/MetFilterRecoTree`, `zdcanalyzer/zdcrechit`, `particleFlowAnalyser/pftree`, `Dfinder/ntDkpi`.\n'
  printf -- '- Reads `hltanalysis/HltTree` by `(run,lumi,event)` lookup because it is not entry-aligned in the checked forest batch.\n'
  printf -- '- Copies event metadata, ZDC/HF quantities, key Dfinder branches, and computes `DpassCut23PAS` from the AN Table 7 rectangular cuts.\n\n'
  printf '## Current Caveats\n\n'
  printf -- '- This is a first-pass analysis skim subset, not yet a bit-for-bit clone of the Wang reference skim.\n'
  printf -- '- `DpassCutNominal` is temporarily set equal to recreated `DpassCut23PAS` until the exact nominal-cut definition is recovered.\n'
  printf -- '- `clusComp_quality` and `nTrackInAcceptanceHP` are placeholders in this first pass.\n'
  printf -- '- `HFEMaxPlus`, `HFEMaxMinus`, and derived gap flags are placeholders unless `--compute-hf` was used.\n'
  printf -- '- HLT trigger branches are filled only when the separate `hltanalysis/HltTree` contains a matching event key.\n\n'
  printf '## Summary Metrics\n\n'
  if [[ -f "$PLOT_DIR/recreated_skim_summary.tsv" ]]; then
    printf '```tsv\n'
    cat "$PLOT_DIR/recreated_skim_summary.tsv"
    printf '```\n'
  else
    printf 'Summary file was not produced.\n'
  fi
} > "$MANIFEST"

{
  printf '%s\t%s\t%s\t%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "recreate-skim" "$LABEL" "$OUTPUT_ROOT"
} >> "$BASE/logs/action-log.tsv"

printf 'manifest=%s\n' "$MANIFEST"
printf 'log=%s\n' "$LOG"
printf 'output_root=%s\n' "$OUTPUT_ROOT"
