#!/usr/bin/env bash
set -euo pipefail

BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
AREA_PARENT="${D0_REPRO_CMSSW_PARENT:-/afs/cern.ch/work/e/evzhang/codex-work/cmssw}"
RELEASE="${D0_REPRO_CMSSW132_RELEASE:-CMSSW_13_2_15_patch2}"
ARCH="${D0_REPRO_CMSSW132_ARCH:-el8_amd64_gcc11}"
SOURCE16="${D0_REPRO_FOREST_CMSSW_SRC:-/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src}"

mkdir -p "$BASE/logs" "$AREA_PARENT"
LOG="$BASE/logs/setup_cmssw132x_${RELEASE}_$(date -u +%Y%m%dT%H%M%SZ).log"

case "$AREA_PARENT" in
  /afs/cern.ch/user/e/evzhang*|/afs/cern.ch/work/e/evzhang*|/eos/user/e/evzhang*) ;;
  *)
    printf 'Refusing to write outside Evan-owned CERN paths: %s\n' "$AREA_PARENT" >&2
    exit 1
    ;;
esac

{
  printf 'started_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf 'release=%s\n' "$RELEASE"
  printf 'arch=%s\n' "$ARCH"
  printf 'area_parent=%s\n' "$AREA_PARENT"
  printf 'source16=%s\n' "$SOURCE16"

  if [[ ! -d "$AREA_PARENT/$RELEASE/src" ]]; then
    cmssw-el8 --command-to-run "source /cvmfs/cms.cern.ch/cmsset_default.sh; export SCRAM_ARCH=$ARCH; cd $AREA_PARENT; cmsrel $RELEASE"
  fi

  TARGET="$AREA_PARENT/$RELEASE/src"
  mkdir -p "$TARGET"
  for package in HIForestSetupPbPbRun2025 Bfinder HeavyIonsAnalysis; do
    printf 'sync_package=%s\n' "$package"
    rsync -a --exclude='.git' --exclude='tmp' --exclude='__pycache__' \
      "$SOURCE16/$package/" "$TARGET/$package/"
  done

  cmssw-el8 --command-to-run "source /cvmfs/cms.cern.ch/cmsset_default.sh; export SCRAM_ARCH=$ARCH; cd $TARGET; eval \$(scram runtime -sh); scram b -j 4"
  printf 'finished_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} 2>&1 | tee "$LOG"

printf 'cmssw_src=%s/%s/src\n' "$AREA_PARENT" "$RELEASE"
printf 'log=%s\n' "$LOG"
