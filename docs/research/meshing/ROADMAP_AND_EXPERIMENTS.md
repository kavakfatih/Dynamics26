# Original Meshing R&D Roadmap and Experiment Program

## 1. Program objective

Develop a verified original Dynamics26 unstructured meshing system without destabilizing the existing StructuredHexMesher baseline.

The structured HEX8 path remains available for current validated workflows while the new mesher matures behind research/experimental capability gates.

## 2. Work packages

### M0 — Knowledge library and contracts

Status: **ACTIVE**

Deliverables:

- source registry,
- algorithm taxonomy,
- clean-room policy,
- commercial benchmark,
- open-source architecture studies,
- original architecture proposal,
- benchmark corpus definition.

Exit:
- research documents committed,
- first ADR recorded.

### M1 — Robust predicates and geometry kernel utilities

Status: **QUALIFIED**

Research package:
- `docs/research/meshing/m1-robust-geometry/`

Deliver:

- orient2d/orient3d,
- incircle/insphere,
- deterministic filtered/adaptive strategy,
- adversarial tests,
- spatial search baseline,
- point/triangle/tetra primitives independent of OCCT.

Test corpus:

- exact coplanar/cospherical,
- epsilon perturbations,
- scale range,
- coordinate translations,
- randomized property tests.

#### M1.1 — Independent exact predicate oracle

Status: **QUALIFIED**

Research package:
- `docs/research/meshing/m1-robust-geometry/m1.1-exact-oracle/`

Deliver:
- exact rational oracle,
- exact dyadic-integer oracle,
- exact binary64 fixture serialization,
- naive-double adversarial corpus,
- filter experiment plan,
- ANSYS / COMSOL / Marc tolerance/repair cross-check.

Exit:
- two exact oracle paths agree,
- fixture round-trip is bit-exact,
- commercial repair/tolerance semantics remain separate from predicate truth.

#### M1.3 — Degeneracy & deterministic symbolic perturbation

Status: **QUALIFIED (M1 FOUNDATION)**

Research package:
- `docs/research/meshing/m1-robust-geometry/m1.3-degeneracy/`

Deliver:
- degeneracy taxonomy,
- exact duplicate canonicalization policy,
- affine-dimension behavior,
- stable PointId semantics,
- SoS-style symbolic Delaunay tie specification,
- permutation/insertion-order determinism experiments,
- ANSYS / COMSOL / Marc degeneracy cross-check.

Exit:
- exact duplicates and lower-dimensional inputs have explicit behavior,
- symbolic policy is formal rather than heuristic,
- exact symbolic oracle can verify tie decisions,
- canonical topology determinism target is testable.

#### M1.4 — Spatial search, point location & tetra topology

Status: **QUALIFIED (M1 PRIMITIVES; M2 SEARCH DEFERRED)**

Scope clarification (ADR-MESH-0012): point-location states, walk/fallback policy, cavity records and insertion-order items in this research package are M2 design inputs. M1 executable qualification is limited to the tetra handle/opposite-face/canonical-face/topology-validation primitives; production point location and cavity mutation are not M1 deliverables.

Research package:
- `docs/research/meshing/m1-robust-geometry/m1.4-spatial-topology/`

Deliver:
- tetra topology storage contract,
- generation-checked handle policy,
- opposite-face neighbor convention,
- typed point-location states,
- walk-first locator with correctness fallback,
- cavity boundary/replacement records,
- spatial insertion-order benchmark,
- ANSYS / COMSOL / Marc scalability cross-check.

Exit:
- point-location and cavity data contracts frozen,
- walk/fallback agreement test designed,
- insertion-order performance experiment designed,
- memory telemetry defined,
- serial M2 implementation can start without external mesher data structures.

#### M1.5 — Executable verification harness

Status: **QUALIFIED**

Research package:
- `docs/research/meshing/m1-robust-geometry/m1.5-verification-harness/`

Deliver:
- bit-exact predicate fixture schema,
- dual exact-oracle generation pipeline,
- committed + generated corpus policy,
- test tier / CTest integration plan,
- failure replay contract,
- topology/point-location reference verification,
- ANSYS / COMSOL / Marc mesh-verification benchmark.

Exit:
- fixture/oracle contracts frozen,
- every generated failure replayable,
- CI tiers and labels frozen,
- first executable M1 oracle/predicate harness can be implemented without architecture ambiguity.

#### M1.6 — Executable robust-geometry verification prototype

