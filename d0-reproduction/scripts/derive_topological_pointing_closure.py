#!/usr/bin/env python3
"""Prepare and summarize D0 pointing-cut closure against nearby plateau points.

The closure is deliberately post-derivation: it starts from the already frozen
pointing plateau and checks whether the AN row gives stable mass-window and
MC-efficiency-corrected yields compared with neighboring accepted plateau rows.
"""

from __future__ import annotations

import argparse
import csv
import math
from collections import defaultdict
from pathlib import Path


DEFAULT_BASE = Path("research/cms-d0-analysis/output/cut-derivation")
DEFAULT_POINTING = DEFAULT_BASE / "topological_pointing_justification_20260618"
DEFAULT_DATA_SCAN = DEFAULT_BASE / "topocuts_rect_an_sideband_full_20260617/d_topology_scan.tsv"
DEFAULT_FLOORS = DEFAULT_BASE / "topological_floor_justification_20260618/floor_derivation.tsv"
DEFAULT_OUTDIR = DEFAULT_BASE / "topological_pointing_closure_20260618"

BIN_META = {
    "absy0to1_pt2to5": (0.0, 1.0, 2.0, 5.0),
    "absy0to1_pt5to8": (0.0, 1.0, 5.0, 8.0),
    "absy0to1_pt8to12": (0.0, 1.0, 8.0, 12.0),
    "absy1to2_pt2to5": (1.0, 2.0, 2.0, 5.0),
    "absy1to2_pt5to8": (1.0, 2.0, 5.0, 8.0),
    "absy1to2_pt8to12": (1.0, 2.0, 8.0, 12.0),
}
BIN_ORDER = list(BIN_META)


def read_tsv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle, delimiter="\t"))


def write_tsv(path: Path, rows: list[dict[str, object]]) -> None:
    if not rows:
        path.write_text("", encoding="utf-8")
        return
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, delimiter="\t", lineterminator="\n", fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def f(row: dict[str, str], key: str) -> float:
    return float(row[key])


def fmt(value: float) -> str:
    if math.isnan(value):
        return "nan"
    return f"{value:.6g}"


def read_floors(path: Path) -> dict[str, tuple[float, float]]:
    floors: dict[str, tuple[float, float]] = {}
    for row in read_tsv(path):
        floors[row["bin"]] = (f(row, "derivedSvpvSigMin"), f(row, "derivedChi2clMin"))
    return floors


def read_signal_efficiencies(path: Path) -> dict[tuple[str, float, float, float, float], float]:
    out: dict[tuple[str, float, float, float, float], float] = {}
    for row in read_tsv(path):
        key = (
            row["bin"],
            f(row, "alphaMax"),
            f(row, "svpvSigMin"),
            f(row, "chi2clMin"),
            f(row, "dthetaMax"),
        )
        out[key] = f(row, "signalEfficiency")
    return out


def grid_index(values: list[float], value: float) -> int:
    for idx, item in enumerate(values):
        if abs(item - value) < 1e-9:
            return idx
    raise ValueError(f"value {value} not in grid {values}")


