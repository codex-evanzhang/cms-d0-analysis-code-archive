#!/usr/bin/env python3
"""Train and validate D0 topology BDTs, then apply them to recreated forests.

The BDT training protocol is intentionally separated from the data peak:

* signal proxy: prompt truth-matched D0 MC candidates;
* background proxy: 2023 data candidates in the mass sidebands;
* application sample: 2023 data candidates in the full fit window.

Two threshold scans are reported. The first is a fully peak-blind Punzi scan on
held-out MC signal and held-out data sidebands. The second is a data-split yield
scan: one half of the data chooses the threshold with a sideband-subtracted
peak-significance diagnostic, and the other half validates the frozen threshold.
"""

from __future__ import annotations

import argparse
import csv
import glob
import json
import math
import os
import shutil
from array import array
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import ROOT


D0_MASS = 1.86484
MASS_MIN = 1.70
MASS_MAX = 2.05
SIDEBAND_INNER = 0.07
SIDEBAND_OUTER = 0.12
SIGNAL_HALF_WIDTH = 0.035
ZDC_PLUS_1N = 1100.0
ZDC_MINUS_1N = 1000.0
HF_PLUS_GAP = 9.2
HF_MINUS_GAP = 8.6


@dataclass(frozen=True)
class AnalysisBin:
    label: str
    abs_y_min: float
    abs_y_max: float
    pt_min: float
    pt_max: float
    an_alpha: float
    an_svpv: float
    an_chi2: float
    an_dtheta: float


BINS = [
    AnalysisBin("absy0to1_pt2to5", 0.0, 1.0, 2.0, 5.0, 0.40, 2.5, 0.10, 0.50),
    AnalysisBin("absy0to1_pt5to8", 0.0, 1.0, 5.0, 8.0, 0.35, 3.5, 0.10, 0.30),
    AnalysisBin("absy0to1_pt8to12", 0.0, 1.0, 8.0, 12.0, 0.40, 3.5, 0.10, 0.30),
    AnalysisBin("absy1to2_pt2to5", 1.0, 2.0, 2.0, 5.0, 0.20, 2.5, 0.10, 0.30),
    AnalysisBin("absy1to2_pt5to8", 1.0, 2.0, 5.0, 8.0, 0.25, 3.5, 0.10, 0.30),
    AnalysisBin("absy1to2_pt8to12", 1.0, 2.0, 8.0, 12.0, 0.25, 3.5, 0.10, 0.30),
]


def owned_output(path: str | Path) -> bool:
    text = str(path)
    return text.startswith("/afs/cern.ch/user/e/evzhang/") or text.startswith(
        "/afs/cern.ch/work/e/evzhang/"
    ) or text.startswith("/eos/user/e/evzhang/")


def finite(*values: float) -> bool:
    return all(math.isfinite(value) for value in values)


def find_bin(pt: float, y: float) -> int:
    ay = abs(y)
    for idx, b in enumerate(BINS):
        in_pt = pt >= b.pt_min and (pt < b.pt_max or (b.pt_max == 12.0 and pt <= b.pt_max))
        in_y = ay >= b.abs_y_min and (ay < b.abs_y_max or (b.abs_y_max == 2.0 and ay <= b.abs_y_max))
        if in_pt and in_y:
            return idx
    return -1


def in_fit_range(mass: float) -> bool:
    return MASS_MIN < mass < MASS_MAX


def in_sideband(mass: float) -> bool:
    dm = abs(mass - D0_MASS)
    return SIDEBAND_INNER < dm < SIDEBAND_OUTER


def in_signal_window(mass: float) -> bool:
    return abs(mass - D0_MASS) < SIGNAL_HALF_WIDTH


def best_vertex_z(n_vtx: int, z_vtx, pt_sum_vtx) -> float:
    if n_vtx <= 0 or z_vtx is None or len(z_vtx) == 0:
        return float("nan")
    best = 0
    if pt_sum_vtx is not None and len(pt_sum_vtx) > 0:
        for idx in range(1, min(n_vtx, len(pt_sum_vtx))):
            if pt_sum_vtx[idx] > pt_sum_vtx[best]:
                best = idx
    if best >= len(z_vtx):
        best = 0
    return float(z_vtx[best])


def max_hf_energy(pf_id, pf_eta, pf_e, eta_min: float, eta_max: float) -> float:
    out = 0.0
    n = min(len(pf_id), len(pf_eta), len(pf_e))
    for i in range(n):
        if int(pf_id[i]) in (6, 7) and eta_min < float(pf_eta[i]) < eta_max:
            out = max(out, float(pf_e[i]))
    return out


def split_flag(entry: int, cand: int, bin_index: int) -> bool:
    value = (entry * 1103515245 + cand * 2654435761 + bin_index * 97531 + 12345) & 0xFFFFFFFF
    return (value % 2) == 0


class BranchTree:
    def __init__(self, name: str):
        self.values = {
            "mass": array("f", [0.0]),
            "pt": array("f", [0.0]),
            "y": array("f", [0.0]),
            "alpha": array("f", [0.0]),
            "svpvSig": array("f", [0.0]),
            "chi2cl": array("f", [0.0]),
            "dtheta": array("f", [0.0]),
            "weight": array("f", [1.0]),
        }
        self.ints = {
            "bin": array("i", [0]),
        }
        self.tree = ROOT.TTree(name, name)
        for key, val in self.values.items():
            self.tree.Branch(key, val, f"{key}/F")
        for key, val in self.ints.items():
            self.tree.Branch(key, val, f"{key}/I")

    def fill(self, row: dict[str, float], bin_index: int) -> None:
        for key in self.values:
            self.values[key][0] = float(row.get(key, 1.0 if key == "weight" else 0.0))
        self.ints["bin"][0] = bin_index
        self.tree.Fill()


