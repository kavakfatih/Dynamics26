# M1.2 Commercial CAE Check — ANSYS / COMSOL / Marc

## ANSYS 2026 R1

Official ANSYS Meshing documentation exposes Repair Topology controls including:

- Edges of Faces Repair,
- Interior Edge Suppression,
- Interior Vertex Deletion,
- Face Merging,
- Edge Suppression,
- Partial Defeature,
- Thin Face Removal,
- Short Edge Collapse,
- Sharp Angle Face Removal,
- Face Pinching,
- Topology Protection.

The official API also exposes `RepairTopologyTolerance` as a topology-conditioning tolerance.

### Dynamics26 lesson

Repair tolerance is an explicit topology-conditioning control.

It is not evidence that ANSYS decides Delaunay orientation with a single epsilon.

Dynamics26 continues to separate:

```text
GeometryConditioning
!=
RobustPredicates
```

ANSYS Fluent documentation also exposes distinct tetra sliver-improvement operations such as refinement, swapping and smoothing, reinforcing that generation and quality optimization are separate stages.

## COMSOL 6.4

COMSOL exposes repair tolerance as explicit geometry-operation state:

- Automatic,
- Relative,
- Absolute.

Documentation states that sufficiently close entities may be merged according to the repair tolerance.

The Repair feature separately exposes:
- delete small details,
- fix errors,
- heal edges,
- minimize tolerances,
- face-to-face repair.

### Dynamics26 lesson

Geometry repair is scoped, parameterized and diagnosable.

It should become a future explicit Dynamics26 geometry-conditioning operation, not a hidden predicate epsilon.

## Marc / Mentat 2026.1

Hexagon's current Marc product page states that Marc 2026.1 includes advances in Mentat meshing.

Public Marc support material also shows practical tetra-volume prerequisites:
- surface mesh must represent a closed volume,
- gaps/free edges prevent tetra volume meshing,
- adaptive remeshing is a distinct later workflow.

### Dynamics26 lesson

Typed meshing diagnostics should identify whether failure belongs to:
- surface closure,
- volume generation,
- quality,
- later remeshing.

No public source reviewed in M1.2 specifies Marc's internal orient3d/insphere arithmetic.

## Cross-check

| Concern | ANSYS | COMSOL | Marc/Mentat | Dynamics26 |
|---|---|---|---|---|
| Repair tolerance | Explicit | Explicit | geometry/meshing workflows | Future explicit conditioning |
| Defeaturing/topology change | Explicit | Explicit | available workflows | Explicit, never hidden |
| Tetra quality improvement | Explicit | mesh/remesh controls | remeshing/adaptivity | M6 later |
| Public exact predicate internals | No | No | No | Original literature-backed kernel |

## Conclusion

Commercial comparison supports the architecture, not the internal arithmetic formula:

```text
user-controlled geometry repair
!=
certified internal topological decisions
```