def prepare(args: argparse.Namespace) -> Path:
    args.outdir.mkdir(parents=True, exist_ok=True)
    plateau_path = args.pointing_dir / "pointing_plateau_candidates.tsv"
    plateau_rows = read_tsv(plateau_path)
    floors = read_floors(args.floor_derivation)
    efficiencies = read_signal_efficiencies(args.data_sideband_scan)
    by_bin: dict[str, list[dict[str, str]]] = defaultdict(list)
    for row in plateau_rows:
        by_bin[row["bin"]].append(row)

    out_rows: list[dict[str, object]] = []
    for label in BIN_ORDER:
        rows = by_bin[label]
        accepted = [row for row in rows if row["acceptedPlateau"] == "true"]
        an = next(row for row in rows if row["isANRow"] == "true")
        alpha_values = sorted({f(row, "alphaMax") for row in rows})
        dtheta_values = sorted({f(row, "dthetaMax") for row in rows})
        an_alpha = f(an, "alphaMax")
        an_dtheta = f(an, "dthetaMax")
        an_ai = grid_index(alpha_values, an_alpha)
        an_di = grid_index(dtheta_values, an_dtheta)
        selected: list[tuple[int, float, dict[str, str]]] = []
        for row in accepted:
            alpha = f(row, "alphaMax")
            dtheta = f(row, "dthetaMax")
            distance = abs(grid_index(alpha_values, alpha) - an_ai) + abs(grid_index(dtheta_values, dtheta) - an_di)
            if distance <= args.max_grid_distance:
                score = f(row, "jointScoreRatio")
                selected.append((distance, -score, row))
        selected.sort(key=lambda item: (item[0], item[1], f(item[2], "alphaMax"), f(item[2], "dthetaMax")))
        if len(selected) > args.max_points_per_bin:
            keep = selected[: args.max_points_per_bin]
            if not any(item[2]["isANRow"] == "true" for item in keep):
                keep[-1] = next(item for item in selected if item[2]["isANRow"] == "true")
            selected = sorted(keep, key=lambda item: (item[0], item[1], f(item[2], "alphaMax"), f(item[2], "dthetaMax")))
        svpv, chi2 = floors[label]
        abs_y_min, abs_y_max, pt_min, pt_max = BIN_META[label]
        for index, (distance, _minus_score, row) in enumerate(selected):
            alpha = f(row, "alphaMax")
            dtheta = f(row, "dthetaMax")
            signal_eff = efficiencies.get((label, alpha, svpv, chi2, dtheta), float("nan"))
            point_id = f"{label}__a{alpha:g}_dt{dtheta:g}".replace(".", "p")
            out_rows.append(
                {
                    "pointId": point_id,
                    "bin": label,
                    "pointIndex": index,
                    "gridDistanceFromAN": distance,
                    "absYMin": abs_y_min,
                    "absYMax": abs_y_max,
                    "ptMin": pt_min,
                    "ptMax": pt_max,
                    "alphaMax": fmt(alpha),
                    "svpvSigMin": fmt(svpv),
                    "chi2clMin": fmt(chi2),
                    "dthetaMax": fmt(dtheta),
                    "signalEfficiency": fmt(signal_eff),
                    "signalRetentionLoose": row["signalRetentionLoose"],
                    "jointScoreRatio": row["jointScoreRatio"],
                    "isANRow": row["isANRow"],
                }
            )

    path = args.outdir / "closure_points.tsv"
    write_tsv(path, out_rows)
    return path


def stat_error_ratio(value: float, err: float, ref: float, ref_err: float) -> float:
    if ref == 0 or math.isnan(value) or math.isnan(ref):
        return float("nan")
    return abs(value / ref) * math.sqrt((err / value) ** 2 + (ref_err / ref) ** 2) if value != 0 else abs(ref_err / ref)


