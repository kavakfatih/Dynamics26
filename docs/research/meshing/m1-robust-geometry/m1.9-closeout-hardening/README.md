# M1.9 — Closeout Hardening

**State:** VERIFYING — EXECUTABLE BLOCKERS CLOSED; SECOND FINAL AUDIT READY  
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
- local combinatorial topology validator,
- malformed invalid-handle rejection,
- shared-face adjacency completeness check,
- non-manifold face detection,
- deliberate corruption tests.

Evidence: base primitive workflow #238 SUCCESS; hardening commit `d6501f1d...` exact-head workflow #241 SUCCESS.

### D — Predicate telemetry

- fast/fallback/exact-zero counters,
- deterministic baseline,
- Release performance observation.

### E — Closeout synchronization

- implementation specs synchronized,
- ADR scope synchronized,
- roadmap states synchronized,
- candidate synchronization `d007fca6...` workflow #240 SUCCESS,
- topology-hardening `d6501f1d...` workflow #241 SUCCESS,
- second final exact-head audit is the remaining gate.

## Explicit non-goals

M1.9 does not implement:
- Bowyer-Watson insertion,
- Delaunay cavity retriangulation,
- production point-location walk,
- CAD surface meshing,
- boundary recovery.

Those remain M2+ only after M1 qualification.
