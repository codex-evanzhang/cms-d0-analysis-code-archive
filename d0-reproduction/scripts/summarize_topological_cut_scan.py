#!/usr/bin/env python3
"""Summarize D0 topological-cut scans against the AN rectangular cuts."""

from __future__ import annotations

import argparse
import csv
import math
from pathlib import Path


NOMINAL = {
    "absy0to1_pt2to5": (0.40, 2.5, 0.10, 0.50),
    "absy0to1_pt5to8": (0.35, 3.5, 0.10, 0.30),
    "absy0to1_pt8to12": (0.40, 3.5, 0.10, 0.30),
    "absy1to2_pt2to5": (0.20, 2.5, 0.10, 0.30),
    "absy1to2_pt5to8": (0.25, 3.5, 0.10, 0.30),
    "absy1to2_pt8to12": (0.25, 3.5, 0.10, 0.30),
}


def read_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="") as handle:
        return list(csv.DictReader(handle, delimiter="\t"))


def f(row: dict[str, str], key: str) -> float:
    return float(row[key])


def close(a: float, b: float) -> bool:
    return abs(a - b) < 1e-9


def row_label(row: dict[str, str]) -> str:
    return (
        f"alpha<{row['alphaMax']}, svpv>{row['svpvSigMin']}, "
        f"chi2>{row['chi2clMin']}, dtheta<{row['dthetaMax']}"
    )


def exact_nominal(rows: list[dict[str, str]], values: tuple[float, float, float, float]) -> dict[str, str] | None:
    alpha, svpv, chi2, dtheta = values
    for row in rows:
        if (
            close(f(row, "alphaMax"), alpha)
            and close(f(row, "svpvpvSigMin") if "svpvpvSigMin" in row else f(row, "svpvSigMin"), svpv)
            and close(f(row, "chi2clMin"), chi2)
            and close(f(row, "dthetaMax"), dtheta)
        ):
            return row
    return None


def scan_distance(row: dict[str, str], values: tuple[float, float, float, float]) -> float:
    alpha, svpv, chi2, dtheta = values
    return math.sqrt(
        ((f(row, "alphaMax") - alpha) / 0.35) ** 2
        + ((f(row, "svpvSigMin") - svpv) / 3.0) ** 2
        + ((f(row, "chi2clMin") - chi2) / 0.18) ** 2
        + ((f(row, "dthetaMax") - dtheta) / 0.30) ** 2
    )


def choose_near_nominal(rows: list[dict[str, str]], nominal: dict[str, str] | None, values: tuple[float, float, float, float]) -> dict[str, str]:
    best_score = max(f(row, "score") for row in rows)
    if nominal:
        nominal_bkg = f(nominal, "backgroundFraction")
        nominal_eff = f(nominal, "signalEfficiency")
        allowed_bkg = min(1.0, max(nominal_bkg * 1.05, nominal_bkg + 1.0 / max(f(nominal, "backgroundBase"), 1.0)))
        candidates = [
            row
            for row in rows
            if f(row, "backgroundFraction") <= allowed_bkg
            and f(row, "signalEfficiency") >= 0.90 * nominal_eff
        ]
        if candidates:
            best_eff = max(f(row, "signalEfficiency") for row in candidates)
            plateau = [row for row in candidates if f(row, "signalEfficiency") >= 0.98 * best_eff]
            return min(plateau, key=lambda row: scan_distance(row, values))
    plateau = [row for row in rows if f(row, "score") >= 0.90 * best_score]
    return min(plateau, key=lambda row: scan_distance(row, values))


