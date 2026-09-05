# M1.4 — Spatial Search, Point Location & Tetra Topology

**Program:** Dynamics26 Original Meshing System R&D  
**Parent:** M1 — Robust Geometry Foundation  
**Depends on:** M1.1 exact oracle, M1.2 certified filters, M1.3 degeneracy policy  
**State:** RESEARCHING  
**Baseline:** 2026-09-05  
**Production code:** none

## Objective

Define the data structures and search contracts required by the M2 incremental 3D Delaunay / Bowyer-Watson prototype.

M1.4 separates four problems:

1. tetrahedral combinatorial storage,
2. point location,
3. cavity traversal/retriangulation support,
4. spatial ordering / optional acceleration.

## Core architecture

```text
Stable meshing sites
        ↓
Spatial insertion ordering
        ↓
PointLocation
        ↓
seed tetrahedron
        ↓
Delaunay cavity traversal
        ↓
boundary extraction
        ↓
local retriangulation
        ↓
updated topology
```

## Leading design

Initial M2 should start simple and deterministic:

- serial algorithm,
- compact index-based tetra storage,
- stable generation-checked handles,
- 4 vertices + 4 opposite-face neighbors,
- exact/certified orientation for walking,
- neighbor-walk point location with a good seed/hint,
- deterministic spatial insertion ordering,
- explicit slow fallback for correctness,
- no mandatory dynamic BVH/kd-tree in the first prototype.

A secondary point-location accelerator is added only if experiments justify its memory and maintenance cost.

## Documents

- `POINT_LOCATION_THEORY.md`
- `TET_TOPOLOGY_DATA_MODEL.md`
- `SPATIAL_ORDERING_AND_SEARCH.md`
- `CAVITY_DATA_STRUCTURES.md`
- `COMMERCIAL_OPEN_SOURCE_COMPARISON.md`
- `EXPERIMENT_PLAN.md`
- `M1_4_DECISION_SPEC.md`

## M1.4 exit

Research can move to M2 implementation when:

- tetra local-face convention is frozen,
- handle lifetime/stale-reference policy is frozen,
- point-location result states are frozen,
- seed/hint and fallback policy are frozen,
- cavity boundary record is frozen,
- insertion-order experiments are specified,
- no external data-structure implementation is required by the design.
