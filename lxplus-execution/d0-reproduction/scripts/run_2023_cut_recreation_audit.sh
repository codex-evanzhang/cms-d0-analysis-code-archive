#!/usr/bin/env bash
set -euo pipefail

BASE="/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction"
EOS_BASE="/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction"
LABEL="hiForward0_27files_selected_first_pass"
SECTION5=""
SECTION7=""
OUTDIR=""

usage() {
  cat <<USAGE
Usage:
  run_2023_cut_recreation_audit.sh [--label LABEL] [--section5 FILE] [--section7 FILE] [--outdir DIR]

Default inputs are the first-pass HIForward0 selected-skim outputs.
The script writes a compact cut audit under Evan-owned EOS.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --label)
      LABEL="$2"
      shift 2
      ;;
    --section5)
      SECTION5="$2"
      shift 2
      ;;
    --section7)
      SECTION7="$2"
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

if [[ -z "$SECTION5" ]]; then
  SECTION5="$EOS_BASE/recreated-selected-skim-2023/$LABEL/section5_event_selected.root"
fi
if [[ -z "$SECTION7" ]]; then
  SECTION7="$EOS_BASE/recreated-selected-skim-2023/$LABEL/section7_d_selected.root"
fi
if [[ -z "$OUTDIR" ]]; then
  OUTDIR="$EOS_BASE/cut-recreation-audits/$LABEL"
fi

if [[ ! -f "$SECTION5" ]]; then
  echo "Missing Section 5 input: $SECTION5" >&2
  exit 1
fi
if [[ ! -f "$SECTION7" ]]; then
  echo "Missing Section 7 input: $SECTION7" >&2
  exit 1
fi

mkdir -p "$OUTDIR"
cp "$BASE/CUT_RECREATION_PLAN.md" "$OUTDIR/CUT_RECREATION_PLAN.md"

root -l -b -q "$BASE/scripts/audit_2023_cut_recreation.C+(\"$SECTION5\",\"$SECTION7\",\"$OUTDIR\")"

LOG="$BASE/LOG.md"
{
  printf '\n## Cut Recreation Audit: %s\n\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf -- '- Label: `%s`\n' "$LABEL"
  printf -- '- Section 5 input: `%s`\n' "$SECTION5"
  printf -- '- Section 7 input: `%s`\n' "$SECTION7"
  printf -- '- Output directory: `%s`\n' "$OUTDIR"
  printf -- '- Scope: audit current nominal cuts and threshold scans; not final EBX/MC derivation.\n'
} >> "$LOG"

printf 'Cut recreation audit complete: %s\n' "$OUTDIR"
