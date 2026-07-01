#!/usr/bin/env python3
"""Summarize D0 topology sideband derivation outputs."""

from __future__ import annotations

import argparse
import csv
from collections import defaultdict
from pathlib import Path


BIN_ORDER = [
    "absy0to1_pt2to5",
    "absy0to1_pt5to8",
    "absy0to1_pt8to12",
    "absy1to2_pt2to5",
    "absy1to2_pt5to8",
    "absy1to2_pt8to12",
]


def read_tsv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle, delimiter="\t"))


def f(row: dict[str, str], key: str) -> float:
    return float(row[key])


def maybe_plot(outdir: Path, resolution: list[dict[str, str]], stability: list[dict[str, str]], rec: dict[str, str]) -> None:
    try:
        import matplotlib.pyplot as plt
        from PIL import Image, ImageOps
    except Exception as exc:  # pragma: no cover
        (outdir / "sideband_plot_warning.txt").write_text(f"plotting unavailable: {exc}\n", encoding="utf-8")
        return

    plot_dir = outdir / "sideband_plots"
    plot_dir.mkdir(exist_ok=True)

    labels = [row["bin"] for row in resolution]
    xs = list(range(len(labels)))
    sigma = [1000.0 * f(row, "sigma68") for row in resolution]
    inner3 = [1000.0 * f(row, "inner3sigma") for row in resolution]
    outer5 = [1000.0 * f(row, "outer5sigma") for row in resolution]
    inner = 1000.0 * f(rec, "innerAbsDm")
    outer = 1000.0 * f(rec, "outerAbsDm")
    an_inner = 1000.0 * f(rec, "anInnerAbsDm")
    an_outer = 1000.0 * f(rec, "anOuterAbsDm")

    fig, ax = plt.subplots(figsize=(8.0, 4.4))
    ax.plot(xs, sigma, marker="o", label=r"$\sigma_{68}$")
    ax.plot(xs, inner3, marker="s", label=r"$3\sigma_{68}$")
    ax.plot(xs, outer5, marker="^", label=r"$5\sigma_{68}$")
    ax.axhline(inner, color="tab:blue", linestyle="--", linewidth=1.2, label="derived inner")
    ax.axhline(outer, color="tab:orange", linestyle="--", linewidth=1.2, label="derived outer")
    ax.axhline(an_inner, color="tab:blue", linestyle=":", linewidth=1.2, label="AN inner")
    ax.axhline(an_outer, color="tab:orange", linestyle=":", linewidth=1.2, label="AN outer")
    ax.set_xticks(xs)
    ax.set_xticklabels(labels, rotation=35, ha="right", fontsize=8)
    ax.set_ylabel("mass offset [MeV]")
    ax.set_title("MC truth-matched mass resolution sideband rule")
    ax.grid(alpha=0.25)
    ax.legend(fontsize=8, ncol=2)
    fig.tight_layout()
    resolution_path = plot_dir / "sideband_resolution_rule.png"
    fig.savefig(resolution_path, dpi=160)
    plt.close(fig)

    derived_rows = [
        row
        for row in stability
        if abs(f(row, "innerAbsDm") - f(rec, "innerAbsDm")) < 1e-9
        and abs(f(row, "outerAbsDm") - f(rec, "outerAbsDm")) < 1e-9
    ]
    derived_rows.sort(key=lambda row: BIN_ORDER.index(row["bin"]) if row["bin"] in BIN_ORDER else 99)
    asym = [f(row, "countAsymmetry") for row in derived_rows]
    leakage = [100.0 * f(row, "signalLeakageFraction") for row in derived_rows]

    fig, ax1 = plt.subplots(figsize=(8.0, 4.4))
    ax1.bar(xs, asym, color="tab:gray", alpha=0.65, label="upper/lower count asymmetry")
    ax1.axhline(0.0, color="black", linewidth=0.8)
    ax1.set_ylabel("sideband count asymmetry")
    ax1.set_xticks(xs)
    ax1.set_xticklabels(labels, rotation=35, ha="right", fontsize=8)
    ax1.grid(alpha=0.25, axis="y")
    ax2 = ax1.twinx()
    ax2.plot(xs, leakage, color="tab:red", marker="o", label="MC signal leakage")
    ax2.set_ylabel("MC signal leakage [%]")
    handles1, labels1 = ax1.get_legend_handles_labels()
    handles2, labels2 = ax2.get_legend_handles_labels()
    ax1.legend(handles1 + handles2, labels1 + labels2, fontsize=8, loc="upper right")
    ax1.set_title("Derived sideband data balance and signal leakage")
    fig.tight_layout()
    stability_path = plot_dir / "sideband_stability.png"
    fig.savefig(stability_path, dpi=160)
    plt.close(fig)

    images = [ImageOps.contain(Image.open(path).convert("RGB"), (1100, 650)) for path in [resolution_path, stability_path]]
    width = max(image.width for image in images)
    height = max(image.height for image in images)
    pad = 28
    canvas = Image.new("RGB", (width + 2 * pad, 2 * height + 3 * pad), "white")
    for idx, image in enumerate(images):
        canvas.paste(image, (pad, pad + idx * (height + pad)))
    canvas.save(plot_dir / "sideband_derivation_grid.png", quality=95)


