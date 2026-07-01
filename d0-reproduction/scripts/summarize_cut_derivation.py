#!/usr/bin/env python3
"""Summarize D0 cut-derivation scans into near-nominal working points."""

from __future__ import annotations

import argparse
import csv
import math
from pathlib import Path


NOMINAL_D = {
    "absy0to1_pt2to5": (0.40, 2.5, 0.10, 0.50),
    "absy0to1_pt5to8": (0.35, 3.5, 0.10, 0.30),
    "absy0to1_pt8to12": (0.40, 3.5, 0.10, 0.30),
    "absy1to2_pt2to5": (0.20, 2.5, 0.10, 0.30),
    "absy1to2_pt5to8": (0.25, 3.5, 0.10, 0.30),
    "absy1to2_pt8to12": (0.25, 3.5, 0.10, 0.30),
}

NOMINAL_ZDC = (1100.0, 1000.0)
NOMINAL_HF = (9.2, 8.6)


def read_tsv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="") as handle:
        return list(csv.DictReader(handle, delimiter="\t"))


def f(row: dict[str, str], key: str) -> float:
    return float(row[key])


def close(a: float, b: float) -> bool:
    return abs(a - b) < 1e-9


def score_distance(row: dict[str, str], nominal: tuple[float, float, float, float]) -> float:
    # Normalize each coordinate by a practical scan scale so "nearest" is not
    # dominated by the SVPV significance units.
    alpha, svpv, chi2, dtheta = nominal
    return math.sqrt(
        ((f(row, "alphaMax") - alpha) / 0.35) ** 2
        + ((f(row, "svpvSigMin") - svpv) / 3.0) ** 2
        + ((f(row, "chi2clMin") - chi2) / 0.18) ** 2
        + ((f(row, "dthetaMax") - dtheta) / 0.30) ** 2
    )


def exact_d_row(rows: list[dict[str, str]], nominal: tuple[float, float, float, float]) -> dict[str, str] | None:
    alpha, svpv, chi2, dtheta = nominal
    for row in rows:
        if (
            close(f(row, "alphaMax"), alpha)
            and close(f(row, "svpvSigMin"), svpv)
            and close(f(row, "chi2clMin"), chi2)
            and close(f(row, "dthetaMax"), dtheta)
        ):
            return row
    return None


def choose_topology_working_point(
    rows: list[dict[str, str]],
    nominal: tuple[float, float, float, float],
    nominal_row: dict[str, str] | None,
) -> dict[str, str]:
    """Choose a scan point that improves signal without loosening sidebands.

    The raw score is useful for debugging, but for a reproduction task we should
    not loosen a nominal sideband rejection unless the statistics are strong.
    """
    if not nominal_row:
        return choose_near_score(rows, nominal, 0.90)
    nominal_bkg = f(nominal_row, "backgroundFraction")
    allowed_bkg = nominal_bkg if nominal_bkg <= 0 else min(1.0, 1.05 * nominal_bkg)
    candidates = [row for row in rows if f(row, "backgroundFraction") <= allowed_bkg]
    if not candidates:
        return nominal_row
    best_eff = max(f(row, "signalEfficiency") for row in candidates)
    plateau = [row for row in candidates if f(row, "signalEfficiency") >= 0.98 * best_eff]
    return min(plateau, key=lambda row: score_distance(row, nominal))


def choose_near_score(rows: list[dict[str, str]], nominal: tuple[float, float, float, float], frac: float) -> dict[str, str]:
    best_score = max(f(row, "score") for row in rows)
    plateau = [row for row in rows if f(row, "score") >= frac * best_score]
    return min(plateau, key=lambda row: score_distance(row, nominal))


def choose_event_near_nominal(
    rows: list[dict[str, str]],
    first_key: str,
    second_key: str,
    nominal: tuple[float, float],
) -> dict[str, str]:
    nominal_row = exact_event_row(rows, first_key, second_key, nominal)
    best_score = max(f(row, "score") for row in rows)
    if nominal_row and f(nominal_row, "score") >= 0.90 * best_score:
        return nominal_row
    max_retention = max(f(row, "dataRetention") for row in rows)
    nominal_fake = f(nominal_row, "controlFakeFraction") if nominal_row else min(f(row, "controlFakeFraction") for row in rows)
    plateau = [
        row
        for row in rows
        if f(row, "dataRetention") >= 0.995 * max_retention
        and f(row, "controlFakeFraction") <= max(2.0 * nominal_fake, nominal_fake + 2e-5)
    ]
    if not plateau:
        plateau = rows
    a0, b0 = nominal
    return min(
        plateau,
        key=lambda row: math.sqrt(((f(row, first_key) - a0) / max(a0, 1.0)) ** 2 + ((f(row, second_key) - b0) / max(b0, 1.0)) ** 2),
    )


def exact_event_row(
    rows: list[dict[str, str]],
    first_key: str,
    second_key: str,
    nominal: tuple[float, float],
) -> dict[str, str] | None:
    a, b = nominal
    for row in rows:
        if close(f(row, first_key), a) and close(f(row, second_key), b):
            return row
    return None


def fmt(row: dict[str, str] | None, key: str) -> str:
    if not row:
        return "NA"
    return row[key]


