# FEMCAE V1.0.2 — Repository / Reproducible Release Hardening

V1.0.2 is a repository and release-reproducibility patch over V1.0.1. It adds no new FEM formulation and intentionally preserves the V1.0 numerical contracts.

## Added

- Deterministic source ZIP generator with normalized entry order, timestamps and Unix permissions.
- Archive-internal `SHA256SUMS.txt` generated from the exact source bytes included in the ZIP.
- Repository hygiene gate for generated build/stage/dist files, compiled binaries and Apple signing credential material.
- GitHub bootstrap helper that can initialize `main`, create an initial source commit, configure `origin`, optionally create a repository through authenticated GitHub CLI, and push.
- V1.0.2 CTest release gate that verifies deterministic archive bytes and performs a real no-push Git bootstrap in a temporary extracted source tree.
- Git `.gitattributes` and expanded `.gitignore` release/repository policies.
- GitHub CI source-integrity/reproducibility gate, workflow concurrency and manual CI dispatch.
- Signed release workflow now produces a deterministic source archive alongside signed/notarized macOS application artifacts.
- GitHub bootstrap/branch-check documentation.

## Changed

- Application/library patch version raised to `1.0.2`.
- Shared library `VERSION` and `SOVERSION` derive from `PROJECT_VERSION` / `PROJECT_VERSION_MAJOR`; the shared-library version is no longer duplicated as a literal.
- `femcae_geometry` is now defined before `femcae_meshing`, so `target_link_libraries(femcae_meshing PUBLIC femcae_geometry)` is a real CMake target dependency rather than an order-sensitive plain linker item.
- macOS CI artifact names and release workflow default version updated to 1.0.2.

## Verification

- Portable Debug: **124/124 PASS**.
- Portable Release: **124/124 PASS**.
- V1.0.2 repository/reproducibility CTest: PASS.
- Two independently produced source ZIPs are byte-for-byte identical and have the same SHA256.
- Bootstrap smoke creates a real initial Git commit and expected `origin` in a temporary extracted source tree.
- Installed CLI reports `1.0.2`.
- Installed C API consumer: PASS.
- Installed meshing/geometry C++ consumer: PASS.
- Building only the `femcae_meshing` target from a clean build also builds `femcae_geometry`: PASS.
- Debug/Release build warning scan: 0 source compiler warnings.

## Remote GitHub status

No FEMCAE repository was visible through the connected GitHub account during this release, and the available connector does not expose repository creation. V1.0.2 therefore does **not** claim that a remote repository was created or that native GitHub Actions have already run. The source tree is prepared for that first publication.

## Native release gate

Developer-ID signing, notarization, Qt/VTK/OCCT/Accelerate execution and final Apple Silicon `.app` evidence remain native macOS execution gates exactly as in V1.0.1.
