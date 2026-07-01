#!/usr/bin/env python3
"""Make 2026 D0 mass plots binned in pT and signed rapidity.

This controller runs ROOT on LXPLUS, copies plots back to the local analysis
workspace, and rewrites the general Codex output buffer.
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
LOCAL_PLOTS = WORKSPACE / "plots" / "2026_pt_y_mass"
GENERAL_OUTPUT = ROOT / "repos" / "general-codex-output"
RUN_LABEL = "510, 511, 523, 527, 529, 543, 549"
REMOTE_BASE = "/afs/cern.ch/user/e/evzhang/RootAnalysis/MITanalysisTest/510&511&523&527&529&543&549"
SECTION7_FILE = f"{REMOTE_BASE}/outputs/section7_d_selected.root"


ROOT_MACRO = r'''
#include <TBranch.h>
#include <TCanvas.h>
#include <TClass.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TKey.h>
#include <TLatex.h>
#include <TLine.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>
#include <TH1D.h>

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

void write_discovery(TTree* tree, const char* output_dir, const std::string& message) {
  std::ofstream out(std::string(output_dir) + "/discovery.txt");
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

int bin_integral(TH1D* hist, double width) {
  const int lo = hist->GetXaxis()->FindFixBin(kD0Mass - width);
  const int hi = hist->GetXaxis()->FindFixBin(kD0Mass + width);
  return static_cast<int>(hist->Integral(lo, hi));
}

void draw_hist(TH1D* hist, double pt_lo, double pt_hi, double y_lo, double y_hi, const std::string& out_name) {
  TCanvas canvas("canvas", "canvas", 850, 650);
  canvas.SetLeftMargin(0.12);
  canvas.SetRightMargin(0.04);
  canvas.SetTopMargin(0.08);
  canvas.SetBottomMargin(0.12);
  hist->SetLineColor(kAzure + 2);
  hist->SetFillColorAlpha(kAzure - 9, 0.28);
  hist->SetLineWidth(2);
  hist->GetXaxis()->SetTitle("M_{K#pi} [GeV]");
  hist->GetYaxis()->SetTitle("Selected candidates");
  hist->GetXaxis()->SetTitleSize(0.042);
  hist->GetYaxis()->SetTitleSize(0.042);
  hist->GetXaxis()->SetLabelSize(0.036);
  hist->GetYaxis()->SetLabelSize(0.036);
  hist->Draw("hist");

  TLine line(kD0Mass, 0, kD0Mass, hist->GetMaximum() * 1.08);
  line.SetLineColor(kRed + 1);
  line.SetLineStyle(2);
  line.SetLineWidth(2);
  line.Draw("same");

  TLatex label;
  label.SetNDC();
  label.SetTextSize(0.035);
  label.DrawLatex(0.16, 0.88, "2026 seven-run D^{0} selection");
  label.DrawLatex(0.16, 0.83, Form("%.0f < p_{T} #leq %.0f GeV, %.0f #leq y %s %.0f",
                                   pt_lo, pt_hi, y_lo, (y_hi == 2.0 ? "#leq" : "<"), y_hi));
  label.DrawLatex(0.16, 0.78, Form("Entries: %.0f", hist->GetEntries()));
  canvas.SaveAs((out_name + ".png").c_str());
  canvas.SaveAs((out_name + ".pdf").c_str());
}
}

int make_2026_pt_y_mass_plots(const char* input_file, const char* output_dir) {
  gStyle->SetOptStat(0);
  gSystem->mkdir(output_dir, true);

  TFile file(input_file, "READ");
  if (file.IsZombie()) {
    std::ofstream out(std::string(output_dir) + "/discovery.txt");
    out << "Could not open input file: " << input_file << "\n";
    return 1;
  }

  std::vector<std::string> preferred_trees = {"SelectedD", "Dfinder/ntDkpi", "Candidates", "Events"};
  TTree* tree = first_tree_recursive(&file, preferred_trees);
  if (!tree) {
    write_discovery(nullptr, output_dir, "No TTree found in input file.");
    return 2;
  }

  std::string mass = first_branch(tree, {"Dmass", "D_mass", "D0mass", "D0Mass", "mass", "m"});
  std::string pt = first_branch(tree, {"Dpt", "D_pt", "D0pt", "D0Pt", "pt"});
  std::string y = first_branch(tree, {"Dy", "D_y", "D0y", "D0Y", "y", "rapidity"});
  if (mass.empty() || pt.empty() || y.empty()) {
    write_discovery(tree, output_dir, "Missing required mass, pT, or rapidity branch.");
    return 3;
  }

  std::ofstream meta(std::string(output_dir) + "/metadata.txt");
  meta << "input_file=" << input_file << "\n";
  meta << "tree=" << tree->GetName() << "\n";
  meta << "mass_branch=" << mass << "\n";
  meta << "pt_branch=" << pt << "\n";
  meta << "y_branch=" << y << "\n";
  meta << "mass_range=" << kMassMin << "," << kMassMax << "\n";
  meta << "mass_bins=" << kMassBins << "\n";

  std::ofstream csv(std::string(output_dir) + "/bin_counts.csv");
  csv << "pt_low,pt_high,y_low,y_high,entries,peak_bin_center,peak_bin_content,"
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
        "d0_mass_2026_pt" + sanitize(pt_lo) + "to" + sanitize(pt_hi) +
        "_y" + sanitize(y_lo) + "to" + sanitize(y_hi);

      TH1D* hist = new TH1D(name.c_str(), "", kMassBins, kMassMin, kMassMax);
      hist->Sumw2();
      std::ostringstream cut;
      cut << "(" << pt << ">" << pt_lo << "&&" << pt << "<=" << pt_hi
          << "&&" << y << ">=" << y_lo << "&&" << y << (y_hi == 2.0 ? "<=" : "<") << y_hi << ")";
      tree->Draw((mass + ">>" + name).c_str(), cut.str().c_str(), "goff");

      const std::string out_base = std::string(output_dir) + "/" + name;
      draw_hist(hist, pt_lo, pt_hi, y_lo, y_hi, out_base);

      const int peak_bin = hist->GetMaximumBin();
      csv << pt_lo << "," << pt_hi << "," << y_lo << "," << y_hi << ","
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
    hists[i]->SetLineColor(kAzure + 2);
    hists[i]->SetFillColorAlpha(kAzure - 9, 0.28);
    hists[i]->SetLineWidth(2);
    hists[i]->GetXaxis()->SetTitle("M_{K#pi} [GeV]");
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
  grid.SaveAs((std::string(output_dir) + "/d0_mass_2026_pt_y_grid.png").c_str());
  grid.SaveAs((std::string(output_dir) + "/d0_mass_2026_pt_y_grid.pdf").c_str());
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


def format_bin_label(pt_lo: str, pt_hi: str, y_lo: str, y_hi: str) -> str:
    upper = r"\le" if y_hi == "2" else "<"
    return rf"${pt_lo}<p_T\le {pt_hi}$, ${y_lo}\le y{upper}{y_hi}$"


def parse_counts(csv_path: Path) -> list[dict[str, str]]:
    with csv_path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def latex_report(plot_dir: Path) -> str:
    rows = parse_counts(plot_dir / "bin_counts.csv")
    date = dt.datetime.now(dt.UTC).strftime("%Y-%m-%d")
    table_rows = []
    figure_blocks = []
    for idx, row in enumerate(rows):
        label = format_bin_label(row["pt_low"], row["pt_high"], row["y_low"], row["y_high"])
        table_rows.append(
            f"{label} & {row['entries']} & {row['peak_bin_center']} & "
            f"{row['window_pm20mev']} & {row['window_pm30mev']} & {row['window_pm50mev']} \\\\"
        )
        if idx % 2 == 0:
            figure_blocks.append("\\begin{figure}[H]\n\\centering")
        figure_blocks.append(
            "\\begin{subfigure}{0.49\\textwidth}\n"
            f"  \\includegraphics[width=\\textwidth]{{figures/2026_pt_y_mass/{row['plot']}}}\n"
            f"  \\caption{{{label}}}\n"
            "\\end{subfigure}"
        )
        if idx % 2 == 1:
            figure_blocks.append("\\end{figure}\n")
    if len(rows) % 2:
        figure_blocks.append("\\end{figure}\n")

    return rf"""\documentclass[10pt]{{article}}

