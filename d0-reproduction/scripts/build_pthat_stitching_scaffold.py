#!/usr/bin/env python3
"""Build a scaffold for D0 private-MC pThat stitching targets."""

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


REQUIRED_FIELDS = [
    "sample_label",
    "dataset_or_file",
    "pthat_min",
    "process_type",
    "direct_resolved_class",
    "generated_events",
    "cross_section",
    "filter_efficiency",
    "stitching_weight",
    "source_status",
]


INPUTS = [
    {
        "id": "private_pthat_sample_registry",
        "status": "needed",
        "owner": "mc/private",
        "why": "Defines every pThat bin and sample category entering the stitched distribution.",
        "acceptable_proxy": "Stored official prompt/nonprompt McM table documents only the official flat MC, not private pThat bins.",
        "specific_request": "Private pThat sample list with DAS/EOS paths, pThat_min, process type, direct/resolved classification, generated event counts, xsecs, and filter efficiencies.",
    },
    {
        "id": "pthat_weight_convention",
        "status": "needed",
        "owner": "mc/corrections",
        "why": "Needed to combine pThat bins into a single physical distribution.",
        "acceptable_proxy": "Template formula only.",
        "specific_request": "Approved stitching equation, normalization convention, event-weight branch names, and treatment of forced D0 decay/filter efficiency.",
    },
    {
        "id": "jet_and_d0_correlation_branches",
        "status": "needed",
        "owner": "mc/forest",
        "why": "Needed for AN appendix plots of pThat, PF jet pT, calo jet pT, and D0-pThat correlations.",
        "acceptable_proxy": "No final proxy; current MC proxy has D0 kinematics but not a frozen pThat registry.",
        "specific_request": "Branch names and tree paths for pThat, PF jet pT, calo jet pT, D0 pT, process ID, and event weights.",
    },
]


TARGETS = [
    {"target": "Figure 67", "plot": "stitched pThat distribution", "input_id": "private_pthat_sample_registry"},
    {"target": "Figure 68", "plot": "PF/calo jet pT by pThat bin and stitched", "input_id": "jet_and_d0_correlation_branches"},
    {"target": "Figure 69", "plot": "D0 pT versus pThat correlation", "input_id": "jet_and_d0_correlation_branches"},
    {"target": "Tables 12-18", "plot": "private pThat sample tables", "input_id": "private_pthat_sample_registry"},
]


def read_tsv(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        return []
    with path.open(encoding="utf-8") as handle:
        return list(csv.DictReader(handle, delimiter="\t"))


def write_tsv(path: Path, rows: list[dict[str, object]], fields: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        for row in rows:
            writer.writerow({field: row.get(field, "") for field in fields})


def build_registry_template(official_table: Path | None) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    official_rows = read_tsv(official_table) if official_table else []
    for row in official_rows:
        rows.append(
            {
                "sample_label": row.get("flavor", "official_mc"),
                "dataset_or_file": row.get("miniaod_dataset", ""),
                "pthat_min": "",
                "process_type": "",
                "direct_resolved_class": "",
                "generated_events": row.get("miniaod_events_in_das", ""),
                "cross_section": row.get("cross_section", ""),
                "filter_efficiency": row.get("filter_efficiency", ""),
                "stitching_weight": "",
                "source_status": "official McM provenance only; not a private pThat stitching row",
            }
        )
    if not rows:
        rows.append({field: "" for field in REQUIRED_FIELDS} | {"source_status": "template row; fill frozen private pThat sample"})
    return rows


def plot_status(outdir: Path, rows: list[dict[str, object]]) -> None:
    categories = ["private registry", "xsec/filter", "pThat bins", "jet branches", "weights"]
    available = [
        0.0,
        1.0 if any(str(row.get("filter_efficiency", "")) for row in rows) else 0.0,
        1.0 if any(str(row.get("pthat_min", "")) for row in rows) else 0.0,
        0.0,
        0.0,
    ]
    fig, ax = plt.subplots(figsize=(9, 4.8))
    ax.bar(categories, available, color=["#C44E52" if val == 0 else "#4C78A8" for val in available])
    ax.set_ylim(0, 1.1)
    ax.set_ylabel("available in current scaffold")
    ax.set_title("pThat stitching input coverage")
    ax.tick_params(axis="x", rotation=25)
    ax.grid(axis="y", alpha=0.25)
    fig.tight_layout()
    fig.savefig(outdir / "pthat_input_coverage.png", dpi=180)
    fig.savefig(outdir / "pthat_input_coverage.pdf")
    plt.close(fig)


def write_formula(outdir: Path) -> None:
    text = """# pThat Stitching Formula Scaffold

<!-- DOC_OWNER: cms-analysis-manager pThat stitching scaffold. -->
<!-- TOKEN_NOTE: This is a formula/input contract, not a final stitched MC result. -->

For each private MC pThat bin, the future stitcher should compute an event
weight with a form like:

```text
w_i = sigma_i * filter_eff_i / N_generated_i
```

The exact convention must be frozen with the MC production notes, especially
whether forced D0 decays, direct/resolved process selections, and any generator
event weights are already included in `sigma_i` or `filter_eff_i`.

Required output plots after inputs are frozen:

- stitched pThat distribution;
- PF jet pT distributions per pThat bin and stitched;
- calo jet pT distributions per pThat bin and stitched;
- D0 pT versus pThat correlation map.
"""
    (outdir / "pthat_stitching_formula.md").write_text(text, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--label", default="pthat_stitching_scaffold_20260622")
    parser.add_argument("--official-mc-table", type=Path, default=OUTPUT / "mc-sample-tables/official_mcm_20260621/official_mc_sample_table.tsv")
    args = parser.parse_args()

    outdir = OUTPUT / "pthat-stitching-scaffold" / args.label
    outdir.mkdir(parents=True, exist_ok=True)

    rows = build_registry_template(args.official_mc_table)
    write_tsv(outdir / "pthat_inputs_needed.tsv", INPUTS, ["id", "status", "owner", "why", "acceptable_proxy", "specific_request"])
    write_tsv(outdir / "pthat_target_status.tsv", TARGETS, ["target", "plot", "input_id"])
    write_tsv(outdir / "pthat_sample_registry.template.tsv", rows, REQUIRED_FIELDS)
    plot_status(outdir, rows)
    write_formula(outdir)

    readme = f"""# pThat Stitching Scaffold

Generated UTC: `{datetime.now(timezone.utc).isoformat()}`

This directory defines the private-MC pThat stitching input contract. It is not
a stitched MC result.

- Input requests: `pthat_inputs_needed.tsv`
- Target mapping: `pthat_target_status.tsv`
- Sample registry template: `pthat_sample_registry.template.tsv`
- Formula note: `pthat_stitching_formula.md`
"""
    (outdir / "README.md").write_text(readme, encoding="utf-8")
    print(f"out_dir={outdir}")
    print(f"registry_rows={len(rows)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
