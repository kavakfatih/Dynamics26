# M2.0 — Research Freeze Audit

Date: 2026-09-05
Audit baseline before freeze commit: 2925e1d3c355e00e490f7cc66fea5a1f709a5f87

## 1. Audit conclusion

M2.0 research has reached **DESIGN FROZEN** status.

This means:
- the first serial reference-construction architecture is sufficiently specified to implement,
- mathematical sign/degeneracy/topology contracts are committed and reviewable,
- acceptance evidence is defined before production/reference implementation,
- later implementation may discover a defect and reopen an ADR, but it may not silently invent a
  new topology rule in code.

It does **not** mean:
- M2 is QUALIFIED,
- point-cloud Delaunay production code exists,
- arbitrary CAD can be tetrahedralized,
- FEM TET4 is product-qualified.

M2 qualification still requires executable M2 gates.

## 2. M1 prerequisite

M1 robust-geometry foundation is already QUALIFIED and supplies:
- finite binary64 validation,
- canonical exact-coordinate site identity,
- signed-zero normalization,
- stable PointId,
- exact affine-dimension classification,
- Orient2D/Orient3D/InCircle/InSphere filtered-exact predicates,
- test-only general symbolic oracle,
- finite tetra handle/neighbor/canonical-face primitives,
- replay/adversarial/metamorphic verification infrastructure.

M2 does not weaken or replace those predicate truths.

## 3. Research-question closure matrix

| ID | Question | Frozen M2.0 answer | Authority |
|---|---|---|---|
| Q01 | Numeric super-tetra or unbounded topology? | finite + ghost/infinite cells; no numeric super-tetra | M2_0_REFERENCE_ARCHITECTURE.md |
| Q02 | How is 3D construction bootstrapped? | exact affine pre-scan + deterministic four-site basis + four ghost cells | M2_0_REFERENCE_ARCHITECTURE.md |
| Q03 | What is a finite Delaunay conflict? | positive stored tet + exact/filtered InSphere; exact zero goes to D26LIFT1 candidate | DELAUNAY_MATHEMATICS.md |
| Q04 | How are exact co-spherical/cocircular ties resolved? | lift-only formal perturbation; x/y/z unchanged; PointId relative priority | DELAUNAY_MATHEMATICS.md |
| Q05 | How is a point located? | brute-force exact oracle + deterministic adjacency visibility walk | M2_0_REFERENCE_ARCHITECTURE.md |
| Q06 | How is a conflict flood seeded safely? | typed verified ConflictSeed; outside walk retains violated hull/ghost witness | LOCAL_CORRECTNESS_AND_SEED_CONTRACT.md |
| Q07 | What is the cavity validity contract? | connected unified conflict 3-ball, 4C=2I+B, closed boundary 2-sphere | CAVITY_TRANSACTION_SPEC.md |
| Q08 | How is replacement patch built? | cone each boundary facet; finite/ghost kind from facet type; exact orientation normalization | PATCH_ORIENTATION_AND_STITCHING.md |
| Q09 | How are mutations made failure-safe? | Plan -> Validate -> reserve -> Commit; predicate-free commit | CAVITY_TRANSACTION_SPEC.md |
| Q10 | How is Infinite stored? | typed vertex, never PointId/coordinate; ghost Infinite fixed at local slot 0 | CELL_STORAGE_AND_MUTATION_MODEL.md |
| Q11 | How is Delaunay correctness validated? | topology -> embedding/hull -> weak local -> symbolic local -> independent global oracle | LOCAL_CORRECTNESS_AND_SEED_CONTRACT.md |
| Q12 | What is reproducibility scope? | same exact sites + same versioned topology policy => same finite/hull topology | DETERMINISM_SCOPE_AND_POLICY_VERSIONING.md |
| Q13 | How are memory/resource failures interpreted? | separate typed operational failure; never geometry/topology invalidity | COMPLEXITY_AND_RESOURCE_MODEL.md |
| Q14 | What is the output-complexity assumption? | no linear upper-bound assumption; 3D worst case may be quadratic | COMPLEXITY_AND_RESOURCE_MODEL.md |
| Q15 | What is the first construction algorithm? | serial correctness-first Bowyer-Watson cavity insertion | M2_0_REFERENCE_ARCHITECTURE.md |
| Q16 | When may locality/parallelism change the kernel? | only after reference/oracle agreement; topology policy cannot change silently | EXPERIMENT_PLAN.md |

No unresolved architectural question above blocks M2.1-A.

## 4. Frozen mathematical conventions

### 4.1 Finite tetra orientation

All live finite M2 tetrahedra are stored with:

    orient3d(v0,v1,v2,v3) == Positive.

For a positive tetra:
- InSphere Positive = inside/conflict,
- InSphere Negative = outside/non-conflict,
- InSphere Zero = exact Delaunay tie.

### 4.2 Exact zero

