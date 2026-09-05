# M1.1 — Independent Exact Predicate Oracle

**Program:** Dynamics26 Original Meshing System R&D  
**Parent work package:** M1 — Robust Geometry Foundation  
**State:** RESEARCHING  
**Research baseline:** 2026-09-05  
**Production code:** none in this work package

## Objective

Create an independent mathematical truth source for:

- orient2d,
- orient3d,
- incircle,
- insphere.

The oracle is test infrastructure. It is deliberately independent of the future C++ robust-predicate implementation.

## Why an independent oracle is required

A robust predicate implementation is easy to test incorrectly by reproducing the same arithmetic assumptions in the test.

Dynamics26 therefore uses two separate concepts:

```text
Production predicate
→ optimized filtered/adaptive C++ implementation

Independent oracle
→ slow exact arithmetic over the exact stored binary64 inputs
```

They may share mathematical definitions, but not arithmetic implementation.

## Research conclusion

The preferred oracle design is a **two-path exact reference**:

1. exact-rational determinant using Python standard-library integers/Fraction,
2. exact dyadic-integer determinant using common power-of-two scaling plus an integer determinant algorithm.

Both paths must return the same sign over generated corpora.

This gives an internal cross-check before either is trusted as a fixture generator.

## Files

- `EXACT_ORACLE_DESIGN.md`
- `DYADIC_INTEGER_MATHEMATICS.md`
- `FILTER_ARCHITECTURE_RESEARCH.md`
- `COMMERCIAL_CAE_CHECK.md`
- `EXPERIMENT_PLAN.md`

## M1.1 exit

M1.1 research is ready for a test-oracle prototype when:

- exact input representation is frozen,
- exact determinant sign conventions are frozen,
- fixture serialization preserves binary64 values exactly,
- commercial tolerance behavior remains explicitly separated from predicate truth,
- oracle A/B cross-check plan is defined.
