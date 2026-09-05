# Filter Architecture Research

## 1. Goal

The oracle is slow by design.

Production predicates require:

```text
correct sign
+
ordinary-case speed
```

The leading architecture is therefore filtering.

## 2. Literature consensus

Shewchuk explains the core problem:

- orientation/incircle-class predicates are determinant-sign tests,
- floating-point roundoff can produce a wrong sign near zero,
- exact arbitrary-precision arithmetic everywhere can be expensive,
- adaptive computation can do only as much work as needed to certify the sign.

Devillers and Preparata likewise describe arithmetic filters:

```text
rounded evaluation
→ error estimate
→ certified result or exact fallback
```

CGAL publicly implements filtered-kernel concepts where interval/static filtering is used before an exact predicate path.

These are architecture/theory references, not code templates.

## 3. Candidate production tiers

### Strategy F1 — dynamic analytic filter

For each predicate:

1. evaluate a carefully ordered floating expression,
2. compute a conservative error bound from actual input-term magnitudes,
3. certify sign if `|d_fast| > E`,
4. otherwise fallback.

Pros:
- fast ordinary path,
- no global coordinate bound required,
- direct control over performance.

Cons:
- error analysis is predicate/expression-specific,
- compiler arithmetic assumptions must match derivation.

### Strategy F2 — interval filter

Evaluate the predicate over outward-rounded intervals.

If the resulting interval excludes zero:

- sign is certified.

If it contains zero:

- fallback.

Pros:
- generic conceptual model,
- error enclosure explicit.

Cons:
- directed rounding/environment management,
- potential overhead,
- platform/compiler complexity.

### Strategy F3 — semi-static filter

Precompute bounds from coordinate limits and use very cheap sign checks.

Pros:
- extremely cheap successful path.

Cons:
- depends on input bound assumptions,
- more specialized proof,
- should be an optimization after correctness.

## 4. Recommended development order

Do not jump directly to the most optimized architecture.

### P0 — exact slow prototype

```text
fast determinant
→ if obviously far from zero, optional provisional path
→ exact dyadic integer fallback for every verification call
```

Used only to establish corpus and sign conventions.

### P1 — dynamic filter prototype

Derive an original conservative error bound and validate:

```text
if filter says certified
→ exact oracle must agree
```

A single false certification rejects the filter.

### P2 — adaptive production fallback

Develop a faster exact-sign fallback, likely based on independently implemented expansion-style arithmetic or another exact dyadic method.

### P3 — optional static/interval acceleration

Only after profile data proves value.

## 5. Compiler contract

Clang documents that `-ffast-math` permits:

- reassociation,
- reciprocal transformations,
- finite-only assumptions,
- relaxed signed-zero behavior,
- fast FP contraction.

This can invalidate a hand-derived floating error bound.

Therefore the M1 predicate target remains:

```text
no fast-math
explicit FP contraction mode
known rounding assumptions
compiler-matrix tests
```

The exact flags will be frozen only when F1 arithmetic is derived.

## 6. FMA

Fused multiply-add is not intrinsically unsafe.

It can reduce one rounding event.

But:

> A proof for separate multiply and add cannot simply be reused after contraction.

Dynamics26 must either:

- derive the filter assuming FMA at specific sites, or
- disable contraction for the certified expression.

This decision belongs to the filter specification.

## 7. Static coordinate normalization

Translation and positive power-of-two scaling can improve numerical range.

Power-of-two scaling is especially attractive because it is exact in binary arithmetic when it remains representable.

Still:

- normalization is an optimization,
- it does not replace exact fallback,
- the exact oracle checks the transformed path.

## 8. Telemetry

Research builds should count:

- total calls,
- F1 certified,
- fallback,
- exact zero,
- invalid non-finite input.

For each geometry corpus, record fallback ratio.

This tells us whether production complexity is justified by actual CAD workloads.

## 9. Acceptance rule

The filter contract is one-sided:

> It may fallback unnecessarily, but it may never certify the wrong sign.

A conservative slow filter is correct.

An aggressive filter with one wrong sign is unusable.
