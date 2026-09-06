# M2.1-B — Typed Finite/Infinite Bootstrap Implementation Record

**Date:** 2026-09-06  
**Development milestone:** DEV-MESH-P1B  
**Research authority:** M2.0 DESIGN FROZEN  
**Status at commit creation:** IMPLEMENTATION CANDIDATE / EXACT-HEAD QUALIFICATION PENDING

## Scope

This subphase implements the frozen M2.1-B topology foundation only:

- typed `Finite(PointId)` versus `Infinite` vertex domain,
- M2-specific slot/generation cell handle,
- append-only reference arena with no slot reuse,
- deterministic exact affine-basis selection,
- one exact-positive finite tetrahedron,
- four outward-oriented ghost cells,
- finite/ghost and ghost/ghost reciprocal adjacency,
- closed unified-complex validator.

No point location, conflict flood, cavity mutation or serial insertion is introduced.

## Research decisions preserved

- M1 `TetRecord` remains finite-only and unchanged.
- Infinite is not a PointId sentinel and has no coordinate.
- Infinite never enters Orient3D/InSphere or D26LIFT1 priority.
- Ghost Infinite is local vertex 0.
- `neighbor[i]` is opposite `vertex[i]`.
- Positive finite-tetra outward faces use the frozen M2 face table.
- reference storage is append-only/tombstoned; slot reuse is deferred.

Current CGAL public 3D triangulation documentation was rechecked only as an architecture sanity-check:
it likewise models an auxiliary infinite vertex with no meaningful coordinates and opposite-index
neighbor convention. Dynamics26 code remains original and uses the frozen repository contract.

## Bootstrap topology oracle

The minimal 3D bootstrap is combinatorially the boundary of a 4-simplex:

```text
finite cells = 1
ghost cells  = 4
V = 5
E = 10
F = 10
C = 5
V - E + F - C = 0
```

Every triangular face must have exactly two incident unified cells.

## Executable evidence

`unit_m21b_delaunay_bootstrap` checks:

- non-sequential PointIds; no PointId==index assumption,
- deterministic first valid affine basis under input reversal,
- exact-positive stored finite tetra,
- four ghosts with Infinite fixed at slot 0,
- reciprocal finite/ghost and ghost/ghost adjacency,
- 5/10/10/5 S3 incidence statistics,
- explicit Empty/0D/1D/2D outcomes with no cell mutation,
- duplicate PointId / duplicate coordinate rejection,
- stale-generation, wrong ghost orientation and missing-Infinite negative controls.

## Gate intent

This subphase targets:

- M2-G03,
- M2-G04,
- M2-G05.

It also creates executable bootstrap-scope evidence toward M2-G25. M2-G25 is not claimed complete
until the typed validator is exercised across later mutation states.

PASS claims require exact-head macOS arm64 Debug/Release evidence.

## Next subphase

After exact-head qualification:

`DEV-MESH-P1C / M2.1-C — brute-force exact point location`.
