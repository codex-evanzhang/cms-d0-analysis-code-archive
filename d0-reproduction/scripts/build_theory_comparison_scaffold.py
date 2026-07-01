#!/usr/bin/env python3
"""Build a scaffold for D0 FONLL/theory-comparison targets.

This script does not invent theory curves. It creates the reusable output
contract, input request tables, and status plots needed to turn frozen FONLL,
EPPS21/CT18/CGC, photon-flux, and final-measurement inputs into AN-style
comparison plots.
"""

from __future__ import annotations

import argparse
import csv
import os
from datetime import datetime, timezone
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


ROOT = Path(os.environ.get("AGENT_NETWORK_ROOT", "/home/ubuntu/agent-network")).resolve()
CMS = ROOT / "research/cms-d0-analysis"
OUTPUT = CMS / "output"


INPUTS = [
    {
        "id": "final_measurement_points",
        "status": "needed",
        "owner": "analysis/corrections",
        "why": "Required for final data/theory overlays.",
        "acceptable_proxy": "Raw-yield proxy table only checks plotting plumbing.",
        "specific_request": "Final corrected gammaN and Ngamma D0 cross-section points with stat/syst uncertainties, bin widths, and covariance convention.",
    },
    {
        "id": "fonll_h1_zeus_reference_tables",
        "status": "needed",
        "owner": "theory/reference",
        "why": "Recreates AN reference comparisons to H1/ZEUS and validates the local FONLL generation setup.",
        "acceptable_proxy": "None for final comparison; only target slots can be scaffolded.",
        "specific_request": "Published H1/ZEUS points and FONLL central/scale/PDF uncertainty tables or exact generation cards.",
    },
    {
        "id": "fonll_epPS21_ct18_cgc_predictions",
        "status": "needed",
        "owner": "theory/final",
        "why": "Defines the final model curves compared against the D0 measurement.",
        "acceptable_proxy": "None for final comparison.",
        "specific_request": "FONLL+EPPS21, FONLL+CT18, and CGC prediction tables binned to the analysis pT/y convention with uncertainty bands.",
    },
    {
        "id": "photon_flux_emd_inputs",
        "status": "needed",
        "owner": "physics/constants",
        "why": "Needed to reproduce the EMD probability estimate and gammaN/Ngamma mapping.",
        "acceptable_proxy": "Normalization scaffold lists this as an external input.",
        "specific_request": "Photon-flux configuration, EMD probability/correction convention, uncertainty treatment, and source references.",
    },
    {
        "id": "pythia_kinematic_auxiliary_branches",
        "status": "needed",
        "owner": "mc",
        "why": "Needed for appendix Q2, Bjorken-x, and Egamma maps if they are not already in the flat MC tree.",
        "acceptable_proxy": "Generated D0 pT/y and ProcessID proxy plots already exist.",
        "specific_request": "Branch names or producer recipe for Q2, Bjorken-x, Egamma, direct/resolved process ID, event weights, and pThat.",
    },
]


TARGETS = [
    {"target": "Figure 58", "plot": "Q2 and y photoproduction cuts", "input_id": "fonll_h1_zeus_reference_tables"},
    {"target": "Figure 59", "plot": "ZEUS data versus FONLL reference", "input_id": "fonll_h1_zeus_reference_tables"},
    {"target": "Figure 60", "plot": "H1 data versus FONLL reference", "input_id": "fonll_h1_zeus_reference_tables"},
    {"target": "Figure 61", "plot": "Local FONLL closure against H1 reference", "input_id": "fonll_h1_zeus_reference_tables"},
    {"target": "Figure 63", "plot": "EMD probability estimate", "input_id": "photon_flux_emd_inputs"},
    {"target": "Figure 64", "plot": "Final D0 cross section versus FONLL/CGC", "input_id": "final_measurement_points"},
    {"target": "Figure 73", "plot": "PYTHIA Q2 versus D0 kinematics", "input_id": "pythia_kinematic_auxiliary_branches"},
    {"target": "Figure 74", "plot": "PYTHIA Bjorken-x versus D0 kinematics", "input_id": "pythia_kinematic_auxiliary_branches"},
    {"target": "Figure 75", "plot": "PYTHIA Egamma versus D0 kinematics", "input_id": "pythia_kinematic_auxiliary_branches"},
]