\usepackage[margin=0.55in]{{geometry}}
\usepackage[T1]{{fontenc}}
\usepackage{{lmodern}}
\usepackage{{microtype}}
\usepackage{{booktabs}}
\usepackage{{tabularx}}
\usepackage{{graphicx}}
\usepackage{{float}}
\usepackage{{subcaption}}
\usepackage{{hyperref}}
\hypersetup{{colorlinks=true,urlcolor=blue,linkcolor=blue}}
\setlength{{\parindent}}{{0pt}}
\setlength{{\parskip}}{{0.25em}}
\sloppy

\title{{2026 $D^0$ Mass Peak by $p_T$ and Signed Rapidity}}
\author{{Evan Zhang}}
\date{{Prepared {date}}}

\begin{{document}}
\maketitle
\vspace{{-1.2em}}

\section*{{Scope}}

This report uses the seven-run 2026 output directory for runs {RUN_LABEL}. The
selected $D^0$ candidates are split into three $p_T$ bins, $2$--$5$, $5$--$8$,
and $8$--$12$ GeV, and four signed-rapidity bins, $[-2,-1)$, $[-1,0)$,
$[0,1)$, and $[1,2]$.

\section*{{Summary Grid}}

\begin{{figure}}[H]
\centering
\includegraphics[width=\textwidth]{{figures/2026_pt_y_mass/d0_mass_2026_pt_y_grid.png}}
\caption{{Selected 2026 $D^0$ mass spectra in the requested $p_T$ and signed-$y$ bins. The dashed red line is the nominal $D^0$ mass, 1.86484 GeV.}}
\end{{figure}}

