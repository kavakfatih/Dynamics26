# Dyadic Integer Mathematics for the Exact Oracle

## 1. Binary64 as an exact rational

A finite binary64 value is exactly representable as:

```text
x = m × 2^e
```

or equivalently:

```text
x = n / 2^q
```

with integer `n`.

This includes normal and subnormal values.

The exact-oracle design does not need to reinterpret a decimal spelling.

## 2. Common integer scale

For all coordinates used in one predicate, let their exact denominators be:

```text
2^q1, 2^q2, ..., 2^qN
```

Choose:

```text
Q = max(q1, ..., qN)
S = 2^Q
```

Then:

```text
X_i = S x_i
```

is an integer for every coordinate.

Since `S` is positive, orientation relations are unchanged.

## 3. orient2d scale proof

Translated orient2d is:

```text
det [
 ax-cx   ay-cy
 bx-cx   by-cy
]
```

After all coordinates are multiplied by `S`, each matrix entry is multiplied by `S`.

A 2×2 determinant therefore becomes:

```text
det' = S^2 det
```

and sign is preserved.

## 4. orient3d scale proof

Translated orient3d has three coordinate-difference columns.

Uniform coordinate scaling gives:

```text
det' = S^3 det
```

with positive `S^3`.

## 5. incircle scale proof

Using the lifted matrix columns:

```text
x, y, x²+y², 1
```

uniform coordinate scaling yields column factors:

```text
S, S, S², 1
```

therefore:

```text
det' = S^4 det
```

and sign is preserved.

## 6. insphere scale proof

The 3D lifted columns are:

```text
x, y, z, x²+y²+z², 1
```

which scale as:

```text
S, S, S, S², 1
```

therefore:

```text
det' = S^5 det
```

and sign is preserved.

## 7. Translation before lifting

For incircle/insphere, translated determinant forms subtract the query point or another reference point before forming squared norms.

This can reduce integer growth.

However, the oracle should first prioritize transparency and correctness.

Two forms may be deliberately implemented and compared:

- homogeneous lifted matrix,
- translated lifted matrix.

Agreement becomes another metamorphic/reference check.

## 8. Integer growth

Worst-case integer sizes can become large when:

- coordinate magnitudes are huge,
- subnormals force a large common `Q`,
- squared lifted coordinates are formed,
- determinant products multiply several large integers.

This is acceptable for a test oracle.

Performance is measured but not optimized at the expense of simplicity.

## 9. Bareiss as optional fast exact determinant

After integer scaling, Bareiss fraction-free elimination is attractive because it:

- stays in exact integer arithmetic,
- uses exact divisions,
- generally controls coefficient growth better than naive rational elimination.

But using Bareiss is not required for oracle correctness.

Initial recommended sequence:

```text
permutation determinant
→ validate
→ add Bareiss cross-check
→ benchmark
```

## 10. Production implication

The dyadic-integer oracle is not currently proposed as the product fallback.

It is a correctness authority.

The production fallback can later use adaptive expansion arithmetic or another independently justified exact-sign method, provided it matches the oracle on the complete corpus.
