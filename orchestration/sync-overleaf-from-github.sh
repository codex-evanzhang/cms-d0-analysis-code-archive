#!/usr/bin/env bash
# Mirror GitHub heads to registered Overleaf Git remotes for external GitHub pushes.
set -euo pipefail

ROOT="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
PROJECT="${1:---all}"
source "$ROOT/scripts/ledger-hook.sh"
ledger_hook_start script sync-overleaf-from-github "sync-overleaf-from-github.sh $PROJECT" "overleaf github sync"

python3 - "$ROOT" "$PROJECT" <<'PY'
import importlib.util
import os
import subprocess
import sys
from pathlib import Path

root = Path(sys.argv[1])
project_arg = sys.argv[2]
spec = importlib.util.spec_from_file_location("project_registry", root / "scripts" / "project-registry.py")
pr = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pr)

data = pr.load_registry()
rows = pr.projects(data) if project_arg == "--all" else [pr.project_by_id(data, project_arg)]
failed = False
for project in rows:
    if not project.get("overleaf_git_url"):
        continue
    ident = project.get("id")
    path = pr.project_path(project)
    branch = str(project.get("default_branch") or "main")
    if not (path / ".git").exists():
        print(f"{ident}: missing local repo {path}", file=sys.stderr)
        failed = True
        continue
    if pr.git(["status", "--short"], path).stdout.strip():
        print(f"{ident}: dirty local repo; not mirroring", file=sys.stderr)
        failed = True
        continue
    try:
        pr.git(["fetch", "origin", branch], path, capture=False)
        pr.git(["checkout", branch], path, capture=False)
        pr.git(["merge", "--ff-only", f"origin/{branch}"], path, capture=False)
        pr.push_overleaf_remote(project, path)
        pr.verify_overleaf_remote(project, path)
        if str(path).startswith(str(root / "private")):
            subprocess.run(["find", str(path), "-type", "d", "-exec", "chmod", "700", "{}", "+"], check=False)
            subprocess.run(["find", str(path), "-type", "f", "-exec", "chmod", "600", "{}", "+"], check=False)
        print(f"{ident}: mirrored GitHub to Overleaf at {pr.git(['rev-parse', '--short', 'HEAD'], path).stdout.strip()}")
    except Exception as exc:
        print(f"{ident}: mirror failed: {exc}", file=sys.stderr)
        failed = True
if failed:
    raise SystemExit(1)
PY