def summarize(args: argparse.Namespace) -> None:
    args.outdir.mkdir(parents=True, exist_ok=True)
    points = {row["pointId"]: row for row in read_tsv(args.outdir / "closure_points.tsv")}
    yields_path = args.outdir / "closure_yields.tsv"
    if not yields_path.exists():
        return
    rows = read_tsv(yields_path)
    by_bin: dict[str, list[dict[str, str]]] = defaultdict(list)
    for row in rows:
        by_bin[row["bin"]].append(row)

    summary: list[dict[str, object]] = []
    ratios: list[dict[str, object]] = []
    for label in BIN_ORDER:
        bin_rows = by_bin[label]
        an = next(row for row in bin_rows if row["isANRow"] == "true")
        an_raw = f(an, "sidebandSubtractedYield")
        an_raw_err = f(an, "sidebandSubtractedYieldErr")
        an_corr = f(an, "efficiencyCorrectedYield")
        an_corr_err = f(an, "efficiencyCorrectedYieldErr")
        max_abs_shift = 0.0
        max_abs_raw_shift = 0.0
        max_z = 0.0
        passing = 0
        tested = 0
        for row in bin_rows:
            raw = f(row, "sidebandSubtractedYield")
            raw_err = f(row, "sidebandSubtractedYieldErr")
            corr = f(row, "efficiencyCorrectedYield")
            corr_err = f(row, "efficiencyCorrectedYieldErr")
            raw_ratio = raw / an_raw if an_raw else float("nan")
            corr_ratio = corr / an_corr if an_corr else float("nan")
            raw_ratio_err = stat_error_ratio(raw, raw_err, an_raw, an_raw_err)
            corr_ratio_err = stat_error_ratio(corr, corr_err, an_corr, an_corr_err)
            combined = math.sqrt(corr_err * corr_err + an_corr_err * an_corr_err)
            z = abs(corr - an_corr) / combined if combined > 0 else float("nan")
            if not row["isANRow"] == "true":
                tested += 1
                if not math.isnan(corr_ratio):
                    max_abs_shift = max(max_abs_shift, abs(corr_ratio - 1.0))
                if not math.isnan(raw_ratio):
                    max_abs_raw_shift = max(max_abs_raw_shift, abs(raw_ratio - 1.0))
                if not math.isnan(z):
                    max_z = max(max_z, z)
                if (not math.isnan(z) and z <= args.compatibility_sigma) or (
                    not math.isnan(corr_ratio) and abs(corr_ratio - 1.0) <= args.compatibility_fraction
                ):
                    passing += 1
            point = points[row["pointId"]]
            ratios.append(
                {
                    "bin": label,
                    "pointId": row["pointId"],
                    "gridDistanceFromAN": point["gridDistanceFromAN"],
                    "alphaMax": row["alphaMax"],
                    "dthetaMax": row["dthetaMax"],
                    "isANRow": row["isANRow"],
                    "massWindowCount": row["massWindowCount"],
                    "sidebandSubtractedYield": fmt(raw),
                    "sidebandSubtractedYieldErr": fmt(raw_err),
                    "rawYieldRatioToAN": fmt(raw_ratio),
                    "rawYieldRatioToANErr": fmt(raw_ratio_err),
                    "signalEfficiency": row["signalEfficiency"],
                    "efficiencyCorrectedYield": fmt(corr),
                    "efficiencyCorrectedYieldErr": fmt(corr_err),
                    "correctedYieldRatioToAN": fmt(corr_ratio),
                    "correctedYieldRatioToANErr": fmt(corr_ratio_err),
                    "correctedYieldZFromAN": fmt(z),
                }
            )
        closure = (
            max_z <= args.compatibility_sigma
            or max_abs_shift <= args.compatibility_fraction
            or passing == tested
        )
        summary.append(
            {
                "bin": label,
                "neighborPointsTested": tested,
                "neighborsPassingCompatibility": passing,
                "maxAbsRawYieldRatioShift": fmt(max_abs_raw_shift),
                "maxAbsCorrectedYieldRatioShift": fmt(max_abs_shift),
                "maxCorrectedYieldZFromAN": fmt(max_z),
                "closurePass": str(closure).lower(),
            }
        )

    write_tsv(args.outdir / "pointing_closure_ratios.tsv", ratios)
    write_tsv(args.outdir / "pointing_closure_summary.tsv", summary)
    maybe_plot(args.outdir, ratios)
    write_summary_md(args, summary, ratios)


