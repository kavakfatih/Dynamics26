# F0 Range and Compiler Policy

## 1. Safe fast domain

F0 uses:

```text
safe proved arithmetic domain
+
exact fallback outside it
```

Fast certification requires:

- finite binary64 coordinates,
- round-to-nearest arithmetic,
- no forbidden fast-math compilation,
- no unhandled overflow/underflow/subnormal intermediates.

## 2. Range fallback

If an arithmetic stage:

- becomes non-finite,
- produces an unproved nonzero subnormal,
- has a suspicious underflow-to-zero case,
- falls outside the derived model,

return:

```text
Fallback Required
```

No sign is certified.

## 3. Exact zero differences

If two input coordinates are exactly equal, their zero difference is accepted.

If a computed difference is zero while inputs are not identical, F0 conservatively falls back.

## 4. Product zero

If a product is zero:
- allowed when an exact factor is zero,
- otherwise fallback.

## 5. Fast determinant zero

If the fast determinant evaluates to zero while `P_hat > 0`:

```text
Fallback
```

F0 never certifies degeneracy.

## 6. Subnormals

The simple M1.2 proof does not include gradual-underflow additive terms.

Therefore F0 does not certify through subnormal intermediate arithmetic.

A later filter may extend the domain with a separate proof.

## 7. Power-of-two normalization

Future F1 work may translate and scale coordinates by exact powers of two to improve exponent range.

This is not needed to qualify the first F0 filter.

## 8. Clang contract

Current Clang documentation states that `-ffast-math` enables aggressive potentially lossy assumptions including reassociation and fast FP contraction.

The first F0 proof counts separate operations.

Proposed initial build contract:

```text
-fno-fast-math
-ffp-contract=off
```

for the predicate target.

A later FMA-aware design may change this with a new proof.

## 9. Platform assertions

Research/CI should verify relevant assumptions such as:

- `std::numeric_limits<double>::is_iec559`,
- radix 2,
- binary64 precision,
- expected rounding mode,
- subnormal support.

## 10. Apple Silicon

Apple Silicon is the product target.

Actual Apple Clang/arm64 Debug and Release builds are required evidence because compiler semantics are part of the numerical contract.
