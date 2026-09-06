# M2.1-B — Typed Finite/Infinite Bootstrap Implementation Record

**Date:** 2026-09-06  
**Development milestone:** DEV-MESH-P1B  
**Research authority:** M2.0 DESIGN FROZEN  
**Status:** QUALIFIED IN P1B BOOTSTRAP SUBSCOPE  
**Source:** `6c626f3884aeedcda727f93fc1a4b174cad12d51`  
**CI:** [macOS arm64 #299](https://github.com/kavakfatih/Dynamics26/actions/runs/34038076186), completed/success; Debug/Release 159/159 CTest, gui-fast success.  
**Gates:** M2-G03/G04/G05 PASS; M2-G25 bootstrap-only PARTIAL.

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


## Independent P1B audit and hardening — 2026-09-06

Baseline `d2163476bd52d085e2efc0680ec1e5b4899b8460` passed existing tests
(macOS workflow 34036613946; Debug 159/159). Independent negative controls
nevertheless reproduced three validator defects:

1. Two ghost lateral faces replaced with reciprocal **self** links were accepted.
   Require the two actual face-incidence owners to point at each other.
2. Two copies of one finite tetra with reciprocal links passed all incidence
   counts and Euler=0. Reject duplicate canonical cell connectivity explicitly.
3. A missing finite-neighbor opposite PointId threw during ghost orientation
   lookup after already being classified as invalid. Return MissingFinitePoint
   evidence without dereferencing an absent coordinate.

The ghost-orientation negative test now swaps the corresponding neighbor slots
too and requires **only** GhostHullOrientationInvalid. It can no longer pass
merely because adjacency was broken. Corruption tests check specific typed codes;
new coverage includes invalid Infinite patterns, dead/out-of-range/zero-generation
slots, direct stale arena handle access, near-maximum PointIds, scales 2^-500/1/2^500
and independent enumeration of every actual two-owner reciprocal face.

The constructor's one-finite/four-ghost output remains unchanged. M1 and public
installed headers remain unchanged. P1B hardening source qualification passed its own exact-head macOS arm64
Debug/Release workflow #299, identified above.

**Qualification boundary:** Euler and face incidence are necessary corruption
checks, not a proof that an arbitrary complex is S³. P1B constructs the explicit
boundary-of-a-4-simplex bootstrap; general embedding, links and mutation-state
qualification are later obligations. M2-G25 remains bootstrap-only PARTIAL;
M2 OVERALL = NOT QUALIFIED. No cavity or walk is introduced here.