def maybe_plot(outdir: Path, ratios: list[dict[str, object]]) -> None:
    try:
        import matplotlib.pyplot as plt
        from PIL import Image, ImageOps
    except Exception as exc:  # pragma: no cover - optional plotting
        (outdir / "plot_warning.txt").write_text(f"plot dependencies unavailable: {exc}\n", encoding="utf-8")
        return

    plot_dir = outdir / "closure_plots"
    plot_dir.mkdir(exist_ok=True)
    by_bin: dict[str, list[dict[str, object]]] = defaultdict(list)
    for row in ratios:
        by_bin[str(row["bin"])].append(row)

    ratio_paths: list[Path] = []
    mass_paths: list[Path] = []
    hist_rows = read_tsv(outdir / "closure_mass_hist.tsv") if (outdir / "closure_mass_hist.tsv").exists() else []
    hist_by_bin: dict[str, list[dict[str, str]]] = defaultdict(list)
    for row in hist_rows:
        hist_by_bin[row["bin"]].append(row)

    for label in BIN_ORDER:
        rows = by_bin[label]
        rows.sort(key=lambda row: (int(row["gridDistanceFromAN"]), row["alphaMax"], row["dthetaMax"]))
        fig, ax = plt.subplots(figsize=(6.2, 4.0))
        xs = list(range(len(rows)))
        ys = [float(row["correctedYieldRatioToAN"]) for row in rows]
        yerr = [float(row["correctedYieldRatioToANErr"]) for row in rows]
        colors = ["red" if row["isANRow"] == "true" else "black" for row in rows]
        ax.errorbar(xs, ys, yerr=yerr, fmt="none", ecolor="0.45", capsize=2, linewidth=0.9)
        ax.scatter(xs, ys, c=colors, s=34, zorder=3)
        ax.axhline(1.0, color="red", linewidth=1.0)
        ax.axhspan(0.85, 1.15, color="green", alpha=0.10, linewidth=0)
        ax.set_ylim(max(0.0, min(ys) - 0.25), max(1.4, max(ys) + 0.25))
        ax.set_title(label)
        ax.set_ylabel("efficiency-corrected yield / AN row")
        ax.set_xticks(xs)
        ax.set_xticklabels([f"a<{row['alphaMax']}\ndt<{row['dthetaMax']}" for row in rows], rotation=45, ha="right", fontsize=7)
        ax.grid(alpha=0.25, axis="y")
        fig.tight_layout()
        path = plot_dir / f"{label}_corrected_yield_ratio.png"
        fig.savefig(path, dpi=160)
        plt.close(fig)
        ratio_paths.append(path)

        if hist_by_bin[label]:
            fig, ax = plt.subplots(figsize=(6.2, 4.0))
            point_ids = [str(row["pointId"]) for row in rows]
            an_id = next(str(row["pointId"]) for row in rows if row["isANRow"] == "true")
            for point_id in point_ids:
                hrows = [row for row in hist_by_bin[label] if row["pointId"] == point_id]
                if not hrows:
                    continue
                centers = [(float(row["massBinLow"]) + float(row["massBinHigh"])) / 2.0 for row in hrows]
                counts = [float(row["count"]) for row in hrows]
                if point_id == an_id:
                    ax.step(centers, counts, where="mid", color="red", linewidth=1.7, label="AN row")
                else:
                    ax.step(centers, counts, where="mid", color="0.55", alpha=0.35, linewidth=0.8)
            ax.axvline(1.86484, color="black", linestyle=":", linewidth=1)
            ax.set_title(label)
            ax.set_xlabel(r"$m_{K\pi}$ [GeV]")
            ax.set_ylabel("candidates / bin")
            ax.grid(alpha=0.2)
            ax.legend(fontsize=8)
            fig.tight_layout()
            path = plot_dir / f"{label}_mass_overlay.png"
            fig.savefig(path, dpi=160)
            plt.close(fig)
            mass_paths.append(path)

    make_grid(ratio_paths, plot_dir / "corrected_yield_ratio_grid.png")
    make_grid(mass_paths, plot_dir / "mass_overlay_grid.png")


def make_grid(paths: list[Path], out: Path) -> None:
    if not paths:
        return
    from PIL import Image, ImageOps

    images = [Image.open(path).convert("RGB") for path in paths]
    width = max(image.width for image in images)
    height = max(image.height for image in images)
    pad = 24
    canvas = Image.new("RGB", (3 * width + 4 * pad, 2 * height + 3 * pad), "white")
    for idx, image in enumerate(images):
        row, col = divmod(idx, 3)
        canvas.paste(ImageOps.contain(image, (width, height)), (pad + col * (width + pad), pad + row * (height + pad)))
    canvas.save(out, quality=95)


