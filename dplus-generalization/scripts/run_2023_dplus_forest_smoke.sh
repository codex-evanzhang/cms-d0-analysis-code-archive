#!/usr/bin/env bash
set -euo pipefail

BASE="${DPLUS_REPRO_ROOT:-/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak}"
CMSSW_SRC="${DPLUS_REPRO_FOREST_CMSSW_SRC:-/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src}"
CONFIG_SRC="$CMSSW_SRC/HIForestSetupPbPbRun2025/forest_CMSSWConfig_Run3_132X_2023PbPb_Jan2024ReReco_UPC_MITHIG.py"
OUTDIR="${DPLUS_REPRO_FOREST_SMOKE_OUTDIR:-/eos/user/e/evzhang/codex-eos/outputs/dplus-peak/recreated-forest-2023/smoke}"
INPUT_FILE="${DPLUS_REPRO_FOREST_SMOKE_INPUT:-root://xrootd-vanderbilt.sites.opensciencegrid.org//store/hidata/HIRun2023A/HIForward0/MINIAOD/16Jan2024-v1/2810000/3cb20bb5-a7b1-43a2-8025-9807f4f51e71.root}"
MAX_EVENTS=100
LABEL="hiforward0_dplus_miniaod_100"
ZDC_MODE="hc2023"
ZDC_VARIANT="default"
GLOBAL_TAG_OVERRIDE=""
DISABLE_MUONS=false
DISABLE_SUPERFILTER=false
DISABLE_ZDC_DIGIS=false
MINIMAL_ANALYSIS=false

usage() {
  cat <<USAGE
Usage:
  run_2023_dplus_forest_smoke.sh [--max-events N] [--input-file ROOT_OR_XROOTD_PATH] [--label NAME] [--outdir PATH]
                           [--global-tag TAG] [--zdc-variant NAME] [--no-muons] [--no-zdc-digis] [--minimal-analysis] [--no-superfilter]

Runs a bounded cmsRun foresting smoke test for the 2023 PbPb UPC Jan 2024
ReReco configuration with Dfinder configured for D+/D- -> Kpipi. It writes only
under Evan-owned CERN paths. It does not submit CRAB or run full production.

Use --no-muons for detector-only MC checks where hltMuTree/GEM geometry is not
needed. Use --global-tag to test an official MC input with its campaign tag.
Use --no-superfilter when measuring raw detector distributions; otherwise the
2023 UPC HLT/PV/ZDC filters are prepended to every path.
Use --minimal-analysis to write a pruned forest containing only the trees and
branches read by the current 2023 D+ Kpipi peak-search validation.

ZDC variants are small reconstruction-parameter tests:
  default, pbpb-uncorrected, pbpb-producer-rechit, force-soi1, force-soi2, force-soi3,
  pbpb-force-soi1, pbpb-force-soi2, pbpb-force-soi3, keep-zs
USAGE
}

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --max-events)
      MAX_EVENTS="${2:?missing value for --max-events}"
      shift 2
      ;;
    --input-file)
      INPUT_FILE="${2:?missing value for --input-file}"
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
    --global-tag)
      GLOBAL_TAG_OVERRIDE="${2:?missing value for --global-tag}"
      shift 2
      ;;
    --zdc-variant)
      ZDC_VARIANT="${2:?missing value for --zdc-variant}"
      shift 2
      ;;
    --no-muons)
      DISABLE_MUONS=true
      shift
      ;;
    --no-zdc-digis)
      DISABLE_ZDC_DIGIS=true
      shift
      ;;
    --minimal-analysis)
      MINIMAL_ANALYSIS=true
      DISABLE_MUONS=true
      DISABLE_ZDC_DIGIS=true
      shift
      ;;
    --no-superfilter)
      DISABLE_SUPERFILTER=true
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

case "$OUTDIR" in
  /afs/cern.ch/user/e/evzhang*|/afs/cern.ch/work/e/evzhang*|/eos/user/e/evzhang*)
    ;;
  *)
    printf 'Refusing to write outside Evan-owned CERN paths: %s\n' "$OUTDIR" >&2
    exit 1
    ;;
esac

RUN_DIR="$OUTDIR/$LABEL"
mkdir -p "$BASE/logs" "$BASE/manifests" "$RUN_DIR"
printf '{}\n' > "$RUN_DIR/inst_lumi.json"

