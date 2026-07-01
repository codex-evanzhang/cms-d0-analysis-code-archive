#!/usr/bin/env python3
"""Derive D0 topology quality floors from scan tables.

This script deliberately does not use the AN cut values to select the floors.
Nominal AN values are included only as a final comparison column.
"""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path


NOMINAL = {
    "absy0to1_pt2to5": (2.5, 0.10),
    "absy0to1_pt5to8": (3.5, 0.10),
    "absy0to1_pt8to12": (3.5, 0.10),
    "absy1to2_pt2to5": (2.5, 0.10),
    "absy1to2_pt5to8": (3.5, 0.10),
    "absy1to2_pt8to12": (3.5, 0.10),
}

BIN_LABEL = {
    "absy0to1_pt2to5": r"$0\leq |y|<1,\ 2<p_T<5$",
    "absy0to1_pt5to8": r"$0\leq |y|<1,\ 5<p_T<8$",
    "absy0to1_pt8to12": r"$0\leq |y|<1,\ 8<p_T<12$",
    "absy1to2_pt2to5": r"$1\leq |y|\leq2,\ 2<p_T<5$",
    "absy1to2_pt5to8": r"$1\leq |y|\leq2,\ 5<p_T<8$",
    "absy1to2_pt8to12": r"$1\leq |y|\leq2,\ 8<p_T<12$",
}


@dataclass(frozen=True)
class Row:
    bin: str
    alpha: float
    svpv: float
    chi2: float
    dtheta: float
    sig_eff: float
    bkg_frac: float
    score: float


def read_scan(path: Path) -> dict[str, list[Row]]:
    out: dict[str, list[Row]] = {}
    with path.open(newline="", encoding="utf-8") as handle:
        for raw in csv.DictReader(handle, delimiter="\t"):
            row = Row(
                bin=raw["bin"],
                alpha=float(raw["alphaMax"]),
                svpv=float(raw["svpvSigMin"]),
                chi2=float(raw["chi2clMin"]),
                dtheta=float(raw["dthetaMax"]),
                sig_eff=float(raw["signalEfficiency"]),
                bkg_frac=float(raw["backgroundFraction"]),
                score=float(raw["score"]),
            )
            out.setdefault(row.bin, []).append(row)
    return out


def close(a: float, b: float) -> bool:
    return abs(a - b) < 1e-9


def floor_row(rows: list[Row], svpv: float, chi2: float, alpha: float, dtheta: float) -> Row:
    for row in rows:
        if close(row.svpv, svpv) and close(row.chi2, chi2) and close(row.alpha, alpha) and close(row.dtheta, dtheta):
            return row
    raise KeyError((svpv, chi2, alpha, dtheta))


def frac_reduction(value: float, baseline: float) -> float:
    if baseline <= 0:
        return 0.0
    return 1.0 - value / baseline


def fmt(value: float) -> str:
    return f"{value:.6g}"