class FlatWriter:
    def __init__(self, path: Path):
        self.path = path
        self.file = ROOT.TFile(str(path), "RECREATE")
        self.trees: dict[str, BranchTree] = {}
        for idx in range(len(BINS)):
            for sample in ("sig_train", "sig_test", "bkg_train", "bkg_test", "data"):
                name = f"{sample}_bin{idx}"
                self.trees[name] = BranchTree(name)

    def fill_signal(self, bin_index: int, row: dict[str, float], train: bool) -> None:
        self.trees[f"{'sig_train' if train else 'sig_test'}_bin{bin_index}"].fill(row, bin_index)

    def fill_background(self, bin_index: int, row: dict[str, float], train: bool) -> None:
        self.trees[f"{'bkg_train' if train else 'bkg_test'}_bin{bin_index}"].fill(row, bin_index)

    def fill_data(self, bin_index: int, row: dict[str, float]) -> None:
        self.trees[f"data_bin{bin_index}"].fill(row, bin_index)

    def close(self) -> None:
        self.file.cd()
        for tree in self.trees.values():
            tree.tree.Write()
        self.file.Close()


def branch_exists(tree, name: str) -> bool:
    return bool(tree and tree.GetBranch(name))


def vector_size(*vectors) -> int:
    sizes = [len(v) for v in vectors if v is not None]
    return min(sizes) if sizes else 0


def candidate_row(mass, pt, y, alpha, distance, distance_err, chi2cl, dtheta) -> dict[str, float] | None:
    if distance_err <= 0:
        return None
    svpv_sig = distance / distance_err
    if not finite(mass, pt, y, alpha, svpv_sig, chi2cl, dtheta):
        return None
    return {
        "mass": mass,
        "pt": pt,
        "y": y,
        "alpha": alpha,
        "svpvSig": svpv_sig,
        "chi2cl": chi2cl,
        "dtheta": dtheta,
        "weight": 1.0,
    }


def pass_track_baseline(
    pt: float,
    y: float,
    trk1_pt: float,
    trk2_pt: float,
    trk1_pt_err: float,
    trk2_pt_err: float,
    trk1_eta: float,
    trk2_eta: float,
    trk1_hp: bool | None = None,
    trk2_hp: bool | None = None,
    require_high_purity: bool = False,
) -> bool:
    if pt <= 2.0 or pt > 12.0 or abs(y) > 2.0:
        return False
    if trk1_pt <= 1.0 or trk2_pt <= 1.0:
        return False
    if abs(trk1_eta) >= 2.4 or abs(trk2_eta) >= 2.4:
        return False
    if trk1_pt <= 0.0 or trk2_pt <= 0.0:
        return False
    if trk1_pt_err / trk1_pt >= 0.1 or trk2_pt_err / trk2_pt >= 0.1:
        return False
    if require_high_purity and (not trk1_hp or not trk2_hp):
        return False
    return True


def truth_matched(tree, idx: int) -> bool:
    if branch_exists(tree, "DisSignalCalcPrompt") and idx < len(tree.DisSignalCalcPrompt):
        return bool(tree.DisSignalCalcPrompt[idx])
    if branch_exists(tree, "DisSignalCalc") and idx < len(tree.DisSignalCalc):
        return bool(tree.DisSignalCalc[idx])
    if branch_exists(tree, "Dgen") and idx < len(tree.Dgen):
        return int(tree.Dgen[idx]) != 0
    return False


def collect_mc_signal(writer: FlatWriter, mc_path: str, max_events: int, max_signal_per_bin: int) -> dict:
    ROOT.TH1.AddDirectory(False)
    file = ROOT.TFile.Open(mc_path)
    if not file or file.IsZombie():
        raise RuntimeError(f"could not open MC file {mc_path}")
    tree = file.Get("Tree")
    if not tree:
        raise RuntimeError(f"missing Tree in MC file {mc_path}")
    for name in [
        "Dsize",
        "Dmass",
        "Dpt",
        "Dy",
        "Dalpha",
        "DsvpvDistance",
        "DsvpvDisErr",
        "Dchi2cl",
        "Ddtheta",
        "Dtrk1Pt",
        "Dtrk2Pt",
        "Dtrk1PtErr",
        "Dtrk2PtErr",
        "Dtrk1Eta",
        "Dtrk2Eta",
    ]:
        if not branch_exists(tree, name):
            raise RuntimeError(f"missing MC branch {name}")

    counts = [0 for _ in BINS]
    entries = tree.GetEntries()
    if max_events >= 0:
        entries = min(entries, max_events)
    for entry in range(entries):
        tree.GetEntry(entry)
        n_cand = min(
            int(tree.Dsize),
            vector_size(
                tree.Dmass,
                tree.Dpt,
                tree.Dy,
                tree.Dalpha,
                tree.DsvpvDistance,
                tree.DsvpvDisErr,
                tree.Dchi2cl,
                tree.Ddtheta,
            ),
        )
        for idx in range(n_cand):
            mass = float(tree.Dmass[idx])
            pt = float(tree.Dpt[idx])
            y = float(tree.Dy[idx])
            if not in_fit_range(mass):
                continue
            bin_index = find_bin(pt, y)
            if bin_index < 0:
                continue
            if max_signal_per_bin > 0 and counts[bin_index] >= max_signal_per_bin:
                continue
            if not truth_matched(tree, idx):
                continue
            if not pass_track_baseline(
                pt,
                y,
                float(tree.Dtrk1Pt[idx]),
                float(tree.Dtrk2Pt[idx]),
                float(tree.Dtrk1PtErr[idx]),
                float(tree.Dtrk2PtErr[idx]),
                float(tree.Dtrk1Eta[idx]),
                float(tree.Dtrk2Eta[idx]),
                require_high_purity=False,
            ):
                continue
            row = candidate_row(
                mass,
                pt,
                y,
                float(tree.Dalpha[idx]),
                float(tree.DsvpvDistance[idx]),
                float(tree.DsvpvDisErr[idx]),
                float(tree.Dchi2cl[idx]),
                float(tree.Ddtheta[idx]),
            )
            if row is None:
                continue
            writer.fill_signal(bin_index, row, split_flag(entry, idx, bin_index))
            counts[bin_index] += 1
        if max_signal_per_bin > 0 and all(count >= max_signal_per_bin for count in counts):
            break
        if entry and entry % 250000 == 0:
            print(f"MC progress entry={entry} counts={counts}", flush=True)
    file.Close()
    return {"mc_events_scanned": int(entries), "signal_counts": counts}


