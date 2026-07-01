#!/usr/bin/env python3
"""Build final-yield fit QA artifacts from the current diagnostic mass fits.

The output is deliberately a raw-yield QA product. It checks mass-fit stability
and produces AN-style tables/plots, but it does not apply luminosity,
efficiency, trigger, prompt-fraction, or EMD corrections.
"""

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
D0_MASS = 1.86484


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
        writer = csv.DictWriter(handle, fieldnames=fields, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        for row in rows:
            writer.writerow({field: row.get(field, "") for field in fields})


def bin_sort_key(label: str) -> tuple[int, int]:
    ay = 0 if "absy0to1" in label else 1
    if "pt2to5" in label:
        pt = 0
    elif "pt5to8" in label:
        pt = 1
    elif "pt8to12" in label:
        pt = 2
    else:
        pt = 9
    return ay, pt


def pretty(label: str) -> str:
    return (
        label.replace("absy0to1_", "|y| 0-1, ")
        .replace("absy1to2_", "|y| 1-2, ")
        .replace("pt2to5", "pT 2-5")
        .replace("pt5to8", "pT 5-8")
        .replace("pt8to12", "pT 8-12")
    )


def f(row: dict[str, str], key: str) -> float:
    try:
        return float(row.get(key, "nan"))
    except ValueError:
        return math.nan


def classify(row: dict[str, str]) -> tuple[str, str]:
    entries = f(row, "entries")
    mean = f(row, "mean")
    sigma = f(row, "sigma")
    chi2ndf = f(row, "chi2ndf")
    fit_status = row.get("fit_status", "")
    reasons = []
    if fit_status != "ok":
        reasons.append(f"fit_status={fit_status}")
    if not math.isfinite(entries) or entries < 100:
        reasons.append("entries_lt_100")
    elif entries < 200:
        reasons.append("entries_lt_200")
    if not math.isfinite(mean) or abs(mean - D0_MASS) > 0.020:
        reasons.append("mass_mean_shift_gt_20MeV")
    elif abs(mean - D0_MASS) > 0.010:
        reasons.append("mass_mean_shift_gt_10MeV")
    if not math.isfinite(sigma) or sigma < 0.003 or sigma > 0.030:
        reasons.append("sigma_outside_3to30MeV")
    if not math.isfinite(chi2ndf) or chi2ndf > 3.0:
        reasons.append("chi2ndf_gt_3")
    if any(reason in reasons for reason in ["entries_lt_100", "mass_mean_shift_gt_20MeV", "sigma_outside_3to30MeV", "chi2ndf_gt_3"]):
        return "fail", ",".join(reasons) if reasons else "none"
    if reasons:
        return "warn", ",".join(reasons)
    return "ok", "none"


def build_rows(mass_fit_summary: Path) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for row in read_tsv(mass_fit_summary):
        label = row.get("bin", "")
        if not label or label == "all":
            continue
        status, flags = classify(row)
        mean = f(row, "mean")
        sigma = f(row, "sigma")
        rows.append(
            {
                "bin": label,
                "pretty_bin": pretty(label),
                "entries": row.get("entries", ""),
                "raw_gaussian_yield": row.get("yield_gaus", ""),
                "mean": row.get("mean", ""),
                "mass_shift_mev": (mean - D0_MASS) * 1000.0 if math.isfinite(mean) else "",
                "sigma_mev": sigma * 1000.0 if math.isfinite(sigma) else "",
                "chi2ndf": row.get("chi2ndf", ""),
                "fit_status": row.get("fit_status", ""),
                "qa_status": status,
                "qa_flags": flags,
                "classification": "raw-yield-fit-QA; not efficiency/lumi corrected",
            }
        )
    rows.sort(key=lambda item: bin_sort_key(str(item["bin"])))
    return rows


def plot_rows(rows: list[dict[str, object]], outdir: Path) -> None:
    labels = [str(row["pretty_bin"]) for row in rows]
    x = list(range(len(rows)))
    yields = [float(row["raw_gaussian_yield"]) for row in rows]
    shifts = [float(row["mass_shift_mev"]) for row in rows]
    sigmas = [float(row["sigma_mev"]) for row in rows]
    chi2 = [float(row["chi2ndf"]) for row in rows]
    colors = ["#4C78A8" if row["qa_status"] == "ok" else "#F58518" if row["qa_status"] == "warn" else "#C44E52" for row in rows]

    fig, ax = plt.subplots(figsize=(10.5, 5.5))
    ax.bar(x, yields, color=colors)
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=30, ha="right")
    ax.set_ylabel("Raw Gaussian yield")
    ax.set_title("Raw D0 yield QA by bin")
    ax.grid(axis="y", alpha=0.25)
    fig.tight_layout()
    fig.savefig(outdir / "raw_yield_fit_qa_by_bin.png", dpi=180)
    fig.savefig(outdir / "raw_yield_fit_qa_by_bin.pdf")
    plt.close(fig)

    fig, ax1 = plt.subplots(figsize=(10.5, 5.5))
    ax1.axhline(0, color="#444444", lw=1)
    ax1.plot(x, shifts, "o-", color="#4C78A8", label="mass shift from PDG D0 [MeV]")
    ax1.set_ylabel("mass shift [MeV]", color="#4C78A8")
    ax1.tick_params(axis="y", labelcolor="#4C78A8")
    ax1.set_xticks(x)
    ax1.set_xticklabels(labels, rotation=30, ha="right")
    ax2 = ax1.twinx()
    ax2.plot(x, sigmas, "s--", color="#F58518", label="Gaussian sigma [MeV]")
    ax2.set_ylabel("sigma [MeV]", color="#F58518")
    ax2.tick_params(axis="y", labelcolor="#F58518")
    ax1.set_title("D0 mass-fit centroid and width QA")
    fig.tight_layout()
    fig.savefig(outdir / "mass_mean_sigma_qa.png", dpi=180)
    fig.savefig(outdir / "mass_mean_sigma_qa.pdf")
    plt.close(fig)

    fig, ax = plt.subplots(figsize=(10.5, 5.5))
    ax.bar(x, chi2, color=colors)
    ax.axhline(3.0, color="#C44E52", lw=1.5, ls="--", label="QA threshold")
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=30, ha="right")
    ax.set_ylabel("chi2 / ndf")
    ax.set_title("Mass-fit goodness QA")
    ax.grid(axis="y", alpha=0.25)
    ax.legend()
    fig.tight_layout()
    fig.savefig(outdir / "mass_fit_chi2_qa.png", dpi=180)
    fig.savefig(outdir / "mass_fit_chi2_qa.pdf")
    plt.close(fig)