def summarize(indir: Path) -> None:
    d_rows = read_tsv(indir / "d_topology_scan.tsv")
    zdc_rows = read_tsv(indir / "zdc_threshold_scan.tsv")
    hf_rows = read_tsv(indir / "hf_gap_threshold_scan.tsv")

    rows_by_bin: dict[str, list[dict[str, str]]] = {}
    for row in d_rows:
        rows_by_bin.setdefault(row["bin"], []).append(row)

    rec_path = indir / "near_nominal_cut_recommendations.tsv"
    with rec_path.open("w", newline="") as handle:
        writer = csv.writer(handle, delimiter="\t")
        writer.writerow(
            [
                "family",
                "bin_or_cut",
                "recommended",
                "raw_best",
                "nominal",
                "signal_eff_or_retention",
                "background_or_fake",
                "raw_best_score",
                "nominal_score",
                "stats_note",
            ]
        )

        for label in NOMINAL_D:
            rows = rows_by_bin[label]
            raw = max(rows, key=lambda row: f(row, "score"))
            nominal = exact_d_row(rows, NOMINAL_D[label])
            near = choose_topology_working_point(rows, NOMINAL_D[label], nominal)
            bkg_base = int(float(raw["backgroundBase"]))
            sig_base = int(float(raw["signalBase"]))
            stats_note = "ok_for_bounded_scan" if bkg_base >= 20 and sig_base >= 1000 else "low_sideband_stats_keep_nominal_or_scan_more"
            writer.writerow(
                [
                    "d_topology",
                    label,
                    f"alpha<{near['alphaMax']}, svpv>{near['svpvSigMin']}, chi2>{near['chi2clMin']}, dtheta<{near['dthetaMax']}",
                    f"alpha<{raw['alphaMax']}, svpv>{raw['svpvSigMin']}, chi2>{raw['chi2clMin']}, dtheta<{raw['dthetaMax']}",
                    f"alpha<{NOMINAL_D[label][0]}, svpv>{NOMINAL_D[label][1]}, chi2>{NOMINAL_D[label][2]}, dtheta<{NOMINAL_D[label][3]}",
                    near["signalEfficiency"],
                    near["backgroundFraction"],
                    raw["score"],
                    fmt(nominal, "score"),
                    stats_note,
                ]
            )

        for family, rows, first_key, second_key, nominal in [
            ("zdc_xor", zdc_rows, "plusThreshold", "minusThreshold", NOMINAL_ZDC),
            ("hf_gap", hf_rows, "hfPlusThreshold", "hfMinusThreshold", NOMINAL_HF),
        ]:
            raw = max(rows, key=lambda row: f(row, "score"))
            nominal_row = exact_event_row(rows, first_key, second_key, nominal)
            near = choose_event_near_nominal(rows, first_key, second_key, nominal)
            writer.writerow(
                [
                    "event",
                    family,
                    f"{first_key}={near[first_key]}, {second_key}={near[second_key]}",
                    f"{first_key}={raw[first_key]}, {second_key}={raw[second_key]}",
                    f"{first_key}={nominal[0]}, {second_key}={nominal[1]}",
                    near["dataRetention"],
                    near["controlFakeFraction"],
                    raw["score"],
                    fmt(nominal_row, "score"),
                    "event scan is plateau-like; use distributions/physics labels before changing nominal",
                ]
            )

    md_path = indir / "derived_cut_summary.md"
    with md_path.open("w") as out:
        out.write("# Derived Cut Summary\n\n")
        out.write("This summary postprocesses the raw scan into working points that are both data-supported and close to the nominal analysis values when the scan is flat.\n\n")
        out.write("## Event Thresholds\n\n")
        out.write("- Raw ZDC/HF score optima are edge/plateau choices because EmptyBX fake rates are tiny across the grid.\n")
        out.write("- Therefore the near-nominal recommendation is preferred unless a dedicated ZDC 1n peak or HF-gap plateau study shows otherwise.\n")
        out.write("- Official BeamA/BeamB MC supports the HF side assignment and shows nominal HF cuts keep about 99% of checked MC gap-side events.\n")
        out.write("- Official BeamA/BeamB MC cannot set ZDC thresholds because checked ZDC sums are zero.\n\n")
        out.write("## D-Candidate Topology\n\n")
        out.write("- Low-pT bins have useful MC statistics; high-pT and high-|y| sidebands remain sparse in a 1M-entry scan.\n")
        out.write("- Use the table `near_nominal_cut_recommendations.tsv` to compare raw optima, nominal values, and near-nominal high-score points.\n")
        out.write("- Treat bins marked `low_sideband_stats_keep_nominal_or_scan_more` as not derivable yet.\n\n")
        out.write("## Files\n\n")
        out.write(f"- Recommendations: `{rec_path.name}`\n")
        out.write("- Raw D scan: `d_topology_scan.tsv`\n")
        out.write("- Raw ZDC scan: `zdc_threshold_scan.tsv`\n")
        out.write("- Raw HF scan: `hf_gap_threshold_scan.tsv`\n")

    print(f"wrote {rec_path}")
    print(f"wrote {md_path}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("indir", type=Path)
    args = parser.parse_args()
    summarize(args.indir)


if __name__ == "__main__":
    main()
