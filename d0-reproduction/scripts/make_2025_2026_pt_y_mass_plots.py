#!/usr/bin/env python3
"""Make 2025/2026 D0 mass plots binned in pT and signed rapidity.

Runs ROOT on LXPLUS, copies plots back locally, rewrites the reusable
general-codex-output Overleaf/GitHub project, and optionally syncs it.
"""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
WORKSPACE = ROOT / "research" / "cms-d0-analysis"
LOCAL_PLOTS = WORKSPACE / "plots" / "2025_2026_pt_y_mass"
GENERAL_OUTPUT = ROOT / "repos" / "general-codex-output"
FIGURE_DIR_NAME = "d0_mass_pt_y_2025_2026"

DATA_ROOT = "/afs/cern.ch/user/e/evzhang/RootAnalysis/MITanalysisTest"
INPUT_2025 = f"{DATA_ROOT}/2025Data/outputs/section7_d_selected.root"
INPUT_2026 = f"{DATA_ROOT}/510&511&523&527&529&543&549/outputs/section7_d_selected.root"
RUN_LABEL_2026 = "510, 511, 523, 527, 529, 543, 549"

SAMPLES = [
    {
        "id": "2025",
        "title": "2025 selected data",
        "input": INPUT_2025,
    },
    {
        "id": "2026",
        "title": "2026 seven-run selected data",
        "input": INPUT_2026,
    },
]