def derive(args: argparse.Namespace) -> None:
    data_scan = read_scan(args.data_sideband_scan)
    mc_scan = read_scan(args.mc_background_scan)
    bins = [label for label in NOMINAL if label in data_scan and label in mc_scan]
    if len(bins) != len(NOMINAL):
        missing = sorted(set(NOMINAL) - set(bins))
        raise SystemExit(f"Missing bins in input scans: {missing}")

    args.outdir.mkdir(parents=True, exist_ok=True)
    profile_rows: list[dict[str, str]] = []
    summary_rows: list[dict[str, str]] = []

    for label in bins:
        data_rows = data_scan[label]
        mc_rows = mc_scan[label]
        alpha_loose = max(row.alpha for row in mc_rows)
        dtheta_loose = max(row.dtheta for row in mc_rows)
        svpv_values = sorted({row.svpv for row in mc_rows})
        chi2_values = sorted({row.chi2 for row in mc_rows})
        svpv_min = svpv_values[0]
        chi2_min = chi2_values[0]

        mc_chi2_base = floor_row(mc_rows, svpv_min, chi2_min, alpha_loose, dtheta_loose)
        data_chi2_base = floor_row(data_rows, svpv_min, chi2_min, alpha_loose, dtheta_loose)
        chosen_chi2: float | None = None
        chi2_reason = ""

        for chi2 in chi2_values[1:]:
            mc_row = floor_row(mc_rows, svpv_min, chi2, alpha_loose, dtheta_loose)
            data_row = floor_row(data_rows, svpv_min, chi2, alpha_loose, dtheta_loose)
            sig_loss = frac_reduction(mc_row.sig_eff, mc_chi2_base.sig_eff)
            mc_bkg_rej = frac_reduction(mc_row.bkg_frac, mc_chi2_base.bkg_frac)
            data_bkg_rej = frac_reduction(data_row.bkg_frac, data_chi2_base.bkg_frac)
            if chosen_chi2 is None and mc_bkg_rej >= args.min_chi2_mc_bkg_rejection and sig_loss <= args.max_chi2_signal_loss:
                chosen_chi2 = chi2
                chi2_reason = (
                    f"first chi2 floor with MC background rejection >= {args.min_chi2_mc_bkg_rejection:g} "
                    f"and signal loss <= {args.max_chi2_signal_loss:g}"
                )
            profile_rows.append(
                {
                    "bin": label,
                    "profile": "chi2_at_loose_svpv",
                    "svpvSigMin": fmt(svpv_min),
                    "chi2clMin": fmt(chi2),
                    "signalRetention": fmt(mc_row.sig_eff / mc_chi2_base.sig_eff if mc_chi2_base.sig_eff else 0.0),
                    "signalLoss": fmt(sig_loss),
                    "mcBackgroundRejection": fmt(mc_bkg_rej),
                    "dataBackgroundRejection": fmt(data_bkg_rej),
                    "selected": "pending",
                }
            )

        if chosen_chi2 is None:
            chosen_chi2 = chi2_min
            chi2_reason = "no stricter chi2 floor passed the independent criterion"

        mc_svpv_base = floor_row(mc_rows, svpv_min, chosen_chi2, alpha_loose, dtheta_loose)
        data_svpv_base = floor_row(data_rows, svpv_min, chosen_chi2, alpha_loose, dtheta_loose)
        chosen_svpv: float | None = None
        svpv_reason = ""
        selected_profile_index: int | None = None

        for svpv in svpv_values:
            mc_row = floor_row(mc_rows, svpv, chosen_chi2, alpha_loose, dtheta_loose)
            data_row = floor_row(data_rows, svpv, chosen_chi2, alpha_loose, dtheta_loose)
            sig_ret = mc_row.sig_eff / mc_svpv_base.sig_eff if mc_svpv_base.sig_eff else 0.0
            mc_bkg_rej = frac_reduction(mc_row.bkg_frac, mc_svpv_base.bkg_frac)
            data_bkg_rej = frac_reduction(data_row.bkg_frac, data_svpv_base.bkg_frac)
            if (
                chosen_svpv is None
                and mc_bkg_rej >= args.min_svpv_mc_bkg_rejection
                and data_bkg_rej >= args.min_svpv_data_bkg_rejection
                and sig_ret >= args.min_svpv_signal_retention
            ):
                chosen_svpv = svpv
                selected_profile_index = len(profile_rows)
                svpv_reason = (
                    f"first svpv floor with MC background rejection >= {args.min_svpv_mc_bkg_rejection:g}, "
                    f"data-sideband rejection >= {args.min_svpv_data_bkg_rejection:g}, "
                    f"and signal retention >= {args.min_svpv_signal_retention:g}"
                )
            profile_rows.append(
                {
                    "bin": label,
                    "profile": "svpv_at_selected_chi2",
                    "svpvSigMin": fmt(svpv),
                    "chi2clMin": fmt(chosen_chi2),
                    "signalRetention": fmt(sig_ret),
                    "signalLoss": fmt(1.0 - sig_ret),
                    "mcBackgroundRejection": fmt(mc_bkg_rej),
                    "dataBackgroundRejection": fmt(data_bkg_rej),
                    "selected": "false",
                }
            )

        if chosen_svpv is None:
            # Fall back to the strictest value that preserves the majority of signal.
            viable = []
            for svpv in svpv_values:
                mc_row = floor_row(mc_rows, svpv, chosen_chi2, alpha_loose, dtheta_loose)
                sig_ret = mc_row.sig_eff / mc_svpv_base.sig_eff if mc_svpv_base.sig_eff else 0.0
                if sig_ret >= args.min_svpv_signal_retention:
                    viable.append(svpv)
            chosen_svpv = viable[-1] if viable else svpv_min
            svpv_reason = "fallback: strictest svpv floor retaining the required signal fraction"

        if selected_profile_index is not None:
            profile_rows[selected_profile_index]["selected"] = "true"
        for row in profile_rows:
            if row["bin"] == label and row["profile"] == "chi2_at_loose_svpv" and close(float(row["chi2clMin"]), chosen_chi2):
                row["selected"] = "true"

        selected_mc = floor_row(mc_rows, chosen_svpv, chosen_chi2, alpha_loose, dtheta_loose)
        selected_data = floor_row(data_rows, chosen_svpv, chosen_chi2, alpha_loose, dtheta_loose)
        nominal_svpv, nominal_chi2 = NOMINAL[label]
        summary_rows.append(
            {
                "bin": label,
                "derivedSvpvSigMin": fmt(chosen_svpv),
                "derivedChi2clMin": fmt(chosen_chi2),
                "nominalSvpvSigMin": fmt(nominal_svpv),
                "nominalChi2clMin": fmt(nominal_chi2),
                "matchesNominalFloors": str(close(chosen_svpv, nominal_svpv) and close(chosen_chi2, nominal_chi2)).lower(),
                "looseAlphaMax": fmt(alpha_loose),
                "looseDthetaMax": fmt(dtheta_loose),
                "signalRetentionAtFloor": fmt(selected_mc.sig_eff / mc_svpv_base.sig_eff if mc_svpv_base.sig_eff else 0.0),
                "mcBackgroundRejectionAtFloor": fmt(frac_reduction(selected_mc.bkg_frac, mc_svpv_base.bkg_frac)),
                "dataBackgroundRejectionAtFloor": fmt(frac_reduction(selected_data.bkg_frac, data_svpv_base.bkg_frac)),
                "chi2Reason": chi2_reason,
                "svpvReason": svpv_reason,
            }
        )

    write_tsv(args.outdir / "floor_profiles.tsv", profile_rows)
    write_tsv(args.outdir / "floor_derivation.tsv", summary_rows)
    write_summary(args, summary_rows)
    maybe_plot(args.outdir, profile_rows, summary_rows)
    print(f"wrote {args.outdir / 'floor_derivation.tsv'}")
    print(f"wrote {args.outdir / 'floor_justification_summary.md'}")


