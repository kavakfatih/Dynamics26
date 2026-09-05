# Point Location Theory

## 1. Problem

For an incremental Delaunay insertion, a new site `p` must first be located relative to the current tetrahedralization.

The desired result is not merely a tetra index.

A robust locator should distinguish:

```text
VERTEX
EDGE
FACET
CELL
OUTSIDE_CONVEX_HULL
OUTSIDE_AFFINE_HULL
```

and preserve exact degeneracy information.

CGAL publicly exposes essentially this class of typed point-location result. This is an architecture reference, not an API to copy.

## 2. Neighbor walking

A tetrahedralization already stores local adjacency.

Starting from a seed tetrahedron, evaluate the query point against the oriented faces.

If the point lies outside one face, cross to that face's neighbor and continue.

Advantages:

- tiny additional memory,
- excellent locality when the seed is close,
- natural fit to incremental insertion,
- uses topology already required by Bowyer-Watson.

Risks:

- poor seed can cause long walks,
- degeneracy must be handled carefully,
- naive deterministic face choice can cycle if predicates/tie policy are inconsistent,
- outside-hull handling requires explicit infinite/super-domain representation or fallback policy.

Devillers, Pion and Teillaud studied walking strategies in 2D/3D triangulations and emphasize both practical simplicity and robustness issues near degeneracy.

## 3. Hint quality matters

CGAL documentation allows `locate(query, start)` and explicitly states that a good starting hint can make the default walk preferable to an additional fast-location structure.

When range-inserting points, CGAL uses spatial sorting to obtain useful locality/hints.

TetGen's 2015 paper likewise states that, for large data sets, point insertion efficiency depends strongly on insertion order; its spatial sorting makes point location close to constant time in practice.

### Dynamics26 implication

M2 should treat:

```text
point location strategy
+
insertion ordering
```

as one performance system.

Do not benchmark walk location under adversarial random ordering and conclude that a heavy search tree is mandatory.

## 4. Spatial sorting / BRIO

Pure random insertion provides useful expected-complexity properties but poor cache locality.

Amenta, Choi and Rote's BRIO work introduces a biased randomized insertion order that preserves enough randomness while improving memory locality.

Later Delaunay implementations combine BRIO-like rounds with space-filling-curve ordering.

HXT/Gmsh research code and Marot–Pellerin–Remacle's published work show this family scaling to very large 3D Delaunay problems.

### Dynamics26 leading research direction

For initial reproducible M2:

```text
deterministic BRIO-like rounds
+
deterministic Morton/Hilbert/Moore-style local ordering
```

is a candidate, but not yet accepted.

The first implementation may begin with a simpler deterministic Morton order and compare it against:
- PointId order,
- random seeded order,
- BRIO + spatial curve.

## 5. Typed point-location result

Proposed conceptual result:

```text
PointLocationResult
- type
- containingTet / incidentTet
- localFace / localEdge / localVertex as applicable
- exact predicate classification
- walkSteps
- fallbackUsed
```

The final type should not expose debugging fields unless useful.

## 6. Exact face classification

A query relative to a positively oriented tetrahedron can be classified by four orientation predicates against its oriented boundary faces.

The local face orientation convention must be frozen so that:
- inside means a consistent sign/nonnegative relation,
- one zero means on a facet,
- two independent zeros may indicate an edge,
- three may indicate a vertex.

M1.3 symbolic tie policy is **not** automatically applied when the locator's job is to report that the query lies exactly on a face/edge/vertex.

Geometric classification and combinatorial tie-breaking are different operations.

## 7. Slow fallback

The first M2 implementation needs an obviously correct escape path when walking fails.

Candidate prototype fallback:

1. scan all live tetrahedra using exact/certified classification,
2. if no finite cell contains the point, classify outside hull / invalid topology.

This is O(n), but simple enough to validate the walk implementation.

Later alternatives:
- dynamic AABB/BVH,
- Delaunay hierarchy,
- auxiliary sampled triangulation,
- spatial hash/grid.

A fallback should be added for measured need, not architecture fashion.

## 8. Infinite region representation

Two candidates:

### P0 — super-tetrahedron
Start with an enclosing artificial tetrahedron and remove all tetrahedra touching super vertices at the end.

Pros:
- common incremental-construction technique,
- simple finite topology during insertion.

Risks:
- constructing a numerically safe enclosing tetra over huge coordinate ranges,
- artificial vertices interact with predicates.

### P1 — explicit infinite/ghost tetrahedra
Represent convex-hull facets with an infinite/ghost vertex or boundary-neighbor state.

Pros:
- explicit hull topology,
- mature triangulation libraries use related concepts.

Risks:
- more complex predicates and local conventions.

M2 should prototype P0 first for simplicity, but keep hull representation abstract enough to change after experiments.

## 9. Acceptance

Point location is qualified when:
- exact classifications match oracle fixtures,
- walk and slow fallback agree,
- no infinite loop/cycle occurs,
- walk step count benefits measurably from spatial ordering,
- input permutation under stable symbolic identity does not change canonical topology.


## 10. M2.0 resolution note — 2026-09-05

Section 8 captured two M1.4 candidates and its then-leading suggestion to prototype P0
super-tetrahedron first.

M2.0 research has now resolved this open decision in favor of P1 finite+ghost topology. The numeric
super-tetrahedron is rejected for the reference architecture because it introduces artificial extreme
coordinates into the robust-predicate boundary.

Current authority:
- docs/research/meshing/m2-delaunay/M2_0_REFERENCE_ARCHITECTURE.md
- docs/research/meshing/m2-delaunay/DELAUNAY_MATHEMATICS.md
- docs/research/meshing/m2-delaunay/LOCAL_CORRECTNESS_AND_SEED_CONTRACT.md

This note supersedes the old "prototype P0 first" suggestion without rewriting M1.4 history.
