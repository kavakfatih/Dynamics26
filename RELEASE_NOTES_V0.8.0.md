# FEMCAE V0.8.0 — Release Notes

**Release:** Nonlinear Solver + First Nonlinear GUI Source  
**Date:** 2026-08-29

## Added

- Full Newton-Raphson nonlinear static solver.
- Modified Newton option.
- Residual, displacement-correction and optional energy convergence criteria.
- Backtracking line search.
- Load stepping, adaptive increment growth, cutback and retry.
- Trial/commit/revert failure semantics.
- Detailed convergence history.
- In-memory nonlinear checkpoint/restart foundation.
- C API Total-Lagrangian HEX8 nonlinear preset with convergence-history export.
- Qt `Nonlinear Static / Large Displacement` workflow and convergence table.
- VTK deformed nonlinear HEX8 preset visualization source.
- VER-V080-001 full Newton finite stretch.
- VER-V080-002 modified Newton finite stretch.
- VER-V080-003 checkpoint/restart continuation.
- Nonlinear option/error-path and convergence-history unit tests.

## Changed

- `evaluate_nonlinear_system` accepts optional load factor and scales external loads consistently.
- Project/application version advanced to 0.8.0.
- GUI project JSON stores nonlinear analysis settings additively.
- macOS CI nonlinear gates include Newton/restart labels.

## Fixed / Hardened

- Failed load increments never overwrite last committed state.
- Minimum-increment failure returns an explicit numerical-failure status.
- Line-search candidate failures are handled as rejected trial states rather than silent corruption.
- Invalid convergence configurations are rejected early.
- Release compiler warnings in nonlinear line-search path removed.

## Scope boundaries

- StVK remains a verification/reference material; not a production large-strain rubber model.
- Nonzero prescribed-displacement stepping is not part of V0.8 load-control baseline.
- Arc-length/Riks is not implemented.
- Follower surface-pressure integration is not implemented.
- Checkpoint is in-memory only, not a durable restart file format.
- GUI nonlinear flow is a solver-integration preset, not a full arbitrary-mesh preprocessor.
- Native macOS arm64 Qt/VTK/Accelerate execution remains a CI release gate.
