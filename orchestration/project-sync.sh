#!/usr/bin/env bash
# Compile, commit, push, and report Overleaf action for a registered project.
set -euo pipefail

ROOT="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
source "$ROOT/scripts/ledger-hook.sh"
ledger_hook_start script project-sync "project-sync.sh ${1:-PROJECT}" "project sync"
"$ROOT/scripts/project-registry.py" sync "$@"
