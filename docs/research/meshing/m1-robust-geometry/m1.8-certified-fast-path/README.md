# M1.8 — Certified Fast Predicate Path

**State:** QUALIFIED  
**Exact-head evidence:** `f1e3ab433d94...`, macOS arm64 workflow #234 SUCCESS

## Implemented architecture

```text
safe finite/normal fast domain
→ homogeneous determinant/lift evaluation
→ conservative absolute-term envelope
→ certified sign when outside error envelope
→ exact M1.7 fallback otherwise
```

The executable F0 uses a deliberately conservative exactly representable coefficient:

```text
2^-43 = 1024 * u
```

against the computed absolute determinant-term sum.

The coefficient is chosen above the documented worst-case gamma-model accumulation for the supported matrices up to the 5x5 lifted insphere determinant.

## Safety rules

Fast path falls back on:
- non-finite input,
- non-normal nonzero input/intermediate,
- overflow,
- suspicious underflow-to-zero,
- zero determinant,
- determinant inside the error envelope.

Fast path never certifies `Zero`.

Compiler contract on the source:
- no fast-math,
- FP contraction off.

## Verification

The same executable is tested against:
- committed exact fixtures,
- deterministic Python-generated exact-oracle corpus,
- Debug and Release on target macOS arm64.

M1.8 qualification means the filter is accepted as an M1 implementation component; it does not qualify M1 as a whole.


## M1.9 proof hardening

The implemented homogeneous determinant graph now has a dedicated proof:
- `HOMOGENEOUS_BOUND_PROOF.md`.

The source also contains a compile-time inequality proving that the chosen `2^-43 = 1024u` coefficient dominates the documented worst supported 5x5 lifted determinant bound after denominator and final-rounding corrections.

Workflow #239 Release baseline:
```text
cases=1049
calls=1051
fast=1043
exact=6
zero=4
invalid=2
mismatch=0
```