def collect_data_candidates(writer: FlatWriter, data_files: list[str], max_files: int) -> dict:
    files = data_files[: max_files if max_files > 0 else None]
    side_counts = [0 for _ in BINS]
    data_counts = [0 for _ in BINS]
    events_seen = 0
    events_pass = 0
    for file_index, path in enumerate(files, 1):
        root_file = ROOT.TFile.Open(path)
        if not root_file or root_file.IsZombie():
            print(f"WARNING could not open data file {path}", flush=True)
            continue
        hi_tree = root_file.Get("hiEvtAnalyzer/HiTree")
        track_tree = root_file.Get("PbPbTracks/trackTree")
        skim_tree = root_file.Get("skimanalysis/HltTree")
        met_tree = root_file.Get("l1MetFilterRecoTree/MetFilterRecoTree")
        zdc_tree = root_file.Get("zdcanalyzer/zdcrechit")
        pf_tree = root_file.Get("particleFlowAnalyser/pftree")
        d_tree = root_file.Get("Dfinder/ntDkpi")
        if not all([hi_tree, track_tree, skim_tree, met_tree, zdc_tree, pf_tree, d_tree]):
            print(f"WARNING missing required tree in {path}", flush=True)
            root_file.Close()
            continue
        met_reader = ROOT.TTreeReader(met_tree)
        csc_filter = ROOT.TTreeReaderValue["Bool_t"](met_reader, "cscTightHalo2015Filter")
        entries = min(
            hi_tree.GetEntries(),
            track_tree.GetEntries(),
            skim_tree.GetEntries(),
            met_tree.GetEntries(),
            zdc_tree.GetEntries(),
            pf_tree.GetEntries(),
            d_tree.GetEntries(),
        )
        for entry in range(entries):
            hi_tree.GetEntry(entry)
            track_tree.GetEntry(entry)
            skim_tree.GetEntry(entry)
            if not met_reader.Next():
                print(f"WARNING MET filter reader stopped early in {path}", flush=True)
                break
            zdc_tree.GetEntry(entry)
            pf_tree.GetEntry(entry)
            d_tree.GetEntry(entry)
            events_seen += 1

            if branch_exists(skim_tree, "pzdcEnergyFilter0nOr") and int(skim_tree.pzdcEnergyFilter0nOr) != 1:
                continue
            if int(skim_tree.pprimaryVertexFilter) != 1:
                continue
            best_z = best_vertex_z(int(track_tree.nVtx), track_tree.zVtx, track_tree.ptSumVtx)
            if not math.isfinite(best_z) or abs(best_z) >= 15.0:
                continue
            if int(track_tree.nVtx) <= 0 or int(track_tree.nVtx) > 3:
                continue
            if int(skim_tree.pclusterCompatibilityFilter) != 1:
                continue
            if not bool(csc_filter.Get()[0]):
                continue
            zdc_plus = float(zdc_tree.sumPlus)
            zdc_minus = float(zdc_tree.sumMinus)
            gamma_n = zdc_minus > ZDC_MINUS_1N and zdc_plus < ZDC_PLUS_1N
            n_gamma = zdc_plus > ZDC_PLUS_1N and zdc_minus < ZDC_MINUS_1N
            if not (gamma_n or n_gamma):
                continue
            hf_plus = max_hf_energy(pf_tree.pfId, pf_tree.pfEta, pf_tree.pfE, 3.0, 5.2)
            hf_minus = max_hf_energy(pf_tree.pfId, pf_tree.pfEta, pf_tree.pfE, -5.2, -3.0)
            if not ((gamma_n and hf_plus < HF_PLUS_GAP) or (n_gamma and hf_minus < HF_MINUS_GAP)):
                continue
            events_pass += 1

            n_cand = min(
                int(d_tree.Dsize),
                vector_size(
                    d_tree.Dmass,
                    d_tree.Dpt,
                    d_tree.Dy,
                    d_tree.Dalpha,
                    d_tree.DsvpvDistance,
                    d_tree.DsvpvDisErr,
                    d_tree.Dchi2cl,
                    d_tree.Ddtheta,
                ),
            )
            has_hp = branch_exists(d_tree, "Dtrk1highPurity") and branch_exists(d_tree, "Dtrk2highPurity")
            for idx in range(n_cand):
                mass = float(d_tree.Dmass[idx])
                pt = float(d_tree.Dpt[idx])
                y = float(d_tree.Dy[idx])
                if not in_fit_range(mass):
                    continue
                bin_index = find_bin(pt, y)
                if bin_index < 0:
                    continue
                trk1_hp = bool(d_tree.Dtrk1highPurity[idx]) if has_hp else None
                trk2_hp = bool(d_tree.Dtrk2highPurity[idx]) if has_hp else None
                if not pass_track_baseline(
                    pt,
                    y,
                    float(d_tree.Dtrk1Pt[idx]),
                    float(d_tree.Dtrk2Pt[idx]),
                    float(d_tree.Dtrk1PtErr[idx]),
                    float(d_tree.Dtrk2PtErr[idx]),
                    float(d_tree.Dtrk1Eta[idx]),
                    float(d_tree.Dtrk2Eta[idx]),
                    trk1_hp,
                    trk2_hp,
                    require_high_purity=has_hp,
                ):
                    continue
                row = candidate_row(
                    mass,
                    pt,
                    y,
                    float(d_tree.Dalpha[idx]),
                    float(d_tree.DsvpvDistance[idx]),
                    float(d_tree.DsvpvDisErr[idx]),
                    float(d_tree.Dchi2cl[idx]),
                    float(d_tree.Ddtheta[idx]),
                )
                if row is None:
                    continue
                writer.fill_data(bin_index, row)
                data_counts[bin_index] += 1
                if in_sideband(mass):
                    writer.fill_background(bin_index, row, split_flag(entry, idx, bin_index))
                    side_counts[bin_index] += 1
        root_file.Close()
        print(f"DATA progress file={file_index}/{len(files)} events_seen={events_seen} pass={events_pass}", flush=True)
    return {
        "data_files": len(files),
        "data_events_seen": events_seen,
        "data_events_pass": events_pass,
        "data_candidate_counts": data_counts,
        "data_sideband_counts": side_counts,
    }


def tree_entries(file: ROOT.TFile, name: str) -> int:
    tree = file.Get(name)
    return int(tree.GetEntries()) if tree else 0