def write_tsv(path: Path, rows: list[dict[str, object]], fields: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        for row in rows:
            writer.writerow({field: row.get(field, "") for field in fields})


def plot_input_status(outdir: Path) -> None:
    labels = [row["id"] for row in INPUTS]
    values = [0.0 for _ in labels]
    fig, ax = plt.subplots(figsize=(11, 5.2))
    ax.barh(range(len(labels)), values, color="#C44E52")
    ax.set_yticks(range(len(labels)))
    ax.set_yticklabels(labels)
    ax.set_xlim(0, 1)
    ax.set_xlabel("frozen input available")
    ax.set_title("Theory-comparison input status")
    ax.text(0.02, -0.9, "All final theory inputs are intentionally marked missing until frozen tables/configs are supplied.", fontsize=9)
    ax.grid(axis="x", alpha=0.25)
    fig.tight_layout()
    fig.savefig(outdir / "theory_input_status.png", dpi=180)
    fig.savefig(outdir / "theory_input_status.pdf")
    plt.close(fig)


def plot_dependency_flow(outdir: Path) -> None:
    fig, ax = plt.subplots(figsize=(11, 5.8))
    ax.axis("off")
    boxes = [
        (0.05, 0.65, "Frozen theory tables\nFONLL / EPPS21 / CT18 / CGC"),
        (0.05, 0.35, "Final corrected data\ncross sections + covariance"),
        (0.38, 0.50, "Binning adapter\npT, y, gammaN/Ngamma convention"),
        (0.70, 0.65, "Reference closure\nH1 / ZEUS / local FONLL"),
        (0.70, 0.35, "Final AN overlays\nD0 data versus models"),
    ]
    for x, y, text in boxes:
        ax.text(
            x,
            y,
            text,
            ha="left",
            va="center",
            bbox={"boxstyle": "round,pad=0.35", "fc": "#F2F2F2", "ec": "#444444"},
            fontsize=10,
        )
    arrows = [
        ((0.30, 0.65), (0.38, 0.55)),
        ((0.30, 0.35), (0.38, 0.47)),
        ((0.62, 0.55), (0.70, 0.65)),
        ((0.62, 0.47), (0.70, 0.35)),
    ]
    for start, end in arrows:
        ax.annotate("", xy=end, xytext=start, arrowprops={"arrowstyle": "->", "lw": 1.5})
    ax.set_title("Theory-comparison producer contract")
    fig.tight_layout()
    fig.savefig(outdir / "theory_plot_dependency_flow.png", dpi=180)
    fig.savefig(outdir / "theory_plot_dependency_flow.pdf")
    plt.close(fig)


def write_plotter_template(outdir: Path) -> None:
    text = """#!/usr/bin/env python3
\"\"\"Template for final D0 theory-comparison plotting.

Expected inputs:
- final corrected measurement table
- FONLL/EPPS21/CT18/CGC prediction tables
- H1/ZEUS reference tables or generation cards
- photon-flux/EMD constants

This template is intentionally not executed by default because the required
physics inputs are not frozen in the current workspace.
\"\"\"

from pathlib import Path


def main() -> int:
    raise SystemExit("Fill frozen input paths before producing final theory plots.")


if __name__ == "__main__":
    raise SystemExit(main())
"""
    path = outdir / "plot_theory_comparison.template.py"
    path.write_text(text, encoding="utf-8")
    path.chmod(0o755)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--label", default="theory_scaffold_20260622")
    args = parser.parse_args()

    outdir = OUTPUT / "theory-scaffold" / args.label
    outdir.mkdir(parents=True, exist_ok=True)

    write_tsv(outdir / "theory_inputs_needed.tsv", INPUTS, ["id", "status", "owner", "why", "acceptable_proxy", "specific_request"])
    write_tsv(outdir / "theory_target_status.tsv", TARGETS, ["target", "plot", "input_id"])
    plot_input_status(outdir)
    plot_dependency_flow(outdir)
    write_plotter_template(outdir)

    readme = f"""# D0 Theory Comparison Scaffold

Generated UTC: `{datetime.now(timezone.utc).isoformat()}`

This directory defines the input contract for AN Sections 13-14 and the theory
appendix targets. It does not contain final FONLL, EPPS21, CT18, CGC, or
photon-flux results.

- Input requests: `theory_inputs_needed.tsv`
- Target mapping: `theory_target_status.tsv`
- Producer template: `plot_theory_comparison.template.py`
"""
    (outdir / "README.md").write_text(readme, encoding="utf-8")
    print(f"out_dir={outdir}")
    print(f"targets={len(TARGETS)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
