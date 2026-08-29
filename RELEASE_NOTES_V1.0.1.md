# FEMCAE V1.0.1 — macOS Release Engineering Hardening

V1.0.1 is a release-engineering patch over the V1.0.0 verified FEM/CAE source baseline. It intentionally adds no new element, constitutive, contact, meshing or solver formulation.

## Changes

- Application/library version synchronized at `1.0.1`.
- Qt project JSON loading now uses `ProjectFileMigrator` instead of rejecting every schema-less legacy project.
- Recognized schema-less V0.x projects can be migrated to project schema 1; arbitrary JSON, malformed schema values and future schemas are rejected.
- A native QtCore migration test is registered with CTest label `migration`.
- GUI application version is derived from CMake `PROJECT_VERSION`; the C++ executable no longer owns a duplicate literal version.
- `FEMCAE.app/Contents/MacOS/FEMCAE --bundle-smoke` validates dyld dependency closure without starting the GUI platform layer.
- macOS bundle audit now validates Info.plist metadata, arm64-only Mach-O files, external dylib references and external `LC_RPATH` entries.
- Signing helper can use either a local `notarytool` keychain profile or an App Store Connect API key.
- Signing flow audits the bundle before notarization, signs nested Mach-O/framework content, signs the outer app last, notarizes ZIP and DMG, staples tickets and runs Gatekeeper assessment.
- New manually triggered `.github/workflows/macos-release.yml` builds, tests, signs, notarizes and uploads macOS arm64 release artifacts when protected Apple secrets are configured.
- Normal macOS GUI CI includes the migration gate and the hardened standalone bundle audit.

## Portable verification in this environment

- Debug: `123/123 PASS`
- Release: `123/123 PASS`
- Installed CLI: PASS
- Installed C API consumer: PASS
- Installed meshing/geometry C++ consumer: PASS
- Workflow YAML parse: PASS
- macOS shell scripts `bash -n`: PASS

## Native release gates still requiring macOS execution

This Linux validation host cannot claim the following as executed:

- Qt `ProjectFileMigrator` native test,
- OCCT/VTK/Accelerate native execution,
- `.app` `otool` / `LC_RPATH` audit,
- Developer ID signing,
- Apple notarization and stapling,
- Gatekeeper `spctl` acceptance.

Those operations are encoded as fail-fast macOS CI/release gates rather than being reported as portable PASS results.
