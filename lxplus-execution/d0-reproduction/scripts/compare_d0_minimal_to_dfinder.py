#!/usr/bin/env python3
"""Compare EvanAnalysis/D0MinimalForest output with the existing Dfinder skim."""

from __future__ import annotations

import argparse
import csv
import math
from pathlib import Path

import ROOT


ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptStat(0)


def owned_output(path: str) -> bool:
    return path.startswith("/afs/cern.ch/user/e/evzhang") or path.startswith(
        "/afs/cern.ch/work/e/evzhang"
    ) or path.startswith("/eos/user/e/evzhang")


def open_tree(path: str, tree_name: str) -> tuple[ROOT.TFile, ROOT.TTree]:
    root_file = ROOT.TFile.Open(path)
    if not root_file or root_file.IsZombie():
        raise RuntimeError(f"failed to open {path}")
    tree = root_file.Get(tree_name)
    if not tree:
        raise RuntimeError(f"missing tree {tree_name} in {path}")
    return root_file, tree


def make_hist(name: str, title: str, color: int, bins: int = 160, lo: float = 1.5, hi: float = 2.3):
    hist = ROOT.TH1D(name, title, bins, lo, hi)
    hist.Sumw2()
    hist.SetDirectory(0)
    hist.SetLineColor(color)
    hist.SetMarkerColor(color)
    hist.SetLineWidth(2)
    hist.SetMarkerStyle(20)
    hist.SetMarkerSize(0.55)
    hist.GetXaxis().SetTitle("m(K#pi) [GeV]")
    hist.GetYaxis().SetTitle("Candidates")
    return hist


def clone_normalized(hist, name: str, color: int):
    clone = hist.Clone(name)
    clone.SetDirectory(0)
    clone.SetLineColor(color)
    clone.SetMarkerColor(color)
    integral = clone.Integral()
    if integral > 0:
        clone.Scale(1.0 / integral)
    clone.GetYaxis().SetTitle("Normalized candidates")
    return clone


def write_hist_csv(hist, path: Path) -> None:
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(["bin", "low_edge", "high_edge", "center", "count", "error"])
        axis = hist.GetXaxis()
        for index in range(1, hist.GetNbinsX() + 1):
            writer.writerow(
                [
                    index,
                    axis.GetBinLowEdge(index),
                    axis.GetBinUpEdge(index),
                    axis.GetBinCenter(index),
                    hist.GetBinContent(index),
                    hist.GetBinError(index),
                ]
            )


def draw_overlay(hists, labels, path_stem: Path, normalized: bool = False) -> None:
    canvas = ROOT.TCanvas("canvas", "", 950, 720)
    canvas.SetLeftMargin(0.12)
    canvas.SetRightMargin(0.04)
    canvas.SetTopMargin(0.08)
    canvas.SetBottomMargin(0.12)

    draw_hists = []
    if normalized:
        colors = [ROOT.kBlue + 2, ROOT.kRed + 1, ROOT.kGreen + 2, ROOT.kMagenta + 2]
        for index, hist in enumerate(hists):
            draw_hists.append(clone_normalized(hist, f"{hist.GetName()}_norm", colors[index % len(colors)]))
    else:
        draw_hists = hists

    max_y = max((hist.GetMaximum() for hist in draw_hists), default=0.0)
    first = True
    for hist in draw_hists:
        hist.SetMaximum(max_y * 1.25 if max_y > 0 else 1.0)
        hist.Draw("hist" if first else "hist same")
        first = False

    line = ROOT.TLine(1.86484, 0.0, 1.86484, max_y * 1.15 if max_y > 0 else 1.0)
    line.SetLineColor(ROOT.kGray + 2)
    line.SetLineStyle(2)
    line.SetLineWidth(2)
    line.Draw("same")

    legend = ROOT.TLegend(0.42, 0.64, 0.92, 0.88)
    legend.SetBorderSize(0)
    legend.SetFillStyle(0)
    for hist, label in zip(draw_hists, labels):
        legend.AddEntry(hist, label, "l")
    legend.AddEntry(line, "PDG D^{0} mass", "l")
    legend.Draw()

    canvas.SaveAs(str(path_stem.with_suffix(".png")))
    canvas.SaveAs(str(path_stem.with_suffix(".pdf")))