def write_readme(outdir: Path, mass_fit_summary: Path, rows: list[dict[str, object]]) -> None:
    counts = {"ok": 0, "warn": 0, "fail": 0}
    for row in rows:
        counts[str(row["qa_status"])] = counts.get(str(row["qa_status"]), 0) + 1
    text = f"""# Final-Yield Fit QA Scaffold

Generated UTC: `{datetime.now(timezone.utc).isoformat()}`

Input mass-fit summary: `{mass_fit_summary}`

This output checks the current diagnostic raw-yield fits for stability. It is
not a final signal-extraction result and does not include corrections.

QA counts:

- ok: `{counts.get("ok", 0)}`
- warn: `{counts.get("warn", 0)}`
- fail: `{counts.get("fail", 0)}`

Files:

- `yield_fit_quality.tsv`
- `raw_yield_fit_qa_by_bin.png`
- `mass_mean_sigma_qa.png`
- `mass_fit_chi2_qa.png`
"""
    (outdir / "README.md").write_text(text, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--label", default="final_yield_fit_summary_20260622")
    parser.add_argument("--mass-fit-summary", type=Path, default=None)
    args = parser.parse_args()

    proxy_dir = latest_dir(OUTPUT / "an-proxy-plots")
    mass_fit_summary = args.mass_fit_summary or (proxy_dir / "mass_fit_summary.tsv" if proxy_dir else Path())
    outdir = OUTPUT / "final-yield-fit-summary" / args.label
    outdir.mkdir(parents=True, exist_ok=True)

    rows = build_rows(mass_fit_summary)
    write_tsv(
        outdir / "yield_fit_quality.tsv",
        rows,
        [
            "bin",
            "pretty_bin",
            "entries",
            "raw_gaussian_yield",
            "mean",
            "mass_shift_mev",
            "sigma_mev",
            "chi2ndf",
            "fit_status",
            "qa_status",
            "qa_flags",
            "classification",
        ],
    )
    if rows:
        plot_rows(rows, outdir)
    write_readme(outdir, mass_fit_summary, rows)

    print(f"out_dir={outdir}")
    print(f"rows={len(rows)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
