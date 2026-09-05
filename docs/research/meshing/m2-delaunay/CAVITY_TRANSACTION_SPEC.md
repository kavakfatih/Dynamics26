# M2.0 — Conflict Cavity and Transaction Specification

Status: PROPOSED / implementation acceptance contract
Date: 2026-09-05

## Unified cavity model

With finite + ghost cells, insertion uses one topological model whether the new point is inside the
convex hull, on an internal face/edge, on the convex hull, or outside the current convex hull.

A Delaunay conflict set is connected and forms a hole. Public CGAL documentation exposes conflict
cells, boundary facets and internal facets. Devillers-Teillaud describe the incremental conflict hole
as star-shaped with respect to the inserted point, which is the geometric reason it can be
retriangulated by coning the point to every boundary triangle.

## Cell/facet counting invariant

Let:
- C = number of conflict cells,
- I = number of facets shared by two conflict cells,
- B = number of cavity boundary facets.

Every tetra/ghost cell has four facets. Internal facets are counted twice and boundary facets once:

    4 C = 2 I + B

This is a mandatory reference pre-commit check.

## Cavity boundary manifold checks

In the finite+ghost combinatorial topology, a valid insertion cavity is a topological 3-ball. Its
boundary is a closed triangulated 2-sphere.

For boundary vertex/edge/triangle counts Vb, Eb, B:

    3 B = 2 Eb
    Vb - Eb + B = 2

Therefore:
- every boundary edge has incidence exactly 2,
- boundary connectivity is one component,
- Euler characteristic is 2,
- B is even.

These are reference/debug qualification invariants.

## Why face/edge insertions use the same algorithm

A Euclidean ball is strictly convex.

A query in the interior of an existing triangular facet is a positive convex combination of three
points on the circumsphere of each incident finite tetrahedron, so it lies strictly inside those
balls.

A query in the interior of an existing edge lies strictly inside the circumball of every finite
tetrahedron incident to that edge.

For a hull facet, its circumdisk is convex; a point in the triangle interior lies inside that disk, so
the incident ghost cell is also in conflict.

Thus exact face/edge insertion does not require a separate epsilon topology hack. Location is exact;
normal conflict flooding removes the complete star that must be retriangulated.

## Canonical insertion cases

Counts below describe minimal/local topology. Additional surrounding cells may conflict if their
circumspheres contain the query.

### Exact duplicate / existing vertex

Upstream M1 canonicalization merges identical coordinates.

If M2 sees a VERTEX locate result for a supposedly new canonical site, treat it as duplicate/identity
handling, not tetra insertion.

### Point strictly inside one tetrahedron

Minimal case:

    C = 1
    I = 0
    B = 4
    new cells = 4

All four new cells are finite.

### Point in a shared facet interior

Both cells incident to the facet conflict. This works identically for finite/finite internal facet
and finite/ghost convex-hull facet.

    C = 2
    I = 1
    B = 6
    new cells = 6

Internal finite/finite case: six finite tetrahedra.
Hull finite/ghost minimal case: three finite + three ghost cells.

The old facet containing the query is internal to the cavity, so coning the boundary cannot create
the obvious flat tetrahedron on that old facet.

### Point in an edge interior

Let m be the number of incident cells around the edge in the unified finite+ghost topology. They form
a cyclic star and all conflict.

    C = m
    I = m
    B = 2m
    new cells = 2m

For a hull edge, ghost cells close the incident cycle.

### Exterior point beyond one hull facet

Choose a fixture far enough that the adjacent finite tetrahedron circumsphere does not contain it.

Only the relevant ghost cell conflicts:

    C = 1 ghost
    I = 0
    B = 4

Coning creates one new finite tetrahedron through the old hull facet and three new ghost cells.

### Exact co-spherical insertion

There is no universal cavity-count formula independent of symbolic priority.

Raw InSphere Zero is resolved by the fixed M2 lift-only symbolic contract. The resulting cavity must
still satisfy every manifold/count/transaction invariant.

### Large cavity

For arbitrary connected cavity:
- new cell count = B,
- unified net cell-count change = B - C,
- all boundary facets must produce valid candidate cells,
- all outside-neighbor patches must remain reciprocal.

## InsertionPlan

Conceptual reference data:

    InsertionPlan
    - pointId
    - topologyGeneration
    - locateType
    - locateCell
    - walkPath[]
    - conflictCells[]
    - internalFacets[]
    - boundaryFacets[]
    - candidateCells[]
    - outsideNeighborPatches[]
    - predicateTrace[]
    - symbolicTrace[]
    - validationSummary

