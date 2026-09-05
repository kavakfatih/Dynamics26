# M1 Data Structures and Contracts

This is an implementation-neutral design study.

## 1. Point identity

A meshing point needs two distinct concepts:

```text
PointId
→ stable algorithm/topology identity

Point3
→ coordinate value
```

Two records can be geometrically coincident yet have different identity before preprocessing. Conversely, one shared CAD vertex must not be duplicated merely because two faces reference it.

## 2. Predicate result

Proposed:

```text
enum class PredicateSign {
    Negative,
    Zero,
    Positive
}
```

Do not return a Boolean for orientation because `Zero` is engineering-relevant.

Do not expose raw determinant magnitude as the primary result.

## 3. Predicate API semantics

Conceptual interface:

```text
orient2d(a,b,c)       -> PredicateSign
orient3d(a,b,c,d)     -> PredicateSign
incircle(a,b,c,d)     -> PredicateSign
insphere(a,b,c,d,e)   -> PredicateSign
```

The API documentation freezes:

- coordinate convention,
- orientation convention,
- inside/outside convention,
- true-degeneracy behavior,
- finite-input requirement.

## 4. Predicate context

Avoid global mutable exact-arithmetic state if possible.

Candidate design:

```text
RobustPredicateKernel
- immutable machine/error constants
- stateless predicate calls
- optional external telemetry sink
```

Initialization should be deterministic and thread-safe.

## 5. Tolerance object

Separate type:

```text
GeometryTolerancePolicy
- absolute floor
- relative/model-scale rule
- CAD subshape tolerance access
- explicit repair limits
```

No method on this object is allowed to answer Delaunay orientation/insphere sign.

## 6. Basic topology records for M2

M1 should define enough semantics to make M2 possible without fixing storage prematurely.

### Tetrahedron topology

Conceptual:

```text
TetId
vertex[4]   : PointId
neighbor[4]: TetId / Boundary
```

Invariant:

- local vertex order corresponds to positive canonical orient3d,
- neighbor slot convention is documented relative to opposite vertex/face,
- degenerate tetra is never a valid stored topology record.

### Canonical face key

A topological face key can be based on sorted stable PointIds for equality/hash purposes while retaining an oriented local face representation separately.

This prevents geometric-coordinate comparison from being used as topology identity.

## 7. Geometry provenance

Later CAD meshing adds:

```text
PointId
├─ optional CAD Vertex/Edge/Face provenance
└─ parameter coordinate where relevant

BoundaryFacet
└─ GeometryEntityId
```

M1 predicates remain unaware of Qt, ProjectModel and NamedSelection.

## 8. Predicate/construction separation

Suggested namespace/module boundary:

```text
geometry::predicates
geometry::metrics
geometry::constructions
meshing::topology
```

Examples:

- `orient3d` belongs to predicates,
- `signedTetraVolume` belongs to metrics,
- `circumcenter` belongs to constructions,
- cavity adjacency belongs to meshing topology.

This prevents accidental use of approximate construction output as a topological predicate.

## 9. Error reporting

Predicate calls normally do not produce diagnostics for negative/zero/positive results.

Only invalid input is exceptional/error state.

A higher-level algorithm may produce diagnostics such as:

- duplicate input point,
- cospherical tie resolved by symbolic policy,
- degenerate surface input.

## 10. Memory/performance constraint

Do not over-engineer arbitrary-precision storage into every point/tetra record.

The common path stores normal binary64 coordinates. Exact/adaptive work is localized to predicate evaluation.

This preserves a compact mesh topology representation.
