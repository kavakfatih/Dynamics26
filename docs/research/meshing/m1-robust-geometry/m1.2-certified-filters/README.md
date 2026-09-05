# M1.2 — Certified Floating-Point Filter Mathematics

**Program:** Dynamics26 Original Meshing System R&D  
**Parent:** M1 — Robust Geometry Foundation  
**Depends on:** M1.1 exact-oracle design  
**State:** RESEARCHING  
**Baseline:** 2026-09-05  
**Production code:** none in this work package

## Objective

Derive an original conservative fast-filter architecture for geometric predicates without copying optimized constants or arithmetic schedules from external meshing libraries.

The filter answers only:

```text
Can the sign of this fast binary64 determinant be certified?
```

Possible outcomes:

```text
Certified Negative
Certified Positive
Fallback Required
```

An exact zero is never guessed by the fast filter. Zero is established by the exact fallback.

## Key research decision

The first Dynamics26 filter will be a deliberately conservative **F0 reference filter** based on standard floating-point error analysis:

```text
fl(x op y) = (x op y)(1 + δ), |δ| <= u
gamma_n = n u / (1 - n u)
```

For a frozen straight-line determinant expansion, derive:

```text
|d_fast - d_exact| <= E
```

If:

```text
d_fast >  E → Positive
d_fast < -E → Negative
otherwise    → Fallback
```

This filter may fallback more often than optimized literature implementations. It may never falsely certify a sign.

## Documents

- `FLOATING_POINT_MODEL.md`
- `REFERENCE_FILTER_DERIVATION.md`
- `PREDICATE_FILTER_BOUNDS.md`
- `RANGE_AND_COMPILER_POLICY.md`
- `COMMERCIAL_CAE_CHECK.md`
- `EXPERIMENT_PLAN.md`

## Clean-room note

Published robust-predicate implementations contain highly optimized error constants, evaluation schedules and expansion code.

M1.2 does not transcribe those constants or schedules.

Dynamics26 derives its own initial conservative bounds from the general floating-point model and validates every certification against the independent M1.1 exact oracle.
