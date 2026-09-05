# M2.0 — Conflict Seed and Local Correctness Contract

Status: PROPOSED / derivation record
Date: 2026-09-05

## 1. Purpose

This document closes two questions that must not be left implicit in the first Delaunay constructor:

1. how a guaranteed conflict seed is obtained after exact point location,
2. how local post-insertion legality is connected to global Delaunay correctness.

The contract is written for the Dynamics26 finite+ghost topology and the M2 lift-only symbolic
priority policy.

## 2. Conflict seed is a first-class result

A cavity flood may start only from a cell that the semantic conflict predicate has explicitly
classified as conflict.

Public CGAL documentation imposes the same precondition on its conflict-hole query: the starting
cell must be in conflict. Dynamics26 therefore does not let "located cell" silently mean
"conflict seed".

The M2 locator/planner interface should distinguish:

    LocateResult
    - Vertex
    - Cell
    - Facet
    - Edge
    - OutsideConvexHull

from:

    ConflictSeed
    - verified cell handle
    - verification semantic
    - optional witness facet

## 3. Interior CELL seed

Let p lie strictly inside a finite tetrahedron T.

T is the convex hull of four affinely independent vertices on its circumsphere. A closed Euclidean
ball is convex, and its boundary sphere is strictly convex. Therefore a strict convex combination of
the four tetra vertices lies strictly inside the circumball.

Consequently:

    locate == CELL
    => located finite cell is strictly in conflict

No second search is needed.

## 4. FACET seed

If p lies in the strict interior of a finite/finite shared triangular facet, p is a strict convex
combination of the three face vertices.

Those three vertices lie on each incident tetrahedron circumsphere. Strict convexity of each
circumball places p strictly inside both balls.

Therefore both incident finite cells are conflict cells. Either may be the deterministic seed.

For a finite/ghost hull facet, the finite cell conflicts by the same argument. The ghost cell also
conflicts because the point lies in the finite facet's circumdisk.

Thus exact hull-face insertion is still an ordinary two-cell cavity start.

## 5. EDGE seed

If p lies in the strict interior of an existing edge, it is a strict convex combination of the two
edge endpoints.

Every finite tetrahedron incident to that edge has both endpoints on its circumsphere, so p lies
strictly inside every incident circumball.

In the unified finite+ghost topology, the incident cells form a closed cyclic star. For a hull edge,
the ghost cells complete the cycle.

Therefore an incident finite cell is always a valid seed; the flood discovers the full edge star.

## 6. VERTEX result

Exact coordinate duplicates are removed by M1 canonicalization.

If M2 nevertheless obtains a VERTEX locate result for a supposedly new canonical site, insertion
does not begin. It is an identity/duplicate-contract violation or an already-inserted-site case.

## 7. OUTSIDE_CONVEX_HULL seed

The deterministic walk crosses from a finite cell to a ghost cell only through a violated hull facet.

Let the crossed hull facet be outward-oriented F=(a,b,c), with finite opposite vertex q satisfying

    O(a,b,c,q) < 0.

A strictly violated crossing has

    O(a,b,c,p) > 0,

so p lies in the exterior half-space. By the M2 ghost conflict semantic, the crossed ghost cell is
strictly in conflict and is a valid seed.

Important contract:

    OutsideConvexHull
    must retain the crossed hull facet / ghost witness.

A locator is not allowed to return an arbitrary infinite cell and assume it conflicts.

Reference fallback:
if a future locator cannot produce a verified witness, scan ghost cells with the semantic conflict
predicate and compare with the reference oracle. Qualification requires the normal deterministic walk
path to produce a valid witness without hidden guessing.

## 8. Walk safety contract

The visibility walk uses only strictly violated facets as crossings. Exact-zero facets are boundary
classification information, not negative-progress steps.

Reference implementation records:
- visited cell handles,
- crossed canonical facet keys,
- Orient3D signs.

If a cell repeats, a stale handle appears, or the walk exceeds a conservative topology-sized guard,
return WalkStalled and run the brute-force oracle for diagnosis.

A fallback may preserve a research run, but qualification requires zero unexplained walk/oracle
disagreement and zero hidden cycles on the accepted corpus.

## 9. Local finite-facet geometric embedding

Let two finite tetrahedra share face F={a,b,c}, with opposite vertices d and e.

Choose face ordering so:

    O(a,b,c,d) > 0.

For a valid non-overlapping tetrahedral embedding, e must be on the opposite side:

    O(a,b,c,e) < 0.

If the signs are equal, the cells overlap on the same side of their common face or the local
embedding is otherwise invalid.

If either sign is zero, a finite tetrahedron is degenerate.

This check is independent of Delaunay legality and must run first.

## 10. Local Delaunay legality

With O(a,b,c,d)>0, evaluate:

    S = InSphere(a,b,c,d,e).

Dynamics26 convention:

- S < 0: e is outside the circumsphere of abcd -> strictly locally Delaunay,
- S > 0: e is inside -> locally illegal,
- S = 0: the two tetrahedra are weakly locally Delaunay; geometric Delaunay is degenerate.