\section*{{Bin Counts}}

\begin{{table}}[H]
\centering
\small
\begin{{tabularx}}{{\textwidth}}{{@{{}}Xrrrrr@{{}}}}
\toprule
Bin & Entries & Peak center & $\pm20$ MeV & $\pm30$ MeV & $\pm50$ MeV \\
\midrule
{chr(10).join(table_rows)}
\bottomrule
\end{{tabularx}}
\caption{{Counts from the selected mass spectra. Window counts are centered on the nominal $D^0$ mass.}}
\end{{table}}

\section*{{Individual Plots}}

{chr(10).join(figure_blocks)}

\section*{{Source}}

Input ROOT file:
\begin{{verbatim}}
{SECTION7_FILE}
\end{{verbatim}}

The plot generator records the chosen tree and branch names in
\texttt{{figures/2026\_pt\_y\_mass/metadata.txt}}.

\end{{document}}
"""


def readme_text() -> str:
    return f"""# 2026 D0 pT-y Mass Plots

Reusable Codex output buffer for the 2026 D0 mass spectra binned in pT and
signed rapidity.

2026 sample:

- {RUN_LABEL}

Requested pT bins:

- 2-5 GeV
- 5-8 GeV
- 8-12 GeV

Requested signed-y bins:

- -2 to -1
- -1 to 0
- 0 to 1
- 1 to 2

Input ROOT file:

- `{SECTION7_FILE}`

Generator:

- `/home/ubuntu/agent-network/research/cms-d0-analysis/scripts/make_2026_pt_y_mass_plots.py`

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
    if not (plot_dir / "bin_counts.csv").exists():
        raise RuntimeError(f"missing generated bin_counts.csv in {plot_dir}")
    clear_general_payload()
    figures = GENERAL_OUTPUT / "figures" / "2026_pt_y_mass"
    figures.mkdir(parents=True, exist_ok=True)
    for path in plot_dir.iterdir():
        if path.suffix.lower() in {".png", ".pdf", ".csv", ".txt"}:
            shutil.copy2(path, figures / path.name)
    write_text(GENERAL_OUTPUT / "README.md", readme_text())
    write_text(GENERAL_OUTPUT / "IMPORT.md", import_text())
    write_text(GENERAL_OUTPUT / "main.tex", latex_report(figures))


