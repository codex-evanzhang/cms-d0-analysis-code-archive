#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
# shellcheck source=/home/ubuntu/agent-network/scripts/ledger-hook.sh
source "$ROOT_DIR/scripts/ledger-hook.sh"
ledger_hook_start script cms-d0-an-promote "promote_an_recreation_to_general_output.sh" "cms d0 overleaf latex"

SOURCE_DIR="${1:?usage: promote_an_recreation_to_general_output.sh SOURCE_DIR [COMMIT_MESSAGE]}"
MESSAGE="${2:-Recreate D0 AN draft from reproduced data}"
TARGET="$ROOT_DIR/repos/general-codex-output"

if [[ ! -f "$SOURCE_DIR/main.tex" ]]; then
  printf 'Missing generated main.tex under %s\n' "$SOURCE_DIR" >&2
  exit 2
fi

mkdir -p "$TARGET"
find "$TARGET" -mindepth 1 -maxdepth 1 \
  ! -name '.git' \
  ! -name '.gitignore' \
  -exec rm -rf {} +

rsync -a \
  --exclude='*.aux' \
  --exclude='*.fdb_latexmk' \
  --exclude='*.fls' \
  --exclude='*.log' \
  --exclude='*.out' \
  --exclude='latexmk.stdout' \
  "$SOURCE_DIR/" "$TARGET/"

cat >"$TARGET/IMPORT.md" <<'EOF'
# Import Into Overleaf

This repository is generated from the CMS \(D^0\) AN recreation workflow.
Use `main.tex` as the main document.
EOF

cat >"$TARGET/README.md" <<EOF
# General Codex Output

Generated from:

\`$SOURCE_DIR\`

This output is intentionally overwritten for current general reporting tasks.
EOF

(
  cd "$TARGET"
  latexmk -pdf -interaction=nonstopmode -halt-on-error main.tex
)

"$ROOT_DIR/scripts/project-sync.sh" general-output --target overleaf --skip-compile -m "$MESSAGE"

"$ROOT_DIR/scripts/cms-analysisctl" ledger \
  --event d0-an-recreation-promoted-overleaf \
  --detail "source=$SOURCE_DIR target=$TARGET message=$MESSAGE" \
  --source "research/cms-d0-analysis/scripts/promote_an_recreation_to_general_output.sh" \
  --tag cms \
  --tag d0 \
  --tag overleaf >/dev/null

printf 'promoted_source=%s\n' "$SOURCE_DIR"
printf 'target=%s\n' "$TARGET"
printf 'pdf=%s\n' "$TARGET/main.pdf"
