# M2.0 — Delaunay Mathematics Contract

Status: PROPOSED / derivation record
Date: 2026-09-05

## Repository predicate convention

M2 consumes the qualified M1 predicates in include/femcae/meshing/RobustPredicates.h.

Committed M1 fixtures establish the actual Dynamics26 convention:
- orient3d(e_x,e_y,e_z,0) is Positive,
- with the same positively oriented tetrahedron, an interior query gives Positive InSphere,
- swapping tetra orientation reverses the InSphere sign,
- an outside query gives the opposite sign.

Therefore M2 must not copy sign conventions from another implementation without translating them to
the Dynamics26 matrix layout.

Production finite cells are stored with:

    orient3d(v0,v1,v2,v3) == Positive

Under that invariant:
- InSphere Positive = inside/conflict,
- InSphere Negative = outside/non-conflict,
- InSphere Zero = exact co-spherical tie requiring the M2 symbolic rule.

For an arbitrary-order oracle tetra, orientation-independent inside semantic is the sign of
orient3d multiplied by insphere.

## Orient3D

Dynamics26 M1 evaluates the sign of the row determinant:

    | ax ay az 1 |
    | bx by bz 1 |
    | cx cy cz 1 |
    | dx dy dz 1 |

Denote it O(a,b,c,d).

M2 finite-cell invariant is O(a,b,c,d) > 0.

No epsilon threshold defines coplanarity. Exact M1 Zero is geometric truth.

## Point-in-tetra without division

For positively oriented T=(a,b,c,d) and query p:

    N0 = O(p,b,c,d)
    N1 = O(a,p,c,d)
    N2 = O(a,b,p,d)
    N3 = O(a,b,c,p)
    D  = O(a,b,c,d) > 0

Barycentric coordinates are lambda_i = Ni / D, but location needs only signs.

Containment requires all Ni >= 0.

If contained:
- zero count 0 -> CELL interior,
- zero count 1 -> FACET,
- zero count 2 -> EDGE,
- zero count 3 -> VERTEX.

A zero numerator does not mean stop if another numerator is negative; then the query is on a
supporting-plane extension but outside the tetrahedron.

A negative Ni identifies a violated facet opposite vertex i.

## Finite InSphere determinant

M1 uses row-major lifted coordinates:

    | x0 y0 z0 t0 1 |
    | x1 y1 z1 t1 1 |
    | x2 y2 z2 t2 1 |
    | x3 y3 z3 t3 1 |
    | x4 y4 z4 t4 1 |

where ti = xi^2 + yi^2 + zi^2.

For a positively oriented first tetrahedron, raw determinant sign has the Dynamics26 semantic above.
Shewchuk's robust-predicate work is the numerical authority for certified sign decisions rather than
tolerance-based determinant magnitude.

## M1 symbolic oracle versus production M2 tie rule

M1.9 includes a test-only general symbolic oracle that perturbs spatial coordinate components. It is
valuable as an independent degeneracy oracle.

For production 3D Delaunay, M2 adopts the specialized Devillers-Teillaud idea: perturb only the
fourth/lifted coordinate.

Reasons:
- real x/y/z positions never move,
- exact Orient3D Zero remains real coplanarity,
- a symbolic topology choice cannot hide a flat tetrahedron,
- the co-spherical tie reduces to exact orientation cofactors.

Devillers and Teillaud show that this selects a unique PP-regular triangulation for a fixed total
site order when sites are not all coplanar. The limit is weakly Delaunay/regular; it should not be
overclaimed as necessarily realizable by a non-degenerate coordinate perturbation in every
degenerate configuration.

Reference: O. Devillers, M. Teillaud, Perturbations for Delaunay and weighted Delaunay 3D
triangulations, Computational Geometry 44(3), 2011, DOI 10.1016/j.comgeo.2010.09.010.

## Dynamics26 lift-only symbolic InSphere

For global symbolic priority sigma(i), conceptually:

    ti' = ti + epsilon^(sigma(i)), epsilon -> 0+

No numeric epsilon is ever constructed.

Because the determinant is linear in the lift column, for the Dynamics26 row-major
[x y z t 1] layout:

    D_epsilon =
    D
    - O(p1,p2,p3,p4) * eps^sigma(0)
    + O(p0,p2,p3,p4) * eps^sigma(1)
    - O(p0,p1,p3,p4) * eps^sigma(2)
    + O(p0,p1,p2,p4) * eps^sigma(3)
    - O(p0,p1,p2,p3) * eps^sigma(4)