def vector_at(vec, index):
    return vec[index]


def passes_minimal_baseline(event, index: int) -> bool:
    pt = vector_at(event.dPt, index)
    y = vector_at(event.dY, index)
    if not (2.0 < pt < 12.0 and abs(y) < 2.0):
        return False
    trk1_pt = vector_at(event.trk1Pt, index)
    trk2_pt = vector_at(event.trk2Pt, index)
    if trk1_pt <= 1.0 or trk2_pt <= 1.0:
        return False
    if abs(vector_at(event.trk1Eta, index)) >= 2.4 or abs(vector_at(event.trk2Eta, index)) >= 2.4:
        return False
    if int(vector_at(event.trk1HighPurity, index)) != 1 or int(vector_at(event.trk2HighPurity, index)) != 1:
        return False
    trk1_err = vector_at(event.trk1PtError, index)
    trk2_err = vector_at(event.trk2PtError, index)
    if trk1_err < 0 or trk2_err < 0:
        return False
    return trk1_err / trk1_pt < 0.1 and trk2_err / trk2_pt < 0.1


def fill_minimal(tree):
    event_keys: set[tuple[int, int, int]] = set()
    h_raw = make_hist("h_minimal_raw_kin", "Minimal forest raw kinematic candidates", ROOT.kBlue + 2)
    h_base = make_hist("h_minimal_baseline", "Minimal forest baseline-track candidates", ROOT.kAzure + 1)

    total_candidates = 0
    raw_kin = 0
    baseline = 0
    for event in tree:
        event_keys.add((int(event.run), int(event.lumi), int(event.event)))
        n_cands = int(event.nD0Candidates)
        total_candidates += n_cands
        for index in range(n_cands):
            mass = vector_at(event.dMass, index)
            pt = vector_at(event.dPt, index)
            y = vector_at(event.dY, index)
            if 2.0 < pt < 12.0 and abs(y) < 2.0:
                h_raw.Fill(mass)
                raw_kin += 1
            if passes_minimal_baseline(event, index):
                h_base.Fill(mass)
                baseline += 1
    return event_keys, h_raw, h_base, {
        "minimal_events": tree.GetEntries(),
        "minimal_total_candidates": total_candidates,
        "minimal_raw_kinematic_candidates": raw_kin,
        "minimal_baseline_candidates": baseline,
    }