def write_tsv(path: Path, rows: list[dict[str, str]]) -> None:
    if not rows:
        path.write_text("", encoding="utf-8")
        return
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, delimiter="\t", lineterminator="\n", fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def write_summary(args: argparse.Namespace, rows: list[dict[str, str]]) -> None:
    matched = sum(row["matchesNominalFloors"] == "true" for row in rows)
    lines = [
        "# D0 Topological Floor Justification",
        "",
        "This derivation chooses only the `DsvpvSig` and `Dchi2cl` quality floors.",
        "The AN values are not used by the selection rule; they appear only as a final comparison.",
        "",
        "## Rule",
        "",
        "- Use loose `Dalpha` and `Ddtheta` so the scan isolates the floor variables.",
        f"- Choose the first `Dchi2cl` floor with MC-background rejection >= `{args.min_chi2_mc_bkg_rejection:g}` and MC signal loss <= `{args.max_chi2_signal_loss:g}`.",
        f"- Then choose the first `DsvpvSig` floor with MC-background rejection >= `{args.min_svpv_mc_bkg_rejection:g}`, data-sideband rejection >= `{args.min_svpv_data_bkg_rejection:g}`, and MC signal retention >= `{args.min_svpv_signal_retention:g}`.",
        "- The MC-background proxy is non-truth-matched prompt-MC candidates in the scan sidebands.",
        "- The data-background proxy is the 2023 data sideband scan supplied to this script.",
        "",
        "## Derived Floors",
        "",
        "| bin | derived floors | AN floors | signal retention | MC bkg rejection | data bkg rejection | match |",
        "|---|---:|---:|---:|---:|---:|---|",
    ]
    for row in rows:
        lines.append(
            f"| {row['bin']} | `svpv>{row['derivedSvpvSigMin']}, chi2>{row['derivedChi2clMin']}` | "
            f"`svpv>{row['nominalSvpvSigMin']}, chi2>{row['nominalChi2clMin']}` | "
            f"{float(row['signalRetentionAtFloor']):.3f} | "
            f"{float(row['mcBackgroundRejectionAtFloor']):.3f} | "
            f"{float(row['dataBackgroundRejectionAtFloor']):.3f} | "
            f"{row['matchesNominalFloors']} |"
        )
    lines.extend(
        [
            "",
            "## Result",
            "",
            f"The independent floor rule matches the AN floor values in `{matched}/{len(rows)}` bins.",
            "This is a practical justification of the rectangular quality floors, not a proof that the AN floors are unique.",
            "",
            "Remaining AN dependence in this step: the AN floor values are used only as the final comparison target.",
            "",
            "Generated files:",
            "",
            "- `floor_derivation.tsv`",
            "- `floor_profiles.tsv`",
            "- `floor_profiles/chi2_floor_profile_grid.png` if plotting dependencies are available",
            "- `floor_profiles/floor_profile_grid.png` if plotting dependencies are available",
        ]
    )
    (args.outdir / "floor_justification_summary.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def maybe_plot(outdir: Path, profile_rows: list[dict[str, str]], summary_rows: list[dict[str, str]]) -> None:
    try:
        import matplotlib.pyplot as plt
        from PIL import Image, ImageOps
    except Exception as exc:  # pragma: no cover - optional dependency
        (outdir / "floor_plot_warning.txt").write_text(f"plotting unavailable: {exc}\n", encoding="utf-8")
        return

    plot_dir = outdir / "floor_profiles"
    plot_dir.mkdir(exist_ok=True)
    def render_grid(
        *,
        profile: str,
        x_key: str,
        selected_key: str,
        xlabel: str,
        suffix: str,
        grid_name: str,
    ) -> None:
        selected = {row["bin"]: float(row[selected_key]) for row in summary_rows}
        plot_paths: list[Path] = []

        for label in [row["bin"] for row in summary_rows]:
            rows = [row for row in profile_rows if row["bin"] == label and row["profile"] == profile]
            xs = [float(row[x_key]) for row in rows]
            signal = [float(row["signalRetention"]) for row in rows]
            mc_rej = [float(row["mcBackgroundRejection"]) for row in rows]
            data_rej = [float(row["dataBackgroundRejection"]) for row in rows]

            fig, ax = plt.subplots(figsize=(5.5, 3.6))
            ax.plot(xs, signal, marker="o", label="MC signal retention")
            ax.plot(xs, mc_rej, marker="s", label="MC bkg rejection")
            ax.plot(xs, data_rej, marker="^", label="data sideband rejection")
            ax.axvline(selected[label], color="black", linestyle="--", linewidth=1, label="derived floor")
            ax.set_ylim(-0.03, 1.03)
            ax.set_xlabel(xlabel)
            ax.set_ylabel("fraction")
            ax.set_title(BIN_LABEL.get(label, label))
            ax.grid(alpha=0.25)
            ax.legend(fontsize=7, loc="best")
            fig.tight_layout()
            path = plot_dir / f"{label}_{suffix}.png"
            fig.savefig(path, dpi=160)
            plot_paths.append(path)
            plt.close(fig)

        images = [Image.open(path).convert("RGB") for path in plot_paths]
        width = max(image.width for image in images)
        height = max(image.height for image in images)
        pad = 24
        canvas = Image.new("RGB", (3 * width + 4 * pad, 2 * height + 3 * pad), "white")
        for idx, image in enumerate(images):
            row, col = divmod(idx, 3)
            canvas.paste(ImageOps.contain(image, (width, height)), (pad + col * (width + pad), pad + row * (height + pad)))
        canvas.save(plot_dir / grid_name, quality=95)

    render_grid(
        profile="chi2_at_loose_svpv",
        x_key="chi2clMin",
        selected_key="derivedChi2clMin",
        xlabel=r"$D_{\chi^2{\rm cl}}$ floor",
        suffix="chi2_floor_profile",
        grid_name="chi2_floor_profile_grid.png",
    )
    render_grid(
        profile="svpv_at_selected_chi2",
        x_key="svpvSigMin",
        selected_key="derivedSvpvSigMin",
        xlabel=r"$D_{\rm svpvSig}$ floor",
        suffix="floor_profile",
        grid_name="floor_profile_grid.png",
    )


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "--data-sideband-scan",
        type=Path,
        default=Path("research/cms-d0-analysis/output/cut-derivation/topocuts_rect_an_sideband_full_20260617/d_topology_scan.tsv"),
    )
    p.add_argument(
        "--mc-background-scan",
        type=Path,
        default=Path("research/cms-d0-analysis/output/cut-derivation/topocuts_mc_background_full_20260617/d_topology_scan.tsv"),
    )
    p.add_argument(
        "--outdir",
        type=Path,
        default=Path("research/cms-d0-analysis/output/cut-derivation/topological_floor_justification_20260618"),
    )
    p.add_argument("--min-chi2-mc-bkg-rejection", type=float, default=0.04)
    p.add_argument("--max-chi2-signal-loss", type=float, default=0.06)
    p.add_argument("--min-svpv-mc-bkg-rejection", type=float, default=1.0 / 3.0)
    p.add_argument("--min-svpv-data-bkg-rejection", type=float, default=0.18)
    p.add_argument("--min-svpv-signal-retention", type=float, default=0.55)
    return p


def main() -> None:
    derive(parser().parse_args())


if __name__ == "__main__":
    main()
