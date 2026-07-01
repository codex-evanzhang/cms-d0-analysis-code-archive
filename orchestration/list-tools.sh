#!/usr/bin/env bash
# Query the machine-readable tool registry.
set -euo pipefail

ROOT="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
case "${1:-}" in
  list|search|info|check)
    exec "$ROOT/scripts/tool-registry.py" "$@"
    ;;
  *)
    exec "$ROOT/scripts/tool-registry.py" list "$@"
    ;;
esac
