#!/usr/bin/env python3
"""Generate a public-safe manifest for archived code files."""

from __future__ import annotations

import hashlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "manifests" / "code_manifest.tsv"

SKIP_DIRS = {".git", "__pycache__", ".ipynb_checkpoints"}
SKIP_SUFFIXES = {
    ".root",
    ".lhe",
    ".hepmc",
    ".edm",
    ".h5",
    ".hdf5",
    ".parquet",
    ".db",
    ".sqlite",
    ".pkl",
    ".pickle",
    ".npy",
    ".npz",
    ".pdf",
    ".png",
    ".jpg",
    ".jpeg",
    ".gif",
    ".log",
    ".out",
    ".err",
    ".tgz",
    ".tar",
    ".gz",
    ".zip",
}


def category(path: Path) -> str:
    parts = path.parts
    if parts[0] == "d0-reproduction":
        return "d0"
    if parts[0] == "dplus-generalization":
        return "dplus"
    if parts[0] == "official-mc-fragments":
        return "official_mc"
    if parts[0] == "reporting":
        return "reporting"
    if parts[0] == "orchestration":
        return "orchestration"
    if parts[0] in {"manifests", "tools"}:
        return parts[0]
    return "repo"


def include(path: Path) -> bool:
    if any(part in SKIP_DIRS for part in path.parts):
        return False
    if path.name == "code_manifest.tsv":
        return False
    return path.suffix.lower() not in SKIP_SUFFIXES


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> int:
    rows = []
    for path in sorted(ROOT.rglob("*")):
        if not path.is_file():
            continue
        rel = path.relative_to(ROOT)
        if not include(rel):
            continue
        rows.append((str(rel), category(rel), path.stat().st_size, sha256(path)))

    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("w", encoding="utf-8") as handle:
        handle.write("path\tcategory\tsize_bytes\tsha256\n")
        for rel, cat, size, digest in rows:
            handle.write(f"{rel}\t{cat}\t{size}\t{digest}\n")
    print(f"wrote {OUT.relative_to(ROOT)} with {len(rows)} rows")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
