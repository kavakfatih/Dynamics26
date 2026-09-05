# M1.2 Certified Filter Experiment Plan

## EXP-MESH-0120 — F0 evaluation-graph audit
Verify that the executable straight-line expression exactly matches the documented polynomial and operation order.

## EXP-MESH-0121 — F0 certification safety
Corpus: ordinary random, near-degenerate, exact zero, large offsets, scale families and CAD-like clusters.

Acceptance:
```text
CertifiedPositive => oracle Positive
CertifiedNegative => oracle Negative
```
False certification count must be exactly zero.

## EXP-MESH-0122 — Fallback profile
Record calls, certified, fallback, exact-zero, range-fallback and near-degeneracy-fallback counts per predicate.

## EXP-MESH-0123 — Range gate
Exercise subnormal, overflow, underflow and mixed-exponent cases. Unsafe arithmetic must fallback, never certify.

## EXP-MESH-0124 — Compiler semantics
Approved first prototype:
```text
-fno-fast-math
-ffp-contract=off
```
Debug and Release must agree with the exact oracle on Apple Silicon.

## EXP-MESH-0125 — Tighter-filter comparison
After F0 passes, compare grouped dynamic bounds, interval filters, semi-static filters and power-of-two normalization. No F1 replaces F0 without a new proof and zero false certifications.

## EXP-MESH-0126 — Commercial robustness corpus
Prepare short-edge, thin-face, sliver-face, tiny-gap, near-coincident and open-shell geometries. When ANSYS / COMSOL / Marc access is available, record repair controls, topology changes and diagnostics. This is a workflow benchmark, not a hidden-predicate comparison.
