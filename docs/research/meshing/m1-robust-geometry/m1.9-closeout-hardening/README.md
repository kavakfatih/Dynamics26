# M1.9 — Closeout Hardening

**State:** QUALIFIED  
**Parent:** M1 Robust Geometry Foundation  
**Rule:** M1.9 is closed by the second/final audit; M2 starts with M2.0 only after the final closeout status commit itself is exact-head CI green.

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
- documentation synchronization `6e939eb6...` workflow #242 SUCCESS,
- second/final audit G01–G20 PASS, blocker count 0,
- final closeout status commit exact-head CI is the remaining operational authorization check.

## Explicit non-goals

M1.9 does not implement:
- Bowyer-Watson insertion,
- Delaunay cavity retriangulation,
- production point-location walk,
- CAD surface meshing,
- boundary recovery.

Those remain M2+ only after M1 qualification.
