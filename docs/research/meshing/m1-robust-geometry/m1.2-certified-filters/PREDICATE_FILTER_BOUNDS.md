# Predicate-Specific F0 Bounds

For binary64:

```text
u = 2^-53
gamma_n = n u / (1 - n u)
```

These are conservative research bounds, not final optimized constants.

## orient2d

```text
D =
 (ax-cx)(by-cy)
-
 (ay-cy)(bx-cx)
```

```text
m = 2
dmax = 2
p = 3
K = 4
```

```text
E =
 gamma_4
 ---------------------------- * P_hat
 (1-gamma_3)(1-gamma_1)
```

Leading first-order size:

```text
≈ 4u * P_hat
```

## orient3d

The translated 3×3 determinant expands to six signed degree-3 monomials:

```text
m = 6
dmax = 3
p = 5
K = 10
```

```text
E =
 gamma_10
 ---------------------------- * P_hat
 (1-gamma_5)(1-gamma_5)
```

Leading first-order size:

```text
≈ 10u * P_hat
```

## incircle

Using a query-point-translated lifted expression and fully expanding into primitive monomials:

```text
m = 12
dmax = 4
p = 7
K = 18
```

```text
E =
 gamma_18
 ----------------------------- * P_hat
 (1-gamma_7)(1-gamma_11)
```

Leading first-order size:

```text
≈ 18u * P_hat
```

## insphere

Using the translated 4×4 lifted determinant and fully expanding:

```text
m = 72
dmax = 5
p = 9
K = 80
```

```text
E =
 gamma_80
 ----------------------------- * P_hat
 (1-gamma_9)(1-gamma_71)
```

Leading first-order size:

```text
≈ 80u * P_hat
```

This is deliberately conservative and potentially expensive.

## Summary

| Predicate | m | dmax | p | K | First-order factor |
|---|---:|---:|---:|---:|---:|
| orient2d | 2 | 2 | 3 | 4 | ~4u |
| orient3d | 6 | 3 | 5 | 10 | ~10u |
| incircle | 12 | 4 | 7 | 18 | ~18u |
| insphere | 72 | 5 | 9 | 80 | ~80u |

These numbers come from the Dynamics26 path-count derivation.

They are not claimed to be optimal.

## Exact zero rule

If:

```text
|Dhat| <= E
```

the result is **Fallback Required**, not Zero.

Only exact fallback can return Zero.
