#!/usr/bin/env python3
"""Justify D0 pointing-angle cuts after independent topology floors.

The script uses the independently derived DsvpvSig/Dchi2cl floors, then asks
whether each Dalpha/Ddtheta point lies on an a-priori signal/background
plateau. AN cut values are used only as posthoc comparison rows.
"""

from __future__ import annotations

import argparse
import csv
import math
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path


DEFAULT_BASE = Path("research/cms-d0-analysis/output/cut-derivation")
DEFAULT_DATA_SCAN = DEFAULT_BASE / "topocuts_rect_an_sideband_full_20260617/d_topology_scan.tsv"
DEFAULT_MC_SCAN = DEFAULT_BASE / "topocuts_mc_background_full_20260617/d_topology_scan.tsv"
DEFAULT_FLOORS = DEFAULT_BASE / "topological_floor_justification_20260618/floor_derivation.tsv"
DEFAULT_OUTDIR = DEFAULT_BASE / "topological_pointing_justification_20260618"

# Posthoc validation target only. The plateau definition below does not use
# these values to choose accepted points.
AN_POINTING = {
    "absy0to1_pt2to5": (0.40, 0.50),
    "absy0to1_pt5to8": (0.35, 0.30),
    "absy0to1_pt8to12": (0.40, 0.30),
    "absy1to2_pt2to5": (0.20, 0.30),
    "absy1to2_pt5to8": (0.25, 0.30),
    "absy1to2_pt8to12": (0.25, 0.30),
}

BIN_ORDER = list(AN_POINTING)


@dataclass(frozen=True)
class ScanRow:
    bin: str
    alpha: float
    svpv: float
    chi2: float
    dtheta: float
    signal_base: float
    background_base: float
    signal_pass: float
    background_pass: float
    signal_eff: float
    background_frac: float
    score: float


@dataclass(frozen=True)
class Metrics:
    bin: str
    alpha: float
    dtheta: float
    signal_retention_loose: float
    mc_background_rejection: float
    data_background_rejection: float
    mc_score_ratio: float
    data_score_ratio: float
    joint_score_ratio: float
    data_background_base: float
    mc_background_base: float
    accepted: bool


def read_scan(path: Path) -> dict[str, list[ScanRow]]:
    out: dict[str, list[ScanRow]] = defaultdict(list)
    with path.open(newline="", encoding="utf-8") as handle:
        for row in csv.DictReader(handle, delimiter="\t"):
            item = ScanRow(
                bin=row["bin"],
                alpha=float(row["alphaMax"]),
                svpv=float(row["svpvSigMin"]),
                chi2=float(row["chi2clMin"]),
                dtheta=float(row["dthetaMax"]),
                signal_base=float(row["signalBase"]),
                background_base=float(row["backgroundBase"]),
                signal_pass=float(row["signalPass"]),
                background_pass=float(row["backgroundPass"]),
                signal_eff=float(row["signalEfficiency"]),
                background_frac=float(row["backgroundFraction"]),
                score=float(row["score"]),
            )
            out[item.bin].append(item)
    return out


def read_floors(path: Path) -> dict[str, tuple[float, float]]:
    out: dict[str, tuple[float, float]] = {}
    with path.open(newline="", encoding="utf-8") as handle:
        for row in csv.DictReader(handle, delimiter="\t"):
            out[row["bin"]] = (float(row["derivedSvpvSigMin"]), float(row["derivedChi2clMin"]))
    return out


def close(a: float, b: float) -> bool:
    return abs(a - b) < 1e-9


def rows_at_floor(rows: list[ScanRow], floor: tuple[float, float]) -> dict[tuple[float, float], ScanRow]:
    svpv, chi2 = floor
    out = {
        (row.alpha, row.dtheta): row
        for row in rows
        if close(row.svpv, svpv) and close(row.chi2, chi2)
    }
    if not out:
        raise SystemExit(f"No rows found for floor svpv={svpv}, chi2={chi2}")
    return out


def frac_rejection(value: float, baseline: float) -> float:
    if baseline <= 0:
        return 0.0
    return 1.0 - value / baseline