def write_pending_report(reason: str) -> None:
    date = dt.datetime.now(dt.UTC).strftime("%Y-%m-%d")
    clear_general_payload()
    write_text(GENERAL_OUTPUT / "README.md", readme_text() + "\n## Current Status\n\n" + reason + "\n")
    write_text(GENERAL_OUTPUT / "IMPORT.md", import_text())
    write_text(
        GENERAL_OUTPUT / "main.tex",
        rf"""\documentclass[10pt]{{article}}
\usepackage[margin=0.7in]{{geometry}}
\usepackage{{hyperref}}
\usepackage{{booktabs}}
\usepackage{{tabularx}}
\setlength{{\parindent}}{{0pt}}
\setlength{{\parskip}}{{0.35em}}
\title{{2026 $D^0$ Mass Peak by $p_T$ and Signed Rapidity}}
\author{{Evan Zhang}}
\date{{Pending data access, {date}}}
\begin{{document}}
\maketitle

\section*{{Requested Plots}}

The requested output is the 2026 selected $D^0$ mass spectrum in 12 bins:

\begin{{table}}[h]
\centering
\begin{{tabularx}}{{\textwidth}}{{@{{}}XX@{{}}}}
\toprule
$p_T$ bins & signed-$y$ bins \\
\midrule
2--5, 5--8, 8--12 GeV & $[-2,-1)$, $[-1,0)$, $[0,1)$, $[1,2]$ \\
\bottomrule
\end{{tabularx}}
\end{{table}}

\section*{{Status}}

The plots were not generated in this run because the local VM does not contain
the required ROOT files and non-interactive LXPLUS SSH currently fails. No fake
or inferred spectra are included.

\section*{{Input Needed}}

Restore SSH access for \texttt{{lxplus-codex}}, then rerun:

\begin{{verbatim}}
/home/ubuntu/agent-network/research/cms-d0-analysis/scripts/make_2026_pt_y_mass_plots.py --sync
\end{{verbatim}}

\section*{{Source Target}}

\begin{{verbatim}}
{SECTION7_FILE}
\end{{verbatim}}

\end{{document}}
""",
    )


def compile_latex() -> None:
    proc = run(["latexmk", "-pdf", "-interaction=nonstopmode", "main.tex"], cwd=GENERAL_OUTPUT, check=False)
    if proc.returncode != 0:
        print(proc.stdout, file=sys.stderr)
        raise RuntimeError("latexmk failed")


def sync_general_output(message: str) -> None:
    run([str(ROOT / "scripts" / "project-sync.sh"), "general-output", "-m", message], cwd=ROOT)


def remote_generate(args: argparse.Namespace) -> Path:
    LOCAL_PLOTS.mkdir(parents=True, exist_ok=True)
    timestamp = dt.datetime.now(dt.UTC).strftime("%Y%m%dT%H%M%SZ")
    remote_dir = f"/tmp/codex_d0_pty_mass_{os.getuid()}_{timestamp}"
    remote_macro = f"{remote_dir}/make_2026_pt_y_mass_plots.C"
    remote_out = f"{remote_dir}/plots"

    with tempfile.TemporaryDirectory() as tmp:
        macro_path = Path(tmp) / "make_2026_pt_y_mass_plots.C"
        macro_path.write_text(ROOT_MACRO, encoding="utf-8")
        run(["ssh", "-o", "BatchMode=yes", args.ssh_host, f"mkdir -p {shlex.quote(remote_dir)}"])
        run(["scp", "-q", str(macro_path), f"{args.ssh_host}:{remote_macro}"])
        macro_call = f'{remote_macro}("{args.input_file}","{remote_out}")'
        remote_cmd = (
            "set -euo pipefail; "
            "command -v root >/dev/null; "
            f"root -l -b -q {shlex.quote(macro_call)}; "
            f"test -f {shlex.quote(remote_out + '/bin_counts.csv')}"
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
    parser.add_argument("--input-file", default=SECTION7_FILE)
    parser.add_argument("--sync", action="store_true", help="Commit/push the general output project after generation")
    parser.add_argument("--pending-on-failure", action="store_true", help="Write a pending-access report if generation fails")
    parser.add_argument("--skip-compile", action="store_true")
    args = parser.parse_args(argv)

    try:
        plot_dir = remote_generate(args)
        update_general_output(plot_dir)
        if not args.skip_compile:
            compile_latex()
        if args.sync:
            sync_general_output("Add 2026 D0 pT-y binned mass plots")
        print(f"Generated plots in {rel(plot_dir)}")
        print(f"Updated {rel(GENERAL_OUTPUT)}")
        return 0
    except Exception as exc:
        if args.pending_on_failure:
            reason = f"Generation failed: `{str(exc).splitlines()[0]}`"
            write_pending_report(reason)
            if not args.skip_compile:
                compile_latex()
            if args.sync:
                sync_general_output("Record pending 2026 D0 pT-y mass plot access")
            print(reason, file=sys.stderr)
            return 2
        print(str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
