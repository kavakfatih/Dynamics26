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

Status: **RESEARCHING**

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

### M2 — Delaunay point-cloud tetrahedralization

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
| M1 | Robust predicates | RESEARCHING |
| M2 | Point-cloud Delaunay tetra | NOT STARTED |
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

1. independent exact-rational predicate oracle prototype/specification,
2. derive and benchmark candidate fast-filter/error-bound strategies,
3. compare adaptive expansion arithmetic against a deliberately slow exact-reference path,
4. freeze exact-zero and symbolic tie-break boundary between M1 and M2,
5. run first Apple Silicon compiler/optimization sensitivity experiment,
6. after M1 evidence, begin M2 point-location and Bowyer-Watson correctness research.