def maybe_plot(
    rows_by_bin: dict[str, list[dict[str, str]]],
    outdir: Path,
    summary_rows: list[dict[str, str]],
    background_axis_label: str,
) -> None:
    try:
        import matplotlib.pyplot as plt
    except Exception as exc:  # pragma: no cover - optional plotting dependency
        (outdir / "topology_plot_warning.txt").write_text(f"matplotlib unavailable: {exc}\n")
        return

    marker_by_bin = {row["bin"]: row for row in summary_rows}
    plot_dir = outdir / "topology_plots"
    plot_dir.mkdir(exist_ok=True)
    plot_paths: list[Path] = []

    for label, rows in rows_by_bin.items():
        xs = [f(row, "backgroundFraction") for row in rows]
        ys = [f(row, "signalEfficiency") for row in rows]
        ss = [f(row, "score") for row in rows]
        fig, ax = plt.subplots(figsize=(6.2, 4.4))
        sc = ax.scatter(xs, ys, c=ss, s=10, cmap="viridis", alpha=0.65, linewidths=0)
        ax.set_xlabel(background_axis_label)
        ax.set_ylabel("MC signal efficiency")
        ax.set_title(label)
        ax.grid(alpha=0.25)
        fig.colorbar(sc, ax=ax, label="scan score")

        marker = marker_by_bin[label]
        for prefix, color, shape in [
            ("nominal", "red", "o"),
            ("rawBest", "black", "s"),
            ("near", "magenta", "^"),
        ]:
            bx = float(marker[f"{prefix}BackgroundFraction"])
            by = float(marker[f"{prefix}SignalEfficiency"])
            ax.scatter([bx], [by], c=color, marker=shape, s=65, edgecolors="white", linewidths=0.8, label=prefix)
        ax.legend(fontsize=8)
        fig.tight_layout()
        plot_path = plot_dir / f"{label}_eff_vs_sideband.png"
        fig.savefig(plot_path, dpi=160)
        plot_paths.append(plot_path)
        plt.close(fig)

    try:
        from PIL import Image, ImageOps
    except Exception:
        return

    if len(plot_paths) != 6:
        return
    images = [Image.open(path).convert("RGB") for path in plot_paths]
    width = max(image.width for image in images)
    height = max(image.height for image in images)
    pad = 24
    canvas = Image.new("RGB", (3 * width + 4 * pad, 2 * height + 3 * pad), "white")
    for idx, image in enumerate(images):
        row, col = divmod(idx, 3)
        canvas.paste(
            ImageOps.contain(image, (width, height)),
            (pad + col * (width + pad), pad + row * (height + pad)),
        )
    canvas.save(plot_dir / "topology_eff_vs_sideband_grid.png", quality=95)


def guided_rows(rows_by_bin: dict[str, list[dict[str, str]]]) -> list[dict[str, str]]:
    """Apply the AN-guided rectangular reproduction logic.

    The AN text motivates the SVPV significance floors and the vertex
    probability cut separately from a blind score maximization. Once those are
    fixed, alpha/dtheta are treated as a coarse plateau choice.
    """
    out: list[dict[str, str]] = []
    for label, values in NOMINAL.items():
        alpha, svpv, chi2, dtheta = values
        rows = rows_by_bin[label]
        nominal = exact_nominal(rows, values)
        if nominal is None:
            raise RuntimeError(f"nominal row missing from scan grid for {label}")
        fixed = [
            row
            for row in rows
            if close(f(row, "svpvSigMin"), svpv)
            and close(f(row, "chi2clMin"), chi2)
        ]
        fixed_best = max(fixed, key=lambda row: f(row, "score"))
        ratio = f(nominal, "score") / f(fixed_best, "score") if f(fixed_best, "score") > 0 else 0.0
        accepted = ratio >= 0.85
        out.append(
            {
                "bin": label,
                "fixedSvpvSigMin": f"{svpv:g}",
                "fixedChi2clMin": f"{chi2:g}",
                "nominalAlphaMax": f"{alpha:g}",
                "nominalDthetaMax": f"{dtheta:g}",
                "nominalSignalEfficiency": nominal["signalEfficiency"],
                "nominalBackgroundFraction": nominal["backgroundFraction"],
                "nominalScore": nominal["score"],
                "constrainedBestCuts": row_label(fixed_best),
                "constrainedBestSignalEfficiency": fixed_best["signalEfficiency"],
                "constrainedBestBackgroundFraction": fixed_best["backgroundFraction"],
                "constrainedBestScore": fixed_best["score"],
                "nominalOverConstrainedBest": f"{ratio:.6g}",
                "guidedDecision": "reproduce_AN_cut" if accepted else "not_on_plateau",
            }
        )
    return out


