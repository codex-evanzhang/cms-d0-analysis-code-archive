#!/usr/bin/env python3
"""Assemble an AN-shaped D0 reproduction draft from current recreated outputs.

This script deliberately uses the original AN only as a section/target scaffold.
It does not copy AN prose. Each section is filled from reproduced artifacts when
available and marked partial/missing when the current reproduction has not yet
produced the corresponding tables or plots.
"""

from __future__ import annotations

import argparse
import csv
import json
import re
import shutil
import subprocess
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path


WORKSPACE = Path(__file__).resolve().parents[1]
AGENT_ROOT = WORKSPACE.parents[1]
CMS_OUTPUT = WORKSPACE / "output"
GENERAL = AGENT_ROOT / "repos/general-codex-output"
AN_TEXT = AGENT_ROOT / "private/cms-analysis/extracted/D0_production_in_UPC_PbPb_in_0nXn_events_with_rapidity_gap_AN.txt"
AN_PDF = AGENT_ROOT / "private/cms-analysis/analysis-notes/D0_production_in_UPC_PbPb_in_0nXn_events_with_rapidity_gap_AN.pdf"


# Primary artifact registry for this generated draft.
#
# Keep these paths pointed at durable CMS output directories, not at
# repos/general-codex-output, because that repository is intentionally
# overwritten whenever a new report is promoted to Overleaf.
FIGURE_SOURCES = {
    "figures/preflight_medium_scale/zdc_plus_1n_threshold.png": CMS_OUTPUT / "zdc-1n-thresholds/zerobias_forest_emptybx_forest_5m_20260617/zdc_plus_1n_threshold.png",
    "figures/preflight_medium_scale/zdc_minus_1n_threshold.png": CMS_OUTPUT / "zdc-1n-thresholds/zerobias_forest_emptybx_forest_5m_20260617/zdc_minus_1n_threshold.png",
    "figures/hf_gap_blind/hf_plus_threshold_efficiencies.png": CMS_OUTPUT / "cut-derivation/hf_gap_data_mc_full_blind_20260618/hf_plus_threshold_efficiencies.png",
    "figures/hf_gap_blind/hf_minus_threshold_efficiencies.png": CMS_OUTPUT / "cut-derivation/hf_gap_data_mc_full_blind_20260618/hf_minus_threshold_efficiencies.png",
    "figures/hf_gap_blind/hf_plus_gap_distributions.png": CMS_OUTPUT / "cut-derivation/hf_gap_data_mc_full_blind_20260618/hf_plus_gap_distributions.png",
    "figures/hf_gap_blind/hf_minus_gap_distributions.png": CMS_OUTPUT / "cut-derivation/hf_gap_data_mc_full_blind_20260618/hf_minus_gap_distributions.png",
    "figures/preflight_medium_scale/zdc_scale_scatter.png": CMS_OUTPUT / "preflight-medium-scale-20260618/zdc-scale-closure/zdc_scale_scatter.png",
    "figures/miniaod_chain/d0_mass_produced_skim_selected.png": CMS_OUTPUT / "skim-to-peak/crab_98forest_apriori_20260619/peak/d0_mass_produced_skim_selected.png",
    "figures/miniaod_chain/d0_mass_produced_skim_pt_y_grid.png": CMS_OUTPUT / "skim-to-peak/crab_98forest_apriori_20260619/peak/d0_mass_produced_skim_pt_y_grid.png",
    "figures/topology_sideband_independent/sideband_resolution_rule.png": CMS_OUTPUT / "cut-derivation/topology_sideband_derivation_20260622_gap_push/sideband_plots/sideband_resolution_rule.png",
    "figures/topological_floor_independent_sideband/floor_profile_grid.png": CMS_OUTPUT / "cut-derivation/topological_floor_independent_sideband_20260622_gap_push/floor_profiles/floor_profile_grid.png",
    "figures/topological_floor_independent_sideband/chi2_floor_profile_grid.png": CMS_OUTPUT / "cut-derivation/topological_floor_independent_sideband_20260622_gap_push/floor_profiles/chi2_floor_profile_grid.png",
    "figures/topological_pointing_independent_sideband/pointing_plateau_grid.png": CMS_OUTPUT / "cut-derivation/topological_pointing_independent_sideband_20260622_gap_push/pointing_plots/pointing_plateau_grid.png",
    "figures/topology_rect_independent_sideband/topology_eff_vs_sideband_grid.png": CMS_OUTPUT / "cut-derivation/topocuts_rect_independent_sideband_full_20260622_gap_push/topology_plots/topology_eff_vs_sideband_grid.png",
    "figures/topology_mc_background_independent_sideband/topology_eff_vs_sideband_grid.png": CMS_OUTPUT / "cut-derivation/topocuts_mc_background_independent_sideband_full_20260622_gap_push/topology_plots/topology_eff_vs_sideband_grid.png",
    "figures/topological_pointing_closure/corrected_yield_ratio_grid.png": CMS_OUTPUT / "cut-derivation/topological_pointing_closure_20260618/closure_plots/corrected_yield_ratio_grid.png",
    "figures/topological_pointing_closure/mass_overlay_grid.png": CMS_OUTPUT / "cut-derivation/topological_pointing_closure_20260618/closure_plots/mass_overlay_grid.png",
    "figures/bdt_topology_98forests/absy0to1_pt2to5_mass_bdt_vs_an.png": CMS_OUTPUT / "bdt-topology/bdt_topology_98forests_20260619/plots/absy0to1_pt2to5_mass_bdt_vs_an.png",
}

TABLE_SOURCES = {
    "data/miniaod_chain/produced_skim_d0_peak_manifest.txt": CMS_OUTPUT / "skim-to-peak/crab_98forest_apriori_20260619/peak/produced_skim_d0_peak_manifest.txt",
    "data/miniaod_chain/pt_y_bin_counts.csv": CMS_OUTPUT / "skim-to-peak/crab_98forest_apriori_20260619/peak/pt_y_bin_counts.csv",
    "data/preflight_medium_scale/zdc_1n_threshold_candidates.tsv": CMS_OUTPUT / "zdc-1n-thresholds/zerobias_forest_emptybx_forest_5m_20260617/zdc_1n_threshold_candidates.tsv",
    "data/preflight_medium_scale/zdc_1n_fit_params.tsv": CMS_OUTPUT / "zdc-1n-thresholds/zerobias_forest_emptybx_forest_5m_20260617/zdc_1n_fit_params.tsv",
    "data/preflight_medium_scale/zdc_scale_closure.tsv": CMS_OUTPUT / "preflight-medium-scale-20260618/zdc-scale-closure/zdc_scale_closure.tsv",
    "data/hf_gap_blind/hf_recommended_cutoffs.tsv": CMS_OUTPUT / "cut-derivation/hf_gap_data_mc_full_blind_20260618/hf_recommended_cutoffs.tsv",
    "data/hf_gap_blind/hf_quantiles.tsv": CMS_OUTPUT / "cut-derivation/hf_gap_data_mc_full_blind_20260618/hf_quantiles.tsv",
    "data/hf_gap_blind/hf_plus_threshold_scan.tsv": CMS_OUTPUT / "cut-derivation/hf_gap_data_mc_full_blind_20260618/hf_plus_threshold_scan.tsv",
    "data/hf_gap_blind/hf_minus_threshold_scan.tsv": CMS_OUTPUT / "cut-derivation/hf_gap_data_mc_full_blind_20260618/hf_minus_threshold_scan.tsv",
    "data/topology_sideband_independent/recommended_sidebands.tsv": CMS_OUTPUT / "cut-derivation/topology_sideband_derivation_20260622_gap_push/recommended_sidebands.tsv",
    "data/topology_rect_independent_sideband/topology_guided_an_reproduction.tsv": CMS_OUTPUT / "cut-derivation/topocuts_rect_independent_sideband_full_20260622_gap_push/topology_guided_an_reproduction.tsv",
    "data/topology_mc_background_independent_sideband/topology_guided_an_reproduction.tsv": CMS_OUTPUT / "cut-derivation/topocuts_mc_background_independent_sideband_full_20260622_gap_push/topology_guided_an_reproduction.tsv",
    "data/topological_floor_independent_sideband/floor_derivation.tsv": CMS_OUTPUT / "cut-derivation/topological_floor_independent_sideband_20260622_gap_push/floor_derivation.tsv",
    "data/topological_pointing_independent_sideband/pointing_an_validation.tsv": CMS_OUTPUT / "cut-derivation/topological_pointing_independent_sideband_20260622_gap_push/pointing_an_validation.tsv",
    "data/topological_pointing_independent_sideband/pointing_rederived_representative.tsv": CMS_OUTPUT / "cut-derivation/topological_pointing_independent_sideband_20260622_gap_push/pointing_rederived_representative.tsv",
    "data/bdt_topology_98forests/bdt_thresholds.tsv": CMS_OUTPUT / "bdt-topology/bdt_topology_gap_push_20260622/bdt_thresholds.tsv",
    "data/bdt_topology_98forests/bdt_overtraining.tsv": CMS_OUTPUT / "bdt-topology/bdt_topology_gap_push_20260622/bdt_overtraining.tsv",
    "data/bdt_topology_98forests/bdt_data_yields.tsv": CMS_OUTPUT / "bdt-topology/bdt_topology_gap_push_20260622/bdt_data_yields.tsv",
    "data/selection_independence/selection_independence_audit.tsv": CMS_OUTPUT / "selection-independence/selection_independence_audit.tsv",
    "data/topological_pointing_closure/pointing_closure_summary.tsv": CMS_OUTPUT / "cut-derivation/topological_pointing_closure_20260618/pointing_closure_summary.tsv",
    "data/mc_sample_tables/official_mc_sample_table.tsv": CMS_OUTPUT / "mc-sample-tables/official_mcm_20260621/official_mc_sample_table.tsv",
}


@dataclass
class Figure:
    source: Path
    caption: str
    label: str


