# Dynamics26 Meshing Knowledge Library

**Program:** Dynamics26 Original Meshing System R&D  
**Owner repository:** \`kavakfatih/Dynamics26\`  
**Platform:** macOS / Apple Silicon  
**Research baseline:** 2026-09-05  
**Status:** ACTIVE RESEARCH  
**Implementation policy:** original Dynamics26 implementation; no external meshing-engine product dependency is assumed.

## 1. Purpose

This directory is the permanent research and engineering knowledge base for the Dynamics26 meshing system.

The objective is not to wrap or rebrand Netgen, Gmsh, MMG, TetGen, CGAL or another meshing engine. Their public documentation, papers and source trees are studied to understand algorithm families, architecture patterns, failure modes, quality controls and verification practices. Dynamics26 implementations are designed from first principles and written in the Dynamics26 codebase under its own architecture and test contracts.

Commercial programs such as ANSYS Mechanical/Meshing, Marc/Mentat and COMSOL are used only as product-behavior benchmarks. Their implementation source is not available and no claim is made about undocumented internal algorithms.

## 2. Core invariant

\`\`\`text
CAD Geometry
!= Display Tessellation
!= FEM Mesh
\`\`\`

The authoritative chain for V1.2+ research is:

\`\`\`text
CAD / B-Rep authority
→ persistent Body / Face / Edge / Vertex identity
→ mesh controls / Named Selections
→ curve and surface discretization
→ volume meshing
→ quality optimization
→ GeometryEntityId → FEM entity provenance
→ validation / diagnostics
→ solver-ready SimulationMesh
\`\`\`

Display triangles are never treated as CAD authority and are never silently promoted into the solver mesh.

## 3. Strategic decision

The V1.2 meshing program now follows an **original-engine strategy**:

- build an in-house Dynamics26 meshing kernel,
- use academic literature as the primary algorithmic authority,
- use open-source repositories for architecture study and independent verification ideas,
- use commercial CAE products for capability/UX benchmarking,
- do not copy, translate, port or cosmetically rewrite external source code,
- keep third-party code out of the production meshing core unless a future explicit ADR approves a narrowly scoped dependency.

See \`CLEAN_ROOM_POLICY.md\` and \`DECISION_LOG.md\`.

## 4. Library map

| Document | Purpose |
|---|---|
| \`SOURCE_REGISTRY.md\` | Papers, vendor docs, source repositories and license notes |
| \`ALGORITHM_TAXONOMY.md\` | Meshing problem decomposition and algorithm families |
| \`COMMERCIAL_BENCHMARKS.md\` | ANSYS / COMSOL / Marc observable behavior |
| \`OPEN_SOURCE_ARCHITECTURE_STUDIES.md\` | Netgen / Gmsh / MMG / TetGen / CGAL source-level lessons |
| \`CLEAN_ROOM_POLICY.md\` | Source-use and implementation-boundary rules |
| \`DYNAMICS26_ORIGINAL_MESHER_ARCHITECTURE.md\` | Proposed original Dynamics26 architecture |
| \`ROADMAP_AND_EXPERIMENTS.md\` | Work packages, gates and benchmark corpus |
| \`EXPERIMENT_TEMPLATE.md\` | Repeatable R&D experiment record |
| \`DECISION_LOG.md\` | Meshing ADR-style decisions and open decisions |

## 5. Evidence hierarchy

1. peer-reviewed theory / geometry / mesh-generation literature,
2. original author technical reports and academic preprints,
3. official commercial-vendor documentation for observable semantics,
4. official open-source documentation,
5. open-source source trees for architecture and test-study only,
6. forums only for troubleshooting clues or explicitly labeled product observations.

A source-level observation never becomes a Dynamics26 implementation specification by itself.

## 6. Research lifecycle

Each substantial meshing capability follows:

\`\`\`text
PROBLEM
→ THEORY SOURCES
→ COMMERCIAL BENCHMARK
→ OPEN-SOURCE ARCHITECTURE STUDY
→ DYNAMICS26 CONTRACT
→ ORIGINAL PROTOTYPE
→ UNIT / PROPERTY TESTS
→ GEOMETRY CORPUS
→ QUALITY / PERFORMANCE BENCHMARK
→ ADR
→ PRODUCT IMPLEMENTATION
\`\`\`

## 7. Initial target capability

The first original unstructured-meshing target is intentionally narrower than a mature commercial mesher:

- watertight single-solid B-Rep,
- robust curve/edge sampling,
- CAD-conforming triangular surface mesh,
- tetrahedral volume mesh,
- global and geometry-scoped local size controls,
- curvature-aware size control,
- explicit geometry-to-boundary-facet provenance,
- deterministic diagnostics,
- TET4 first,
- TET10 only after topology and geometry-ordering contracts are stable,
- no hidden repair or bounding-box approximation.

Sweep/hex-dominant, boundary layers and adaptive nonlinear remeshing are later tracks.

## 8. Current repository starting point

Current Dynamics26 already provides useful foundations:

- \`GeometryDocument\` persistent identities,
- OCCT STEP/B-Rep import path,
- \`NamedSelectionService\`,
- \`MeshingPlan\`,
- \`SimulationMesh\`,
- \`MeshFacet::sourceGeometryId\`,
- \`GeometryAssociationMap\`,
- \`StructuredHexMesher\` verification/product baseline,
- mesh lifecycle generation and stale-state handling.

The original unstructured mesher should extend these contracts rather than replace them with mesher-owned IDs.

## 9. Update discipline

Every new study should:

1. add or update a source in \`SOURCE_REGISTRY.md\`,
2. record the engineering question,
3. separate observed fact from proposed Dynamics26 decision,
4. create an experiment record when code or numerical testing begins,
5. update \`DECISION_LOG.md\` only after evidence is sufficient.

This directory is intended to remain useful after individual chat or coding sessions end.


## Active robust-geometry subprograms

- M1.1 exact predicate oracle
- M1.2 certified floating-point filters
- M1.3 degeneracy / deterministic symbolic perturbation
- M1.4 spatial search / point location / tetra topology

These research packages are prerequisites for the first original serial 3D Delaunay prototype.


- M1.5 verification harness / exact corpus / CI architecture

M1.5 is the bridge from research documents to executable evidence. The next coding step is the test-only exact oracle and fixture reader, not yet the production Delaunay mesher.


- M1.6 executable robust-geometry verification prototype

The first executable meshing R&D evidence now lives under `tools/meshing_oracle/` and `tests/meshing/robust_geometry/`. Production Delaunay implementation is intentionally still deferred.
