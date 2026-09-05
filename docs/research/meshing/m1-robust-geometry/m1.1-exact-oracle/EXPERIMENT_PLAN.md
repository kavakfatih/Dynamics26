# M1.1 Experiment Plan

## EXP-MESH-0101 — Exact oracle A/B agreement

### Question

Can two independently structured exact arithmetic paths produce identical signs for every generated predicate fixture?

### Oracle A

Python:
- `float.as_integer_ratio()`,
- `fractions.Fraction`,
- exact determinant.

### Oracle B

Python:
- exact dyadic common scaling,
- arbitrary-size integers,
- direct permutation determinant.

Later add Bareiss as a third path if useful.

### Corpus

- 10,000 ordinary random cases per predicate,
- 10,000 near-degenerate structured cases,
- exact zero fixtures,
- coordinate-scale families,
- common-large-offset families,
- subnormal/min-normal boundary fixtures.

### Acceptance

```text
A sign == B sign
for every fixture
```

No majority vote.

Any mismatch is a research blocker.

---

## EXP-MESH-0102 — Binary64 round-trip fixture format

### Question

Does the chosen fixture representation reproduce exact coordinate bits across Python and C++?

Candidates:
- hexadecimal float text,
- uint64 raw bits.

### Acceptance

- Python generated bits equal C++ parsed bits,
- +0 / -0 behavior recorded explicitly,
- normal/subnormal cases round-trip,
- no decimal formatting dependency.

---

## EXP-MESH-0103 — Naive double failure corpus

### Purpose

Demonstrate and preserve cases where ordinary determinant evaluation gives an uncertain or wrong sign.

This corpus justifies the robust kernel.

### Method

Generate near-degenerate exact-rational configurations and compare:

```text
naive binary64 determinant sign
vs
exact oracle sign
```

Record:
- wrong-sign cases,
- false-zero cases,
- determinant magnitude,
- coordinate scale.

### Acceptance

Not a PASS/FAIL of production code.

Output is a permanent adversarial corpus.

---

## EXP-MESH-0104 — Filter certification prototype

### Question

Can an original dynamic error bound certify ordinary predicate signs without false certification?

### Acceptance

For every case where filter returns `Certified`:

```text
filter sign == exact oracle sign
```

Fallback rate is measured but has no initial maximum.

---

## EXP-MESH-0105 — Apple Silicon floating-mode sensitivity

### Build matrix

- Debug precise,
- Release precise,
- Release with selected contraction policy,
- intentionally unsafe fast-math negative-control build if isolated test setup permits.

### Purpose

Show that the production build contract matters and that CI can detect forbidden floating modes.

### Acceptance

Approved builds:
- zero oracle mismatches.

Negative-control:
- used only as evidence/test of guard behavior, never production.

---

## EXP-MESH-0106 — Commercial workflow benchmark

This experiment does not compare hidden predicate results.

Use simple geometries containing:
- short edges,
- thin faces,
- near-coincident features.

Record in ANSYS / COMSOL / Marc when available:

- repair/defeature controls used,
- topology changes,
- meshing success/failure,
- user-visible diagnostics,
- preservation of scoped topology.

Purpose:

Define future Dynamics26 geometry-repair UX without inferring proprietary algorithms.

---

## Implementation sequence after research

```text
EXP-0101 oracle prototype
→ EXP-0102 fixture round-trip
→ EXP-0103 adversarial corpus
→ M1.2 filter derivation
→ EXP-0104 filter validation
→ EXP-0105 compiler gate
→ ADR-MESH-0005 decision
```
