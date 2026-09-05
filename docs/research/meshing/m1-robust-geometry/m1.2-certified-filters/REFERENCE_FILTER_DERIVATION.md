# F0 Reference Filter Derivation

## 1. Goal

Build the simplest independently auditable certified filter.

F0 is a reference architecture, not the final performance design.

## 2. Predicate polynomial

Assume:

```text
D = Σ(i=1..m) s_i T_i
```

with:

```text
T_i = Π(j=1..d_i) Y_ij
Y_ij = X_a - X_b
d_i <= dmax
```

All input `X` values are finite binary64 values.

## 3. Computed monomial

A degree-`d` product uses:

- `d` rounded differences,
- `d-1` rounded multiplications.

Therefore:

```text
That_i = T_i(1 + θ_(2d-1))
```

and uniformly:

```text
|That_i - T_i|
<= gamma_p |T_i|

p = 2 dmax - 1
```

## 4. Signed accumulation

Sequentially summing `m` monomials adds at most `m-1` more roundings to the longest term path.

Using the standard theta-composition bound:

```text
K = p + m - 1
```

and:

```text
|Dhat - D|
<= gamma_K Σ|T_i|
```

This is a worst-case absolute forward-error bound.

## 5. Computable magnitude bound

Define:

```text
Pcomp_exact = Σ |That_i|
```

Then:

```text
Σ|T_i|
<= Pcomp_exact / (1 - gamma_p)
```

The machine-computed positive sum:

```text
Phat = fl(Pcomp_exact)
```

satisfies conservatively:

```text
Pcomp_exact
<= Phat / (1 - gamma_(m-1))
```

Therefore:

```text
|Dhat - D|
<=
gamma_K
-------------------------------- * Phat
(1-gamma_p)(1-gamma_(m-1))
```

## 6. Why this is intentionally pessimistic

The derivation:

- charges every monomial the maximum degree,
- charges every term the maximum summation path,
- ignores exact-subtraction opportunities,
- does not exploit FMA,
- does not exploit determinant grouping.

This produces more fallback but makes the first proof understandable.

## 7. Why pessimism is acceptable

For a filter:

```text
false fallback = performance cost
false certification = topology corruption risk
```

The design optimizes correctness first.

## 8. F1 later

After F0 is executable and oracle-verified, later research may derive tighter filters using:

- grouped determinant structure,
- per-term path counts,
- exact subtraction cases,
- power-of-two normalization,
- optional FMA-aware proofs,
- interval/semi-static prefilters.

Each change requires a separate proof and experiment.

## 9. No external optimized constants

Shewchuk and other robust-predicate sources publish efficient specialized bounds.

They are scientific references demonstrating feasibility.

Dynamics26 F0 does not copy their predicate-specific constants, source macros or operation schedules.
