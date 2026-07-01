# Orchestration Helpers

Selected helper scripts from the agent host.

These are not physics-analysis code, but they were part of the practical
reproduction workflow:

- `lxplusctl`: SSH wrapper for LXPLUS command execution.
- `cms-analysisctl`: local CMS workspace helper for summaries, status, and
  ledger-style operations.
- `cms-official-mcctl`: helper for official-MC setup tasks.
- `project-sync.sh`, `project-status.sh`, `sync-overleaf-from-github.sh`:
  GitHub/Overleaf project synchronization helpers.
- `check-tool-registry.sh`, `list-tools.sh`: local tool-registry checks.

No secrets are included. These scripts expect credentials/configuration to live
outside this public repo.