def write_summary(outdir: Path, resolution: list[dict[str, str]], stability: list[dict[str, str]], rec: dict[str, str]) -> None:
    inner = f(rec, "innerAbsDm")
    outer = f(rec, "outerAbsDm")
    derived = [
        row
        for row in stability
        if abs(f(row, "innerAbsDm") - inner) < 1e-9 and abs(f(row, "outerAbsDm") - outer) < 1e-9
    ]
    leakages = [f(row, "signalLeakageFraction") for row in derived]
    max_leakage = max(leakages) if leakages else 0.0
    asymmetries = [abs(f(row, "countAsymmetry")) for row in derived]
    max_asymmetry = max(asymmetries) if asymmetries else 0.0
    lines = [
        "# D0 Topology Sideband Derivation Summary",
        "",
        "<!-- DOC_OWNER: cms-analysis-manager topology sideband derivation. -->",
        "<!-- TOKEN_NOTE: AN sideband is a posthoc comparison target only. -->",
        "",
        "## Rule",
        "",
        "- Estimate per-bin MC truth-matched D0 mass resolution with `(q84.135 - q15.865)/2`.",
        "- Use one global sideband from the worst-bin resolution.",
        "- Inner edge: `round(3*sigma_max / 0.01)*0.01` GeV.",
        "- Outer edge: `round(5*sigma_max / 0.01)*0.01` GeV.",
        "- Compare with the AN sideband only after freezing the rule.",
        "",
        "## Result",
        "",
        f"- Derived sideband: `{inner:.2f} < |Dmass - 1.86484| < {outer:.2f}` GeV.",
        f"- AN comparison sideband: `{float(rec['anInnerAbsDm']):.2f} < |Dmass - 1.86484| < {float(rec['anOuterAbsDm']):.2f}` GeV.",
        f"- Matches AN: `{rec['matchesAN']}`.",
        f"- Max MC signal leakage in derived sideband: `{100.0 * max_leakage:.3f}%`.",
        f"- Max data upper/lower sideband count asymmetry: `{max_asymmetry:.3f}`.",
        "",
        "## Resolution Table",
        "",
        "| bin | MC signal | sigma68 [MeV] | 3 sigma [MeV] | 5 sigma [MeV] |",
        "|---|---:|---:|---:|---:|",
    ]
    for row in resolution:
        lines.append(
            f"| {row['bin']} | {row['mcSignalCount']} | {1000*f(row, 'sigma68'):.2f} | "
            f"{1000*f(row, 'inner3sigma'):.1f} | {1000*f(row, 'outer5sigma'):.1f} |"
        )
    lines.extend(
        [
            "",
            "## Derived Sideband Stability",
            "",
            "| bin | signal leakage | lower count | upper count | count asymmetry |",
            "|---|---:|---:|---:|---:|",
        ]
    )
    derived_by_bin = {row["bin"]: row for row in derived}
    for label in BIN_ORDER:
        row = derived_by_bin[label]
        lines.append(
            f"| {label} | {100*f(row, 'signalLeakageFraction'):.3f}% | "
            f"{row['lowerCount']} | {row['upperCount']} | {f(row, 'countAsymmetry'):.3f} |"
        )
    lines.extend(
        [
            "",
            "## Outputs",
            "",
            "- `sideband_resolution.tsv`",
            "- `recommended_sidebands.tsv`",
            "- `sideband_stability_scan.tsv`",
            "- `sideband_plots/sideband_derivation_grid.png`",
        ]
    )
    (outdir / "sideband_derivation_summary.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("outdir", type=Path)
    args = parser.parse_args()
    resolution = read_tsv(args.outdir / "sideband_resolution.tsv")
    recommended = read_tsv(args.outdir / "recommended_sidebands.tsv")[0]
    stability = read_tsv(args.outdir / "sideband_stability_scan.tsv")
    write_summary(args.outdir, resolution, stability, recommended)
    maybe_plot(args.outdir, resolution, stability, recommended)
    print(f"Wrote {args.outdir / 'sideband_derivation_summary.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
