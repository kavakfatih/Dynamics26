# M1 Theory — Robust Geometry

## 1. Why this is the first meshing problem

Delaunay, constrained Delaunay, advancing-front and many mesh-quality algorithms repeatedly ask discrete geometric questions:

- on which side of a line/plane is a point?
- is a point inside, outside or exactly on a circumcircle/circumsphere?
- is a tetrahedron positively oriented, negatively oriented or degenerate?

The output is a **topological decision**. A wrong sign can create a wrong cavity, wrong face orientation, invalid adjacency or non-manifold connectivity.

Shewchuk's robust-predicate work emphasizes that orientation and circle/sphere tests are determinant-sign evaluations and that ordinary floating-point roundoff can return the wrong result when the determinant is close to zero. The important point is not that the computed determinant magnitude is slightly inaccurate; the control-flow branch itself can become wrong.

Primary sources: TH-006, TH-007 in `SOURCE_REGISTRY.md`.

## 2. Predicates vs constructions

A useful computational-geometry separation is:

### Predicate

Returns a discrete relation:

```text
NEGATIVE
ZERO
POSITIVE
```

Examples:
- orient2d,
- orient3d,
- incircle,
- insphere.

### Construction

Computes a new geometric value:

- circumcenter,
- line/plane intersection,
- projected point,
- surface parameter,
- midpoint.

CGAL publicly distinguishes exact predicates from geometric constructions and provides a kernel where predicates are exact while many constructions remain inexact. This is an important architecture lesson, not a CGAL API to copy.

Dynamics26 should therefore avoid implementing a predicate by first constructing an approximate plane/sphere and then comparing against it when a direct determinant predicate is available.

## 3. Why an epsilon is not enough

A common but unsafe pattern is:

```text
if abs(det) < epsilon:
    treat as zero
```

Problems:

1. determinant scale changes with geometry size,
2. determinant dimension changes by predicate,
3. translation and cancellation change numerical behavior,
4. one epsilon can classify a genuinely nonzero topological relation as zero,
5. the resulting artificial zero then forces arbitrary topology.

A tolerance can be valid for a modeling decision, for example deciding whether two CAD vertices should be healed. It is not a universal substitute for a certified predicate sign.

## 4. Filtering

The practical robust-computation pattern seen in the literature is:

```text
fast floating-point evaluation
+ certified error estimate
        │
        ├── sign is provably outside error interval
        │      → accept fast sign
        │
        └── sign is uncertain
               → more accurate / exact fallback
```

Fortune/Van Wyk and later filtering literature show why this strategy is attractive: most ordinary cases stay close to floating-point speed, while difficult near-degenerate cases pay the extra exactness cost.

Sources: M1-TH-001, M1-TH-002, M1-TH-003.

## 5. Adaptive exactness

Shewchuk's approach uses adaptive arithmetic: perform only as much extra work as needed to certify the sign. The exact representation of a binary floating-point input can be manipulated without pretending the original coordinates have more physical accuracy than they contain.

Important distinction:

> Exact predicate means exact with respect to the input floating-point numbers, not exact with respect to an unknowable ideal CAD geometry.

This is exactly what the meshing topology needs: consistent decisions on the coordinates actually supplied to the algorithm.

## 6. Degeneracy

True degeneracies exist:

- collinear points,
- coplanar points,
- cocircular points,
- cospherical points,
- duplicate points.

The predicate layer should report mathematical equality as `ZERO`.

The triangulation algorithm then needs a separate policy.

Candidate policy family:
- explicit special-case handling,
- deterministic symbolic perturbation / Simulation of Simplicity,
- constrained input preprocessing where appropriate.

Edelsbrunner and Mücke's Simulation of Simplicity is a key theoretical source for systematic degeneracy handling.

**Dynamics26 rule:** do not hide algorithmic tie-breaking inside the robust predicate API.

## 7. Commercial-program lesson

Public ANSYS, COMSOL and Marc documentation exposes geometry repair/defeaturing/tolerance controls, but does not publicly specify their internal orient3d/insphere implementation.

Therefore:

- commercial products are useful benchmarks for **geometry tolerance and repair UX**,
- they are not sources for an undocumented exact-predicate algorithm.

COMSOL explicitly documents geometry repair tolerances used to merge close geometric entities. ANSYS documents defeature size and topology protection. Those behaviors reinforce that geometry-cleanup policy is an explicit model operation, not a replacement for internal topological correctness.

## 8. Dynamics26 separation

Target layers:

```text
OCCT B-Rep tolerances
        ↓
GeometryTolerancePolicy
        ↓
geometry validation / healing decisions
        │
        └─────────────────────────────┐
                                      │
Algorithm point coordinates          │
        ↓                             │
RobustPredicates                      │
        ↓                             │
exact sign decisions                  │
        ↓                             │
Delaunay / surface / volume topology ◄┘
```

The layers exchange geometry, but their semantics are not merged.

## 9. Research conclusion

M1 recommends a **filtered exact-predicate architecture** as the leading design, with an independently specified exact test oracle and an explicit higher-level degeneracy policy.

This is still a research conclusion. Performance and implementation complexity will be measured before ADR-MESH-0005 is accepted.
