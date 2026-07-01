#!/usr/bin/env bash
set -euo pipefail

BASE="${D0_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction}"
CMSSW_SRC="${D0_REPRO_FOREST_CMSSW_SRC:-/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src}"
TEMPLATE_CFG="${D0_REPRO_CRAB_TEMPLATE_CFG:-/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-forest-2023/analysis-settings-smoke/hiforward0_analysis_settings_smoke_20260618/forest_smoke_cfg.py}"
LABEL="d0_forest_crab_smoke_$(date -u +%Y%m%d_%H%M%S)"
STORAGE_SITE="T3_CH_CERNBOX"
OUT_LFN_BASE="/store/user/evzhang/codex-d0-crab-smoke"
COMPUTE_WHITELIST_CSV="T2_US_Vanderbilt,T2_CH_CERN"
MAX_EVENTS=100
SUBMIT=true
MINIMAL_ANALYSIS=false
CUSTOM_INPUTS=false

INPUT_FILES=(
  "/store/hidata/HIRun2023A/HIForward0/MINIAOD/16Jan2024-v1/2810000/3cb20bb5-a7b1-43a2-8025-9807f4f51e71.root"
  "/store/hidata/HIRun2023A/HIForward0/MINIAOD/16Jan2024-v1/2810000/0640e99d-84f5-49fd-a012-48be69252e99.root"
)

usage() {
  cat <<USAGE
Usage:
  run_2023_forest_crab_smoke.sh [--label NAME] [--storage-site SITE] [--out-lfn-base LFN]
                                [--compute-whitelist SITE[,SITE...]] [--max-events N]
                                [--input-file LFN_OR_PFN] [--input-list FILE]
                                [--minimal-analysis] [--no-submit]

Creates a tiny CRAB foresting smoke test using the current 2023 analysis-style
forest config. Splitting is FileBased with unitsPerJob=1, so N input files
produce N CRAB jobs. The default uses two files to verify that behavior.
For reliable bounded smokes, prefer --input-file pointing to a pre-skimmed
tiny MINIAOD file; prior FileBased jobs ignored the PSet maxEvents at runtime.

Compute defaults intentionally follow Evan's old 2026 foresting CRAB config:
whitelist T2_US_Vanderbilt and T2_CH_CERN, FileBased unitsPerJob=1, 2500 MB
memory, and long runtime allowance. Output intentionally does not inherit the
old group-store path; it writes through CERNBOX to Evan's EOS user area:
T3_CH_CERNBOX with LFN /store/user/evzhang/..., physically under /eos/user/e/evzhang/.
USAGE
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --label)
      LABEL="${2:?missing value for --label}"
      shift 2
      ;;
    --storage-site)
      STORAGE_SITE="${2:?missing value for --storage-site}"
      shift 2
      ;;
    --out-lfn-base)
      OUT_LFN_BASE="${2:?missing value for --out-lfn-base}"
      shift 2
      ;;
    --compute-whitelist)
      COMPUTE_WHITELIST_CSV="${2:?missing value for --compute-whitelist}"
      shift 2
      ;;
    --max-events)
      MAX_EVENTS="${2:?missing value for --max-events}"
      shift 2
      ;;
    --input-file)
      if [[ "$CUSTOM_INPUTS" == "false" ]]; then
        INPUT_FILES=()
        CUSTOM_INPUTS=true
      fi
      INPUT_FILES+=("${2:?missing value for --input-file}")
      shift 2
      ;;
    --input-list)
      if [[ "$CUSTOM_INPUTS" == "false" ]]; then
        INPUT_FILES=()
        CUSTOM_INPUTS=true
      fi
      while IFS= read -r input; do
        [[ -z "$input" || "$input" =~ ^[[:space:]]*# ]] && continue
        INPUT_FILES+=("$input")
      done < "${2:?missing value for --input-list}"
      shift 2
      ;;
    --minimal-analysis)
      MINIMAL_ANALYSIS=true
      shift
      ;;
    --no-submit)
      SUBMIT=false
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