def load_scores(reader, tree) -> list[tuple[float, dict[str, float]]]:
    values = {key: array("f", [0.0]) for key in ("alpha", "svpvSig", "chi2cl", "dtheta")}
    for key, arr in values.items():
        tree.SetBranchAddress(key, arr)
    aux = {key: array("f", [0.0]) for key in ("mass", "pt", "y")}
    for key, arr in aux.items():
        tree.SetBranchAddress(key, arr)
    scores: list[tuple[float, dict[str, float]]] = []
    for i in range(tree.GetEntries()):
        tree.GetEntry(i)
        score = float(reader.EvaluateMVA("BDT"))
        scores.append(
            (
                score,
                {
                    "mass": float(aux["mass"][0]),
                    "pt": float(aux["pt"][0]),
                    "y": float(aux["y"][0]),
                    "alpha": float(values["alpha"][0]),
                    "svpvSig": float(values["svpvSig"][0]),
                    "chi2cl": float(values["chi2cl"][0]),
                    "dtheta": float(values["dtheta"][0]),
                },
            )
        )
    return scores


def make_reader(weight_xml: Path):
    variables = {key: array("f", [0.0]) for key in ("alpha", "svpvSig", "chi2cl", "dtheta")}
    reader = ROOT.TMVA.Reader("!Color:!Silent")
    reader.AddVariable("alpha", variables["alpha"])
    reader.AddVariable("svpvSig", variables["svpvSig"])
    reader.AddVariable("chi2cl", variables["chi2cl"])
    reader.AddVariable("dtheta", variables["dtheta"])
    reader.BookMVA("BDT", str(weight_xml))

    def evaluate(row: dict[str, float]) -> float:
        for key in variables:
            variables[key][0] = float(row[key])
        return float(reader.EvaluateMVA("BDT"))

    return reader, evaluate


def hist_from_scores(name: str, scores: list[tuple[float, dict[str, float]]], bins: int = 80):
    hist = ROOT.TH1D(name, name, bins, -1.0, 1.0)
    hist.SetDirectory(0)
    for score, _ in scores:
        hist.Fill(score)
    return hist


def scan_threshold(sig_scores, bkg_scores) -> dict[str, float]:
    if not sig_scores or not bkg_scores:
        return {
            "threshold": 999.0,
            "punzi": 0.0,
            "signal_eff": 0.0,
            "bkg_rejection": 0.0,
            "bkg_pass": 0.0,
        }
    thresholds = [-0.95 + i * 0.01 for i in range(191)]
    best = None
    sideband_to_signal_width = (2.0 * SIGNAL_HALF_WIDTH) / (2.0 * (SIDEBAND_OUTER - SIDEBAND_INNER))
    for threshold in thresholds:
        sig_pass = sum(1 for score, _ in sig_scores if score >= threshold)
        bkg_pass = sum(1 for score, _ in bkg_scores if score >= threshold)
        sig_eff = sig_pass / len(sig_scores)
        bkg_frac = bkg_pass / len(bkg_scores)
        b_est = bkg_pass * sideband_to_signal_width
        punzi = sig_eff / (1.5 + math.sqrt(max(b_est, 0.0)))
        if sig_eff < 0.20:
            punzi *= 0.5
        item = {
            "threshold": threshold,
            "punzi": punzi,
            "signal_eff": sig_eff,
            "bkg_rejection": 1.0 - bkg_frac,
            "bkg_pass": bkg_pass,
        }
        if best is None or (item["punzi"], item["signal_eff"]) > (best["punzi"], best["signal_eff"]):
            best = item
    assert best is not None
    return best


def an_pass(row: dict[str, float], bin_index: int) -> bool:
    b = BINS[bin_index]
    return (
        row["alpha"] < b.an_alpha
        and row["svpvSig"] > b.an_svpv
        and row["chi2cl"] > b.an_chi2
        and row["dtheta"] < b.an_dtheta
    )


def count_yield(rows: Iterable[dict[str, float]]) -> dict[str, float]:
    signal = 0
    side = 0
    lower = 0
    upper = 0
    for row in rows:
        mass = row["mass"]
        dm = mass - D0_MASS
        if abs(dm) < SIGNAL_HALF_WIDTH:
            signal += 1
        if SIDEBAND_INNER < abs(dm) < SIDEBAND_OUTER:
            side += 1
            if dm < 0:
                lower += 1
            else:
                upper += 1
    scale = (2.0 * SIGNAL_HALF_WIDTH) / (2.0 * (SIDEBAND_OUTER - SIDEBAND_INNER))
    variance = signal + scale * scale * side
    diagnostic_yield = signal - scale * side
    return {
        "signal_window": signal,
        "sideband": side,
        "sideband_scaled": scale * side,
        "diagnostic_yield": diagnostic_yield,
        "diagnostic_uncertainty": math.sqrt(variance) if variance > 0 else 0.0,
        "diagnostic_significance": diagnostic_yield / math.sqrt(variance) if variance > 0 else 0.0,
        "lower_sideband": lower,
        "upper_sideband": upper,
        "sideband_asymmetry": (upper - lower) / (upper + lower) if (upper + lower) > 0 else 0.0,
    }


def rows_above_threshold(scores, threshold: float) -> list[dict[str, float]]:
    return [row for score, row in scores if score >= threshold]


def split_scores_by_index(scores):
    optimize = []
    validate = []
    for idx, item in enumerate(scores):
        if idx % 2 == 0:
            optimize.append(item)
        else:
            validate.append(item)
    return optimize, validate


def threshold_grid_from_scores(scores) -> list[float]:
    base = [-0.95 + i * 0.01 for i in range(191)]
    if not scores:
        return base
    sorted_scores = sorted(score for score, _ in scores)
    quantiles = []
    for frac in (0.05, 0.10, 0.15, 0.20, 0.25, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80):
        idx = min(len(sorted_scores) - 1, max(0, int(frac * len(sorted_scores))))
        quantiles.append(sorted_scores[idx])
    return sorted({round(value, 5) for value in base + quantiles})


