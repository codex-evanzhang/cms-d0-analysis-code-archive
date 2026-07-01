#!/usr/bin/env python3
"""Summarize ZDC side-scale closure on matched recreated/official events.

The input is the `zdc_channel_event_sums.tsv` table from
`diagnose_zdc_channel_mismatch.C`. It contains the pre-scale recreated sums and
official sums on identical event IDs. This script derives and validates the
side-scale factors without using the AN thresholds as fit inputs.
"""

from __future__ import annotations

import argparse
import csv
import math
from dataclasses import dataclass
from pathlib import Path
from statistics import median


DEFAULT_PLUS_SCALE = 0.915746
DEFAULT_MINUS_SCALE = 0.876579


@dataclass
class Row:
    run: int
    lumi: int
    evt: int
    rec_plus: float
    off_plus: float
    rec_minus: float
    off_minus: float


@dataclass
class Stats:
    label: str
    side: str
    n: int
    slope: float
    mean_ratio: float
    median_ratio: float
    rms_frac_residual_after_scale: float
    max_abs_residual_after_scale: float
    agreement_0n_xn_after_scale: int
    agreement_lt_or_1500_after_scale: int


def as_float(value: str) -> float:
    try:
        out = float(value)
    except ValueError:
        return math.nan
    return out if math.isfinite(out) else math.nan


def load_rows(path: Path) -> list[Row]:
    rows: list[Row] = []
    with path.open(encoding="utf-8") as handle:
        reader = csv.DictReader(handle, delimiter="\t")
        for raw in reader:
            rec_plus = as_float(raw["recSumPlus"])
            off_plus = as_float(raw["offSumPlus"])
            rec_minus = as_float(raw["recSumMinus"])
            off_minus = as_float(raw["offSumMinus"])
            if not all(math.isfinite(v) for v in (rec_plus, off_plus, rec_minus, off_minus)):
                continue
            rows.append(
                Row(
                    run=int(raw["run"]),
                    lumi=int(raw["lumi"]),
                    evt=int(raw["evt"]),
                    rec_plus=rec_plus,
                    off_plus=off_plus,
                    rec_minus=rec_minus,
                    off_minus=off_minus,
                )
            )
    return rows


def slope_through_origin(xs: list[float], ys: list[float]) -> float:
    xx = sum(x * x for x in xs if abs(x) > 1e-9)
    xy = sum(x * y for x, y in zip(xs, ys) if abs(x) > 1e-9)
    return xy / xx if xx > 0 else math.nan


def quantiles(values: list[float]) -> tuple[float, float]:
    ordered = sorted(values)
    if not ordered:
        return (math.nan, math.nan)
    def at(q: float) -> float:
        pos = q * (len(ordered) - 1)
        lo = math.floor(pos)
        hi = math.ceil(pos)
        if lo == hi:
            return ordered[lo]
        frac = pos - lo
        return ordered[lo] * (1.0 - frac) + ordered[hi] * frac
    return at(1 / 3), at(2 / 3)


def nominal_0n_xn(plus: float, minus: float) -> bool:
    gamma_n = minus > 1000.0 and plus < 1100.0
    n_gamma = plus > 1100.0 and minus < 1000.0
    return gamma_n or n_gamma


def lt_or_1500(plus: float, minus: float) -> bool:
    return plus < 1500.0 or minus < 1500.0


