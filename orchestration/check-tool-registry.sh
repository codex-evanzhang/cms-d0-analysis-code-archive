#!/usr/bin/env bash
# Validate the machine-readable tool registry.
set -euo pipefail

ROOT="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
exec "$ROOT/scripts/tool-registry.py" check "$@"
