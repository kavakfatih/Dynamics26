# M1.3 Degeneracy Experiment Plan

## EXP-MESH-0130 — Exact duplicate canonicalization
Acceptance:
- one site per exact coordinate,
- source provenance preserved,
- no zero-length Delaunay edge from duplicate records,
- canonical site set independent of input record order.

## EXP-MESH-0131 — Affine dimension
Corpora:
- identical,
- collinear,
- coplanar,
- minimally 3D,
- near-coplanar but exactly 3D.

Acceptance:
- all-coplanar never creates positive-volume tetrahedra,
- exact nonzero near-coplanar set remains 3D.

## EXP-MESH-0132 — Symbolic orientation oracle
Build a test-only exact formal perturbation oracle.

Acceptance:
- exact nonzero predicates unchanged,
- exact-zero cases receive deterministic symbolic signs,
- permutation identities remain consistent.

## EXP-MESH-0133 — Co-spherical insphere tie
Corpora:
- five points on one sphere,
- cube vertices,
- regular polyhedra,
- exact dyadic co-spherical sets.

Acceptance:
- valid Delaunay topology,
- no flat stored tetrahedra,
- deterministic completion under stable PointId policy.

## EXP-MESH-0134 — Input permutation
Keep site coordinates and PointIds fixed, permute input record order.

Acceptance:
- identical canonical topology fingerprint.

## EXP-MESH-0135 — Insertion-order independence
Keep symbolic identity order fixed but vary Delaunay insertion order.

Acceptance:
- same canonical topology fingerprint.

If not, symbolic policy is incomplete or insertion algorithm is not canonical.

## EXP-MESH-0136 — Near-degenerate continuity
Perturb exact degenerate cases by exact representable positive/negative offsets.

Acceptance:
- any exact nonzero predicate overrides symbolic tie,
- symbolic path runs only at exact zero.

## EXP-MESH-0137 — Commercial degeneracy corpus
Prepare:
- duplicate face,
- duplicate edge,
- short edge,
- sliver face,
- tiny gap,
- open shell,
- nearly coincident bodies.

When ANSYS / COMSOL / Marc access is available, record diagnostics, repair controls, topology result and meshing outcome.

## EXP-MESH-0138 — Open-source topology cross-check
Use CGAL/Gmsh/TetGen externally on small degenerate site sets.

Compare validity/determinism classes, not exact topology equality; different valid perturbation policies may choose different triangulations.
