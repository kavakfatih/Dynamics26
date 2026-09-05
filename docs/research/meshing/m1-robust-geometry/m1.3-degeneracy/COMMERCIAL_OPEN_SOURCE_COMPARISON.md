# M1.3 Comparison — Commercial CAE and Open-Source Degeneracy Behavior

## ANSYS

Public ANSYS documentation exposes explicit detection/repair of:
- short edges,
- slivers,
- duplicate edges,
- duplicate faces,
- self-intersections,
- misalignments,
- unconnected edges,
- overlapping faces.

ANSYS also documents that tetra meshing requires a sufficiently closed volume and that duplicate nodes/faces can prevent or complicate mesh generation.

### Dynamics26 lesson
Duplicate/invalid topology is a preprocessing/domain-validity concern. Do not hide it with Delaunay symbolic perturbation.

## COMSOL

COMSOL 6.4 documents:
- Check for tolerance faults and invalid CAD entities,
- Repair for invalid manifolds, self-intersections, missing manifolds, invalid edge/vertex tolerances and face-to-face inconsistencies,
- deletion of short edges/sliver faces under explicit repair tolerance,
- geometry degeneracies that can cause meshing/analysis problems.

### Dynamics26 lesson
CAD degeneracy belongs to explicit diagnosis/repair:
```text
CAD degeneracy
!=
Delaunay tie in an otherwise valid site set
```

## Marc / Mentat

Public Marc community material confirms practical node-equivalence/duplicate-node workflows and the need to resolve free edges/gaps before tetra volume meshing.

No public source reviewed here specifies Marc's internal symbolic perturbation or exact predicate implementation.

### Dynamics26 lesson
Keep node/site equivalencing, surface closure and tetrahedralization degeneracy as separate layers.

## CGAL

Public CGAL triangulation documentation exposes two useful behaviors:

1. inserting a point that coincides with an existing vertex leaves the triangulation unchanged and returns the existing vertex;
2. co-spherical Delaunay ambiguity is resolved by a symbolic perturbation scheme.

This supports:
- exact duplicate → canonical site,
- distinct co-spherical → symbolic tie.

## Gmsh source architecture study

High-level source observation:
- robust `insphere` is evaluated,
- exact zero enters a separate symbolic-perturbation path,
- predicate code and Delaunay topology code are separate concerns.

Dynamics26 does not copy Gmsh ordering, formulas or source structure.

## TetGen release study

TetGen release notes state that symbolic perturbation was introduced to remove spherical degeneracies in constrained Delaunay tetrahedralization.

This is architecture evidence only.

## Summary

| Problem | Commercial behavior | Open-source lesson | Dynamics26 |
|---|---|---|---|
| Exact duplicate point/node | diagnose/merge/equivalence | canonical insertion exists | canonical site |
| Near-coincident geometry | tolerance/repair | varies | explicit conditioning only |
| Open shell/free edges | explicit diagnostic | boundary validation | typed failure |
| Co-spherical ambiguity | internal not public | symbolic perturbation documented | M1.3 S0 candidate |
| All-coplanar 3D sites | not used as source truth | dimension-aware triangulation | dimension-reduced state |
