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


---

## ADR-MESH-0020 — Boundary-cone patch orientation and stitching

**Status:** PROPOSED
**Date:** 2026-09-05

### Decision candidate

Each cavity-boundary facet creates exactly one replacement cell by coning to the inserted point.

- finite boundary facet -> finite cell normalized to positive Orient3D,
- Infinite boundary facet -> ghost cell with Infinite fixed at slot 0.

The original boundary facet reconnects to the old outside neighbor. The three point-containing
lateral faces pair new-new through canonical face keys.

The closed 2-manifold boundary guarantees every boundary edge, and therefore every lateral face, has
exactly two owners.

Ghost finite face 0 is oriented outward after its finite neighbor/opposite vertex is identified.

### Rationale

This separates face identity from geometric orientation, removes reliance on incidental owner-cell
vertex order and makes the commit phase predicate-free.

### Acceptance

M2-G34..G36.


---

## ADR-MESH-0021 — M2.0 research design freeze

**Status:** ACCEPTED (RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-05

### Decision

The M2.0 research package is frozen as the implementation authority for the first serial Dynamics26
point-cloud Delaunay reference constructor.

Frozen design areas:
- ghost/infinite hull representation,
- deterministic affine bootstrap,
- finite/ghost conflict semantics,
- lift-only exact tie policy,
- point-location/reference-oracle architecture,
- verified conflict seed,
- cavity topology and transaction model,
- finite/ghost storage semantics,
- local/global correctness ladder,
- deterministic fingerprint/policy versioning,
- patch orientation/stitching,
- complexity/resource semantics,
- qualification experiment matrix.

### Meaning of ACCEPTED

This ADR accepts the **research architecture**, not the M2 implementation.

M2 remains unqualified until executable mandatory gates pass.

### Implementation authorization

M2.1-A semantic predicate implementation is the next authorized stage. Full cavity construction is
not the first coding task.

### Reopen rule

If executable evidence contradicts a frozen mathematical/architectural contract, reopen the relevant
ADR and derivation explicitly. Do not patch around the contradiction inside production code.


---

## ADR-MESH-0022 — Separate tetra validity, shape quality and analysis suitability

**Status:** ACCEPTED (M6 RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-05

### Decision candidate

Dynamics26 will not represent tetrahedral quality with one undifferentiated scalar.

Separate:
1. topology/orientation validity,
2. initial geometric shape quality,
3. CAD/sizing fidelity,
4. solver/formulation suitability,
5. current nonlinear distortion.

Leading isotropic TET4 metric roles:
- exact positive orientation: hard validity,
- mean ratio: primary shape/optimization candidate,
- weighted-Jacobian condition score: solver-facing diagnostic,
- min/max dihedral: angle/sliver diagnostic,
- radius ratio: independent shape cross-check,
- radius-edge: Delaunay refinement/spacing diagnostic.

No final numerical release thresholds are accepted before M7 solver-correlation evidence.

### Rationale

Literature shows different metrics have different blind spots and FEM interpolation/conditioning
objectives do not fully agree. One bad element can also be hidden by an average.

### Evidence needed

M6-R01..M6-R11.

---

## ADR-MESH-0023 — Symbolic Delaunay lift and quality weights are different policies

**Status:** ACCEPTED (M6 RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-05

### Decision candidate

D26LIFT1 remains an infinitesimal exact-tie policy for M2.

Finite vertex weights used by a future weighted/regular-Delaunay quality or sliver-exudation method
belong to a separate M6 algorithm/policy and may not reuse D26LIFT1 semantics.

### Rationale

D26LIFT1 changes topology only at exact Delaunay ties and exists for determinism. Sliver exudation
deliberately uses finite weights to change regular triangulation for quality.

Conflating them would make a quality parameter silently change M2 mathematical identity.

### Evidence needed

M6-R12 and a future weighted-quality research package before implementation.

---

## ADR-MESH-0024 — Nonlinear distortion and incompressibility are not initial mesh quality

**Status:** ACCEPTED (M6 RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-05

### Decision candidate

For TET4 nonlinear mechanics distinguish:

    A0 = reference geometry map
    F  = deformation gradient
    J_F = det(F) physical volume ratio
    At = F A0 current geometry map.

Initial shape quality derives from A0.
Current distortion derives from At/F.
Volumetric locking is a formulation/stability phenomenon and is not cured by good initial q_MR.

A future TET4 nearly-incompressible rubber capability requires its own mixed/stabilized formulation
qualification independent of M6 geometric-quality qualification.

### Rationale

This prevents the words "Jacobian", "distortion" and "quality" from mixing geometry validity with
physical incompressibility and element formulation.

### Evidence needed

M6-R09..M6-R11 plus M7 formulation-specific tests.


---

## ADR-MESH-0025 — Combined interior reconnection and smoothing is the leading M6 optimizer architecture

**Status:** SUPERSEDED BY ADR-MESH-0039
**Date:** 2026-09-05

### Decision candidate

The first M6 quality optimizer should not be only a Delaunay flip pass or only a smoother.

Leading architecture:
1. preserve hard topology/CAD/provenance validity,
2. target poor interior tetrahedra deterministically,
3. use point-set-preserving local reconnection (2<->3 / 3<->2),
4. use general interior edge removal when elementary flips stall,
5. apply smart interior Laplacian relocation,
6. use q_MR-oriented optimization smoothing for remaining low-tail stars,
7. iterate topology and smoothing until versioned stop criteria.

The boundary remains fixed in the first implementation candidate.

### Rationale

Classical tetra-mesh experiments show connectivity changes and smoothing solve different failure
modes and are stronger in combination. Delaunay/in-sphere connectivity alone is not a final FEM
quality objective.

### Not accepted yet

The exact QualityKey, pass schedule and activation thresholds require M6-R13..R21 experiments.

---

## ADR-MESH-0026 — General interior edge removal uses an original link-polygon DP reference

**Status:** ACCEPTED (M6 RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-05

### Decision candidate

For an interior edge with N incident tetrahedra, Dynamics26 research models the edge link as a cyclic
N-gon. Removing the edge and triangulating the link produces 2N-4 tetrahedra.

The reference optimizer uses an independently derived max-min dynamic program:

    Q[i,j] = max_k min(Q[i,k], Q[k,j], w(i,k,j))

where w is the worse q_MR of the two pole tetrahedra generated by one link triangle, after exact
validity gates.

Complexity target:
- O(N^3) time,
- O(N^2) storage.

The DP result is checked against exhaustive Catalan triangulation enumeration for small N.

### Clean-room consequence

Do not copy precomputed edge-removal tables from external meshers. Any later cache/table is generated
from the committed Dynamics26 recurrence and tested against the reference DP.

### Evidence needed

M6-R14 and M6-R15.

---

## ADR-MESH-0027 — Mesh untangling is explicit recovery, not a normal post-hoc construction fix

**Status:** ACCEPTED (M6 RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-05

### Decision candidate

M6 normal generation-time improvement accepts only moves that preserve positive exact element
orientation.

A separate untangling method may research max-min signed-volume relocation for imported/remeshed
invalid local stars, but it must not silently repair an M2/M4 construction failure and then report
ordinary success.

### Rationale

Signed simplex volume is affine in one free vertex, making max-min volume an attractive convex/linear
untangling formulation. The same objective is not a good final shape metric, and silent repair would
erase the source of a meshing correctness defect.

### Evidence needed

M6-R17 plus explicit result/provenance semantics before any implementation.

---

## ADR-MESH-0028 — CAD boundary vertices have constrained mobility dimensions

**Status:** ACCEPTED (M6 DESIGN PRINCIPLE; M3/M4 BOUNDARY IMPLEMENTATION DEFERRED)
**Date:** 2026-09-05

### Decision candidate

Future M6 vertex motion is classified by authoritative CAD topology:
- volume interior: 3 DOF,
- CAD Face interior: 2 DOF constrained surface motion,
- CAD Edge interior: 1 DOF curve motion,
- CAD Vertex: 0 DOF,
- protected shared/bonded interfaces: explicit interface policy.

The first M6 implementation candidate freezes all boundary vertices until M3/M4 geometry/provenance
and surface-fidelity contracts are executable.

Display tessellation is never a projection/motion authority.

### Evidence needed

M6-R20.


---

## ADR-MESH-0029 — TET4 quality is a layered vector, not a composite scalar

**Status:** ACCEPTED (M6 RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-05

### Decision candidate

Dynamics26 shall not define tetrahedral FEM suitability by one opaque quality number.

The leading quality contract is layered:

1. exact topology/orientation validity,
2. initial isotropic element shape,
3. refinement/sizing/gradation,
4. FEM interpolation/conditioning context,
5. current nonlinear distortion,
6. element/formulation suitability.

Within the isotropic shape layer:
- q_MR is the primary optimization candidate,
- q_kappa is a mapping-condition diagnostic and is mathematically dependent on q_MR,
- q_RR is a formula-independent cross-check but belongs to an equivalent shape-regularity family,
- dihedral extrema are complementary pathology sentinels,
- rho_RE belongs primarily to Delaunay refinement/spacing.

The exact derived relation:

    q_kappa(T)^2
      = q_MR(T) q_MR(T^-1)

must be used as a verification oracle rather than as a second quality vote.

### Rationale

The analytic sliver family shows a radius-edge blind spot.

The new analytic needle family shows the complementary angle blind spot:

    q_MR -> 0
    q_kappa -> 0
    q_RR -> 0

while interior dihedral extrema tend to approximately 45 and 90 degrees.

Classical interpolation theory also permits certain degenerating tetrahedra under a maximum-angle
condition, whereas stiffness conditioning follows different geometric sensitivities.

Finally, shape-regular local refinement can alter size distribution without making individual
elements badly shaped, and nearly-incompressible locking remains a formulation problem.

Therefore one scalar cannot preserve the engineering meaning of all these effects.

### Product-language consequence

Do not infer:

    Mesh Quality = Good
      =>
    interpolation accurate
      =>
    stiffness well-conditioned
      =>
    nonlinear robust
      =>
    rubber-ready.

Each implication requires its own evidence layer.

### Evidence needed

M6-R22..M6-R29 and T4-G09.


---

## ADR-MESH-0030 — Spectral shape scores and angle morphology remain separate diagnostics

**Status:** ACCEPTED (M6 RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-06

### Decision candidate

Dynamics26 shall not infer complete tetra morphology from q_MR and q_kappa.

Reason:

    T = U Sigma V^T

has five similarity-shape DOFs after physical rotation and scale are removed.

The normalized singular spectrum Sigma contributes only two.

Therefore Layer-1 research telemetry is extended with:

    face_angle_min / face_angle_max
    dihedral theta_min / theta_max
    solid_angle_min / solid_angle_max

in addition to q_MR, q_kappa and q_RR.

The pathology names:

    sliver, wedge, cap, spire/needle, splinter, spindle, spear, spike, spade

are explanatory metadata/fixture classes, not acceptance authority.

### Spectral-duality evidence

The canonical families:

    Sigma_F = diag(1,1,epsilon)
    Sigma_N = diag(1,epsilon,epsilon)

have different q_MR asymptotics but exactly identical q_kappa.

The research-only diagnostic:

    q_MR_inv = q_kappa^2/q_MR

swaps these two spectra.

A fixed-spectrum right-rotation family additionally preserves q_MR/q_kappa while changing
angle/radius diagnostics.

### FEM interpolation consequence

Tetrahedral maximum-angle theory has independent:
- triangular-face maximum-angle,
- dihedral maximum-angle

conditions.

Therefore dihedral-only quality telemetry cannot stand in for interpolation suitability.

### Acceptance evidence

M6-R30..M6-R36.

No production metric threshold or optimizer implementation is authorized by this ADR.


---

## ADR-MESH-0031 — Strong reconnection is a bounded transactional escape from local optimization traps

**Status:** ACCEPTED (M6 RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-06

### Decision candidate

Dynamics26 defines local optimality relative to an explicit operation family O:

    M is O-local optimum
    iff
    no one-operation legal neighbor in N_O(M)
    strictly improves the versioned quality order.

Therefore:
- elementary-flip local optimum,
- edge-removal local optimum,
- fixed-cavity retriangulation optimum

are different statements.

The leading M6 architecture uses cheap operations first and reserves general fixed-cavity strong
reconnection for residual low-tail traps.

### Transaction rule

Composite/strong search may cross non-improving intermediate states only in private temporary state.

Committed mesh history remains:
- exact-valid,
- provenance-safe,
- strictly quality improving.

Pattern:

    Plan
    -> Search
    -> Validate
    -> Compare final
    -> Commit or Discard.

### Fixed-cavity SPR research objective

First reference strong-search objective:

    maximize over legal triangulations
      minimum q_MR.

Reason:
the partial minimum of a branch is an admissible upper bound for max-min branch-and-bound pruning.

Secondary aggregate objectives are deferred until their pruning/correctness semantics are derived.

### Cavity-selection boundary

An exhaustive optimum for a fixed cavity does not imply global mesh optimality.

Report separately:
- cavity-selection policy,
- fixed-cavity search completeness,
- effort-budget result.

### Termination boundary

For fixed coordinates, fixed finite sites and point-set-preserving connectivity operations, strict
quality improvement is cycle-free because the legal connectivity state space is finite.

This proof does not cover:
- smoothing,
- point insertion/deletion.

Those need explicit convergence/resource policies.

### First implementation implication

This ADR does not authorize SPR implementation.

Before strong reconnection production work, Dynamics26 should qualify:
- elementary/edge reconnection,
- smoothing,
- exact cavity validation,
- deterministic QualityKey behavior.

SPR/multi-face remain research and oracle tracks.

### Evidence needed

M6-R37..M6-R48.


---

## ADR-MESH-0032 — Commit acceptance uses exact sorted mean-ratio order; aggregate objectives generate proposals

**Status:** ACCEPTED (M6 RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-06

### Decision candidate

For a legal changed cavity C, define:

    QMRVector(C)
      =
    sorted q_MR values from worst to best.

Candidate commit requires:

    QMRVector(new)
      >_lex
    QMRVector(old).

Rules:
- first differing q_MR decides,
- if one vector is an exact prefix of the other, shorter wins,
- exactly equal vectors do not mutate topology,
- canonical connectivity only breaks ties among multiple equally improving candidates.

### Why not q_min alone

q_min is the first component and remains strongly protected.

But QMRVector can also improve:
- second worst,
- third worst,
- deeper low-tail elements

when q_min itself is unchanged.

### Why not an aggregate mean

Arithmetic/harmonic/inverse-mean objectives can:
- improve while the worst tetrahedron becomes worse,
- reject a useful second-worst improvement because high-quality cells decrease.

They remain valuable differentiable proposal objectives, especially for smoothing.

### Exact q_MR order

For positive tetrahedron:

    q_MR
      =
    12(3V)^(2/3)/S

with:

    D = 6V
    S = sum of six squared edge lengths.

Then:

    q_MR^3
      =
    432 D^2/S^3.

Hence the exact quality order is the sign of:

    D_A^2 S_B^3
      -
    D_B^2 S_A^3.

Canonical binary64 coordinates make D and S exact dyadic rationals under exact evaluation.

This gives a research path to a deterministic exact q_MR comparator without pow/cube-root or
pairwise epsilon equality.

### Local/global theorem

If a local changed submesh quality multiset improves lexicographically, adding the identical unchanged
mesh-quality multiset to old and new preserves the comparison.

Therefore local cavity comparison is sufficient for global monotone QualityVector improvement.

### Strong-search update

Max-min remains an independent first-component oracle.

For full lexicographic SPR search, a partial sorted QMRVector padded with +infinity is an admissible
upper bound for every completion.

Therefore lexicographic branch-and-bound is a valid research target.

The earlier max-min equality prune applies only to max-min-only optimization.

### Termination consequence

For fixed coordinates/sites and point-set-preserving connectivity:
- finite state space,
- strict exact QMRVector improvement,
- no mutation on equality

imply cycle-free finite accepted mutation history.

Smoothing remains continuous and still needs independent convergence controls.

### Policy placeholders

Research-only candidate version names:

    D26QMR1
    D26QV1
    D26QACC1.

They are not accepted API contracts.

### Evidence needed

M6-R49..M6-R60.

No production M6 implementation is authorized.


---

## ADR-MESH-0033 — D26QMR1 uses exact binary64-lattice ordering with certified filtered fallback

**Status:** ACCEPTED (M6 RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-06

### Decision candidate

D26QMR1 is an exact **mean-ratio ordering** contract.

It does not attempt to construct an exact real-valued q_MR scalar.

For a positive tetrahedron:

    q_MR^3
      =
    432 D^2/S^3.

Therefore authoritative comparison reduces to:

    compare(
      D_A^2/S_A^3,
      D_B^2/S_B^3
    ).

### Exact reference representation

Every finite binary64 coordinate is represented exactly on the common lattice:

    x
      =
    I_x 2^-1074.

For one tetra:
- remove exact translation,
- divide the common integer gcd of coordinate differences,
- compute primitive integer determinant magnitude d,
- compute primitive integer six-edge squared sum s.

Then:

    q_MR^3
      =
    432 d^2/s^3

and pairwise order is:

    sign(
      d_A^2 s_B^3
      -
      d_B^2 s_A^3
    ).

No root, division or quality epsilon is required by the exact order.

### Validity boundary

Exact M1/M2 validity precedes quality comparison.

Because d^2 hides determinant sign:
- inverted tetrahedra may not enter D26QMR1 acceptance,
- zero orientation is a geometry failure, not a quality tie.

### Arithmetic bound

For primitive pairwise scalar edge-component bit width B:

    B <= 2099

for arbitrary finite binary64 inputs.

The exact comparison magnitude has the conservative bound:

    < 12B+22 bits

and therefore below approximately 25210 bits at the input-format worst case.

This is an arithmetic-size bound, not a performance claim.

### Filtered acceleration candidate

A later fast path may:
1. power-of-two normalize each tetra independently,
2. compute a certified outward interval for q_MR^3,
3. return Less/Greater only for disjoint intervals,
4. return Uncertain otherwise,
5. invoke the exact lattice comparator.

Interval overlap is never equality.

Only the exact stage may return Equal.

### Environmental boundary

The exact lattice oracle depends on stored binary64 bits and exact integer arithmetic.

A floating filter additionally requires a qualified:
- radix/precision contract,
- rounding environment,
- subnormal behavior,
- compiler contraction/FMA policy.

Do not let fast-math or unproved interval shortcuts change the authoritative sign.

### Backend boundary

This ADR freezes semantics, not the implementation backend.

Candidates remain:
- arbitrary-precision integer exact fallback,
- floating-expansion exact fallback,
- interval/static filter variants.

The exact primitive-lattice oracle is the qualification reference even if a different backend is later
selected for production.

### Evidence needed

M6-R61..M6-R74.

No production M6 code is authorized.


---

## ADR-MESH-0034 — D26QMRF1 first qualifies a dynamic interval cross-polynomial filter

**Status:** ACCEPTED (M6 RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-06

### Decision candidate

The first accelerated Dynamics26 mean-ratio order path should be:

    exact-positive validity
      ->
    D26QMRF1 dynamic interval filter
      ->
    D26QMR1 exact fallback.

The filter evaluates:

    F(A,B)
      =
    D_A^2 S_B^3
      -
    D_B^2 S_A^3.

It returns only:

    Less
    Greater
    Uncertain.

Only exact D26QMR1 may return Equal.

### Why the cross polynomial

Compared with filtering:

    D^2/S^3

directly, F:
- removes division,
- removes denominator inversion,
- removes cube-root concerns,
- is polynomial in the geometric quantities.

### Independent normalization theorem

For positive independent scales alpha,beta:

    F(alpha A,beta B)
      =
    alpha^6 beta^6
    F(A,B).

Therefore tetra A and B may be normalized independently by exact powers of two without changing the
sign.

This is the core range-control mechanism for the filter.

### Qualified dynamic range

If normalized anchor-relative scalar components satisfy:

    |r| < 1,

then:

    D^2 < 27
    S < 72

and each cross term is below:

    10,077,696.

Hence:

    |F| < 20,155,392.

This eliminates ordinary mathematical overflow from the normalized comparison expression.

It does not eliminate:
- interval widening,
- subnormal component risk,
- floating-environment requirements.

### Interval authority

D26QMRF1 may certify a sign only if its interval enclosure excludes zero.

If the interval:
- overlaps zero,
- encounters unsupported range,
- encounters unsupported subnormal behavior,
- cannot satisfy the frozen compiler/rounding contract,

the result is Uncertain and exact fallback is mandatory.

### Semi-static policy

A future semi-static filter is desirable for performance but is **not** accepted first.

Paper-analysis candidates such as:

    gamma_10 * determinant permanent
    gamma_20 * edge-sum scale

must be independently/formally certified for the actual expression tree.

No copied Orient3D/InSphere constant and no generic K*epsilon threshold is allowed.

### FMA/compiler boundary

FMA and non-contracted arithmetic are different expression contracts.

The proof must match the code.

Implicit compiler contraction may not alter a certified expression tree.

Unsafe fast-math cannot be enabled for the filter unless a new proof explicitly supports it.

### Repository observation

At this research point, default-branch search found no occurrences of:
- -ffast-math,
- fast-math,
- fp-contract,
- FENV_ACCESS.

This is not a permanent guarantee; future implementation must add explicit CI/compiler gates.

### Long-term cascade

Possible later optimized cascade:

    optional D26QMRS1 semi-static filter
      ->
    D26QMRF1 dynamic interval filter
      ->
    D26QMR1 exact.

First qualification remains the simpler two-stage filtered/exact path.

### Evidence needed

M6-R75..M6-R90.

No production M6 filter code is authorized.


---

## ADR-MESH-0035 — D26INT1 uses RN per-primitive adjacent-representable widening

**Status:** ACCEPTED (M6 RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-06

### Decision candidate

The first interval backend for D26QMRF1 should be:

    D26INT1
      =
    FE_TONEAREST
      +
    per-primitive adjacent-representable outward widening
      +
    fixed expression trees
      +
    environment guards
      +
    exact D26QMR1 fallback.

It does not mutate the rounding mode.

### Primitive-local proof

For one correctly rounded finite binary64 primitive result:

    r = RN(z),

the exact result z is enclosed by the two adjacent representable neighbors around r.

Therefore:
- addition,
- subtraction,
- multiplication

may use one-step outward widening per endpoint primitive.

The proof does **not** authorize:
- evaluating a multi-operation expression,
- widening only the final result by one step.

Compound expressions must compose certified interval primitives.

### Fixed trees

Research freezes explicit trees for:
- 3x3 determinant,
- balanced six-edge squared-length sum,
- final D26QMRF1 cross polynomial.

A changed arithmetic tree is a changed filter contract until requalified.

### Compiler boundary

Clang's default precise model can permit FP contraction.

Therefore:

    repository has no explicit fast-math flag

does not prove a non-FMA expression tree.

When implementation is authorized, D26INT1 requires an explicit non-contraction/reassociation policy
for the certified kernel.

Explicit FMA belongs to separate D26INTE1 research.

### Runtime environment

D26INT1 fast path requires:
- binary radix 2,
- 53-bit double precision,
- IEC-60559-compatible binary64 capability,
- FE_TONEAREST,
- demonstrated gradual-subnormal behavior,
- finite supported operands.

If a requirement is not established:

    UncertainEnvironment
      ->
    exact D26QMR1 fallback.

### Apple Silicon boundary

AArch64 FPCR contains:
- RMode,
- FZ.

Apple XNU arm64 definitions expose corresponding fields.

Therefore Apple Silicon is capable of runtime states incompatible with the filter assumptions.

Do not rely on platform name as proof of:
- RN mode,
- gradual underflow.

Portable behavior probes remain part of qualification.

### Adjacent-value backend

Reference:

    std::nextafter.

Future optimization may use exact binary64 bit stepping to implement:
- nextDown,
- nextUp

without libm/exception-flag side effects.

Such an optimization requires exhaustive equivalence against the reference behavior.

### Normalization

Reference power-of-two scaling uses std::scalbn semantics.

For positive finite M:

    k = -ilogb(M)-1

targets:

    1/2 <= scalbn(M,k) < 1.

Unsupported range behavior returns UncertainRange.

### Alternative backends

Research placeholders:

    D26INTD1
      dynamic directed rounding

    D26INTE1
      EFT / explicit FMA.

Neither is the first qualification dependency.

### Evidence needed

M6-R91..M6-R108.

No production interval implementation is authorized.


---

## ADR-MESH-0036 — D26QMRB1 reuses the qualified M1 dyadic BigInt mechanism

**Status:** ACCEPTED (M6 RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-06

### Decision candidate

The first exact backend for D26QMR1 should be:

    D26QMRB1
      =
    shared/internal Dynamics26 dyadic BigInt arithmetic

derived from the already-qualified M1 exact predicate mechanism.

This is preferred over introducing a new expansion or external multiprecision backend before
telemetry demonstrates a need.

### Repository-truth correction

Current M1 production exact predicates do not use floating expansions.

They use:
- exact binary64 dyadic decoding,
- per-call common exponent,
- signed arbitrary-precision integer coordinates,
- exact integer determinant.

Therefore reuse M1 expansion arithmetic is not a valid description of current code.

### No-gcd theorem

For one tetra's exact common scale alpha:

    D = alpha^3 d
    S = alpha^2 s.

Thus:

    D^2/S^3
      =
    d^2/s^3.

Different tetrahedra may use different positive alpha values.

Therefore D26QMR1 does not require:
- gcd,
- arbitrary-precision division,
- reduced rational storage.

Primitive gcd normalization remains optional compression only.

### Shared-kernel boundary

Future refactoring should share only arithmetic mechanism:

    Binary64 dyadic decode
    SignedBigInt
    exact integer helpers.

Policy remains separate:
- M1 owns robust predicate semantics,
- M6 owns q_MR order semantics.

BigInt must remain internal and absent from installed/public ABI.

### Fixed exact expression

D26QMR1 should use:
- exact edge differences,
- fixed 3x3 determinant,
- exact six-edge squared sum,
- unreduced (d^2,s^3) or equivalent lazy key,
- exact cross multiplication.

M1's generic recursive determinant remains a useful qualification cross-check, not the intended
performance architecture for M6.

### Cache policy

No permanent global exact-key cache is accepted first.

Leading policy:
- local/cavity/pass-scoped memoization,
- strongest use in SPR and edge-removal search,
- no stale PointId-only reuse after smoothing coordinates move.

Worst-case unreduced (d^2,s^3) raw payload is approximately 3156 bytes per tetra with the current
32-bit-limb model, before container overhead.

This is sufficient reason to require telemetry before global caching.

### Alternative backends

D26QMRE1 — Shewchuk-style expansions:
- mathematically credible,
- new backend family,
- experimental until fallback telemetry justifies it.

D26QMRX1 — external arbitrary precision such as Boost cpp_int:
- credible independent oracle/benchmark,
- not selected as first dependency.

No GMP/runtime backend is selected without evidence that exact arithmetic dominates end-to-end cost.

### Promotion rule

Any alternative must show:
- identical exact order/ties,
- deterministic replay,
- acceptable compiler/dependency contract,
- material optimizer-level benefit.

Microbenchmark speed alone is insufficient.

### Evidence needed

M6-R109..M6-R124.

No production refactor or M6 exact comparator implementation is authorized.


---

## ADR-MESH-0037 — Quality keys are coordinate-state artifacts; parallel M6 uses deterministic snapshot rounds

**Status:** ACCEPTED (M6 RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-06

### Decision candidate

Dynamics26 separates:
- exact q_MR numeric identity,
- topology-storage identity,
- constraint legality,
- optimizer scheduling state.

Research placeholders:

    D26QKEY1
      QualityKey lifecycle/cache domain

    D26QSCHED1
      deterministic round scheduler.

### QualityKey identity

For fixed coordinate state:

    CanonicalTetKey
      =
    sorted four PointIds

is sufficient tetra identity for q_MR caching inside the same coordinate/snapshot domain.

Quality identity does not include:
- TetHandle slot,
- TetHandle generation,
- visitEpoch,
- adjacency.

A same-four-point tetra recreated in another slot has the same numeric q_MR key if point coordinates
are unchanged.

### Coordinate invalidation

Accepted smoothing changes the numeric geometry.

Every tetra containing the moved point has stale q_MR cache state.

First implementation need not introduce per-point coordinate generations:
- keep connectivity-phase caches local,
- use proposal-local caches for smoothing,
- rebuild affected-star keys after accepted motion.

Future coordinateGeneration(PointId) is an optimization candidate.

### Constraint separation

Changing protected/CAD/size legality without changing coordinates:
- does not change numeric q_MR,
- does invalidate operation proposals/legality caches.

Therefore a combined "quality + legality" cache is forbidden as the first design.

### Parallel scheduling

First parallel optimizer architecture:

    immutable round snapshot
      ->
    parallel private proposal planning
      ->
    explicit semantic read/write footprints
      ->
    deterministic total ScheduleKey
      ->
    deterministic conflict-free winner set
      ->
    ordered revalidation/commit
      ->
    affected-neighborhood rebuild.

The first performance target is parallel evaluation/search, not simultaneous authoritative mutation.

### Conflict rule

Proposals conflict on:
- write/write overlap,
- write/read overlap.

Read/read overlap is allowed.

Topology write footprints include outside neighbor records patched by commit.

False conflicts only reduce performance.

False non-conflicts threaten correctness and are unacceptable.

### Deterministic reference selection

Reference winner selection is deterministic greedy MIS under the total ScheduleKey.

The ScheduleKey excludes:
- thread id,
- completion time,
- pointer address,
- hash iteration,
- wall time.

### Serializability boundary

Pairwise non-conflicting proposals planned on one immutable snapshot commute over their declared
semantic state.

This supports a serializable selected round.

It does not prove equivalence to a different serial optimizer that regenerates all proposals after
every individual commit.

D26QSCHED1 is a versioned round-based algorithm and must be tested as such.

### Commit policy

First qualification commits selected winners in canonical ScheduleKey order.

Reason:
- semantic operations may commute while storage allocation/order effects do not,
- M2 already values deterministic append/write ordering.

Future parallel commit requires deterministic slot/range preassignment and canonical-fingerprint
equivalence.

### Cache concurrency

First mutable quality caches are:
- proposal local,
- cavity local,
- worker local where safe.

Shared immutable coordinate/dyadic tables are acceptable.

A shared mutable global QualityKey cache is deferred until telemetry proves value.

Cache hits/misses must never affect semantic result.

### Evidence needed

M6-R125..M6-R146.

No production cache, scheduler or parallel M6 implementation is authorized.


---

## ADR-MESH-0038 — Non-conflicting strict local winners make every non-empty round globally strict-monotone

**Status:** ACCEPTED (M6 RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-06

### Decision candidate

D26QV1 is compatible with multiset union:

    B > A
      =>
    B union C > A union C.

Therefore for a conflict-free selected round:

    A_i -> B_i
    with
    B_i > A_i

for every committed winner, repeated union compatibility and transitivity give:

    GlobalNew > GlobalOld.

This is the authoritative parallel-round progress invariant.

### Scheduler dependency

The proof requires each local old/new comparison to remain valid through the selected batch.

D26QSCHED1 provides this by:
- immutable planning snapshot,
- complete semantic read/write footprints,
- pairwise non-conflicting winners,
- ordered validated commit.

### Binary64 termination refinement

For first-scope fixed finite PointIds:
- coordinates are finite canonical binary64,
- connectivity state is finite,
- every accepted topology/smoothing commit strictly improves global D26QV1.

Therefore the accepted authoritative state sequence is finite.

This strengthens the earlier continuum-only smoothing discussion.

Private search routines still require:
- finite enumeration,
- or count-bounded iterations/branch search.

### Active-set reference

D26ACTREF1 globally rebuilds eligible active targets after each committed round.

Incremental invalidation is an optimization and must match the reference.

No fixed one-ring/two-ring radius is frozen as architecture truth; semantic dependency footprints own
invalidation correctness.

### Evidence needed

M6-R147..M6-R160.

No production scheduler implementation is authorized.

---

## ADR-MESH-0039 — D26OPS1 freezes a cheap-cycle-first point-set-preserving optimizer schedule

**Status:** ACCEPTED (M6 RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-06

### Decision candidate

First Dynamics26 M6 operation order:

    O1 Smart interior smoothing
      ->
    O2 2->3 face flips
      ->
    O3 general edge-removal DP
      ->
    O4 optimization-based interior smoothing
      ->
    repeat cheap/local cycle while progress
      ->
    O5 bounded SPR on stall
      ->
    if O5 succeeds, return to O1
      ->
    otherwise stop/status.

General edge removal includes:
- N=3 -> 3->2,
- N=4 -> 4->4,
- larger legal edge-star reconnections.

### Why this order

Literature consistently supports combining smoothing and topology.

HXT specifically prioritizes smoothing and edge removal, then invokes Growing SPR as a slower
last-resort escape from local maxima.

Klingner/Shewchuk likewise combine smoothing/topological transformations and reserve stronger
composite operations for difficult local optima.

Dynamics26 adds exact D26QV1 commit acceptance and deterministic round scheduling.

### Scope

First D26OPS1 is:
- fixed point set,
- fixed CAD/boundary coordinates,
- point-set-preserving topology,
- interior smoothing,
- bounded fixed-cavity SPR.

Deferred:
- point insertion/deletion,
- edge contraction,
- boundary smoothing/projection,
- weighted sliver exudation,
- anisotropic metric-space optimization.

### Threshold boundary

D26OPS1 does not freeze a universal q_MR cutoff.

Target activation is separate.

Reference exhaustive mode may examine every finite tetrahedron.

Targeted/product modes must scope their local-optimum claims to the activated set.

### Status boundary

Bounded strong search distinguishes:
- ExhaustiveNoImprovement,
- SearchBudgetExhausted,
- ResourceFailure,
- ConstraintBlocked.

No resource limit is converted into a geometry/quality conclusion.

### Evidence needed

M6-R161..M6-R170.

No production optimizer implementation is authorized.


---

## ADR-MESH-0040 — M6 tetra-quality / deterministic-optimizer research design freeze

**Status:** ACCEPTED (RESEARCH/DESIGN FREEZE)
**Date:** 2026-09-06

### Decision

M6 research is closed under freeze identifier:

    D26-M6-FREEZE-1.

Authoritative closeout:

    docs/research/meshing/m6-quality/M6_DESIGN_FREEZE.md

with qualification grouping:

    docs/research/meshing/m6-quality/M6_QUALIFICATION_GATE_MATRIX.md.

### Meaning

Accepted:
- M6 mathematical and architecture contracts are frozen for first implementation,
- superseded candidate text is non-authoritative,
- M6-R01..M6-R170 define the complete qualification contract.

Not claimed:
- production M6 optimizer implemented,
- M6 executable gates passed,
- generic q_MR FEM/rubber threshold validated.

### Change control

Any change to frozen D26QMR1, D26QV1, D26QACC1, D26QMRF1, D26INT1, D26QMRB1, D26QKEY1,
D26QSCHED1, D26ACTREF1 or D26OPS1 semantics requires a new ADR and affected-gate update.

### Next phase

Proceed to:
- implementation specification,
- qualification-harness implementation,
- serial reference implementation before parallel optimization,
- M7 solver-correlation evidence for numeric product thresholds.


---

## ADR-MESH-0041 — O3 edge removal optimizes full D26QV1 by union-compatible polygon DP

**Status:** ACCEPTED (M6 DESIGN-FREEZE CLARIFICATION)  
**Date:** 2026-09-06

### Context

Implementation-spec audit found a historical mismatch:
- frozen D26OPS1 requires O3 to find the best legal edge-removal triangulation under D26QV1,
- EDGE_REMOVAL_DYNAMIC_PROGRAMMING.md still described the earlier scalar max-min recurrence as the
  main algorithm.

### Decision

For link-polygon subproblem [i,j]:

    V[i,i+1] = empty

and for split i<k<j:

    C(i,k,j)
      =
    merge_sorted(
      V[i,k],
      V[k,j],
      W(i,k,j)
    )

where W contains the exact D26QMR1 qualities of the two pole tetrahedra.

Then:

    V[i,j]
      =
    max_D26QV1 over valid k
      C(i,k,j).

### Why optimal substructure holds

D26QV1 is compatible with multiset union:

    A > B
      =>
    A union C > B union C.

Therefore replacing either subpolygon by its D26QV1-better completion cannot worsen the complete
candidate for a fixed split.

### Max-min role

The historical max-min recurrence remains:
- first-vector-component oracle,
- qualification cross-check,
- optional performance diagnostic.

It is not authoritative O3 proposal selection.

### Qualification

M6-R39 and M6-R162 are clarified accordingly.

---

## ADR-MESH-0042 — First M6 implementation remains a private femcae_meshing subsystem

**Status:** ACCEPTED (IMPLEMENTATION SPEC)  
**Date:** 2026-09-06

### Decision

Before M6 Q9 qualification:
- M6 exact arithmetic, quality keys, optimizer proposals, scheduler and caches remain private to
  femcae_meshing,
- private headers live under src/meshing/internal and src/meshing/m6,
- no SignedBigInt/Dyadic internal type is installed,
- no new GUI/SimulationMesh optimizer authority is introduced.

### Exact arithmetic extraction

The existing M1 BigInt/dyadic mechanism is extracted as a shared internal mechanism only.

Policy ownership remains:
- M1 RobustPredicates owns predicate semantics,
- M6 owns quality-order semantics.

### Tetra state boundary

The first M6 reference state is derived from:
- CanonicalSite / PointId coordinates,
- TetSlot topology,
- explicit fixed/protected constraint input.

It does not use current Hex8/Quad4-oriented SimulationMesh as authoritative tetra optimizer storage.

### Public API timing

A public/install quality/optimizer facade may be designed only after deterministic/reference
qualification and requires a separate API review.

### Implementation authority

Detailed repository mapping:

    M6_IMPLEMENTATION_SPEC.md
    M6_IMPLEMENTATION_SEQUENCE.md.


## ADR-MESH-0043 — Coplanar ghost circles preserve the 3D Euclidean lift

**Status:** ACCEPTED — explicit correction of an M2 frozen derivation defect  
**Date:** 2026-09-06

Independent P1 audit reproduced a metric error in both the frozen projected-circle
wording and P1A implementation. Orthographic projection is not generally an
isometry of an oblique plane. Use the first nonzero XY/XZ/YZ orientation only to
choose coordinates; evaluate `[u v x*x+y*y+z*z 1]` by exact dyadic arithmetic.
On genuine zero use the same projected orientation cofactors and ascending
PointId lift priority. Spatial coordinates, M1 predicates and D26LIFT1 formal
perturbation are unchanged. This corrects Euclidean geometry, not site ordering.

Authority, proof, counterexample and executable symbols:
`m2-delaunay/M2_1A_COPLANAR_METRIC_CORRECTION.md`.
Reopen M2-G09 and the coplanar portion of M2-G10 until the corrected source HEAD
passes macOS arm64 Debug/Release. Old evidence remains historical and cannot
qualify oblique hulls. P1C must wait for prerequisite repairs and qualification.


## ADR-MESH-0044 — Physical CAD edge chains are shared; pcurves belong to FaceUse

**Status:** ACCEPTED (M3 RESEARCH/DESIGN FREEZE)  
**Date:** 2026-09-06

### Decision

A physical CAD Edge is discretized once into one ordered 3D `PhysicalEdgeChain` with Edge `GeometryEntityId` provenance. Each incident Face consumes that physical chain through a distinct FaceUse/coedge that owns orientation and face-specific pcurve/UV representation.

The architecture `CAD Edge -> one universal UV polyline` is rejected.

### Rationale

One edge may be used by several Faces, reversed per use, or appear as a seam with more than one pcurve on one Face. Physical conformity and chart representation therefore require separate identities.

### Consequence

Adjacent Faces cannot independently recreate shared-edge physical nodes. M3-G02..G05 qualify this contract.

## ADR-MESH-0045 — Chart aliases may map many-to-one to physical MeshNode identity

**Status:** ACCEPTED (M3 RESEARCH/DESIGN FREEZE)  
**Date:** 2026-09-06

### Decision

`ChartVertexId -> MeshNodeId` may be many-to-one for seams, periodic surfaces, poles and degenerated edges. Final physical TRI3 acceptance requires three distinct `MeshNodeId` values.

Face-local CAD-tolerance reconciliation may canonicalize pcurve endpoints before triangulation; that tolerance is not a general FEM topology epsilon.

### Rationale

Distinct parameter-space points can represent one physical CAD point. Conflating chart identity with physical mesh identity either tears shared geometry or creates invalid zero-area physical triangles.

## ADR-MESH-0046 — M4 recovers constraints as exact subcomplex coverage

**Status:** ACCEPTED (M4 RESEARCH/DESIGN FREEZE)  
**Date:** 2026-09-06

### Decision

Recovery does not require one input segment/facet to remain one output segment/facet. An original constraint is satisfied when the union of its recovered child subcomplex equals the original constraint, with complete coverage, no gaps, no illegal overlap or leakage, and inherited provenance.

### Rationale

Robust boundary recovery may require Steiner subdivision. One-to-one facet preservation would reject valid conforming recovery strategies.

### Consequence

Segment and facet coverage proofs are first-class M4 qualification gates.

## ADR-MESH-0047 — SegmentLNC is the first exact constructed-point representation

**Status:** ACCEPTED (M4 RESEARCH/DESIGN FREEZE)  
**Date:** 2026-09-06

### Decision

The first M4 constructed site is:

```text
P = A + t(B-A)
```

A/B are authoritative constraint endpoints and t is an exact canonical dyadic parameter. Construction identity and exact dyadic value are authoritative; binary64 Vec3 is realization/cache only. Endpoint values t=0 and t=1 canonicalize to the corresponding `ExplicitSite`.

### Rationale

Binary64 endpoints are exact dyadics, so dyadic segment interpolation remains inside the existing BigInt/dyadic exact arithmetic domain. Rounded Steiner coordinates therefore need not become topology authority.

### Future extension

General homogeneous implicit points may be added as a separate research/oracle extension without changing SegmentLNC semantics.

## ADR-MESH-0048 — Constructed sites use versioned D26SITE2 / D26LIFT2 symbolic identity

**Status:** ACCEPTED (M4 RESEARCH/DESIGN FREEZE)  
**Date:** 2026-09-06

### Decision

D26LIFT1 is not silently extended. The constructed-site domain introduces:
- `D26SITE2`: canonical total site identity/order,
- `D26LIFT2`: symbolic Delaunay degeneracy policy over D26SITE2.

Keys distinguish:

```text
ExplicitSite
  -> canonical explicit-site identity

ConstructedSegmentSite
  -> canonical source constraint-segment identity
  -> exact canonical dyadic t
```

The first versioned kind order is `ExplicitSite < ConstructedSegmentSite`; order within a kind is lexicographic over its canonical fields.

### Determinism

The order is independent of pointer, allocation order, arena slot, thread, task completion order and wall-clock insertion time.

### Predicate rule

D26LIFT2 participates only when the exact geometric predicate is genuinely zero. It never overrides a nonzero exact sign.

## ADR-MESH-0049 — M5 scalar sizing criteria compose by mathematical minimum

**Status:** ACCEPTED (M5 RESEARCH/DESIGN FREEZE)  
**Date:** 2026-09-06

### Decision

The first M5 physical size request is scalar/isotropic and composes as the pointwise minimum of global, scoped, curvature, proximity and future scalar criteria. Overlapping rule enumeration order cannot change the result.

Named Selection scopes resolve to `GeometryEntityId` before kernel evaluation.

### Consequence

Anisotropic tensor sizing is not part of this policy and remains deferred to M9.

## ADR-MESH-0050 — M5 gradation is the greatest k-Lipschitz minorant

**Status:** ACCEPTED (M5 RESEARCH/DESIGN FREEZE)  
**Date:** 2026-09-06

### Decision

The authoritative gradated field is:

```text
h(x) = inf_y [ h_req(y) + k ||x-y|| ]
```

This is the greatest k-Lipschitz function not exceeding `h_req`.

### Consequence

Implementation may change, but the semantics may not. Any GUI growth-ratio representation maps to k through a versioned policy included in the settings fingerprint.

## ADR-MESH-0051 — Minimum size is a typed limitation, not silent criterion satisfaction

**Status:** ACCEPTED (M5 RESEARCH/DESIGN FREEZE)  
**Date:** 2026-09-06

### Decision

When geometry or another criterion requires `h < h_min`, a protection clamp may stop refinement but cannot yield `CriteriaSatisfied`. The result must expose `MinimumSizeLimited` (or a versioned equivalent) and the unsatisfied criteria/features.

### Rationale

Minimum size protects resources; it is not evidence that CAD deviation, proximity or other requested criteria were met.

## ADR-MESH-0052 — Product TET4 requires a versioned data-contract migration

**Status:** ACCEPTED (M7 RESEARCH/DESIGN FREEZE)  
**Date:** 2026-09-06

### Decision

TET4 product support is a cross-layer data-contract migration, not one `MeshTopology` enum addition. `SimulationMesh`, TRI3 boundary facets, Fortran topology/reference metadata, `AnalysisSnapshot`, `SolverInputBuilder`, ABI/schema, result handling, surface integration and persistence may all require versioned migration.

```text
TET4 topology != TET4 mechanical formulation
```

remains mandatory.

### Current repository evidence

At closeout baseline `60275d9430b13a3cf3b3fdd26709dee11e3d7f8b`, product contracts remain HEX8/QUAD4-oriented and the solver-input/product bridge remains HEX8-specific.

### Consequence

DEV-MESH-P6 owns product integration after P1-P5 dependencies. Dedicated TET4 formulation qualification remains under `docs/research/fem/tet4-nearly-incompressible/`.


## ADR-MESH-0053 — Research-closeout clarification: oriented Face use, Region transitions and physical-domain sizing

**Status:** ACCEPTED (RESEARCH CLOSEOUT HARDENING)  
**Date:** 2026-09-06

### Decision

The D26-M3/M4/M5 freezes are clarified without authorizing implementation:

- M3: `Su x Sv` is only the parametric normal. Final TRI3 winding follows the oriented CAD
  Face/shell use. One `PhysicalEdgeChain` is authoritative per geometry revision + meshing
  transaction; later refinement may replace that authority only coherently for all incident FaceUses.
- M4: domain classification is an explicit typed Region/shell state transition. Unbounded ghost is
  OUTSIDE, unconstrained crossings preserve state, oriented exterior/nested shells change declared
  material state, and a RegionA <-> RegionB transition exists only for an explicitly declared
  interface. Ambiguous/non-manifold/undeclared ownership fails explicitly.
- M5: inactive criteria equal `+infinity`; `h_req` is the minimum over active criteria. Gradation
  is defined on the declared physical domain using 3D Euclidean distance. First-scope propagation is
  component-local: disconnected Body/Region components do not influence one another unless a future
  versioned policy explicitly couples them. Curvature predictor domains are explicit and NaN is
  never topology/sizing authority.

### Consequence

These clarifications harden qualification fixtures and settings/lifecycle fingerprints. They do not
implement M3/M4/M5, do not alter D26SITE1/D26LIFT1, and do not promote M2 or any downstream package
to qualified status.
