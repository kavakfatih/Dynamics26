# M2.1-E — Deterministic Adjacency Walk Implementation Record

**Date:** 2026-09-07  
**Development milestone:** DEV-MESH-P1E  
**Identity:** M2.1-E — Deterministic Adjacency Walk  
**Research authority:** M2.0 DESIGN FROZEN  
**Primary gate:** M2-G07 — deterministic walk agrees with brute-force oracle  
**Status:** QUALIFIED IN DECLARED DETERMINISTIC-WALK SUBSCOPE  
**M2 OVERALL:** NOT QUALIFIED

## 1. Starting baseline and source identity

P1E began from the fully closed P1D documentation/current baseline:

- starting `main`: `905125610d254bb05ff74dd3532cef448988f3d1`,
- starting commit: `docs: close m2 p1d final qualification`,
- starting exact-head macOS arm64 workflow: #314, SUCCESS,
- P1D final evidence source: `f5ab7e8dfcad9e6cd731722c2a1cc9c47c26f915`.

P1E source/test implementation:

- source SHA: `bb232ed3f308375271af07a100adee531444d499`,
- commit: `feat: add deterministic m2 adjacency walk`,
- exact-head macOS arm64 workflow: [#315](https://github.com/kavakfatih/Dynamics26/actions/runs/34085624974), SUCCESS.

Workflow #315 exact-source evidence:

- Debug full CTest: **162/162 PASS**,
- Release full CTest: **162/162 PASS**,
- Debug meshing-labelled CTest: **39/39 PASS**,
- Release meshing-labelled CTest: **39/39 PASS**,
- P1A/P1B/P1C/P1D/P1E: PASS Debug + Release,
- dedicated direct P1E qualification step: SUCCESS Debug + Release,
- `gui-fast`: SUCCESS,
- `gui-bundle-audit`: SKIPPED,
- repository/reproducible-source integrity: Debug SUCCESS / Release SKIPPED,
- M1 predicate telemetry baseline: Debug SKIPPED / Release SUCCESS.

Release M1 predicate telemetry remained:

```text
cases=1049
calls=1051
fast=1043
exact=6
zero=4
invalid=2
mismatch=0
```

No separate local build/test execution is claimed by this record; the qualification authority is the native macOS arm64 exact-head workflow.

## 2. Frozen authority

P1E preserves `EXPERIMENT_PLAN.md`, `LOCAL_CORRECTNESS_AND_SEED_CONTRACT.md`,
`DETERMINISM_SCOPE_AND_POLICY_VERSIONING.md`, `DELAUNAY_MATHEMATICS.md`,
`CELL_STORAGE_AND_MUTATION_MODEL.md`, `CAVITY_TRANSACTION_SPEC.md`,
`M2_0_REFERENCE_ARCHITECTURE.md`, `M2_1C_IMPLEMENTATION_RECORD.md` and
`M2_1D_IMPLEMENTATION_RECORD.md`.

`locateBruteForceExact(...)` remains unchanged as the independent P1C correctness oracle. P1E never consults it to choose a walk facet or turn a failed walk into success.

## 3. Private implementation architecture

P1E adds `DelaunayWalk.h/.cpp` and the dedicated `unit_m21e_delaunay_walk` target.
The private result model separates `DelaunayWalkStatus`, optional `DelaunayLocationResult`,
optional `DelaunayConflictSeed`, `DelaunayWalkTrace`, optional stalled-walk diagnostic oracle and non-semantic telemetry.
No installed/public meshing API changes.

## 4. Deterministic start-cell policy

The no-hint path chooses the lexicographically smallest canonical finite-cell connectivity identity.
This is a query-independent identity scan, not a geometric containment scan. It uses no query predicate.
First slot, slot zero, smallest raw handle, pointer ordering and unordered iteration are not authorities.

A valid optional hint must be a current live finite cell. Stale/dead/out-of-range/ghost hints return typed
`InvalidStartHint` with no hidden fallback. Qualification proves different valid hints produce the same semantic result.

## 5. Walk mathematics

P1E reuses `outwardFiniteFace(...)`. For a positive finite tetra, the opposite finite vertex satisfies:

```text
Orient3D(outward face, opposite finite vertex) < 0
```

Thus query sign is Negative=inside, Zero=exact boundary, Positive=strict violation. Only Positive facets are crossed.
Zero is never a progress direction. No epsilon or D26LIFT1 symbolic tie enters visibility crossing.

When several facets are strictly violated, the chosen crossing is the minimum `canonicalDelaunayFaceKey`,
not local face number or storage order. The tie fixture repeats after an even local face/vertex permutation and chooses the same canonical facet.

## 6. Finite adjacency and outside transition

Chosen neighbors must be current and reciprocal on the same canonical face. finite→finite continues the walk.
finite→ghost returns OUTSIDE only when the ghost finite face is local face 0 and exactly matches the crossed facet.
The returned witness keeps the exact outward violated face and reciprocal finite/ghost handles; the Outside conflict seed is that crossed ghost.

P1C chooses the globally smallest canonical violated hull face, while the frozen walk contract requires the actually crossed verified hull witness.
No frozen document requires witness identity equality. G07 OUTSIDE agreement is therefore same OUTSIDE classification plus independently exact,
reciprocal and storage-deterministic walk witness. No ADR/freeze revision was required.

## 7. Exact contained classification

If no facet is Positive, the query is contained. Negative face signs correspond to positive barycentric weights.
Their opposite PointIds form the same minimal-simplex identity as P1C:

- 4 Negative: CELL,
- 3: FACET,
- 2: EDGE,
- 1: VERTEX.

Zeros preserve boundary dimension. Incident finite/ghost cells are gathered by local reciprocal adjacency restricted to cells containing the entity.

## 8. ConflictSeed semantics

- CELL -> containing finite seed / `CellInterior`.
- FACET -> canonical-minimum incident finite seed / `FacetIncident`.
- EDGE -> canonical-minimum incident finite seed / `EdgeIncident`.
- VERTEX -> no insertion seed.
- OUTSIDE -> crossed exact violated ghost / `OutsideGhost` with witness facet.

Location and conflict seed remain separate concepts.

## 9. Walk safety and termination

Full `validateDelaunayTopology(...)` runs before the correctness-first walk.
Trace records start, visited finite cells, crossed canonical faces and a topology-sized step guard.
The normal guard is the live finite-cell count. Repeat, stale traversal neighbor, reciprocity failure or guard exhaustion is `WalkStalled`.

Only after `WalkStalled` is fixed as the walk status does P1C run for diagnosis; its answer is stored separately in `diagnosticOracle`.
A qualification-only zero guard proves a failed walk stays failed even though the diagnostic oracle returns a valid CELL answer.

## 10. Read-only semantics

The locator receives const spans and uses local traversal state. It does not mutate `visitEpoch`, live/dead state,
generations, vertices or neighbors. Exact state fingerprints before/after the grid corpus and negative controls are identical.

## 11. Independent oracle methodology

For every accepted query, test code runs:

```text
walk = locateDeterministicWalk(...)
oracle = locateBruteForceExact(...)
```

For contained results it compares kind, sorted entity PointIds and sorted canonical incident-cell identities.
For OUTSIDE it compares classification and independently proves the walk witness strict violation and finite/ghost reciprocity.
Analytic fixtures provide a third evidence layer.

## 12. Qualification corpus and telemetry

The target `unit_m21e_delaunay_walk` has labels:
`unit;meshing;m2;delaunay;location;walk;determinism`.

Coverage includes CELL/FACET/EDGE/VERTEX/OUTSIDE, shared and hull facets, hull edge,
exact-zero no-crossing, one-ULP both sides, multi-violation canonical tie, local-face permutation,
reverse slot/storage and site enumeration, multiple valid starts, stale hint, invalid reciprocity,
explicit WalkStalled negative control, binary scale range and near-coplanar exact-nonzero geometry.

Direct runtime telemetry was identical in Debug and Release on #315:

```text
checks=11627
queries=1093
successes=1093
stalls=0
mismatches=0
walk steps=1461
max walk steps=2
CELL/FACET/EDGE/VERTEX/OUTSIDE=10/46/55/11/971
```

The explicit negative-control stall is separate from the accepted-corpus `stalls=0` metric.

## 13. Determinism scope

Executable evidence proves input/site enumeration, cell-slot permutation, default-start selection,
local face enumeration and legal start hints do not change the P1E semantic result.
Debug and Release qualification telemetry are identical. This is P1E walk determinism, not final constructor fingerprint determinism.

## 14. Gate verdict

| Gate | Status | Evidence |
| --- | --- | --- |
| M2-G06 | **PASS — existing P1C** | unchanged brute-force exact location oracle remains green |
| M2-G07 | **PASS** | strict/zero/tie/outside/stall/read-only/storage-order evidence; 1093 accepted queries, zero stalls/mismatches, Debug+Release #315 |
| M2-G25 | **PARTIAL** | P1E adds topology-state evidence; full serial-constructor state family remains P1F |
| M2-G20 | **DEFERRED** | P1E telemetry does not close complete serial constructor qualification |

M2-G16..G24 and M2-G30..G33 remain deferred to P1F/final-constructor qualification.

## 15. Qualification boundary

After the documentation/current-status commit receives its own exact-head CI, the authorized claim is:

```text
DEV-MESH-P1E
M2.1-E
Deterministic Adjacency Walk
QUALIFIED IN DECLARED SUBSCOPE
```

This is deterministic walk agreement against P1C in the declared P1E scope. It is not a complete Delaunay constructor,
final topology fingerprint/replay qualification, CAD-conforming tetra mesher or TET4 product/solver claim.

```text
M2 OVERALL = NOT QUALIFIED
```

## 16. Next phase boundary

After final docs exact-head CI:

```text
DEV-MESH-P1F
M2.1-F
Serial Constructor Determinism / Fingerprint / Replay
```

P1F is not implemented by this task.
