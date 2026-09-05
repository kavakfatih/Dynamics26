# M6 Research — D26QMRF1 Certified Filtered Mean-Ratio Comparator

Status: RESEARCHING / numerical-filter contract candidate
Date: 2026-09-06

## 1. Engineering question

D26QMR1 provides an exact authoritative comparison for tetrahedral mean ratio.

How should Dynamics26 accelerate the common case without allowing:
- an approximate sign error,
- a floating epsilon tie,
- overflow/underflow corruption,
- FMA/compiler changes to alter topology,
- interval overlap to masquerade as equality?

The leading research answer is:

    dynamic certified interval filter
      ->
    exact D26QMR1 fallback.

A semi-static filter is a later optimization layer, not the first qualification target.

Primary references:
- M6-NUM-001 — Shewchuk adaptive exact predicate philosophy,
- M6-NUM-004 — Bronnimann/Burnikel/Pion dynamic interval filters,
- M6-NUM-005 — Melquiond/Pion formally certified semi-static filters,
- M6-NUM-006 — Higham floating-point gamma_n analysis,
- M6-NUM-003 — CGAL interval/semi-static/exact cascade architecture.

## 2. Why a filter is allowed

The exact decision is:

    sign(
      F(A,B)
    )

where:

    F(A,B)
      =
    D_A^2 S_B^3
      -
    D_B^2 S_A^3.

The filter is not asked to approximate q_MR for reporting.

It is asked only:

    can the sign of F be certified cheaply?

If yes:
- return Less or Greater.

If no:
- return Uncertain,
- call exact D26QMR1.

This is the exact-predicate paradigm applied to quality ordering.

## 3. Filter classes

Research distinguishes:

### Static filter

A pre-derived global error bound depending mainly on known input-range bounds.

Pros:
- extremely cheap.

Cons:
- often conservative,
- sensitive to expression/range assumptions.

### Semi-static filter

A fixed arithmetic error formula combined with cheap runtime magnitude data.

Pros:
- faster than general intervals in many cases.

Cons:
- derivation is expression-tree and floating-environment specific,
- easy to get subtly wrong.

### Dynamic interval filter

Propagate intervals through the actual expression at runtime.

Pros:
- naturally captures input-dependent cancellation and range,
- straightforward exact-fallback semantics,
- strong first qualification candidate.

Cons:
- higher runtime overhead than a good semi-static filter.

The first Dynamics26 fast-filter qualification should target the dynamic interval form.

## 4. Exact comparison polynomial

For already-valid positive tetrahedra:

    q_MR(A) > q_MR(B)

iff:

    D_A^2/S_A^3
      >
    D_B^2/S_B^3.

Because:

    S_A > 0
    S_B > 0,

cross multiplication gives:

    F(A,B)
      =
    D_A^2 S_B^3
      -
    D_B^2 S_A^3.

Then:

    sign(F)
      =
    sign(q_MR(A)-q_MR(B)).

The fast filter should target F directly.

## 5. Why cross-polynomial filtering is preferable to ratio filtering

A ratio filter for:

    R=D^2/S^3

needs:
- cubing,
- denominator interval positivity,
- interval division.

The cross polynomial needs only:
- addition/subtraction,
- multiplication,
- squaring/cubing by multiplication.

It also avoids:
- division rounding,
- denominator reciprocal widening,
- root operations.

Therefore the leading D26QMRF1 expression is F.

## 6. Bi-homogeneity theorem

Scale tetra A by positive alpha and tetra B by positive beta.

Then:

    D_A -> alpha^3 D_A
    S_A -> alpha^2 S_A

and:

    D_B -> beta^3 D_B
    S_B -> beta^2 S_B.

Therefore:

    D_A^2 S_B^3
      ->
    alpha^6 beta^6
    D_A^2 S_B^3

and:

    D_B^2 S_A^3
      ->
    beta^6 alpha^6
    D_B^2 S_A^3.

Thus:

    F
      ->
    alpha^6 beta^6 F.

