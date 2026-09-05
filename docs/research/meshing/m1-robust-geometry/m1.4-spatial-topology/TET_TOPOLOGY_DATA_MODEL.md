# Tetra Topology Data Model

## 1. Design goals

The M2 topology store must support:

- frequent cavity deletion,
- frequent local insertion,
- neighbor traversal,
- stale-reference detection,
- deterministic testing,
- compact memory,
- future migration to parallel/SoA layout without changing mathematical semantics.

## 2. Point storage

Conceptual:

```text
Site
- PointId
- Point3 coordinate
- source/provenance metadata outside the hot predicate record
```

The hot Delaunay kernel should avoid dragging Qt/CAD/project metadata through every predicate call.

## 3. Tetra record

Proposed initial semantic record:

```text
TetRecord
- PointId vertex[4]
- TetHandle neighbor[4]
- state / flags
- visitEpoch
```

Invariant:

> neighbor[i] is across the face opposite vertex[i].

This opposite-vertex convention simplifies local face indexing and must be frozen by tests.

## 4. Stable handle

Do not expose raw pointers as long-lived topology identity.

Proposed:

```text
TetHandle
- slot
- generation
```

When a deleted slot is reused:
- its generation increments,
- stale handles fail validation.

This protects against:
- vector reallocation,
- use-after-delete through old cavity references,
- accidental reuse of dead tetra slots.

## 5. Slot store

Prototype:

```text
vector<TetSlot>
freeList
liveCount
```

Each slot:
- generation,
- live/dead,
- TetRecord.

Advantages:
- simple,
- contiguous-ish memory,
- deterministic slot allocation policy can be tested,
- later conversion to structure-of-arrays remains possible.

## 6. Face convention

For tetra vertices:

```text
v0 v1 v2 v3
```

local face `i` is the oriented triangle opposite `vi`.

The orientation table must satisfy:
- positive tetra orientation,
- outward or inward face orientation chosen once,
- neighbor mirror relation has explicit slot mapping.

Never reconstruct orientation by sorting IDs.

## 7. Face identity vs face orientation

Use two different representations.

### CanonicalFaceKey

For equality/hash:

```text
sort(PointId a,b,c)
```

### OrientedFace

For geometry/topology:

```text
(a,b,c)
ownerTet
localFace
```

Sorting destroys orientation, so the two concepts must not be conflated.

## 8. Neighbor invariant

If:

```text
A.neighbor[i] = B
```

then exactly one local face `j` in B must have the same canonical face key and:

```text
B.neighbor[j] = A
```

This is a core topology validation rule.

## 9. Tombstone / deletion

Cavity deletion should not immediately invalidate iteration memory by moving unrelated tetra records.

Preferred prototype:
- mark cavity slots dead,
- detach boundary neighbor links transactionally,
- allocate replacement tetra from free slots/new slots,
- validate complete local ball,
- commit final neighbor relationships.

## 10. Transactional local update

Conceptually:

```text
old local cavity
→ build candidate replacement
→ verify positive orientation
→ verify paired interior faces
→ verify exterior boundary mapping
→ commit
```

A failed insertion should leave the topology in the pre-insertion state.

Implementation may use temporary buffers rather than a full generic transaction framework.

## 11. Epoch marks

Repeated cavity traversals should not allocate a set/hash table for visited tetrahedra if avoidable.

Concept:

```text
globalTraversalEpoch++
slot.visitEpoch = currentEpoch
```

Membership test:
```text
slot.visitEpoch == currentEpoch
```

Wraparound must be handled explicitly.

This is an original design choice based on general graph-traversal practice.

## 12. Memory model research

For each live tetra, rough hot state includes:
- 4 point indices,
- 4 neighbor handles/indices,
- flags/epoch.

If 64-bit handles are used naively, memory grows quickly for tens/hundreds of millions of tetrahedra.

M2 should first favor clarity; M2.x benchmarking then evaluates:
- 32-bit local point indices when safe,
- packed neighbor face encoding,
- SoA vs AoS,
- generation storage cost,
- optional debug-only epochs.

HXT demonstrates how far dedicated compact arrays can scale, but its layout is not a source template.

## 13. Validation API

Required research validator:

- all live tetrahedra positive,
- four valid distinct vertices,
- every live-live neighbor reciprocal,
- shared face keys agree,
- no neighbor points to stale generation,
- no duplicate live tetra connectivity,
- liveCount consistent.

Run this aggressively in Debug/test builds.


## 14. M2.0 resolution note — 2026-09-05

This M1.4 document records the finite-tetra foundation that was later implemented and qualified.

M2.0 subsequently selected explicit ghost/infinite topology. Therefore the finite-only
PointId[4] TetRecord above is not the final M2 cell semantic and must not encode Infinite through
InvalidPointId or a reserved PointId.

Current M2 authority:
docs/research/meshing/m2-delaunay/CELL_STORAGE_AND_MUTATION_MODEL.md.

M1 TetHandle/TetRecord remains valid evidence for finite topology primitives and stale-handle design;
the M2 cell layer extends the concept with typed finite/infinite vertices.