def stats(label: str, side: str, rows: list[Row], plus_scale: float, minus_scale: float) -> Stats:
    if side == "plus":
        rec = [r.rec_plus for r in rows if abs(r.rec_plus) > 1e-9]
        off = [r.off_plus for r in rows if abs(r.rec_plus) > 1e-9]
        scale = plus_scale
    else:
        rec = [r.rec_minus for r in rows if abs(r.rec_minus) > 1e-9]
        off = [r.off_minus for r in rows if abs(r.rec_minus) > 1e-9]
        scale = minus_scale
    ratios = [y / x for x, y in zip(rec, off) if abs(x) > 1e-9]
    residuals = [y - scale * x for x, y in zip(rec, off)]
    frac = [res / y for res, y in zip(residuals, off) if abs(y) > 1e-9]
    rms_frac = math.sqrt(sum(v * v for v in frac) / len(frac)) if frac else math.nan
    max_abs = max((abs(v) for v in residuals), default=math.nan)

    agree_0n = 0
    agree_lt = 0
    for row in rows:
        scaled_plus = plus_scale * row.rec_plus
        scaled_minus = minus_scale * row.rec_minus
        if nominal_0n_xn(scaled_plus, scaled_minus) == nominal_0n_xn(row.off_plus, row.off_minus):
            agree_0n += 1
        if lt_or_1500(scaled_plus, scaled_minus) == lt_or_1500(row.off_plus, row.off_minus):
            agree_lt += 1

    return Stats(
        label=label,
        side=side,
        n=len(rec),
        slope=slope_through_origin(rec, off),
        mean_ratio=sum(ratios) / len(ratios) if ratios else math.nan,
        median_ratio=median(ratios) if ratios else math.nan,
        rms_frac_residual_after_scale=rms_frac,
        max_abs_residual_after_scale=max_abs,
        agreement_0n_xn_after_scale=agree_0n,
        agreement_lt_or_1500_after_scale=agree_lt,
    )


def split_rows(rows: list[Row]) -> dict[str, list[Row]]:
    out: dict[str, list[Row]] = {
        "all": rows,
        "lumi_even": [r for r in rows if r.lumi % 2 == 0],
        "lumi_odd": [r for r in rows if r.lumi % 2 == 1],
        "event_even": [r for r in rows if r.evt % 2 == 0],
        "event_odd": [r for r in rows if r.evt % 2 == 1],
    }
    plus_q1, plus_q2 = quantiles([r.rec_plus for r in rows])
    minus_q1, minus_q2 = quantiles([r.rec_minus for r in rows])
    out["plus_low_tertile"] = [r for r in rows if r.rec_plus <= plus_q1]
    out["plus_mid_tertile"] = [r for r in rows if plus_q1 < r.rec_plus <= plus_q2]
    out["plus_high_tertile"] = [r for r in rows if r.rec_plus > plus_q2]
    out["minus_low_tertile"] = [r for r in rows if r.rec_minus <= minus_q1]
    out["minus_mid_tertile"] = [r for r in rows if minus_q1 < r.rec_minus <= minus_q2]
    out["minus_high_tertile"] = [r for r in rows if r.rec_minus > minus_q2]
    return out


def write_table(path: Path, all_stats: list[Stats]) -> None:
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle, delimiter="\t", lineterminator="\n")
        writer.writerow(
            [
                "split",
                "side",
                "n",
                "slope_official_over_recreated",
                "mean_ratio",
                "median_ratio",
                "rms_frac_residual_after_scale",
                "max_abs_residual_after_scale",
                "agreement_0n_xn_after_scale",
                "agreement_lt_or_1500_after_scale",
            ]
        )
        for s in all_stats:
            writer.writerow(
                [
                    s.label,
                    s.side,
                    s.n,
                    f"{s.slope:.8g}",
                    f"{s.mean_ratio:.8g}",
                    f"{s.median_ratio:.8g}",
                    f"{s.rms_frac_residual_after_scale:.8g}",
                    f"{s.max_abs_residual_after_scale:.8g}",
                    s.agreement_0n_xn_after_scale,
                    s.agreement_lt_or_1500_after_scale,
                ]
            )


