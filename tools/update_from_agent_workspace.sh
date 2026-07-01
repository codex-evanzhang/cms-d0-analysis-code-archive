#!/usr/bin/env bash
# Refresh this public code archive from the agent-network workspace.
set -euo pipefail

ARCHIVE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
AGENT_ROOT="${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}"

sync_dir() {
  local src="$1"
  local dst="$2"
  mkdir -p "$dst"
  rsync -a --delete \
    --exclude '__pycache__/' \
    --exclude '*.pyc' \
    --exclude '*.root' \
    --exclude '*.png' \
    --exclude '*.pdf' \
    "$src"/ "$dst"/
}

copy_file() {
  local src="$1"
  local dst="$2"
  mkdir -p "$(dirname "$dst")"
  cp "$src" "$dst"
}

cd "$AGENT_ROOT"

sync_dir "research/cms-d0-analysis/scripts" "$ARCHIVE_ROOT/d0-reproduction/scripts"
sync_dir "research/cms-d0-analysis/config" "$ARCHIVE_ROOT/d0-reproduction/config"
sync_dir "research/cms-dplus-analysis/scripts" "$ARCHIVE_ROOT/dplus-generalization/scripts"
sync_dir "work/cms-official-mc/python" "$ARCHIVE_ROOT/official-mc-fragments/python"
sync_dir "work/cms-official-mc/raw" "$ARCHIVE_ROOT/official-mc-fragments/raw"
sync_dir "repos/general-codex-output/support/slides/scripts" "$ARCHIVE_ROOT/reporting/scripts"

for name in \
  README.md COMMANDS.md RUNS.md CUT_DERIVATION.md DATASETS.md MC_OFFICIAL_MODEL.md \
  PYTHIA_MC.md PLOTS.md CHECKLISTS.md ANALYSIS_ROADMAP.md CMS_ANALYSIS_PLAN.md
do
  copy_file "research/cms-d0-analysis/$name" "$ARCHIVE_ROOT/d0-reproduction/docs/$name"
done
copy_file "research/cms-d0-analysis/registry.yaml" "$ARCHIVE_ROOT/d0-reproduction/docs/registry.yaml"

copy_file "research/cms-dplus-analysis/README.md" "$ARCHIVE_ROOT/dplus-generalization/docs/README.md"
copy_file "research/cms-dplus-analysis/RUNS.md" "$ARCHIVE_ROOT/dplus-generalization/docs/RUNS.md"

for name in d0_reproduction_workflow.tex d0_recreated_analysis_note.tex dplus_mass_peak_search.tex main.tex
do
  copy_file "repos/general-codex-output/$name" "$ARCHIVE_ROOT/reporting/tex/$name"
done

for name in lxplusctl cms-analysisctl cms-official-mcctl sync-overleaf-from-github.sh project-sync.sh project-status.sh check-tool-registry.sh list-tools.sh
do
  copy_file "scripts/$name" "$ARCHIVE_ROOT/orchestration/$name"
done

cd "$ARCHIVE_ROOT"
python3 tools/build_usage_manifest.py
python3 tools/build_code_manifest.py
tools/public_safety_check.sh
