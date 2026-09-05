# M2.0 — Delaunay Reference Architecture

Status: PROPOSED / research freeze candidate
Date: 2026-09-05

## Construction target

The first Dynamics26 unstructured-volume constructor will be a serial, deterministic,
correctness-first incremental 3D Delaunay reference implementation.

The algorithm family is the Bowyer-Watson cavity insertion family described independently by Bowyer
and Watson in 1981. The first implementation is not performance-led: brute-force reference paths
remain enabled until the optimized path agrees with them.

Primary literature:
- A. Bowyer, Computing Dirichlet Tessellations, Computer Journal 24(2), 1981,
  DOI 10.1093/comjnl/24.2.162.
- D. F. Watson, Computing the n-dimensional Delaunay tessellation with application to Voronoi
  polytopes, Computer Journal 24(2), 1981, DOI 10.1093/comjnl/24.2.167.

## Why no numeric super-tetrahedron

A numeric super-tetrahedron introduces artificial extreme coordinates into precisely the part of the
kernel where M1 established exact predicate truth.

M2 therefore does not ask how large an artificial tetrahedron must be or how its artificial vertices
participate in predicate and symbolic identity rules. The unbounded region is represented
topologically.

## Finite + ghost topology

Every convex-hull triangle is incident to one finite tetrahedral cell on the model side and one
ghost/infinite cell on the unbounded side.

The ghost cell contains an auxiliary Infinite topological vertex.

    TopologicalVertex
    - Finite(PointId)
    - Infinite

The infinite vertex:
- has no coordinate,
- is never passed to Orient3D/InSphere,
- is never encoded as a reserved integer PointId,
- does not participate in canonical-site or symbolic-site ordering.

This mirrors the useful combinatorial property documented by CGAL: every facet has two incident
cells and convex-hull special cases become ordinary adjacency cases. This is an architecture lesson,
not a source-code dependency.

## Bootstrap

Input first passes through the qualified M1 canonicalization contract:

    finite validation
    -> exact-coordinate duplicate grouping
    -> signed-zero normalization
    -> deterministic PointId assignment
    -> exact affine-dimension classification

If affine dimension is below 3, M2 returns an explicit lower-dimensional/not-3D result. It does not
invent flat tetrahedra.

For a 3D site set, choose deterministically:
1. the first canonical site,
2. the first distinct canonical site,
3. the first site non-collinear with the first two,
4. the first site non-coplanar with the first three.

Orient the resulting finite tetrahedron positively, create its four ghost neighbors, then insert the
remaining sites.

Symbolic-priority order is fixed by canonical site identity. Insertion order is a separate policy.

## Point location

Two locators are required.

### Brute-force locator

Correctness oracle for small/reference problems:
- test every finite tetrahedron with exact Orient3D sign classification,
- classify CELL/FACET/EDGE/VERTEX/OUTSIDE_CONVEX_HULL,
- never use tolerance for topological classification.

### Deterministic adjacency walk

Main reference path:
- start from a valid hint cell, initially the previous successful insertion neighborhood,
- evaluate the four barycentric Orient3D numerators,
- if all are non-negative for a positively oriented tetra, return exact boundary/interior type,
- otherwise cross a violated opposite facet,
- if several facets are violated, choose the lexicographically smallest canonical facet key.

Walking research by Devillers, Pion and Teillaud is the theory/performance reference; Dynamics26
defines its own deterministic tie and replay policy.

Every early M2 test tier compares walk and brute-force results.

## Conflict discovery

Once a conflicting seed cell is known:

    seed
    -> adjacency flood
    -> conflict cells
    -> internal facets
    -> boundary facets

The conflict region must be connected. Boundary facets separate one conflicting and one
non-conflicting cell.

Conflict semantic branches:
- finite cell: exact/filtered InSphere plus Delaunay symbolic tie,
- ghost cell: hull half-space; on exact coplanarity use deterministic projected InCircle plus
  Delaunay symbolic tie.

## Transactional insertion

Mutation policy:

    LOCATE
    -> DISCOVER CAVITY
    -> EXTRACT BOUNDARY
    -> BUILD CANDIDATE PATCH
    -> VALIDATE MATHEMATICS
    -> VALIDATE TOPOLOGY
    -> RESERVE STORAGE
    -> COMMIT
    -> GLOBAL/LOCAL POSTCHECK

Before COMMIT the existing triangulation is unchanged.

The plan records inserted PointId, locate result and walk path, conflict-cell handles,
internal/boundary facets, outside-neighbor patch targets, candidate finite/ghost cells,
predicate/symbolic decisions and a topology-generation snapshot.

## Adjacency stitching

Each new cell is created by coning the inserted site to one cavity-boundary facet.

Canonical face keys pair lateral faces of new cells. The original boundary facet reconnects the new
cell to the old non-conflict neighbor.

For every finite new tetra:
- geometric vertices must be distinct,
- exact Orient3D must be non-zero,
- stored ordering must be positive.

A zero-volume candidate is a hard transaction failure.

## Fingerprints

Regression identity is independent of cell allocation order.

Finite topology fingerprint:
1. sort the four stable PointId values in every finite tetra,
2. lexicographically sort all tetra records,
3. serialize with a versioned schema,
4. hash only as a convenience; preserve canonical connectivity for human diff.

Hull fingerprint:
- sorted triples of finite PointId values for convex-hull triangles.

Raw cell handles and creation-order TetIds are not regression identities.

## Correctness before locality

M2 reference sequence:

    correct
    -> deterministic
    -> robust on degeneracy
    -> replayable
    -> measured
    -> optimized

Morton/Hilbert/BRIO ordering, hierarchy acceleration, memory packing and parallel insertion are later
experiments. They may not change canonical topology under the fixed symbolic policy.
