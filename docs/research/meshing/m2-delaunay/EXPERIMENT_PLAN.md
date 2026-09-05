# M2.0 — Delaunay Experiment and Qualification Plan

Status: PROPOSED
Date: 2026-09-05

## Objective

Freeze executable evidence required before the M2 point-cloud Delaunay constructor can be called
qualified.

The reference implementation is serial and correctness-first. Optimization cannot replace a
reference path until agreement is demonstrated.

## Implementation sequence after M2.0 freeze

### M2.1-A — Semantic predicates
- positive-cell InSphere semantic wrapper,
- lift-only symbolic InSphere,
- ghost half-space + projected InCircle semantic,
- lift-only symbolic InCircle,
- exact sign/permutation tests.

### M2.1-B — Bootstrap + ghost topology
- deterministic affine basis,
- one finite tetra + four ghost cells,
- lower-dimensional explicit result,
- typed infinite vertex,
- topology validation.

### M2.1-C — Brute-force reference location
- exact CELL/FACET/EDGE/VERTEX/OUTSIDE classification,
- no walking optimization yet.

### M2.1-D — Cavity oracle + transaction
- brute-force all-cell conflict oracle,
- adjacency flood conflict path,
- boundary/internal extraction,
- Plan -> Validate -> Commit,
- proof that validation failure does not mutate topology.

### M2.1-E — Deterministic walk
- adjacency visibility walk,
- canonical facet tie,
- walk vs brute-force agreement.

### M2.1-F — Determinism / fingerprint
- canonical finite topology,
- hull fingerprint,
- insertion-order permutations,
- replay and telemetry.

## Golden fixtures

### G-A minimal bootstrap tetra
Checks positive orientation, four reciprocal ghost neighbors and closed hull.

### G-B tetra + interior point
Minimal expected cavity: C=1, I=0, B=4.

### G-C internal facet point
Two finite cells sharing a facet; query in strict interior.
Expected local cavity: C=2, I=1, B=6.

### G-D hull facet point
One finite + one ghost cell sharing hull facet; query in strict interior.
Expected local cavity: C=2, I=1, B=6.

### G-E edge point
Known edge star with m incident unified cells.
Expected: C=m, I=m, B=2m.

### G-F exterior far point
Expected minimal cavity: one ghost cell; one new finite and three new ghost cells.

### G-G five exact co-spherical sites
Canonical sites and expected two-tetra fingerprint are frozen in DELAUNAY_MATHEMATICS.md.
Run all 5! = 120 insertion permutations.

### G-H unit cube
Expected six-tetra fingerprint is frozen in DELAUNAY_MATHEMATICS.md.
Run all 40,320 insertion permutations. Every permutation must produce the same canonical finite and
hull fingerprints under fixed symbolic priority.

## Adversarial families

- exact coplanar input -> explicit not-3D outcome,
- plane plus one off-plane site,
- nearly coplanar tetra,
- exact co-spherical five-site sets,
- nearly co-spherical site,
- Cartesian grids,
- sphere-shell sites,
- thin slabs,
- clustered points,
- very large coordinates,
- very small coordinates,
- mixed large/small scales,
- signed-zero variants,
- duplicate source records,
- reversed input enumeration,
- reversed insertion order,
- deterministic generated random clouds.

Near-coincident but mathematically distinct sites are not merged by an M2 epsilon.

## Independent oracles

### Location oracle
Brute-force exact point-in-tetra/hull classification.

### Conflict oracle
For small triangulations, evaluate every finite/ghost cell independently and compare canonical
conflict set with adjacency flood.

### Delaunay oracle
For small-N output:
- every finite tetra positive,
- no other site is strictly inside an unperturbed finite circumsphere,
- exact co-spherical cases are geometrically allowed,
- symbolic golden fingerprint verifies deterministic subdivision separately.

This separates weak-Delaunay geometric truth from symbolic topology choice.

## M2 qualification gates

| Gate | Requirement |
|---|---|
| M2-G01 | research/source/clean-room package committed |
| M2-G02 | Dynamics26 Orient3D/InSphere sign convention executable test |
| M2-G03 | deterministic affine-basis bootstrap |
| M2-G04 | infinite vertex typed; never PointId/coordinate |
| M2-G05 | finite+ghost bootstrap topology valid |
| M2-G06 | brute-force location classification passes boundary cases |
| M2-G07 | deterministic walk agrees with brute-force oracle |
| M2-G08 | finite conflict semantic matches exact oracle |
| M2-G09 | ghost half-space/coplanar-circle semantic passes |
| M2-G10 | lift-only symbolic InSphere/InCircle resolves ties without changing spatial orientation truth |
| M2-G11 | conflict flood equals brute-force conflict oracle |
| M2-G12 | cavity connectivity and 4C=2I+B pass |
| M2-G13 | boundary 2-manifold + Euler checks pass |
| M2-G14 | invalid plan leaves triangulation logically unchanged |
| M2-G15 | interior/face/edge/hull/exterior insertion fixtures pass |
| M2-G16 | five-site co-spherical 120-permutation fingerprint invariant |
| M2-G17 | cube 40,320-permutation fingerprint invariant |
| M2-G18 | generated small-N global weak-Delaunay oracle passes |
| M2-G19 | standalone replay reproduces at least one injected failure |
| M2-G20 | exact-head macOS arm64 Debug/Release CI + telemetry baseline green |

M2 is not QUALIFIED until mandatory gates are executable evidence.

## Telemetry

Record at minimum:
- insertion calls/success/failure,
- locate calls, walk steps, max walk, brute-force cells, mismatch count,
- cavity cells/internal facets/boundary facets/max cavity/oracle mismatch,
- Orient3D and InSphere fast/exact/zero counts,
- symbolic InSphere/InCircle tie counts,
- transaction plan/commit failures,
- finite/ghost cell counts.

Performance telemetry is observational until correctness gates close.

## Optimization experiments after reference qualification

Candidate comparisons:
- canonical insertion order,
- reverse order,
- Morton order,
- Hilbert order,
- deterministic BRIO-like batching,
- previous-cell hint versus other deterministic hints.

Required invariant:

    fixed canonical sites + fixed symbolic priority
    => identical canonical final topology

If an optimization changes fingerprint on a degenerate set, it is a correctness bug or a different
explicitly versioned topology policy.

## Product boundary

M2 output is a research tetrahedralization of a point set. It is not yet arbitrary-CAD product mesh.

M3/M4 must still solve CAD curve/facet conformity, boundary recovery, region classification and
GeometryEntityId provenance. M7 performs actual TET4 solver/product qualification.