Preserve Zero for geometric truth:
- duplicate coordinate,
- collinearity,
- coplanarity,
- face/edge/vertex point location,
- zero-volume candidate.

Resolve Zero symbolically only when a Delaunay topology choice is required:
- co-spherical InSphere,
- co-circular projected InCircle for ghost coplanarity.

### 4.3 No numeric epsilon topology

M2.0 defines no determinant-magnitude epsilon for topological truth and constructs no numeric
symbolic epsilon.

### 4.4 Geometry remains unchanged

D26LIFT1 candidate perturbs only the formal lift coordinate. Stored x/y/z coordinates are unchanged.

## 5. Frozen combinatorial conventions

- one typed Infinite vertex represents the unbounded topology,
- every valid ghost has exactly one Infinite vertex,
- ghost Infinite is local vertex 0,
- neighbor[i] is opposite vertex[i],
- canonical face key is identity only,
- oriented finite face is geometry only,
- every unified triangular face has two incident cells,
- the whole finite+ghost complex is checked as an S^3 triangulation in reference/global validation.

## 6. Frozen transaction conventions

Before commit:
- point location complete,
- verified conflict seed complete,
- conflict flood complete,
- boundary/internal classification complete,
- candidate cell types complete,
- finite orientation complete,
- ghost hull orientation complete,
- new-new lateral pairing complete,
- new-old patch targets complete,
- resource/capacity checks complete,
- storage reserved.

After commit barrier:
- no geometric predicate,
- no symbolic predicate,
- no topology choice,
- no expected allocation.

## 7. Frozen determinism candidates

Policy identifiers proposed by M2.0:
- D26SITE1 — canonical binary64 site identity/order,
- D26LIFT1 — lift-only Delaunay tie with D26SITE1 PointId priority,
- D26DT1 — canonical finite/hull topology serialization.

These identifiers are part of replay/fingerprint semantics.

The guarantee is intentionally scoped to the same exact canonical site set and same policy version.
Exact-degenerate topology is not claimed invariant under coordinate transformations that change the
symbolic site order.

## 8. Golden topology candidates

### Five co-spherical sites

Expected finite connectivity:
- 1 2 3 4
- 2 3 4 5

Qualification: all 120 insertion permutations.

### Unit cube

Expected finite connectivity:
- 1 2 3 5
- 2 3 4 5
- 2 4 5 6
- 3 4 5 7
- 4 5 6 7
- 4 6 7 8

Qualification: all 40,320 insertion permutations plus canonical hull fingerprint.

These are research-derived golden candidates until executable M2 evidence confirms them.

## 9. Gate classification

### Reference-qualification mandatory

M2-G01..M2-G26 and M2-G28..M2-G36 are mandatory for the serial M2 reference constructor.

### Post-reference optimization gate

M2-G27 (deterministic slot reuse) is deliberately non-blocking for the append-only M2.1 reference
constructor. It becomes mandatory only before slot reuse is allowed to replace the append-only
reference arena.

This prevents a storage optimization from blocking the first correctness-qualified construction while
still giving it an explicit evidence gate.

## 10. M2.1 implementation sequence authorized by the freeze

The research freeze authorizes implementation in narrow order:

### M2.1-A — semantic predicates
Implement and independently test:
- positive finite-cell InSphere semantic,
- lift-only symbolic InSphere,
- ghost half-space semantic,
- exact projected InCircle,
- lift-only symbolic InCircle.

No cavity mutation is needed yet.

### M2.1-B — typed ghost bootstrap
Implement:
- M2 finite/infinite vertex type,
- append-only reference arena,
- deterministic affine basis,
- one finite + four ghost bootstrap,
- validators.

### M2.1-C — brute-force exact location
Implement the slow oracle before the optimized walk.

### M2.1-D — cavity oracle + transactional patch
Implement brute-force conflict oracle, flood, boundary extraction, plan validation and commit.

### M2.1-E — deterministic walk
Only after brute-force location is trusted.

### M2.1-F — deterministic topology/fingerprint/replay
Close exhaustive degeneracy and reproducibility gates.

Each substage must preserve the committed mathematical contracts.

## 11. Explicitly deferred

Not authorized by M2.0 freeze as a production feature:
- CAD boundary recovery,
- constrained facets/segments,
- quality/sliver optimization,
- size fields,
- TET4 solver product integration,
- parallel insertion,
- compact/SoA arena replacing reference storage,
- slot reuse replacing append-only reference storage,
- Delaunay hierarchy,
- provenance-derived symbolic priority,
- GPU meshing.

## 12. Research freeze result

    M1 = QUALIFIED
    M2.0 = DESIGN FROZEN
    M2 implementation = NOT QUALIFIED
    M2.1-A = NEXT AUTHORIZED IMPLEMENTATION STAGE

The first code written after this freeze should be the semantic-predicate layer, not the full
Bowyer-Watson constructor.
