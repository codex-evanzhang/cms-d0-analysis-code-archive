#!/usr/bin/env bash
set -euo pipefail

BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
CMSSW_SRC="${D0_REPRO_CMSSW_SRC:-/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src}"
OUTDIR="${D0_REPRO_PYTHIA_OUTDIR:-/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/pythia-d0-smoke}"
STANDALONE_EVENTS="${STANDALONE_EVENTS:-1000}"
CMSSW_EVENTS="${CMSSW_EVENTS:-20}"
RUN_CMSSW="${RUN_CMSSW:-1}"

usage() {
  cat <<USAGE
Usage:
  run_pythia_d0_smoke.sh [--standalone-events N] [--cmssw-events N] [--outdir PATH] [--standalone-only]

Build and run a small Pythia8 D0 -> K pi smoke test using the CMSSW-provided
Pythia8 installation. Optionally also create and run a CMS GEN config with
cmsDriver.py.
USAGE
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --standalone-events)
      STANDALONE_EVENTS="${2:?missing value for --standalone-events}"
      shift 2
      ;;
    --cmssw-events)
      CMSSW_EVENTS="${2:?missing value for --cmssw-events}"
      shift 2
      ;;
    --outdir)
      OUTDIR="${2:?missing value for --outdir}"
      shift 2
      ;;
    --standalone-only)
      RUN_CMSSW=0
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

for path in "$BASE" "$CMSSW_SRC" "$OUTDIR"; do
  case "$path" in
    /afs/cern.ch/user/e/evzhang*|/afs/cern.ch/work/e/evzhang*|/eos/user/e/evzhang*)
      ;;
    *)
      printf 'Refusing to use path outside Evan-owned CERN roots: %s\n' "$path" >&2
      exit 1
      ;;
  esac
done

SRC_DIR="$BASE/mc/pythia"
BIN_DIR="$BASE/bin"
LOG_DIR="$BASE/logs"
PKG_DIR="$CMSSW_SRC/EvanAnalysis/D0PythiaMC"
FRAGMENT="$PKG_DIR/python/D0Kpi_5362GeV_TuneCP5_pythia8_cfi.py"

mkdir -p "$OUTDIR" "$BIN_DIR" "$LOG_DIR" "$PKG_DIR/python" "$PKG_DIR/test"
touch "$CMSSW_SRC/EvanAnalysis/__init__.py" "$PKG_DIR/__init__.py" "$PKG_DIR/python/__init__.py"

source /cvmfs/cms.cern.ch/cmsset_default.sh
cd "$CMSSW_SRC"
eval "$(scram runtime -sh)"
PYTHIA8_BASE="${PYTHIA8_BASE:-$(scram tool tag pythia8 PYTHIA8_BASE 2>/dev/null || true)}"
if [[ -z "$PYTHIA8_BASE" ]]; then
  printf 'Could not determine PYTHIA8_BASE from scram.\n' >&2
  exit 1
fi

LOG="$LOG_DIR/run_pythia_d0_smoke_$(date -u +%Y%m%dT%H%M%SZ).log"
MANIFEST="$OUTDIR/manifest.txt"

{
  printf 'started_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf 'host=%s\n' "$(hostname -f 2>/dev/null || hostname)"
  printf 'cmssw_src=%s\n' "$CMSSW_SRC"
  printf 'cmssw_version=%s\n' "${CMSSW_VERSION:-unknown}"
  printf 'outdir=%s\n' "$OUTDIR"
  printf 'standalone_events=%s\n' "$STANDALONE_EVENTS"
  printf 'cmssw_events=%s\n' "$CMSSW_EVENTS"
  printf 'run_cmssw=%s\n' "$RUN_CMSSW"
  scram tool info pythia8 | sed -n '1,40p'

  cp "$SRC_DIR/D0Kpi_5362GeV_TuneCP5_pythia8_cfi.py" "$FRAGMENT"
  cp "$SRC_DIR/BuildFile.xml" "$PKG_DIR/BuildFile.xml"
  cp "$SRC_DIR/standalone_pythia_d0_smoke.cc" "$PKG_DIR/test/standalone_pythia_d0_smoke.cc"

  c++ -O2 -std=c++17 \
    -I"$PYTHIA8_BASE/include" \
    "$PKG_DIR/test/standalone_pythia_d0_smoke.cc" \
    -L"$PYTHIA8_BASE/lib" -Wl,-rpath,"$PYTHIA8_BASE/lib" -lpythia8 \
    -o "$BIN_DIR/standalone_pythia_d0_smoke"

  "$BIN_DIR/standalone_pythia_d0_smoke" \
    "$STANDALONE_EVENTS" \
    "$OUTDIR/standalone_pythia_d0_summary.csv" \
    "$OUTDIR/pythia_commands.cmnd"

  if [[ "$RUN_CMSSW" == "1" ]]; then
    scram b -j 4
    eval "$(scram runtime -sh)"
    cmsDriver.py EvanAnalysis/D0PythiaMC/python/D0Kpi_5362GeV_TuneCP5_pythia8_cfi.py \
      --python_filename "$OUTDIR/d0_pythia_gen_cfg.py" \
      --fileout "file:$OUTDIR/d0_pythia_gen.root" \
      --eventcontent RAWSIM \
      --datatier GEN \
      --conditions auto:phase1_2024_realistic \
      --beamspot Realistic25ns13p6TeVEarly2023Collision \
      --step GEN \
      --geometry DB:Extended \
      --era Run3 \
      --no_exec \
      --mc \
      -n "$CMSSW_EVENTS"

    cmsRun "$OUTDIR/d0_pythia_gen_cfg.py"
    edmDumpEventContent "$OUTDIR/d0_pythia_gen.root" > "$OUTDIR/d0_pythia_gen_event_content.txt"
    edmFileUtil -P "file:$OUTDIR/d0_pythia_gen.root" > "$OUTDIR/d0_pythia_gen_file_util.txt" || true
  fi

  {
    printf 'result=pythia_d0_smoke\n'
    printf 'finished_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    printf 'cmssw_src=%s\n' "$CMSSW_SRC"
    printf 'cmssw_version=%s\n' "${CMSSW_VERSION:-unknown}"
    printf 'pythia8_base=%s\n' "$PYTHIA8_BASE"
    printf 'fragment=%s\n' "$FRAGMENT"
    printf 'standalone_summary=%s\n' "$OUTDIR/standalone_pythia_d0_summary.csv"
    printf 'pythia_commands=%s\n' "$OUTDIR/pythia_commands.cmnd"
    if [[ "$RUN_CMSSW" == "1" ]]; then
      printf 'cmssw_cfg=%s\n' "$OUTDIR/d0_pythia_gen_cfg.py"
      printf 'cmssw_output=%s\n' "$OUTDIR/d0_pythia_gen.root"
      printf 'event_content=%s\n' "$OUTDIR/d0_pythia_gen_event_content.txt"
    fi
  } > "$MANIFEST"

  printf 'finished_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf 'manifest=%s\n' "$MANIFEST"
} 2>&1 | tee "$LOG"

{
  printf '%s\t%s\t%s\t%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "pythia-d0-smoke" "d0-kpi-5362gev" "$OUTDIR"
} >> "$BASE/logs/action-log.tsv"

printf 'log=%s\n' "$LOG"
printf 'manifest=%s\n' "$MANIFEST"
