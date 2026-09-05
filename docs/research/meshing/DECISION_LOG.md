# Meshing Decision Log

## ADR-MESH-0001 — Dynamics26 will develop an original meshing engine

**Status:** ACCEPTED  
**Date:** 2026-09-05

### Context

The initial V1.2 roadmap considered integrating a mature external volume mesher. Research confirmed that Netgen, Gmsh, MMG, TetGen and CGAL provide valuable algorithms, architectures and benchmark ideas. The project direction has now changed: Dynamics26 should own the production meshing implementation.

### Decision

- Do not make Netgen, Gmsh, MMG, TetGen or CGAL the V1.2 production meshing engine.
- Develop original Dynamics26 meshing algorithms under the existing \`femcae::meshing\` architecture.
- Use peer-reviewed papers and mathematical references as primary algorithm sources.
- Use open-source code only for architecture/failure/test study under \`CLEAN_ROOM_POLICY.md\`.
- Preserve \`GeometryEntityId\` as CAD authority and \`SimulationMesh\` as solver mesh authority.
- Keep \`StructuredHexMesher\` as a validated baseline while the unstructured engine matures.

### Rationale

Advantages:

- full control of geometry/provenance contracts,
- no product dependence on a third-party meshing API,
- no inherited external implementation/license constraints in the core,
- ability to tune specifically for nonlinear/rubber automotive workflows,
- meshing research becomes an owned Dynamics26 competency.

Costs:

- substantially larger R&D effort,
- robust computational geometry is difficult,
- quality and failure handling must be earned through a large verification corpus,
- arbitrary CAD meshing arrives later than an adapter-based approach.

### Consequence

V1.2 should be treated as a staged meshing R&D program, not a single feature.

---

## ADR-MESH-0002 — Delaunay-first tetrahedral research track

**Status:** PROPOSED  
**Date:** 2026-09-05

### Context

Strong literature exists for incremental Delaunay, constrained boundary recovery and Delaunay refinement. Commercial/open systems also demonstrate Delaunay and advancing-front hybrids.

### Proposed direction

Start original volume-meshing research with:

1. robust geometric predicates,
2. incremental Delaunay tetrahedralization on point clouds,
3. constrained boundary recovery,
4. Delaunay refinement,
5. separate quality optimization.

Keep advancing-front tetra meshing as an independent research track and potential hybrid component.

### Why not accepted yet

Boundary recovery, sliver control, sizing behavior and CAD-surface strategy need experiments before the production algorithm family is fixed.

---

## ADR-MESH-0003 — Named Selection is a meshing scope

**Status:** PROPOSED  
**Date:** 2026-09-05

### Context

ANSYS and COMSOL both reuse persistent selections for mesh controls. Dynamics26 already owns a persistent Named Selection system.

### Proposed direction

Allow future mesh-control definitions such as local size to scope through persistent Named Selection ObjectIds, resolved into GeometryEntityIds before meshing.

### Constraint

The low-level meshing kernel receives resolved immutable geometry identities; it does not depend on Qt/ProjectModel/NamedSelectionService.

---

## Open decisions

| ID | Topic | Needed evidence |
|---|---|---|
| ADR-MESH-0002 | Delaunay-first volume track | M1/M2/M4 experiments |
| ADR-MESH-0003 | Named Selection mesh-control scope | mesh-control document model design |
| ADR-MESH-0004 | First surface mesher | parameter-space CDT vs advancing front experiments |
| ADR-MESH-0006 | Quality metric release gates | nonlinear TET4 sensitivity study |
| ADR-MESH-0007 | Curvature/proximity size-field formulas | M3/M5 experiments |


---

## ADR-MESH-0005 — Robust predicate implementation strategy

**Status:** ACCEPTED  
**Date:** 2026-09-05

### Research finding

M1 establishes that CAD/modeling tolerance and topological predicate correctness are separate concerns. A fixed epsilon on determinant magnitude is not accepted as a general topology policy.

### Leading design

```text
finite-input validation
→ fast binary64 determinant
→ certified error filter
→ adaptive exact fallback when uncertain
→ PredicateSign {Negative, Zero, Positive}
```

True exact degeneracy remains `Zero`. Symbolic perturbation/tie-breaking belongs to M2 or the consuming topology algorithm.

### Verification authority

An independent exact-rational oracle generated from the exact binary64 input values must exist before the production adaptive path can be qualified.

### Compiler constraint

The predicate target must not be compiled under floating-point modes that invalidate the arithmetic/error-bound assumptions. Fast-math is therefore disallowed for this target; FP contraction policy must match the eventual filter proof.

### Acceptance evidence

All original acceptance conditions are now executable:

1. dual independent exact-oracle paths and raw-bit fixtures exist,
2. the M1.7 C++ exact dyadic fallback is independent from the Python oracle,
3. the M1.8 fast path has an implementation-specific homogeneous determinant proof,
4. the 2^-43 certification coefficient is protected by a compile-time inequality,
5. fast-math is forbidden and FP contraction is disabled for the predicate source,
6. committed, generated, adversarial and metamorphic corpora pass,
7. macOS arm64 Debug/Release workflow #239 is SUCCESS,
8. the kernel has no external mesher/predicate runtime/source dependency.

Decision: the Dynamics26 filtered-exact predicate strategy is accepted for M1/M2 use.


### M1.1 exact-oracle research update — 2026-09-05

Evidence now supports the following parts of ADR-MESH-0005:

- predicate truth is exact with respect to stored finite binary64 inputs,
- a test-only exact-rational oracle is feasible using standard integer/rational arithmetic,
- binary64 inputs can be converted to common dyadic integers without changing predicate sign,
- filter architecture should permit conservative fallback but never false certification,
- commercial ANSYS/COMSOL/Marc tolerance and repair controls are geometry/meshing policy, not evidence for an epsilon-based predicate design.

Historical note: this condition was satisfied during M1.6–M1.9; ADR-MESH-0005 is now **ACCEPTED**.


### M1.2 certified-filter research update — 2026-09-05
The first Dynamics26 F0 filter specification is now derived independently:
- expanded-monomial evaluation graph,
- gamma_n error accumulation,
- fast sign certification only outside the computed error envelope,
- no fast-path Zero,
- explicit normal-range gate,
- initial `-fno-fast-math -ffp-contract=off` compiler contract.

Leading conservative first-order factors are approximately:
- orient2d: 4u,
- orient3d: 10u,
- incircle: 18u,
- insphere: 80u,
times the documented computed absolute-monomial sum and denominator corrections.

Historical note: executable M1.6–M1.9 evidence has satisfied this condition; ADR-MESH-0005 is now **ACCEPTED**.


---

## ADR-MESH-0008 — Deterministic degeneracy policy

**Status:** ACCEPTED  
**Date:** 2026-09-05

### Research findings

M1.3 separates multiple degeneracy classes that must not share one epsilon/tie rule.

Leading policy:
- exact duplicate coordinates are canonicalized into one site,
- near-coincident distinct coordinates remain distinct unless explicit geometry conditioning merges them,
- lower-dimensional point sets are reported explicitly,
- robust predicates preserve exact `Zero`,
- distinct co-spherical Delaunay sites may use a formal SoS-style perturbation based on stable PointId ordering,
- CAD/domain invalidity is never hidden by symbolic perturbation.

### Determinism target

For the same immutable sites, stable PointIds, settings and algorithm version, the canonical topology should not depend on transient pointer layout, input enumeration or supported insertion ordering.

### Acceptance evidence

M1.9-B now provides:

1. exact duplicate canonicalization with signed-zero normalization,
2. deterministic PointId assignment independent of input enumeration,
3. exact affine-dimension classification,
4. a Python standard-library formal symbolic perturbation oracle,
5. stable PointId-driven perturbation ranking,
6. exact coplanar/cocircular/cospherical symbolic fixtures,
7. predicate permutation checks,
8. macOS arm64 Debug/Release workflow #237 SUCCESS.

The production Delaunay algorithm will consume this policy in M2. Actual insertion-order/cavity determinism belongs to M2 and is not silently claimed by M1.


---

## ADR-MESH-0009 — M2 tetra topology and point-location foundation

**Status:** PROPOSED  
**Date:** 2026-09-05

### Leading design

- separate geometry predicates, combinatorial topology and location policy,
- tetra record has four vertices and four neighbors, where neighbor[i] is opposite vertex[i],
- use index+generation `TetHandle` semantics to detect stale references,
- preserve oriented face representation separately from sorted canonical face keys,
- use adjacency walking from a good seed as the primary first point locator,
- use an exact slow fallback during the reference implementation,
- spatial insertion ordering is benchmarked as part of point-location performance,
- cavity traversal uses reusable buffers and epoch marks,
- failed local retriangulation must not leave a corrupted mesh,
- M2.0 remains serial.

### Commercial benchmark findings

- ANSYS exposes parallel part/method meshing with explicit CPU and memory guidance.
- COMSOL's 3D tetra mesher parallelizes over faces/domains; one single imported CAD domain may see little parallel speedup.
- Marc exposes rich remeshing/density/protected-entity controls but not internal point-location/storage algorithms.

These findings support measuring memory and designing later parallelism, but do not dictate M2 internal search code.

### Why PROPOSED

Acceptance requires executable evidence from M1.4/M2:
1. topology invariant tests,
2. walk vs brute-force agreement,
3. stale-handle tests,
4. insertion-order benchmark,
5. memory profile,
6. cavity rollback validation.


---

## ADR-MESH-0010 — Meshing verification harness architecture

**Status:** ACCEPTED  
**Date:** 2026-09-05

### Leading design

- use existing CMake/CTest infrastructure,
- no new C++ unit-test framework for M1/M2,
- exact oracle remains test-only Python standard library,
- predicate fixture coordinates use raw binary64 bit-pattern serialization,
- Fraction and dyadic-integer oracle paths must agree before fixture generation,
- permanent golden/regression fixtures are committed,
- larger deterministic corpora are generated from fixed seeds,
- every mismatch emits a standalone replay record,
- optimized production paths are always compared to an independent slow/reference path.

### Commercial benchmark findings

ANSYS and COMSOL publicly expose multi-metric mesh verification, statistics/distributions and warning/error workflows. Marc public material strongly connects mesh refinement/remeshing with nonlinear solution adequacy but does not provide a current public internal quality-metric specification suitable as an oracle.

Therefore Dynamics26 will distinguish:
- topology validity,
- geometric mesh quality,
- sizing conformity,
- provenance,
- reproducibility,
- solver/analysis qualification.

### Acceptance evidence

The harness now has:
1. dual exact oracles,
2. bit-exact Python/C++ round-trip,
3. deterministic corpus regeneration,
4. committed golden fixtures,
5. CTest integration,
6. generated adversarial/metamorphic corpus,
7. one-case D26PRED replay round-trip into the C++ production/reference predicate runner,
8. target Debug/Release CI evidence through workflows #231, #233, #234, #236 and #239.

ADR-MESH-0010 is accepted.


### M1.6 executable evidence update — 2026-09-05

The M1.5 verification architecture now has a first executable implementation:

- dual exact predicate oracles implemented independently,
- all four target predicates represented,
- D26PRED raw-bit fixture schema implemented,
- committed golden corpus contains 25 cases,
- deterministic generated test cross-checks 1024 additional cases per run,
- C++ parser verifies binary64 bit round-trip and strict schema behavior,
- signed-zero canonicalization policy is exercised,
- CTest integration is present.

Local research-prototype checks passed before commit. Exact-head macOS arm64 CI is still the qualification authority.

Historical note: those CI and replay requirements are now satisfied; ADR-MESH-0010 is **ACCEPTED**.


---

## ADR-MESH-0011 — M1 closeout gate is mandatory before M2

**Status:** ACCEPTED  
**Date:** 2026-09-05

### Decision

Dynamics26 will not begin M2 Bowyer-Watson / point-cloud Delaunay implementation merely because predicate tests are green.

A formal M1 closeout audit must pass first.

### Audit history

The first audit at `f1e3ab433d94...` found:

PASS:
- exact dual oracle,
- bit-exact fixtures,
- exact C++ predicate kernel,
- certified fast path,
- compiler contract,
- target macOS arm64 Debug/Release CI,
- clean-room kernel boundary.

BLOCKERS:
- executable duplicate/affine-dimension foundation,
- formal symbolic perturbation oracle,
- stable PointId perturbation hierarchy,
- tetra primitive/validator foundation,
- failure replay proof,
- broader adversarial/metamorphic corpus,
- telemetry baseline,
- documentation synchronization.

### Second-audit hardening update — 2026-09-05

The synchronized candidate commit `d007fca6...` passed exact-head workflow #240. Independent re-audit of the executable topology validator then found that noncanonical invalid neighbor handles and shared/non-manifold face-incidence corruption could be underdiagnosed. Commit `d6501f1d...` hardened those M1 combinatorial invariants and passed exact-head workflow #241. M2 remained blocked throughout.

### Second/final closeout result — 2026-09-05

Documentation synchronization `6e939eb6...` passed exact-head workflow #242. The second/final audit re-evaluated G01–G20 from repository evidence and returned PASS for every gate with M1 blocker count 0. The full decision record is `m1-robust-geometry/M1_FINAL_CLOSEOUT.md`.

### Consequence

```text
M1    = QUALIFIED
M1.9  = QUALIFIED
M2    = AUTHORIZED AFTER FINAL-CLOSEOUT EXACT-HEAD CI
```

The first M2 activity is M2.0 — Delaunay Reference Architecture & Experiment Plan. Bowyer-Watson production insertion does not start before the M2.0 decisions and acceptance tests are frozen.


---

## ADR-MESH-0012 — M1/M2 executable scope boundary

**Status:** ACCEPTED  
**Date:** 2026-09-05

### Decision

M1 must deliver executable numerical/topological foundations, but it must not pre-implement the M2 Delaunay algorithm merely to satisfy closeout.

M1 owns and verifies:
- exact/filtered robust predicates,
- exact duplicate canonicalization,
- signed-zero site normalization,
- stable PointId assignment,
- affine-dimension classification,
- formal test-only symbolic perturbation oracle,
- tetra handle / opposite-face / canonical-face primitives,
- reciprocal-neighbor topology validator,
- replay/adversarial/metamorphic verification,
- predicate telemetry.

M2 owns:
- point-location walking,
- super-tetra/ghost-hull choice,
- cavity discovery and retriangulation,
- actual production symbolic tie consumption,
- insertion-order experiments,
- canonical final Delaunay topology fingerprint,
- memory/locality optimization of the insertion engine.

### Rationale

This boundary prevents two opposite errors:
1. starting M2 before M1 mathematics/topology is trustworthy,
2. hiding M2 implementation inside M1 merely to close the gate.

M2 may begin only after the final M1 closeout audit is QUALIFIED.


---

## ADR-MESH-0013 — M2.0 serial ghost-hull reference architecture

**Status:** PROPOSED
**Date:** 2026-09-05

### Decision candidate

The first Dynamics26 3D point-cloud Delaunay constructor will use:
- serial incremental Bowyer-Watson construction,
- no numeric super-tetrahedron,
- finite + infinite/ghost-cell hull topology,
- tagged Infinite vertex separate from PointId,
- deterministic affine-basis bootstrap,
- brute-force location oracle,
- deterministic adjacency walk,
- canonical face keys for local patch stitching,
- canonical finite/hull fingerprints.

### Rationale

M1 already established exact predicate truth and canonical site identity. Artificial extreme
coordinates would reintroduce scale and conditioning choices at the construction boundary.

Ghost topology makes convex-hull insertion part of the same adjacency/cavity model and gives every
facet two incident cells.

### Acceptance

Remains PROPOSED until M2-G01..G20 executable evidence closes the architecture gates.

---

## ADR-MESH-0014 — Delaunay-specific lift-only symbolic tie policy

**Status:** PROPOSED
**Date:** 2026-09-05

### Decision candidate

The production M2 Delaunay tie rule will not directly reuse the M1 general spatial-coordinate
symbolic oracle.

For exact InSphere/InCircle zero:
- keep real x/y/z coordinates unchanged,
- conceptually perturb only the lifted coordinate by ordered infinitesimals,
- use canonical PointId relative order as fixed global symbolic priority,
- evaluate the first non-zero exact orientation cofactor,
- never construct numeric epsilon.

Exact Orient3D zero remains geometric truth and is never symbolically promoted into a finite
tetrahedron.

### Evidence

Devillers-Teillaud 2011 provides specialized 3D perturbation theory and a fixed-order unique
PP-regular triangulation without flat tetrahedra when sites are not all coplanar.

Dynamics26 row-major cofactor signs are derived in
m2-delaunay/DELAUNAY_MATHEMATICS.md from the existing M1 determinant layout and require executable
exact fixtures before acceptance.

### Semantic limit

The selected degenerate subdivision is weakly Delaunay/regular at the limit. Do not overclaim that
every selected degenerate connectivity is the Delaunay triangulation of a non-degenerate spatial
coordinate perturbation.

---

## ADR-MESH-0015 — Transactional cavity insertion contract

**Status:** PROPOSED
**Date:** 2026-09-05

### Decision candidate

M2 mutation follows Plan -> Validate -> Commit.

The existing triangulation is unchanged while location, conflict flood, cavity boundary, candidate
cells, outside-neighbor patches and symbolic decisions are being validated.

Reference validation includes:
- connected conflict set,
- 4C = 2I + B,
- closed connected boundary 2-manifold,
- Euler characteristic 2,
- non-zero/positive finite candidate orientation,
- reciprocal outside-neighbor patches,
- exact oracle agreement in small/reference tests.

### Acceptance

Injected invalid plans must prove no mutation; interior/face/edge/hull/exterior fixtures must pass;
brute-force/flood conflict sets must agree; replay must reproduce failures; exact-head CI must pass.


---

## ADR-MESH-0016 — Layered local/global Delaunay verification

**Status:** PROPOSED
**Date:** 2026-09-05

### Decision candidate

M2 correctness is not represented by one boolean validator.

The reference constructor will verify five layers:
1. combinatorial finite+ghost topology,
2. positive/non-overlapping geometric embedding and convex-hull support,
3. unperturbed weak local Delaunay legality,
4. canonical lift-only symbolic local legality for exact ties,
5. independent small-N global location/conflict/empty-sphere and permutation oracles.

The unified finite+ghost complex is additionally checked as an S^3 triangulation:

    V - E + F - C = 0
    F = 2C
    E = V + C

### Conflict seed contract

Cavity flooding starts only from an explicitly verified conflict cell. The OUTSIDE_CONVEX_HULL
location result retains the strictly violated hull facet and its ghost cell as a witness; an arbitrary
infinite cell is never assumed to conflict.

### Rationale

The Delaunay Lemma links local facet legality with global Delaunay correctness for a valid
triangulation, while exact co-spherical configurations still admit multiple weak Delaunay
triangulations. The separate symbolic-local layer is therefore required to verify the fixed
Dynamics26 topology policy.

### Acceptance

Requires executable M2-G21..G24 in addition to the original M2.0 qualification gates.


---

## ADR-MESH-0017 — Typed ghost cell and append-only reference arena

**Status:** PROPOSED
**Date:** 2026-09-05

### Decision candidate

M2 does not encode the infinite vertex as a reserved PointId and does not silently change the meaning
of the M1 finite TetRecord.

The M2 reference topology uses:
- typed Finite(PointId) / Infinite vertex references,
- exactly one Infinite vertex in a ghost cell,
- Infinite fixed at ghost local slot 0,
- neighbor[i] opposite vertex[i],
- separate canonical topological face keys and oriented finite faces,
- append-only/tombstoned cell arena for M2.1 correctness qualification.

Slot reuse, packing and SoA conversion are deferred performance/storage experiments.

### Rationale

The type boundary prevents Infinite from reaching coordinate lookup, exact predicates or symbolic
site priority. Append-only allocation allows candidate capacity to be reserved before the commit
barrier and removes free-list/generation-reuse behavior from first-principles topology debugging.

### Acceptance

M2-G25..G27 plus the existing topology/transaction gates.


---

## ADR-MESH-0018 — Resource limits are not geometric validity

**Status:** PROPOSED
**Date:** 2026-09-05

### Decision candidate

M2 treats computational resource exhaustion as an operational state separate from geometry and
topology validity.

3D Delaunay output can have quadratic worst-case complexity. No constant tetra/site ratio is a
mathematical validity bound.

Resource checks occur before the commit barrier using checked arithmetic and the known candidate
cell count. If the configured budget cannot accommodate the insertion, the existing triangulation
remains unchanged and a typed ResourceLimitExceeded result is returned.

### Forbidden fallback

Resource pressure does not authorize silent site merging, skipped insertions, partial-mesh success,
predicate weakening or unversioned topology changes.

### Acceptance

M2-G28 and M2-G29 plus complexity telemetry over ordinary and adversarial distributions.


---

## ADR-MESH-0019 — Determinism scope and versioned topology policy

**Status:** PROPOSED
**Date:** 2026-09-05

### Decision candidate

M2 determinism is scoped to the same exact canonical binary64 site set and the same versioned
canonicalization/symbolic topology policy.

Under that scope, final finite/hull topology is independent of input enumeration, supported insertion
order, memory allocation and build mode.

The current symbolic priority is coordinate-derived canonical PointId order. Therefore M2 does not
claim exact-degenerate symbolic connectivity is invariant under coordinate transforms that alter that
priority. The transformed result must instead satisfy the current policy for its transformed exact
site set.

### Versioning candidate

- D26SITE1 — canonical binary64 site identity/order,
- D26LIFT1 — lift-only Delaunay tie using D26SITE1 PointId priority,
- D26DT1 — canonical finite/hull topology serialization.

Replay and fingerprint metadata carry these versions.

### Future option

A provenance-derived stable SymbolicPriorityKey may be researched for CAD/adaptation stages if
degenerate topology must survive model transforms. It is not part of M2.0 default semantics.

### Acceptance

M2-G30..G33.