RUN_CFG="$RUN_DIR/forest_smoke_cfg.py"
OUTPUT_ROOT="$RUN_DIR/HiForest_2023PbPbUPC_Jan24Reco_smoke.root"
CMSRUN_OUTPUT_ROOT="$OUTPUT_ROOT"
RAW_OUTPUT_ROOT=""
if [[ "$MINIMAL_ANALYSIS" == "true" ]]; then
  RAW_OUTPUT_ROOT="$RUN_DIR/HiForest_2023PbPbUPC_Jan24Reco_smoke_raw.root"
  CMSRUN_OUTPUT_ROOT="$RAW_OUTPUT_ROOT"
fi
LOG="$BASE/logs/run_2023_forest_smoke_${LABEL}_$(date -u +%Y%m%dT%H%M%SZ).log"
MANIFEST="$BASE/manifests/recreated_forest_smoke_2023_${LABEL}.md"

cp "$CONFIG_SRC" "$RUN_CFG"
perl -0pi -e "s#INPUT_TEST_FILE = .*#INPUT_TEST_FILE = \"$INPUT_FILE\"#; s#INPUT_MAX_EVENTS\\s*=\\s*[-0-9]+#INPUT_MAX_EVENTS    = $MAX_EVENTS#; s#OUTPUT_FILE_NAME\\s*=\\s*\"[^\"]+\"#OUTPUT_FILE_NAME    = \"$CMSRUN_OUTPUT_ROOT\"#; s#INCLUDE_PFJETS\\s*=\\s*True#INCLUDE_PFJETS      = False#" "$RUN_CFG"
cat >> "$RUN_CFG" <<'PYCFG'

# Codex D+ peak-search mode: enable D+/D- -> Kpipi and disable D0 Kpi channels.
process.Dfinder.Dchannel = cms.vint32(
    0, 0,  # K+pi-, K-pi+
    1, 1,  # K-pi+pi+, K+pi-pi-
    0, 0,  # K-pi-pi+pi+, K+pi+pi-pi-
    0, 0,  # Ds+/- -> KKpi
    0, 0,  # D*+/- -> D0(Kpi)pi
    0, 0,  # D*+/- -> D0(K3pi)pi
    0, 0,  # B+/- -> D0 pi
    0, 0,  # LambdaC+/- -> pKpi
    0, 0,  # LambdaC+/- -> pKs
)
PYCFG
if [[ -n "$GLOBAL_TAG_OVERRIDE" ]]; then
  perl -0pi -e "s#GLOBAL_TAG\\s*=\\s*\"[^\"]+\"#GLOBAL_TAG = \"$GLOBAL_TAG_OVERRIDE\"#" "$RUN_CFG"
fi
if [[ "$DISABLE_MUONS" == "true" ]]; then
  perl -0pi -e "s#INCLUDE_MUONS\\s*=\\s*True#INCLUDE_MUONS       = False#" "$RUN_CFG"
fi
if [[ "$DISABLE_SUPERFILTER" == "true" ]]; then
  perl -0pi -e "s#process\\.superFilterPath = cms\\.Path\\(process\\.filterSequence\\)\\nprocess\\.skimanalysis\\.superFilters = cms\\.vstring\\('superFilterPath'\\)\\nfor path in process\\.paths:\\n   getattr\\(process, path\\)\\._seq = process\\.filterSequence \\* getattr\\(process,path\\)\\._seq#process.skimanalysis.superFilters = cms.vstring()#s" "$RUN_CFG"
fi

# Prefer the restored 2023 ZDC wrapper. If it is absent on a migrated machine,
# fall back to the newer Run 3 PbPb wrapper so the smoke still runs, but record
# that the event selection is no longer an exact official-2023 reproduction.
if [[ -f "$CMSSW_SRC/HeavyIonsAnalysis/ZDCAnalysis/python/ZDCAnalyzersHC2023_cff.py" ]]; then
  perl -0pi -e 's#(process\.load\('\''HeavyIonsAnalysis\.ZDCAnalysis\.HiZDCfilter_cfi'\''\)\n)#$1for _zdc_filter in [process.zdcEnergyFilter1nOr, process.zdcEnergyFilter0nOr, process.zdcEnergyFilter0nAnd, process.zdcEnergyFilterXOr]:\n    _zdc_filter.ZDCRecHitSource = cms.InputTag('\''zdcreco2023HardCode'\'')\n#' "$RUN_CFG"
else
  ZDC_MODE="run3-pbpb-fallback"
  perl -0pi -e "s#HeavyIonsAnalysis\\.ZDCAnalysis\\.ZDCAnalyzersHC2023_cff#HeavyIonsAnalysis.ZDCAnalysis.ZDCAnalyzersPbPb_cff#g; s#process\\.zdcSequence\\b#process.zdcSequencePbPb#g; s#process\\.zdcreco2023HardCode\\b#process.zdcrecoRun3#g" "$RUN_CFG"
