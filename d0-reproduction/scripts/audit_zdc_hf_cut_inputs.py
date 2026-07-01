#!/usr/bin/env python3
"""Audit the MC/input basis for the D0 ZDC and HF cut scans.

This is intentionally lightweight: it reads the saved TSV scan artifacts and
local MC notes, then writes a compact report. The ROOT-level branch histogram
comparison still has to run on LXPLUS because the official EOS inputs are not
mounted locally.
"""

from __future__ import annotations

import csv
import datetime as dt
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
CMS = ROOT / "research" / "cms-d0-analysis"
CUT_DIR = CMS / "output" / "cut-derivation" / "prompt_mc_data_sideband_emptybx_200k"
OUT_DIR = CMS / "output" / "zdc_hf_cut_audit"
MCM_DIR = CMS / "output" / "mcm"

NOMINAL = {
    "zdc_xor": {"plus": 1100.0, "minus": 1000.0},
    "hf_gap": {"plus": 9.2, "minus": 8.6},
}


def utc_now() -> str:
    return dt.datetime.now(dt.UTC).strftime("%Y-%m-%dT%H:%M:%SZ")


def read_tsv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle, delimiter="\t"))


def fnum(value: str) -> float:
    return float(value)


def fmt(value: float) -> str:
    if abs(value) >= 100:
        return f"{value:.0f}"
    if abs(value) >= 1:
        return f"{value:.4g}"
    return f"{value:.6g}"


def row_for_threshold(rows: list[dict[str, str]], plus_key: str, minus_key: str, plus: float, minus: float) -> dict[str, str]:
    for row in rows:
        if abs(fnum(row[plus_key]) - plus) < 1e-9 and abs(fnum(row[minus_key]) - minus) < 1e-9:
            return row
    raise KeyError(f"Could not find threshold {plus}, {minus}")


def best_row(rows: list[dict[str, str]]) -> dict[str, str]:
    return max(rows, key=lambda row: fnum(row["score"]))


def edge_flag(rows: list[dict[str, str]], plus_key: str, minus_key: str, row: dict[str, str]) -> bool:
    plus_values = sorted({fnum(item[plus_key]) for item in rows})
    minus_values = sorted({fnum(item[minus_key]) for item in rows})
    plus = fnum(row[plus_key])
    minus = fnum(row[minus_key])
    return plus in {plus_values[0], plus_values[-1]} or minus in {minus_values[0], minus_values[-1]}


def mcm_summary(request: str) -> dict[str, str]:
    data = json.loads((MCM_DIR / f"{request}.json").read_text(encoding="utf-8"))["results"]
    params = data.get("generator_parameters", [])
    effs = []
    for item in params:
        if "filter_efficiency" in item:
            effs.append(f"{item['filter_efficiency']} +- {item.get('filter_efficiency_error', 0)}")
    return {
        "request": request,
        "dataset": data.get("dataset_name", ""),
        "cmssw": data.get("cmssw_release", ""),
        "events": str(data.get("total_events", "")),
        "filter_efficiency": "; ".join(effs),
    }


def write_threshold_audit() -> list[dict[str, str]]:
    zdc_rows = read_tsv(CUT_DIR / "zdc_threshold_scan.tsv")
    hf_rows = read_tsv(CUT_DIR / "hf_gap_threshold_scan.tsv")

    audits: list[dict[str, str]] = []
    for family, rows, plus_key, minus_key in [
        ("zdc_xor", zdc_rows, "plusThreshold", "minusThreshold"),
        ("hf_gap", hf_rows, "hfPlusThreshold", "hfMinusThreshold"),
    ]:
        nominal = row_for_threshold(rows, plus_key, minus_key, NOMINAL[family]["plus"], NOMINAL[family]["minus"])
        best = best_row(rows)
        audits.append(
            {
                "family": family,
                "nominal_plus": nominal[plus_key],
                "nominal_minus": nominal[minus_key],
                "nominal_data_retention": nominal["dataRetention"],
                "nominal_control_fake": nominal["controlFakeFraction"],
                "nominal_score": nominal["score"],
                "best_plus": best[plus_key],
                "best_minus": best[minus_key],
                "best_data_retention": best["dataRetention"],
                "best_control_fake": best["controlFakeFraction"],
                "best_score": best["score"],
                "best_at_scan_edge": str(edge_flag(rows, plus_key, minus_key, best)),
            }
        )

    out_path = OUT_DIR / "event_threshold_audit.tsv"
    with out_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(audits[0].keys()), delimiter="\t")
        writer.writeheader()
        writer.writerows(audits)
    return audits