def scan_data_split_threshold(sig_scores, bkg_scores, data_scores) -> dict[str, float]:
    """Choose a threshold on half the data and validate it on the other half."""
    if not data_scores or not sig_scores or not bkg_scores:
        return {
            "threshold": 999.0,
            "opt_yield": 0.0,
            "opt_significance": 0.0,
            "val_yield": 0.0,
            "val_significance": 0.0,
            "signal_eff": 0.0,
            "bkg_rejection": 0.0,
            "scan_rows": [],
        }
    opt_scores, val_scores = split_scores_by_index(data_scores)
    thresholds = threshold_grid_from_scores(data_scores)
    min_signal_eff = 0.30
    best = None
    scan_rows = []
    for threshold in thresholds:
        sig_pass = sum(1 for score, _ in sig_scores if score >= threshold)
        bkg_pass = sum(1 for score, _ in bkg_scores if score >= threshold)
        sig_eff = sig_pass / len(sig_scores)
        bkg_frac = bkg_pass / len(bkg_scores)
        opt_yield = count_yield(rows_above_threshold(opt_scores, threshold))
        val_yield = count_yield(rows_above_threshold(val_scores, threshold))
        objective = opt_yield["diagnostic_significance"]
        if sig_eff < min_signal_eff:
            objective -= 1000.0 * (min_signal_eff - sig_eff)
        item = {
            "threshold": threshold,
            "objective": objective,
            "opt_yield": opt_yield["diagnostic_yield"],
            "opt_significance": opt_yield["diagnostic_significance"],
            "val_yield": val_yield["diagnostic_yield"],
            "val_significance": val_yield["diagnostic_significance"],
            "signal_eff": sig_eff,
            "bkg_rejection": 1.0 - bkg_frac,
            "opt_signal_window": opt_yield["signal_window"],
            "opt_sideband": opt_yield["sideband"],
            "val_signal_window": val_yield["signal_window"],
            "val_sideband": val_yield["sideband"],
        }
        scan_rows.append(item)
        if best is None or (
            item["objective"],
            item["val_significance"],
            item["signal_eff"],
        ) > (
            best["objective"],
            best["val_significance"],
            best["signal_eff"],
        ):
            best = item
    assert best is not None
    best = dict(best)
    best["scan_rows"] = scan_rows
    return best


def draw_threshold_scan(outdir: Path, label: str, scan_rows: list[dict[str, float]], chosen: float) -> None:
    if not scan_rows:
        return
    graph_opt = ROOT.TGraph()
    graph_val = ROOT.TGraph()
    graph_eff = ROOT.TGraph()
    graph_rej = ROOT.TGraph()
    for idx, row in enumerate(scan_rows):
        graph_opt.SetPoint(idx, row["threshold"], row["opt_significance"])
        graph_val.SetPoint(idx, row["threshold"], row["val_significance"])
        graph_eff.SetPoint(idx, row["threshold"], row["signal_eff"])
        graph_rej.SetPoint(idx, row["threshold"], row["bkg_rejection"])
    canvas = ROOT.TCanvas(f"c_scan_{label}", "", 900, 700)
    canvas.SetLeftMargin(0.12)
    graph_opt.SetTitle(f"{label};BDT threshold;diagnostic / efficiency")
    graph_opt.SetLineColor(ROOT.kBlue + 1)
    graph_opt.SetLineWidth(2)
    graph_val.SetLineColor(ROOT.kCyan + 2)
    graph_val.SetLineWidth(2)
    graph_eff.SetLineColor(ROOT.kGreen + 2)
    graph_eff.SetLineWidth(2)
    graph_rej.SetLineColor(ROOT.kRed + 1)
    graph_rej.SetLineWidth(2)
    graph_opt.Draw("AL")
    graph_val.Draw("L same")
    graph_eff.Draw("L same")
    graph_rej.Draw("L same")
    ymin = min(
        min(row["opt_significance"] for row in scan_rows),
        min(row["val_significance"] for row in scan_rows),
        0.0,
    )
    ymax = max(
        max(row["opt_significance"] for row in scan_rows),
        max(row["val_significance"] for row in scan_rows),
        1.05,
    )
    graph_opt.GetYaxis().SetRangeUser(ymin - 0.1 * abs(ymin), ymax * 1.18)
    line = ROOT.TLine(chosen, ymin, chosen, ymax)
    line.SetLineStyle(2)
    line.Draw()
    legend = ROOT.TLegend(0.50, 0.68, 0.88, 0.89)
    legend.SetBorderSize(0)
    legend.SetFillStyle(0)
    legend.AddEntry(graph_opt, "opt half significance", "l")
    legend.AddEntry(graph_val, "validation half significance", "l")
    legend.AddEntry(graph_eff, "MC signal efficiency", "l")
    legend.AddEntry(graph_rej, "sideband rejection", "l")
    legend.Draw()
    canvas.SaveAs(str(outdir / f"{label}_bdt_threshold_scan.png"))
    canvas.SaveAs(str(outdir / f"{label}_bdt_threshold_scan.pdf"))


def draw_mass_plot(outdir: Path, label: str, rows_bdt, rows_an, rows_base) -> None:
    hist_base = ROOT.TH1D(f"{label}_base", f"{label};m(K#pi) [GeV];Candidates", 70, MASS_MIN, MASS_MAX)
    hist_bdt = ROOT.TH1D(f"{label}_bdt", f"{label};m(K#pi) [GeV];Candidates", 70, MASS_MIN, MASS_MAX)
    hist_an = ROOT.TH1D(f"{label}_an", f"{label};m(K#pi) [GeV];Candidates", 70, MASS_MIN, MASS_MAX)
    for hist in (hist_base, hist_bdt, hist_an):
        hist.SetDirectory(0)
        hist.Sumw2()
    for row in rows_base:
        hist_base.Fill(row["mass"])
    for row in rows_bdt:
        hist_bdt.Fill(row["mass"])
    for row in rows_an:
        hist_an.Fill(row["mass"])
    hist_base.SetLineColor(ROOT.kGray + 1)
    hist_an.SetLineColor(ROOT.kRed + 1)
    hist_bdt.SetLineColor(ROOT.kBlue + 1)
    hist_base.SetLineWidth(2)
    hist_an.SetLineWidth(2)
    hist_bdt.SetLineWidth(2)
    canvas = ROOT.TCanvas(f"c_{label}", "", 900, 700)
    canvas.SetLeftMargin(0.12)
    canvas.SetBottomMargin(0.12)
    ymax = max(hist_base.GetMaximum(), hist_an.GetMaximum(), hist_bdt.GetMaximum()) * 1.25 + 1.0
    hist_base.SetMaximum(ymax)
    hist_base.Draw("hist")
    hist_an.Draw("hist same")
    hist_bdt.Draw("hist same")
    line = ROOT.TLine(D0_MASS, 0.0, D0_MASS, ymax * 0.85)
    line.SetLineStyle(2)
    line.Draw()
    legend = ROOT.TLegend(0.55, 0.72, 0.88, 0.88)
    legend.SetBorderSize(0)
    legend.SetFillStyle(0)
    legend.AddEntry(hist_base, "baseline", "l")
    legend.AddEntry(hist_an, "AN rectangular", "l")
    legend.AddEntry(hist_bdt, "BDT optimized", "l")
    legend.Draw()
    canvas.SaveAs(str(outdir / f"{label}_mass_bdt_vs_an.png"))
    canvas.SaveAs(str(outdir / f"{label}_mass_bdt_vs_an.pdf"))


