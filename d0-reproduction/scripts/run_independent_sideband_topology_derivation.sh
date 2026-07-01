#!/usr/bin/env bash
set -euo pipefail

ROOT="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
REMOTE_SCRIPT_DIR="/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts"
REMOTE_OUT_BASE="/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/cut-derivation"
LOCAL_OUT_BASE="$ROOT/research/cms-d0-analysis/output/cut-derivation"

SIGNAL_MC="/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root"
DATA_2023="/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root"
EMPTYBX="/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root"

MAX_EVENTS="-1"
LABEL="20260618"

usage() {
  cat <<USAGE
Usage: $0 [--max-events N] [--label LABEL]

Derives topology sidebands from MC resolution, compares to the AN sideband,
then reruns topology scans and independent floor/pointing derivations using the
derived sideband.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --max-events)
      MAX_EVENTS="${2:?missing value for --max-events}"
      shift 2
      ;;
    --label)
      LABEL="${2:?missing value for --label}"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      usage >&2
      exit 2
      ;;
  esac
done

cd "$ROOT"

SIDE_REMOTE="$REMOTE_OUT_BASE/topology_sideband_derivation_${LABEL}"
DATA_SCAN_REMOTE="$REMOTE_OUT_BASE/topocuts_rect_independent_sideband_full_${LABEL}"
MC_SCAN_REMOTE="$REMOTE_OUT_BASE/topocuts_mc_background_independent_sideband_full_${LABEL}"

SIDE_LOCAL="$LOCAL_OUT_BASE/topology_sideband_derivation_${LABEL}"
DATA_SCAN_LOCAL="$LOCAL_OUT_BASE/topocuts_rect_independent_sideband_full_${LABEL}"
MC_SCAN_LOCAL="$LOCAL_OUT_BASE/topocuts_mc_background_independent_sideband_full_${LABEL}"
FLOOR_LOCAL="$LOCAL_OUT_BASE/topological_floor_independent_sideband_${LABEL}"
POINTING_LOCAL="$LOCAL_OUT_BASE/topological_pointing_independent_sideband_${LABEL}"

mkdir -p "$SIDE_LOCAL" "$DATA_SCAN_LOCAL" "$MC_SCAN_LOCAL" "$FLOOR_LOCAL" "$POINTING_LOCAL"

scripts/lxplusctl ssh "mkdir -p '$REMOTE_SCRIPT_DIR' '$SIDE_REMOTE' '$DATA_SCAN_REMOTE' '$MC_SCAN_REMOTE'"
scp \
  research/cms-d0-analysis/scripts/derive_d0_topology_sidebands.C \
  research/cms-d0-analysis/scripts/derive_2023_d0_topology_cuts_an_sideband.C \
  research/cms-d0-analysis/scripts/derive_2023_d0_topology_cuts_mc_background.C \
  lxplus-codex:"$REMOTE_SCRIPT_DIR/"

scripts/lxplusctl ssh \
  "set -e; source /cvmfs/cms.cern.ch/cmsset_default.sh; cd '$REMOTE_SCRIPT_DIR'; root -l -b -q 'derive_d0_topology_sidebands.C++(\"$SIGNAL_MC\",\"$DATA_2023\",\"$SIDE_REMOTE\",$MAX_EVENTS)'"

scp \
  lxplus-codex:"$SIDE_REMOTE/README.md" \
  lxplus-codex:"$SIDE_REMOTE/sideband_resolution.tsv" \
  lxplus-codex:"$SIDE_REMOTE/recommended_sidebands.tsv" \
  lxplus-codex:"$SIDE_REMOTE/sideband_stability_scan.tsv" \
  "$SIDE_LOCAL/"

.venv-plotting/bin/python \
  research/cms-d0-analysis/scripts/summarize_topology_sideband_derivation.py \
  "$SIDE_LOCAL"

read -r SIDEBAND_INNER SIDEBAND_OUTER < <(awk -F '\t' '$1=="global_resolution_rule"{print $4, $5}' "$SIDE_LOCAL/recommended_sidebands.tsv")

scripts/lxplusctl ssh \
  "set -e; source /cvmfs/cms.cern.ch/cmsset_default.sh; cd '$REMOTE_SCRIPT_DIR'; root -l -b -q 'derive_2023_d0_topology_cuts_an_sideband.C++(\"$SIGNAL_MC\",\"$DATA_2023\",\"$EMPTYBX\",\"$DATA_SCAN_REMOTE\",$MAX_EVENTS,$SIDEBAND_INNER,$SIDEBAND_OUTER)'"

