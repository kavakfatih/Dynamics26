# M2.0 — Patch Orientation and Neighbor Stitching Contract

Status: PROPOSED / local-retriangulation derivation
Date: 2026-09-05

## 1. Purpose

Bowyer-Watson cavity extraction is not complete until Dynamics26 can turn the cavity boundary into a
fully oriented finite+ghost replacement patch without relying on accidental vertex order.

This document fixes:
- how each cavity-boundary facet determines the kind of new cell,
- how finite-cell orientation is normalized,
- how ghost-cell hull orientation is normalized,
- why new-new lateral faces pair exactly twice,
- how new-old outside neighbors are patched without an orientation guess.

## 2. Boundary-facet type determines candidate-cell type

Every valid M2 boundary facet contains either:
- three Finite(PointId) vertices, or
- one Infinite vertex and two Finite(PointId) vertices.

A valid boundary facet cannot contain two Infinite vertices because every valid ghost cell contains
exactly one Infinite vertex.

For inserted finite site p:

### Finite boundary facet

    F = {a,b,c}

Cone:

    F * p = {a,b,c,p}

Candidate kind: finite tetrahedron.

### Infinite boundary facet

    F = {Infinite,a,b}

Cone:

    F * p = {Infinite,a,b,p}

Candidate kind: ghost cell.

Thus no heuristic decides finite versus ghost replacement. The boundary topology decides it.

## 3. Do not make boundary key order carry geometry orientation

Cavity membership and identity use a canonical DelaunayFaceKey. Sorting vertices is correct for
equality but destroys geometric orientation.

Therefore:

    canonical boundary key
    != geometric orientation

The replacement planner may preserve an oriented finite face as diagnostic/derivable information, but
finite candidate orientation is independently certified from exact Orient3D.

This prevents a future change in owner-cell local ordering from changing geometric correctness.

## 4. Finite candidate orientation

For finite boundary facet with finite sites a,b,c and inserted p, construct the four-site candidate.

Evaluate exact/certified:

    O(a,b,c,p).

- if Positive: one valid positive ordering is (a,b,c,p),
- if Negative: swap any two face vertices, for example (a,c,b,p),
- if Zero: hard PlanValidationFailure / DegenerateNewTetra.

The chosen stored order must satisfy:

    orient3d(v0,v1,v2,v3) == Positive.

No epsilon is allowed.

## 5. Why a correct cavity should not produce a flat finite boundary cone

The Bowyer-Watson conflict hole is star-shaped with respect to the inserted site.

An old facet/edge that actually contains the inserted point belongs to the interior of the full
conflict star:
- an internal finite facet point causes both incident finite cells to conflict,
- a hull facet point causes the finite and incident ghost cell to conflict,
- an edge point causes its whole incident unified cell star to conflict.

Therefore the facet/edge containing p is not supposed to remain as a finite cavity-boundary triangle.

A finite boundary triangle yielding exact Orient3D Zero with p is consequently a strong diagnostic of:
- incomplete conflict discovery,
- wrong ghost conflict semantics,
- invalid pre-existing topology,
- or an unsupported degeneracy not covered by the fixed symbolic policy.

The reference implementation still checks Zero explicitly; theory is not a substitute for validation.

## 6. Candidate face classes

Every new cell has one face opposite inserted p:

    base face = original cavity-boundary facet.

That face reconnects to the old non-conflict outside neighbor.

The other three faces contain p and one boundary edge. Call them lateral faces.

Therefore:
- base faces are new-old interfaces,
- lateral faces are new-new interfaces.

This decomposition is independent of finite versus ghost cell.

## 7. Why every lateral face pairs exactly twice

The validated cavity boundary is a closed triangulated 2-sphere.

Every boundary edge has incidence exactly two.

Let boundary edge E belong to boundary facets F1 and F2. Coning both facets to p produces two
candidate cells. Both contain the same lateral face:

    E union {p}.

No third boundary facet contains E.

Therefore each candidate lateral face has incidence exactly two.

This gives the direct topological proof behind the implementation invariant:

    every new-new lateral DelaunayFaceKey occurs exactly twice.

The proof also covers edges containing Infinite, because the boundary 2-sphere is the unified
finite+ghost complex.

## 8. New-new pairing algorithm

Reference planner:

