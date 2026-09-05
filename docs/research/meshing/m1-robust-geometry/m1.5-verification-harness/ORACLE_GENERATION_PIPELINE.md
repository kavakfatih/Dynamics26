# Exact Oracle Generation Pipeline

## 1. Oracle A — Fraction path

For every finite input coordinate:

```text
float.as_integer_ratio()
→ Fraction
→ exact predicate determinant
→ sign
```

This path follows M1.1.

## 2. Oracle B — Dyadic integer path

For the same binary64 coordinates:

```text
exact dyadic decomposition
→ common power-of-two scale
→ arbitrary-size integers
→ exact integer determinant
→ sign
```

## 3. Corpus acceptance

A case is written only if:

```text
OracleA.sign == OracleB.sign
```

Any disagreement aborts generation.

The production C++ implementation is never consulted during corpus generation.

## 4. Structured generators

Do not rely only on uniform random coordinates.

Generators include:

### G0 — canonical
Simple exact configurations with known orientation.

### G1 — exact degenerate
Construct:
- collinear,
- coplanar,
- cocircular,
- cospherical.

### G2 — representable perturbation
Begin from exact degeneracy and perturb one coordinate by exact powers of two.

### G3 — common large offset
Create small local geometry around a very large representable translation.

Purpose:
- cancellation stress.

### G4 — exponent spread
Coordinates spanning widely separated exponents.

### G5 — signed-zero and duplicate
Exact duplicate sites and +0/-0 combinations.

### G6 — structured random planes/spheres
Random exact dyadic base geometry constrained to near-degenerate manifolds.

### G7 — regression
Hand/promoted failures.

## 5. Seed policy

Every randomized generator receives:
- generator ID,
- integer seed,
- case count.

No hidden system-time seed.

Recommended seed list is committed.

## 6. Oracle self-verification

Before generating production fixtures:

- test Oracle A against known hand determinants,
- test Oracle B against known hand determinants,
- cross-check A/B over large random corpus,
- cross-check determinant permutation identities,
- cross-check power-of-two scale transformations.

## 7. Property generation

Some properties do not need a golden sign file.

Examples:

```text
orient3d(a,b,c,d)
==
-orient3d(b,a,c,d)
```

and stable positive power-of-two scaling.

Property-test inputs can be generated deterministically at runtime and checked using the exact oracle or exact expected transformation.

## 8. Reduction/minimization

Initial failure minimization can be simple and deterministic:

1. try removing common power-of-two scale,
2. translate by one point where exact-representable,
3. zero nonessential coordinates,
4. reduce mantissa bits,
5. preserve the failing relation.

Automatic sophisticated delta-debugging is optional later.

## 9. Generator versioning

Any change to:
- determinant formula,
- generator family,
- normalization,
- fixture schema

increments an explicit generator version.

A fixture should be reproducible from:
- generator commit,
- seed,
- generator ID,
- case index.