case "$BASE" in
  /afs/cern.ch/user/e/evzhang*|/afs/cern.ch/work/e/evzhang*|/eos/user/e/evzhang*) ;;
  *)
    printf 'Refusing non-Evan-owned BASE: %s\n' "$BASE" >&2
    exit 1
    ;;
esac

case "$OUT_LFN_BASE" in
  /store/user/evzhang/*) ;;
  *)
    printf 'Refusing non-Evan user EOS LFN: %s\n' "$OUT_LFN_BASE" >&2
    exit 1
    ;;
esac

COMPUTE_WHITELIST_PY="["
IFS=',' read -ra COMPUTE_SITES <<< "$COMPUTE_WHITELIST_CSV"
for site in "${COMPUTE_SITES[@]}"; do
  site="${site#"${site%%[![:space:]]*}"}"
  site="${site%"${site##*[![:space:]]}"}"
  if [[ -z "$site" ]]; then
    continue
  fi
  case "$site" in
    T[0-9]_*|T[0-9][0-9]_*) ;;
    *)
      printf 'Refusing malformed CRAB compute site name: %s\n' "$site" >&2
      exit 1
      ;;
  esac
  if [[ "$COMPUTE_WHITELIST_PY" != "[" ]]; then
    COMPUTE_WHITELIST_PY+=", "
  fi
  COMPUTE_WHITELIST_PY+="\"$site\""
done
COMPUTE_WHITELIST_PY+="]"

if [[ "$COMPUTE_WHITELIST_PY" == "[]" ]]; then
  printf 'Refusing empty CRAB compute whitelist.\n' >&2
  exit 1
fi

if [[ "${#INPUT_FILES[@]}" -eq 0 ]]; then
  printf 'Refusing empty CRAB input file list.\n' >&2
  exit 1
fi

WORK_ROOT="$BASE/crab-smoke"
TASK_DIR="$WORK_ROOT/$LABEL"
mkdir -p "$TASK_DIR"

PSET="$TASK_DIR/forest_crab_cfg.py"
CRAB_CFG="$TASK_DIR/crabConfig.py"
MANIFEST="$TASK_DIR/manifest.md"
INST_LUMI="$TASK_DIR/inst_lumi.json"

cp "$TEMPLATE_CFG" "$PSET"
perl -0pi -e "s#INPUT_MAX_EVENTS\\s*=\\s*[-0-9]+#INPUT_MAX_EVENTS    = $MAX_EVENTS#; s#OUTPUT_FILE_NAME\\s*=\\s*\"[^\"]+\"#OUTPUT_FILE_NAME    = \"HiForest_2023PbPbUPC_Jan24Reco_smoke.root\"#" "$PSET"
if [[ "$MINIMAL_ANALYSIS" == "true" ]]; then
  perl -0pi -e "s#INCLUDE_MUONS\\s*=\\s*True#INCLUDE_MUONS       = False#; s#INCLUDE_PFJETS\\s*=\\s*True#INCLUDE_PFJETS      = False#" "$PSET"
  {
    printf '\n# Codex CRAB minimal-analysis smoke toggles.\n'
    printf 'process.zdcanalyzer.doZdcDigis = cms.bool(False)\n'
    printf 'process.zdcanalyzer.skipRpdDigis = cms.bool(True)\n'
  } >> "$PSET"
fi
printf '{}\n' > "$INST_LUMI"

{
  printf 'from CRABClient.UserUtilities import config\n\n'
  printf 'config = config()\n'
  printf 'config.General.requestName = "%s"\n' "$LABEL"
  printf 'config.General.workArea = "%s/crab-projects"\n' "$WORK_ROOT"
  printf 'config.General.transferOutputs = True\n'
  printf 'config.General.transferLogs = True\n\n'
  printf 'config.JobType.pluginName = "Analysis"\n'
  printf 'config.JobType.psetName = "%s"\n' "$PSET"
  printf 'config.JobType.outputFiles = ["HiForest_2023PbPbUPC_Jan24Reco_smoke.root"]\n'
  printf 'config.JobType.inputFiles = ["%s"]\n' "$INST_LUMI"
  printf 'config.JobType.allowUndistributedCMSSW = True\n'
  printf 'config.JobType.maxMemoryMB = 2500\n'
  printf 'config.JobType.maxJobRuntimeMin = 1250\n\n'
  printf 'config.Data.outputPrimaryDataset = "HIForward0D0ForestSmoke"\n'
  printf 'config.Data.userInputFiles = [\n'
  for input in "${INPUT_FILES[@]}"; do
    printf '    "%s",\n' "$input"
  done
  printf ']\n'
  printf 'config.Data.splitting = "FileBased"\n'
  printf 'config.Data.unitsPerJob = 1\n'
  printf 'config.Data.publication = False\n'
  printf 'config.Data.outputDatasetTag = "%s"\n' "$LABEL"
  printf 'config.Data.outLFNDirBase = "%s"\n' "$OUT_LFN_BASE"
  printf 'config.Data.ignoreLocality = True\n\n'
  printf 'config.Site.storageSite = "%s"\n' "$STORAGE_SITE"
  printf 'config.Site.whitelist = %s\n' "$COMPUTE_WHITELIST_PY"
} > "$CRAB_CFG"

{
  printf '# 2023 Forest CRAB Smoke: %s\n\n' "$LABEL"
  printf '<!-- DOC_OWNER: cms-analysis-manager CRAB smoke manifest. -->\n'
  printf '<!-- TOKEN_NOTE: tiny two-file CRAB task; full production still needs separate approval. -->\n\n'
  printf -- '- CMSSW source: `%s`\n' "$CMSSW_SRC"
  printf -- '- Template local-smoke config: `%s`\n' "$TEMPLATE_CFG"
  printf -- '- CRAB PSet: `%s`\n' "$PSET"
  printf -- '- CRAB config: `%s`\n' "$CRAB_CFG"
  printf -- '- Packaged lumi JSON: `%s`\n' "$INST_LUMI"
  printf -- '- Storage site: `%s`\n' "$STORAGE_SITE"
  printf -- '- Output LFN base: `%s`\n' "$OUT_LFN_BASE"
  printf -- '- Physical EOS target: `/eos/user/e/evzhang/...` via CERNBOX\n'
  printf -- '- Compute whitelist: `%s`\n' "$COMPUTE_WHITELIST_PY"
  printf -- '- Max CRAB memory/runtime: `2500 MB`, `1250 min`\n'
  printf -- '- Max events per CRAB job: `%s`\n' "$MAX_EVENTS"
  printf -- '- Minimal-analysis toggles: `%s`\n' "$MINIMAL_ANALYSIS"
  printf -- '- Splitting: `FileBased`, `unitsPerJob = 1`\n'
  printf -- '- Input files: `%s`\n\n' "${#INPUT_FILES[@]}"
  for input in "${INPUT_FILES[@]}"; do
    printf '  - `%s`\n' "$input"
  done
} > "$MANIFEST"

cd "$CMSSW_SRC"
eval "$(scramv1 runtime -sh)"

printf 'label=%s\n' "$LABEL"
printf 'task_dir=%s\n' "$TASK_DIR"
printf 'pset=%s\n' "$PSET"
printf 'crab_config=%s\n' "$CRAB_CFG"
printf 'manifest=%s\n' "$MANIFEST"
printf 'storage_site=%s\n' "$STORAGE_SITE"
printf 'out_lfn_base=%s\n' "$OUT_LFN_BASE"
printf 'compute_whitelist=%s\n' "$COMPUTE_WHITELIST_PY"
printf 'minimal_analysis=%s\n' "$MINIMAL_ANALYSIS"
printf 'input_files=%s\n' "${#INPUT_FILES[@]}"
printf 'splitting=FileBased unitsPerJob=1 expected_jobs=%s\n' "${#INPUT_FILES[@]}"

if [[ "$SUBMIT" == "true" ]]; then
  PROXY_PATH="$(voms-proxy-info -path 2>/dev/null || true)"
  if [[ -n "$PROXY_PATH" && -r "$PROXY_PATH" ]]; then
    crab submit --proxy "$PROXY_PATH" -c "$CRAB_CFG"
  else
    crab submit -c "$CRAB_CFG"
  fi
else
  printf 'submit=skipped\n'
fi
