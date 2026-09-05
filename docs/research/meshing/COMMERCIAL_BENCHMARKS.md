# Commercial Meshing Benchmarks

The purpose of this document is to record **observable product semantics**, not infer proprietary source code.

## 1. ANSYS Mechanical / Meshing

### Observed capability pattern

ANSYS exposes meshing as a set of methods and controls rather than one global algorithm.

Important public behaviors:

- Patch Conforming Tetra is documented as a Delaunay tetra mesher using advancing-front point insertion for refinement.
- Patch Independent Tetra is documented as a top-down spatial-subdivision approach and is useful for dirty/poorly formed geometry.
- MultiZone combines decomposition/sweep-like strategies and can preserve selected surface patches.
- Local Sizing can be scoped to Body, Face, Edge or Vertex.
- Local Sizing can also be scoped through Named Selection.
- Named Selections can have geometry- and mesh-related semantics and may be protected when important to BC/mesh controls.

### Dynamics26 lesson

Do not make "meshing method" and "mesh control" the same concept.

Target conceptual tree:

\`\`\`text
Mesh
├─ Method
├─ Global Size
├─ Local Size — Named Selection A
├─ Local Size — Edge Group B
└─ Quality
\`\`\`

Named Selection should be a common scope mechanism for both analysis and meshing.

### Do not copy

- command names/layout,
- object hierarchy,
- undisclosed algorithms,
- parameter defaults.

The benchmark is the capability separation, not the UI identity.

## 2. COMSOL

### Observed capability pattern

COMSOL presents a mesh sequence made of operations.

Public documentation shows:

- Free Tetrahedral is a domain-scoped operation,
- scope may be manual or a Named Selection,
- mesh sequences can create their own named selections,
- selections produced by mesh operations can be reused/tracked,
- swept/mapped/free operations coexist in one mesh sequence.

### Dynamics26 lesson

A future mesh definition should be able to become an ordered operation graph/sequence without changing the fundamental GeometryEntityId scope model.

Early V1.2 does not need to reproduce COMSOL's full sequence system, but the model should avoid a dead-end singleton "Generate Mesh" configuration.

## 3. Marc / Mentat

### Observed capability pattern

Marc/Mentat documentation and verified support discussions expose:

- Global Remeshing,
- Local Adaptivity,
- automatic 2D/3D remeshing capabilities,
- hexahedral mesher capability,
- mesh-on-mesh and voxel-mesher features in release history,
- remeshing as a nonlinear-analysis capability rather than only preprocessing.

### Dynamics26 lesson

Separate:

\`\`\`text
Initial Mesh Generation
!=
Mesh Adaptation / Remeshing
\`\`\`

This matters for future rubber/contact/large-deformation work where element distortion can become a solver-level problem.

## 4. Cross-product capability matrix

| Capability | ANSYS | COMSOL | Marc/Mentat | Dynamics26 research target |
|---|---|---|---|---|
| Geometry-scoped sizing | Yes | Yes | Yes-class capability | V1.2 |
| Named reusable scope | Strong | Strong | Sets/groups | V1.2 via existing Named Selection |
| Free tetra | Yes | Yes | Yes | Original implementation |
| Multiple mesh methods | Yes | Yes | Yes | Architecture-ready |
| Sweep/structured | Yes | Yes | Yes | Later |
| Hex automation | MultiZone/other | Swept/mapped | Hex mesher | Later |
| Dirty-geometry strategy | Patch Independent class | geometry/mesh controls | preprocessing/remesh | Research |
| Mesh quality controls | Strong | Strong | Strong | V1.2 |
| Adaptive/remeshing | Available classes | Available workflows | Strong | Later |
| CAD→mesh scope continuity | Strong product behavior | Strong selection behavior | Product sets/groups | Mandatory invariant |

## 5. Benchmark questions for every Dynamics26 milestone

When a feature is designed, compare it against commercial behavior with questions such as:

1. Can the user scope the control by persistent geometry identity?
2. Does remeshing silently invalidate BC/load scope?
3. Is the size control global, local, curvature or proximity based?
4. Are invalid/unsupported combinations disabled or silently approximated?
5. Can quality and failure reasons be inspected?
6. Does the program distinguish preprocessing mesh generation from nonlinear remeshing?
7. Does visualization remain separate from engineering topology?

Commercial-product parity is not a release criterion; these questions are used to expose missing engineering concepts.
