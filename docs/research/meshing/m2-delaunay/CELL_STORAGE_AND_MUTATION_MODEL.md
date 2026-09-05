# M2.0 — Delaunay Cell Storage and Mutation Model

Status: PROPOSED / reference-storage contract
Date: 2026-09-05

## 1. Why M1 TetRecord is not silently extended

M1 qualified a finite tetra topology primitive:

    TetRecord
    - PointId vertices[4]
    - TetHandle neighbors[4]

That record intentionally assumes every vertex is a real canonical site.

M2.0 has now chosen finite+ghost topology. A ghost cell contains one topological Infinite vertex
that:
- is not a PointId,
- has no coordinate,
- cannot enter a geometric predicate.

Encoding Infinite as InvalidPointId or UINT64_MAX would make a semantic category look like an
ordinary site identifier and create a route for accidental coordinate lookup/predicate use.

Therefore M1 TetRecord remains a qualified finite-only primitive. M2 introduces a typed cell layer
rather than redefining the meaning of M1 data.

## 2. Conceptual vertex reference

Leading semantic type:

    DelaunayVertexRef
    - Finite(PointId)
    - Infinite

Required invariants:
- Finite contains a valid nonzero canonical PointId,
- Infinite carries no numeric site identity,
- predicates accept only finite coordinates,
- symbolic priority accepts only Finite(PointId),
- canonical topological keys may contain Infinite but use a separate comparator.

The ordering used for topological keys is not the symbolic geometric priority.

A simple fixed key order such as

    Finite(id) < Infinite

is sufficient for hashing/sorting; Infinite never participates in epsilon priority.

## 3. Conceptual cell record

    DelaunayCellRecord
    - DelaunayVertexRef vertex[4]
    - DelaunayCellHandle neighbor[4]
    - visitEpoch
    - state/flags only if derivable state cannot be avoided

A valid cell is exactly one of:

### Finite cell
- four Finite vertices,
- four distinct PointIds,
- exact positive Orient3D.

### Ghost cell
- exactly one Infinite vertex,
- exactly three distinct Finite vertices,
- no Orient3D/InSphere call on the four-entry record.

Cells with two or more Infinite vertices are invalid in the 3D M2 topology.

## 4. Freeze the ghost local convention

For a ghost cell, place Infinite at local vertex 0:

    ghost.vertex[0] = Infinite
    ghost.vertex[1..3] = hull triangle

Then:

    ghost.neighbor[0]

is the finite cell across the hull triangle.

Neighbors 1..3 are ghost cells across faces that contain Infinite and a hull edge.

This removes a large class of local-index special cases.

The finite triple vertex[1..3] is stored in outward hull orientation.

## 5. Outward finite-face table

For a positive finite tetrahedron (v0,v1,v2,v3), use the fixed outward face orientation:

    face 0: (v1,v2,v3)
    face 1: (v0,v3,v2)
    face 2: (v0,v1,v3)
    face 3: (v0,v2,v1)

With the current Dynamics26 Orient3D convention, the opposite tetra vertex lies on the negative side
of each outward-oriented face.

This table is an executable fixture requirement. Do not reconstruct geometric orientation by sorting
PointIds.

## 6. Identity key versus oriented face

Two face representations remain mandatory.

### DelaunayFaceKey
For equality, pairing, maps and replay:
- three DelaunayVertexRef values,
- sorted by the fixed topological comparator.

### OrientedFiniteFace
For geometry:
- exactly three Finite PointIds in oriented order,
- owner cell/local face,
- safe to pass to Orient3D/projected InCircle logic.

A face containing Infinite has no 3D geometric plane and cannot become OrientedFiniteFace.

## 7. Handle domain

Long-lived references must not be raw pointers.

M1 proved slot+generation semantics useful. M2 should retain the concept, but the handle type belongs
to the M2 cell arena so a finite-only M1 TetSlot cannot be confused with a finite/ghost Delaunay
cell slot.

Conceptually:

    DelaunayCellHandle
    - slot
    - generation

A handle is valid only when:
- slot is in range,
- slot is live,
- stored generation matches,
- generation is nonzero.

## 8. Reference arena: append-only first

For M2.1 correctness qualification, slot reuse is deliberately disabled.

Reference storage:

    vector<DelaunayCellSlot>
    liveCount
    traversalEpoch

Commit behavior:
- reserve capacity for all candidate cells before destructive mutation,
- append candidate cells,
- patch adjacency,
- mark old conflict cells dead at the commit barrier/order chosen by the implementation,
- never recycle a dead slot in M2.1.

Benefits:
- no free-list ordering affects debugging,
- stale handles point to dead slots rather than newly unrelated cells,
- candidate handles can be determined before patching,
- commit can be engineered as a no-allocation/no-predicate phase,
- replay is easier to inspect.

This is a correctness/reference decision, not the final memory architecture.

## 9. Why append-only is acceptable initially

M2.1 is not the large-production mesher.

Its purpose is to establish:
- exact construction,
- determinism,
- failure semantics,
- independent oracles.

Dead-slot growth is measured as telemetry.

After reference qualification, deterministic slot reuse becomes an optimization experiment and must
prove:
- identical canonical finite/hull fingerprints,
- identical mathematical predicate decisions,
- stale-handle protection,
- no change in replay semantic identity.

## 10. Later deterministic reuse

Candidate later policy:
- reclaim only cells from a fully committed old cavity,
- increment generation on reuse,
- generation zero is forbidden,
- deterministic free-slot choice independent of hash iteration,
- on generation exhaustion, fail explicitly or retire the slot; never wrap silently.

The exact free-list structure is not frozen by M2.0.

## 11. Commit barrier

Before commit:
- candidate records live in temporary plan storage,
- all predicates have already been evaluated,
- all new-new face pairings are known,
- all new-old outside patch targets are generation-validated,
- destination vector capacity is reserved.

After crossing the commit barrier:
- no geometric decision is allowed,
- no symbolic decision is allowed,
- no operation expected to allocate memory should remain,
- writes occur in deterministic order,
- post-commit validation may diagnose a development bug but does not invent recovery topology.

## 12. Epoch marks

Traversal epoch is an optimization for set membership, not semantic identity.

Rules:
- epoch values never enter fingerprints,
- wraparound is explicit,
- if increment would collide with the reset sentinel, clear visit marks in a controlled pass before
  continuing,
- replay correctness cannot depend on the absolute epoch number.

## 13. Finite/ghost validators

Reference validator extensions:

Cell validity:
- finite cell: 4 finite distinct vertices + positive Orient3D,
- ghost: Infinite only at local slot 0 + three finite distinct vertices,
- no other infinite multiplicity.

Adjacency:
- every face has exactly two incident unified cells,
- neighbor[i] is opposite vertex[i],
- shared DelaunayFaceKey matches,
- neighbor relation reciprocal.

Hull:
- every ghost neighbor[0] is finite,
- every finite hull face has exactly one ghost across it,
- ghost faces 1..3 pair with ghost cells,
- ghost count equals hull-triangle count.

Global:
- unified S^3 incidence/Euler contracts in LOCAL_CORRECTNESS_AND_SEED_CONTRACT.md.

## 14. Performance boundary

M2.0 does not freeze:
- AoS versus SoA,
- packed neighbors,
- 32-bit local point indices,
- cache-line layout,
- parallel ownership,
- lock-free insertion,
- compact production free-list.

Those require measurement after the reference topology is qualified.

The mathematical topology, handle validity, opposite-face convention and typed Infinite semantic must
survive any later storage rewrite.
