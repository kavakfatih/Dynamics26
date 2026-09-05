# M1.5 — Executable Verification Harness Research

**Program:** Dynamics26 Original Meshing System R&D  
**Parent:** M1 — Robust Geometry Foundation  
**Depends on:** M1.1–M1.4  
**State:** RESEARCHING  
**Baseline:** 2026-09-05  
**Production mesher code:** none in this work package

## Objective

Convert the M1 mathematical and topology research into an executable verification architecture before the first serial Bowyer-Watson implementation begins.

The harness must independently verify:

- exact predicate truth,
- fast-filter certification safety,
- degeneracy policy,
- tetra topology invariants,
- point-location correctness,
- deterministic replay,
- later M2 Delaunay invariants.

## Existing repository fit

Dynamics26 already uses:

- CMake/CTest,
- test labels,
- dedicated C++ meshing tests,
- Python test scripts,
- Debug/Release macOS arm64 CI.

M1.5 extends this existing infrastructure. It does not introduce a new C++ unit-test framework.

## Leading harness architecture

```text
Python standard-library exact oracle
        ↓
bit-exact predicate fixtures
        ↓
C++ fixture reader
        ↓
RobustPredicate tests
        ↓
Degeneracy tests
        ↓
Tet topology tests
        ↓
Point-location tests
        ↓
M2 Delaunay verification
        ↓
CTest labels / macOS arm64 CI
```

## Documents

- `HARNESS_ARCHITECTURE.md`
- `FIXTURE_FORMAT.md`
- `ORACLE_GENERATION_PIPELINE.md`
- `TEST_TIERS_AND_CI.md`
- `COMMERCIAL_CAE_VERIFICATION_BENCHMARK.md`
- `EXPERIMENT_PLAN.md`
- `M1_5_DECISION_SPEC.md`

## Core principle

No optimized algorithm is allowed to define its own test truth.

```text
production implementation
!=
reference oracle
```

A production optimization passes only when it agrees with an independently generated truth corpus and the mathematical/topological invariants.

## M1.5 exit

Research is ready for executable prototype when:

1. fixture format is frozen,
2. oracle A/B agreement is required,
3. corpus tiers are frozen,
4. failure replay/minimization contract is frozen,
5. CTest labels and CI gates are specified,
6. commercial mesh-quality benchmarks are explicitly separated from internal predicate correctness.