def write_summary_md(args: argparse.Namespace, summary: list[dict[str, object]], ratios: list[dict[str, object]]) -> None:
    passed = sum(1 for row in summary if row["closurePass"] == "true")
    with (args.outdir / "pointing_closure_summary.md").open("w", encoding="utf-8") as out:
        out.write("# D0 Pointing-Cut Closure\n\n")
        out.write("<!-- DOC_OWNER: cms-analysis-manager pointing-cut closure. -->\n")
        out.write("<!-- TOKEN_NOTE: post-derivation mass/yield closure; do not use to choose cuts. -->\n\n")
        out.write("## Method\n\n")
        out.write("- Start from accepted neighboring points on the frozen `Dalpha`/`Ddtheta` plateau.\n")
        out.write("- Keep event cuts, track cuts, `DsvpvSig`, and `Dchi2cl` fixed.\n")
        out.write("- On 2023 data, compare mass-window counts and sideband-subtracted yields.\n")
        out.write("- Correct the raw sideband-subtracted yield by the MC signal efficiency from the frozen topology scan.\n")
        out.write(
            f"- Treat a neighbor as compatible if the corrected-yield shift is within `{args.compatibility_sigma:g}` sigma "
            f"or within `{100 * args.compatibility_fraction:g}%` of the AN row.\n\n"
        )
        out.write("This is a robustness/closure check, not the final AN mass-fit model.\n\n")
        out.write("## Result\n\n")
        out.write(f"`{passed}/6` bins pass the neighboring-plateau closure criterion.\n\n")
        out.write("| bin | neighbors tested | passing | max raw shift | max corrected shift | max corrected-yield z | closure |\n")
        out.write("|---|---:|---:|---:|---:|---:|---:|\n")
        for row in summary:
            out.write(
                f"| {row['bin']} | {row['neighborPointsTested']} | {row['neighborsPassingCompatibility']} | "
                f"{float(row['maxAbsRawYieldRatioShift']):.3f} | {float(row['maxAbsCorrectedYieldRatioShift']):.3f} | "
                f"{float(row['maxCorrectedYieldZFromAN']):.2f} | {row['closurePass']} |\n"
            )
        out.write("\n## Interpretation\n\n")
        if passed == 6:
            out.write(
                "The AN pointing rows are stable against nearby accepted plateau choices under this diagnostic closure. "
                "This strengthens the a-priori justification: the exact row is not just accepted by the plateau rule; "
                "nearby accepted rows give statistically compatible efficiency-corrected yields.\n\n"
            )
        else:
            out.write(
                "At least one bin shows non-negligible yield movement across nearby accepted plateau choices. "
                "That does not invalidate the AN row, but it means topology-cut variation should be carried as a "
                "systematic until a final blinded mass-fit closure is run.\n\n"
            )
        out.write("Remaining limitations: the topology sideband definition and the diagnostic mass-window method are not yet independently derived final-yield machinery.\n\n")
        out.write("## Outputs\n\n")
        out.write("- `closure_points.tsv`\n")
        out.write("- `closure_yields.tsv`\n")
        out.write("- `closure_mass_hist.tsv`\n")
        out.write("- `pointing_closure_ratios.tsv`\n")
        out.write("- `pointing_closure_summary.tsv`\n")
        if (args.outdir / "closure_plots" / "corrected_yield_ratio_grid.png").exists():
            out.write("- `closure_plots/corrected_yield_ratio_grid.png`\n")
        if (args.outdir / "closure_plots" / "mass_overlay_grid.png").exists():
            out.write("- `closure_plots/mass_overlay_grid.png`\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pointing-dir", type=Path, default=DEFAULT_POINTING)
    parser.add_argument("--data-sideband-scan", type=Path, default=DEFAULT_DATA_SCAN)
    parser.add_argument("--floor-derivation", type=Path, default=DEFAULT_FLOORS)
    parser.add_argument("--outdir", type=Path, default=DEFAULT_OUTDIR)
    parser.add_argument("--max-grid-distance", type=int, default=2)
    parser.add_argument("--max-points-per-bin", type=int, default=10)
    parser.add_argument("--compatibility-sigma", type=float, default=2.0)
    parser.add_argument("--compatibility-fraction", type=float, default=0.15)
    parser.add_argument("--prepare-only", action="store_true")
    parser.add_argument("--summarize-only", action="store_true")
    args = parser.parse_args()

    if not args.summarize_only:
        path = prepare(args)
        print(f"Wrote {path}")
    if not args.prepare_only:
        summarize(args)
        if (args.outdir / "pointing_closure_summary.md").exists():
            print(f"Wrote {args.outdir / 'pointing_closure_summary.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
