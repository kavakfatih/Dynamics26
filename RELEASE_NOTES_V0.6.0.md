# FEMCAE V0.6.0 — Release Notes

## Modal Dynamics + Element Family Extension

V0.6.0 lineer structural çekirdeği free-vibration/modal analysis ile genişletir.

### Added

- Consistent/lumped structural mass matrices.
- TRUSS2 mass.
- 2D Euler–Bernoulli beam/frame mass.
- Plane QUAD4 mass.
- Axisymmetric QUAD4 mass with `2πr` measure.
- HEX8 mass.
- Generalized symmetric eigenproblem utilities.
- Vendor-independent dense reference eigensolver.
- ARPACK-NG `dsaupd/dseupd` backend.
- macOS Accelerate/LAPACK `DSYGV` backend source.
- Backend-independent `EigenSolver` facade.
- General `solve_modal_analysis()` driver.
- Frequency, angular-frequency, mass-normalized mode and modal-residual results.
- Full physical-DOF mode reconstruction.
- Global stiffness nullity based zero/rigid-mode count metadata.
- Element orientation-frame metadata validation.
- C API axial modal preset.
- Qt modal result UI and mode selector.
- VTK axial mode-shape animation.
- Two V0.6 modal verification problems and dedicated error-path tests.
- macOS CI ARPACK install/modal gate.

### Verification

- `VER-V060-001`: two-element axial FE modal closed-form discrete eigenvalues.
- `VER-V060-002`: Euler–Bernoulli cantilever first bending frequency.
- `VER-V060-003`: free-free axial rigid translation detection + flexible eigenvalue.

### Scope boundary

V0.6 does **not** claim production-scale sparse shift-invert modal performance. Global `K/M` assembly is sparse, but current generalized eigen backends operate on a dense representation at the eigensolver boundary. ARPACK is functional and verified, while sparse shift-invert/factorization reuse is intentionally deferred to a later performance-hardening step.

Shell orientation infrastructure is extended, but shell stiffness/modal formulation is not yet implemented.

GUI modal workflow remains an integration preset, not an arbitrary mesh/model preprocessor.

### Release gates still open

- Native macOS/arm64 Accelerate execution.
- Native Qt/VTK app bundle build/execution.
- Final project `LICENSE` selection.