scripts/lxplusctl ssh \
  "set -e; source /cvmfs/cms.cern.ch/cmsset_default.sh; cd '$REMOTE_SCRIPT_DIR'; root -l -b -q 'derive_2023_d0_topology_cuts_mc_background.C++(\"$SIGNAL_MC\",\"unused\",\"unused\",\"$MC_SCAN_REMOTE\",$MAX_EVENTS,$SIDEBAND_INNER,$SIDEBAND_OUTER)'"

scp \
  lxplus-codex:"$DATA_SCAN_REMOTE/README.md" \
  lxplus-codex:"$DATA_SCAN_REMOTE/cut_rationale.md" \
  lxplus-codex:"$DATA_SCAN_REMOTE/d_topology_scan.tsv" \
  lxplus-codex:"$DATA_SCAN_REMOTE/recommended_d_topological_cuts.tsv" \
  lxplus-codex:"$DATA_SCAN_REMOTE/recommended_event_thresholds.tsv" \
  lxplus-codex:"$DATA_SCAN_REMOTE/zdc_threshold_scan.tsv" \
  lxplus-codex:"$DATA_SCAN_REMOTE/hf_gap_threshold_scan.tsv" \
  "$DATA_SCAN_LOCAL/"

scp \
  lxplus-codex:"$MC_SCAN_REMOTE/README.md" \
  lxplus-codex:"$MC_SCAN_REMOTE/cut_rationale.md" \
  lxplus-codex:"$MC_SCAN_REMOTE/d_topology_scan.tsv" \
  lxplus-codex:"$MC_SCAN_REMOTE/recommended_d_topological_cuts.tsv" \
  "$MC_SCAN_LOCAL/"

.venv-plotting/bin/python \
  research/cms-d0-analysis/scripts/summarize_topological_cut_scan.py \
  "$DATA_SCAN_LOCAL" \
  --background-label 'data candidates in independently derived mass sidebands' \
  --background-axis-label 'independent-sideband data fraction passing'

.venv-plotting/bin/python \
  research/cms-d0-analysis/scripts/summarize_topological_cut_scan.py \
  "$MC_SCAN_LOCAL" \
  --background-label 'non-truth-matched MC candidates in independently derived sidebands' \
  --background-axis-label 'independent-sideband nonmatched MC fraction passing'

.venv-plotting/bin/python \
  research/cms-d0-analysis/scripts/derive_topological_floor_justification.py \
  --data-sideband-scan "$DATA_SCAN_LOCAL/d_topology_scan.tsv" \
  --mc-background-scan "$MC_SCAN_LOCAL/d_topology_scan.tsv" \
  --outdir "$FLOOR_LOCAL"

.venv-plotting/bin/python \
  research/cms-d0-analysis/scripts/derive_topological_pointing_justification.py \
  --data-sideband-scan "$DATA_SCAN_LOCAL/d_topology_scan.tsv" \
  --mc-background-scan "$MC_SCAN_LOCAL/d_topology_scan.tsv" \
  --floor-derivation "$FLOOR_LOCAL/floor_derivation.tsv" \
  --outdir "$POINTING_LOCAL"

cat > "$LOCAL_OUT_BASE/topology_independent_sideband_${LABEL}_manifest.md" <<MANIFEST
# Independent-Sideband Topology Derivation

Label: \`$LABEL\`

- Derived sideband output: \`$SIDE_LOCAL\`
- Data-sideband topology scan: \`$DATA_SCAN_LOCAL\`
- MC-background topology scan: \`$MC_SCAN_LOCAL\`
- Floor derivation: \`$FLOOR_LOCAL\`
- Pointing derivation: \`$POINTING_LOCAL\`
- Derived sideband: \`$SIDEBAND_INNER < |Dmass - 1.86484| < $SIDEBAND_OUTER\` GeV

The AN sideband is only a posthoc comparison target in this workflow.
MANIFEST

echo "Derived sideband: $SIDEBAND_INNER < |Dmass - 1.86484| < $SIDEBAND_OUTER GeV"
echo "Manifest: research/cms-d0-analysis/output/cut-derivation/topology_independent_sideband_${LABEL}_manifest.md"
