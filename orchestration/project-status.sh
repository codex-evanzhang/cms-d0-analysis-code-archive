#!/usr/bin/env bash
# Show local/remote status for registered GitHub/Overleaf projects.
set -euo pipefail

ROOT="$(realpath -m "${AGENT_NETWORK_ROOT:-/home/ubuntu/agent-network}")"
exec "$ROOT/scripts/project-registry.py" status "$@"