No production mutation occurs while the plan is incomplete.

## Pre-commit validation

Mandatory reference checks:

1. query PointId is a valid canonical finite site;
2. locate result agrees with brute-force oracle in reference test mode;
3. conflict set is non-empty for an actual insertion;
4. every conflict handle is current/generation-valid;
5. conflict cells are unique;
6. conflict adjacency graph is connected;
7. every internal facet has exactly two conflict owners;
8. every boundary facet has exactly one conflict owner and one non-conflict outside neighbor;
9. 4C = 2I + B;
10. boundary facet keys are unique;
11. every boundary edge incidence is 2;
12. boundary is connected and Euler characteristic is 2;
13. each finite candidate has four distinct finite sites;
14. each finite candidate has exact non-zero Orient3D;
15. finite candidate ordering can be made positive;
16. every expected new internal face pairs exactly twice;
17. every outside-neighbor patch target is valid and reciprocal;
18. storage required for commit is reserved before destructive mutation.

Any failure rejects the whole plan.

## Commit

The serial reference commit performs deterministic patch mutation:

1. mark/remove conflict cells,
2. allocate/reuse candidate slots,
3. write finite/ghost vertices,
4. connect new-new neighbors by canonical face keys,
5. patch new-old boundary neighbors,
6. advance topology generation/version,
7. invalidate stale old handles.

No geometric predicate makes a new decision during COMMIT. All decisions belong to the validated plan.

## Post-commit validation

Reference path:
- M1 tetra topology validator passes,
- all finite cells have positive orientation,
- no duplicate finite tetra connectivity,
- neighbor reciprocity holds,
- ghost hull is closed/consistent,
- local Delaunay relation holds across affected finite adjacency,
- small-N global empty-sphere oracle passes,
- canonical topology/hull fingerprints regenerate,
- walk locator agrees with brute-force locator on probes.

A post-commit failure is a development error. The failing pre-state and plan must be emitted as replay
evidence.

## Replay extension

M2 replay should preserve enough data to reconstruct a failure without a random seed alone:

    D26DT-REPLAY
    - canonical site raw bits + PointIds
    - insertion order
    - point being inserted
    - topology fingerprint before insertion
    - locate start and walk path
    - exact predicate outcomes/evaluation paths
    - symbolic tie decisions
    - conflict cell canonical connectivity
    - boundary/internal facet keys
    - candidate patch
    - failed invariant / error code


## Global closed-complex checks

The finite hull is a 3-ball and the ghost cone is a second 3-ball. Their shared boundary is the hull
2-sphere, so the unified finite+ghost complex is topologically S^3.

Reference/global checks:

    V - E + F - C = 0
    F = 2C
    E = V + C

where V includes the Infinite vertex and C includes finite+ghost cells.

These checks complement, rather than replace, the local cavity boundary Euler check.

## Local legality after patching

For every internal finite face, first verify geometric embedding: the two opposite finite vertices
must lie on opposite sides of the exact face plane.

Then test local Delaunay legality with exact InSphere. A positive-inside result is illegal. Exact zero
is weakly legal geometrically but must additionally pass the lift-only symbolic tie rule for the
canonical Dynamics26 topology.

For hull faces, small-N validation checks exact convex-hull support. Coplanar adjacent hull triangles
also pass the projected InCircle + symbolic InCircle legality rule.

See LOCAL_CORRECTNESS_AND_SEED_CONTRACT.md.


## Replacement-patch construction rule

The cavity boundary itself decides candidate kind:
- finite boundary triangle -> finite tetra candidate,
- boundary triangle containing Infinite -> ghost candidate.

Do not use sorted boundary-key order as a geometric orientation. Every finite candidate is separately
normalized to positive exact Orient3D.

Each candidate base face is the original cavity boundary and patches exactly one old outside
neighbor. Its three lateral faces contain the inserted point.

Since every boundary edge has incidence two, every lateral face occurs exactly twice and must pair
two candidate cells. This is the reference new-new adjacency proof.

Ghost candidates fix Infinite at local slot 0. Their finite face 0 must pair a finite cell and is
oriented outward using that finite neighbor's opposite vertex.

See PATCH_ORIENTATION_AND_STITCHING.md for the full sign/local-index contract.