For positive alpha,beta:

    sign(F)
      unchanged.

### Consequence

Tetra A and B may be normalized independently.

This is stronger than ordinary common-scale invariance and is particularly useful for avoiding
overflow/underflow in the filter.

## 7. Canonical filter enumeration

The exact D26QMR1 value is invariant to tetra vertex permutation.

A floating interval expression can have different widths under different operation orders.

If filter telemetry is part of deterministic replay, use a canonical local vertex enumeration before:
- selecting the determinant anchor,
- evaluating interval operations.

Candidate:
- canonical PointId order,
- then normalize orientation according to the existing positive-tetra contract.

Final comparison correctness never depends on this choice because Uncertain falls back to exact.

Canonical enumeration is for reproducible filter behavior, not mathematical q_MR identity.

## 8. Dynamic interval input model

Each stored binary64 coordinate enters as the point interval:

    [x,x].

Interval operations must contain the exact real result represented by the stored binary64 inputs.

Required interval primitives:
- subtraction,
- multiplication,
- addition,
- square,
- positive integer power,
- absolute-value envelope where needed,
- exact power-of-two scaling when certified safe.

No approximate coordinate reconstruction is allowed outside the interval enclosure.

## 9. Local relative-coordinate intervals

For one canonical tetra:

    R_1 = X_1-X_0
    R_2 = X_2-X_0
    R_3 = X_3-X_0.

Compute these as interval subtractions.

Let:

    M
      =
    max upper(|R_i_component|).

If:
- M is non-finite,
- M cannot be certified positive,

the filter returns Uncertain.

Exact validity remains external and authoritative.

## 10. Power-of-two normalization

Choose an exact positive power of two:

    lambda = 2^k

from the certified upper magnitude M such that the normalized relative-component magnitude is bounded
near one.

Research target:

    1/2 < lambda M <= 1

when the floating range permits it.

Then:

    R'_i = lambda R_i.

Because lambda is a power of two:
- mathematical scaling is exact,
- q_MR order is unchanged,
- F sign remains unchanged after independent normalization of A and B.

If the interval backend cannot certify safe scaling without overflow/underflow:
- do not guess,
- return Uncertain.

## 11. Mathematical dynamic-range bound after normalization

Assume exact normalized anchor-relative components satisfy:

    |R'_i_component| < 1.

Then each column of the 3x3 determinant matrix has Euclidean norm less than:

    sqrt(3).

By Hadamard:

    |D'|
      <
    (sqrt(3))^3
      =
    3 sqrt(3).

Therefore:

    D'^2 < 27.

Any pairwise edge component is the difference of two anchor-relative components, so:

    |edge_component| < 2.

There are:
- six edges,
- three scalar components per edge.

Hence:

    S'
      <
    18 * 2^2
      =
    72.

Then each exact cross term obeys:

    D'^2 S'^3
      <
    27 * 72^3
      =
    10,077,696.

And:

    |F'|
      <
    20,155,392.

This is many orders of magnitude below binary64 overflow.

### Meaning

After certified normalization:
- overflow in the mathematical filter expression is not an inherent problem,
- remaining hazards are interval widening, underflow in tiny components, and rounding certification.

## 12. Determinant interval

Use the fixed canonical 3x3 determinant expression.

Conceptually:

    D_I
      =
    det_interval(
      R'_1,
      R'_2,
      R'_3
    ).

The interval must contain the exact signed determinant.

Exact topology has already established that the true determinant is positive.

However D_I may still cross zero because:
- the tetra is nearly degenerate,
- the interval is conservative.

A zero-crossing interval is allowed.

It typically causes the final sign filter to become Uncertain.

Do not clamp D_I to positive merely because validity is known.

## 13. Edge-sum interval

For all six tetra edges compute interval coordinate differences and squared norms.

Then:

    S_I
      =
    sum six interval squared lengths.

For a valid finite tetra the exact S is positive.

If the computed interval cannot certify:

    lower(S_I) > 0,

the filter returns Uncertain.

