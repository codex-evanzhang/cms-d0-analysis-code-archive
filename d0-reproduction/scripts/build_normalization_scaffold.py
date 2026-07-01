#!/usr/bin/env python3
"""Build a D0 final-normalization scaffold without pretending final inputs exist."""

from __future__ import annotations

import argparse
import csv
import math
import os
from datetime import datetime, timezone
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


ROOT = Path(os.environ.get("AGENT_NETWORK_ROOT", "/home/ubuntu/agent-network")).resolve()
CMS = ROOT / "research/cms-d0-analysis"
OUTPUT = CMS / "output"


EXTERNAL_INPUTS = [
    {
        "id": "final_run_lumi_table",
        "status": "needed",
        "owner": "analysis/lumi",
        "why": "Normalizes raw yields to differential cross sections.",
        "acceptable_proxy": "None for final result; current scaffold can only plot raw yields.",
        "specific_request": "Golden JSON/run list and brilcalc/OMS integrated luminosity table with units and certification.",
    },
    {
        "id": "trigger_menu_prescales_eras",
        "status": "needed",
        "owner": "trigger",
        "why": "Defines trigger paths, prescale weighting, and era-dependent efficiency maps.",
        "acceptable_proxy": "Candidate-level trigger bit fractions from the TWiki reduced tree.",
        "specific_request": "HLT/L1 menu names, seed logic, prescales, run ranges, and final trigger-efficiency correction convention.",
    },
    {
        "id": "final_efficiency_maps",
        "status": "needed",
        "owner": "mc/corrections",
        "why": "Converts fitted yields into acceptance/efficiency-corrected yields by pT and y bin.",
        "acceptable_proxy": "Prompt-MC reco/gen proxy maps already produced.",
        "specific_request": "Prompt/nonprompt, direct/resolved, pThat-stitched efficiency maps with bin definitions and uncertainties.",
    },
    {
        "id": "mc_sample_registry",
        "status": "needed",
        "owner": "mc",
        "why": "Freezes pThat bins, cross sections, filter efficiencies, process IDs, and stitching weights.",
        "acceptable_proxy": "Stored McM snapshots and official prompt/nonprompt sample table.",
        "specific_request": "Full official/private MC sample list including DAS paths, generator fragments, xsecs, filter efficiencies, pThat ranges, direct/resolved process-ID mapping, and event counts.",
    },
    {
        "id": "zdc_calibration_backport",
        "status": "needed",
        "owner": "detector/zdc",
        "why": "Determines final ZDC energy scale, SOI/SOI+1 interpretation, and threshold table.",
        "acceptable_proxy": "Data/EmptyBX ZDC-sum fits and threshold scans.",
        "specific_request": "Final 2023 ZDC calibration/backport convention, raw channel/SOI branch source, and approved 1n/2n/3n fit model.",
    },
    {
        "id": "emd_and_branching_constants",
        "status": "needed",
        "owner": "physics/constants",
        "why": "Needed for final gammaN/Ngamma normalization and branching-ratio propagation.",
        "acceptable_proxy": "None for final result.",
        "specific_request": "EMD probability/correction inputs, D0 branching fraction value/version, and uncertainty treatment.",
    },
    {
        "id": "fonll_theory_tables",
        "status": "needed",
        "owner": "theory",
        "why": "Needed to reproduce final data/theory comparison figures.",
        "acceptable_proxy": "Plotting script can be built once tables exist; no final overlay without theory points.",
        "specific_request": "FONLL/EPPS21/CT18/CGC prediction tables or generation configs, scale/PDF uncertainty bands, and bin-matching convention.",
    },
]


def latest_dir(base: Path) -> Path | None:
    if not base.exists():
        return None
    dirs = [path for path in base.iterdir() if path.is_dir()]
    if not dirs:
        return None
    return max(dirs, key=lambda path: path.stat().st_mtime)


