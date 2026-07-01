#!/usr/bin/env bash
# Refresh public-safe LXPLUS execution and CMSSW snapshots.
set -euo pipefail

ARCHIVE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SSH_HOST="${LXPLUS_SSH_HOST:-lxplus-codex}"

rsync_code() {
  rsync -a --prune-empty-dirs "$@"
}

mkdir -p \
  "$ARCHIVE_ROOT/lxplus-execution/d0-reproduction" \
  "$ARCHIVE_ROOT/lxplus-execution/dplus-peak" \
  "$ARCHIVE_ROOT/cmssw-custom" \
  "$ARCHIVE_ROOT/cmssw-reference"

rsync_code \
  --include='*/' --include='*.sh' --include='*.py' --include='*.C' --include='*.h' \
  --include='*.cc' --include='*.cpp' --include='*.cfg' --include='*.yaml' --include='*.yml' \
  --exclude='*' \
  "$SSH_HOST:/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/" \
  "$ARCHIVE_ROOT/lxplus-execution/d0-reproduction/scripts/"

rsync_code \
  --include='*/' --include='*.py' --include='*.sh' --include='*.cc' --include='*.md' \
  --exclude='*' \
  "$SSH_HOST:/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/mc/" \
  "$ARCHIVE_ROOT/lxplus-execution/d0-reproduction/mc/"

rsync_code \
  --exclude='logs*/***' --exclude='*.txt' --exclude='*.json' \
  --include='*/' --include='*.py' --include='*.md' --exclude='*' \
  "$SSH_HOST:/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/crab-smoke/" \
  "$ARCHIVE_ROOT/lxplus-execution/d0-reproduction/crab-smoke/"

rsync_code \
  --exclude='logs/***' --exclude='crab-smoke/***' --exclude='scripts/***' --exclude='mc/***' \
  --include='*/' --include='*.md' --include='*.txt' --exclude='*' \
  "$SSH_HOST:/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/" \
  "$ARCHIVE_ROOT/lxplus-execution/d0-reproduction/"

rsync_code \
  --exclude='logs*/***' --exclude='*.txt' --exclude='*.json' \
  --include='*/' --include='*.sh' --include='*.py' --include='*.C' --include='*.h' \
  --include='*.cc' --include='*.cpp' --include='*.cfg' --include='*.md' --exclude='*' \
  "$SSH_HOST:/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak/" \
  "$ARCHIVE_ROOT/lxplus-execution/dplus-peak/"

rsync_code \
  --include='*/' --include='*.py' --include='*.cc' --include='*.h' --include='*.xml' \
  --include='*.txt' --include='*.md' --include='*.sh' --exclude='*' \
  "$SSH_HOST:/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src/EvanAnalysis/" \
  "$ARCHIVE_ROOT/cmssw-custom/EvanAnalysis/"

rsync -aR \
  "$SSH_HOST:/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src/./Bfinder/Bfinder/src/Dfinder.cc" \
  "$SSH_HOST:/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src/./HeavyIonsAnalysis/ZDCAnalysis/python/ZDCAnalyzersHC2023_cff.py" \
  "$SSH_HOST:/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src/./HeavyIonsAnalysis/ZDCAnalysis/src/ZDCHardCodeHelper.cc" \
  "$SSH_HOST:/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src/./HeavyIonsAnalysis/ZDCAnalysis/src/ZDCHardCodeHelper.h" \
  "$SSH_HOST:/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src/./HeavyIonsAnalysis/ZDCAnalysis/src/ZDCHardCodeHelper.cc.codex-pre-2023offline-20260618" \
  "$SSH_HOST:/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src/./HeavyIonsAnalysis/ZDCAnalysis/src/ZDCHardCodeHelper.h.codex-pre-2023offline-20260618" \
  "$SSH_HOST:/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src/./HiForestSetupPbPbRun2026/forest_CRABConfig_161X_2026PbPb_DATA_ZeroBias.py" \
  "$ARCHIVE_ROOT/cmssw-reference/"

cd "$ARCHIVE_ROOT"
python3 tools/build_usage_manifest.py
python3 tools/build_code_manifest.py
tools/public_safety_check.sh
