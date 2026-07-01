#!/usr/bin/env python3
"""Build a file-level usage manifest for the archived analysis code."""

from __future__ import annotations

from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]
AGENT_ROOT = ROOT.parents[1]
TOOL_REGISTRY = AGENT_ROOT / "registry" / "tools.yaml"
OUT = ROOT / "manifests" / "file_usage.tsv"


def load_tool_purposes() -> dict[str, list[str]]:
    if not TOOL_REGISTRY.exists():
        return {}
    data = yaml.safe_load(TOOL_REGISTRY.read_text(encoding="utf-8")) or {}
    result: dict[str, list[str]] = {}
    for item in data.get("tools", []):
        path = str(item.get("path") or "")
        purpose = str(item.get("purpose") or "").replace("\t", " ").replace("\n", " ")
        if not path or not purpose:
            continue
        result.setdefault(path, []).append(purpose)
    return result


def source_for(rel: Path) -> str:
    text = str(rel)
    if text.startswith("d0-reproduction/scripts/"):
        return text.replace("d0-reproduction/scripts/", "research/cms-d0-analysis/scripts/", 1)
    if text.startswith("d0-reproduction/config/"):
        return text.replace("d0-reproduction/config/", "research/cms-d0-analysis/config/", 1)
    if text.startswith("d0-reproduction/docs/"):
        return "research/cms-d0-analysis/" + rel.name
    if text.startswith("dplus-generalization/scripts/"):
        return text.replace("dplus-generalization/scripts/", "research/cms-dplus-analysis/scripts/", 1)
    if text.startswith("dplus-generalization/docs/"):
        return "research/cms-dplus-analysis/" + rel.name
    if text.startswith("official-mc-fragments/"):
        return text.replace("official-mc-fragments/", "work/cms-official-mc/", 1)
    if text.startswith("lxplus-execution/d0-reproduction/"):
        return text.replace(
            "lxplus-execution/d0-reproduction/",
            "/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/",
            1,
        )
    if text.startswith("lxplus-execution/dplus-peak/"):
        return text.replace(
            "lxplus-execution/dplus-peak/",
            "/afs/cern.ch/work/e/evzhang/codex-work/analysis/dplus-peak/",
            1,
        )
    if text.startswith("cmssw-custom/"):
        return text.replace(
            "cmssw-custom/",
            "/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src/",
            1,
        )
    if text.startswith("cmssw-reference/"):
        return text.replace(
            "cmssw-reference/",
            "/afs/cern.ch/user/e/evzhang/Foresting/CMSSW_16_1_1/src/",
            1,
        )
    if text.startswith("reporting/scripts/"):
        return text.replace("reporting/scripts/", "repos/general-codex-output/support/slides/scripts/", 1)
    if text.startswith("reporting/tex/"):
        return "repos/general-codex-output/" + rel.name
    if text.startswith("orchestration/"):
        return "scripts/" + rel.name
    return text


def context_for(rel: Path) -> str:
    text = str(rel)
    if text.startswith("d0-reproduction/scripts/"):
        return "D0 reproduction analysis workflow"
    if text.startswith("d0-reproduction/config/"):
        return "D0 reproduction configuration"
    if text.startswith("d0-reproduction/docs/"):
        return "D0 reproduction documentation and command provenance"
    if text.startswith("dplus-generalization/"):
        return "D+ Kpipi peak generalization test"
    if text.startswith("official-mc-fragments/"):
        return "Official D0 UPC MC setup"
    if text.startswith("lxplus-execution/"):
        return "Exact LXPLUS execution snapshot"
    if text.startswith("cmssw-custom/"):
        return "Custom CMSSW code created for the reproduction"
    if text.startswith("cmssw-reference/"):
        return "Small CMSSW/Dfinder/ZDC reference subset"
    if text.startswith("reporting/"):
        return "Report, Overleaf, and slide generation"
    if text.startswith("orchestration/"):
        return "Agent-host orchestration helper"
    if text.startswith("tools/"):
        return "Archive maintenance"
    if text.startswith("manifests/"):
        return "Archive manifest"
    return "Repository metadata"


def heuristic_purpose(rel: Path) -> str:
    name = rel.name
    stem = rel.stem
    if name.endswith(".md"):
        return "Documentation describing workflow state, command recipes, or archive policy."
    if name.endswith((".yaml", ".yml")):
        return "Structured configuration or registry used by the workflow."
    if stem.startswith("run_") or stem.startswith("launch_"):
        return "Workflow runner used to execute local, LXPLUS, CRAB, or detached analysis steps."
    if stem.startswith("derive_"):
        return "Cut-derivation or selection-study code used to justify analysis thresholds."
    if stem.startswith("render_") or stem.startswith("plot_") or stem.startswith("make_"):
        return "Plotting or visualization code used to create diagnostic analysis figures."
    if stem.startswith("build_"):
        return "Report, table, scaffold, or configuration builder."
    if stem.startswith("summarize_") or stem.startswith("audit_"):
        return "Summary/audit code used to compact validation outputs into tables or notes."
    if stem.startswith("compare_") or stem.startswith("validate_") or stem.startswith("inspect_"):
        return "Validation/inspection code used to compare branches, schemas, forests, or outputs."
    if stem.startswith("minimize_"):
        return "Forest-pruning code used to keep only analysis-relevant branches."
    if name.endswith((".C", ".h")):
        return "ROOT/C++ analysis helper or macro."
    if name.endswith(".py"):
        return "Python workflow, plotting, validation, or maintenance helper."
    if name.endswith(".sh"):
        return "Shell workflow wrapper."
    if name.endswith(".tex"):
        return "TeX source for public-facing analysis/report output."
    return "Archived workflow file."


def include(rel: Path) -> bool:
    if any(part in {".git", "__pycache__"} for part in rel.parts):
        return False
    if rel == Path("manifests/file_usage.tsv"):
        return False
    return rel.is_file()


def main() -> int:
    purposes = load_tool_purposes()
    rows = []
    for path in sorted(ROOT.rglob("*")):
        rel = path.relative_to(ROOT)
        if not include(rel):
            continue
        source = source_for(rel)
        purpose = "; ".join(purposes.get(source, [])) or heuristic_purpose(rel)
        rows.append((str(rel), source, context_for(rel), purpose))

    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("w", encoding="utf-8") as handle:
        handle.write("archive_path\tsource_path\tusage_context\tpurpose\n")
        for row in rows:
            handle.write("\t".join(cell.replace("\t", " ").replace("\n", " ") for cell in row) + "\n")
    print(f"wrote {OUT.relative_to(ROOT)} with {len(rows)} rows")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
