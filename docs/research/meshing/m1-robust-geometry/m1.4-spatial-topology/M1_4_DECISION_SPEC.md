# M1.4 Proposed Decision Specification

**Status:** research proposal

## D1 — Geometry/combinatorics/search separation

Candidate ACCEPT:
- predicates/constructions,
- tetra combinatorial store,
- point-location policy
are separate modules.

## D2 — Opposite-face neighbor convention

Candidate ACCEPT:

```text
Tet.vertex[4]
Tet.neighbor[4]
neighbor[i] is across face opposite vertex[i]
```

## D3 — Generation-checked handles

Candidate ACCEPT:
- long-lived tetra references are index+generation handles,
- stale references are detectable,
- raw pointers may only be transient implementation details if ever used.

## D4 — Typed point-location state

Candidate ACCEPT:
- Vertex,
- Edge,
- Facet,
- Cell,
- OutsideConvexHull,
- OutsideAffineHull,
- InvalidTopology/Input.

## D5 — Walk-first point location

**PROPOSED**:
- neighbor walk from a good hint is the primary M2 locator,
- exact brute-force scan is the first correctness fallback,
- heavy dynamic spatial index is deferred until telemetry.

## D6 — Spatial insertion ordering

**PROPOSED**:
- deterministic spatial ordering is part of M2 performance design,
- benchmark Morton/Hilbert/BRIO candidates,
- symbolic PointId order remains independent from insertion order.

## D7 — Cavity epoch marks

Candidate ACCEPT:
- use reusable traversal epoch/mark storage rather than allocate a visited hash set per insertion.

## D8 — Transactional local replacement

Candidate ACCEPT:
- construct/validate cavity replacement before committing external neighbor mutations where practical,
- failed insertion must leave a valid old triangulation.

## D9 — Serial before parallel

Candidate ACCEPT:
- M2.0 is serial,
- compactness/locality optimized before parallel architecture,
- parallel partitioning requires later ADR.

## D10 — Performance telemetry

Candidate ACCEPT:
Every M2 benchmark should record:
- walk steps,
- fallback rate,
- cavity tetra count,
- boundary face count,
- insertion time,
- live tetra count,
- peak temporary memory.

## Open questions

1. super-tetra vs explicit ghost/infinite hull,
2. first spatial ordering algorithm,
3. free-slot reuse policy and canonical TetId ordering,
4. whether generation is stored per slot in Release,
5. exact fallback classifier representation,
6. threshold for adding a point-location acceleration structure.