fi
perl -0pi -e 's#(process\.load\('\''Configuration\.StandardSequences\.MagneticField_38T_cff'\''\)\n)#$1process.load('\''RecoLocalCalo.HcalRecAlgos.hcalRecAlgoESProd_cfi'\'')\n#' "$RUN_CFG"

{
  printf '\n# Codex ZDC reconstruction variant: %s\n' "$ZDC_VARIANT"
  case "$ZDC_VARIANT" in
    default)
      ;;
    pbpb-uncorrected)
      printf 'process.zdcreco2023HardCode.correctionMethodEM = cms.int32(0)\n'
      printf 'process.zdcreco2023HardCode.correctionMethodHAD = cms.int32(0)\n'
      printf 'process.zdcreco2023HardCode.ootpuRatioEM = cms.double(-1.0)\n'
      printf 'process.zdcreco2023HardCode.ootpuRatioHAD = cms.double(-1.0)\n'
      printf 'process.zdcreco2023HardCode.ootpuFracEM = cms.double(1.0)\n'
      printf 'process.zdcreco2023HardCode.ootpuFracHAD = cms.double(1.0)\n'
      ;;
    pbpb-producer-rechit)
      printf 'process.zdcreco2023HardCode.correctionMethodEM = cms.int32(0)\n'
      printf 'process.zdcreco2023HardCode.correctionMethodHAD = cms.int32(0)\n'
      printf 'process.zdcreco2023HardCode.ootpuRatioEM = cms.double(-1.0)\n'
      printf 'process.zdcreco2023HardCode.ootpuRatioHAD = cms.double(-1.0)\n'
      printf 'process.zdcreco2023HardCode.ootpuFracEM = cms.double(1.0)\n'
      printf 'process.zdcreco2023HardCode.ootpuFracHAD = cms.double(1.0)\n'
      printf 'process.zdcanalyzer.use2023OfflineHardcodedSums = cms.bool(False)\n'
      ;;
    force-soi1|force-soi2|force-soi3)
      soi="${ZDC_VARIANT#force-soi}"
      noise=$((soi - 1))
      printf 'process.zdcreco2023HardCode.forceSOI = cms.bool(True)\n'
      printf 'process.zdcreco2023HardCode.signalSOI = cms.vuint32(%s)\n' "$soi"
      printf 'process.zdcreco2023HardCode.noiseSOI = cms.vuint32(%s)\n' "$noise"
      ;;
    pbpb-force-soi1|pbpb-force-soi2|pbpb-force-soi3)
      soi="${ZDC_VARIANT#pbpb-force-soi}"
      noise=$((soi - 1))
      printf 'process.zdcreco2023HardCode.correctionMethodEM = cms.int32(0)\n'
      printf 'process.zdcreco2023HardCode.correctionMethodHAD = cms.int32(0)\n'
      printf 'process.zdcreco2023HardCode.ootpuRatioEM = cms.double(-1.0)\n'
      printf 'process.zdcreco2023HardCode.ootpuRatioHAD = cms.double(-1.0)\n'
      printf 'process.zdcreco2023HardCode.ootpuFracEM = cms.double(1.0)\n'
      printf 'process.zdcreco2023HardCode.ootpuFracHAD = cms.double(1.0)\n'
      printf 'process.zdcreco2023HardCode.forceSOI = cms.bool(True)\n'
      printf 'process.zdcreco2023HardCode.signalSOI = cms.vuint32(%s)\n' "$soi"
      printf 'process.zdcreco2023HardCode.noiseSOI = cms.vuint32(%s)\n' "$noise"
      ;;
    keep-zs)
      printf 'process.zdcreco2023HardCode.dropZSmarkedPassed = cms.bool(False)\n'
      ;;
    *)
      printf 'Unknown --zdc-variant: %s\n' "$ZDC_VARIANT" >&2
      exit 2
      ;;
  esac
} >> "$RUN_CFG"

if [[ "$DISABLE_ZDC_DIGIS" == "true" ]]; then
  {
    printf '\n# Codex analysis-minimal ZDC output: keep rechit sums, drop digi waveform tree.\n'
    printf 'process.zdcanalyzer.doZdcDigis = cms.bool(False)\n'
    printf 'process.zdcanalyzer.skipRpdDigis = cms.bool(True)\n'
  } >> "$RUN_CFG"
fi