This is a filter limitation, not a geometry failure.

## 14. Cross-polynomial interval

For tetra A and B obtain:

    D_A_I, S_A_I
    D_B_I, S_B_I.

Form:

    F_I
      =
    square(D_A_I) * cube(S_B_I)
      -
    square(D_B_I) * cube(S_A_I).

The interval square operation must correctly handle an interval crossing zero.

The exact F must be contained in F_I.

## 15. Certified result semantics

If:

    lower(F_I) > 0

then:

    q_MR(A) > q_MR(B)

is certified.

Return:

    GreaterFast.

If:

    upper(F_I) < 0

then:

    q_MR(A) < q_MR(B)

is certified.

Return:

    LessFast.

Otherwise:

    0 in F_I

or sign cannot be certified.

Return:

    Uncertain

and invoke exact D26QMR1.

## 16. Fast equality is forbidden

Even if:

    F_I = [0,0]

under some particular interval execution, D26QMRF1 does not own equality semantics.

Reference policy:

    fast path
      ->
    Less or Greater only.

Every apparent zero/tie:
- falls back,
- exact D26QMR1 decides Equal/Less/Greater.

This keeps one authority for exact ties.

## 17. Why interval overlap is expected

Interval filtering becomes uncertain when:
- the two tetra qualities are genuinely close,
- the determinant is near zero,
- subtraction cancellation widens local coordinate intervals,
- subnormal range reduces useful precision,
- the backend deliberately widens endpoints conservatively.

These are exactly the cases where paying for exact arithmetic is justified.

A high fallback rate on adversarial corpora is not a correctness defect.

## 18. Directed rounding versus nextafter widening

Two implementation families are plausible.

### A. Directed rounding

For each interval operation:
- evaluate lower endpoint with rounding downward,
- upper endpoint with rounding upward.

Benefits:
- direct interval semantics.

Risks/costs:
- floating-environment control,
- compiler support requirements,
- rounding-mode switch overhead,
- thread/runtime integration.

### B. Round-to-nearest plus explicit outward widening

Perform a correctly rounded ordinary operation, then widen outward.

Benefits:
- avoids frequent global rounding-mode switches.

Risks:
- one-nextafter sufficiency must be proved for every primitive,
- overflow/subnormal behavior must be handled,
- FMA contraction can invalidate the assumed operation tree.

No backend is frozen.

The qualification target is interval containment, not a specific technique.

## 19. FMA policy

A fused multiply-add computes:

    a*b+c

with one final rounding rather than a rounded multiply followed by rounded add.

This can:
- improve accuracy,
- change floating results,
- invalidate an error proof derived for separate operations.

Therefore Dynamics26 must choose one of:

### Explicit-FMA filter

The proof and code both call an explicit FMA operation.

### Non-contracted filter

The build contract prevents implicit contraction for the certified expression tree.

Forbidden:

    prove non-FMA arithmetic
    while allowing the compiler to fuse it opportunistically.

The exact D26QMR1 fallback is independent of this choice.

## 20. Underflow policy

The mathematical normalization removes ordinary overflow.

It does not guarantee every tiny component remains normal.

For first qualification:

    any interval primitive whose containment proof depends on unsupported subnormal/underflow behavior
      ->
    Uncertain
      ->
    exact fallback.

Do not enable flush-to-zero semantics in a certified filter unless a separate proof explicitly covers it.

Gradual underflow may be supported later as part of the frozen floating environment.

## 21. Overflow policy

After successful local normalization the exact D,S,F values are bounded.

If the interval backend nevertheless signals:
- overflow,
- non-finite endpoint,

the filter returns Uncertain.

This protects against:
- backend bugs,
- excessive conservative widening,
- unsupported environment.

Exact fallback remains capable of handling the original finite binary64 coordinates.

## 22. Existing repository floating policy observation

A repository search at this research point found no current default-branch occurrences of:

    -ffast-math
    fast-math
    fp-contract
    FENV_ACCESS.

This is only a point-in-time observation.

It is not a frozen build guarantee.