def read_tsv(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        return []
    with path.open(encoding="utf-8") as handle:
        return list(csv.DictReader(handle, delimiter="\t"))


def write_tsv(path: Path, rows: list[dict[str, object]], fields: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        for row in rows:
            writer.writerow({field: row.get(field, "") for field in fields})


def bin_sort_key(label: str) -> tuple[int, int]:
    ay = 0 if "absy0to1" in label else 1
    if "pt2to5" in label:
        pt = 0
    elif "pt5to8" in label:
        pt = 1
    else:
        pt = 2
    return ay, pt


def label_pretty(label: str) -> str:
    return (
        label.replace("absy0to1_", "|y| 0-1, ")
        .replace("absy1to2_", "|y| 1-2, ")
        .replace("pt2to5", "pT 2-5")
        .replace("pt5to8", "pT 5-8")
        .replace("pt8to12", "pT 8-12")
    )


def load_raw_yields(mass_fit_summary: Path) -> list[dict[str, object]]:
    rows = []
    for row in read_tsv(mass_fit_summary):
        label = row.get("bin", "")
        if label == "all" or not label:
            continue
        try:
            fit_yield = float(row.get("yield_gaus", "nan"))
            entries = float(row.get("entries", "nan"))
            mean = float(row.get("mean", "nan"))
            sigma = float(row.get("sigma", "nan"))
            chi2ndf = float(row.get("chi2ndf", "nan"))
        except ValueError:
            continue
        pt_width = 3.0
        if "pt2to5" in label:
            pt_width = 3.0
        elif "pt5to8" in label:
            pt_width = 3.0
        elif "pt8to12" in label:
            pt_width = 4.0
        y_width = 1.0
        proxy_density = fit_yield / (pt_width * y_width) if pt_width > 0 else math.nan
        rows.append(
            {
                "bin": label,
                "pretty_bin": label_pretty(label),
                "entries": entries,
                "raw_gaussian_yield": fit_yield,
                "mean": mean,
                "sigma": sigma,
                "chi2ndf": chi2ndf,
                "pt_width": pt_width,
                "y_width": y_width,
                "proxy_raw_yield_density": proxy_density,
                "classification": "raw-yield-proxy; not efficiency/lumi corrected",
            }
        )
    rows.sort(key=lambda row: bin_sort_key(str(row["bin"])))
    return rows


def plot_yields(rows: list[dict[str, object]], outdir: Path) -> None:
    labels = [str(row["pretty_bin"]) for row in rows]
    yields = [float(row["raw_gaussian_yield"]) for row in rows]
    densities = [float(row["proxy_raw_yield_density"]) for row in rows]
    x = list(range(len(rows)))

    fig, ax = plt.subplots(figsize=(10, 5.5))
    ax.bar(x, yields, color="#4C78A8")
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=30, ha="right")
    ax.set_ylabel("Raw Gaussian yield from diagnostic fit")
    ax.set_title("D0 raw-yield proxy by pT and |y| bin")
    ax.grid(axis="y", alpha=0.25)
    fig.tight_layout()
    fig.savefig(outdir / "raw_yield_proxy_by_bin.png", dpi=180)
    fig.savefig(outdir / "raw_yield_proxy_by_bin.pdf")
    plt.close(fig)

    fig, ax = plt.subplots(figsize=(10, 5.5))
    ax.bar(x, densities, color="#59A14F")
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=30, ha="right")
    ax.set_ylabel("Raw yield / (Delta pT Delta |y|), arbitrary units")
    ax.set_title("Cross-section pipeline placeholder: uncorrected density only")
    ax.grid(axis="y", alpha=0.25)
    fig.tight_layout()
    fig.savefig(outdir / "proxy_cross_section_density_by_bin.png", dpi=180)
    fig.savefig(outdir / "proxy_cross_section_density_by_bin.pdf")
    plt.close(fig)


def write_formula(outdir: Path, mass_fit_summary: Path) -> None:
    text = f"""# D0 Normalization Scaffold

<!-- DOC_OWNER: cms-analysis-manager final-normalization scaffold. -->
<!-- TOKEN_NOTE: This is not a final cross-section result; it lists missing inputs and plots raw-yield proxies. -->

Generated UTC: `{datetime.now(timezone.utc).isoformat()}`

Raw-yield source: `{mass_fit_summary}`

## Formula to Implement Once Inputs Are Frozen

For each bin, the final differential cross section should be produced from a
versioned table using a form like

```text
d sigma / (dpT dy) =
  N_signal
  / (L_int * Delta pT * Delta y * B(D0 -> K pi)
     * epsilon_trigger * epsilon_event * epsilon_D
     * C_EMD * C_prompt/nonprompt * C_other)
```

The current plots set all correction/normalization factors to `not available`
and therefore show only the raw Gaussian fitted yield and the raw yield density.
They are useful for script plumbing and visual QA, not physics conclusions.

## Required Frozen Inputs

See `external_inputs_needed.tsv`.
"""
    (outdir / "normalization_formula.md").write_text(text, encoding="utf-8")


def write_template_config(outdir: Path) -> None:
    config = """# Template consumed by future final cross-section builder.
version: draft
luminosity:
  source: null
  value: null
  units: null
branching_fraction:
  d0_to_kpi: null
  uncertainty: null
trigger_efficiency_maps:
  gammaN: null
  Ngamma: null
event_selection_efficiency:
  gammaN: null
  Ngamma: null
d_selection_efficiency:
  prompt: null
  nonprompt: null
emd_corrections: null
mc_stitching:
  sample_registry: null
  direct_resolved_process_ids: null
  pthat_weights: null
theory_inputs:
  fonll_tables: null
  cgc_tables: null
"""
    (outdir / "normalization_config.template.yaml").write_text(config, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--label", default="normalization_scaffold_20260622")
    parser.add_argument("--mass-fit-summary", type=Path, default=None)
    args = parser.parse_args()

    proxy_dir = latest_dir(OUTPUT / "an-proxy-plots")
    mass_fit_summary = args.mass_fit_summary or (proxy_dir / "mass_fit_summary.tsv" if proxy_dir else Path())
    outdir = OUTPUT / "normalization-scaffold" / args.label
    outdir.mkdir(parents=True, exist_ok=True)

    rows = load_raw_yields(mass_fit_summary)
    write_tsv(
        outdir / "raw_yield_proxy.tsv",
        rows,
        [
            "bin",
            "pretty_bin",
            "entries",
            "raw_gaussian_yield",
            "mean",
            "sigma",
            "chi2ndf",
            "pt_width",
            "y_width",
            "proxy_raw_yield_density",
            "classification",
        ],
    )
    write_tsv(
        outdir / "external_inputs_needed.tsv",
        EXTERNAL_INPUTS,
        ["id", "status", "owner", "why", "acceptable_proxy", "specific_request"],
    )
    if rows:
        plot_yields(rows, outdir)
    write_formula(outdir, mass_fit_summary)
    write_template_config(outdir)

    readme = f"""# Normalization Scaffold

This directory contains final-normalization plumbing and raw-yield proxy plots.
It does not contain a final physics cross section.

- Input mass-fit summary: `{mass_fit_summary}`
- Raw-yield proxy table: `raw_yield_proxy.tsv`
- External/frozen input requests: `external_inputs_needed.tsv`
- Formula note: `normalization_formula.md`
- Future config template: `normalization_config.template.yaml`
"""
    (outdir / "README.md").write_text(readme, encoding="utf-8")
    print(f"out_dir={outdir}")
    print(f"rows={len(rows)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