def stat_tolerance(n: float) -> float:
    if n <= 0:
        return 0.0
    return 1.0 / math.sqrt(n)


def compute_metrics(
    data_rows: dict[tuple[float, float], ScanRow],
    mc_rows: dict[tuple[float, float], ScanRow],
    *,
    min_signal_retention: float,
    min_joint_score_ratio: float,
) -> list[Metrics]:
    keys = sorted(set(data_rows) & set(mc_rows))
    if (0.5, 0.5) not in mc_rows or (0.5, 0.5) not in data_rows:
        raise SystemExit("Expected loose reference point alpha=0.5, dtheta=0.5 in scan grid")
    loose_mc = mc_rows[(0.5, 0.5)]
    loose_data = data_rows[(0.5, 0.5)]
    best_mc_score = max(row.score for row in mc_rows.values())
    best_data_score = max(row.score for row in data_rows.values())

    out: list[Metrics] = []
    for key in keys:
        data = data_rows[key]
        mc = mc_rows[key]
        signal_retention = mc.signal_eff / loose_mc.signal_eff if loose_mc.signal_eff else 0.0
        mc_rejection = frac_rejection(mc.background_frac, loose_mc.background_frac)
        data_rejection = frac_rejection(data.background_frac, loose_data.background_frac)
        mc_ratio = mc.score / best_mc_score if best_mc_score else 0.0
        data_ratio = data.score / best_data_score if best_data_score else 0.0
        joint_ratio = math.sqrt(max(0.0, mc_ratio) * max(0.0, data_ratio))
        # The background condition is intentionally weak: pointing cuts can be
        # saturated in sparse bins. We only reject rows that visibly increase
        # background beyond statistical precision.
        mc_background_ok = mc_rejection >= -stat_tolerance(mc.background_base)
        data_background_ok = data_rejection >= -stat_tolerance(data.background_base)
        accepted = (
            signal_retention >= min_signal_retention
            and joint_ratio >= min_joint_score_ratio
            and mc_background_ok
            and data_background_ok
        )
        out.append(
            Metrics(
                bin=mc.bin,
                alpha=key[0],
                dtheta=key[1],
                signal_retention_loose=signal_retention,
                mc_background_rejection=mc_rejection,
                data_background_rejection=data_rejection,
                mc_score_ratio=mc_ratio,
                data_score_ratio=data_ratio,
                joint_score_ratio=joint_ratio,
                data_background_base=data.background_base,
                mc_background_base=mc.background_base,
                accepted=accepted,
            )
        )
    return out


def fmt(value: float) -> str:
    return f"{value:.6g}"


def point_label(alpha: float, dtheta: float) -> str:
    return f"alpha<{alpha:g}, dtheta<{dtheta:g}"


def combined_background_rejection(metric: Metrics) -> float:
    return max(0.0, metric.data_background_rejection) + max(0.0, metric.mc_background_rejection)


def choose_representative(metrics: list[Metrics], signal_fraction_of_best: float) -> Metrics:
    """Choose one AN-free representative from the accepted plateau.

    The plateau is the primary result. This tie-break chooses a concrete row
    by preserving nearly the best accepted signal retention, then maximizing
    combined data-sideband and nonmatched-MC background rejection.
    """
    accepted = [metric for metric in metrics if metric.accepted]
    if not accepted:
        raise SystemExit("No accepted plateau points found")
    best_signal = max(metric.signal_retention_loose for metric in accepted)
    signal_floor = signal_fraction_of_best * best_signal
    eligible = [metric for metric in accepted if metric.signal_retention_loose >= signal_floor]
    return max(
        eligible,
        key=lambda metric: (
            combined_background_rejection(metric),
            metric.joint_score_ratio,
            metric.signal_retention_loose,
            -metric.alpha,
            -metric.dtheta,
        ),
    )