Future D26QMRF1 qualification must add explicit compiler/FP-environment gates rather than rely on
absence of current flags.

## 23. Dynamic interval filter is the first qualification candidate

Reasoning:

1. exact D26QMR1 already defines truth,
2. interval arithmetic can directly enclose coordinate subtraction and determinant cancellation,
3. the high-degree cross expression can be evaluated without hand-tuned error constants,
4. Bronnimann/Burnikel/Pion provide strong precedent for dynamic interval filters in exact geometric
   decisions,
5. CGAL uses interval filtering with exact fallback and optionally adds semi-static filters,
6. formal-filter literature shows semi-static hand derivation contains subtle pitfalls.

Therefore first reference acceleration should be:

    D26QMRF1
      dynamic interval
      ->
    D26QMR1 exact.

## 24. Semi-static research: standard gamma notation

For later work define unit roundoff:

    u = 2^-53

for binary64 round-to-nearest.

For:

    n u < 1

use:

    gamma_n
      =
    n u/(1-nu).

Higham's theta/gamma framework provides a compact way to combine finite chains of correctly rounded
operations when underflow/overflow assumptions are satisfied.

These bounds are tools for a future certificate.

They are not a license to use:

    K * epsilon

without an expression-specific proof.

## 25. Determinant semi-static candidate bound

Consider one fixed non-FMA expression tree.

Assume:
- all required subtractions/multiplications/additions remain in the qualified normal range,
- every anchor-relative coordinate is formed by one rounded subtraction,
- each determinant monomial is a product of three such components,
- each triple product uses two rounded multiplications,
- six signed triple products are summed using five rounded additions.

For one exact determinant monomial:
- three input subtraction roundings,
- two product roundings

give a five-operation theta-type factor.

A six-term sum introduces up to five more addition roundings.

This motivates the conservative research form:

    |D_hat-D|
      <=
    gamma_10
    P_D

where:

    P_D
      =
    sum of absolute exact determinant monomials.

This is a **paper derivation candidate**.

It is not yet a Dynamics26 certificate because:
- P_D must itself be safely bounded from computed quantities,
- subnormal behavior must be included or excluded,
- the exact compiler expression tree must be frozen,
- FMA/non-FMA behavior must match the proof.

## 26. Edge-sum semi-static candidate bound

Let one scalar edge component be computed by one rounded subtraction:

    e_hat = fl(x-y).

Then its square uses one rounded multiplication.

Relative to the exact squared component, the subtraction perturbation appears twice plus the multiply
rounding, motivating a three-operation theta-type factor.

There are 18 non-negative squared scalar terms in S.

Sequential summation uses 17 rounded additions.

This motivates the research form:

    relative S error
      on the order of
    gamma_20

under the same normal-range/non-FMA assumptions.

Again this is not frozen.

A formal or independently audited derivation must define:
- exact summation tree,
- whether pairwise/Kahan summation is used,
- input subtraction handling,
- FMA policy.

## 27. Why no semi-static constant is accepted yet

The comparison polynomial is degree 12 in the two tetra coordinate scales.

A seemingly small mistake in:
- subtraction error,
- determinant cancellation,
- repeated powers,
- FMA contraction,
- underflow,
- endpoint conversion

can turn an intended certificate into a wrong-sign filter.

Melquiond/Pion specifically demonstrate that formal checking can expose subtle filter pitfalls.

Therefore:

    D26QMRS1
      remains research-only.

No numerical error constant is accepted by this document.

## 28. Future a-posteriori scalar enclosure

A possible middle layer between full dynamic intervals and a static constant is:

1. compute D_hat and an a-posteriori determinant error bound E_D,
2. compute S_hat and a certified bound E_S,
3. form scalar intervals:

       D in [D_hat-E_D, D_hat+E_D]
       S in [S_hat-E_S, S_hat+E_S]

4. propagate only these small scalar intervals through F,
5. certify sign or fall back.

This can reduce interval overhead while retaining explicit containment.

It is not yet selected.