ROOT_MACRO = r'''
#include <TCanvas.h>
#include <TClass.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TKey.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TLine.h>
#include <TObjArray.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>
#include <TH1D.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
const double kD0Mass = 1.86484;
const double kMassMin = 1.75;
const double kMassMax = 1.98;
const int kMassBins = 92;

struct FoundTree {
  TTree* tree = nullptr;
  std::string mass;
  std::string pt;
  std::string y;
};

std::string clean_token(const std::string& text) {
  std::string out;
  for (char c : text) {
    if (std::isalnum(static_cast<unsigned char>(c))) out.push_back(c);
    else if (c == '_' || c == '-') out.push_back(c);
  }
  return out.empty() ? "sample" : out;
}

std::string sanitize(double value) {
  std::ostringstream out;
  if (value < 0) out << "neg" << static_cast<int>(-value);
  else out << static_cast<int>(value);
  return out.str();
}

bool has_branch(TTree* tree, const std::string& name) {
  return tree && (tree->GetBranch(name.c_str()) || tree->GetLeaf(name.c_str()));
}

std::string first_branch(TTree* tree, const std::vector<std::string>& names) {
  for (const auto& name : names) {
    if (has_branch(tree, name)) return name;
  }
  return "";
}

TTree* first_tree_recursive(TDirectory* dir, const std::vector<std::string>& preferred) {
  if (!dir) return nullptr;
  for (const auto& name : preferred) {
    TObject* obj = dir->Get(name.c_str());
    if (obj && obj->InheritsFrom(TTree::Class())) return static_cast<TTree*>(obj);
  }
  TIter next(dir->GetListOfKeys());
  TKey* key = nullptr;
  while ((key = static_cast<TKey*>(next()))) {
    TClass* cls = gROOT->GetClass(key->GetClassName());
    if (!cls) continue;
    if (cls->InheritsFrom(TTree::Class())) {
      return static_cast<TTree*>(key->ReadObj());
    }
    if (cls->InheritsFrom(TDirectory::Class())) {
      TObject* obj = key->ReadObj();
      TTree* found = first_tree_recursive(static_cast<TDirectory*>(obj), preferred);
      if (found) return found;
    }
  }
  return nullptr;
}

void write_discovery(TTree* tree, const std::string& output_dir, const std::string& message) {
  std::ofstream out(output_dir + "/discovery.txt");
  out << message << "\n";
  if (!tree) return;
  out << "tree=" << tree->GetName() << "\n";
  out << "branches:\n";
  TObjArray* branches = tree->GetListOfBranches();
  for (int i = 0; branches && i < branches->GetEntries(); ++i) {
    TObject* branch = branches->At(i);
    if (branch) out << "  " << branch->GetName() << "\n";
  }
}

FoundTree discover_tree(TFile& file, const std::string& output_dir) {
  FoundTree found;
  std::vector<std::string> preferred_trees = {"SelectedD", "Dfinder/ntDkpi", "Candidates", "Events"};
  found.tree = first_tree_recursive(&file, preferred_trees);
  if (!found.tree) {
    write_discovery(nullptr, output_dir, "No TTree found in input file.");
    return found;
  }
  found.mass = first_branch(found.tree, {"Dmass", "D_mass", "D0mass", "D0Mass", "mass", "m"});
  found.pt = first_branch(found.tree, {"Dpt", "D_pt", "D0pt", "D0Pt", "pt"});
  found.y = first_branch(found.tree, {"Dy", "D_y", "D0y", "D0Y", "y", "rapidity"});
  if (found.mass.empty() || found.pt.empty() || found.y.empty()) {
    write_discovery(found.tree, output_dir, "Missing required mass, pT, or rapidity branch.");
  }
  return found;
}

int bin_integral(TH1D* hist, double width) {
  const int lo = hist->GetXaxis()->FindFixBin(kD0Mass - width);
  const int hi = hist->GetXaxis()->FindFixBin(kD0Mass + width);
  return static_cast<int>(hist->Integral(lo, hi));
}

void style_mass_hist(TH1D* hist, int color) {
  hist->SetLineColor(color);
  hist->SetFillColorAlpha(color, 0.16);
  hist->SetLineWidth(2);
  hist->GetXaxis()->SetTitle("M_{K#pi} [GeV]");
  hist->GetYaxis()->SetTitle("Selected candidates");
  hist->GetXaxis()->SetTitleSize(0.042);
  hist->GetYaxis()->SetTitleSize(0.042);
  hist->GetXaxis()->SetLabelSize(0.036);
  hist->GetYaxis()->SetLabelSize(0.036);
}

void draw_single_hist(TH1D* hist, double pt_lo, double pt_hi, double y_lo, double y_hi,
                      const std::string& sample_title, const std::string& out_name) {
  TCanvas canvas("canvas", "canvas", 850, 650);
  canvas.SetLeftMargin(0.12);
  canvas.SetRightMargin(0.04);
  canvas.SetTopMargin(0.08);
  canvas.SetBottomMargin(0.12);
  style_mass_hist(hist, kAzure + 2);
  hist->Draw("hist");

  TLine line(kD0Mass, 0, kD0Mass, hist->GetMaximum() * 1.08);
  line.SetLineColor(kRed + 1);
  line.SetLineStyle(2);
  line.SetLineWidth(2);
  line.Draw("same");

  TLatex label;
  label.SetNDC();
  label.SetTextSize(0.035);
  label.DrawLatex(0.16, 0.88, sample_title.c_str());
  label.DrawLatex(0.16, 0.83, Form("%.0f < p_{T} #leq %.0f GeV, %.0f #leq y %s %.0f",
                                   pt_lo, pt_hi, y_lo, (y_hi == 2.0 ? "#leq" : "<"), y_hi));
  label.DrawLatex(0.16, 0.78, Form("Entries: %.0f", hist->GetEntries()));
  canvas.SaveAs((out_name + ".png").c_str());
  canvas.SaveAs((out_name + ".pdf").c_str());
}

void draw_full_hist(TH1D* hist, const std::string& sample_title, const std::string& out_name) {
  TCanvas canvas("full", "full", 850, 650);
  canvas.SetLeftMargin(0.12);
  canvas.SetRightMargin(0.04);
  canvas.SetTopMargin(0.08);
  canvas.SetBottomMargin(0.12);
  style_mass_hist(hist, kAzure + 2);
  hist->Draw("hist");

  TLine line(kD0Mass, 0, kD0Mass, hist->GetMaximum() * 1.08);
  line.SetLineColor(kRed + 1);
  line.SetLineStyle(2);
  line.SetLineWidth(2);
  line.Draw("same");

  TLatex label;
  label.SetNDC();
  label.SetTextSize(0.036);
  label.DrawLatex(0.16, 0.88, sample_title.c_str());
  label.DrawLatex(0.16, 0.83, Form("All selected candidates, N=%.0f", hist->GetEntries()));
  canvas.SaveAs((out_name + ".png").c_str());
  canvas.SaveAs((out_name + ".pdf").c_str());
}

TH1D* make_full_hist(TTree* tree, const std::string& mass, const std::string& name) {
  TH1D* hist = new TH1D(name.c_str(), "", kMassBins, kMassMin, kMassMax);
  hist->Sumw2();
  tree->Draw((mass + ">>" + name).c_str(), "", "goff");
  return hist;
}
}

int make_pt_y_mass_plots(const char* input_file, const char* output_root, const char* sample, const char* sample_title) {
  gStyle->SetOptStat(0);
  const std::string sample_id = clean_token(sample);
  const std::string output_dir = std::string(output_root) + "/" + sample_id;
  gSystem->mkdir(output_dir.c_str(), true);

  TFile file(input_file, "READ");
  if (file.IsZombie()) {
    std::ofstream out(output_dir + "/discovery.txt");
    out << "Could not open input file: " << input_file << "\n";
    return 1;
  }

  FoundTree found = discover_tree(file, output_dir);
  if (!found.tree) return 2;
  if (found.mass.empty() || found.pt.empty() || found.y.empty()) return 3;

  std::ofstream meta(output_dir + "/metadata.txt");
  meta << "sample=" << sample_id << "\n";
  meta << "sample_title=" << sample_title << "\n";
  meta << "input_file=" << input_file << "\n";
  meta << "tree=" << found.tree->GetName() << "\n";
  meta << "mass_branch=" << found.mass << "\n";
  meta << "pt_branch=" << found.pt << "\n";
  meta << "y_branch=" << found.y << "\n";
  meta << "mass_range=" << kMassMin << "," << kMassMax << "\n";
  meta << "mass_bins=" << kMassBins << "\n";

  TH1D* full = make_full_hist(found.tree, found.mass, "full_mass_" + sample_id);
  draw_full_hist(full, sample_title, output_dir + "/d0_mass_" + sample_id + "_all_selected");

  std::ofstream csv(output_dir + "/bin_counts.csv");
  csv << "sample,pt_low,pt_high,y_low,y_high,entries,peak_bin_center,peak_bin_content,"
      << "window_pm20mev,window_pm30mev,window_pm50mev,plot\n";

  const double pt_bins[4] = {2, 5, 8, 12};
  const double y_bins[5] = {-2, -1, 0, 1, 2};
  std::vector<TH1D*> hists;

  for (int iy = 0; iy < 4; ++iy) {
    for (int ipt = 0; ipt < 3; ++ipt) {
      const double pt_lo = pt_bins[ipt];
      const double pt_hi = pt_bins[ipt + 1];
      const double y_lo = y_bins[iy];
      const double y_hi = y_bins[iy + 1];
      const std::string name =
        "d0_mass_" + sample_id + "_pt" + sanitize(pt_lo) + "to" + sanitize(pt_hi) +
        "_y" + sanitize(y_lo) + "to" + sanitize(y_hi);

      TH1D* hist = new TH1D(name.c_str(), "", kMassBins, kMassMin, kMassMax);
      hist->Sumw2();
      std::ostringstream cut;
      cut << "(" << found.pt << ">" << pt_lo << "&&" << found.pt << "<=" << pt_hi
          << "&&" << found.y << ">=" << y_lo << "&&" << found.y << (y_hi == 2.0 ? "<=" : "<") << y_hi << ")";
      found.tree->Draw((found.mass + ">>" + name).c_str(), cut.str().c_str(), "goff");

      const std::string out_base = output_dir + "/" + name;
      draw_single_hist(hist, pt_lo, pt_hi, y_lo, y_hi, sample_title, out_base);

      const int peak_bin = hist->GetMaximumBin();
      csv << sample_id << "," << pt_lo << "," << pt_hi << "," << y_lo << "," << y_hi << ","
          << static_cast<long long>(hist->GetEntries()) << ","
          << std::fixed << std::setprecision(5) << hist->GetBinCenter(peak_bin) << ","
          << std::setprecision(0) << hist->GetBinContent(peak_bin) << ","
          << bin_integral(hist, 0.02) << ","
          << bin_integral(hist, 0.03) << ","
          << bin_integral(hist, 0.05) << ","
          << name << ".png\n";
      hists.push_back(hist);
    }
  }

  TCanvas grid("grid", "grid", 1800, 1900);
  grid.Divide(3, 4, 0.001, 0.001);
  for (int i = 0; i < static_cast<int>(hists.size()); ++i) {
    grid.cd(i + 1);
    gPad->SetLeftMargin(0.13);
    gPad->SetRightMargin(0.04);
    gPad->SetTopMargin(0.11);
    gPad->SetBottomMargin(0.12);
    style_mass_hist(hists[i], kAzure + 2);
    hists[i]->GetYaxis()->SetTitle("Candidates");
    hists[i]->GetXaxis()->SetLabelSize(0.045);
    hists[i]->GetYaxis()->SetLabelSize(0.045);
    hists[i]->GetXaxis()->SetTitleSize(0.050);
    hists[i]->GetYaxis()->SetTitleSize(0.050);
    hists[i]->Draw("hist");
    TLine* line = new TLine(kD0Mass, 0, kD0Mass, hists[i]->GetMaximum() * 1.08);
    line->SetLineColor(kRed + 1);
    line->SetLineStyle(2);
    line->SetLineWidth(2);
    line->Draw("same");
    const int iy = i / 3;
    const int ipt = i % 3;
    TLatex label;
    label.SetNDC();
    label.SetTextSize(0.050);
    label.DrawLatex(0.17, 0.86, Form("%.0f<p_{T}#leq%.0f, %.0f#leqy%s%.0f",
                                      pt_bins[ipt], pt_bins[ipt + 1],
                                      y_bins[iy], (y_bins[iy + 1] == 2.0 ? "#leq" : "<"),
                                      y_bins[iy + 1]));
    label.SetTextSize(0.045);
    label.DrawLatex(0.17, 0.79, Form("N=%.0f", hists[i]->GetEntries()));
  }
  grid.SaveAs((output_dir + "/d0_mass_" + sample_id + "_pt_y_grid.png").c_str());
  grid.SaveAs((output_dir + "/d0_mass_" + sample_id + "_pt_y_grid.pdf").c_str());
  return 0;
}

int make_full_mass_overlay(const char* input_a, const char* sample_a,
                           const char* input_b, const char* sample_b,
                           const char* output_root) {
  gStyle->SetOptStat(0);
  const std::string output_dir = std::string(output_root) + "/sanity";
  gSystem->mkdir(output_dir.c_str(), true);

  TFile file_a(input_a, "READ");
  TFile file_b(input_b, "READ");
  if (file_a.IsZombie() || file_b.IsZombie()) {
    std::ofstream out(output_dir + "/overlay_discovery.txt");
    out << "Could not open one of the input files.\n";
    return 1;
  }

  FoundTree a = discover_tree(file_a, output_dir);
  FoundTree b = discover_tree(file_b, output_dir);
  if (!a.tree || !b.tree || a.mass.empty() || b.mass.empty()) return 2;

  TH1D* hist_a = make_full_hist(a.tree, a.mass, "overlay_full_" + clean_token(sample_a));
  TH1D* hist_b = make_full_hist(b.tree, b.mass, "overlay_full_" + clean_token(sample_b));

  std::ofstream csv(output_dir + "/all_mass_overlay_counts.csv");
  csv << "sample,entries,integral_raw\n";
  csv << clean_token(sample_a) << "," << static_cast<long long>(hist_a->GetEntries()) << "," << hist_a->Integral() << "\n";
  csv << clean_token(sample_b) << "," << static_cast<long long>(hist_b->GetEntries()) << "," << hist_b->Integral() << "\n";

  if (hist_a->Integral() > 0) hist_a->Scale(1.0 / hist_a->Integral());
  if (hist_b->Integral() > 0) hist_b->Scale(1.0 / hist_b->Integral());
  hist_a->SetLineColor(kAzure + 2);
  hist_b->SetLineColor(kOrange + 7);
  hist_a->SetLineWidth(3);
  hist_b->SetLineWidth(3);
  hist_a->SetFillStyle(0);
  hist_b->SetFillStyle(0);
  hist_a->GetXaxis()->SetTitle("M_{K#pi} [GeV]");
  hist_a->GetYaxis()->SetTitle("Area-normalized selected candidates");
  hist_a->GetXaxis()->SetTitleSize(0.042);
  hist_a->GetYaxis()->SetTitleSize(0.042);
  hist_a->GetXaxis()->SetLabelSize(0.036);
  hist_a->GetYaxis()->SetLabelSize(0.036);
  hist_a->SetMaximum(1.18 * std::max(hist_a->GetMaximum(), hist_b->GetMaximum()));

  TCanvas canvas("overlay", "overlay", 900, 660);
  canvas.SetLeftMargin(0.12);
  canvas.SetRightMargin(0.04);
  canvas.SetTopMargin(0.08);
  canvas.SetBottomMargin(0.12);
  hist_a->Draw("hist");
  hist_b->Draw("hist same");

  TLine line(kD0Mass, 0, kD0Mass, hist_a->GetMaximum() * 1.02);
  line.SetLineColor(kRed + 1);
  line.SetLineStyle(2);
  line.SetLineWidth(2);
  line.Draw("same");

  TLegend legend(0.58, 0.74, 0.91, 0.89);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.AddEntry(hist_a, Form("%s, N=%.0f", sample_a, hist_a->GetEntries()), "l");
  legend.AddEntry(hist_b, Form("%s, N=%.0f", sample_b, hist_b->GetEntries()), "l");
  legend.AddEntry(&line, "Nominal D^{0}", "l");
  legend.Draw();

  TLatex label;
  label.SetNDC();
  label.SetTextSize(0.036);
  label.DrawLatex(0.16, 0.88, "All selected candidates; area-normalized sanity overlay");

  canvas.SaveAs((output_dir + "/d0_mass_2025_2026_all_selected_overlay.png").c_str());
  canvas.SaveAs((output_dir + "/d0_mass_2025_2026_all_selected_overlay.pdf").c_str());
  return 0;
}
'''