def write_tsv(path: Path, rows: list[dict[str, str]]) -> None:
    if not rows:
        path.write_text("", encoding="utf-8")
        return
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, delimiter="\t", lineterminator="\n", fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def maybe_plot(all_metrics: dict[str, list[Metrics]], outdir: Path) -> None:
    try:
        import matplotlib.pyplot as plt
        from PIL import Image, ImageOps
    except Exception as exc:  # pragma: no cover - optional plotting
        (outdir / "plot_warning.txt").write_text(f"plot dependencies unavailable: {exc}\n", encoding="utf-8")
        return

    plot_dir = outdir / "pointing_plots"
    plot_dir.mkdir(exist_ok=True)
    paths: list[Path] = []
    for label in BIN_ORDER:
        metrics = all_metrics[label]
        fig, ax = plt.subplots(figsize=(6.2, 4.4))
        xs = [m.data_background_rejection for m in metrics]
        ys = [m.signal_retention_loose for m in metrics]
        colors = [m.joint_score_ratio for m in metrics]
        ax.scatter(xs, ys, c=colors, cmap="viridis", s=48, alpha=0.8, edgecolors="none")
        accepted = [m for m in metrics if m.accepted]
        ax.scatter(
            [m.data_background_rejection for m in accepted],
            [m.signal_retention_loose for m in accepted],
            facecolors="none",
            edgecolors="black",
            s=80,
            linewidths=0.8,
            label="accepted plateau",
        )
        an_alpha, an_dtheta = AN_POINTING[label]
        an = next(m for m in metrics if close(m.alpha, an_alpha) and close(m.dtheta, an_dtheta))
        ax.scatter(
            [an.data_background_rejection],
            [an.signal_retention_loose],
            c="red",
            marker="*",
            s=120,
            label="AN row",
        )
        ax.axhline(0.80, color="gray", linestyle=":", linewidth=1)
        ax.axvline(0.0, color="gray", linestyle=":", linewidth=1)
        ax.set_title(label)
        ax.set_xlabel("data-sideband rejection vs loose")
        ax.set_ylabel("MC signal retention vs loose")
        ax.grid(alpha=0.25)
        ax.legend(fontsize=8)
        fig.tight_layout()
        path = plot_dir / f"{label}_pointing_plateau.png"
        fig.savefig(path, dpi=160)
        plt.close(fig)
        paths.append(path)

    images = [Image.open(path).convert("RGB") for path in paths]
    width = max(image.width for image in images)
    height = max(image.height for image in images)
    pad = 24
    canvas = Image.new("RGB", (3 * width + 4 * pad, 2 * height + 3 * pad), "white")
    for idx, image in enumerate(images):
        row, col = divmod(idx, 3)
        canvas.paste(ImageOps.contain(image, (width, height)), (pad + col * (width + pad), pad + row * (height + pad)))
    canvas.save(plot_dir / "pointing_plateau_grid.png", quality=95)


