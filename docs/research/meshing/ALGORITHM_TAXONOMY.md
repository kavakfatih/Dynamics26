# Meshing Algorithm Taxonomy

This document decomposes a production meshing system into researchable algorithmic layers. It is not a commitment to one algorithm.

## 1. Geometry intake and conditioning

Responsibilities:

- read authoritative B-Rep topology from Dynamics26 geometry services,
- validate manifold/closed-solid requirements,
- classify Body / Face / Edge / Vertex,
- detect seams, degenerate edges, tiny features and tolerance conflicts,
- compute adjacency,
- preserve persistent engineering identity.

Research topics:

- B-Rep tolerance policy,
- topology healing vs explicit failure,
- feature angle / curvature classification,
- minimum feature-size estimates,
- local feature size.

**Rule:** CAD repair must never be silently performed by display tessellation.

## 2. Robust geometric predicates

A reliable unstructured mesher requires decisions whose sign must remain correct near degeneracy:

- orient2d,
- orient3d,
- incircle,
- insphere,
- segment/triangle-side classification,
- coplanarity and near-coplanarity.

Naive double-precision determinant evaluation can change topology when roundoff changes a sign. Shewchuk's adaptive-precision work is therefore a primary theoretical reference.

Dynamics26 research direction:

\`\`\`text
fast filtered predicate
→ certified error bound
→ adaptive/exact fallback
\`\`\`

The final implementation must be original and separately tested with adversarial nearly-degenerate fixtures.

## 3. Spatial search and point location

Needed by almost every 3D algorithm:

- point location in tetrahedralization,
- nearest geometry feature,
- collision/intersection candidate search,
- advancing-front neighborhood query,
- cavity discovery,
- sizing-field evaluation.

Candidate structures:

- AABB tree / BVH,
- R-tree,
- kd-tree,
- uniform/hash grid,
- walking point-location plus fallback search,
- Hilbert/Morton ordering for locality.

Data structure choice is a separate performance decision from the mathematical meshing algorithm.

## 4. Mesh-size field h(x)

The size field is a first-class engineering object, not an incidental scalar.

Initial components:

1. global target size,
2. Body-scoped local size,
3. Face-scoped local size,
4. Edge-scoped local size,
5. curvature-derived upper bound,
6. proximity/thickness-derived upper bound,
7. minimum / maximum size,
8. gradation constraint.

Conceptually:

\`\`\`text
h_target(x) = min(
    h_global,
    h_local_scopes(x),
    h_curvature(x),
    h_proximity(x)
)
\`\`\`

followed by a gradation/regularization operation.

Named Selections should be reusable mesh-control scopes.

## 5. Curve / CAD-edge discretization

Before surface meshing, model edges need consistent sampling.

Requirements:

- shared edge has one canonical node chain,
- vertices are protected,
- edge orientation is explicit,
- chordal deviation and tangent/curvature error are controlled,
- neighboring faces consume the same edge discretization,
- local sizing controls are respected.

This stage is essential to conformal multi-face surface meshing.

## 6. Surface triangulation families

### 6.1 Parametric-face triangulation

Map a CAD Face into a 2D parameter domain, mesh there, then evaluate/project points back to the surface.

Strengths:
- direct access to CAD parameterization,
- clean boundary control.

Risks:
- distorted parameter spaces,
- singularities and seams,
- trimmed-surface complexity.

### 6.2 Advancing-front surface meshing

Start from discretized boundary curves and progressively fill the face.

Strengths:
- intuitive local size control,
- often good element orientation and boundary conformity.

Risks:
- collision/front closure complexity,
- robustness near narrow regions,
- sophisticated search and local templates required.

### 6.3 Delaunay / constrained Delaunay in parameter space

Maintain a 2D Delaunay triangulation and enforce trimmed boundaries.

Strengths:
- strong theoretical base,
- natural refinement and quality control.

Risks:
- robust boundary recovery,
- CAD parameter-space distortion.

### 6.4 Restricted-Delaunay surface meshing

Use a 3D Delaunay/Voronoi structure and select the triangulation restricted to the target surface.

Strengths:
- strong topology/approximation literature,
- avoids relying purely on a distorted 2D parameterization.

Risks:
- significantly greater implementation complexity,
- feature protection and surface-oracle robustness.

**Initial Dynamics26 recommendation:** research parametric-face Delaunay and advancing-front approaches first; keep restricted Delaunay as a high-value advanced track.

## 7. Volume tetrahedralization families

### 7.1 Incremental Delaunay / Bowyer-Watson

Maintain a Delaunay tetrahedralization while inserting points by deleting the cavity whose circumspheres contain the new point.

Core research problems:

- robust orient3d/insphere,
- point location,
- cavity extraction,
- adjacency maintenance,
- degenerate/cospherical cases,
- deterministic insertion ordering.

### 7.2 Constrained Delaunay / boundary recovery

An unconstrained Delaunay tetrahedralization does not automatically contain every required CAD boundary edge/triangle.

Boundary recovery therefore needs explicit algorithms:

- edge recovery,
- face recovery,
- local flips,
- Steiner insertion,
- cavity retriangulation.

George/Hecht/Saltel and TetGen literature are primary references for the importance of this stage.

### 7.3 Delaunay refinement

Detect bad tetrahedra or missing boundary-quality conditions and insert Steiner points under a termination/quality strategy.

Research metrics:

- radius-edge ratio,
- size error,
- boundary approximation,
- local feature size,
- sliver behavior.

### 7.4 Advancing-front tetrahedral meshing

Start from a triangulated surface front and create tetrahedra while consuming front faces.

Strengths:
- local control,
- natural boundary-first construction.

Risks:
- closure/failure modes,
- geometric search,
- template/rule complexity,
- robustness in narrow cavities.

Netgen and Löhner/Parikh are useful study references.

### 7.5 Octree / spatial-subdivision meshing

Subdivide space to satisfy size/geometry conditions, then conform to the boundary.

Strengths:
- robust size hierarchy,
- useful for dirty/complex geometry,
- natural parallelism.

Risks:
- boundary conformity and transition quality,
- potentially less direct CAD fidelity.

ANSYS Patch Independent is a commercial benchmark for the capability class, not an implementation specification.

## 8. Quality improvement

A production tetra mesher should separate **mesh creation** from **mesh optimization**.

Candidate operations:

### Vertex movement
- Laplacian smoothing,
- smart Laplacian,
- optimization-based smoothing,
- ODT-like smoothing,
- boundary-tangent constrained smoothing.

### Connectivity changes
- 2↔3 flips,
- 3↔2 flips,
- 4↔4 and generalized cavity reconnection,
- edge/face swapping.

### Topology/point changes
- point insertion,
- point removal,
- edge split/collapse for adaptation,
- cavity remeshing.

### Sliver treatment
- perturbation,
- weighted-Delaunay/sliver-exudation concepts,
- optimization,
- targeted cavity operations.

A quality optimizer must never invert elements silently.

## 9. Quality metrics

No single metric is sufficient.

Research set:

- signed volume / Jacobian,
- radius-edge ratio,
- radius ratio,
- minimum/maximum dihedral angle,
- mean ratio,
- condition-number-based shape metric,
- aspect ratio,
- size-field conformity,
- surface chordal deviation,
- normal deviation.

Dynamics26 should record distributions, worst values and invalid counts rather than only one average number.

## 10. Geometry-to-mesh provenance

Every generated boundary facet must retain authoritative origin:

\`\`\`text
CAD Face GeometryEntityId
→ generated surface triangles
→ volume-mesh boundary facets
→ Named Selection / BC / Load resolution
\`\`\`

Provider-local or algorithm-local IDs are diagnostic only.

For shared/intersection topology, ownership must be explicit rather than inferred from coordinates after meshing.

## 11. Higher order

TET10 is not simply "add midpoint nodes."

Required research:

- edge-node ordering contract,
- projection of midside nodes to CAD edges/faces,
- curved-boundary Jacobian quality,
- high-order untangling,
- solver integration convention.

TET10 begins only after TET4 topology/provenance is stable.

## 12. Adaptation / remeshing

Later track:

\`\`\`text
solution/error metric
→ isotropic/anisotropic metric field
→ split/collapse/swap/move
→ field transfer
→ history/state transfer
→ quality validation
\`\`\`

MMG and Marc are useful architecture/product references. This is not an initial V1.2 deliverable.

## 13. Hybrid/hex direction

ANSYS MultiZone, COMSOL Swept and Marc Hexmesh show that mature CAE systems use multiple meshing methods.

Dynamics26 should therefore avoid designing its data model as "tetra forever." However, original sweep/hex-dominant algorithms are a later program after a reliable tetra baseline.