def train_and_apply(flat_path: Path, outdir: Path) -> dict:
    ROOT.TMVA.Tools.Instance()
    flat_file = ROOT.TFile.Open(str(flat_path))
    if not flat_file or flat_file.IsZombie():
        raise RuntimeError(f"could not open flat ntuple {flat_path}")
    workdir = Path.cwd()
    os.chdir(outdir)
    plots_dir = outdir / "plots"
    plots_dir.mkdir(exist_ok=True)
    rows_summary = []
    rows_overtraining = []
    rows_yield = []

    for bin_index, b in enumerate(BINS):
        sig_train = flat_file.Get(f"sig_train_bin{bin_index}")
        sig_test = flat_file.Get(f"sig_test_bin{bin_index}")
        bkg_train = flat_file.Get(f"bkg_train_bin{bin_index}")
        bkg_test = flat_file.Get(f"bkg_test_bin{bin_index}")
        data_tree = flat_file.Get(f"data_bin{bin_index}")
        counts = {
            "sig_train": int(sig_train.GetEntries()) if sig_train else 0,
            "sig_test": int(sig_test.GetEntries()) if sig_test else 0,
            "bkg_train": int(bkg_train.GetEntries()) if bkg_train else 0,
            "bkg_test": int(bkg_test.GetEntries()) if bkg_test else 0,
            "data": int(data_tree.GetEntries()) if data_tree else 0,
        }
        if min(counts["sig_train"], counts["sig_test"], counts["bkg_train"], counts["bkg_test"]) < 20:
            rows_summary.append({"bin": b.label, "status": "insufficient_training_stats", **counts})
            continue

        tmva_output = ROOT.TFile(str(outdir / f"tmva_{b.label}.root"), "RECREATE")
        factory = ROOT.TMVA.Factory(
            f"TMVA_{b.label}",
            tmva_output,
            "!V:!Silent:Color:AnalysisType=Classification",
        )
        dataset = f"dataset_{b.label}"
        loader = ROOT.TMVA.DataLoader(dataset)
        for var in ("alpha", "svpvSig", "chi2cl", "dtheta"):
            loader.AddVariable(var, "F")
        loader.AddSignalTree(sig_train, 1.0, ROOT.TMVA.Types.kTraining)
        loader.AddSignalTree(sig_test, 1.0, ROOT.TMVA.Types.kTesting)
        loader.AddBackgroundTree(bkg_train, 1.0, ROOT.TMVA.Types.kTraining)
        loader.AddBackgroundTree(bkg_test, 1.0, ROOT.TMVA.Types.kTesting)
        loader.PrepareTrainingAndTestTree(ROOT.TCut(""), ROOT.TCut(""), "NormMode=NumEvents:!V")
        factory.BookMethod(
            loader,
            ROOT.TMVA.Types.kBDT,
            "BDT",
            "!H:!V:NTrees=300:MinNodeSize=2.5%:MaxDepth=3:BoostType=AdaBoost:"
            "AdaBoostBeta=0.5:UseBaggedBoost:BaggedSampleFraction=0.5:"
            "SeparationType=GiniIndex:nCuts=20",
        )
        factory.TrainAllMethods()
        factory.TestAllMethods()
        factory.EvaluateAllMethods()
        tmva_output.Close()

        weight_xml = outdir / dataset / "weights" / f"TMVA_{b.label}_BDT.weights.xml"
        if not weight_xml.exists():
            raise RuntimeError(f"missing TMVA weight file {weight_xml}")
        reader, eval_row = make_reader(weight_xml)
        scores_sig_train = load_scores(reader, sig_train)
        scores_sig_test = load_scores(reader, sig_test)
        scores_bkg_train = load_scores(reader, bkg_train)
        scores_bkg_test = load_scores(reader, bkg_test)
        scores_data = load_scores(reader, data_tree)
        threshold_punzi = scan_threshold(scores_sig_test, scores_bkg_test)
        threshold = scan_data_split_threshold(scores_sig_test, scores_bkg_test, scores_data)
        draw_threshold_scan(plots_dir, b.label, threshold["scan_rows"], threshold["threshold"])

        h_sig_train = hist_from_scores(f"{b.label}_sig_train", scores_sig_train)
        h_sig_test = hist_from_scores(f"{b.label}_sig_test", scores_sig_test)
        h_bkg_train = hist_from_scores(f"{b.label}_bkg_train", scores_bkg_train)
        h_bkg_test = hist_from_scores(f"{b.label}_bkg_test", scores_bkg_test)
        ks_sig = h_sig_train.KolmogorovTest(h_sig_test) if h_sig_train.GetEntries() and h_sig_test.GetEntries() else 0.0
        ks_bkg = h_bkg_train.KolmogorovTest(h_bkg_test) if h_bkg_train.GetEntries() and h_bkg_test.GetEntries() else 0.0

        canvas = ROOT.TCanvas(f"c_over_{b.label}", "", 900, 700)
        canvas.SetLeftMargin(0.12)
        h_sig_train.SetLineColor(ROOT.kBlue + 1)
        h_sig_test.SetLineColor(ROOT.kBlue + 1)
        h_sig_test.SetLineStyle(2)
        h_bkg_train.SetLineColor(ROOT.kRed + 1)
        h_bkg_test.SetLineColor(ROOT.kRed + 1)
        h_bkg_test.SetLineStyle(2)
        for hist in (h_sig_train, h_sig_test, h_bkg_train, h_bkg_test):
            hist.SetLineWidth(2)
            if hist.Integral() > 0:
                hist.Scale(1.0 / hist.Integral())
        ymax = max(h.GetMaximum() for h in (h_sig_train, h_sig_test, h_bkg_train, h_bkg_test)) * 1.25
        h_sig_train.SetTitle(f"{b.label};BDT score;normalized candidates")
        h_sig_train.SetMaximum(ymax)
        h_sig_train.Draw("hist")
        h_sig_test.Draw("hist same")
        h_bkg_train.Draw("hist same")
        h_bkg_test.Draw("hist same")
        line = ROOT.TLine(threshold["threshold"], 0.0, threshold["threshold"], ymax * 0.85)
        line.SetLineStyle(2)
        line.Draw()
        legend = ROOT.TLegend(0.54, 0.70, 0.88, 0.89)
        legend.SetBorderSize(0)
        legend.SetFillStyle(0)
        legend.AddEntry(h_sig_train, "signal train", "l")
        legend.AddEntry(h_sig_test, "signal test", "l")
        legend.AddEntry(h_bkg_train, "background train", "l")
        legend.AddEntry(h_bkg_test, "background test", "l")
        legend.Draw()
        canvas.SaveAs(str(plots_dir / f"{b.label}_bdt_overtraining.png"))
        canvas.SaveAs(str(plots_dir / f"{b.label}_bdt_overtraining.pdf"))

        data_rows_base = [row for _, row in scores_data]
        data_rows_bdt = rows_above_threshold(scores_data, threshold["threshold"])
        data_rows_an = [row for row in data_rows_base if an_pass(row, bin_index)]
        data_scores_opt, data_scores_val = split_scores_by_index(scores_data)
        data_rows_bdt_opt = rows_above_threshold(data_scores_opt, threshold["threshold"])
        data_rows_bdt_val = rows_above_threshold(data_scores_val, threshold["threshold"])

        draw_mass_plot(plots_dir, b.label, data_rows_bdt, data_rows_an, data_rows_base)

        y_base = count_yield(data_rows_base)
        y_bdt = count_yield(data_rows_bdt)
        y_bdt_opt = count_yield(data_rows_bdt_opt)
        y_bdt_val = count_yield(data_rows_bdt_val)
        y_an = count_yield(data_rows_an)
        for selection, yrow in (
            ("baseline", y_base),
            ("bdt", y_bdt),
            ("bdt_optimization_half", y_bdt_opt),
            ("bdt_validation_half", y_bdt_val),
            ("an_rectangular", y_an),
        ):
            rows_yield.append(
                {
                    "bin": b.label,
                    "selection": selection,
                    **{key: f"{value:.6g}" for key, value in yrow.items()},
                }
            )

        rows_overtraining.append(
            {
                "bin": b.label,
                "ks_signal": f"{ks_sig:.6g}",
                "ks_background": f"{ks_bkg:.6g}",
                **{key: str(value) for key, value in counts.items()},
            }
        )
        rows_summary.append(
            {
                "bin": b.label,
                "status": "trained",
                **counts,
                "bdt_threshold": f"{threshold['threshold']:.6g}",
                "threshold_source": "data_split_yield_significance",
                "punzi_threshold": f"{threshold_punzi['threshold']:.6g}",
                "punzi": f"{threshold_punzi['punzi']:.6g}",
                "punzi_signal_eff": f"{threshold_punzi['signal_eff']:.6g}",
                "punzi_bkg_rejection": f"{threshold_punzi['bkg_rejection']:.6g}",
                "test_signal_eff": f"{threshold['signal_eff']:.6g}",
                "test_bkg_rejection": f"{threshold['bkg_rejection']:.6g}",
                "opt_diagnostic_yield": f"{threshold['opt_yield']:.6g}",
                "opt_diagnostic_significance": f"{threshold['opt_significance']:.6g}",
                "val_diagnostic_yield": f"{threshold['val_yield']:.6g}",
                "val_diagnostic_significance": f"{threshold['val_significance']:.6g}",
                "ks_signal": f"{ks_sig:.6g}",
                "ks_background": f"{ks_bkg:.6g}",
                "data_bdt_diagnostic_yield": f"{y_bdt['diagnostic_yield']:.6g}",
                "data_bdt_diagnostic_significance": f"{y_bdt['diagnostic_significance']:.6g}",
                "data_an_diagnostic_yield": f"{y_an['diagnostic_yield']:.6g}",
                "data_an_diagnostic_significance": f"{y_an['diagnostic_significance']:.6g}",
            }
        )
        print(
            f"BDT bin {b.label} threshold={threshold['threshold']:.3f} "
            f"valZ={threshold['val_significance']:.2f} y_bdt={y_bdt['diagnostic_yield']:.2f}",
            flush=True,
        )

    os.chdir(workdir)
    flat_file.Close()
    return {
        "summary_rows": rows_summary,
        "overtraining_rows": rows_overtraining,
        "yield_rows": rows_yield,
    }