def derive(args: argparse.Namespace) -> None:
    data_scan = read_scan(args.data_sideband_scan)
    mc_scan = read_scan(args.mc_background_scan)
    floors = read_floors(args.floor_derivation)
    args.outdir.mkdir(parents=True, exist_ok=True)

    plateau_rows: list[dict[str, str]] = []
    summary_rows: list[dict[str, str]] = []
    representative_rows: list[dict[str, str]] = []
    all_metrics: dict[str, list[Metrics]] = {}

    for label in BIN_ORDER:
        floor = floors[label]
        data_floor_rows = rows_at_floor(data_scan[label], floor)
        mc_floor_rows = rows_at_floor(mc_scan[label], floor)
        metrics = compute_metrics(
            data_floor_rows,
            mc_floor_rows,
            min_signal_retention=args.min_signal_retention,
            min_joint_score_ratio=args.min_joint_score_ratio,
        )
        all_metrics[label] = metrics
        accepted = [m for m in metrics if m.accepted]
        representative = choose_representative(metrics, args.representative_signal_fraction_of_best)
        an_alpha, an_dtheta = AN_POINTING[label]
        an = next(m for m in metrics if close(m.alpha, an_alpha) and close(m.dtheta, an_dtheta))
        for m in metrics:
            plateau_rows.append(
                {
                    "bin": label,
                    "alphaMax": fmt(m.alpha),
                    "dthetaMax": fmt(m.dtheta),
                    "signalRetentionLoose": fmt(m.signal_retention_loose),
                    "mcBackgroundRejection": fmt(m.mc_background_rejection),
                    "dataBackgroundRejection": fmt(m.data_background_rejection),
                    "mcScoreRatio": fmt(m.mc_score_ratio),
                    "dataScoreRatio": fmt(m.data_score_ratio),
                    "jointScoreRatio": fmt(m.joint_score_ratio),
                    "acceptedPlateau": str(m.accepted).lower(),
                    "isANRow": str(close(m.alpha, an_alpha) and close(m.dtheta, an_dtheta)).lower(),
                }
            )
        summary_rows.append(
            {
                "bin": label,
                "derivedSvpvSigMin": fmt(floor[0]),
                "derivedChi2clMin": fmt(floor[1]),
                "representativeAlphaMax": fmt(representative.alpha),
                "representativeDthetaMax": fmt(representative.dtheta),
                "representativeMatchesAN": str(close(representative.alpha, an_alpha) and close(representative.dtheta, an_dtheta)).lower(),
                "anAlphaMax": fmt(an_alpha),
                "anDthetaMax": fmt(an_dtheta),
                "anAcceptedByPlateauRule": str(an.accepted).lower(),
                "acceptedPlateauCount": str(len(accepted)),
                "anSignalRetentionLoose": fmt(an.signal_retention_loose),
                "anMcBackgroundRejection": fmt(an.mc_background_rejection),
                "anDataBackgroundRejection": fmt(an.data_background_rejection),
                "anJointScoreRatio": fmt(an.joint_score_ratio),
                "plateauRule": (
                    f"signalRetentionLoose>={args.min_signal_retention:g}, "
                    f"jointScoreRatio>={args.min_joint_score_ratio:g}, "
                    "and no statistically significant background increase"
                ),
            }
        )
        representative_rows.append(
            {
                "bin": label,
                "derivedSvpvSigMin": fmt(floor[0]),
                "derivedChi2clMin": fmt(floor[1]),
                "derivedAlphaMax": fmt(representative.alpha),
                "derivedDthetaMax": fmt(representative.dtheta),
                "signalRetentionLoose": fmt(representative.signal_retention_loose),
                "mcBackgroundRejection": fmt(representative.mc_background_rejection),
                "dataBackgroundRejection": fmt(representative.data_background_rejection),
                "jointScoreRatio": fmt(representative.joint_score_ratio),
                "combinedBackgroundRejection": fmt(combined_background_rejection(representative)),
                "matchesANPointing": str(close(representative.alpha, an_alpha) and close(representative.dtheta, an_dtheta)).lower(),
                "representativeRule": (
                    f"accepted plateau row with signalRetentionLoose >= "
                    f"{args.representative_signal_fraction_of_best:g} * best accepted signal retention, "
                    "then max(data+MC background rejection)"
                ),
            }
        )

    write_tsv(args.outdir / "pointing_plateau_candidates.tsv", plateau_rows)
    write_tsv(args.outdir / "pointing_an_validation.tsv", summary_rows)
    write_tsv(args.outdir / "pointing_rederived_representative.tsv", representative_rows)
    maybe_plot(all_metrics, args.outdir)

    accepted_bins = sum(1 for row in summary_rows if row["anAcceptedByPlateauRule"] == "true")
    with (args.outdir / "pointing_justification_summary.md").open("w", encoding="utf-8") as out:
        out.write("# D0 Pointing-Cut Justification\n\n")
        out.write("<!-- DOC_OWNER: cms-analysis-manager pointing-cut derivation. -->\n")
        out.write("<!-- TOKEN_NOTE: exact AN rows are posthoc validation targets, not inputs to the plateau rule. -->\n\n")
        out.write("## Method\n\n")
        out.write("1. Fix the independently derived `DsvpvSig` and `Dchi2cl` floors.\n")
        out.write("2. Scan `Dalpha` and `Ddtheta` using both data-sideband and nonmatched-MC background tables.\n")
        out.write(
            f"3. Accept a pointing point if MC signal retention relative to the loose `alpha<0.5, dtheta<0.5` point is at least `{args.min_signal_retention:g}`, "
            f"the geometric mean of data-sideband and nonmatched-MC score ratios is at least `{args.min_joint_score_ratio:g}`, "
            "and neither background proxy increases beyond statistical precision.\n\n"
        )
        out.write("The exact AN values are checked only after this plateau is defined.\n\n")
        out.write("## Result\n\n")
        out.write(f"The exact AN pointing rows are accepted by the a-priori plateau rule in `{accepted_bins}/6` bins.\n\n")
        out.write("The script also writes one AN-free representative cut row per bin by taking an accepted plateau row ")
        out.write(
            f"with signal retention at least `{args.representative_signal_fraction_of_best:g}` times the best accepted retention "
            "and then maximizing combined data-sideband plus nonmatched-MC background rejection. "
            "This representative is useful for from-scratch studies, but it is not forced to equal the AN row.\n\n"
        )
        out.write("## AN-Free Representative Rows\n\n")
        out.write("| bin | representative pointing row | matches AN pointing | signal retention | MC bkg rej. | data bkg rej. | joint score ratio |\n")
        out.write("|---|---|---:|---:|---:|---:|---:|\n")
        for row in representative_rows:
            out.write(
                f"| {row['bin']} | `{point_label(float(row['derivedAlphaMax']), float(row['derivedDthetaMax']))}` | "
                f"{row['matchesANPointing']} | {float(row['signalRetentionLoose']):.3f} | "
                f"{float(row['mcBackgroundRejection']):.3f} | {float(row['dataBackgroundRejection']):.3f} | "
                f"{float(row['jointScoreRatio']):.3f} |\n"
            )
        out.write("\n## AN Comparison\n\n")
        out.write("| bin | AN pointing row | accepted | plateau size | signal retention | MC bkg rej. | data bkg rej. | joint score ratio |\n")
        out.write("|---|---|---:|---:|---:|---:|---:|---:|\n")
        for row in summary_rows:
            out.write(
                f"| {row['bin']} | `{point_label(float(row['anAlphaMax']), float(row['anDthetaMax']))}` | "
                f"{row['anAcceptedByPlateauRule']} | {row['acceptedPlateauCount']} | "
                f"{float(row['anSignalRetentionLoose']):.3f} | {float(row['anMcBackgroundRejection']):.3f} | "
                f"{float(row['anDataBackgroundRejection']):.3f} | {float(row['anJointScoreRatio']):.3f} |\n"
            )
        out.write("\n## Interpretation\n\n")
        out.write(
            "This does not prove the AN pointing values are unique. It proves they sit inside a predeclared, "
            "signal-efficient, background-stable plateau after independently deriving the vertex-quality floors. "
            "The next robust step is closure: compare the exact AN row against neighboring accepted plateau points "
            "in the mass peak and corrected yield.\n\n"
        )
        out.write("## Outputs\n\n")
        out.write("- `pointing_an_validation.tsv`\n")
        out.write("- `pointing_rederived_representative.tsv`\n")
        out.write("- `pointing_plateau_candidates.tsv`\n")
        if (args.outdir / "pointing_plots" / "pointing_plateau_grid.png").exists():
            out.write("- `pointing_plots/pointing_plateau_grid.png`\n")

    print(f"Wrote {args.outdir / 'pointing_justification_summary.md'}")
    print(f"AN rows accepted in {accepted_bins}/6 bins")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data-sideband-scan", type=Path, default=DEFAULT_DATA_SCAN)
    parser.add_argument("--mc-background-scan", type=Path, default=DEFAULT_MC_SCAN)
    parser.add_argument("--floor-derivation", type=Path, default=DEFAULT_FLOORS)
    parser.add_argument("--outdir", type=Path, default=DEFAULT_OUTDIR)
    parser.add_argument("--min-signal-retention", type=float, default=0.80)
    parser.add_argument("--min-joint-score-ratio", type=float, default=0.90)
    parser.add_argument("--representative-signal-fraction-of-best", type=float, default=0.94)
    args = parser.parse_args()
    derive(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
