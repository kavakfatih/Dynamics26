# M1.4 Experiment Plan

## EXP-MESH-0140 — Tetra local topology invariants

Create small hand-authored tetra complexes.

Acceptance:
- positive orientation,
- reciprocal neighbors,
- face-key equality,
- stale-handle rejection,
- validator detects every intentionally corrupted case.

## EXP-MESH-0141 — Walk vs exact brute-force location

Generate valid tetrahedralizations and query:
- cell interiors,
- facets,
- edges,
- vertices,
- outside convex hull.

Acceptance:
- typed walk result equals exact brute-force classifier,
- no cycles,
- deterministic step sequence for fixed settings.

## EXP-MESH-0142 — Seed quality

Compare seeds:
- fixed first tetra,
- previous insertion tetra,
- nearest known site incident tetra,
- random deterministic seed.

Metrics:
- mean/p50/p95/max walk steps,
- fallback rate.

## EXP-MESH-0143 — Insertion ordering benchmark

Orders:
- PointId,
- deterministic Morton,
- deterministic Hilbert/Moore candidate,
- BRIO + spatial order,
- seeded shuffle.

Metrics:
- walk steps,
- cavity size,
- cache/performance counters where available,
- total time,
- final topology fingerprint.

Correctness gate:
- same canonical topology under accepted symbolic degeneracy policy.

## EXP-MESH-0144 — Cavity traversal

For each insertion:
- compare BFS/DFS cavity membership against exhaustive insphere scan,
- validate boundary faces,
- ensure each external boundary relation is unique.

## EXP-MESH-0145 — Replacement topology transaction

Inject failures at:
- invalid orientation,
- stale neighbor,
- duplicate boundary face,
- interior pairing failure.

Acceptance:
- original triangulation remains valid after failed insertion.

## EXP-MESH-0146 — Storage/memory profile

Measure:
- bytes/live tetra,
- bytes/site,
- peak temporary cavity memory,
- free-list reuse,
- effect of generation handles.

Run from 1e3 toward largest practical synthetic point sets.

## EXP-MESH-0147 — Optional accelerator decision

After baseline walk telemetry, compare one or more:
- vertex kd-tree seed,
- uniform spatial hash,
- Delaunay hierarchy,
- tetra AABB index.

Decision criterion:
- wall time gain vs memory/update complexity.

No accelerator is adopted merely because asymptotic lookup is better.

## EXP-MESH-0148 — Commercial scalability benchmark

When ANSYS/COMSOL/Marc access is available, run geometrically similar single-domain and multi-domain tetra meshing cases.

Record:
- wall time,
- CPU utilization,
- memory,
- element count,
- thread setting,
- diagnostics.

Purpose:
- product scalability benchmark only.

## EXP-MESH-0149 — Apple Silicon locality benchmark

On target macOS/Apple Silicon:
- compare AoS prototype vs a later SoA experiment,
- spatial order variants,
- stable-handle overhead.

Do not change topology semantics while optimizing layout.
