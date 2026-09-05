# M1 Implementation Specification — Executable Baseline

**Status:** IMPLEMENTED / SECOND FINAL AUDIT READY  
**Baseline:** 2026-09-05

## 1. Objective

Provide a small independently verified geometry-decision and topology-foundation kernel that M2 can trust without importing external mesher predicate code.

## 2. Implemented production/reference modules

```text
include/femcae/meshing/
├── RobustPredicates.h
├── RobustGeometry.h
└── TetraTopology.h

src/meshing/
├── RobustPredicates.cpp
├── RobustGeometry.cpp
└── TetraTopology.cpp
```

Test-only independent authorities:

```text
tools/meshing_oracle/
├── exact_oracle.py
└── symbolic_oracle.py

tests/meshing/robust_geometry/
└── ...
```

## 3. Predicate contract

Implemented predicates:
- orient2d,
- orient3d,
- incircle,
- insphere.

Contract:
1. finite binary64 input,
2. exact sign with respect to stored binary64 values,
3. true degeneracy returns Zero,
4. no CAD/user tolerance argument,
5. deterministic,
6. no global mutable telemetry,
7. invalid non-finite input is explicit.

## 4. Arithmetic architecture

```text
finite/range checks
→ homogeneous binary64 determinant
→ conservative certified envelope
→ FastCertified if safe
→ exact dyadic BigInt determinant fallback otherwise
```

Exact fallback is independently authored for Dynamics26.

The test oracle is separate:
- Oracle A: Fraction over exact binary64 ratios,
- Oracle B: exact dyadic homogeneous integer determinant.

Production code does not call Python.

## 5. Fast-filter proof

The M1.8 implemented graph differs from the earlier translated-form research model, so it has a dedicated proof:

`m1.8-certified-fast-path/HOMOGENEOUS_BOUND_PROOF.md`

Worst supported graph:
- lifted 5x5 insphere,
- term/lift path <= gamma_8,
- conservative determinant accumulation <= gamma_128,
- computed-permanent correction uses gamma_120.

Chosen coefficient:
```text
2^-43 = 1024u
```

A source `static_assert` verifies the coefficient still dominates the derived bound after final multiplication rounding.

## 6. Compiler contract

For `src/meshing/RobustPredicates.cpp`:
- `-fno-fast-math`,
- `-ffp-contract=off` on supported Clang/GNU compilers,
- compile-time rejection under `__FAST_MATH__`.

Target evidence:
- macOS arm64 Debug,
- macOS arm64 Release.

## 7. Site identity and affine dimension

`RobustGeometry` implements:
- signed-zero normalization at site boundary,
- exact-coordinate duplicate canonicalization,
- deterministic PointId assignment independent of input enumeration,
- source-record provenance aggregation,
- exact affine dimension 0D/1D/2D/3D using robust predicates.

Near-coincident but distinct coordinates are not silently merged.

## 8. Symbolic degeneracy authority

Production predicates preserve exact Zero.

M1 provides a **test-only formal symbolic perturbation oracle** using:
- stable positive PointIds,
- fixed component order,
- sparse exact polynomial arithmetic,
- base-4 exponent hierarchy to avoid degree-2 lift exponent collisions.

Production Delaunay tie consumption remains M2 and must be validated against this authority.

## 9. Tetra topology primitives

`TetraTopology` implements:
- `TetHandle {slot,generation}`,
- invalid/stale handle semantics,
- `TetRecord vertex[4] / neighbor[4]`,
- neighbor[i] opposite vertex[i],
- canonical sorted face key,
- reciprocal-neighbor validator,
- duplicate vertex/tetra detection,
- dead/out-of-range/stale neighbor diagnostics,
- rejection of malformed noncanonical invalid neighbor handles,
- detection of missing adjacency on a face shared by exactly two live tetrahedra,
- detection of non-manifold faces shared by more than two live tetrahedra.

Point-location walking and cavity mutation are M2 scope.

## 10. Verification harness

Executable evidence includes:
- 25 committed golden predicate cases,
- deterministic generated exact-oracle corpus,
- adversarial near-degenerate families,
- positive power-of-two scales,
- exact-representable translations,
- permutation metamorphic checks,
- one-case D26PRED failure replay,
- deliberate tetra corruption corpus including stale/dead/out-of-range, malformed-handle, missing-shared-face adjacency and non-manifold-face cases,
- macOS arm64 Debug/Release CI.

## 11. Telemetry

Caller-owned optional `PredicateTelemetry` records:
- calls,
- fastCertified,
- exactFallback,
- exactZero,
- invalidInput.

No global mutable counter is used.

Release workflow #239 baseline:
```text
cases=1049
calls=1051
fast=1043
exact=6
zero=4
invalid=2
mismatch=0
```

This is an R&D baseline, not a release performance threshold.

## 12. Clean-room boundary

No Gmsh, Netgen, TetGen, CGAL or commercial CAE predicate/meshing source is copied into the M1 kernel.

External projects remain:
- theory/architecture references,
- benchmark/failure-study references,
- optional external research oracles.

## 13. Explicit M1 non-goals

M1 does not implement:
- Delaunay point insertion,
- point-location walking,
- cavity extraction/retriangulation,
- super-tetra/ghost hull,
- CAD surface meshing,
- boundary recovery,
- tetra quality optimization.

Those belong to M2+.

## 14. M2 assumptions after final M1 qualification

M2 may assume:
- orient3d/insphere never intentionally return heuristic epsilon signs,
- FastCertified is bounded by the documented proof,
- uncertainty reaches exact fallback,
- Zero is true degeneracy,
- site duplicates/affine dimension can be resolved before insertion,
- stable PointIds and symbolic test oracle exist,
- tetra handle/face topology validators exist.

M2 remains blocked until the second M1 closeout audit passes.
