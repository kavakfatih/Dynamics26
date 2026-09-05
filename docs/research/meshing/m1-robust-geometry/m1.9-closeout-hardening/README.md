# M1.9 — Closeout Hardening

**State:** ACTIVE  
**Parent:** M1 Robust Geometry Foundation  
**Rule:** M2 remains blocked until M1.9 and the final M1 re-audit pass.

## Workstreams

### A — Replay and adversarial corpus

- failure replay schema + executable round-trip,
- near-collinear/coplanar/cocircular/cospherical generated families,
- permutation/scale/translation metamorphic tests.

### B — Degeneracy foundation

- canonical site construction,
- signed-zero normalization,
- affine dimension,
- stable PointId,
- test-only formal symbolic perturbation oracle.

### C — Tetra primitives

- generation-checked TetHandle,
- opposite-face neighbor convention,
- canonical/oriented face separation,
- local topology validator,
- corruption tests.

### D — Predicate telemetry

- fast/fallback/exact-zero counters,
- deterministic baseline,
- Release performance observation.

### E — Closeout synchronization

- implementation specs,
- ADR status,
- roadmap states,
- final exact-head audit.

## Explicit non-goals

M1.9 does not implement:
- Bowyer-Watson insertion,
- Delaunay cavity retriangulation,
- production point-location walk,
- CAD surface meshing,
- boundary recovery.

Those remain M2+ only after M1 qualification.