The sign sequence differs from formulas printed with transposed/column-oriented determinant layouts.
This row-major form was independently re-derived against the current M1 determinant convention.

When raw D is non-zero, return the M1 sign unchanged.

When raw D is zero:
1. order the five participating sites by global symbolic priority,
2. inspect the corresponding exact orientation cofactor from smallest exponent upward,
3. the first non-zero cofactor fixes symbolic determinant sign.

For a valid finite tetrahedron the final tetra-only cofactor is non-zero, so the tie resolves.

No base-4 runtime exponent or polynomial object is necessary for this specialized predicate.

## Symbolic priority and insertion order are different domains

Current M1 canonicalization assigns stable PointId values after finite validation, signed-zero
normalization, exact-coordinate duplicate grouping and deterministic raw-bit site-key ordering.

Leading M2 policy:

    SymbolicPriority = relative ascending canonical PointId order

The algorithm uses only relative total order; it does not numerically evaluate epsilon^PointId.

Insertion order is independent:

    symbolic identity order != insertion order

This enables later Morton/Hilbert/BRIO experiments without redefining degeneracy semantics.

## Ghost/infinite-cell conflict

Let finite hull facet F=(a,b,c) be outward oriented, meaning opposite finite vertex q satisfies:

    O(a,b,c,q) < 0

For query p let H=O(a,b,c,p).

- H > 0: exterior half-space -> ghost conflict.
- H < 0: triangulation side -> no ghost conflict.
- H == 0: exact coplanarity -> use hull-triangle circumdisk.

This agrees with public infinite-cell sphere semantics documented by CGAL and with the
Devillers-Teillaud perturbation treatment.

### Exact coplanar circumdisk test

Project the four coplanar points without a floating heuristic.

Try coordinate-plane projected orientation in a fixed order such as XY, XZ, YZ. Use the first
projection whose exact orient2d is non-zero. A non-collinear 3D triangle must have at least one.

Orient projected triangle positively, then:
- incircle Positive -> inside circumdisk -> ghost conflict,
- incircle Negative -> outside -> no conflict,
- incircle Zero -> apply 2D lift-only symbolic tie.

For M1 row-major 2D matrix [x y t 1], lift cofactors alternate:

    + O2(p1,p2,p3)
    - O2(p0,p2,p3)
    + O2(p0,p1,p3)
    - O2(p0,p1,p2)

## Exact-zero policy

Geometric classification preserves exact zero:
- duplicate coordinate,
- collinearity,
- coplanarity,
- point on face/edge,
- zero-volume candidate.

Delaunay topology resolves exact zero only for:
- co-spherical InSphere,
- co-circular InCircle on ghost coplanarity.

Symbolic perturbation chooses topology; it never changes stored coordinates.

## Golden degenerate sets

### Five co-spherical sites

Canonical sites:

    1 (0,0,0)
    2 (0,0,1)
    3 (0,1,0)
    4 (1,0,0)
    5 (1,1,1)

All five are co-spherical. Under ascending PointId symbolic priority and the lift-only contract,
exact determinant enumeration gives canonical finite topology:

    1 2 3 4
    2 3 4 5

This is a future M2 golden fixture, not yet executable production evidence.

### Unit cube

Canonical site order:

    1 (0,0,0)
    2 (0,0,1)
    3 (0,1,0)
    4 (0,1,1)
    5 (1,0,0)
    6 (1,0,1)
    7 (1,1,0)
    8 (1,1,1)

The lift-only contract was independently cross-checked by exact determinant enumeration during M2.0
research. Expected canonical finite tetra connectivity:

    1 2 3 5
    2 3 4 5
    2 4 5 6
    3 4 5 7
    4 5 6 7
    4 6 7 8

Qualification should run all 8! = 40,320 insertion permutations and require the same canonical
finite/hull fingerprint under fixed symbolic priority.

## Quality is a different contract

Delaunay/weak-Delaunay topology is not automatically a high-quality FEM mesh. Co-spherical tie
selection can affect tetra quality.

M2 freezes correctness and determinism. Sliver treatment and quality-driven topology optimization
belong to M6 and must not be mixed with M2 predicate truth.
