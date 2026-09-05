# Numerical Robustness Policy Research

## 1. Failure mechanisms

### Cancellation

Two large nearly equal terms are subtracted and most significant digits cancel.

Near-coplanar and near-cospherical predicates are classic examples.

### Overflow / underflow

Large coordinate magnitude or extreme scale separation can make intermediate products leave a safe floating range even when the final determinant is representable.

### Reassociation

A compiler may legally rearrange expressions under aggressive floating-point optimization, changing an error bound assumed by the predicate implementation.

### Fused multiply-add contraction

FMA can improve accuracy, but if a certified error analysis assumes separate rounding of multiplication and addition, contraction changes that model.

The issue is not "FMA is bad"; the issue is that implementation and proof/error bound must agree on the arithmetic actually executed.

### Non-finite input

NaN or infinity cannot participate in a meaningful topology predicate. Reject before predicate execution.

## 2. Compiler policy

Clang documents that `-ffast-math` permits aggressive, potentially lossy assumptions such as associativity and also enables broad floating-point transformations. GCC gives a similar warning that fast-math can produce incorrect output for programs depending on IEEE/ISO behavior.

**Proposed Dynamics26 M1 policy:**

- predicate translation unit/library compiled without fast-math,
- do not use `-Ofast` for predicates,
- make FP contraction policy explicit,
- reject `__FAST_MATH__` at build/test time for the predicate target,
- keep this restriction local to the robust-geometry target rather than unnecessarily forcing the entire solver to one FP mode.

If filter error bounds are derived assuming no contraction, use a no-contraction compile policy for that implementation.

This becomes a verification gate, not a developer convention.

## 3. Apple Silicon

The historical x87 excess-precision issue noted in older robust-predicate literature is not the same hardware model as Apple Silicon.

Nevertheless, macOS/arm64 still requires explicit testing because:

- compiler transformations matter,
- FMA/contraction behavior matters,
- optimization levels matter,
- vectorization/reassociation policies matter.

Therefore correctness is tested on the actual target compiler/toolchain, not inferred from architecture marketing.

## 4. Geometry tolerance is separate

Dynamics26 uses OCCT as CAD authority.

OCCT stores tolerances on sub-shapes and exposes tools to inspect/limit/set vertex, edge and face tolerances. Its documentation also reflects relationships between B-Rep sub-shape tolerances.

Those tolerances answer CAD consistency questions.

Examples:

- do a vertex and edge curve endpoint agree within modeling tolerance?
- should a healing operation enlarge a vertex tolerance?
- is a pcurve/3D curve mismatch acceptable?

They do **not** answer:

- what is the exact sign of orient3d for the point coordinates given to Delaunay?
- is a fifth point exactly inside or outside a tetra circumsphere?

## 5. Commercial-product comparison

### COMSOL

COMSOL documents relative, absolute and automatic repair tolerances. Geometric entities closer than repair tolerance may be merged by geometry operations.

This is explicit model-geometry policy.

### ANSYS

ANSYS documents defeature size and topology-protection controls. Small features may be removed under meshing policy, while selected topology can be protected.

This is explicit mesh/geometry simplification policy.

### Marc/Mentat

Public release documentation exposes automatic geometry/display tolerance and sweep-node tolerance behaviors, but not an internal exact-predicate contract.

**Conclusion:** commercial CAE products visibly separate tolerance-controlled geometry simplification from the user-facing mesh algorithm. Internal predicate implementation remains undocumented.

## 6. Proposed GeometryTolerancePolicy

This is application/geometry policy, not robust predicate math.

Candidate inputs:

```text
CAD-provided subshape tolerance
model characteristic length
user repair/defeature setting
minimum permitted absolute tolerance
maximum permitted repair tolerance
```

Candidate decisions:

- duplicate/coincident preprocessing,
- geometry validity diagnostics,
- edge sampling endpoint reconciliation,
- optional explicit repair operations.

A future user-facing "Repair Tolerance" must be an explicit operation/settings object and must never silently change predicate signs.

## 7. Predicate fallback telemetry

M1 proposes optional debug/research counters:

```text
predicateCalls
fastCertified
fallbackRequired
exactZero
nonFiniteRejected
maxExpansionLength / equivalent effort metric
```

These are R&D/performance diagnostics, not solver engineering state.

They allow us to answer:

- how often difficult geometry triggers exact work?
- which benchmark models dominate predicate cost?
- do compiler/build changes alter fallback rates unexpectedly?

## 8. Determinism

For identical input coordinates, stable point IDs and algorithm version:

- predicate sign must be deterministic,
- exact zero must be deterministic,
- tie-break policy must be deterministic,
- no hidden random epsilon perturbation.

If randomized insertion is later used for Delaunay performance, the seed/order is a separate recorded algorithm parameter.

## 9. Coordinate normalization

Local translation/scaling can improve ordinary floating-point conditioning and reduce overflow risk.

It is allowed as an optimization if:

- the transformation is mathematically sign-preserving,
- transformation metadata is deterministic,
- it does not replace certified fallback,
- tests prove invariance.

## 10. Proposed hard failures

Reject predicate input when:

- coordinate is NaN,
- coordinate is infinity,
- input point record is invalid.

True degeneracy is **not** a predicate failure. It is a valid `Zero` result.