## 29. Future filter cascade candidate

Long-term candidate:

    exact-positive validity
      ->
    D26QMRS1 semi-static/almost-static filter
      ->
    D26QMRF1 dynamic interval filter
      ->
    D26QMR1 exact fallback.

The first implementation/qualification stage should omit D26QMRS1:

    validity
      ->
    D26QMRF1
      ->
    D26QMR1.

This gives a simpler correctness ladder.

## 30. Performance telemetry

Required counters for research:

    qmr_compare_count
    qmr_dynamic_certified_less
    qmr_dynamic_certified_greater
    qmr_dynamic_uncertain
    qmr_dynamic_range_guard_fallback
    qmr_dynamic_zero_overlap
    qmr_dynamic_subnormal_fallback
    qmr_exact_fallback
    qmr_exact_equal.

Optional timing/cost telemetry:
- interval primitive count,
- exact fallback bigint width,
- operation class,
- pathology family.

No wall-time threshold is part of correctness qualification.

## 31. Fallback-rate experiment classes

Measure separately on:

### Ordinary random shape-regular tetrahedra

Expected:
- high fast-certification rate.

### Near-equal quality pairs

Expected:
- higher exact fallback.

### Sliver/near-degenerate families

Expected:
- determinant interval uncertainty,
- increased fallback.

### Exact q_MR ties

Expected:
- fast path cannot establish Equal,
- exact fallback mandatory.

### Wide exponent-span binary64 fixtures

Expected:
- range/subnormal guards exercised.

### Large translated coordinates

Purpose:
- expose subtraction-conditioning effects without confusing them with exact geometry.

## 32. Operation-level fallback rates

The pair distribution depends on optimizer operation.

Record separately:
- elementary flips,
- edge removal,
- smoothing acceptance,
- SPR branch comparisons.

SPR may compare many closely competing candidates and therefore have a different fallback profile from ordinary
local smoothing.

One global fallback percentage is insufficient.

## 33. Determinism

For the same:
- canonical binary64 inputs,
- canonical filter expression order,
- D26QMRF1 version,
- qualified build environment,

filter result/telemetry should replay deterministically.

Across a different qualified filter implementation:
- fast/uncertain path may differ,
- final D26QMR1 comparison must not differ.

Topology determinism depends on the final exact comparison, not on identical performance path.

## 34. Verification ladder

### F0 — exact oracle authority

Every filtered result cross-checks D26QMR1 in qualification builds.

### F1 — interval containment unit tests

Every primitive is checked against independent high-precision/exact arithmetic.

### F2 — expression containment

D_I, S_I and F_I contain independent exact references.

### F3 — sign safety

Every Less/Greater fast result agrees with D26QMR1.

### F4 — uncertainty safety

Overlap/range/subnormal cases return Uncertain rather than guessed sign.

### F5 — adversarial/replay

Permutation, scale, translation, exponent-span and pathology corpora replay in Debug/Release.

### F6 — performance evidence

Only after F0-F5 pass should fallback rate and speed influence filter selection.

## 35. Research gates

    M6-R75
      comparison polynomial F is cross-checked against exact D26QMR1 order

    M6-R76
      bi-homogeneous independent tetra scale invariance is verified

    M6-R77
      canonical filter vertex order gives replay-stable filter telemetry

    M6-R78
      dynamic power-of-two normalization preserves sign and enforces documented range bounds

    M6-R79
      interval D,S,F enclosures contain independent exact references

    M6-R80
      normalized mathematical bounds D^2<27, S<72 and |F|<20,155,392 are verified

    M6-R81
      fast Less/Greater never disagrees with exact D26QMR1

    M6-R82
      interval overlap can only produce Uncertain

    M6-R83
      exact q_MR ties always reach exact fallback

    M6-R84
      subnormal/overflow/environment guard failures fall back safely

    M6-R85
      FMA-enabled and non-contracted candidate paths are tested as separate expression contracts

    M6-R86
      repository CI rejects unsafe-math/filter-environment drift once implementation exists

    M6-R87
      gamma_10 determinant and gamma_20 edge-sum research bounds are independently derived or corrected

    M6-R88
      no semi-static threshold is accepted without expression-specific certification

    M6-R89
      fallback telemetry is stratified by operation and pathology class

    M6-R90
      Debug/Release/replay preserve final exact comparison and qualified filter semantics.

