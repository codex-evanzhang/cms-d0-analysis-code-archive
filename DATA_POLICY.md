# Data And Public-Safety Policy

This repository is public. It must not contain CMS data, private credentials,
or private operational state.

Allowed:

- Source code, shell scripts, ROOT macros, Python scripts, headers, and configs.
- Small documentation files explaining how code was used.
- Small TSV/YAML/JSON manifests that describe code provenance.
- Public or collaboration-neutral path references needed for provenance.

Excluded:

- `.root`, `.lhe`, `.hepmc`, `.edm`, `.h5`, `.parquet`, `.sqlite`, `.db`, and
  other data products.
- CRAB task outputs, job sandboxes, proxy material, logs containing credentials,
  and large transient runtime directories.
- Secrets of any kind, including API tokens, passwords, private keys, VOMS
  proxy material, and OAuth tokens.
- Private diary, people-memory, email, browser, or UI state.

If a future workflow needs a data product, record only the path/provenance and
the command that consumes it. Do not commit the data product itself.

Generated CRAB `PSet.py` / `PSetDump.py` files are allowed when they are needed
to document exactly which CMSSW configuration was submitted. CRAB logs and job
outputs are not allowed.