def summarize(indir: Path, background_label: str, background_axis_label: str) -> None:
    rows = read_rows(indir / "d_topology_scan.tsv")
    rows_by_bin: dict[str, list[dict[str, str]]] = {}
    for row in rows:
        rows_by_bin.setdefault(row["bin"], []).append(row)

    summary: list[dict[str, str]] = []
    for label, values in NOMINAL.items():
        bin_rows = rows_by_bin[label]
        raw = max(bin_rows, key=lambda row: f(row, "score"))
        nominal = exact_nominal(bin_rows, values)
        if nominal is None:
            raise RuntimeError(f"nominal row missing from scan grid for {label}")
        near = choose_near_nominal(bin_rows, nominal, values)
        bkg_base = int(float(nominal["backgroundBase"]))
        sig_base = int(float(nominal["signalBase"]))
        best_score = f(raw, "score")
        nominal_score = f(nominal, "score")
        score_ratio = nominal_score / best_score if best_score > 0 else 0.0
        note = "adequate_sideband_stats" if bkg_base >= 50 and sig_base >= 1000 else "sideband_limited"
        summary.append(
            {
                "bin": label,
                "nominalCuts": row_label(nominal),
                "nominalSignalBase": nominal["signalBase"],
                "nominalBackgroundBase": nominal["backgroundBase"],
                "nominalSignalEfficiency": nominal["signalEfficiency"],
                "nominalBackgroundFraction": nominal["backgroundFraction"],
                "nominalScore": nominal["score"],
                "rawBestCuts": row_label(raw),
                "rawBestSignalEfficiency": raw["signalEfficiency"],
                "rawBestBackgroundFraction": raw["backgroundFraction"],
                "rawBestScore": raw["score"],
                "nearCuts": row_label(near),
                "nearSignalEfficiency": near["signalEfficiency"],
                "nearBackgroundFraction": near["backgroundFraction"],
                "nearScore": near["score"],
                "nominalScoreOverBest": f"{score_ratio:.6g}",
                "statsNote": note,
            }
        )

    out_tsv = indir / "topology_an_cut_validation.tsv"
    with out_tsv.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, delimiter="\t", lineterminator="\n", fieldnames=list(summary[0].keys()))
        writer.writeheader()
        writer.writerows(summary)

    maybe_plot(rows_by_bin, indir, summary, background_axis_label)
    guided = guided_rows(rows_by_bin)
    guided_tsv = indir / "topology_guided_an_reproduction.tsv"
    with guided_tsv.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, delimiter="\t", lineterminator="\n", fieldnames=list(guided[0].keys()))
        writer.writeheader()
        writer.writerows(guided)

    out_md = indir / "topology_rectangular_summary.md"
    with out_md.open("w") as out:
        out.write("# Rectangular Topological-Cut Scan\n\n")
        out.write(
            "Signal efficiency is measured with truth-matched prompt D0 MC. "
            f"Background is estimated with {background_label}.\n\n"
        )
        out.write("The exact AN rectangular cut row is present in the scan grid for every bin. The raw optimum is not automatically the recommendation because the AN explicitly notes poor background statistics and uses rectangular cuts rather than the BDT/raw optimizer as the nominal choice.\n\n")
        out.write("| bin | AN cuts | signal eff. | sideband pass frac. | raw best | nominal/best score | note |\n")
        out.write("|---|---:|---:|---:|---:|---:|---|\n")
        for row in summary:
            out.write(
                f"| {row['bin']} | `{row['nominalCuts']}` | {float(row['nominalSignalEfficiency']):.4g} | "
                f"{float(row['nominalBackgroundFraction']):.4g} | `{row['rawBestCuts']}` | "
                f"{float(row['nominalScoreOverBest']):.3g} | {row['statsNote']} |\n"
            )
        out.write("\n## AN-Guided Reproduction\n\n")
        out.write("The AN text separately motivates `DsvpvSig` and `Dchi2cl`: use `DsvpvSig > 2.5` for `2 < pT < 5` and `> 3.5` for higher pT, and use `Dchi2cl > 0.1`. With those floors fixed, the nominal `alpha` and `dtheta` cuts are accepted when their score is at least 85% of the constrained best point. This avoids chasing small score fluctuations with a cut table the AN explicitly did not choose by blind optimization.\n\n")
        out.write("| bin | nominal/constrained-best score | decision |\n")
        out.write("|---|---:|---|\n")
        for row in guided:
            out.write(f"| {row['bin']} | {float(row['nominalOverConstrainedBest']):.3g} | {row['guidedDecision']} |\n")
        out.write("\nGenerated files:\n\n")
        out.write(f"- `{out_tsv.name}`\n")
        out.write(f"- `{guided_tsv.name}`\n")
        if (indir / "topology_plots").exists():
            out.write("- `topology_plots/*_eff_vs_sideband.png`\n")
            if (indir / "topology_plots" / "topology_eff_vs_sideband_grid.png").exists():
                out.write("- `topology_plots/topology_eff_vs_sideband_grid.png`\n")
        elif (indir / "topology_plot_warning.txt").exists():
            out.write("- `topology_plot_warning.txt`\n")

    print(f"wrote {out_tsv}")
    print(f"wrote {out_md}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("indir", type=Path)
    parser.add_argument(
        "--background-label",
        default="data mass sidebands",
        help="Human-readable background source for the markdown summary.",
    )
    parser.add_argument(
        "--background-axis-label",
        default="background sideband fraction passing",
        help="X-axis label for efficiency/rejection plots.",
    )
    args = parser.parse_args()
    summarize(args.indir, args.background_label, args.background_axis_label)


if __name__ == "__main__":
    main()
