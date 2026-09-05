# Exact Oracle Design

## 1. What truth does the oracle represent?

The oracle answers:

> What is the exact mathematical sign of the determinant for the **binary64 coordinate values actually supplied to the meshing algorithm**?

It does not answer:

> What was the ideal design intent before STEP/CAD conversion, floating-point storage or geometric tolerance?

That distinction is essential.

For example, a decimal-looking coordinate such as `0.1` is stored as one exact binary rational. The oracle evaluates that exact stored value.

Python documents that `float.as_integer_ratio()` returns two integers whose ratio is exactly equal to the original float.

Source: M1.1-DOC-001.

## 2. Oracle A — exact rational reference

For each finite coordinate:

```text
x.as_integer_ratio()
→ (numerator, denominator)
→ exact rational x
```

Python's `fractions.Fraction` can represent that ratio exactly.

Then construct the determinant matrix using exact rational operations.

### Advantages

- conceptually direct,
- Python standard library only,
- independent of Dynamics26 C++,
- handles subnormal values,
- easy to inspect during research.

### Disadvantages

- fraction normalization can create overhead,
- not intended for millions of production predicate calls.

That is acceptable because Oracle A is a **test truth source**, not a product algorithm.

## 3. Oracle B — exact dyadic integer reference

Every finite binary floating-point number is a dyadic rational:

```text
x = n / 2^q
```

for integers `n` and nonnegative `q` after normalization.

For one predicate call, choose:

```text
Q = max(q_i)
S = 2^Q
```

and transform every coordinate:

```text
X_i = S * x_i
```

Every `X_i` is then an exact integer.

Because `S > 0`, uniform coordinate scaling preserves predicate sign.

The lifted determinants scale by positive powers:

```text
orient2d  : S^2
orient3d  : S^3
incircle  : S^4
insphere  : S^5
```

Therefore sign is unchanged.

This allows the exact oracle to use only arbitrary-size integers after one common dyadic scaling.

## 4. Integer determinant strategy

The matrices are tiny:

- orient2d: 2×2 translated form,
- orient3d: 3×3 translated form,
- incircle: 4×4 lifted form,
- insphere: 5×5 lifted form.

Two independent integer strategies are useful:

### B1 — direct permutation determinant

For at most 5×5:

```text
det(A) = Σ sign(σ) Π A[i,σ(i)]
```

5! = 120 terms.

This is inefficient in general linear algebra but perfectly reasonable for a slow oracle and very easy to audit.

### B2 — fraction-free Bareiss elimination

Bareiss' integer-preserving elimination computes exact determinants while controlling intermediate coefficient growth better than naive rational Gaussian elimination.

Primary source:
- Erwin H. Bareiss, *Sylvester's Identity and Multistep Integer-Preserving Gaussian Elimination*, Mathematics of Computation, 1968.

For test infrastructure, B1 can be the simplest first truth implementation and B2 can independently cross-check it.

## 5. Why two exact oracle paths?

We do not want:

```text
Production implementation bug
+
test oracle bug caused by same algorithm
→ false green
```

Instead:

```text
Fraction oracle
    ↘
     expected sign
    ↗
Dyadic integer oracle
```

A fixture is accepted only if the exact reference paths agree.

Production C++ is then tested against the accepted fixtures.

## 6. Fixture representation

Do not serialize test coordinates as ordinary decimal text if exact round-trip is uncertain.

Preferred representations:

1. hexadecimal floating-point text,
2. raw binary64 bit pattern,
3. both, for readability and auditing.

Python documents `float.hex()` as an exact representation of the stored float and states that the hexadecimal form can reconstruct the value exactly.

Suggested fixture concept:

```json
{
  "predicate": "orient3d",
  "points_hex": [
    ["0x1.0p+0", "0x0.0p+0", "0x0.0p+0"],
    ...
  ],
  "expected": "Positive",
  "class": "near-coplanar",
  "generator": "M1.1",
  "seed": 1234
}
```

The exact file format will be frozen before code is committed.

## 7. Sign conventions

Fixtures must not only contain magnitude-independent truth; they must freeze Dynamics26 conventions.

For example:

- canonical positive `orient3d` fixture,
- one swap → Negative,
- coplanar → Zero.

For incircle/insphere, expected inside/outside sign depends on defining simplex orientation. The fixture metadata must therefore record the simplex orientation or use canonical positive-oriented inputs.

## 8. True zero

Exact arithmetic must be able to produce:

```text
determinant == 0
→ PredicateSign::Zero
```

No epsilon.

No hidden perturbation.

No "small enough" classification.

Symbolic tie-breaking remains an M2 Delaunay policy.

## 9. Non-finite values

NaN and infinity are not oracle predicate inputs.

The fixture generator rejects them explicitly.

Python's `as_integer_ratio()` itself raises on non-finite values, which aligns with the M1 contract.

## 10. Oracle authority

The exact oracle is authoritative for **predicate sign correctness**.

It is not authoritative for:

- CAD repair tolerance,
- geometry healing,
- whether two vertices should be merged,
- mesh-size decisions,
- engineering model intent.

Those remain separate policies.