@dataclass
class Section:
    number: str
    title: str
    status: str
    purpose: str
    reproduced: list[str] = field(default_factory=list)
    missing: list[str] = field(default_factory=list)
    figures: list[Figure] = field(default_factory=list)
    tables: list[Path] = field(default_factory=list)


def esc(text: object) -> str:
    s = str(text).translate(
        {
            ord("−"): "-",
            ord("–"): "-",
            ord("—"): "-",
            ord("µ"): "mu",
            ord("μ"): "mu",
            ord("σ"): "sigma",
            ord("π"): "pi",
            ord("α"): "alpha",
            ord("η"): "eta",
            ord("ϕ"): "phi",
            ord("φ"): "phi",
            ord("Δ"): "Delta",
            ord("≤"): "<=",
            ord("≥"): ">=",
            ord("±"): "+-",
            ord("×"): "x",
            ord("√"): "sqrt",
            ord("∞"): "infty",
            ord("→"): "->",
            ord("…"): "...",
            ord("\xa0"): " ",
        }
    )
    s = s.encode("ascii", "replace").decode("ascii")
    replacements = {
        "\\": r"\textbackslash{}",
        "&": r"\&",
        "%": r"\%",
        "$": r"\$",
        "#": r"\#",
        "_": r"\_",
        "{": r"\{",
        "}": r"\}",
        "~": r"\textasciitilde{}",
        "^": r"\textasciicircum{}",
    }
    return "".join(replacements.get(ch, ch) for ch in s)


def read_rows(path: Path, delimiter: str = "\t") -> list[dict[str, str]]:
    if not path.exists():
        return []
    with path.open(encoding="utf-8") as handle:
        return list(csv.DictReader(handle, delimiter=delimiter))


def rel_to(path: Path, base: Path) -> str:
    return path.resolve().relative_to(base.resolve()).as_posix()


def copy_if_exists(src: Path, dst_dir: Path) -> Path | None:
    if not src.exists():
        return None
    dst_dir.mkdir(parents=True, exist_ok=True)
    dst = dst_dir / src.name
    shutil.copy2(src, dst)
    return dst


def extract_an_targets(max_items: int = 160) -> list[dict[str, str]]:
    """Extract a compact target index from the AN text extraction."""
    if not AN_TEXT.exists():
        return []
    lines = [line.strip() for line in AN_TEXT.read_text(encoding="utf-8", errors="replace").splitlines()]
    targets: list[dict[str, str]] = []
    pattern = re.compile(r"^(Figure|Table)\s+([0-9A-Za-z.\-]+)\b[:.]?\s*(.*)$")
    for i, line in enumerate(lines):
        match = pattern.match(line)
        if not match:
            continue
        kind, num, rest = match.groups()
        caption_parts = [rest.strip()] if rest.strip() else []
        for nxt in lines[i + 1 : i + 4]:
            if not nxt:
                continue
            if pattern.match(nxt):
                break
            caption_parts.append(nxt)
            if sum(len(p) for p in caption_parts) > 180:
                break
        caption = " ".join(caption_parts)
        caption = re.sub(r"\s+", " ", caption).strip()
        if caption:
            targets.append({"kind": kind, "number": num, "caption": caption[:240]})
        if len(targets) >= max_items:
            break
    return targets


def classify_target(target: dict[str, str]) -> dict[str, str]:
    """Attach a reproducibility status and next production path to an AN target."""
    kind = target["kind"]
    number = target["number"]
    caption = target["caption"]
    low = caption.lower()
    key = f"{kind} {number}"
    status = "missing"
    artifact = ""
    production_path = "Define a dedicated producer from the AN target and add it to the full workflow wrapper."
    blocker = ""

    def fig_num() -> int | None:
        if kind != "Figure":
            return None
        match = re.match(r"(\d+)", number)
        return int(match.group(1)) if match else None

    def table_num() -> int | None:
        if kind != "Table":
            return None
        match = re.match(r"(\d+)", number)
        return int(match.group(1)) if match else None

    fnum = fig_num()
    tnum = table_num()

    if key in {"Figure 6", "Figure 7", "Figure 9"} or "zdc" in low or "emax" in low:
        status = "partial"
        artifact = "ZDC/HF threshold and control plots in output/zdc-1n-thresholds and output/cut-derivation."
        production_path = "run_zdc_1n_threshold_derivation.sh plus derive_hf_gap_cutoffs_data_mc.C; rebuild with run_full_d0_an_recreation_workflow.sh."
        blocker = "AN-style multi-peak fits, online trigger curves, and cross-section threshold scans still need dedicated producers."
    if fnum in {1, 2}:
        status = "partial"
        artifact = "Official prompt-MC ProcessID and generated-D0 kinematic proxy plots are produced."
        production_path = "run_missing_an_mc_proxy_plots.sh; add process-ID mapping once direct/resolved definitions are frozen."
        blocker = "Feynman diagrams and direct/resolved process-ID classification are documentation/modeling targets, not yet fully reproduced."
    if fnum in {8, 10, 11, 12, 13, 14, 15, 16, 17, 18}:
        status = "partial"
        artifact = "HF threshold plots and selected-event proxy plots exist; cross-section/efficiency threshold scans are not final."
        production_path = "Extend derive_hf_gap_cutoffs_data_mc.C to emit AN-style Emax, nTrack, jet-pT, efficiency, and threshold-variation panels."
        blocker = "Needs final efficiency/correction inputs for true cross-section scans."
    if fnum in {23, 24, 25, 26, 27, 28, 29, 30, 31}:
        status = "partial"
        artifact = "Selected-data topology/daughter proxy plots plus BDT and floor/pointing validation plots exist."
        production_path = "render_recreated_an_proxy_plots.C plus BDT/topology derivation scripts; add before/after-BDT and MC/data sideband overlay panels."
        blocker = "Needs finalized signal/background definitions and full-stat MC/data sideband inputs for AN-equivalent overlays."
    if fnum in {32, 33, 34, 35, 36}:
        status = "partial"
        artifact = "98-forest selected-skim mass peak and diagnostic pT/|y| mass-fit grids exist."
        production_path = "Extend render_recreated_an_proxy_plots.C or add final-yield fitter to write AN-style fit, pull, and goodness tables per bin."
        blocker = "Current fits are diagnostic; reflection constraints and full final signal extraction are not implemented."
    if fnum in {19, 20, 21, 22, 45, 46}:
        status = "missing"
        production_path = "Implement MC truth-matching efficiency/reweighting producer over official prompt/nonprompt/direct/resolved samples."
        blocker = "Requires full MC sample registry, truth matching, event/D efficiency denominators, and reweighting definitions."
    if fnum in {37, 38, 39, 40, 41, 42, 43}:
        status = "partial"
        artifact = "Prompt-MC reco/gen proxy maps, selected-shape comparisons, and proxy reweighting maps are produced."
        production_path = "Extend run_missing_an_mc_proxy_plots.sh to prompt/nonprompt and direct/resolved samples, then replace proxy ratios with final correction weights."
        blocker = "Final efficiency needs full MC categories, event efficiency, D reconstruction/selection denominators, and validated reweighting."
    if fnum in {5, 44}:
        status = "partial"
        artifact = "Official-reference trigger-bit occupancy and candidate-level data bit-fraction proxy maps are produced when the official proxy bundle is available."
        production_path = "run_missing_an_official_proxy_plots.sh; replace proxy maps with final trigger menu, prescale, and efficiency-map workflow once inputs are frozen."
        blocker = "Prescale/era handling and final reciprocal trigger-efficiency corrections are not yet reproduced."
    if fnum in {47, 48, 50, 51, 52}:
        status = "partial"
        artifact = "Official-reference systematic mass-variation and DCA prompt-fraction proxy panels are produced when the official proxy bundle is available."
        production_path = "run_missing_an_official_proxy_plots.sh; replace proxies with final corrected-yield systematic runner after final fit/correction chain is complete."
        blocker = "Uses official/pre-forested inputs and candidate-count proxies, not final corrected cross sections."
    if fnum in {53, 54, 55}:
        status = "missing"
        production_path = "Implement systematic-variation runner after final yield, efficiency, prompt-fraction, and fit models are frozen."
        blocker = "Depends on completed final signal extraction and correction chain."
    if fnum in {49}:
        status = "partial"
        artifact = "HF threshold efficiency/leakage scans and HF gap distributions are produced; final corrected-cross-section variation is not."
        production_path = "After final corrections are available, rerun the HF threshold scan through the cross-section builder."
        blocker = "Needs final corrected yields and correction-chain reruns for true systematic effect."
    if fnum in {56, 57, 64}:
        status = "missing"
        production_path = "Implement final cross-section/result plotter after luminosity, branching fraction, trigger/event/D efficiencies, EMD, and systematics are available."
        blocker = "Final corrected yields are not yet produced."
    if fnum in {58, 59, 60, 61, 63, 73, 74, 75}:
        status = "missing"
        production_path = "Implement FONLL/PYTHIA/theory-comparison producer from documented theory inputs and generated MC kinematics."
        blocker = "Theory input tables and final measurement points are not yet reproduced."
    if fnum in {65, 66, 72}:
        status = "external/reference"
        production_path = "Replace screenshots/schematics with cited source references or locally documented metadata where allowed."
        blocker = "These are documentation/reference figures rather than recreated analysis-data plots."
    if fnum in {67, 68, 69}:
        status = "missing"
        production_path = "Implement pThat stitching and jet/D0 correlation producer over private MC sample manifests."
        blocker = "Private MC pThat sample list and scaling factors need a frozen registry."
    if fnum in {70, 71}:
        status = "partial"
        artifact = "ZDC EmptyBX/noise and threshold inputs exist; channel-level AN-style panels are not complete."
        production_path = "Extend ZDC derivation macro to produce channel-by-channel SOI/SOI+1 and peak-fit appendix plots."
        blocker = "Needs final ZDC backport/calibration choice frozen."

    if tnum in {1}:
        status = "missing"
        production_path = "Build luminosity/run table from OMS/brilcalc export and Golden JSON run selection."
        blocker = "Final luminosity source and run list need signoff."
    if tnum in {2, 3, 4, 12, 13, 14, 15, 16, 17, 18}:
        status = "partial"
        artifact = "MC sample and McM notes exist in the CMS workspace."
        production_path = "Extend cms-official-mcctl/McM snapshots into AN-style sample tables with cross sections and filter efficiencies."
        blocker = "Private MC sample registry must be frozen."
    if tnum in {5, 20, 21}:
        status = "partial"
        artifact = "Trigger-bit occupancy/proxy tables are produced when the official proxy bundle is available."
        production_path = "Build final trigger-seed/path tables from HLT/L1 menu snapshots and analysis trigger registry; keep proxy occupancy as validation."
        blocker = "Requires finalized trigger-path source for AN-equivalent trigger tables."
    if tnum in {6, 7}:
        status = "partial"
        artifact = "Selection definitions and independent/partial cut-derivation audits exist."
        production_path = "Convert config/selection_independence.yaml and cut derivation outputs into AN-style selection tables."
        blocker = "Exact final topology/BDT selection freeze remains pending."
    if tnum in {8, 9}:
        status = "partial"
        artifact = "Diagnostic mass-fit summary exists for selected reproduced data."
        production_path = "Add final-yield fitter to write goodness-of-fit tables for 0nXn and Xn0n bins."
        blocker = "Needs final signal extraction model and both beam-direction samples."
    if tnum in {10, 11}:
        status = "missing"
        production_path = "Emit systematic uncertainty tables from the systematic-variation runner."
        blocker = "Depends on final corrections and systematic scans."
    if tnum in {19}:
        status = "partial"
        artifact = "ZDC 1n threshold candidate table exists."
        production_path = "Extend ZDC peak fitter to produce 1n/2n/3n mean/sigma appendix table."
        blocker = "Needs final ZDC peak-fit model."

    return {
        "kind": kind,
        "number": number,
        "caption": caption,
        "status": status,
        "artifact": artifact,
        "production_path": production_path,
        "blocker": blocker,
    }