1. for every candidate cell, enumerate its three lateral faces,
2. compute canonical DelaunayFaceKey for each,
3. insert into a deterministic map/sorted list,
4. require exactly two owners for every key,
5. write the reciprocal candidate-neighbor relationship into the plan.

Any key with incidence 1 or >2 rejects the plan before commit.

This favors auditability over a more specialized edge-walk pairing optimization.

## 9. New-old boundary patching

Each CavityBoundaryFacet already records:
- canonical base-face key,
- conflict owner,
- outside non-conflict neighbor,
- local face index in the outside neighbor or enough information to verify it.

After candidate vertex normalization, find the candidate local face whose canonical key equals the
original boundary key.

That candidate face must be unique.

Plan:

    candidate.neighbor[candidateBaseFace] = outsideNeighbor
    outsideNeighbor.neighbor[outsideLocalFace] = candidateHandle

The outside-neighbor write is not executed until commit.

This key-based resolution avoids assuming that the inserted point remains at a particular finite
local vertex after positive-orientation normalization.

## 10. Ghost candidate local normalization

A ghost candidate produced from {Infinite,a,b} has finite vertices {a,b,p}.

M2 storage requires:

    ghost.vertex[0] = Infinite.

Its face opposite slot 0 is therefore the new finite hull triangle.

Before commit, lateral-face pairing must identify the finite candidate/old finite cell across this
hull triangle.

Let q be that finite neighbor's vertex opposite the shared hull triangle.

Store the ghost finite triple in the order (h0,h1,h2) satisfying:

    O(h0,h1,h2,q) < 0.

Thus the finite triple is outward-oriented from the convex hull, consistent with the ghost conflict
contract.

If:
- no unique finite neighbor exists across ghost face 0,
- q cannot be identified,
- or Orient3D is Zero,

the plan is invalid.

## 11. Ghost base and lateral faces

For normalized ghost:

    vertex[0] = Infinite
    vertex[1..3] = outward hull triangle

- face 0 is finite hull triangle and must neighbor a finite cell,
- faces 1..3 contain Infinite and two finite vertices and must neighbor ghost cells.

This becomes a validator rule.

A ghost-ghost adjacency across a finite face or a finite-finite adjacency across a face containing
Infinite is invalid.

## 12. Finite candidate face orientation after reordering

Finite stored-cell outward face orientation follows CELL_STORAGE_AND_MUTATION_MODEL.md:

    face 0: (v1,v2,v3)
    face 1: (v0,v3,v2)
    face 2: (v0,v1,v3)
    face 3: (v0,v2,v1)

for positive (v0,v1,v2,v3).

This oriented table is used for:
- exact face-side checks,
- hull support when the face is a hull face,
- local embedding diagnostics.

Canonical face keys remain the adjacency identity.

## 13. Commit-order implication

All of the following are completed before the commit barrier:
- finite/ghost candidate classification,
- finite positive orientation,
- candidate local face indices,
- lateral face pairing,
- outside-neighbor patch targets,
- ghost face-0 finite-neighbor identification,
- ghost outward hull orientation,
- exact non-zero checks.

Therefore COMMIT performs only prevalidated writes.

No Orient3D, InSphere, InCircle or symbolic tie is evaluated after the commit barrier.

## 14. Local patch count proof

Let B be cavity boundary triangle count.

Coning each boundary triangle to p creates exactly B candidate cells.

Because every boundary edge occurs twice:
- all lateral faces pair internally,
- every candidate base face remains exactly one new-old interface.

Thus the replacement patch is a triangulated 3-ball with the same boundary as the removed conflict
3-ball.

This is the topological core of transactional replacement.

## 15. Reference verification records

For every insertion, reference diagnostics may record:

    boundaryFacetCount
    finiteBoundaryFacetCount
    infiniteBoundaryFacetCount
    finiteCandidateCount
    ghostCandidateCount
    lateralFaceKeyCount
    lateralPairCount
    outsidePatchCount
    orientationSwaps
    ghostOrientationSwaps

Required identities:

    candidateCount = boundaryFacetCount
    finiteCandidateCount = finiteBoundaryFacetCount
    ghostCandidateCount = infiniteBoundaryFacetCount
    lateralFaceOwnerIncidence = 2 for every lateral key
    outsidePatchCount = boundaryFacetCount.

Counts are diagnostics and invariants, not performance targets.
