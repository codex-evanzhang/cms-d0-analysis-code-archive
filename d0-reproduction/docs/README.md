# CMS D0 Analysis Workspace

<!-- DOC_OWNER: cms-analysis-manager active D0 mass-peak workspace. -->
<!-- TOKEN_NOTE: start here, then use scripts/cms-analysisctl instead of scanning. -->

This workspace is the local control surface for Evan's near-term CMS D0
mass-peak analysis work.

Actual CERN-side reproduction work lives on LXPLUS at
`/afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction`; large
outputs should live under
`/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction`.

Primary goals:

- keep run/sample metadata machine-readable;
- make analysis commands reproducible;
- keep plots and supervisor summaries easy to refresh through the
  `cms-plotting-producer` route;
- route nontrivial claims through producer/plotting/checker/reviewer passes.

Key files:

- `CMS_ANALYSIS_PLAN.md`: summer analysis plan, source-handling policy, phases,
  and cross-check standards.
- `ANALYSIS_ROADMAP.md`: AN-derived section roadmap for data, MC, software,
  filters, corrections, systematics, and final-note assembly.
- `registry.yaml`: run/sample/output registry.
- `RUNS.md`: human-readable run notes.
- `DATASETS.md`: dataset and path notes.
- `COMMANDS.md`: reproducible command recipes.
- `CUT_DERIVATION.md`: cut-derivation notes and selection-independence
  protocol.
- `config/selection_independence.yaml`: machine-readable audit of which
  selection definitions are independent, partial, or still AN-derived.
- `output/selection-independence/selection_independence_audit.md`: generated
  selection-independence report.
- `PLOTS.md`: generated plot gallery index; plot generation and gallery updates
  route through `cms-plotting-producer`.
- `CHECKLISTS.md`: validation and review checklists.
- `SUPERVISOR_SUMMARY.md`: polished rolling update buffer.
- `logs/analysis-ledger.jsonl`: append-only local action ledger.

Controller shortcut:

```bash
/home/ubuntu/agent-network/scripts/cms-analysisctl status
```

LXPLUS reproduction shortcut:

```bash
/home/ubuntu/agent-network/scripts/lxplusctl ssh /afs/cern.ch/work/e/evzhang/codex-work/analysis/d0-reproduction/scripts/d0-reproctl status
```

Reviewed CMS task shortcut:

```bash
/home/ubuntu/agent-network/scripts/run-cms-reviewed-task.sh /home/ubuntu/agent-network/work/tasks/TASK.md LABEL
```
