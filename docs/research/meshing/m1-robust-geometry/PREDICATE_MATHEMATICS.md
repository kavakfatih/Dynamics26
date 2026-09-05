# Predicate Mathematics

## 1. Result type

The production contract should be sign-based:

```text
PredicateSign
- Negative
- Zero
- Positive
```

The determinant magnitude may be useful internally for a fast filter but is **not** the public topological result.

## 2. orient2d

For points

```text
a = (ax, ay)
b = (bx, by)
c = (cx, cy)
```

define

```text
orient2d(a,b,c)
= det [
    ax-cx   ay-cy
    bx-cx   by-cy
  ]
```

or equivalently

```text
(ax-cx)(by-cy) - (ay-cy)(bx-cx)
```

Interpretation under the chosen coordinate orientation:

- positive: one orientation,
- negative: opposite orientation,
- zero: collinear.

The exact naming of clockwise/counter-clockwise must be frozen by unit tests rather than assumed from prose.

## 3. orient3d

For

```text
a,b,c,d in R^3
```

use the translated determinant

```text
orient3d(a,b,c,d)
= det [
  ax-dx  ay-dy  az-dz
  bx-dx  by-dy  bz-dz
  cx-dx  cy-dy  cz-dz
]
```

The sign determines tetrahedral orientation; zero means coplanar.

The determinant is proportional to signed tetrahedral volume, but the robust predicate API should not be reused as the engineering volume calculation API.

Why:

- predicate implementations may use filters/scaling/internal representations,
- mesh quality requires metric values and units,
- topology needs only a certified sign.

## 4. incircle

For four 2D points, an orientation-aware incircle test can be written with lifted coordinates.

A standard determinant form is:

```text
det [
 ax  ay  ax²+ay²  1
 bx  by  bx²+by²  1
 cx  cy  cx²+cy²  1
 dx  dy  dx²+dy²  1
]
```

The sign must be interpreted together with the orientation of the defining triangle `a,b,c`.

Therefore the API contract should document:

```text
incircle(a,b,c,d)
requires/records orientation convention of (a,b,c)
```

rather than treating raw determinant sign as universally "inside".

## 5. insphere

For five 3D points, the corresponding lifted determinant is:

```text
det [
 ax ay az ax²+ay²+az² 1
 bx by bz bx²+by²+bz² 1
 cx cy cz cx²+cy²+cz² 1
 dx dy dz dx²+dy²+dz² 1
 ex ey ez ex²+ey²+ez² 1
]
```

Again, the sign is orientation-dependent. The production contract should establish one canonical tetra orientation and define inside/outside relative to that orientation.

## 6. Translation

Translated forms, subtracting one input point before determinant evaluation, are algebraically equivalent and reduce the size of intermediate terms.

However:

> algebraic translation is not a proof of numerical robustness.

Cancellation can still make the sign uncertain.

## 7. Scaling

Positive uniform scaling should preserve predicate sign for nondegenerate inputs.

For a scale factor `s > 0`:

- orient2d magnitude scales with a power of `s`,
- orient3d magnitude scales with a higher power,
- incircle/insphere lifted determinants scale differently again.

This is another reason a fixed epsilon on raw determinant magnitude is invalid.

Scaling invariance is useful as a metamorphic test.

## 8. Fast filter concept

Let:

```text
d_fast = floating-point determinant estimate
E      = conservative error bound
```

Then:

```text
if d_fast > +E → Positive is certified
if d_fast < -E → Negative is certified
otherwise      → fallback
```

The exact form of `E` is predicate- and evaluation-order-specific.

Dynamics26 must derive and verify its own filter formula from mathematical/numerical-analysis references. It must not reuse an external source file's constants or macro sequence by transcription.

## 9. Exact fallback concepts

Candidate original implementation strategies to evaluate:

### A. Floating-point expansion arithmetic

Represent one exact value as a sum of non-overlapping floating-point components and use error-free transforms for addition/multiplication.

Pros:
- strong literature,
- high performance for moderately difficult cases.

Cons:
- subtle implementation,
- sensitive to compiler floating-point transformations if assumptions are violated.

### B. Exact binary-rational / integer oracle

Because every finite IEEE binary floating-point number represents an exact rational of the form:

```text
integer × 2^exponent
```

an independent oracle can decompose inputs and evaluate the determinant with arbitrary-size integers/rationals.

Pros:
- conceptually simple reference truth,
- excellent for tests.

Cons:
- slower,
- production arbitrary-precision dependency or custom big integer would be substantial.

**M1 proposal:** B is the independent test-oracle model; A is the likely production-performance direction.

## 10. Exact zero

If the exact determinant is zero, the predicate returns `Zero`.

It must not silently choose a positive/negative sign to make Delaunay code easier.

A later Delaunay policy can map:

```text
Zero + stable point IDs
→ deterministic symbolic tie-break
```

while keeping mathematical truth observable.

## 11. Constructions

Circumcenter/circumsphere calculations are not predicates.

A future Delaunay implementation can use approximate constructions for acceleration, provided a certified predicate decides any topology-changing boundary case.

This resembles the general "exact predicates, inexact constructions" architecture documented by CGAL, but Dynamics26 will define its own interfaces and implementation.
