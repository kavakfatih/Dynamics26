# Degeneracy Taxonomy

## D0 — Invalid numeric input

Examples:
- NaN,
- infinity.

Policy:
- hard invalid-input failure.

## D1 — Exact duplicate site

Two records have exactly the same stored binary64 coordinates.

Policy:

```text
multiple source records
→ one canonical geometric site
→ preserve provenance/multiplicity metadata
```

Do not create zero-length Delaunay edges by symbolically separating duplicate records.

## D2 — Near-coincident but distinct sites

Coordinates are close but not exactly equal.

Policy:
- remain distinct in M1/M2,
- only a future explicit geometry-conditioning operation may merge them.

This preserves:

```text
exact topology arithmetic
!=
repair tolerance
```

## D3 — Affine-dimension deficiency

Examples:
- all points identical,
- collinear site set,
- coplanar site set for requested 3D tetrahedralization.

Policy:
- detect affine dimension,
- report a dimension-reduced state,
- do not manufacture positive-volume tetrahedra.

## D4 — Local orientation degeneracy

Example:
- four distinct points are exactly coplanar inside an otherwise 3D site set.

Policy depends on context:
- stored tetrahedron may never be flat,
- geometric location may report ON_FACE/ON_EDGE,
- symbolic policy may be used only for a combinatorial tie that assumes general position.

## D5 — Delaunay sphere degeneracy

Five or more distinct 3D sites are co-spherical.

Consequence:
- the Delaunay tetrahedralization may not be unique.

Policy candidate:
- stable symbolic perturbation using PointId order.

## D6 — Invalid domain/boundary degeneracy

Examples:
- duplicate CAD face,
- sliver face,
- self-intersection,
- open shell,
- non-manifold boundary,
- collapsed CAD edge.

Policy:
- geometry conditioning / surface validation / boundary-recovery diagnostic.

Do not use Delaunay symbolic perturbation to pretend an invalid engineering domain is valid.

## Classification table

| Class | Example | Dynamics26 response |
|---|---|---|
| D0 | NaN | fail |
| D1 | exact duplicate coordinate | canonicalize |
| D2 | near duplicate | preserve unless explicit repair |
| D3 | all coplanar | dimension-reduced state |
| D4 | local coplanarity | context-specific classification/tie |
| D5 | co-spherical distinct sites | symbolic Delaunay tie candidate |
| D6 | open/non-manifold CAD | diagnostic/repair pipeline |

## No universal epsilon

A single tolerance cannot safely replace this taxonomy.

The same geometric distance can represent:
- valid small feature,
- CAD defect,
- intentionally separate interfaces,
- unit-scaled geometry.

Merge/repair thresholds therefore belong to explicit engineering operations with scope and provenance.