def run(cmd: list[str], *, check: bool = True, cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    proc = subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if check and proc.returncode != 0:
        raise RuntimeError(f"command failed ({proc.returncode}): {' '.join(cmd)}\n{proc.stdout}")
    return proc


def rel(path: Path) -> str:
    try:
        return path.resolve().relative_to(ROOT.resolve()).as_posix()
    except ValueError:
        return str(path)


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def format_bin_label(row: dict[str, str]) -> str:
    upper = r"\le" if row["y_high"] in {"2", "2.0"} else "<"
    return rf"${row['pt_low']}<p_T\le {row['pt_high']}$, ${row['y_low']}\le y{upper}{row['y_high']}$"


def parse_counts(csv_path: Path) -> list[dict[str, str]]:
    with csv_path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def table_for_sample(sample_dir: Path) -> str:
    rows = parse_counts(sample_dir / "bin_counts.csv")
    body = []
    for row in rows:
        label = format_bin_label(row)
        body.append(
            f"{label} & {row['entries']} & {row['peak_bin_center']} & "
            f"{row['window_pm30mev']} & {row['window_pm50mev']} \\\\"
        )
    return "\n".join(body)


def read_overlay_counts(plot_dir: Path) -> list[dict[str, str]]:
    path = plot_dir / "sanity" / "all_mass_overlay_counts.csv"
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def latex_report(fig_root: str, plot_dir: Path) -> str:
    date = dt.datetime.now(dt.UTC).strftime("%Y-%m-%d")
    overlay_rows = read_overlay_counts(plot_dir)
    overlay_table = "\n".join(
        f"{row['sample']} & {row['entries']} & {float(row['integral_raw']):.0f} \\\\"
        for row in overlay_rows
    )

    source_lines = "\n".join(f"{sample['id']}: {sample['input']}" for sample in SAMPLES)

    return rf"""\documentclass[10pt]{{article}}
\usepackage[margin=0.55in]{{geometry}}
\usepackage[T1]{{fontenc}}
\usepackage{{lmodern}}
\usepackage{{microtype}}
\usepackage{{booktabs}}
\usepackage{{tabularx}}
\usepackage{{graphicx}}
\usepackage{{float}}
\usepackage{{hyperref}}
\hypersetup{{colorlinks=true,urlcolor=blue,linkcolor=blue}}
\setlength{{\parindent}}{{0pt}}
\setlength{{\parskip}}{{0.25em}}
\sloppy

\title{{2025 and 2026 $D^0$ Mass Peak by $p_T$ and Signed Rapidity}}
\author{{Evan Zhang}}
\date{{Prepared {date}}}

\begin{{document}}
\maketitle
\vspace{{-1.2em}}

\section*{{2025 Grid}}

\begin{{figure}}[H]
\centering
\includegraphics[width=\textwidth]{{{fig_root}/2025/d0_mass_2025_pt_y_grid.png}}
\caption{{2025 selected $D^0$ mass spectra in the requested $p_T$ and signed-$y$ bins. The dashed red line is the nominal $D^0$ mass, 1.86484 GeV.}}
\end{{figure}}

\section*{{2026 Grid}}

\begin{{figure}}[H]
\centering
\includegraphics[width=\textwidth]{{{fig_root}/2026/d0_mass_2026_pt_y_grid.png}}
\caption{{2026 seven-run selected $D^0$ mass spectra in the requested $p_T$ and signed-$y$ bins. Runs: {RUN_LABEL_2026}.}}
\end{{figure}}

\section*{{Bin Counts}}

\begin{{table}}[H]
\centering
\small
\begin{{tabularx}}{{\textwidth}}{{@{{}}Xrrrr@{{}}}}
\toprule
\multicolumn{{5}}{{c}}{{2025}} \\
\midrule
Bin & Entries & Peak center & $\pm30$ MeV & $\pm50$ MeV \\
\midrule
{table_for_sample(plot_dir / "2025")}
\bottomrule
\end{{tabularx}}
\end{{table}}

\begin{{table}}[H]
\centering
\small
\begin{{tabularx}}{{\textwidth}}{{@{{}}Xrrrr@{{}}}}
\toprule
\multicolumn{{5}}{{c}}{{2026}} \\
\midrule
Bin & Entries & Peak center & $\pm30$ MeV & $\pm50$ MeV \\
\midrule
{table_for_sample(plot_dir / "2026")}
\bottomrule
\end{{tabularx}}
\end{{table}}

\section*{{Full-Mass Sanity Check}}

\begin{{figure}}[H]
\centering
\includegraphics[width=0.92\textwidth]{{{fig_root}/sanity/d0_mass_2025_2026_all_selected_overlay.png}}
\caption{{All selected candidates in the full mass window, area-normalized and overlaid for a quick shape sanity check.}}
\end{{figure}}

\begin{{table}}[H]
\centering
\small
\begin{{tabular}}{{@{{}}lrr@{{}}}}
\toprule
Sample & Entries & Raw integral \\
\midrule
{overlay_table}
\bottomrule
\end{{tabular}}
\end{{table}}

\section*{{Sources}}

\begin{{verbatim}}
{source_lines}
\end{{verbatim}}

Metadata files record the chosen tree and branch names:

\begin{{verbatim}}
{fig_root}/2025
{fig_root}/2026
\end{{verbatim}}

\end{{document}}
"""


def readme_text() -> str:
    return f"""# 2025 and 2026 D0 pT-y Mass Plots

Reusable Codex output buffer for selected D0 mass spectra binned in pT and
signed rapidity.

Samples:

- 2025: `{INPUT_2025}`
- 2026: `{INPUT_2026}`

pT bins:

- 2-5 GeV
- 5-8 GeV
- 8-12 GeV

signed-y bins:

- -2 to -1
- -1 to 0
- 0 to 1
- 1 to 2

Included sanity check:

- area-normalized all-selected 2025/2026 mass overlay.

Generator:

- `/home/ubuntu/agent-network/research/cms-d0-analysis/scripts/make_2025_2026_pt_y_mass_plots.py`

This repository is `general-codex-output`, the reusable Codex output buffer.
Its contents may be overwritten by future Codex tasks after the human operator
has saved anything they want to keep.
"""


def import_text() -> str:
    return """# Import Into Overleaf

Use the GitHub repository `codex-evanzhang/general-codex-output`, or pull the
registered Overleaf Git remote.

Overleaf steps:

1. Pull GitHub changes or direct Git changes into the Codex-tagged project.
2. Select `main.tex` as the main document if Overleaf does not detect it.
3. Recompile.

This repository is a reusable overwrite target for general Codex outputs.
"""


def clear_general_payload() -> None:
    for path in [GENERAL_OUTPUT / "figures", GENERAL_OUTPUT / "data"]:
        if path.is_dir():
            shutil.rmtree(path)
        elif path.exists():
            path.unlink()


def update_general_output(plot_dir: Path) -> None:
    if not (plot_dir / "2025" / "bin_counts.csv").exists():
        raise RuntimeError(f"missing 2025 bin_counts.csv in {plot_dir}")
    if not (plot_dir / "2026" / "bin_counts.csv").exists():
        raise RuntimeError(f"missing 2026 bin_counts.csv in {plot_dir}")
    clear_general_payload()
    figures = GENERAL_OUTPUT / "figures" / FIGURE_DIR_NAME
    figures.parent.mkdir(parents=True, exist_ok=True)
    if figures.exists():
        shutil.rmtree(figures)
    shutil.copytree(plot_dir, figures)
    write_text(GENERAL_OUTPUT / "README.md", readme_text())
    write_text(GENERAL_OUTPUT / "IMPORT.md", import_text())
    write_text(GENERAL_OUTPUT / "main.tex", latex_report(f"figures/{FIGURE_DIR_NAME}", figures))


def compile_latex() -> None:
    proc = run(["latexmk", "-pdf", "-interaction=nonstopmode", "main.tex"], cwd=GENERAL_OUTPUT, check=False)
    if proc.returncode != 0:
        print(proc.stdout, file=sys.stderr)
        raise RuntimeError("latexmk failed")


def sync_general_output(message: str) -> None:
    run([str(ROOT / "scripts" / "project-sync.sh"), "general-output", "-m", message], cwd=ROOT)


def remote_generate(args: argparse.Namespace) -> Path:
    timestamp = dt.datetime.now(dt.UTC).strftime("%Y%m%dT%H%M%SZ")
    remote_dir = f"/tmp/codex_d0_2025_2026_pty_{os.getuid()}_{timestamp}"
    remote_macro = f"{remote_dir}/make_2025_2026_pt_y_mass_plots.C"
    remote_driver = f"{remote_dir}/run_2025_2026_pt_y_mass_plots.C"
    remote_out = f"{remote_dir}/plots"

    with tempfile.TemporaryDirectory() as tmp:
        macro_path = Path(tmp) / "make_2025_2026_pt_y_mass_plots.C"
        driver_path = Path(tmp) / "run_2025_2026_pt_y_mass_plots.C"
        macro_path.write_text(ROOT_MACRO, encoding="utf-8")
        driver_path.write_text(
            f'''#include "{remote_macro}"

int run_2025_2026_pt_y_mass_plots() {{
  int rc = 0;
  rc = make_pt_y_mass_plots("{INPUT_2025}", "{remote_out}", "2025", "2025 selected data");
  if (rc != 0) return rc;
  rc = make_pt_y_mass_plots("{INPUT_2026}", "{remote_out}", "2026", "2026 seven-run selected data");
  if (rc != 0) return rc;
  return make_full_mass_overlay("{INPUT_2025}", "2025", "{INPUT_2026}", "2026", "{remote_out}");
}}
''',
            encoding="utf-8",
        )
        run(["ssh", "-o", "BatchMode=yes", args.ssh_host, f"mkdir -p {shlex.quote(remote_dir)}"])
        run(["scp", "-q", str(macro_path), f"{args.ssh_host}:{remote_macro}"])
        run(["scp", "-q", str(driver_path), f"{args.ssh_host}:{remote_driver}"])

        remote_cmd = "set -euo pipefail; command -v root >/dev/null; "
        remote_cmd += f"root -l -b -q {shlex.quote(remote_driver)}; "
        remote_cmd += (
            f"test -f {shlex.quote(remote_out + '/2025/bin_counts.csv')}; "
            f"test -f {shlex.quote(remote_out + '/2026/bin_counts.csv')}; "
            f"test -f {shlex.quote(remote_out + '/sanity/d0_mass_2025_2026_all_selected_overlay.png')}"
        )

        try:
            run(["ssh", "-o", "BatchMode=yes", args.ssh_host, remote_cmd])
        finally:
            shutil.rmtree(LOCAL_PLOTS, ignore_errors=True)
            LOCAL_PLOTS.mkdir(parents=True, exist_ok=True)
            scp = run(["scp", "-q", "-r", f"{args.ssh_host}:{remote_out}/.", str(LOCAL_PLOTS)], check=False)
            if scp.returncode != 0:
                print(scp.stdout, file=sys.stderr)
    return LOCAL_PLOTS


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ssh-host", default="lxplus-codex")
    parser.add_argument("--sync", action="store_true", help="Commit/push the general output project after generation")
    parser.add_argument("--skip-compile", action="store_true")
    args = parser.parse_args(argv)

    try:
        plot_dir = remote_generate(args)
        update_general_output(plot_dir)
        if not args.skip_compile:
            compile_latex()
        if args.sync:
            sync_general_output("Add 2025 and 2026 D0 pT-y mass plots")
        print(f"Generated plots in {rel(plot_dir)}")
        print(f"Updated {rel(GENERAL_OUTPUT)}")
        return 0
    except Exception as exc:
        print(str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
