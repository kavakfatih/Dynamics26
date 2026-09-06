# M3 Design Freeze — CAD Curve and Surface Meshing

Status: **RESEARCH COMPLETE / DESIGN FROZEN / IMPLEMENTATION NOT STARTED**  
Freeze ID: **D26-M3-FREEZE-1**  
Research package: **M3 — CAD Curve + Surface Meshing**  
Date: 2026-09-06

## 1. Authority and product invariant

The authoritative meshing source is the OCCT B-Rep. Display tessellation is visualization data and is neither a meshing input nor persistent engineering identity.

```text
CAD Geometry != Display Tessellation != FEM Mesh

CAD / B-Rep
-> GeometryEntityId
-> mesh provenance
-> GeometryAssociationMap
-> Named Selection
-> BC / Load
-> Solver
-> Results
```

## 2. Shared physical CAD-edge discretization

Each physical CAD Edge is discretized once into one ordered **PhysicalEdgeChain**:

```text
CAD Edge
-> ordered physical MeshNode chain in 3D
-> CAD curve parameters
-> Edge GeometryEntityId provenance
```

Adjacent Faces consume the same physical MeshNode sequence. Separate face triangulators shall not independently create alternative physical nodes on the same shared CAD Edge. A FaceUse may consume the canonical physical chain forward or reversed.

## 3. FaceUse / coedge / pcurve ownership

A shared physical 3D edge chain is not a universal UV representation. Dynamics26 freezes:

```text
PhysicalEdgeChain
+
FaceUse / Coedge
+
Face-specific pcurve / UV chain
```

One CAD Edge may belong to several Faces, have orientation per use, be a seam, or have more than one pcurve use on the same Face. Therefore `CAD Edge -> one universal UV polyline` is rejected. FaceUse identity owns face-local orientation and pcurve choice.

## 4. Chart identity versus physical node identity

```text
ChartVertexId -> MeshNodeId
```

may be many-to-one. This is required for seams, periodic surfaces, poles and degenerated edges.

Every accepted physical TRI3 must contain three distinct `MeshNodeId` values even when multiple chart aliases exist. Distinct chart vertices alone are not sufficient evidence of a valid physical triangle.

## 5. Face-local chart-vertex reconciliation

A B-Rep wire may be topologically closed while neighboring pcurve endpoints are not bit-identical in UV. Each Face therefore performs a deterministic face-local reconciliation stage that maps tolerance-consistent wire endpoints to canonical chart vertices while retaining FaceUse/coedge provenance.

CAD tolerance is permitted for CAD consistency and B-Rep reconciliation only. It is not a FEM-topology epsilon. Exact/deterministic triangulation identity begins after canonical chart vertices are established.

## 6. First reference method

The first reference production candidate is **face-local parameter-space constrained Delaunay triangulation**.

An **advancing-front** method remains an independent research/comparison candidate. External implementations may be studied for theory, behavior and benchmarks; source-code transcription into Dynamics26 is forbidden.

The surface triangulator consumes canonical face-local chart vertices, FaceUse-specific constrained UV chains and the shared physical boundary-node identities.

## 7. Parametric surface metric

For `S(u,v)` with derivatives `Su` and `Sv`:

```text
G = [ Su·Su  Su·Sv
      Su·Sv  Sv·Sv ]
```

For isotropic physical target size `h`:

```text
M = G / h^2
```

This UV/chart metric is refinement and quality guidance only.

## 8. Final physical-space authority

Final acceptance is measured in physical 3D CAD space and must cover:
- surface/chord deviation,
- normal deviation,
- triangle orientation,
- zero-area and repeated-physical-node rejection,
- shared-edge physical-node conformity,
- source-Face containment,
- GeometryEntityId provenance,
- deterministic topology/fingerprint replay,
- typed explicit failure.

A chart-space-valid triangle is not accepted if its realized 3D triangle violates physical CAD criteria.

## 9. Seams, periodicity, poles and degenerated edges

Periodic seams may create separate FaceUse/chart aliases on the same Face while referencing the same physical boundary nodes. A degenerated CAD edge may have chart extent while collapsing to one physical point; it shall not create duplicate physical vertices in a final TRI3.

Unsupported or inconsistent seam/pole topology fails explicitly. Display tessellation and generic geometric epsilons are not repair mechanisms.

## 10. Provenance

At minimum:
- TRI3 interior -> source Face `GeometryEntityId`,
- edge-constrained mesh node/edge -> source Edge `GeometryEntityId`,
- CAD-vertex mesh node -> source Vertex `GeometryEntityId`.

`GeometryAssociationMap` is the persistent bridge. Coordinate matching is not provenance.

## 11. Qualification boundary

D26-M3-FREEZE-1 freezes research architecture and gates sufficiently for future DEV-MESH-P2 implementation after its P1/M2 dependency is qualified enough.

It does **not** mean a CAD surface mesher exists, any M3 executable gate has passed, or arbitrary-CAD TET4 is product-capable.

Qualification authority: `M3_QUALIFICATION_GATE_MATRIX.md`.
