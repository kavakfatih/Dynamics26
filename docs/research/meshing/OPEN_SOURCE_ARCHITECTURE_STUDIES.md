# Open-Source Meshing Architecture Studies

**Purpose:** learn architectural patterns, algorithm decomposition, tests and failure modes without copying implementation code.

## 1. Netgen

### Public architecture observations

Repository structure separates major concerns including:

- \`libsrc/meshing\`,
- \`libsrc/occ\`,
- geometry primitives/CSG,
- interfaces,
- tests.

The classic Schöberl paper decomposes 3D generation into:

1. special-point calculation,
2. edge following,
3. surface meshing,
4. volume meshing,

with surface/volume advancing-front methods plus optimization.

Current source still exposes an advancing-front-oriented volume meshing implementation and local-size queries.

### Useful ideas

- explicit geometry adapter separate from meshing kernel,
- boundary-first staging,
- local mesh-size evaluation,
- generation followed by optimization,
- strong geometry test corpus.

### Do not reproduce

- rule tables,
- class/function layout,
- template logic,
- data structures,
- heuristics/default constants.

Dynamics26 will derive its own algorithms from theory sources and its own B-Rep contracts.

## 2. Gmsh

### Public architecture observations

The source tree has a large \`src/mesh\` subsystem and explicit strategy dispatch for multiple 3D algorithms. The project also contains separate or contributed implementations for HXT, Netgen, MMG and optimization tools.

The public manual describes several 3D algorithms, with Delaunay and HXT having broad feature/size-field support.

A key architectural strength is geometric classification: mesh entities remain associated with geometric entities.

### Useful ideas

- algorithm strategy dispatch,
- common mesh model consumed by multiple algorithms,
- explicit geometry-entity classification,
- large benchmark corpus organized by geometry/problem class,
- quality optimization as separate machinery.

### Dynamics26 adaptation

Use a common Dynamics26 mesh contract and algorithm interfaces, but keep all production implementations original.

Gmsh's rich \`benchmarks/step\`, \`benchmarks/brep\`, \`benchmarks/3d\`, \`benchmarks/hex\` organization is a useful **testing-organization** reference.

## 3. MMG

### Public architecture observations

MMG describes itself as surface and volume remeshing software. Source is separated into:

- common,
- 2D,
- 3D,
- surface modules.

Public examples show tetrahedra and boundary entities carrying reference values and metric/solution fields driving adaptation.

### Useful ideas

- adaptation as a separate engine from initial CAD meshing,
- metric-field abstraction,
- local topology operations,
- distinct low-failure/strong-failure style outcome semantics,
- reference preservation through adaptation.

### Dynamics26 adaptation

Future:

\`\`\`text
MeshAdaptationRequest
+ metric field
+ protected geometry/boundary identities
→ adapted SimulationMesh
+ transfer map
+ diagnostics
\`\`\`

Do not put this into the initial tetra generator.

## 4. TetGen

### Public architecture observations

TetGen explicitly focuses on:

- Delaunay tetrahedralization,
- constrained Delaunay tetrahedralization,
- boundary-conforming meshes,
- Delaunay refinement,
- quality/sliver improvement.

The current repository README also makes license constraints very visible; this is a useful reminder that "source available" does not mean "safe to copy."

### Useful ideas

- boundary recovery is a first-class algorithmic stage,
- robust predicates and degeneracy handling are core infrastructure,
- region/boundary attributes need explicit propagation,
- quality refinement and sliver handling need dedicated tests.

### Dynamics26 adaptation

Use the academic TetGen/Shewchuk literature as algorithmic authority. Do not port TetGen source or its source-specific implementation details.

## 5. CGAL Mesh_3

### Public architecture/documentation observations

Mesh_3 documents a Delaunay-refinement pipeline plus optimization. Its design-history documentation links the implementation to:

- Chew/Ruppert/Shewchuk refinement,
- restricted Delaunay surface approximation,
- feature protection,
- perturbation/sliver-exudation concepts.

### Useful ideas

- clean separation of domain description, mesh criteria and refinement,
- multiple criteria drive generation,
- feature protection is explicit,
- optimization follows refinement.

### Dynamics26 adaptation

A strong original contract would similarly separate:

\`\`\`text
GeometryDomain
MeshCriteria
MeshingAlgorithm
QualityOptimizer
\`\`\`

without copying CGAL APIs or source.

## 6. Cross-source lessons

The projects differ in algorithms, but common production patterns emerge:

1. geometry/domain adapter,
2. persistent feature classification,
3. size/metric field,
4. surface generation,
5. volume generation,
6. boundary recovery,
7. optimization,
8. quality/diagnostics,
9. regression geometry corpus.

These patterns are generic engineering architecture and are appropriate to re-express independently in Dynamics26.

## 7. Source-inspection rule

When a source file is inspected, research notes should record **what engineering problem it solves**, not reproduce its implementation.

Good note:

> "The project separates volume algorithm dispatch from geometry entity objects and runs quality optimization as another stage."

Bad note:

> "Recreate class X with methods A/B/C and copy its cavity logic."

The second pattern is prohibited by \`CLEAN_ROOM_POLICY.md\`.