def latest_dir(base: Path) -> Path | None:
    if not base.exists():
        return None
    dirs = [path for path in base.iterdir() if path.is_dir()]
    if not dirs:
        return None
    return max(dirs, key=lambda path: path.stat().st_mtime)


def latest_dir_with_file(base: Path, required: str) -> Path | None:
    if not base.exists():
        return None
    dirs = [path for path in base.iterdir() if path.is_dir() and (path / required).exists()]
    if not dirs:
        return None
    return max(dirs, key=lambda path: path.stat().st_mtime)


def build_sections(
    outdir: Path,
    proxy_dir: Path | None = None,
    mc_proxy_dir: Path | None = None,
    official_proxy_dir: Path | None = None,
    normalization_dir: Path | None = None,
    theory_dir: Path | None = None,
    pthat_dir: Path | None = None,
    yield_fit_dir: Path | None = None,
    zdc_multipeak_dir: Path | None = None,
    zdc_1n_dir: Path | None = None,
    hf_gap_dir: Path | None = None,
    bdt_dir: Path | None = None,
) -> list[Section]:
    fig_root = outdir / "figures"
    table_root = outdir / "data"

    def gf(path: str, caption: str, label: str) -> Figure | None:
        rel_parent = Path(path).parent
        try:
            rel_parent = rel_parent.relative_to("figures")
        except ValueError:
            pass
        source = FIGURE_SOURCES.get(path, GENERAL / path)
        copied = copy_if_exists(source, fig_root / rel_parent)
        if copied is None:
            return None
        return Figure(copied, caption, label)

    def gt(path: str) -> Path | None:
        rel_parent = Path(path).parent
        try:
            rel_parent = rel_parent.relative_to("data")
        except ValueError:
            pass
        source = TABLE_SOURCES.get(path, GENERAL / path)
        copied = copy_if_exists(source, table_root / rel_parent)
        return copied

    def pf(name: str, caption: str, label: str) -> Figure | None:
        if proxy_dir is None:
            return None
        copied = copy_if_exists(proxy_dir / name, fig_root / "an_proxy_plots")
        if copied is None:
            return None
        return Figure(copied, caption, label)

    def pt(name: str) -> Path | None:
        if proxy_dir is None:
            return None
        copied = copy_if_exists(proxy_dir / name, table_root / "an_proxy_plots")
        return copied

    def mf(name: str, caption: str, label: str) -> Figure | None:
        if mc_proxy_dir is None:
            return None
        copied = copy_if_exists(mc_proxy_dir / name, fig_root / "missing_an_mc_proxy")
        if copied is None:
            return None
        return Figure(copied, caption, label)

    def mt(name: str) -> Path | None:
        if mc_proxy_dir is None:
            return None
        copied = copy_if_exists(mc_proxy_dir / name, table_root / "missing_an_mc_proxy")
        return copied

    def of(name: str, caption: str, label: str) -> Figure | None:
        if official_proxy_dir is None:
            return None
        copied = copy_if_exists(official_proxy_dir / name, fig_root / "missing_an_official_proxy")
        if copied is None:
            return None
        return Figure(copied, caption, label)

    def ot(name: str) -> Path | None:
        if official_proxy_dir is None:
            return None
        copied = copy_if_exists(official_proxy_dir / name, table_root / "missing_an_official_proxy")
        return copied

    def nf(name: str, caption: str, label: str) -> Figure | None:
        if normalization_dir is None:
            return None
        copied = copy_if_exists(normalization_dir / name, fig_root / "normalization_scaffold")
        if copied is None:
            return None
        return Figure(copied, caption, label)

    def nt(name: str) -> Path | None:
        if normalization_dir is None:
            return None
        copied = copy_if_exists(normalization_dir / name, table_root / "normalization_scaffold")
        return copied

    def thf(name: str, caption: str, label: str) -> Figure | None:
        if theory_dir is None:
            return None
        copied = copy_if_exists(theory_dir / name, fig_root / "theory_scaffold")
        if copied is None:
            return None
        return Figure(copied, caption, label)

    def tht(name: str) -> Path | None:
        if theory_dir is None:
            return None
        return copy_if_exists(theory_dir / name, table_root / "theory_scaffold")

    def psf(name: str, caption: str, label: str) -> Figure | None:
        if pthat_dir is None:
            return None
        copied = copy_if_exists(pthat_dir / name, fig_root / "pthat_stitching_scaffold")
        if copied is None:
            return None
        return Figure(copied, caption, label)

    def pst(name: str) -> Path | None:
        if pthat_dir is None:
            return None
        return copy_if_exists(pthat_dir / name, table_root / "pthat_stitching_scaffold")

    def yf(name: str, caption: str, label: str) -> Figure | None:
        if yield_fit_dir is None:
            return None
        copied = copy_if_exists(yield_fit_dir / name, fig_root / "final_yield_fit_summary")
        if copied is None:
            return None
        return Figure(copied, caption, label)

    def yt(name: str) -> Path | None:
        if yield_fit_dir is None:
            return None
        return copy_if_exists(yield_fit_dir / name, table_root / "final_yield_fit_summary")

    def zf(name: str, caption: str, label: str) -> Figure | None:
        if zdc_multipeak_dir is None:
            return None
        copied = copy_if_exists(zdc_multipeak_dir / name, fig_root / "zdc_multipeak")
        if copied is None:
            return None
        return Figure(copied, caption, label)

    def zt(name: str) -> Path | None:
        if zdc_multipeak_dir is None:
            return None
        copied = copy_if_exists(zdc_multipeak_dir / name, table_root / "zdc_multipeak")
        return copied

    def z1f(name: str, caption: str, label: str) -> Figure | None:
        if zdc_1n_dir is None:
            return None
        copied = copy_if_exists(zdc_1n_dir / name, fig_root / "zdc_1n_thresholds")
        if copied is None:
            return None
        return Figure(copied, caption, label)

    def z1t(name: str) -> Path | None:
        if zdc_1n_dir is None:
            return None
        return copy_if_exists(zdc_1n_dir / name, table_root / "zdc_1n_thresholds")

    def hff(name: str, caption: str, label: str) -> Figure | None:
        if hf_gap_dir is None:
            return None
        copied = copy_if_exists(hf_gap_dir / name, fig_root / "hf_gap_derivation")
        if copied is None:
            return None
        return Figure(copied, caption, label)

    def hft(name: str) -> Path | None:
        if hf_gap_dir is None:
            return None
        return copy_if_exists(hf_gap_dir / name, table_root / "hf_gap_derivation")

    bdt_base = (bdt_dir / "plots") if bdt_dir is not None else CMS_OUTPUT / "bdt-topology/bdt_topology_98forests_20260619/plots"

    def bf(name: str, caption: str, label: str) -> Figure | None:
        copied = copy_if_exists(bdt_base / name, fig_root / "bdt_topology_98forests")
        if copied is None:
            return None
        return Figure(copied, caption, label)

    def bt(name: str) -> Path | None:
        if bdt_dir is None:
            return None
        return copy_if_exists(bdt_dir / name, table_root / "bdt_topology_98forests")

    sections: list[Section] = []

    sections.append(
        Section(
            "1",
            "Introduction",
            "partial",
            "Define the UPC D0 measurement, the 0nXn topology, and the reproduction standard.",
            [
                "Current draft states the reproduction policy: AN outputs are validation references, not nominal inputs.",
                "Current data chain starts from 2023 MINIAOD, recreates minimal forests, selected skims, and mass spectra.",
            ],
            [
                "Final physics narrative should wait for full cross sections and systematic uncertainties.",
            ],
        )
    )

    sections.append(
        Section(
            "2",
            "Datasets",
            "partial",
            "Record data, control samples, luminosity inputs, run ranges, and existing validation artifacts.",
            [
                "Dataset registry includes 2023 HIForward MINIAOD, Twiki forests, D0 reference skims, EmptyBX, ZeroBias, Golden JSON, and 98 recreated forest files.",
                "The 98-forest medium sample is documented with file manifests and selected-skim outputs.",
            ],
            [
                "Full run/lumisection and luminosity tables have not yet been regenerated into AN-style tables.",
                "The current medium sample is HIForward0-scale, not the full final dataset.",
            ],
            tables=[p for p in [gt("data/miniaod_chain/produced_skim_d0_peak_manifest.txt")] if p],
        )
    )

    sections.append(
        Section(
            "3",
            "Monte Carlo Samples",
            "strong partial",
            "Document prompt/nonprompt photonuclear D0 MC, forced D0->Kpi decays, and generator provenance.",
            [
                "Official prompt D0 MC paths, generator fragments, filter efficiency records, and limitations are recorded in the registry.",
                "Prompt MC is used for topology and daughter-track signal-efficiency scans.",
                "A prompt-MC process-ID and generated-D0 kinematic proxy bundle is now produced from the official flat MC file.",
            ],
            [
                "Full prompt/nonprompt/direct/resolved MC production tables and pThat stitching closure are not fully rebuilt.",
                "ZDC response in the available prompt MC is not sufficient for deriving ZDC cuts.",
            ],
            figures=[
                f
                for f in [
                    mf("mc_process_id_distribution.png", "PYTHIA ProcessID distribution from the official prompt D0 MC flat tree.", "fig:mc-process-id"),
                    mf("mc_gen_d0_pt_y_map.png", "Generated D0 pT and |y| occupancy in the official prompt MC proxy scan.", "fig:mc-gen-map"),
                    mf("mc_gen_vs_reco_d0_pt.png", "Generated and truth-matched reconstructed D0 pT shapes in the prompt MC proxy scan.", "fig:mc-gen-reco-pt"),
                    mf("mc_gen_vs_reco_d0_y.png", "Generated and truth-matched reconstructed D0 rapidity shapes in the prompt MC proxy scan.", "fig:mc-gen-reco-y"),
                ]
                if f
            ],
            tables=[p for p in [gt("data/mc_sample_tables/official_mc_sample_table.tsv"), mt("mc_proxy_summary.tsv")] if p],
        )
    )

    sections.append(
        Section(
            "4",
            "Trigger Selection",
            "partial",
            "Apply trigger path definitions and derive trigger-efficiency corrections.",
            [
                "Trigger path roles and AN ambiguities are listed in the roadmap.",
                "Official-reference trigger-bit occupancy and candidate-level trigger-efficiency proxy maps are generated from official/pre-forested trees when the official proxy bundle is present.",
            ],
            [
                "Trigger menu snapshots, prescale tables, era handling, and final reciprocal-efficiency weights have not yet been regenerated from the recreated data.",
                "The current trigger plots are proxies from official/pre-forested inputs, not final trigger corrections.",
            ],
            figures=[
                f
                for f in [
                    of("trigger_occupancy_data.png", "Trigger and filter-bit occupancy in the official 2023 reduced data tree.", "fig:official-trigger-occupancy-data"),
                    of("trigger_occupancy_prompt_mc.png", "Trigger-bit occupancy in the official prompt D0 MC tree.", "fig:official-trigger-occupancy-mc"),
                    of("trigger_efficiency_proxy_data_zdcor.png", "Candidate-level L1 ZDCOR bit-fraction proxy from the official 2023 reduced data tree.", "fig:official-trigger-eff-data-zdcor"),
                    of("trigger_efficiency_proxy_data_zdcxorjet8.png", "Candidate-level L1 ZDCXOR+Jet8 bit-fraction proxy from the official 2023 reduced data tree.", "fig:official-trigger-eff-data-xor8"),
                    of("trigger_correction_proxy_data_zdcxorjet8.png", "Reciprocal data bit-fraction proxy for L1 ZDCXOR+Jet8.", "fig:official-trigger-corr-data-xor8"),
                    of("trigger_efficiency_proxy_zdcxorjet8.png", "Prompt-MC trigger-bit availability diagnostic; zero entries indicate the flat MC tree does not currently provide usable trigger emulation.", "fig:official-trigger-mc-availability"),
                ]
                if f
            ],
            tables=[
                p
                for p in [
                    ot("trigger_occupancy_data.tsv"),
                    ot("trigger_efficiency_proxy_data.tsv"),
                    ot("trigger_efficiency_proxy.tsv"),
                ]
                if p
            ],
        )
    )

    sections.append(
        Section(
            "5",
            "Offline Event Selection",
            "strong partial",
            "Recreate event-level UPC selection: PV/MET filters, ZDC 0nXn selection, and HF rapidity gap.",
            [
                "Nominal ZDC selection now follows the AN calibrated offline recipe: ZDCsignal = HADsum + 0.1 EMsum, with thresholds plus > 1100 GeV and minus > 1000 GeV.",
                "The 0nXn class is implemented as an XOR: plus-side Xn with minus-side 0n, or minus-side Xn with plus-side 0n.",
                "EmptyBX events validate the 0n-side noise tail, while fitted 1n/2n/3n ZDC spectra validate the Xn-side efficiency.",
                "The EM-pileup survival probability is treated as a later correction/uncertainty, not as part of the raw XOR cut.",
                "HF gap thresholds are derived with low-track pass and high-track leakage checks, with MC used only where valid.",
                "Event cutflow exists for the two-file CRAB smoke and for the 98-forest selected skim.",
                "AN-proxy event plots are generated from the reproduced 98-forest Section 5 selected event tree.",
            ],
            [
                "Full AN event efficiency tables, final ZDC Xn integration, 0n fake-rate accounting, and EM-pileup correction tables still need a full-statistics pass.",
            ],
            figures=[
                f
                for f in [
                    z1f("zdc_plus_1n_threshold.png", "Full-stat TWiki official reduced-tree plus-side ZDC one-neutron threshold validation against the AN calibrated threshold.", "fig:zdc-plus-latest"),
                    z1f("zdc_minus_1n_threshold.png", "Full-stat TWiki official reduced-tree minus-side ZDC one-neutron threshold validation against the AN calibrated threshold.", "fig:zdc-minus-latest"),
                    gf("figures/preflight_medium_scale/zdc_plus_1n_threshold.png", "Data/EmptyBX plus-side ZDC one-neutron threshold validation.", "fig:zdc-plus"),
                    gf("figures/preflight_medium_scale/zdc_minus_1n_threshold.png", "Data/EmptyBX minus-side ZDC one-neutron threshold validation.", "fig:zdc-minus"),
                    zf("zdc_plus_multipeak_fits.png", "TWiki official reduced-tree ZDC-plus 1n/2n/3n peak-fit proxy; used for AN-shape coverage, not final ZDC calibration.", "fig:zdc-plus-multipeak"),
                    zf("zdc_minus_multipeak_fits.png", "TWiki official reduced-tree ZDC-minus 1n/2n/3n peak-fit proxy; used for AN-shape coverage, not final ZDC calibration.", "fig:zdc-minus-multipeak"),
                    gf("figures/preflight_medium_scale/zdc_scale_scatter.png", "Same-event recreated/offical ZDC scale closure check used to validate the 2023 ZDC backport.", "fig:zdc-scale-closure"),
                    hff("hf_plus_gap_distributions.png", "Full-stat HF-plus gap-side Emax distributions for MC, data controls, and EmptyBX.", "fig:hf-plus-dist-latest"),
                    hff("hf_minus_gap_distributions.png", "Full-stat HF-minus gap-side Emax distributions for MC, data controls, and EmptyBX.", "fig:hf-minus-dist-latest"),
                    hff("hf_plus_threshold_efficiencies.png", "Full-stat HF-plus threshold efficiency and leakage scan.", "fig:hf-plus-latest"),
                    hff("hf_minus_threshold_efficiencies.png", "Full-stat HF-minus threshold efficiency and leakage scan.", "fig:hf-minus-latest"),
                    gf("figures/crab_cern_twofile_medium/event_cutflow.png", "Event cutflow from reproduced forest smoke.", "fig:event-cutflow"),
                    pf("event_cutflow_section5.png", "Section 5 event cutflow from the reproduced 98-forest selected event output.", "fig:proxy-event-cutflow"),
                    pf("pv_z_before_cut.png", "Primary-vertex z distribution before the vertex-z requirement.", "fig:proxy-pvz"),
                    pf("vertex_multiplicity_before_cut.png", "Vertex multiplicity before the nVtx requirement.", "fig:proxy-nvtx"),
                    pf("zdc_plus_minus_correlation.png", "Recreated ZDC plus/minus correlation before the 0nXn XOR requirement.", "fig:proxy-zdc-corr"),
                    pf("hf_gap_side_emax_before_gap.png", "HF gap-side Emax distribution before applying the rapidity-gap cut.", "fig:proxy-hf-gap"),
                ]
                if f
            ],
            tables=[
                p
                for p in [
                    z1t("zdc_1n_threshold_candidates.tsv"),
                    z1t("zdc_1n_fit_params.tsv"),
                    gt("data/preflight_medium_scale/zdc_1n_threshold_candidates.tsv"),
                    gt("data/preflight_medium_scale/zdc_1n_fit_params.tsv"),
                    gt("data/preflight_medium_scale/zdc_scale_closure.tsv"),
                    hft("hf_recommended_cutoffs.tsv"),
                    hft("hf_quantiles.tsv"),
                    hft("hf_plus_threshold_scan.tsv"),
                    hft("hf_minus_threshold_scan.tsv"),
                    zt("zdc_multipeak_fit_params.tsv"),
                    zt("zdc_channel_branch_audit.tsv"),
                    gt("data/crab_cern_twofile_medium/section5_event_cutflow.tsv"),
                    pt("README.md"),
                ]
                if p
            ],
        )
    )

    sections.append(
        Section(
            "6",
            "Event Selection Efficiencies",
            "partial",
            "Convert event-level selection into efficiency corrections in D0 kinematic bins.",
            [
                "ZDC/HF selection rules and medium-scale preflight checks are available.",
            ],
            [
                "Bin-by-bin event-selection efficiency maps have not been fully regenerated.",
                "Trigger/event efficiency separation remains to be finalized.",
            ],
        )
    )

    sections.append(
        Section(
            "7",
            "D-Meson Selection and Signal Extraction",
            "strong partial",
            "Select D0 candidates, justify topology cuts, and extract raw yields from Kpi invariant-mass spectra.",
            [
                "Dfinder ntDkpi branch is reproduced through minimal foresting and selected skims.",
                "Daughter-track, sideband, topology floor, pointing, and BDT checks are documented with scans.",
                "98-forest selected skim reaches the D0 mass-peak plotting stage.",
                "AN-proxy candidate plots and diagnostic mass fits are generated from the reproduced 98-forest Section 7 selected candidate output.",
                "The completed gap-push topology scans independently rederive floor values in 6/6 bins and accept the AN pointing rows in 6/6 bins under a predeclared plateau rule.",
                "A raw-yield fit QA scaffold checks the current diagnostic mass-fit means, widths, chi2/ndf, and raw yields by bin.",
            ],
            [
                "Final AN-style raw-yield fits, pull plots, reflection constraints, and full bin tables are not complete; the current fit panels are diagnostic, and one low-stat bin has a pathological unconstrained fit.",
            ],
            figures=[
                f
                for f in [
                    gf("figures/miniaod_chain/d0_mass_produced_skim_selected.png", "D0 mass spectrum from the reproduced selected skim.", "fig:d0-mass"),
                    gf("figures/miniaod_chain/d0_mass_produced_skim_pt_y_grid.png", "D0 mass spectra in pT and rapidity bins.", "fig:d0-grid"),
                    pf("d_candidate_cutflow_section7.png", "Section 7 D-candidate cutflow from the reproduced selected candidate output.", "fig:proxy-d-cutflow"),
                    pf("d_mass_before_d_cuts.png", "Kpi invariant-mass distribution before final D-candidate cuts.", "fig:proxy-mass-before"),
                    pf("d_mass_selected_recreated.png", "Selected Kpi invariant-mass distribution from the reproduced workflow.", "fig:proxy-mass-selected"),
                    pf("mass_fit_all_selected.png", "Diagnostic Gaussian-plus-linear fit to all selected D0 candidates.", "fig:proxy-mass-fit-all"),
                    pf("mass_fit_grid_selected.png", "Constrained diagnostic selected-candidate mass fits in pT and |y| bins; low-stat bins are not forced to fit.", "fig:proxy-mass-fit-grid"),
                    pf("mass_fit_pull_grid_selected.png", "Pull distributions for the constrained diagnostic selected-candidate mass fits.", "fig:proxy-mass-fit-pulls"),
                    pf("daughter1_pt_before_cut.png", "First daughter-track pT distribution before the daughter-track pT cut.", "fig:proxy-dtrk1-pt"),
                    pf("daughter2_pt_before_cut.png", "Second daughter-track pT distribution before the daughter-track pT cut.", "fig:proxy-dtrk2-pt"),
                    pf("daughter1_rel_pt_err_before_cut.png", "First daughter relative pT uncertainty before the quality cut.", "fig:proxy-dtrk1-pterr"),
                    pf("daughter2_rel_pt_err_before_cut.png", "Second daughter relative pT uncertainty before the quality cut.", "fig:proxy-dtrk2-pterr"),
                    pf("d_pt_before_cut.png", "D0 pT distribution before the analysis pT range cut.", "fig:proxy-dpt"),
                    pf("d_y_before_cut.png", "D0 rapidity distribution before the analysis rapidity range cut.", "fig:proxy-dy"),
                    gf("figures/topology_sideband_independent/sideband_resolution_rule.png", "Topology sideband rule derived from MC mass resolution.", "fig:sideband-rule"),
                    gf("figures/topology_rect_independent_sideband/topology_eff_vs_sideband_grid.png", "Independent data-sideband topology scan: MC signal efficiency versus data-sideband pass fraction.", "fig:topology-data-sideband-grid"),
                    gf("figures/topology_mc_background_independent_sideband/topology_eff_vs_sideband_grid.png", "Independent MC-background topology scan: MC signal efficiency versus nonmatched-MC sideband pass fraction.", "fig:topology-mc-sideband-grid"),
                    gf("figures/topological_floor_independent_sideband/floor_profile_grid.png", "Topological floor scan profile.", "fig:floor"),
                    gf("figures/topological_floor_independent_sideband/chi2_floor_profile_grid.png", "Secondary-vertex probability floor profile used in the independent topology-floor justification.", "fig:chi2-floor"),
                    gf("figures/topological_pointing_independent_sideband/pointing_plateau_grid.png", "Pointing-cut plateau validation.", "fig:pointing"),
                    yf("raw_yield_fit_qa_by_bin.png", "Raw Gaussian D0 yield QA by pT and |y| bin.", "fig:yield-fit-qa"),
                    yf("mass_mean_sigma_qa.png", "Mass-fit centroid and width QA against the D0 mass reference.", "fig:mass-mean-sigma-qa"),
                    yf("mass_fit_chi2_qa.png", "Mass-fit chi2/ndf QA for the diagnostic raw-yield fits.", "fig:mass-fit-chi2-qa"),
                    pf("topology_alpha_grid_before_cut.png", "D-alpha distributions before the pointing-angle cut in pT and |y| bins.", "fig:proxy-alpha-grid"),
                    pf("topology_svpv_sig_grid_before_cut.png", "3D decay-length significance distributions before the topological floor cut.", "fig:proxy-svpv-grid"),
                    pf("topology_chi2cl_grid_before_cut.png", "Secondary-vertex fit probability distributions before the topology cut.", "fig:proxy-chi2cl-grid"),
                    pf("topology_dtheta_grid_before_cut.png", "Delta-theta distributions before the angular-consistency cut.", "fig:proxy-dtheta-grid"),
                    mf("data_vs_mc_alpha.png", "Selected-data versus prompt-MC D-alpha shape comparison after the current rectangular topology selection.", "fig:mc-data-alpha"),
                    mf("data_vs_mc_svpvSig.png", "Selected-data versus prompt-MC 3D decay-length-significance shape comparison.", "fig:mc-data-svpv"),
                    mf("data_vs_mc_svpvSig2D.png", "Selected-data versus prompt-MC 2D decay-length-significance shape comparison.", "fig:mc-data-svpv2d"),
                    mf("data_vs_mc_chi2cl.png", "Selected-data versus prompt-MC secondary-vertex fit-probability shape comparison.", "fig:mc-data-chi2cl"),
                    mf("data_vs_mc_dtheta.png", "Selected-data versus prompt-MC angular-consistency shape comparison.", "fig:mc-data-dtheta"),
                    mf("data_vs_mc_trk1pt.png", "Selected-data versus prompt-MC leading-daughter pT shape comparison.", "fig:mc-data-trk1pt"),
                    mf("data_vs_mc_trk2pt.png", "Selected-data versus prompt-MC subleading-daughter pT shape comparison.", "fig:mc-data-trk2pt"),
                    mf("data_vs_mc_relpt1.png", "Selected-data versus prompt-MC leading-daughter relative pT uncertainty comparison.", "fig:mc-data-relpt1"),
                    mf("data_vs_mc_relpt2.png", "Selected-data versus prompt-MC subleading-daughter relative pT uncertainty comparison.", "fig:mc-data-relpt2"),
                    bf("absy0to1_pt2to5_bdt_threshold_scan.png", "BDT threshold scan in |y|<1, 2<pT<5.", "fig:bdt-scan-0-1-2-5"),
                    bf("absy0to1_pt5to8_bdt_threshold_scan.png", "BDT threshold scan in |y|<1, 5<pT<8.", "fig:bdt-scan-0-1-5-8"),
                    bf("absy0to1_pt8to12_bdt_threshold_scan.png", "BDT threshold scan in |y|<1, 8<pT<12.", "fig:bdt-scan-0-1-8-12"),
                    bf("absy1to2_pt2to5_bdt_threshold_scan.png", "BDT threshold scan in 1<|y|<2, 2<pT<5.", "fig:bdt-scan-1-2-2-5"),
                    bf("absy1to2_pt5to8_bdt_threshold_scan.png", "BDT threshold scan in 1<|y|<2, 5<pT<8.", "fig:bdt-scan-1-2-5-8"),
                    bf("absy1to2_pt8to12_bdt_threshold_scan.png", "BDT threshold scan in 1<|y|<2, 8<pT<12.", "fig:bdt-scan-1-2-8-12"),
                    bf("absy0to1_pt2to5_bdt_overtraining.png", "BDT overtraining check in |y|<1, 2<pT<5.", "fig:bdt-over-0-1-2-5"),
                    bf("absy1to2_pt2to5_bdt_overtraining.png", "BDT overtraining check in 1<|y|<2, 2<pT<5.", "fig:bdt-over-1-2-2-5"),
                    bf("absy0to1_pt2to5_mass_bdt_vs_an.png", "BDT cross-check against rectangular topology cuts in |y|<1, 2<pT<5.", "fig:bdt-mass"),
                    bf("absy0to1_pt5to8_mass_bdt_vs_an.png", "BDT cross-check against rectangular topology cuts in |y|<1, 5<pT<8.", "fig:bdt-mass-0-1-5-8"),
                    bf("absy0to1_pt8to12_mass_bdt_vs_an.png", "BDT cross-check against rectangular topology cuts in |y|<1, 8<pT<12.", "fig:bdt-mass-0-1-8-12"),
                    bf("absy1to2_pt2to5_mass_bdt_vs_an.png", "BDT cross-check against rectangular topology cuts in 1<|y|<2, 2<pT<5.", "fig:bdt-mass-1-2-2-5"),
                    bf("absy1to2_pt5to8_mass_bdt_vs_an.png", "BDT cross-check against rectangular topology cuts in 1<|y|<2, 5<pT<8.", "fig:bdt-mass-1-2-5-8"),
                    bf("absy1to2_pt8to12_mass_bdt_vs_an.png", "BDT cross-check against rectangular topology cuts in 1<|y|<2, 8<pT<12.", "fig:bdt-mass-1-2-8-12"),
                ]
                if f
            ],
            tables=[
                p
                for p in [
                    gt("data/miniaod_chain/stage_counts.csv"),
                    gt("data/topology_sideband_independent/recommended_sidebands.tsv"),
                    gt("data/topology_rect_independent_sideband/topology_guided_an_reproduction.tsv"),
                    gt("data/topology_mc_background_independent_sideband/topology_guided_an_reproduction.tsv"),
                    gt("data/topological_floor_independent_sideband/floor_derivation.tsv"),
                    gt("data/topological_pointing_independent_sideband/pointing_an_validation.tsv"),
                    gt("data/topological_pointing_independent_sideband/pointing_rederived_representative.tsv"),
                    bt("bdt_thresholds.tsv") or gt("data/bdt_topology_98forests/bdt_thresholds.tsv"),
                    bt("bdt_overtraining.tsv") or gt("data/bdt_topology_98forests/bdt_overtraining.tsv"),
                    bt("bdt_data_yields.tsv") or gt("data/bdt_topology_98forests/bdt_data_yields.tsv"),
                    yt("yield_fit_quality.tsv"),
                    pt("mass_fit_summary.tsv"),
                ]
                if p
            ],
        )
    )

    sections.append(
        Section(
            "8",
            "D-Meson Efficiency",
            "partial",
            "Build acceptance, reconstruction, and selection efficiency maps for D0 candidates.",
            [
                "Prompt MC signal-efficiency scans are used locally for topology and daughter-track choices.",
                "A prompt-MC reco/gen proxy map is generated from official MC and current rectangular D0 selection.",
            ],
            [
                "Full alpha-times-efficiency maps, prompt/nonprompt separation, direct/resolved separation, and final correction tables are not rebuilt.",
            ],
            figures=[
                f
                for f in [
                    mf("mc_acceptance_efficiency_proxy_map.png", "Reco/gen D0 efficiency proxy map from official prompt MC after current rectangular D0 selection.", "fig:mc-eff-proxy"),
                    mf("mc_reco_truth_d0_pt_y_map.png", "Truth-matched reconstructed D0 pT and |y| occupancy after current rectangular D0 selection.", "fig:mc-reco-map"),
                    mf("data_vs_mc_selected_d0_pt.png", "Selected-data and truth-matched prompt-MC D0 pT shape comparison.", "fig:data-mc-dpt"),
                    mf("data_vs_mc_selected_d0_y.png", "Selected-data and truth-matched prompt-MC D0 rapidity shape comparison.", "fig:data-mc-dy"),
                ]
                if f
            ],
            tables=[p for p in [mt("mc_proxy_summary.tsv")] if p],
        )
    )

    sections.append(
        Section(
            "9",
            "MC Reweighting",
            "partial",
            "Derive and apply generator-level D0 kinematic reweighting.",
            [
                "A selected-data/prompt-MC shape-ratio proxy weight map is produced as a diagnostic.",
                "The prompt-MC D0 pT and rapidity distributions are plotted before and after applying the proxy weight map.",
            ],
            [
                "FONLL/PYTHIA weight maps and reweighted efficiency closure plots are not yet reproduced.",
            ],
            figures=[
                f
                for f in [
                    mf("data_over_mc_reweighting_proxy_map.png", "Selected-data over prompt-MC D0 shape-ratio proxy weights in pT and |y| bins.", "fig:proxy-reweight-map"),
                    mf("mc_pt_before_after_proxy_reweight.png", "Prompt-MC selected D0 pT distribution before and after proxy reweighting.", "fig:proxy-reweight-pt"),
                    mf("mc_y_before_after_proxy_reweight.png", "Prompt-MC selected D0 rapidity distribution before and after proxy reweighting.", "fig:proxy-reweight-y"),
                ]
                if f
            ],
            tables=[p for p in [mt("mc_proxy_summary.tsv")] if p],
        )
    )

    sections.append(
        Section(
            "10",
            "Cross Section",
            "partial scaffold" if normalization_dir else "missing",
            "Combine raw yields, luminosity, bin widths, branching fraction, prescales, efficiencies, and pileup corrections.",
            [
                "The selected-skim mass spectra and bin counts exist as raw ingredients.",
                "A normalization scaffold produces raw-yield proxy plots and a template config for the final corrected-yield pipeline.",
            ],
            [
                "Final cross-section tables are not yet defensible because trigger, event, D-efficiency, luminosity, EMD, and systematic inputs are incomplete.",
                "The scaffold plots are explicitly uncorrected; all missing frozen inputs are listed in the input-request table.",
            ],
            figures=[
                f
                for f in [
                    nf("raw_yield_proxy_by_bin.png", "Raw Gaussian fitted D0 yield by pT and |y| bin from the current diagnostic mass fits.", "fig:raw-yield-proxy"),
                    nf("proxy_cross_section_density_by_bin.png", "Uncorrected raw-yield density by bin; this is a cross-section-pipeline placeholder, not a physics cross section.", "fig:proxy-xs-density"),
                ]
                if f
            ],
            tables=[
                p
                for p in [
                    gt("data/miniaod_chain/pt_y_bin_counts.csv"),
                    nt("raw_yield_proxy.tsv"),
                    nt("external_inputs_needed.tsv"),
                    nt("normalization_config.template.yaml"),
                ]
                if p
            ],
        )
    )

    sections.append(
        Section(
            "11",
            "Systematic Uncertainties",
            "partial",
            "Evaluate luminosity, trigger, event selection, D selection, tracking, fit, branching-ratio, gap-threshold, and prompt/nonprompt uncertainties.",
            [
                "Several cut-variation and closure studies exist for ZDC, HF, topology sidebands, floors, pointing, and BDT cross-checks.",
                "Official-reference D-selection pass-flag mass-shape variations and DCA prompt-fraction proxy templates are generated when the official proxy bundle is present.",
            ],
            [
                "Full systematic table builder and final source-by-source uncertainty matrix remain incomplete.",
                "Official-reference systematic panels are candidate-count/mass-shape proxies and are not final corrected cross-section uncertainties.",
            ],
            figures=[
                f
                for f in [
                    gf("figures/topological_pointing_closure/corrected_yield_ratio_grid.png", "Neighboring pointing-cut corrected-yield closure.", "fig:pointing-closure"),
                    gf("figures/topological_pointing_closure/mass_overlay_grid.png", "Mass-shape stability under neighboring pointing choices.", "fig:mass-shape"),
                    of("systematic_mass_variation_all.png", "Official-reference D0 mass-shape proxy under D-selection systematic pass-flag variations.", "fig:official-syst-mass-all"),
                    of("systematic_mass_variation_pt2to5_absy0to1.png", "Official-reference track-pT variation mass-shape proxy in 2<pT<5 and |y|<1.", "fig:official-syst-trackpt"),
                    of("systematic_mass_variation_pt8to12_absy1to2.png", "Official-reference high-pT topology/fit variation mass-shape proxy in 8<pT<12 and 1<|y|<2.", "fig:official-syst-highpt"),
                    of("systematic_yield_variation_proxy.png", "Candidate-count proxy for D-selection systematic variations.", "fig:official-syst-yield-proxy"),
                    of("dca_template_all.png", "DCA-significance templates for an official-reference prompt-fraction proxy.", "fig:official-dca-template"),
                    of("prompt_fraction_proxy_by_bin.png", "Template-fit prompt-fraction proxy by D0 kinematic bin.", "fig:official-prompt-fraction-proxy"),
                ]
                if f
            ],
            tables=[
                p
                for p in [
                    gt("data/selection_independence/selection_independence_audit.tsv"),
                    gt("data/topological_pointing_closure/pointing_closure_summary.tsv"),
                    ot("systematic_yield_variation_proxy.tsv"),
                    ot("prompt_fraction_proxy_by_bin.tsv"),
                    ot("official_proxy_summary.tsv"),
                ]
                if p
            ],
        )
    )

    sections.append(
        Section(
            "12",
            "Results",
            "partial scaffold" if normalization_dir else "missing",
            "Present final differential cross sections and consistency checks.",
            [
                "Current reproduced result is a mass peak, not a final corrected measurement.",
                "Raw-yield proxy plots now exist as placeholders for the final result plotting pipeline.",
            ],
            [
                "Final cross-section plots, Xn0n/0nXn compatibility, and AN ratio plots are pending corrections and systematics.",
            ],
            figures=[
                f
                for f in [
                    nf("raw_yield_proxy_by_bin.png", "Raw-yield proxy repeated here as the current non-final result placeholder.", "fig:result-raw-yield-proxy"),
                ]
                if f
            ],
            tables=[p for p in [nt("raw_yield_proxy.tsv"), nt("external_inputs_needed.tsv")] if p],
        )
    )

    sections.append(
        Section(
            "13-14",
            "FONLL Predictions and Comparison",
            "partial scaffold" if theory_dir else "missing",
            "Compare final measurement to perturbative QCD predictions and photonuclear modeling.",
            [
                "A theory-comparison scaffold now defines the exact frozen inputs needed for the FONLL, EPPS21, CT18, CGC, photon-flux, and H1/ZEUS comparison plots.",
                "The scaffold includes a producer contract and target-to-input mapping, but no final theory curves are generated without frozen tables/configs.",
            ],
            [
                "Theory tables, photon-flux/EMD corrections, and data/theory overlays have not been rebuilt.",
                "Required theory and normalization inputs are listed in the normalization scaffold input-request table.",
            ],
            figures=[
                f
                for f in [
                    thf("theory_input_status.png", "Frozen-input status for FONLL, final data, photon-flux/EMD, and auxiliary PYTHIA kinematic targets.", "fig:theory-input-status"),
                    thf("theory_plot_dependency_flow.png", "Producer contract for converting frozen theory and final measurement inputs into AN-style comparison plots.", "fig:theory-dependency-flow"),
                ]
                if f
            ],
            tables=[
                p
                for p in [
                    nt("external_inputs_needed.tsv"),
                    tht("theory_inputs_needed.tsv"),
                    tht("theory_target_status.tsv"),
                    tht("plot_theory_comparison.template.py"),
                ]
                if p
            ],
        )
    )

    sections.append(
        Section(
            "15",
            "Conclusions",
            "partial",
            "Summarize what has been reproduced and what remains.",
            [
                "The current reproducible chain supports the D0 mass peak from recreated forests and selected skims.",
            ],
            [
                "A complete AN recreation still requires final yields, corrections, systematics, and model comparisons.",
            ],
        )
    )

    sections.append(
        Section(
            "Appendices",
            "Provenance and Operational Details",
            "partial",
            "Collect provenance, target outputs, sample lists, and operational details.",
            [
                "The draft includes a compact AN target index and reproduced artifact manifest.",
                "A pThat-stitching scaffold defines the private-MC sample registry fields, weighting formula, and appendix plot targets.",
            ],
            [
                "Run-range, final pThat stitching, generator-production, MET-filter, EMD-pileup, and kinematic-correlation appendices need dedicated regeneration.",
            ],
            figures=[
                f
                for f in [
                    psf("pthat_input_coverage.png", "Current pThat stitching input coverage; private pThat registry and branch/weight conventions remain unfrozen.", "fig:pthat-input-coverage"),
                ]
                if f
            ],
            tables=[
                p
                for p in [
                    pst("pthat_inputs_needed.tsv"),
                    pst("pthat_target_status.tsv"),
                    pst("pthat_sample_registry.template.tsv"),
                    pst("pthat_stitching_formula.md"),
                ]
                if p
            ],
        )
    )

    return sections