def write_mc_audit() -> list[dict[str, str]]:
    prompt = mcm_summary("HIN-HINPbPbWinter24UPCGS-00001")
    nonprompt = mcm_summary("HIN-HINPbPbWinter24UPCGS-00003")
    rows = [
        {
            "sample": "older_pp_pythia_smoke",
            "distribution_status": "does_not_match_official_model",
            "reason": "pp HardQCD charm mechanics test; no PbPb photon flux, nuclear PDF, EvtGen card, prompt filter, detector simulation, ZDC, or HF.",
            "usable_for_zdc_hf_cuts": "no",
        },
        {
            "sample": "official_prompt_gen_smoke",
            "distribution_status": "generator_fragment_matches_mcm_but_statistics_are_tiny",
            "reason": "McM prompt fragment loaded in CMSSW_14_1_7; 6 kept events from 500 generated records versus official filter efficiency about 0.007 +- 0.0007.",
            "usable_for_zdc_hf_cuts": "no, GEN-only output has no detector-level ZDC/HF response.",
        },
        {
            "sample": "provided_prompt_mc_skim",
            "distribution_status": "reference_reco_skim_available",
            "reason": f"{prompt['dataset']}; {prompt['events']} requested events; reduced Tree has D truth plus ZDC/HF branches.",
            "usable_for_zdc_hf_cuts": "only after validating ZDC/HF values against official forest/distribution plots.",
        },
        {
            "sample": "provided_nonprompt_mc",
            "distribution_status": "reference_model_recorded",
            "reason": f"{nonprompt['dataset']}; filter efficiencies {nonprompt['filter_efficiency']}.",
            "usable_for_zdc_hf_cuts": "not the primary prompt D0 signal sample.",
        },
    ]
    out_path = OUT_DIR / "mc_input_audit.tsv"
    with out_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()), delimiter="\t")
        writer.writeheader()
        writer.writerows(rows)
    return rows


def write_report(thresholds: list[dict[str, str]], mc_rows: list[dict[str, str]]) -> None:
    lines = [
        "# ZDC/HF Cut and MC Input Audit",
        "",
        "<!-- DOC_OWNER: cms-analysis-manager ZDC/HF cut audit. -->",
        "<!-- TOKEN_NOTE: read this before rerunning cut scans or discussing event-threshold cuts. -->",
        "",
        f"Generated UTC: `{utc_now()}`",
        "",
        "## Main Finding",
        "",
        "The old retention-versus-EmptyBX scan is not the nominal ZDC method. "
        "The nominal ZDC cut follows the AN calibrated offline recipe: "
        "`ZDCsignal = HADsum + 0.1 * EMsum`, with plus `1100 GeV` and minus "
        "`1000 GeV` one-neutron thresholds, applied as an XOR 0nXn/Xn0n condition. "
        "EmptyBX and ZDC peak fits are validation/efficiency checks. The old score "
        "prefers the loosest scanned ZDC thresholds at the grid edge and must remain "
        "an audit failure mode, not a physics recommendation.",
        "",
        "## MC Input Check",
        "",
        "| Sample | Status | Usable for ZDC/HF cuts? |",
        "|---|---|---|",
    ]
    for row in mc_rows:
        lines.append(f"| `{row['sample']}` | {row['distribution_status']} | {row['usable_for_zdc_hf_cuts']} |")

    lines.extend(
        [
            "",
            "## Threshold Scan Check",
            "",
            "| Family | Nominal | Saved best | Nominal retention/fake | Best retention/fake | Edge optimum? |",
            "|---|---:|---:|---:|---:|---|",
        ]
    )
    for row in thresholds:
        lines.append(
            "| `{family}` | {np}/{nm} | {bp}/{bm} | {nr}/{nf} | {br}/{bf} | {edge} |".format(
                family=row["family"],
                np=fmt(fnum(row["nominal_plus"])),
                nm=fmt(fnum(row["nominal_minus"])),
                bp=fmt(fnum(row["best_plus"])),
                bm=fmt(fnum(row["best_minus"])),
                nr=fmt(fnum(row["nominal_data_retention"])),
                nf=fmt(fnum(row["nominal_control_fake"])),
                br=fmt(fnum(row["best_data_retention"])),
                bf=fmt(fnum(row["best_control_fake"])),
                edge=row["best_at_scan_edge"],
            )
        )

    lines.extend(
        [
            "",
            "## Why The Cuts Differed",
            "",
            "- The older generated Pythia sample was a mechanics smoke test, not the official UPC PbPb MC.",
            "- The later McM-matched sample was only GEN-level and too small; it cannot contain ZDC neutron peaks or HF tower energy.",
            "- The event-cut scan optimized `dataRetention / sqrt(EmptyBXFake + 1/N)` instead of applying the AN calibrated ZDC signal and treating EmptyBX/peak fits as validation.",
            "- The scan therefore rewarded looser ZDC thresholds; both saved best points are at the largest scanned threshold values.",
            "- Prior same-event validation showed local ZDC reconstruction needs official-branch closure before fake-rate claims; it does not change the AN nominal plus/minus thresholds.",
            "",
            "## Correct Next Validation",
            "",
            "1. Confirm that the branch used by downstream cuts is the AN offline ZDC signal or a closed proxy for it.",
            "2. Use EmptyBX events to estimate the 0n-side fake rate above `1100/1000 GeV`.",
            "3. Fit 1n/2n/3n ZDC spectra and integrate above threshold for the Xn-side efficiency.",
            "4. Apply the EM-pileup survival/correction after the raw XOR selection.",
            "5. Validate the HF threshold from rapidity-gap/noise plateau behavior after fixing the ZDC topology side.",
            "6. Only then rerun the D-topology cut optimization with the event selection fixed.",
            "",
            "## Artifacts",
            "",
            "- `event_threshold_audit.tsv`",
            "- `mc_input_audit.tsv`",
        ]
    )
    (OUT_DIR / "README.md").write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    thresholds = write_threshold_audit()
    mc_rows = write_mc_audit()
    write_report(thresholds, mc_rows)
    print(f"Wrote {OUT_DIR.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
