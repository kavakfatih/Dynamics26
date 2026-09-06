# M2.1-A — Semantic Delaunay Predicates Implementation Record

**Date:** 2026-09-06  
**Development milestone:** DEV-MESH-P1A  
**Research authority:** M2.0 DESIGN FROZEN  
**Status at commit creation:** IMPLEMENTATION CANDIDATE / EXACT-HEAD QUALIFICATION PENDING

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

PASS claims require the exact source commit's macOS arm64 Debug/Release test evidence.

## Next subphase

After exact-head qualification:

`DEV-MESH-P1B / M2.1-B — typed finite/infinite bootstrap and reference arena`.