{
  printf 'started_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf 'host=%s\n' "$(hostname -f 2>/dev/null || hostname)"
  printf 'cmssw_src=%s\n' "$CMSSW_SRC"
  printf 'config_src=%s\n' "$CONFIG_SRC"
  printf 'run_cfg=%s\n' "$RUN_CFG"
  printf 'zdc_mode=%s\n' "$ZDC_MODE"
  printf 'global_tag_override=%s\n' "${GLOBAL_TAG_OVERRIDE:-none}"
  printf 'zdc_variant=%s\n' "$ZDC_VARIANT"
  printf 'disable_muons=%s\n' "$DISABLE_MUONS"
  printf 'disable_zdc_digis=%s\n' "$DISABLE_ZDC_DIGIS"
  printf 'minimal_analysis=%s\n' "$MINIMAL_ANALYSIS"
  printf 'disable_superfilter=%s\n' "$DISABLE_SUPERFILTER"
  printf 'input_file=%s\n' "$INPUT_FILE"
  printf 'max_events=%s\n' "$MAX_EVENTS"
  printf 'output_root=%s\n' "$OUTPUT_ROOT"
  if [[ -n "$RAW_OUTPUT_ROOT" ]]; then
    printf 'raw_output_root=%s\n' "$RAW_OUTPUT_ROOT"
  fi
  cd "$CMSSW_SRC"
  eval "$(scram runtime -sh)"
  cd "$RUN_DIR"
  cmsRun "$RUN_CFG"
  if [[ "$MINIMAL_ANALYSIS" == "true" ]]; then
    MINIMIZER="$BASE/scripts/minimize_2023_dplus_forest.C"
    if [[ ! -f "$MINIMIZER" ]]; then
      printf 'Missing minimizer macro: %s\n' "$MINIMIZER" >&2
      exit 1
    fi
    root -l -b -q "$MINIMIZER(\"$RAW_OUTPUT_ROOT\",\"$OUTPUT_ROOT\")"
  fi
  printf 'finished_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} 2>&1 | tee "$LOG"

{
  printf '# 2023 D+ Forest Smoke Reproduction: %s\n\n' "$LABEL"
  printf '<!-- DOC_OWNER: cms-analysis-manager D+ forest smoke manifest. -->\n'
  printf '<!-- TOKEN_NOTE: bounded cmsRun test only; full CRAB production needs approval. -->\n\n'
  printf '## Inputs\n\n'
  printf -- '- CMSSW source: `%s`\n' "$CMSSW_SRC"
  printf -- '- Base config: `%s`\n' "$CONFIG_SRC"
  printf -- '- Generated config: `%s`\n' "$RUN_CFG"
  printf -- '- ZDC mode: `%s`\n' "$ZDC_MODE"
  printf -- '- ZDC variant: `%s`\n' "$ZDC_VARIANT"
  printf -- '- Global tag override: `%s`\n' "${GLOBAL_TAG_OVERRIDE:-none}"
  printf -- '- Muons disabled: `%s`\n' "$DISABLE_MUONS"
  printf -- '- ZDC digi tree disabled: `%s`\n' "$DISABLE_ZDC_DIGIS"
  printf -- '- Analysis-minimal output: `%s`\n' "$MINIMAL_ANALYSIS"
  printf -- '- Superfilter disabled: `%s`\n' "$DISABLE_SUPERFILTER"
  printf -- '- MINIAOD input: `%s`\n' "$INPUT_FILE"
  printf -- '- Max events: `%s`\n\n' "$MAX_EVENTS"
  printf '## Outputs\n\n'
  printf -- '- Smoke forest: `%s`\n' "$OUTPUT_ROOT"
  if [[ -n "$RAW_OUTPUT_ROOT" ]]; then
    printf -- '- Raw pre-pruning forest: `%s`\n' "$RAW_OUTPUT_ROOT"
  fi
  printf -- '- Log: `%s`\n\n' "$LOG"
  printf '## Scope\n\n'
  printf -- '- This validates that the forest + embedded Dfinder D+/D- Kpipi configuration can run from MINIAOD.\n'
  printf -- '- It is not full production and is not a replacement for CRAB over HIForward0-19.\n'
} > "$MANIFEST"

{
  printf '%s\t%s\t%s\t%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "dplus-forest-smoke" "$LABEL" "$OUTPUT_ROOT"
} >> "$BASE/logs/action-log.tsv"

printf 'manifest=%s\n' "$MANIFEST"
printf 'log=%s\n' "$LOG"
printf 'output_root=%s\n' "$OUTPUT_ROOT"
