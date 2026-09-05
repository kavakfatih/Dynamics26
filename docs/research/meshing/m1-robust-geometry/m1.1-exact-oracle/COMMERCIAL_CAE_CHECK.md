# M1.1 Commercial CAE Check — ANSYS / COMSOL / Marc

## 1. Purpose

Commercial CAE software is checked in every meshing research stage, but only for publicly documented behavior.

This M1.1 question is:

> Do mature CAE products expose geometry tolerance/repair as explicit modeling/meshing policy, and do they publicly disclose exact topological-predicate internals?

Finding:

- explicit tolerance/repair/defeature controls: **yes**,
- public exact orient3d/insphere implementation specification: **not found**.

Therefore commercial products validate the **architecture separation**, not the exact arithmetic algorithm.

## 2. ANSYS

### Repair Topology

ANSYS Meshing documents Repair Topology operations including:

- topology-edge/face repair,
- interior-edge suppression,
- face merging,
- partial defeature,
- thin-face removal,
- short-edge collapse,
- sharp-angle face removal,
- face pinching,
- topology protection.

Current ANSYS documentation also exposes a `RepairTopologyTolerance` setting intended to reduce topology variability and prevent collapse/defeaturing of thin or short topology.

### Mesh-based defeaturing

ANSYS documents:

- program-controlled defeaturing,
- explicit global Defeature Size,
- disabling defeaturing,
- selective preservation through Named Selection.

### Dynamics26 interpretation

ANSYS makes topology modification an explicit meshing/model operation.

Dynamics26 should similarly distinguish:

```text
geometry cleanup policy
!=
predicate correctness
```

A future Dynamics26 repair tolerance may influence:
- short-edge collapse,
- small-face merge,
- defeaturing.

It must not be passed as an epsilon to `orient3d` or `insphere`.

### Source limitation

ANSYS source code is proprietary. No public documentation reviewed in M1.1 specifies its internal robust determinant implementation.

## 3. COMSOL

COMSOL documents default repair tolerance types:

- automatic,
- relative,
- absolute.

The documented default relative repair tolerance for the COMSOL kernel is `1e-6`, and changing the default applies to newly created applicable geometry features rather than retroactively changing existing feature tolerances.

### Dynamics26 interpretation

This is an especially useful UX/architecture lesson:

> repair tolerance belongs to a geometry operation/configuration with lifecycle semantics.

It is not a global magic epsilon for all geometry calculations.

A Dynamics26 repair operation should eventually persist:
- tolerance type,
- value,
- affected scope,
- resulting geometry revision/provenance.

### Source limitation

COMSOL implementation source is not public. No internal predicate algorithm claim is made.

## 4. Marc / Mentat

Marc 2026.1 public product material states that Mentat meshing received new capabilities. Cadence's 2026.1 material describes:

- improved automatic meshing for solid sheets and 3D solids,
- smoother transitions between refinement regions,
- integrated mesh-density controls,
- user-defined fine-mesh hotspots,
- triangle/quadrilateral support for 2D/shell contexts,
- tetrahedral support,
- global remeshing workflows preserving tied-region relationships.

Historical Marc release documentation also shows defeaturing/import operations can fail for particular CAD features, reinforcing that geometry conditioning itself is a diagnosable pipeline stage.

### Dynamics26 interpretation

Marc reinforces two ideas:

1. meshing robustness is broader than one tetrahedralization kernel,
2. preprocessing/meshing diagnostics and nonlinear remeshing are distinct capabilities.

For M1 specifically, Marc does not provide a public source-level robust-predicate specification.

## 5. Cross-product conclusion

| Concern | ANSYS | COMSOL | Marc/Mentat | Dynamics26 |
|---|---|---|---|---|
| User-visible repair/tolerance | Strong | Strong | Present in CAD/meshing workflows | Explicit future policy |
| Defeaturing/topology modification | Explicit | Geometry operations | Explicit/history documented | Separate operation |
| Persistent/protected topology | Named Selection / protection | Selection lifecycle | sets/relationships | GeometryEntityId + Named Selection |
| Exact predicate implementation public | No | No | No | Original, literature-backed |
| Automatic tetra meshing | Yes | Yes | Yes | Later M2–M7 |
| Exact oracle test infrastructure visible | Internal/unknown | Internal/unknown | Internal/unknown | Explicit M1.1 |

## 6. Product-quality lesson

A professional Dynamics26 mesher should eventually tell the user **which layer failed**:

```text
CAD validity
Repair / defeature
Curve sampling
Surface meshing
Boundary recovery
Volume meshing
Quality optimization
Solver qualification
```

A generic "mesh failed" message is not enough.

M1.1 begins this discipline by making predicate invalid-input and true-degeneracy semantics explicit.
