# M2.1-A — Semantic Delaunay Predicates Implementation Record


> 2026-09-06 independent audit: ADR-MESH-0043 corrects the oblique coplanar
> circle metric. Corrected source `6962866dde18fa5dc17c48f49212594b091c2e32`
> passed macOS arm64 workflow [#298](https://github.com/kavakfatih/Dynamics26/actions/runs/34037763796).
> M2-G09 / coplanar G10 are requalified for the expanded P1A corpus.
> Historical evidence below remains scoped to its original commit.

**Date:** 2026-09-06  
**Development milestone:** DEV-MESH-P1A  
**Research authority:** M2.0 DESIGN FROZEN  
**Status:** QUALIFIED IN DECLARED P1A SUBSCOPE — exact-head workflow #293 SUCCESS

## Scope

This subphase implements only the frozen semantic-predicate layer:

- positive finite-cell InSphere conflict semantic,
- D26LIFT1 lift-only symbolic InSphere,
- outward ghost half-space semantic,
- exact coplanar projected circumcircle test,
- D26LIFT1 lift-only symbolic InCircle.

No ghost storage, bootstrap, point location, cavity discovery or topology mutation is introduced.

## Mathematical authority

Primary repository derivations:

- `DELAUNAY_MATHEMATICS.md`,
- `M2_0_REFERENCE_ARCHITECTURE.md`,
- `M2_0_RESEARCH_FREEZE_AUDIT.md`,
- `EXPERIMENT_PLAN.md`.

External theory/behavior references already registered in `SOURCE_REGISTRY.md`:

- M2-TH-002 — Devillers & Teillaud, 3D Delaunay symbolic perturbation,
- M2-DOC-001 — CGAL public finite/infinite Delaunay behavior,
- TH-006/TH-007 — robust geometric predicate background.

These are research/behavior references only. Production code is original Dynamics26 code and reuses
the already-qualified M1 predicate API.

## Implementation boundary

Private source ownership:

- `src/meshing/m2/DelaunayPredicates.h`,
- `src/meshing/m2/DelaunayPredicates.cpp`.

The layer does not alter `RobustPredicates` and adds no installed/public header.

Frozen semantics:

```text
positive finite tet:
  InSphere Positive -> Conflict
  InSphere Negative -> NoConflict
  InSphere Zero     -> D26LIFT1

outward hull facet:
  Orient3D(query) Positive -> Conflict
  Orient3D(query) Negative -> NoConflict
  Orient3D(query) Zero     -> projected circumdisk -> D26LIFT1 on exact circle
```

No numeric epsilon is constructed.

## Independent evidence

C++ test:

- positive finite-cell inside/outside semantics,
- five-site exact co-spherical golden set,
- all 120 row permutations,
- four-site exact co-circular golden set,
- all 24 row permutations,
- ghost exterior/interior/coplanar semantics,
- XY -> XZ projection fallback,
- invalid orientation/coplanarity/PointId rejection.

Independent Python oracle:

- represents the formal lift perturbation as an exact sparse polynomial in epsilon,
- evaluates determinants with `fractions.Fraction`,
- does not import production C++ or production exact-arithmetic code,
- verifies the same 120 + 24 permutation families.

## Gate intent

This subphase targets:

- M2-G02,
- M2-G08,
- M2-G09,
- M2-G10.

Exact-head evidence on `0508c7056917d016130e7713f1af1d78d9605012`:

- workflow #293: `completed/success`,
- Debug: 157/157 CTest PASS,
- Release: full build/test and release-hardening gates PASS,
- `unit_m21a_delaunay_predicates`: PASS,
- `unit_m21a_symbolic_polynomial_oracle`: PASS,
- gui-fast: PASS.

Therefore the declared P1A evidence closes:

- **M2-G02: PASS**,
- **M2-G08: PASS**,
- **M2-G09: PASS**,
- **M2-G10: PASS**.

This does not close M2 overall and does not close M2-G20, which belongs to the completed serial
reference constructor qualification.

## Next subphase

`DEV-MESH-P1B / M2.1-B — typed finite/infinite bootstrap and reference arena`.
