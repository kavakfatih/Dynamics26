# M7 Design Freeze — Product TET4 Integration Boundary

Status: **PRODUCT-INTEGRATION RESEARCH COMPLETE / DESIGN FROZEN / IMPLEMENTATION NOT STARTED**  
Freeze ID: **D26-M7-FREEZE-1**  
Research package: **M7 — Product TET4 Integration**  
Date: 2026-09-06

## 1. Purpose

M7 freezes the boundary from meshing output into the Dynamics26 product data model, engineering scopes and solver handoff. It does **not** select a nearly-incompressible TET4 mechanical formulation.

## 2. Current repository data-contract limitation

Audit at baseline HEAD `60275d9430b13a3cf3b3fdd26709dee11e3d7f8b` confirms the product path is still HEX8/QUAD4-oriented:
- `include/femcae/meshing/MeshTypes.h`: `MeshTopology` contains `Hex8` and `Quad4`,
- `src/application/AnalysisSnapshot.cpp`: current snapshot validation requires HEX8 elements,
- `src/application/SolverInputBuilder.cpp`: current flattening path is `buildHex8`,
- `src/model/fem_mesh.f90`: current topology registry usage includes BAR2/QUAD4/HEX8 and no TET4 product topology,
- the nonlinear product bridge remains HEX8-specific.

Therefore TET4 product support requires a **versioned data-contract migration**. It is not equivalent to adding one `MeshTopology` enum value.

Future migration domains may include:
- C++ `SimulationMesh` topology/connectivity,
- boundary-facet TRI3 representation,
- Fortran topology registry and reference-element metadata,
- `AnalysisSnapshot` schema,
- `SolverInputBuilder`,
- C ABI/schema versioning,
- result topology handling,
- TRI3 surface-load integration,
- persistence/project schema and migration,
- capability/preflight declarations.

No migration is implemented by this freeze.

## 3. Topology is not formulation

```text
TET4 topology != TET4 mechanical formulation
```

Possible future combinations include:
- TET4 + compressible displacement P1,
- TET4 + MINI mixed formulation,
- TET4 + stabilized P1/P1,
- TET4 + other future formulation.

Mesh topology metadata and solver formulation identity remain separate versioned contracts.

## 4. Provenance target

```text
volume TET4
-> Body / Region GeometryEntityId

boundary TRI3
-> Face GeometryEntityId

mesh node constrained to CAD Edge
-> Edge GeometryEntityId

CAD vertex mesh node
-> Vertex GeometryEntityId
```

`GeometryAssociationMap` remains the persistent mapping bridge. Coordinate matching is not persistent provenance.

## 5. Named Selection lifecycle

Geometry-domain Named Selection persists by CAD identity:

```text
persistent CAD identity
-> remesh
-> resolve new mesh entities through provenance
```

Mesh-domain Named Selection belongs to one specific mesh generation.

After regenerate, the old mesh scope is **STALE**, even if integer `MeshEntityId` values happen to repeat. Silent mesh-scope rebind is forbidden. Mesh generation/fingerprint identity participates in stale-scope validation.

## 6. Boundary TRI3 supports and loads

Product TET4 requires first-class TRI3 boundary-facet metadata and integration. Existing QUAD4 surface-integration behavior is not silently reused as though TRI3 were identical.

Support/load resolution consumes persistent Face provenance and the current mesh generation.

## 7. Solver research remains separate

M7 cross-references without duplicating:

`docs/research/fem/tet4-nearly-incompressible/`

Product-language invariant:

```text
TET4 mesh generated
!= compressible TET4 solver qualified
!= nearly-incompressible TET4 qualified
!= rubber-ready
```

Current solver-research candidates remain:
- pure P1 displacement baseline / negative control,
- MINI reference candidate,
- stabilized P1/P1 candidate,
- patch F-bar alternative,
- later higher-order hybrid/mixed TET10.

M7 selects no winner.

## 8. Persistence and ABI

Any TET4 addition that changes serialized topology, solver flattening or C ABI uses explicit versioning/migration. Save/reopen behavior is a qualification gate.

Old projects and old mesh-domain scopes shall not be reinterpreted under a new topology schema without declared migration semantics.

## 9. Qualification boundary

D26-M7-FREEZE-1 means the product-integration research boundary is sufficiently defined for future DEV-MESH-P6 planning after P1-P5 dependencies qualify.

It does not mean TET4 product support, TET4 solver qualification or rubber capability exists.

Qualification authority: `M7_QUALIFICATION_GATE_MATRIX.md`. Solver formulation gates remain in the dedicated FEM research package and are not renumbered as M7 gates.