The Delaunay Lemma states that for a triangulation of the convex hull, local Delaunay legality of
all facets is sufficient for global Delaunay legality. The weighted/regular form is equivalently local
convexity on the lifting map.

Primary M2 corroboration:
- J. R. Shewchuk, General-Dimensional Constrained Delaunay and Constrained Regular
  Triangulations, I: Combinatorial Properties, DCG 39, 2008,
  DOI 10.1007/s00454-008-9060-3.

## 11. Symbolic local legality

Raw S=0 is geometrically valid but does not identify the canonical M2 topology.

Apply the same global lift-only symbolic InSphere rule used during insertion.

For the current shared facet:

- symbolic InSphere < 0 -> the current facet is canonically locally legal,
- symbolic InSphere > 0 -> the alternate local subdivision is preferred; current topology is
  symbolically illegal.

Therefore M2 has two distinct postconditions:

    weakLocalDelaunay
    symbolicLocalDelaunay

The first checks geometry. The second checks deterministic topology policy.

This distinction is essential: several weak Delaunay triangulations can exist for co-spherical sites.

## 12. Hull support oracle

Ghost closure can be combinatorially valid while the finite hull geometry is wrong.

For every hull triangle F=(a,b,c), orient it outward. In the small-N/global oracle, require every
finite site p not on F to satisfy:

    O(a,b,c,p) <= 0.

Exact zero is allowed for coplanar hull sites.

This proves the triangle lies on a supporting plane of the convex hull.

## 13. Coplanar hull-patch legality

If two adjacent hull triangles lie in the same exact plane, the shared diagonal may be geometrically
non-unique.

Project with the fixed exact coordinate-plane rule from DELAUNAY_MATHEMATICS.md and apply:
- ordinary InCircle for strict local legality,
- lift-only symbolic InCircle on exact zero.

Thus a square face of the unit cube has a deterministic hull diagonal under the same global site
priority policy.

The finite topology fingerprint and hull fingerprint must therefore both be permutation invariant.

## 14. Unified S^3 topology invariant

The finite tetrahedralization fills a 3-ball: the convex hull.

The ghost cells form the cone from a single Infinite vertex over the hull's triangulated 2-sphere.
Gluing these two 3-balls along their shared hull surface yields a closed complex topologically
equivalent to the 3-sphere S^3.

For the unified complex, including the Infinite vertex, let:
- V = vertices,
- E = edges,
- F = triangular facets,
- C = tetrahedral finite+ghost cells.

Reference/global invariants:

    V - E + F - C = 0

because chi(S^3)=0.

Every triangular facet has exactly two incident cells, so:

    4 C = 2 F
    F = 2 C

and therefore:

    E = V + C.

These are strong topology-corruption detectors independent of geometric Delaunay legality.

## 15. Hull-surface count

Let Vh be the number of finite vertices on the triangulated convex-hull surface and H the number of
hull triangles.

For a closed triangulated 2-sphere:

    H = 2 Vh - 4
    Eh = 3 Vh - 6.

Each hull triangle owns one ghost cell, therefore:

    ghostCellCount = H.

This invariant remains valid when coplanar hull polygons are triangulated, provided all hull vertices
are represented in the simplicial surface.

## 16. Golden global counts

### Bootstrap tetrahedron

    finite vertices = 4
    unified V = 5 including Infinite
    finite cells = 1
    ghost cells = 4
    C = 5
    F = 10
    E = 10
    V-E+F-C = 0

### Five-site triangular bipyramid golden topology

For the two finite tetra topology frozen in DELAUNAY_MATHEMATICS.md:

    finite vertices = 5
    unified V = 6
    finite cells = 2
    hull triangles / ghost cells = 6
    C = 8
    F = 16
    E = 14
    Euler = 0

### Unit cube golden topology

The six finite tetrahedra imply the canonical hull triangles:

    1 2 3
    1 2 5
    1 3 5
    2 3 4
    2 4 6
    2 5 6
    3 4 7
    3 5 7
    4 6 8
    4 7 8
    5 6 7
    6 7 8

Counts:

    finite vertices = 8
    unified V = 9
    finite cells = 6
    hull / ghost cells = 12
    C = 18
    F = 36
    E = 27
    Euler = 0

All 40,320 cube insertion permutations must reproduce both finite connectivity and this hull
fingerprint.

## 17. Correctness ladder

M2 validation is layered:

Level 1 — combinatorial:
- valid handles,
- reciprocal neighbors,
- two cells per unified facet,
- no duplicate cells,
- unified S^3 counts.

Level 2 — geometric embedding:
- every finite tetra positive,
- opposite vertices of an internal finite face lie on opposite sides,
- every hull face is a supporting face in small-N oracle mode.

Level 3 — weak Delaunay:
- every internal finite face locally Delaunay using unperturbed exact InSphere,
- coplanar hull patches locally weak-Delaunay in projected 2D.

Level 4 — symbolic canonical topology:
- every exact tie is locally legal under the fixed lift-only priority,
- finite and hull fingerprints match permutation policy.

Level 5 — independent global oracle:
- small-N all-cell/all-site empty-sphere verification,
- brute-force conflict and point-location agreement,
- exhaustive/sampled permutation suites.

No single level substitutes for the others.
