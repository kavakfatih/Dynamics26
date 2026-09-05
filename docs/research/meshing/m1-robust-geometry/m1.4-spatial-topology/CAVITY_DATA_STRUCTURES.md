# Cavity Data Structures

## 1. Bowyer-Watson local update

For insertion site `p`:

```text
locate seed tetra
→ find all tetra whose circumsphere contains p
→ this connected set is the cavity
→ extract cavity boundary
→ delete cavity tetra
→ connect p to every boundary face
→ reconnect new tetrahedra
```

## 2. Cavity traversal

Leading prototype:
- BFS or DFS through tetra neighbors,
- `insphere` decides cavity membership,
- epoch mark prevents revisits.

Conceptual buffers:

```text
cavityTets[]
boundaryFaces[]
newTets[]
```

Buffers should be reused between insertions to reduce allocator noise.

Gmsh/HXT source studies show reusable local cavity buffers as a mature performance pattern, but Dynamics26 defines its own records and ownership.

## 3. Boundary-face record

Proposed:

```text
CavityBoundaryFace
- OrientedFace face
- TetHandle outsideNeighbor
- localFaceInOutsideNeighbor
```

The face orientation is chosen so a new tetra:
```text
(face vertices + inserted point)
```
can be oriented positively with one deterministic mapping.

## 4. Boundary extraction invariant

A face belongs to the cavity boundary iff:
- its owner tetra is in the cavity,
- the neighbor across the face is not in the cavity, or it is a hull boundary.

Do not discover boundary by global face hashing if neighbor topology is valid.

Global/canonical face hashing remains useful for:
- topology validation,
- initial adjacency construction,
- debug cross-check.

## 5. New-tetra adjacency

Each boundary face generates one new tetra.

External neighbor:
- known from `CavityBoundaryFace`.

New-new neighbors:
- paired through the three faces containing the inserted point.

Candidate prototype method:
- construct a canonical key for each new interior face,
- hash/sort keys,
- pair exactly twice.

This is easy to validate.

Later optimization can exploit boundary-edge connectivity.

## 6. Star-shaped cavity

For ordinary Bowyer-Watson on a correct Delaunay triangulation with consistent predicates, the cavity associated with a point has the needed retriangulation structure.

Nevertheless, implementation must validate:
- each new tetra has positive orientation,
- no duplicate boundary face,
- every new interior face pairs exactly twice,
- external boundary neighbor faces pair exactly once.

If not, insertion fails with a typed topology error.

## 7. Strong failure semantics

Potential failure reasons:

```text
PointLocationFailed
InvalidSeedTopology
NonManifoldCavityBoundary
DegenerateNewTetra
MissingExteriorNeighbor
InteriorFacePairingFailure
StaleTetHandle
PredicateInvalidInput
```

Never continue after local topology invariants fail.

## 8. Rollback

A failed insertion must not leave half-updated neighbors.

Safe prototype sequence:

1. identify cavity/boundary without mutation,
2. construct replacement connectivity in temporary records,
3. validate replacement,
4. detach old cavity,
5. install new tetra,
6. connect outside neighbors,
7. run local validator.

If implementation mutates earlier for performance later, it needs explicit rollback.