These are research gates only.

## 36. Current conclusion

The strongest first fast path is not a guessed relative epsilon.

It is:

    exact comparison polynomial
      +
    independent power-of-two normalization
      +
    certified interval containment
      +
    exact fallback.

This keeps the mathematical authority simple:

    filter may prove a sign;
    exact oracle decides every uncertain case.

Semi-static filtering remains attractive for performance, but should be earned by a separate
certificate rather than hand-tuned constants.

This document does not authorize production M6 code.


## 37. Interval-backend research update

The first D26QMRF1 backend is now narrowed to:

    D26INT1
      =
    FE_TONEAREST primitive arithmetic
      +
    per-primitive adjacent-representable outward widening
      +
    exact/power-of-two normalization
      +
    strict environment guard
      +
    D26QMR1 fallback on uncertainty.

Important:

    per-primitive widening
    !=
    evaluate a compound expression and widen once at the end.

The one-step enclosure theorem applies to one correctly rounded primitive operation.

Therefore:
- cube is built from interval multiplications,
- determinant is built from interval multiplications/additions/subtractions,
- F is built from interval square/cube/multiply/subtract nodes.

See INTERVAL_ARITHMETIC_BACKEND.md.

## 38. One-step primitive enclosure theorem

Let z be the exact finite real result of one binary64 primitive operation and:

    r = RN(z)

the correctly rounded result in round-to-nearest mode.

Let:

    prev(r)
      =
    adjacent binary64 below r

and:

    next(r)
      =
    adjacent binary64 above r.

Then:

    prev(r)
      <=
    z
      <=
    next(r).

Therefore one adjacent-representable outward step encloses the exact result of that one primitive.

The theorem requires the actual executed operation to match the certified primitive:
- binary64 RN semantics,
- no unsupported flush-to-zero,
- no hidden reassociation,
- no unproved FMA contraction.

## 39. Environment guard

The adjacent-step function itself may be rounding-mode independent.

The arithmetic being enclosed is not.

D26INT1 therefore requires:

    active rounding mode == FE_TONEAREST.

If this cannot be established:

    UncertainEnvironment
      ->
    D26QMR1 exact fallback.

The first backend does not change the rounding mode.

## 40. Contraction policy refinement

Clang's default precise floating model permits FP contraction.

Therefore merely observing that the repository does not currently pass an explicit fp-contract flag is
not enough to qualify a non-FMA proof.

When implementation is authorized, the certified D26INT1 kernel must use a frozen contraction policy,
leading candidate:

    -ffp-contract=off

or an equivalently qualified source/compiler contract.

An explicit-FMA interval backend is a different future proof.

## 41. Subnormal capability policy

AArch64 FPCR contains:
- RMode,
- FZ flush-to-zero control.

Apple's arm64 kernel headers expose the same architectural FPCR fields.

Therefore D26INT1 must not infer gradual-underflow capability solely from the CPU family name.

First research policy:
- static binary64 capability checks,
- runtime arithmetic probes that cannot be constant-folded,
- exact fallback if gradual-subnormal behavior is not demonstrated.

Direct FPCR inspection may be a platform-specific diagnostic, but is not required as the portable
first contract.

## 42. Adjacent-step implementation boundary

std::nextafter is the standards/library reference behavior for moving to the adjacent representable
value.

A future optimized backend may use a derived binary64 bit-step implementation to avoid:
- libm call overhead,
- nextafter floating-exception flag side effects.

Such a bit-step implementation must be exhaustively cross-checked against std::nextafter over:
- zeros,
- subnormals,
- normals,
- infinities boundary behavior,
- both signs.

No production choice is frozen here.
