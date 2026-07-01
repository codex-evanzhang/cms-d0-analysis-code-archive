#!/usr/bin/env bash
# Public-safety check for the code archive.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

forbidden_files="$(
  find . -type f \( \
    -name '*.root' -o -name '*.lhe' -o -name '*.hepmc' -o -name '*.edm' -o \
    -name '*.h5' -o -name '*.hdf5' -o -name '*.parquet' -o -name '*.db' -o \
    -name '*.sqlite' -o -name '*.pkl' -o -name '*.pickle' -o -name '*.npy' -o \
    -name '*.npz' -o -name '*.pdf' -o -name '*.png' -o -name '*.jpg' -o \
    -name '*.jpeg' -o -name '*.gif' -o -name '*.zip' -o -name '*.tgz' \
  \) -print
)"
if [[ -n "$forbidden_files" ]]; then
  printf 'Forbidden data/output-like files found:\n%s\n' "$forbidden_files" >&2
  exit 1
fi

large_files="$(find . -type f -size +1M -print)"
if [[ -n "$large_files" ]]; then
  printf 'Files over 1 MB found; review before public push:\n%s\n' "$large_files" >&2
  exit 1
fi

if rg -n \
  '(ghp_[A-Za-z0-9_]+|github_pat_[A-Za-z0-9_]+|sk-[A-Za-z0-9_-]{20,}|-----BEGIN [A-Z ]*PRIVATE KEY-----|GITHUB_PASSWORD|GMAIL_PASSWORD|OVERLEAF_PASSWORD|CERN_PASSWORD|refresh_token|client_secret|AKIA[0-9A-Z]{16})' \
  . -g '!tools/public_safety_check.sh' >/tmp/cms_d0_archive_secret_scan.txt; then
  printf 'High-confidence secret pattern found:\n' >&2
  sed -n '1,80p' /tmp/cms_d0_archive_secret_scan.txt >&2
  exit 1
fi

rm -f /tmp/cms_d0_archive_secret_scan.txt
printf 'Public-safety check passed.\n'