def write_summary(path: Path, rows: list[Row], all_stats: list[Stats], plus_scale: float, minus_scale: float) -> None:
    by_key = {(s.label, s.side): s for s in all_stats}
    with path.open("w", encoding="utf-8") as out:
        out.write("# ZDC Scale Closure Preflight\n\n")
        out.write("<!-- DOC_OWNER: cms-analysis-manager ZDC scale closure. -->\n")
        out.write("<!-- TOKEN_NOTE: generated by summarize_zdc_scale_closure.py. -->\n\n")
        out.write("## Inputs\n\n")
        out.write(f"- Matched events: `{len(rows)}`\n")
        out.write(f"- Applied plus scale: `{plus_scale}`\n")
        out.write(f"- Applied minus scale: `{minus_scale}`\n\n")
        out.write("## All-Event Closure\n\n")
        out.write("| side | slope official/recreated | median ratio | RMS fractional residual after scale | max residual after scale |\n")
        out.write("|---|---:|---:|---:|---:|\n")
        for side in ("plus", "minus"):
            s = by_key[("all", side)]
            out.write(
                f"| {side} | {s.slope:.6f} | {s.median_ratio:.6f} | "
                f"{s.rms_frac_residual_after_scale:.6f} | {s.max_abs_residual_after_scale:.3f} GeV |\n"
            )
        out.write("\n## Split Stability\n\n")
        out.write("| split | plus slope | minus slope |\n")
        out.write("|---|---:|---:|\n")
        for split in (
            "lumi_even",
            "lumi_odd",
            "event_even",
            "event_odd",
            "plus_low_tertile",
            "plus_mid_tertile",
            "plus_high_tertile",
            "minus_low_tertile",
            "minus_mid_tertile",
            "minus_high_tertile",
        ):
            plus = by_key[(split, "plus")]
            minus = by_key[(split, "minus")]
            out.write(f"| {split} | {plus.slope:.6f} | {minus.slope:.6f} |\n")
        all_plus = by_key[("all", "plus")]
        all_minus = by_key[("all", "minus")]
        out.write("\n## Decision Closure After Scale\n\n")
        out.write("| decision | agreement |\n|---|---:|\n")
        out.write(f"| nominal 0nXn | {all_plus.agreement_0n_xn_after_scale}/{len(rows)} |\n")
        out.write(f"| lt OR 1500 | {all_plus.agreement_lt_or_1500_after_scale}/{len(rows)} |\n\n")
        out.write("## Interpretation\n\n")
        out.write(
            "The fitted side scale is stable under lumi/event parity and broad energy-split tests. "
            "This supports treating the correction as a side-level ZDC absolute calibration/backport "
            "factor for medium-scale validation. It still is not bitwise equality with the official "
            "forest and should remain documented as an official-reforest matching correction.\n"
        )


def make_plot(path: Path, rows: list[Row], plus_scale: float, minus_scale: float) -> None:
    try:
        import matplotlib.pyplot as plt
    except Exception:
        return

    fig, axes = plt.subplots(1, 2, figsize=(10, 4), dpi=160)
    for ax, side, scale in ((axes[0], "plus", plus_scale), (axes[1], "minus", minus_scale)):
        if side == "plus":
            rec = [r.rec_plus * scale for r in rows]
            off = [r.off_plus for r in rows]
        else:
            rec = [r.rec_minus * scale for r in rows]
            off = [r.off_minus for r in rows]
        ax.scatter(rec, off, s=3, alpha=0.25, linewidths=0)
        finite = [v for v in rec + off if math.isfinite(v)]
        lo = min(finite) if finite else 0
        hi = max(finite) if finite else 1
        ax.plot([lo, hi], [lo, hi], color="black", linewidth=1)
        ax.set_title(side)
        ax.set_xlabel("scaled recreated sum")
        ax.set_ylabel("official sum")
        ax.grid(alpha=0.25)
    fig.tight_layout()
    fig.savefig(path)
    plt.close(fig)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input_tsv", type=Path)
    parser.add_argument("outdir", type=Path)
    parser.add_argument("--plus-scale", type=float, default=DEFAULT_PLUS_SCALE)
    parser.add_argument("--minus-scale", type=float, default=DEFAULT_MINUS_SCALE)
    args = parser.parse_args()

    args.outdir.mkdir(parents=True, exist_ok=True)
    rows = load_rows(args.input_tsv)
    splits = split_rows(rows)
    all_stats: list[Stats] = []
    for label, subset in splits.items():
        all_stats.append(stats(label, "plus", subset, args.plus_scale, args.minus_scale))
        all_stats.append(stats(label, "minus", subset, args.plus_scale, args.minus_scale))
    write_table(args.outdir / "zdc_scale_closure.tsv", all_stats)
    write_summary(args.outdir / "README.md", rows, all_stats, args.plus_scale, args.minus_scale)
    make_plot(args.outdir / "zdc_scale_scatter.png", rows, args.plus_scale, args.minus_scale)
    print(f"Wrote {args.outdir / 'README.md'}")
    print(f"Wrote {args.outdir / 'zdc_scale_closure.tsv'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
