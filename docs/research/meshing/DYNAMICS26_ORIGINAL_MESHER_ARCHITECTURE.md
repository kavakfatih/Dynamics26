# Dynamics26 Original Mesher — Proposed Architecture

**Status:** research architecture, not yet production API.

## 1. Design objective

Build an original meshing subsystem that can eventually support arbitrary B-Rep automotive geometry while preserving Dynamics26 engineering identities.

The architecture must allow algorithms to evolve without changing solver/document semantics.

## 2. Existing contracts to preserve

Current source already establishes:

\`\`\`text
GeometryDocument
NamedSelection / ScopeReference
MeshingPlan
SimulationMesh
MeshFacet.sourceGeometryId
GeometryAssociationMap
Mesh generation lifecycle
\`\`\`

These should remain the product-facing foundation.

## 3. Proposed subsystem decomposition

\`\`\`text
GeometryService / OCCT B-Rep
        │
        ▼
GeometryMeshingView
        │
        ├── TopologyValidator
        ├── GeometryFeatureAnalyzer
        └── GeometryQuery
        │
        ▼
MeshCriteria
        ├── MeshSizeField
        ├── CurvatureCriterion
        ├── ProximityCriterion
        └── ScopedSizing
        │
        ▼
CurveDiscretizer
        │
        ▼
SurfaceMesher
        │
        ▼
VolumeMesher
        │
        ├── DelaunayKernel
        ├── BoundaryRecovery
        └── RefinementController
        │
        ▼
QualityOptimizer
        │
        ▼
MeshValidator
        │
        ▼
ProvenanceBuilder
        │
        ▼
SimulationMesh + MeshGenerationReport
\`\`\`

Names are provisional. Boundaries are the important part.

## 4. GeometryMeshingView

Read-only meshing view over authoritative CAD.

Responsibilities:

- enumerate solids/faces/edges/vertices,
- topology adjacency,
- evaluate curve/surface points and derivatives,
- project candidate points,
- query bounds and tolerances,
- provide persistent \`GeometryEntityId\`.

It must not expose UI widgets or display tessellation.

## 5. Geometry predicates subsystem

Suggested contract class:

\`\`\`text
GeometryPredicates
- orient2d
- orient3d
- incircle
- insphere
\`\`\`

Implementation plan:

1. naive/reference implementation for test comparison only,
2. filtered double-precision path,
3. adaptive/exact fallback,
4. adversarial randomized/property tests,
5. deterministic cross-platform behavior on arm64.

This subsystem should be small and heavily verified before the tetrahedralizer depends on it.

## 6. MeshCriteria / MeshSizeField

Proposed value semantics:

\`\`\`text
MeshCriteria
- globalTargetSize
- minSize
- maxSize
- maxGrowthRate
- curvatureControl
- proximityControl
- scopedSizeRules[]
\`\`\`

A scoped rule references \`GeometryEntityId\` or a persistent Named Selection ObjectId at the application boundary, then resolves into geometry identities before the algorithm begins.

The mathematical kernel should receive an immutable resolved size-field snapshot, not live Qt service objects.

## 7. CurveDiscretizer

Output concept:

\`\`\`text
CurveMesh
- vertex samples
- ordered edge node chains
- GeometryEntityId per CAD Edge
- parameter coordinate per node
\`\`\`

Shared CAD Edges are discretized once. Adjacent faces consume the same chain.

Acceptance:

- endpoints exact within CAD tolerance,
- monotonic parameter order,
- no duplicate near-coincident nodes,
- sizing and chord-error limits respected.

## 8. SurfaceMesher

First research candidates:

### Candidate S1 — parameter-space constrained Delaunay
Good first implementation target for trimmed analytic/NURBS faces.

### Candidate S2 — advancing-front on parametric surface
Potentially better local control and useful independent cross-check.

### Candidate S3 — restricted Delaunay
Advanced research track for difficult parameterization/feature cases.

The production interface must allow algorithm substitution.

Output:

\`\`\`text
SurfaceMesh
- nodes
- triangles
- triangle → CAD Face GeometryEntityId
- edge-chain ownership
- quality metrics
\`\`\`

## 9. DelaunayKernel

Research baseline:

- incremental point insertion,
- tetra adjacency,
- Bowyer-Watson cavity construction,
- robust in-sphere decisions,
- deterministic tie/degeneracy policy,
- spatial point-location acceleration.

It should initially solve **point-cloud tetrahedralization** before CAD boundary recovery is added.

This creates a clean verification ladder.

## 10. BoundaryRecovery

Separate subsystem because constrained CAD boundaries are not guaranteed by unconstrained Delaunay.

Inputs:

- tetrahedralization,
- required surface triangles,
- required boundary edges.

Possible operations to research:

- local flips,
- Steiner insertion,
- cavity removal/retriangulation.

Acceptance is binary:

> Every required valid boundary entity is recovered or meshing fails explicitly.

No "almost conformal" silent success.

## 11. RefinementController

Iterative loop concept:

\`\`\`text
evaluate bad cells / size error / boundary error
→ choose insertion candidate
→ validate candidate
→ insert
→ update topology
→ repeat
\`\`\`

Early quality driver candidates:

- maximum size,
- radius-edge quality,
- boundary fidelity.

Termination and runaway-size guards are mandatory.

## 12. QualityOptimizer

Independent pipeline after valid topology exists.

Initial operations:

1. smart smoothing,
2. optimization-based smoothing,
3. local flips/swaps,
4. targeted sliver treatment.

Every operation must be transactional at the local cavity level: if quality or validity worsens outside accepted bounds, revert.

## 13. MeshValidator

Hard failures:

- inverted/zero-volume tetra,
- missing required boundary facet,
- non-manifold boundary,
- duplicate invalid connectivity,
- orphan node,
- inconsistent owner relation,
- provenance loss.

Warnings:

- poor dihedral angle,
- large radius-edge ratio,
- size-field deviation,
- strong gradation,
- high curvature approximation error.

## 14. ProvenanceBuilder

Canonical mapping:

\`\`\`text
CAD Face GeometryEntityId
↔ surface triangles
↔ SimulationMesh boundary facets
\`\`\`

Interior elements may carry Body/Region identity.

The provenance builder should generate/validate \`GeometryAssociationMap\`.

Named Selection consumers resolve through this map, not coordinate searches.

## 15. MeshGenerationReport

Suggested immutable result:

\`\`\`text
status
terminationReason
algorithmVersion
geometryRevision
requestedCriteria
nodeCount
surfaceTriangleCount
tetCount
qualitySummary
timings
warnings[]
errors[]
provenanceSummary
determinismFingerprint
\`\`\`

This becomes the basis for GUI diagnostics and R&D benchmark logging.

## 16. Determinism

Raw unstructured mesh IDs need not remain stable across different algorithm versions.

However, for a fixed version/configuration/input:

- deterministic ordering is strongly preferred,
- tests should compute a canonical mesh fingerprint,
- provenance must remain stable,
- randomization must use explicit recorded seeds.

## 17. Threading

Do not parallelize the first algorithm before correctness.

Research order:

1. robust serial reference,
2. profile,
3. isolate independent stages,
4. parallel surface faces where safe,
5. investigate parallel insertion/optimization later.

HXT demonstrates the value of parallelism, but its implementation is not a starting specification.

## 18. Application boundary

Proposed future flow:

\`\`\`text
Project/Geometry/NamedSelection state
→ MeshRequest snapshot
→ femcae meshing core
→ MeshGenerationResult
→ MeshService publishes SimulationMesh
→ DependencyEngine updates stale/readiness
\`\`\`

The meshing core should remain Qt-independent.