Status: **QUALIFIED**

Implementation:
- `tools/meshing_oracle/`
- `tests/meshing/robust_geometry/`

Evidence now implemented:
- dual exact oracle for orient2d/orient3d/incircle/insphere,
- D26PRED raw-bit fixture schema,
- 25-case committed golden corpus,
- 1024-case deterministic dual-oracle cross-check in the test script,
- strict C++ fixture reader,
- signed-zero normalization policy test,
- CTest integration.

Exit:
- exact-head macOS arm64 Debug and Release pass the new robust-geometry tests,
- full existing CTest suite remains green,
- then executable robust-predicate implementation can begin.

#### M1.7 — Exact robust predicate kernel

Status: **QUALIFIED**

Evidence:
- exact C++ dyadic predicate kernel,
- committed + generated exact-oracle corpus,
- macOS arm64 workflow #233 SUCCESS.

#### M1.8 — Certified fast predicate path

Status: **QUALIFIED**

Evidence:
- conservative certified fast path,
- exact M1.7 fallback,
- committed + generated exact-oracle corpus,
- Debug/Release,
- macOS arm64 workflow #234 SUCCESS.

#### M1.9 — Closeout hardening

Status: **QUALIFIED**

Mandatory before M2:
- replay/adversarial/metamorphic verification,
- executable degeneracy foundation,
- tetra primitive/validator foundation,
- predicate telemetry baseline,
- documentation/ADR synchronization,
- final second M1 closeout audit.

### M2 — Delaunay point-cloud tetrahedralization

Status: **RESEARCHING — M2.0 REFERENCE ARCHITECTURE & EXPERIMENT PLAN**

Deliver:

- incremental 3D Delaunay tetrahedralizer,
- point location,
- cavity extraction,
- adjacency,
- deterministic insertion,
- no CAD constraints yet.

Verification:

- empty-sphere property,
- positive orientation,
- Euler/topology invariants where applicable,
- cross-check small cases against independent generated references,
- fuzz tests.

### M3 — CAD curve and surface meshing

Deliver:

- canonical CAD-edge discretizer,
- first Face mesher,
- shared-edge conformity,
- Face provenance,
- chord/normal quality.

Candidate first method:
- constrained Delaunay in CAD parameter space.

Second independent method/prototype:
- advancing front.

### M4 — Boundary recovery + tetra volume meshing

Deliver:

- watertight surface input,
- unconstrained tetra initialization,
- required edge/facet recovery,
- volume-domain classification,
- GeometryEntityId facet provenance.

Exit benchmark:
- all required CAD Face boundaries recovered exactly in topology,
- no inverted tetra,
- explicit failure on unsupported geometry.

### M5 — Size fields

Deliver:

- global size,
- Named Selection / geometry-scoped local size,
- curvature size,
- proximity/thickness size,
- min/max,
- gradation limiter.

Verification:
- monotonic refinement response,
- size histogram,
- no uncontrolled point explosion.

### M6 — Quality optimization

Deliver:

- quality metric suite,
- smart/optimization smoothing,
- local connectivity changes,
- sliver treatment,
- quality regression gates.

### M7 — Product TET4 qualification

Deliver:

- TET4 SimulationMesh topology,
- solver C ABI support where needed,
- patch tests,
- convergence tests,
- nonlinear distortion tests,
- GUI mesh inspector integration,
- full CAD → mesh → Named Selection → BC/load workflow.

Only here can arbitrary-CAD TET4 become product capability.

### M8 — TET10 / curved boundaries

Deliver after M7:

- midside node geometry projection,
- curved boundary mapping,
- high-order quality/untangling,
- solver qualification.

### M9 — Adaptation / remeshing research

Later:

- error/solution metric,
- isotropic/anisotropic adaptation,
- split/collapse/swap/move,
- field/state transfer,
- nonlinear remesh lifecycle.

### M10 — Sweep / hex / hybrid research

Later:

- sweepability detection,
- mapped surface mesh,
- structured volume sweep,
- pyramid/prism transitions,
- hex-dominant research.

## 3. Benchmark geometry corpus

### Tier A — exact/simple

- tetrahedron,
- cube,
- rectangular block,
- wedge,
- sphere,
- cylinder,
- hollow cylinder.

### Tier B — topology/features

- block with through-hole,
- intersecting holes,
- filleted block,
- chamfered block,
- thin plate solid,
- narrow gap,
- small edge/sliver face,
- close-but-disconnected bodies.