def fill_dfinder(reference_tree, event_keys: set[tuple[int, int, int]], max_reference_entries: int):
    h_match_kin = make_hist("h_dfinder_matched_kin", "Dfinder matched-event kinematic candidates", ROOT.kRed + 1)
    h_match_sel = make_hist("h_dfinder_matched_selected", "Dfinder matched-event selected candidates", ROOT.kMagenta + 1)

    reference_tree.SetBranchStatus("*", 0)
    for branch in ["Run", "Lumi", "Event", "Dmass", "Dpt", "Dy", "DpassCut23PAS"]:
        reference_tree.SetBranchStatus(branch, 1)

    entries = reference_tree.GetEntries()
    limit = entries if max_reference_entries <= 0 else min(entries, max_reference_entries)
    matched_events = 0
    matched_candidate_rows = 0
    matched_kin = 0
    matched_sel = 0
    for index in range(limit):
        reference_tree.GetEntry(index)
        key = (int(reference_tree.Run), int(reference_tree.Lumi), int(reference_tree.Event))
        if key not in event_keys:
            continue
        matched_events += 1
        n_candidates = len(reference_tree.Dmass)
        matched_candidate_rows += n_candidates
        for cand_index in range(n_candidates):
            dpt = vector_at(reference_tree.Dpt, cand_index)
            dy = vector_at(reference_tree.Dy, cand_index)
            if not (2.0 < dpt < 12.0 and abs(dy) < 2.0):
                continue
            h_match_kin.Fill(vector_at(reference_tree.Dmass, cand_index))
            matched_kin += 1
            if int(vector_at(reference_tree.DpassCut23PAS, cand_index)):
                h_match_sel.Fill(vector_at(reference_tree.Dmass, cand_index))
                matched_sel += 1

    h_full_sel = make_hist("h_dfinder_full_selected", "Dfinder full selected reference", ROOT.kGreen + 2)
    ROOT.gROOT.cd()
    reference_tree.Draw(
        "Dmass>>h_dfinder_full_selected",
        "Dpt>2 && Dpt<12 && abs(Dy)<2 && DpassCut23PAS",
        "goff",
    )
    h_full_sel.SetDirectory(0)
    h_full_sel.SetLineColor(ROOT.kGreen + 2)
    h_full_sel.SetLineWidth(2)

    return h_match_kin, h_match_sel, h_full_sel, {
        "dfinder_reference_entries": entries,
        "dfinder_reference_scanned_entries": limit,
        "dfinder_matched_events": matched_events,
        "dfinder_matched_candidate_rows": matched_candidate_rows,
        "dfinder_matched_kinematic_candidates": matched_kin,
        "dfinder_matched_selected_candidates": matched_sel,
        "dfinder_full_selected_candidates": h_full_sel.Integral(),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--minimal", required=True)
    parser.add_argument("--reference", required=True)
    parser.add_argument("--outdir", required=True)
    parser.add_argument("--input-label", default="")
    parser.add_argument("--max-reference-entries", type=int, default=0)
    args = parser.parse_args()

    if not owned_output(args.outdir):
        raise SystemExit(f"Refusing to write outside Evan-owned paths: {args.outdir}")
    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)

    minimal_file, minimal_tree = open_tree(args.minimal, "d0MinimalForest/d0minimal")
    reference_file, reference_tree = open_tree(args.reference, "Tree")

    event_keys, h_min_raw, h_min_base, minimal_counts = fill_minimal(minimal_tree)
    h_match_kin, h_match_sel, h_full_sel, dfinder_counts = fill_dfinder(
        reference_tree, event_keys, args.max_reference_entries
    )

    output_root = ROOT.TFile(str(outdir / "minimal_vs_dfinder_histograms.root"), "RECREATE")
    for hist in [h_min_raw, h_min_base, h_match_kin, h_match_sel, h_full_sel]:
        hist.Write()
        write_hist_csv(hist, outdir / f"{hist.GetName()}.csv")
    output_root.Close()

    draw_overlay(
        [h_min_raw, h_min_base, h_match_kin, h_match_sel],
        [
            "Minimal raw, matched events",
            "Minimal baseline tracks, matched events",
            "Dfinder raw kinematic, matched events",
            "Dfinder selected, matched events",
        ],
        outdir / "minimal_vs_dfinder_matched_events",
        normalized=False,
    )
    draw_overlay(
        [h_min_raw, h_min_base, h_match_kin, h_match_sel],
        [
            "Minimal raw, matched events",
            "Minimal baseline tracks, matched events",
            "Dfinder raw kinematic, matched events",
            "Dfinder selected, matched events",
        ],
        outdir / "minimal_vs_dfinder_matched_events_normalized",
        normalized=True,
    )
    draw_overlay(
        [h_min_base, h_full_sel],
        ["Minimal baseline tracks, processed events", "Dfinder full selected reference"],
        outdir / "minimal_baseline_vs_full_dfinder_selected_normalized",
        normalized=True,
    )

    manifest = outdir / "minimal_vs_dfinder_manifest.md"
    with manifest.open("w", encoding="utf-8") as handle:
        handle.write("# Minimal Forest vs Dfinder Comparison\n\n")
        handle.write(f"- Minimal forest: `{args.minimal}`\n")
        handle.write(f"- Dfinder reference: `{args.reference}`\n")
        handle.write(f"- Input label: `{args.input_label}`\n")
        handle.write(f"- Matched unique events: `{len(event_keys)}`\n\n")
        handle.write("## Counts\n\n")
        for key, value in {**minimal_counts, **dfinder_counts}.items():
            handle.write(f"- `{key}`: `{value}`\n")
        handle.write("\n## Interpretation Guardrail\n\n")
        handle.write(
            "The minimal forest does not contain Dfinder's secondary-vertex fit, "
            "decay-length significance, pointing-angle, or topological selections. "
            "Its baseline-track spectrum is therefore expected to be a broad "
            "combinatorial distribution, not a clean D0 peak.\n"
        )

    print(manifest)
    minimal_file.Close()
    reference_file.Close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