def write_tsv(path: Path, rows: list[dict]) -> None:
    if not rows:
        path.write_text("", encoding="utf-8")
        return
    keys = list(rows[0].keys())
    for row in rows:
        for key in row:
            if key not in keys:
                keys.append(key)
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, delimiter="\t", lineterminator="\n", fieldnames=keys)
        writer.writeheader()
        writer.writerows(rows)


def write_summary(outdir: Path, metadata: dict, result: dict) -> None:
    summary_rows = result["summary_rows"]
    yield_rows = result["yield_rows"]
    lines = [
        "# D0 BDT Topology Optimization",
        "",
        "<!-- DOC_OWNER: cms-analysis-manager BDT cross-check output. -->",
        "<!-- TOKEN_NOTE: thresholds are trained on MC signal and data sidebands; data peak is application only. -->",
        "",
        "## Protocol",
        "",
        "- Train one ROOT/TMVA BDT per `(pT, |y|)` analysis bin.",
        "- Inputs: `Dalpha`, `DsvpvSig`, `Dchi2cl`, and `Ddtheta`.",
        "- Signal: prompt truth-matched official D0 MC candidates.",
        "- Background: 2023 data candidates in the independently defined mass sidebands.",
        "- Peak-blind cross-check: held-out MC signal plus held-out data sidebands, maximizing a Punzi-like figure of merit.",
        "- Yield threshold: one half of the data chooses the threshold with a sideband-subtracted peak-significance diagnostic.",
        "- Validation: the frozen threshold is checked on the other half of the data and then applied to the full sample.",
        "",
        "## Inputs",
        "",
        f"- MC file: `{metadata['mc_path']}`",
        f"- Data files processed: `{metadata['data_files']}`",
        f"- Data events passing event cuts: `{metadata.get('data_events_pass', 'reused flat')}` / `{metadata.get('data_events_seen', 'reused flat')}`",
        f"- Flat ntuple: `{metadata['flat_path']}`",
        "",
        "## BDT Thresholds",
        "",
        "| bin | chosen threshold | Punzi threshold | MC signal eff. | sideband rejection | opt Z | val Z | BDT yield | AN yield |",
        "|---|---:|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for row in summary_rows:
        if row.get("status") != "trained":
            lines.append(f"| {row['bin']} | insufficient stats | | | | | | |")
            continue
        lines.append(
            f"| {row['bin']} | {float(row['bdt_threshold']):.3f} | "
            f"{float(row['punzi_threshold']):.3f} | "
            f"{float(row['test_signal_eff']):.3f} | {float(row['test_bkg_rejection']):.3f} | "
            f"{float(row['opt_diagnostic_significance']):.2f} | {float(row['val_diagnostic_significance']):.2f} | "
            f"{float(row['data_bdt_diagnostic_yield']):.1f} | {float(row['data_an_diagnostic_yield']):.1f} |"
        )
    lines.extend(
        [
            "",
            "## Notes",
            "",
            "- The diagnostic yield is a simple signal-window minus scaled-sideband count, not the final AN mass-fit yield.",
            "- The Punzi scan is fully peak-blind. In this sample it can choose a looser threshold than a yield-significance scan.",
            "- The data-split scan is not a final yield measurement; it is a topology-cut optimization stress test with a held-out validation half.",
            "- The BDT is an optimization cross-check. It should not replace the nominal rectangular selection until the mass-fit model, sideband stability, and bin-wise efficiency systematics are reviewed.",
            "- Overtraining and mass-shape diagnostics are stored in the TSV files and plots directory.",
            "",
            "## Outputs",
            "",
            "- `bdt_thresholds.tsv`",
            "- `bdt_overtraining.tsv`",
            "- `bdt_data_yields.tsv`",
            "- `plots/*_bdt_overtraining.png`",
            "- `plots/*_bdt_threshold_scan.png`",
            "- `plots/*_mass_bdt_vs_an.png`",
        ]
    )
    (outdir / "README.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    write_tsv(outdir / "bdt_thresholds.tsv", summary_rows)
    write_tsv(outdir / "bdt_overtraining.tsv", result["overtraining_rows"])
    write_tsv(outdir / "bdt_data_yields.tsv", yield_rows)
    (outdir / "metadata.json").write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def resolve_data_files(args: argparse.Namespace) -> list[str]:
    files: list[str] = []
    if args.data_list:
        with open(args.data_list, encoding="utf-8") as handle:
            files.extend(line.strip() for line in handle if line.strip() and not line.startswith("#"))
    for pattern in args.data_glob:
        files.extend(glob.glob(pattern))
    return sorted(dict.fromkeys(files))


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--mc", required=True, help="Official flat prompt D0 MC ROOT file.")
    p.add_argument("--data-list", help="Text file containing recreated forest ROOT files.")
    p.add_argument("--data-glob", action="append", default=[], help="Glob for recreated forest ROOT files.")
    p.add_argument("--outdir", required=True, help="Evan-owned CERN output directory.")
    p.add_argument("--max-mc-events", type=int, default=8_000_000)
    p.add_argument("--max-signal-per-bin", type=int, default=5000)
    p.add_argument("--max-data-files", type=int, default=-1)
    p.add_argument("--reuse-flat", action="store_true")
    return p


def main() -> int:
    args = parser().parse_args()
    outdir = Path(args.outdir)
    if not owned_output(outdir):
        raise SystemExit(f"Refusing to write outside Evan-owned CERN paths: {outdir}")
    outdir.mkdir(parents=True, exist_ok=True)
    (outdir / "plots").mkdir(exist_ok=True)

    data_files = resolve_data_files(args)
    if not data_files:
        raise SystemExit("No data files matched.")
    data_files = data_files[: args.max_data_files if args.max_data_files > 0 else None]
    flat_path = outdir / "bdt_training_flat.root"
    metadata_path = outdir / "metadata.json"
    previous_metadata = {}
    if args.reuse_flat and metadata_path.exists():
        try:
            previous_metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
        except Exception:
            previous_metadata = {}

    metadata = {
        "mc_path": args.mc,
        "data_files": len(data_files),
        "data_file_list": data_files,
        "flat_path": str(flat_path),
        "max_mc_events": args.max_mc_events,
        "max_signal_per_bin": args.max_signal_per_bin,
        "sideband": f"{SIDEBAND_INNER} < |Dmass-{D0_MASS}| < {SIDEBAND_OUTER}",
        "signal_half_width": SIGNAL_HALF_WIDTH,
    }
    for key in ("data_events_seen", "data_events_pass"):
        if key in previous_metadata:
            metadata[key] = previous_metadata[key]

    if not args.reuse_flat or not flat_path.exists():
        writer = FlatWriter(flat_path)
        try:
            metadata.update(collect_mc_signal(writer, args.mc, args.max_mc_events, args.max_signal_per_bin))
            metadata.update(collect_data_candidates(writer, data_files, args.max_data_files))
        finally:
            writer.close()
    else:
        flat_file = ROOT.TFile.Open(str(flat_path))
        metadata["reused_flat"] = True
        metadata["signal_counts"] = [
            tree_entries(flat_file, f"sig_train_bin{i}") + tree_entries(flat_file, f"sig_test_bin{i}")
            for i in range(len(BINS))
        ]
        metadata["data_sideband_counts"] = [
            tree_entries(flat_file, f"bkg_train_bin{i}") + tree_entries(flat_file, f"bkg_test_bin{i}")
            for i in range(len(BINS))
        ]
        metadata["data_candidate_counts"] = [tree_entries(flat_file, f"data_bin{i}") for i in range(len(BINS))]
        flat_file.Close()

    result = train_and_apply(flat_path, outdir)
    write_summary(outdir, metadata, result)
    print(f"Wrote {outdir / 'README.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