def write_table_preview(tex: list[str], path: Path, outdir: Path, max_rows: int = 8) -> None:
    if not path.exists():
        return
    rel = rel_to(path, outdir)
    tex.append(r"\paragraph{Source table.} \texttt{" + esc(rel) + "}")
    suffix = path.suffix.lower()
    if suffix not in {".csv", ".tsv"}:
        return
    delimiter = "," if suffix == ".csv" else "\t"
    rows = read_rows(path, delimiter=delimiter)
    if not rows:
        return
    fields = list(rows[0].keys())[:5]
    tex.append(r"\begin{center}\scriptsize")
    tex.append(r"\begin{tabular}{|" + "l|" * len(fields) + "}")
    tex.append(r"\hline")
    tex.append(" & ".join(esc(f) for f in fields) + r" \\ \hline")
    for row in rows[:max_rows]:
        tex.append(" & ".join(esc(row.get(f, ""))[:90] for f in fields) + r" \\ \hline")
    tex.append(r"\end{tabular}")
    tex.append(r"\end{center}")


def write_tex(outdir: Path, sections: list[Section], targets: list[dict[str, str]]) -> Path:
    tex: list[str] = []
    target_plan = [classify_target(target) for target in targets]
    generated = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")
    tex.extend(
        [
            r"\documentclass[11pt]{article}",
            r"\usepackage[margin=0.75in]{geometry}",
            r"\usepackage{graphicx}",
            r"\usepackage{booktabs}",
            r"\usepackage{longtable}",
            r"\usepackage{array}",
            r"\usepackage{hyperref}",
            r"\usepackage{xcolor}",
            r"\hypersetup{colorlinks=true, linkcolor=blue, urlcolor=blue}",
            r"\title{Recreated Draft of the \(D^0\) UPC Analysis Note}",
            r"\author{Codex-generated reproduction scaffold}",
            r"\date{" + esc(generated) + r"}",
            r"\begin{document}",
            r"\maketitle",
            r"\section*{Scope and Rule}",
            (
                "This document is an AN-shaped reconstruction draft assembled only from "
                "current reproduced artifacts and durable logs. The original AN is used "
                "as a section and target scaffold; original prose and unsupported final "
                "numbers are not copied. Sections marked \\textbf{missing} or "
                "\\textbf{partial} are intentionally not presented as reproduced results."
            ),
            r"\section*{Coverage Summary}",
            r"\begin{center}",
            r"\begin{tabular}{lll p{0.48\textwidth}}",
            r"\toprule",
            r"Section & Status & Title & Current meaning \\",
            r"\midrule",
        ]
    )
    status_meaning = {
        "strong partial": "Core reproduced plots/checks exist, but final AN tables are incomplete.",
        "partial": "Some ingredients or validation artifacts exist.",
        "missing": "Not yet regenerated from current reproduced data.",
    }
    for section in sections:
        tex.append(
            esc(section.number)
            + " & "
            + esc(section.status)
            + " & "
            + esc(section.title)
            + " & "
            + esc(status_meaning.get(section.status, ""))
            + r" \\"
        )
    tex.extend([r"\bottomrule", r"\end{tabular}", r"\end{center}"])

    for section in sections:
        tex.append(r"\section*{" + esc(section.number) + ". " + esc(section.title) + "}")
        tex.append(r"\textbf{Status:} " + esc(section.status) + r"\\")
        tex.append(r"\textbf{Purpose in AN:} " + esc(section.purpose))
        if section.reproduced:
            tex.append(r"\paragraph{Reproduced or available now.}")
            tex.append(r"\begin{itemize}")
            for item in section.reproduced:
                tex.append(r"\item " + esc(item))
            tex.append(r"\end{itemize}")
        if section.missing:
            tex.append(r"\paragraph{Still missing or not final.}")
            tex.append(r"\begin{itemize}")
            for item in section.missing:
                tex.append(r"\item " + esc(item))
            tex.append(r"\end{itemize}")
        for fig in section.figures:
            tex.extend(
                [
                    r"\begin{center}",
                    r"\includegraphics[width=0.88\textwidth]{" + esc(rel_to(fig.source, outdir)) + r"}",
                    r"\par\smallskip",
                    r"\small\textbf{Figure:} " + esc(fig.caption) + r"\label{" + esc(fig.label) + r"}",
                    r"\end{center}",
                ]
            )
        if section.tables:
            tex.append(r"\paragraph{Representative tables.}")
            for table in section.tables[:4]:
                write_table_preview(tex, table, outdir)

    tex.append(r"\clearpage")
    tex.append(r"\section*{Original AN Target Index}")
    tex.append(
        "This is a compact machine-extracted index of figure/table targets from the AN text. "
        "It is used to guide future reproduction work, not as reproduced content."
    )
    if targets:
        tex.append(r"\begin{longtable}{lll p{0.62\textwidth}}")
        tex.append(r"\toprule")
        tex.append(r"Kind & Number & Current status & Extracted target clue \\")
        tex.append(r"\midrule")
        tex.append(r"\endhead")
        for target in targets[:120]:
            status = "mapped/partial" if any(
                token.lower() in target["caption"].lower()
                for token in ["zdc", "hf", "mass", "topological", "selection", "cut"]
            ) else "pending"
            tex.append(
                esc(target["kind"])
                + " & "
                + esc(target["number"])
                + " & "
                + esc(status)
                + " & "
                + esc(target["caption"])
                + r" \\"
            )
        tex.append(r"\bottomrule")
        tex.append(r"\end{longtable}")
    else:
        tex.append("No target index was extracted.")

    tex.append(r"\section*{Target Reproduction Plan}")
    tex.append(
        "Each extracted AN figure/table target is assigned a current reproduction status "
        "and a concrete next producer path. This table is intentionally operational: "
        "future iterations should work down the missing/partial rows and then rerun the "
        "full workflow wrapper."
    )
    if target_plan:
        tex.append(r"\begin{longtable}{lll p{0.26\textwidth} p{0.34\textwidth}}")
        tex.append(r"\toprule")
        tex.append(r"Kind & Number & Status & Current artifact & Next production path \\")
        tex.append(r"\midrule")
        tex.append(r"\endhead")
        for row in target_plan:
            tex.append(
                esc(row["kind"])
                + " & "
                + esc(row["number"])
                + " & "
                + esc(row["status"])
                + " & "
                + esc(row["artifact"] or row["blocker"] or "not yet produced")
                + " & "
                + esc(row["production_path"])
                + r" \\"
            )
        tex.append(r"\bottomrule")
        tex.append(r"\end{longtable}")
    else:
        tex.append("No target plan was generated.")

    tex.append(r"\section*{Next Jobs Needed for a Full AN Recreation}")
    tex.append(r"\begin{enumerate}")
    for item in [
        "Run or import the full trigger-efficiency workflow and generate AN-style trigger maps/tables.",
        "Generate final raw-yield fits in every pT,y bin from the reproduced selected skim, including residual and pull plots.",
        "Build D0 acceptance/reconstruction/selection efficiency maps from matched prompt/nonprompt MC.",
        "Derive or import MC reweighting tables and run efficiency closure after reweighting.",
        "Compute cross sections with luminosity, branching fraction, prescales, event efficiency, D efficiency, and EMD corrections.",
        "Run all systematic variations and generate the final uncertainty tables.",
        "Regenerate FONLL/theory comparison tables and final result plots.",
    ]:
        tex.append(r"\item " + esc(item))
    tex.append(r"\end{enumerate}")

    tex.append(r"\end{document}")
    tex_path = outdir / "main.tex"
    tex_path.write_text("\n".join(tex) + "\n", encoding="utf-8")
    return tex_path


