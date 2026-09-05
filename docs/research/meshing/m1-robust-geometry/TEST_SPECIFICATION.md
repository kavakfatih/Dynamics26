# M1 Robust Predicate Test Specification

## 1. Test philosophy

M1 tests must verify **mathematical sign correctness**, not merely reproduce one implementation's floating output.

The main test classes are:

1. exact deterministic fixtures,
2. near-degenerate adversarial cases,
3. metamorphic properties,
4. exact independent oracle comparison,
5. compiler/build-mode matrix,
6. performance/fallback telemetry.

## 2. Exact deterministic fixtures

### orient2d
- clear positive,
- clear negative,
- collinear,
- repeated point.

### orient3d
- canonical positive tetra,
- swapped-vertex negative tetra,
- coplanar four points,
- repeated point.

### incircle
- clearly inside,
- clearly outside,
- exactly cocircular,
- orientation reversal.

### insphere
- clearly inside,
- clearly outside,
- exactly cospherical,
- tetra orientation reversal.

## 3. Near-degenerate families

Generate parameterized cases:

```text
exact degenerate configuration
+ perturbation δ
```

with `δ` swept across powers of two around the binary64 resolution relevant to the coordinate scale.

Families:

- nearly collinear,
- nearly coplanar,
- nearly cocircular,
- nearly cospherical.

Acceptance:

> Sign must match the independent exact oracle for every finite representable test input.

## 4. Scale families

Repeat fixtures at coordinate scales such as:

```text
2^-400
2^-200
2^-50
1
2^50
2^200
2^400
```

subject to finite intermediate/reference representation.

This tests overflow/underflow resistance and filter behavior.

## 5. Translation invariance

For a common translation vector `t`:

```text
orient3d(a+t,b+t,c+t,d+t)
```

must have the same sign as the original representable input relation, with exact oracle deciding any cases where rounded translated coordinates genuinely change the input.

Important nuance:

Translation in real arithmetic is invariant. Translation followed by rounding can produce a different binary64 point set. The oracle compares the actual stored points, not the intended real values.

## 6. Permutation properties

Examples:

```text
orient3d(a,b,c,d)
= -orient3d(b,a,c,d)
```

for nonzero cases.

Even permutations preserve sign; odd permutations reverse sign.

Similar orientation-dependent consistency tests apply to incircle/insphere.

## 7. Positive scaling property

For positive exactly representable scale factors that do not overflow/underflow coordinates, nonzero sign should be preserved.

Again, the oracle evaluates actual scaled binary64 values.

## 8. Independent exact oracle

Recommended first oracle:

### Python exact-rational generator

Python `float.as_integer_ratio()` exposes each binary64 input exactly as an integer ratio.

A test-generation script can:

1. convert every coordinate to an exact rational,
2. evaluate determinant with Python arbitrary-size integers/rationals,
3. emit compact fixture input + expected sign.

Advantages:

- independent of the production C++ arithmetic implementation,
- no meshing library dependency,
- exact truth for the stored floating inputs,
- easy generation of adversarial cases.

The production library does not call Python.

A later C++ test-only multiprecision oracle may be evaluated separately.

## 9. Random/property testing

Generate structured random families rather than only uniform random points.

Bias toward:

- near planes,
- near spheres,
- clustered coordinates,
- mixed magnitude coordinates,
- shared leading bits,
- large common offsets plus tiny separations.

Record seed and failing fixture in a replayable format.

Any discovered failure becomes a permanent deterministic regression case.

## 10. Non-finite tests

NaN/Inf input must produce explicit invalid-input handling, never a normal predicate sign.

## 11. Compiler matrix

At minimum on target macOS/Apple Silicon:

- Debug,
- Release,
- current Apple Clang,
- predicate target optimization enabled but fast-math disabled.

Add a compile-time/CI guard ensuring predicate target is not built with fast-math semantics.

If exactness depends on FP contraction being disabled, include an explicit build assertion/configuration test.

## 12. Determinism

Run an adversarial corpus repeatedly.

Acceptance:

- identical input → identical sign sequence,
- exact-zero count stable,
- fallback count stable for fixed build/toolchain,
- no random tie-break inside predicate layer.

## 13. Performance

Correctness is primary, but collect:

- total predicate calls,
- percentage fast-certified,
- percentage fallback,
- exact-zero count,
- wall time per million ordinary calls,
- wall time on adversarial corpus.

No performance target may justify wrong sign.

## 14. M1 release gate

M1 predicate implementation cannot be used by M2 Delaunay until:

- zero known sign mismatches against exact oracle across the committed corpus,
- randomized/adversarial campaign passes,
- Debug and Release agree,
- fast-math guard passes,
- source-boundary review confirms original implementation.