### Tier C — meshing stress

- high aspect-ratio channel,
- thin annulus,
- sharp re-entrant corner,
- multiple scale ranges,
- nearly tangent cylindrical features,
- small-radius fillet adjacent to large face.

### Tier D — automotive/rubber-oriented

- simplified crank pulley hub/rim geometry,
- rubber annulus volume,
- bonded rubber-metal interface geometry,
- simplified engine-mount rubber volume,
- viscous damper ring-like cavity.

Proprietary customer geometry must not be committed to the public corpus without explicit permission.

## 4. Metrics

Every experiment records at minimum:

### Correctness
- generated / failed,
- topology validity,
- missing boundary entities,
- inverted elements,
- non-manifold entities.

### Geometry fidelity
- maximum surface deviation,
- normal deviation,
- feature preservation.

### Mesh quality
- minimum/percentile dihedral angle,
- radius-edge ratio,
- condition/mean-ratio metric,
- aspect ratio,
- invalid count.

### Sizing
- requested vs actual edge scale,
- local sizing compliance,
- gradation.

### Performance
- wall time,
- peak memory where measurable,
- point count,
- triangle count,
- tetra count.

### Reproducibility
- algorithm version,
- input geometry hash,
- settings hash,
- deterministic mesh fingerprint.

## 5. Competitive benchmark protocol

ANSYS/COMSOL/Marc comparison is qualitative/engineering-oriented:

- does a similar geometry mesh successfully?
- how does user scope local sizing?
- what diagnostics are exposed?
- what quality controls exist?
- how are thin/small features handled?
- how are persistent selections retained?

Do not make numerical parity with a commercial proprietary mesh a release requirement because their exact algorithms/defaults are not public.

## 6. Open-source cross-check protocol

Open-source tools may be executed as **external test oracles** during research, but their output is not imported as production implementation.

Use cases:

- compare element count/quality distributions,
- compare failure cases,
- discover missing benchmark classes,
- validate that Dynamics26 results are plausible.

Cross-code agreement is supporting evidence, not proof.

## 7. R&D status board

| ID | Work package | State |
|---|---|---|
| M0 | Knowledge library | IN PROGRESS |
| M1 | Robust predicates | QUALIFIED |
| M1.3 | Degeneracy / symbolic perturbation | QUALIFIED (M1) |
| M1.4 | Spatial search / tetra topology | QUALIFIED (M1 primitives) |
| M1.5 | Verification harness | QUALIFIED |
| M1.6 | Executable verification prototype | QUALIFIED |
| M1.7 | Exact robust predicate kernel | QUALIFIED |
| M1.8 | Certified fast predicate path | QUALIFIED |
| M1.9 | Closeout hardening | QUALIFIED |
| M2 | Point-cloud Delaunay tetra | RESEARCHING / M2.0 |
| M3 | CAD surface meshing | NOT STARTED |
| M4 | Boundary recovery / volume mesh | NOT STARTED |
| M5 | Size fields | NOT STARTED |
| M6 | Quality optimization | NOT STARTED |
| M7 | TET4 product qualification | NOT STARTED |
| M8 | TET10 | NOT STARTED |
| M9 | Adaptation/remeshing | RESEARCH LATER |
| M10 | Sweep/hex/hybrid | RESEARCH LATER |

States:

\`\`\`text
NOT STARTED
RESEARCHING
PROTOTYPE
VERIFYING
QUALIFIED
PRODUCT
BLOCKED
\`\`\`

## 8. Immediate next research tasks

1. M1 is QUALIFIED; M2.0 research is active.
2. Maintain the committed m2-delaunay mathematics/architecture package as implementation authority.
3. Freeze ghost-hull, point-location, symbolic-tie and transactional-cavity contracts.
4. Freeze M2-G01..G20 acceptance tests before production insertion.
5. Then implement the serial correctness-first Bowyer-Watson reference path in auditable steps.
6. Keep spatial ordering, hierarchy, packing and parallelism behind reference-oracle agreement.
7. Continue commercial workflow/quality benchmarking in parallel; proprietary behavior is never predicate truth.

### M1.2 — Certified floating-point filters
Status: **QUALIFIED**

Research package:
- `docs/research/meshing/m1-robust-geometry/m1.2-certified-filters/`

Exit:
- zero false sign certifications on exact-oracle corpus,
- unsafe FP ranges always fallback,
- Apple Silicon Debug/Release agreement,
- fallback profile measured.