def write_manifests(
    outdir: Path,
    sections: list[Section],
    targets: list[dict[str, str]],
    mc_proxy_dir: Path | None = None,
    official_proxy_dir: Path | None = None,
    normalization_dir: Path | None = None,
    theory_dir: Path | None = None,
    pthat_dir: Path | None = None,
    yield_fit_dir: Path | None = None,
    zdc_multipeak_dir: Path | None = None,
    zdc_1n_dir: Path | None = None,
    hf_gap_dir: Path | None = None,
    bdt_dir: Path | None = None,
) -> None:
    rows = []
    for section in sections:
        rows.append(
            {
                "section": section.number,
                "title": section.title,
                "status": section.status,
                "figures": str(len(section.figures)),
                "tables": str(len(section.tables)),
                "missing_items": str(len(section.missing)),
            }
        )
    with (outdir / "coverage_matrix.tsv").open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()), delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)

    with (outdir / "an_target_index.tsv").open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=["kind", "number", "caption"], delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerows(targets)

    target_plan = [classify_target(target) for target in targets]
    with (outdir / "an_reproduction_target_plan.tsv").open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=["kind", "number", "status", "artifact", "production_path", "blocker", "caption"],
            delimiter="\t",
            lineterminator="\n",
        )
        writer.writeheader()
        writer.writerows(target_plan)

    manifest = {
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "an_pdf": str(AN_PDF),
        "an_text": str(AN_TEXT),
        "cms_output_source": str(CMS_OUTPUT),
        "general_output_fallback": str(GENERAL),
        "mc_proxy_source": str(mc_proxy_dir) if mc_proxy_dir else "",
        "official_proxy_source": str(official_proxy_dir) if official_proxy_dir else "",
        "normalization_scaffold_source": str(normalization_dir) if normalization_dir else "",
        "theory_scaffold_source": str(theory_dir) if theory_dir else "",
        "pthat_stitching_scaffold_source": str(pthat_dir) if pthat_dir else "",
        "yield_fit_summary_source": str(yield_fit_dir) if yield_fit_dir else "",
        "zdc_multipeak_source": str(zdc_multipeak_dir) if zdc_multipeak_dir else "",
        "zdc_1n_source": str(zdc_1n_dir) if zdc_1n_dir else "",
        "hf_gap_source": str(hf_gap_dir) if hf_gap_dir else "",
        "bdt_source": str(bdt_dir) if bdt_dir else "",
        "figure_sources": {key: str(value) for key, value in FIGURE_SOURCES.items()},
        "table_sources": {key: str(value) for key, value in TABLE_SOURCES.items()},
        "sections": rows,
        "target_count": len(targets),
        "target_plan": "an_reproduction_target_plan.tsv",
    }
    (outdir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    readme = [
        "# D0 AN Recreation Draft",
        "",
        "<!-- DOC_OWNER: cms-analysis-manager AN recreation scaffold. -->",
        "<!-- TOKEN_NOTE: generated by build_d0_an_recreation.py. -->",
        "",
        "This directory contains an AN-shaped draft assembled from current reproduced D0 artifacts.",
        "It is not a completed replacement for the AN; unsupported final cross-section/systematic/theory sections are marked missing.",
        "",
        "Files:",
        "",
        "- `main.tex`: generated LaTeX source.",
        "- `main.pdf`: compiled draft if LaTeX completed.",
        "- `coverage_matrix.tsv`: section coverage and gaps.",
        "- `an_target_index.tsv`: compact extracted figure/table target index.",
        "- `an_reproduction_target_plan.tsv`: status and next producer path for every extracted AN target.",
        "- `manifest.json`: provenance.",
    ]
    (outdir / "README.md").write_text("\n".join(readme) + "\n", encoding="utf-8")


def compile_latex(outdir: Path) -> int:
    if shutil.which("latexmk") is None:
        return 127
    result = subprocess.run(
        ["latexmk", "-pdf", "-interaction=nonstopmode", "-halt-on-error", "main.tex"],
        cwd=outdir,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    (outdir / "latexmk.stdout").write_text(result.stdout, encoding="utf-8", errors="replace")
    return result.returncode


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--outdir", type=Path, required=True)
    parser.add_argument("--proxy-dir", type=Path, default=None)
    parser.add_argument("--mc-proxy-dir", type=Path, default=None)
    parser.add_argument("--official-proxy-dir", type=Path, default=None)
    parser.add_argument("--normalization-dir", type=Path, default=None)
    parser.add_argument("--theory-dir", type=Path, default=None)
    parser.add_argument("--pthat-dir", type=Path, default=None)
    parser.add_argument("--yield-fit-dir", type=Path, default=None)
    parser.add_argument("--zdc-multipeak-dir", type=Path, default=None)
    parser.add_argument("--zdc-1n-dir", type=Path, default=None)
    parser.add_argument("--hf-gap-dir", type=Path, default=None)
    parser.add_argument("--bdt-dir", type=Path, default=None)
    parser.add_argument("--auto-latest-proxy", action="store_true")
    parser.add_argument("--auto-latest-mc-proxy", action="store_true")
    parser.add_argument("--auto-latest-official-proxy", action="store_true")
    parser.add_argument("--auto-latest-normalization", action="store_true")
    parser.add_argument("--auto-latest-theory", action="store_true")
    parser.add_argument("--auto-latest-pthat", action="store_true")
    parser.add_argument("--auto-latest-yield-fit", action="store_true")
    parser.add_argument("--auto-latest-zdc-multipeak", action="store_true")
    parser.add_argument("--auto-latest-zdc-1n", action="store_true")
    parser.add_argument("--auto-latest-hf-gap", action="store_true")
    parser.add_argument("--auto-latest-bdt", action="store_true")
    parser.add_argument("--compile", action="store_true")
    args = parser.parse_args()

    outdir = args.outdir.resolve()
    outdir.mkdir(parents=True, exist_ok=True)

    proxy_dir = args.proxy_dir
    if args.auto_latest_proxy and proxy_dir is None:
        proxy_dir = latest_dir(WORKSPACE / "output/an-proxy-plots")
    if proxy_dir is not None:
        proxy_dir = proxy_dir.resolve()

    mc_proxy_dir = args.mc_proxy_dir
    if args.auto_latest_mc_proxy and mc_proxy_dir is None:
        mc_proxy_dir = latest_dir(WORKSPACE / "output/missing-an-mc-proxy")
    if mc_proxy_dir is not None:
        mc_proxy_dir = mc_proxy_dir.resolve()

    official_proxy_dir = args.official_proxy_dir
    if args.auto_latest_official_proxy and official_proxy_dir is None:
        official_proxy_dir = latest_dir(WORKSPACE / "output/missing-an-official-proxy")
    if official_proxy_dir is not None:
        official_proxy_dir = official_proxy_dir.resolve()

    normalization_dir = args.normalization_dir
    if args.auto_latest_normalization and normalization_dir is None:
        normalization_dir = latest_dir(WORKSPACE / "output/normalization-scaffold")
    if normalization_dir is not None:
        normalization_dir = normalization_dir.resolve()

    theory_dir = args.theory_dir
    if args.auto_latest_theory and theory_dir is None:
        theory_dir = latest_dir(WORKSPACE / "output/theory-scaffold")
    if theory_dir is not None:
        theory_dir = theory_dir.resolve()

    pthat_dir = args.pthat_dir
    if args.auto_latest_pthat and pthat_dir is None:
        pthat_dir = latest_dir(WORKSPACE / "output/pthat-stitching-scaffold")
    if pthat_dir is not None:
        pthat_dir = pthat_dir.resolve()

    yield_fit_dir = args.yield_fit_dir
    if args.auto_latest_yield_fit and yield_fit_dir is None:
        yield_fit_dir = latest_dir(WORKSPACE / "output/final-yield-fit-summary")
    if yield_fit_dir is not None:
        yield_fit_dir = yield_fit_dir.resolve()

    zdc_multipeak_dir = args.zdc_multipeak_dir
    if args.auto_latest_zdc_multipeak and zdc_multipeak_dir is None:
        zdc_multipeak_dir = latest_dir(WORKSPACE / "output/zdc-multipeak-fits")
    if zdc_multipeak_dir is not None:
        zdc_multipeak_dir = zdc_multipeak_dir.resolve()

    zdc_1n_dir = args.zdc_1n_dir
    if args.auto_latest_zdc_1n and zdc_1n_dir is None:
        zdc_1n_dir = latest_dir(WORKSPACE / "output/zdc-1n-thresholds")
    if zdc_1n_dir is not None:
        zdc_1n_dir = zdc_1n_dir.resolve()

    hf_gap_dir = args.hf_gap_dir
    if args.auto_latest_hf_gap and hf_gap_dir is None:
        hf_gap_dir = latest_dir_with_file(WORKSPACE / "output/cut-derivation", "hf_recommended_cutoffs.tsv")
    if hf_gap_dir is not None:
        hf_gap_dir = hf_gap_dir.resolve()

    bdt_dir = args.bdt_dir
    if args.auto_latest_bdt and bdt_dir is None:
        bdt_dir = latest_dir(WORKSPACE / "output/bdt-topology")
    if bdt_dir is not None:
        bdt_dir = bdt_dir.resolve()

    targets = extract_an_targets()
    sections = build_sections(
        outdir,
        proxy_dir=proxy_dir,
        mc_proxy_dir=mc_proxy_dir,
        official_proxy_dir=official_proxy_dir,
        normalization_dir=normalization_dir,
        theory_dir=theory_dir,
        pthat_dir=pthat_dir,
        yield_fit_dir=yield_fit_dir,
        zdc_multipeak_dir=zdc_multipeak_dir,
        zdc_1n_dir=zdc_1n_dir,
        hf_gap_dir=hf_gap_dir,
        bdt_dir=bdt_dir,
    )
    write_tex(outdir, sections, targets)
    write_manifests(
        outdir,
        sections,
        targets,
        mc_proxy_dir=mc_proxy_dir,
        official_proxy_dir=official_proxy_dir,
        normalization_dir=normalization_dir,
        theory_dir=theory_dir,
        pthat_dir=pthat_dir,
        yield_fit_dir=yield_fit_dir,
        zdc_multipeak_dir=zdc_multipeak_dir,
        zdc_1n_dir=zdc_1n_dir,
        hf_gap_dir=hf_gap_dir,
        bdt_dir=bdt_dir,
    )
    rc = compile_latex(outdir) if args.compile else 0
    if rc == 0:
        print(f"AN_RECREATION_STATUS ok outdir={outdir}")
    elif rc == 127:
        print(f"AN_RECREATION_STATUS latexmk_missing outdir={outdir}")
        return 0
    else:
        print(f"AN_RECREATION_STATUS latex_failed rc={rc} outdir={outdir}")
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
