# M1 Proposed Implementation Specification

**Status:** RESEARCH SPECIFICATION — NO PRODUCTION CODE YET

## 1. Objective

Provide a small, independently verified geometry-decision kernel that later M2/M3/M4 meshing algorithms can trust.

## 2. Production scope

Initial predicates:

```text
orient2d
orient3d
incircle
insphere
```

Required result:

```text
Negative / Zero / Positive
```

## 3. Non-goals

M1 does not implement:

- Delaunay tetrahedralization,
- CAD surface meshing,
- boundary recovery,
- mesh repair,
- tolerance-based CAD healing,
- arbitrary precision geometry constructions,
- surface projection,
- tetra quality optimization.

## 4. Module boundary

Provisional architecture:

```text
include/femcae/geometry/
  RobustPredicates.h
  GeometryTolerance.h

src/geometry/
  RobustPredicates.cpp
  GeometryTolerance.cpp

tests/geometry/
  test_robust_predicates.cpp
  test_robust_predicates_adversarial.cpp
```

Exact filenames are not accepted until implementation kickoff; the separation is the decision.

## 5. Predicate contract

Each predicate:

1. requires finite binary64 coordinates,
2. returns exact sign with respect to those input binary64 values,
3. returns Zero for true degeneracy,
4. has no user/CAD tolerance argument,
5. does not mutate global mesh state,
6. is deterministic and thread-safe,
7. does not allocate in the ordinary fast-certified path if practical.

## 6. Leading arithmetic design

Proposed three-level design:

### Level 0 — input validation

Reject non-finite input.

### Level 1 — fast determinant + certified filter

Compute an ordinary binary64 estimate and an implementation-specific conservative error bound.

If sign is certified, return it.

### Level 2 — adaptive exact fallback

Use an independently written exact/adaptive arithmetic representation derived from mathematical literature.

Stop as soon as sign is certified; true exact zero reaches final Zero.

No external source code is copied.

## 7. Reference oracle

Before Level 2 production code is trusted, build an independent test-data generator:

```text
binary64 coordinates
→ exact integer ratios
→ arbitrary-precision determinant
→ expected PredicateSign
```

Recommended first tool: Python standard-library exact integer/rational arithmetic used only during test fixture generation.

This gives an independent truth source and avoids validating the implementation against itself.

## 8. Degeneracy policy

M1:
- returns Zero.

M2 Delaunay:
- owns symbolic/tie-break policy.

Proposed Delaunay tie-break inputs:
- exact predicate Zero,
- stable PointIds,
- deterministic ordering rule.

The predicate library does not know about PointId.

## 9. Compiler contract

Predicate target:

- no fast-math,
- no Ofast,
- explicit FP contraction policy consistent with error analysis,
- CI/build guard,
- Debug/Release test parity.

If the arithmetic proof assumes separate rounding, use contraction-off specifically for this target.

## 10. Tolerance contract

`GeometryTolerancePolicy` is a separate module used for:

- CAD validity,
- explicit repair/healing decisions,
- coincidence preprocessing,
- edge/surface sampling reconciliation.

It is not passed into orient/incircle/insphere.

## 11. Instrumentation

Research builds may expose counters through an optional telemetry object:

- calls,
- fast-certified,
- fallback,
- exact-zero,
- invalid-input.

No global mutable counter is required in production.

## 12. Verification requirements

Required before M1 is QUALIFIED:

- deterministic exact fixtures,
- adversarial near-degenerate suite,
- exact independent oracle,
- metamorphic permutation/scale/translation tests,
- randomized replayable campaign,
- Apple Silicon Debug/Release,
- fast-math build guard,
- zero source-code reuse finding in clean-room review.

## 13. Performance research

Measure, do not guess.

Track:

```text
ordinary random inputs
near-degenerate inputs
CAD-like clustered inputs
fallback frequency
time / predicate
```

If exact fallback is too frequent, improve the filter or coordinate preprocessing without weakening correctness.

## 14. Future use by M2

M2 Delaunay is allowed to assume:

- orient3d never returns a wrong nonzero sign,
- insphere never returns a wrong nonzero sign,
- Zero is real degeneracy,
- input validation is explicit.

That lets M2 focus on:
- point location,
- cavity topology,
- adjacency,
- insertion order,
- symbolic tie handling.

## 15. Proposed research decision

ADR-MESH-0005 should be changed from OPEN to ACCEPTED only after:

1. an independent exact oracle prototype exists,
2. at least two arithmetic strategies are benchmarked or one is convincingly justified,
3. compiler assumptions are experimentally validated,
4. no-copy review is complete.
