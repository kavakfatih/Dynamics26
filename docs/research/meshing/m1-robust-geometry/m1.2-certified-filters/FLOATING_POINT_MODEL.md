# Floating-Point Model for M1.2

## 1. Standard model

For a basic floating-point operation under round-to-nearest and away from overflow/underflow:

```text
fl(x op y) = (x op y)(1 + δ)
|δ| <= u
```

where `u` is unit roundoff.

For IEEE binary64:

```text
u = 2^-53
  ≈ 1.1102230246251565e-16
```

Higham's standard analysis uses:

```text
gamma_n = n u / (1 - n u)
```

to bound accumulated products of rounding factors.

Primary references:
- Higham, *Accuracy and Stability of Numerical Algorithms*.
- Shewchuk, robust predicate papers.
- Devillers/Preparata, arithmetic filters.

## 2. Composition lemma used by Dynamics26

For rounding factors:

```text
Π(1 + δ_i) = 1 + θ_n
|θ_n| <= gamma_n
```

provided `nu < 1`.

This allows an operation path to be conservatively counted without using another project's predicate-specific constants.

## 3. Why the filter needs an explicit evaluation graph

Two algebraically identical determinant formulas may have different floating-point error.

Therefore the proof attaches to:

```text
mathematical predicate
+
specific straight-line evaluation order
+
compiler FP contract
```

not merely to the symbolic determinant.

M1.2 F0 freezes one simple expanded-monomial evaluation for each predicate.

Optimized regrouping is a later filter version and requires a new error proof.

## 4. Relative model domain

The simple relative model is not used to certify:

- overflow,
- NaN / infinity,
- uncertain underflow,
- subnormal intermediate arithmetic unless separately proved,
- unknown/dynamic rounding mode,
- compiler-reassociated evaluation outside the frozen graph.

Those cases fallback.

This is deliberately conservative.

## 5. Sum-of-monomials model

Write one predicate as:

```text
d = Σ_i s_i t_i
s_i ∈ {-1,+1}
t_i = Π_j y_ij
y_ij = coordinate difference
```

Suppose every monomial has degree at most `dmax`.

Each coordinate difference contributes one rounded operation.

A degree-`d` monomial computed sequentially has:

```text
d subtraction roundings
+
(d - 1) multiplication roundings
=
2d - 1
```

rounding stages on its longest dependency path.

Define:

```text
p = 2 dmax - 1
```

Then:

```text
t_hat_i = t_i (1 + θ_p)
|θ_p| <= gamma_p
```

under the F0 range assumptions.

## 6. Sequential summation

For `m` signed monomials summed sequentially, a worst-case term can accumulate at most `m-1` additional addition/subtraction roundings.

A conservative combined path count is:

```text
K = p + (m - 1)
  = 2 dmax + m - 2
```

which gives:

```text
|d_hat - d|
<= gamma_K Σ |t_i|
```

This is intentionally not tight.

## 7. Bounding exact monomial magnitudes by computed magnitudes

The filter does not know `Σ|t_i|` exactly.

From:

```text
|t_hat_i - t_i| <= gamma_p |t_i|
```

we have:

```text
|t_i| <= |t_hat_i| / (1 - gamma_p)
```

for `gamma_p < 1`.

Let:

```text
P = Σ |t_hat_i|
```

If `P` itself is accumulated in floating point:

```text
P_hat = fl(Σ |t_hat_i|)
```

then conservatively:

```text
P <= P_hat / (1 - gamma_(m-1))
```

for the accepted normal-range path.

Therefore the first reference bound is:

```text
E_F0 =
 gamma_K
 -------------------------------  * P_hat
 (1 - gamma_p)(1 - gamma_(m-1))
```

This formula is the core M1.2 independent result.

## 8. Certification

```text
if d_hat > +E_F0:
    Certified Positive

else if d_hat < -E_F0:
    Certified Negative

else:
    Fallback
```

The filter does not certify Zero.

## 9. Validation requirement

Even after mathematical derivation, the executable filter is not trusted until:

```text
every Certified result
==
M1.1 exact oracle sign
```

over deterministic, adversarial and randomized corpora.
