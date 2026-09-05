# M1.8 Homogeneous Fast-Filter Bound Proof

**Status:** executable proof synchronized with source  
**Date:** 2026-09-05

## 1. Why this document exists

M1.2 originally derived a conservative F0 error model for translated predicate expansions.

The implemented M1.8 fast path instead evaluates a homogeneous determinant:

- 3x3 for orient2d,
- 4x4 for orient3d,
- lifted 4x4 for incircle,
- lifted 5x5 for insphere.

Therefore the executable graph needs its own bound.

Passing random fixtures is verification evidence, but it is not the mathematical proof of the certification envelope.

## 2. Floating-point assumptions

The source is compiled with:

```text
-fno-fast-math
-ffp-contract=off
```

and contains a `__FAST_MATH__` compile guard.

The fast path accepts only finite normal nonzero arithmetic or exact zero cases covered by the range checks. Overflow, nonzero subnormal intermediates and suspicious underflow-to-zero cause exact fallback.

Let:

```text
u = 2^-53
gamma_n = n*u / (1 - n*u)
```

## 3. Worst supported evaluation graph

The largest graph is lifted 5x5 insphere.

For one determinant permutation term:

- a 3D lift `x^2+y^2+z^2` has a longest rounding path of at most 3 operations,
- the determinant term performs at most 5 sequential multiplications,
- therefore one computed term differs from its exact term by at most `gamma_8`.

The determinant has at most:

```text
5! = 120
```

signed permutation terms.

Charging all 120 sequential additions gives a conservative total path:

```text
8 + 120 = 128
```

therefore:

```text
|Dhat - D| <= gamma_128 * sum(|T_i|)
```

for exact terms `T_i`.

## 4. Relating exact terms to the computed permanent

If `That_i` is one computed term:

```text
|T_i| <= |That_i| / (1 - gamma_8)
```

The fast path separately accumulates:

```text
Phat = fl(sum(|That_i|))
```

Using a conservative 120-addition bound:

```text
sum(|That_i|)
<= Phat / (1 - gamma_120)
```

Thus:

```text
|Dhat - D|
<=
gamma_128
-------------------------------- * Phat
(1-gamma_8)(1-gamma_120)
```

This is the implemented homogeneous F0 reference bound.

## 5. Chosen executable coefficient

M1.8 uses:

```text
C = 2^-43 = 1024*u
```

and computes:

```text
Ehat = fl(C * Phat)
```

The final multiplication may round downward, so source qualification requires:

```text
C * (1-u)
>
gamma_128 / ((1-gamma_8)(1-gamma_120))
```

This inequality is now a C++ `static_assert` in `RobustPredicates.cpp`.

Therefore, under the explicitly checked fast-domain assumptions:

```text
|Dhat| > Ehat
→ sign(Dhat) is certified
```

The coefficient has substantial safety margin over the worst supported graph.

## 6. Smaller predicates

orient2d, orient3d and incircle use fewer lift/multiplication/summation operations than the 5x5 insphere bound.

They intentionally share the same larger coefficient.

This increases exact fallback frequency but reduces proof complexity.

## 7. Exact zero

The fast path never certifies Zero.

If the determinant is zero or lies inside the envelope:

```text
→ M1.7 exact dyadic fallback
```

Only the exact fallback may return `PredicateSign::Zero`.

## 8. Verification

The proof is complemented, not replaced, by:

- committed bit-exact fixtures,
- deterministic generated exact-oracle corpus,
- M1.9 adversarial/metamorphic corpus,
- macOS arm64 Debug/Release CI,
- fast/fallback telemetry.

Any future optimization that changes the evaluation graph requires a new proof or a new conservative bound.
