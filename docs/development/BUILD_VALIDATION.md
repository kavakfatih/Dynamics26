# V1.0.2 Build Validation

## Portable validation host

This environment is not macOS and therefore validates the portable source/core path only.

- Debug CTest: **123/123 PASS**
- Release CTest: **123/123 PASS**
- Installed CLI: **PASS**
- Installed C API consumer: **PASS**
- Installed meshing/geometry C++ consumer: **PASS**
- macOS workflow YAML parse: **PASS**
- `audit_bundle.sh` syntax: **PASS**
- `sign_and_notarize.sh` syntax: **PASS**

Installed CLI reports:

```text
FEMCAE Verified Engineering Release
Application version : 1.0.2
Project schema      : 1
Result schema       : 1
C API version       : 1
```

## Native-only gates

Qt project-migration CTest and `.app` signing/notarization/audit require native macOS and are intentionally not marked PASS here.

## V1.0.2 repository/reproducible-release validation — 2026-08-29

- Debug CTest: **124/124 PASS**.
- Release CTest: **124/124 PASS**.
- `unit_v102_repository_hardening`: PASS; deterministic ZIP + temporary-tree Git bootstrap.
- Installed CLI: reports `1.0.2`.
- Installed C API consumer: PASS.
- Installed meshing/geometry C++ consumer: PASS.
- Clean `femcae_meshing` target build also builds `femcae_geometry`: PASS.
- GitHub workflow YAML parse: PASS.
- macOS/github shell syntax: PASS.
- Python release/hygiene script syntax: PASS.
- Debug/Release source compiler warnings observed in build logs: 0.
- Native GitHub/macOS execution: pending because no remote FEMCAE repository is exposed to the connected GitHub app in this session.
