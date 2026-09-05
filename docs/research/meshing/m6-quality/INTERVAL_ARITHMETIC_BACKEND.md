# M6 Research — D26INT1 Outward Interval Arithmetic Backend

Status: RESEARCHING / interval-primitive contract candidate
Date: 2026-09-06

## 1. Engineering question

D26QMRF1 needs certified intervals for a small homogeneous polynomial over binary64 tetra coordinates.

Which C++20 / macOS Apple-Silicon interval backend should Dynamics26 qualify first?

Candidate families:

1. dynamic directed rounding,
2. round-to-nearest plus adjacent-representable outward widening,
3. error-free transformations / explicit FMA-assisted enclosures.

The leading first-qualification candidate is:

    D26INT1
      =
    round-to-nearest
      +
    per-primitive next-down / next-up widening
      +
    no rounding-mode mutation.

D26QMR1 exact comparison remains the authority.

## 2. Evidence hierarchy

Primary/reference evidence used here:

- M6-NUM-007 — IEEE 1788.1 binary64 interval-arithmetic model,
- M6-NUM-008 — Clang floating-point semantic modes and contraction/dynamic-rounding controls,
- M6-NUM-009 — Apple nextafter adjacent-representable behavior,
- M6-NUM-010 — Arm AArch64 FPCR RMode/FZ architecture,
- M6-NUM-011 — C++ nextafter / scalbn / fma semantic reference,
- M6-NUM-012 — Apple XNU arm64 FPCR field corroboration.

The D26INT1 primitive proofs and backend ranking are Dynamics26 first-principles derivations.

## 3. Backend A — dynamic directed rounding

Classic interval arithmetic can compute:

    lower endpoint
      under round-down

and:

    upper endpoint
      under round-up.

### Advantages

- tight primitive enclosures,
- direct outward-rounding semantics,
- classical interval implementation strategy.

### Costs

Dynamic rounding is not merely a hardware feature.

The compiler must honor the floating environment.

Clang provides controls including:
- dynamic rounding,
- strict FP model,
- FENV access/round pragmas,
- FP contraction control.

A correct implementation must also:
- save/restore thread floating environment,
- prevent arithmetic motion across rounding-mode changes,
- define exception-flag behavior,
- avoid leaking caller state.

### Dynamics26 position

Directed rounding is viable but is not the simplest first backend.

Research placeholder:

    D26INTD1.

## 4. Backend B — round-to-nearest plus adjacent widening

Keep certified arithmetic in:

    FE_TONEAREST.

For one exact primitive result z:

    r = RN(z).

Define:

    down1(r)
      =
    previous representable binary64

and:

    up1(r)
      =
    next representable binary64.

Return:

    [down1(r),up1(r)].

This backend does not change the rounding mode.

Research placeholder:

    D26INT1.

## 5. One-step primitive enclosure theorem

Assume:
- z is finite,
- the primitive operation is correctly rounded to binary64 RN,
- gradual-underflow semantics hold where required,
- r is the executed RN result.

Let p and n be the adjacent representable values below and above r.

Round-to-nearest selects r only when z lies in the Voronoi interval bounded by midpoints to adjacent
representable numbers.

Therefore:

    p <= z <= n.

So:

    [p,n]

contains the exact primitive result.

Tie-to-even does not break the proof:
- if z lies exactly on a midpoint,
- one neighbor is chosen by the tie rule,
- the other adjacent representable still bounds z.

## 6. The theorem is primitive-local

This is a critical restriction.

Safe:

    r = RN(a*b)
    enclosure = [down1(r),up1(r)].

Not generally justified:

    r = RN(
          several hidden/additional rounded operations
        )
    enclosure = one step around final r.

Multiple rounding errors can accumulate farther than one final ulp.

Therefore D26INT1 requires outward enclosure after every certified arithmetic node.

## 7. Compound expressions must use interval composition

Examples:

### Square

One multiplication:

    square(x)
      =
    x*x

can use one multiplication enclosure.

### Cube

Do not compute:

    RN(x*x*x)

and widen only once.

Use:

    I2 = mul_interval(I,I)
    I3 = mul_interval(I2,I).

For the positive S interval, a specialized non-negative square/cube path may be used, but every
multiplication remains individually enclosed.

### Determinant

Build the fixed determinant expression from interval:
- multiplication,
- subtraction,
- multiplication,
- addition/subtraction.

Do not evaluate one ordinary floating determinant and widen only the final scalar.

### D26QMRF1 polynomial

Build:

    F_I
      =
    square(D_A_I)
      * cube(S_B_I)
      -
    square(D_B_I)
      * cube(S_A_I)

from the certified interval primitives.

## 8. Interval representation

Research type:

    I = [lo,hi]

with:
- finite binary64 endpoints for the certified fast path,
- lo <= hi,
- mathematical set of all real x satisfying lo <= x <= hi.

Non-finite endpoints cause:

    UncertainRange
      ->
    exact fallback.

The first backend does not require IEEE-1788 decorations as product data.

The interval model is containment-oriented and intentionally small.

## 9. Point intervals

A stored finite binary64 scalar x enters as:

    [x,x].

This interval represents the exact real value encoded by that binary64 bit pattern.

No decimal parsing or re-interpretation occurs in the filter.

## 10. Outward primitive helper

Conceptually:

    outward(r)
      =
    [down1(r),up1(r)].

If r is non-finite:
- the fast path fails.

If r is zero:
- down1(0) is the smallest negative subnormal,
- up1(0) is the smallest positive subnormal,

unless a specialized operation has a stronger mathematical sign bound.

This can be wider than necessary but remains conservative under the qualified environment.

## 11. Addition

For:

    A=[a_lo,a_hi]
    B=[b_lo,b_hi],

exact interval addition is:

    [a_lo+b_lo, a_hi+b_hi].

D26INT1 computes:

    r_lo = RN(a_lo+b_lo)
    r_hi = RN(a_hi+b_hi)

and returns outward endpoints:

    [down1(r_lo),up1(r_hi)].

If either primitive result is non-finite:
- UncertainRange.

## 12. Subtraction

Exact interval subtraction:

    A-B
      =
    [a_lo-b_hi, a_hi-b_lo].

D26INT1 encloses the two primitive endpoint subtractions separately.

This operation is important for:
- anchor-relative coordinates,
- pairwise edge coordinates,
- determinant cofactors,
- final F term subtraction.

## 13. General multiplication

For finite intervals A,B, the exact extrema occur among:

    a_lo*b_lo
    a_lo*b_hi
    a_hi*b_lo
    a_hi*b_hi.

D26INT1:
1. evaluates all four binary64 products in RN,
2. outward-widens every product independently,
3. takes the minimum lower endpoint,
4. takes the maximum upper endpoint.

Do not:
- select the floating min/max first,
- then widen only once,

unless a separate proof establishes equivalence.

## 14. Non-negative multiplication specialization

Many D26QMRF1 quantities are non-negative:
- squared determinant interval after square(),
- S,
- S^2,
- S^3.

If:

    0 <= a_lo <= a_hi
    0 <= b_lo <= b_hi,

then multiplication is monotone:

    A*B
      =
    [a_lo*b_lo,a_hi*b_hi].

Only two endpoint products are needed.

The same per-primitive widening rule applies.

## 15. Square specialization

For:

    I=[lo,hi].

### If 0 is inside I

The exact lower bound is:

    0.

The exact upper bound is:

    max(lo^2,hi^2).

Return:
- lower = +0 exactly,
- upper = outward-up of both candidate endpoint squares, then maximum.

Do not use down1(0) for the lower bound because the mathematical square is known non-negative.

### If lo >= 0

Exact range:

    [lo^2,hi^2].

Enclose each multiplication endpoint.

Clamp the lower endpoint to at least +0.

### If hi <= 0

Exact range:

    [hi^2,lo^2].

Again use primitive multiplication enclosures.

This is tighter than generic interval multiplication I*I.

## 16. Positive cube specialization

D26QMRF1 needs:

    S^3

with certified:

    S_lo > 0.

Construct:

    S2 = square_nonnegative(S)
    S3 = mul_nonnegative(S2,S).

This preserves the per-primitive theorem.

Do not use a library pow(S,3).

## 17. Fixed determinant expression tree

The first backend must freeze one determinant arithmetic tree.

Candidate cofactor form for matrix:

    [a b c
     d e f
     g h i]

is:

    m0 = e*i - f*h
    m1 = d*i - f*g
    m2 = d*h - e*g

    t0 = a*m0
    t1 = b*m1
    t2 = c*m2

    D = (t0 - t1) + t2.

Every symbol above is an interval operation.

Reasons to freeze a tree:
- reproducible interval width,
- reproducible telemetry,
- compiler/contraction audit,
- future semi-static proof must match the same operation graph.

A different equivalent determinant formula is a different filter version unless independently shown to
preserve the qualified contract.

## 18. Edge-sum expression tree

For each of six canonical tetra edges:
1. compute three interval coordinate differences,
2. square each component with square(),
3. sum three squared-component intervals.

This gives six squared-edge intervals.

Then sum all six in a fixed balanced reduction tree.

Reason:
- deterministic interval width,
- lower rounding depth than an arbitrary sequential container reduction,
- no container-order dependence.

The older gamma_20 paper estimate assumed a sequential 18-term sum and remains only a research
reference; any future semi-static certificate must match the actual frozen reduction tree.

## 19. Final cross-polynomial tree

Given certified intervals:

    DA, SA, DB, SB,

compute:

    DA2 = square(DA)
    DB2 = square(DB)

    SA3 = cube_positive(SA)
    SB3 = cube_positive(SB)

    L = mul_nonnegative(DA2,SB3)
    R = mul_nonnegative(DB2,SA3)

    F = sub(L,R).

Fast sign rule:

    F.lo > 0
      =>
    Greater

    F.hi < 0
      =>
    Less

otherwise:

    Uncertain
      ->
    exact D26QMR1.

## 20. std::nextafter as reference adjacent-step oracle

C++/IEC-60559 semantics provide std::nextafter(from,to) as the next representable value from from toward
to.

The returned value is independent of the current rounding mode on an IEC-60559 implementation.

Therefore the simplest reference definitions are:

    down1(x)
      =
    nextafter(x,-infinity)

    up1(x)
      =
    nextafter(x,+infinity).

Apple's documented nextafter behavior also states that it returns the next machine-representable
number in the requested direction.

## 21. nextafter exception-flag side effect

The standard/library behavior may raise floating exceptions around:
- subnormal/zero steps,
- overflow steps.

D26INT1 correctness does not depend on these exception flags.

However production integration must choose a side-effect policy.

Research options:

### Reference implementation

Use std::nextafter.

Pros:
- standards/library semantics,
- simple qualification oracle.

Cons:
- possible exception-flag pollution,
- library-call overhead.

### Future bit-step adjacent implementation

For qualified IEC-60559 binary64:
- derive adjacent representable value from the 64-bit encoding,
- no floating operation required for the step itself.

Pros:
- no nextafter FP-exception side effect,
- cheap.

Cons:
- signed zero / sign ordering / boundary logic must be independently qualified.

Policy:
- std::nextafter is the reference oracle,
- bit-step optimization is not accepted until exhaustive equivalence tests exist.

## 22. Runtime rounding-mode guard

nextafter may be rounding-mode independent.

The arithmetic it encloses is not.

D26INT1 therefore requires:

    fegetround() == FE_TONEAREST

before entering the certified arithmetic kernel.

If false or unsupported:

    UncertainEnvironment
      ->
    D26QMR1 exact fallback.

The first backend never calls fesetround.

## 23. Compiler model for D26INT1

Clang documentation shows:
- default precise mode assumes round-to-nearest,
- default precise mode permits FP contraction,
- strict mode enables dynamic rounding semantics and disables contraction,
- fast/unsafe modes permit transformations incompatible with an unqualified primitive tree.

Leading future kernel contract:

    round-to-nearest only
    no reassociation
    honor finite/NaN/Inf semantics
    signed zero preserved
    FP contraction disabled for the certified kernel.

Important:

    absence of -ffast-math
    is not sufficient.

Because default precise mode can still permit contraction.

The future implementation should make the contraction policy explicit.

## 24. Why D26INT1 does not require dynamic-rounding compilation

The filter does not intentionally change the rounding mode.

It requires the caller environment to already be:

    FE_TONEAREST.

Then the certified kernel can be compiled for round-to-nearest.

The runtime guard must execute before entering that kernel.

This avoids the stronger compiler/fenv coupling required by directed-rounding interval code.

If future integration permits arbitrary caller rounding modes without fallback, that becomes D26INTD1
or another policy.

## 25. AArch64 environment

Arm AArch64 FPCR contains:
- RMode rounding field,
- FZ flush-to-zero control.

Architectural documentation defines:
- RN,
- +infinity,
- -infinity,
- toward-zero rounding modes,
- FZ disabled/enabled behavior.

Apple XNU arm64 register definitions expose corresponding:
- FPCR_FZ,
- FPCR_RMODE

fields.

Therefore the target architecture can represent environment states that invalidate D26INT1's
assumptions.

Do not infer:

    Apple Silicon
      =>
    always RN and gradual underflow.

Qualification/runtime policy must establish the required state.

## 26. Static binary64 capability checks

Future implementation gates should require the equivalent of:

    numeric_limits<double>::radix == 2
    numeric_limits<double>::digits == 53
    numeric_limits<double>::is_iec559 == true
    denormal representation available.

These are necessary but not sufficient.

They describe type/implementation capability, not necessarily the current runtime FPCR state.

## 27. Runtime subnormal capability probe

The first portable contract should use a runtime arithmetic probe, protected from constant folding.

Two distinct capabilities matter:

### Subnormal output preservation

Take runtime/volatile values representing:
- minimum positive normal,
- 0.5.

Require their product to equal the exact half-min-normal subnormal.

If the result becomes zero:
- output flush-to-zero is active or behavior is unsupported.

### Subnormal input preservation

Take runtime/volatile:
- minimum positive subnormal,
- 2.0.

Require arithmetic multiplication to equal the exact next subnormal expected from a non-arithmetic
reference such as scalbn/adjacent representation.

If the input behaves as zero:
- subnormal input handling is unsupported.

Any failed probe:

    D26INT1 disabled
      ->
    exact fallback.

No probe result changes geometry truth.

## 28. Direct FPCR inspection

AArch64 permits FPCR access architecturally at EL0 subject to system configuration.

A macOS-specific implementation could inspect FPCR directly.

This may provide:
- RMode state,
- FZ state.

But first research policy does not require inline assembly.

Reasons:
- portability of the reference backend,
- C++/fenv guard should remain primary where sufficient,
- direct register code becomes platform-specific maintenance.

Possible future diagnostic:

    D26INT1_AppleArm64FpcrProbe.

It must be cross-checked against the portable runtime behavior probes.

## 29. std::scalbn normalization

D26QMRF1 normalizes by exact radix powers.

On IEC-60559 systems, std::scalbn:
- computes multiplication by the radix power,
- is exact and rounding-mode independent unless a range error occurs.

Therefore it is a good reference normalization primitive.

Leading exponent selection for positive finite M:

    e = ilogb(M)
    k = -e-1.

Then, in exact arithmetic:

    1/2 <= scalbn(M,k) < 1.

The previous broader target:

    1/2 < lambda M <= 1

is refined to this conventional half-open interval.

## 30. Scaling interval endpoints

For endpoint x:
- if scalbn(x,k) incurs no range issue, use the exact result,
- if underflow/range uncertainty occurs, either outward-enclose that primitive under the D26INT1
  rules or return UncertainRange,
- non-finite result returns UncertainRange.

Because the filter already has an exact fallback, the first implementation may choose conservative
fallback rather than complicate normalization range recovery.

## 31. Large translation does not invalidate correctness

Anchor-relative subtraction can suffer severe floating cancellation when coordinates have large
absolute offsets.

But D26INT1 subtraction is itself an interval operation over the exact stored binary64 endpoint
values.

Therefore the exact real difference between stored coordinate values remains enclosed.

Consequence:
- large translation may widen intervals and increase fallback,
- it must not produce a wrong certified sign.

Translation-heavy fixtures are therefore performance/containment tests, not expected fast-path
guarantees.

## 32. Sterbenz opportunities are optional

Sterbenz's lemma can make subtraction exact when two same-sign floating numbers are sufficiently
close in ratio.

Large translated CAD coordinates often fall into such cases.

A future optimized filter may exploit exact-subtraction detection to reduce interval width.

First qualification does not depend on it.

Simple conservative outward subtraction remains the reference.

## 33. Signed zero policy

M1 canonical site identity normalizes signed coordinate zero.

Interval arithmetic still encounters zero results.

For generic outward arithmetic:
- exact unknown-sign zero result may be widened to a tiny symmetric interval.

For mathematically non-negative operations such as square:
- lower bound may be exactly +0.

Do not use signed-zero bit ordering as a quality decision.

## 34. Backend C — EFT / explicit FMA

A later backend can use error-free transformations.

std::fma(a,b,c) computes the ternary expression as though with infinite intermediate precision and one
final rounding.

This supports classic strategies such as:
- product plus residual,
- tighter determinant enclosures,
- reduced dynamic interval width.

Research placeholder:

    D26INTE1.

### Benefits

- potentially much tighter intervals,
- lower fallback rate,
- uses native FMA capability common on modern AArch64 hardware.

### Risks

- proof complexity,
- underflow behavior of residual terms,
- explicit FMA tree differs from D26INT1,
- performance must beat the simpler backend in actual optimizer workloads.

D26INTE1 is deferred until D26INT1 produces fallback telemetry.

## 35. Directed-rounding backend D26INTD1

A future D26INTD1 may:
1. save environment,
2. evaluate lower expressions downward,
3. evaluate upper expressions upward,
4. restore environment.

Qualification requires:
- Clang dynamic-rounding/strict contract,
- no arithmetic motion across mode transitions,
- thread-local semantics,
- exception-state policy,
- nested-call safety.

This is a separate backend version.

Its results must cross-check D26QMR1 and D26INT1.

## 36. Backend ranking

Current research ranking:

| Criterion | D26INT1 RN+adjacent | D26INTD1 directed | D26INTE1 EFT/FMA |
|---|---|---|---|
| First correctness proof | simplest | harder | harder |
| Mutates rounding mode | no | yes | no |
| Requires contraction control | yes | yes | explicit FMA tree |
| Primitive tightness | conservative | tight | potentially tightest |
| Subnormal proof required | yes | yes | yes |
| Reference portability | high | medium | medium/high |
| Expected fallback rate | moderate | lower | potentially lowest |
| First qualification choice | **yes** | no | no |

Performance ranking is not frozen.

## 37. Reference versus optimized adjacent step

D26INT1 semantics should name the operation:

    nextDown
    nextUp

rather than hard-code a library backend.

Reference oracle:

    std::nextafter.

Future optimized binary64 bit-step:
- allowed only after exhaustive equivalence qualification.

This keeps the mathematical interval contract independent of libm implementation cost.

## 38. Primitive containment oracle

Every primitive interval test must use an independent exact rational backend.

For point inputs:
- decode binary64 to exact dyadic rational,
- evaluate exact primitive result,
- assert interval containment.

For interval inputs:
- independently enumerate endpoint extrema for monotone/bilinear primitives,
- compare exact extrema against computed interval.

Do not test interval arithmetic only against another floating implementation.

## 39. Final-only-widening negative control

Qualification must include a deliberately unsafe implementation:

    evaluate compound expression in binary64
    widen final result by one adjacent value.

Search/generate examples where:
- accumulated rounding exceeds that final one-step enclosure,
- independent exact oracle lies outside the unsafe interval.

Purpose:
prove why D26INT1 widens per primitive.

This negative control is research/test code only.

## 40. Rounding-mode negative control

Run the arithmetic environment under:
- FE_UPWARD,
- FE_DOWNWARD,
- FE_TOWARDZERO

where supported.

Require:

    D26INT1 environment guard
      =>
    fast path disabled.

No topology result may depend on those modes because exact fallback remains authoritative.

## 41. FTZ negative control

Where test infrastructure permits flush-to-zero/subnormal-mode manipulation:
- enable an incompatible mode,
- execute capability probe,
- require filter disable/fallback.

On platforms where test code cannot portably toggle the mode:
- use architecture-specific CI only as an additional gate,
- never weaken the portable behavior check.

## 42. Exception flags

std::nextafter may raise:
- underflow/inexact,
- overflow/inexact

at representation boundaries.

D26INT1 does not use exception flags as quality truth.

Future integration must choose:

### Policy A — flags are not part of public solver state

Document that filter evaluation may affect flags.

### Policy B — preserve caller flags

Use a qualified mechanism or bit-step adjacent implementation that avoids these side effects.

Leading production preference:
- avoid leaking interval-helper side effects,
- likely favor the bit-step adjacent implementation after qualification.

This is not frozen until the application's FP-exception policy is audited.

## 43. Cache and replay

An interval endpoint is only a performance artifact.

Do not persist D26INT1 intervals as mesh authority.

If caching is researched:
- key by canonical coordinate bits and filter version,
- invalidation must follow coordinate changes,
- exact D26QMR1 remains authoritative.

Replay telemetry may record:
- filter backend version,
- environment capability result,
- number of primitive widenings,
- fallback reason.

## 44. Parallelism implication

D26INT1 does not mutate rounding mode.

That makes it naturally friendlier to future parallel candidate evaluation than D26INTD1.

This is an architectural advantage, not proof of thread safety.

Caches/telemetry still require concurrency design.

## 45. Qualification levels

### I0 — environment

Verify:
- binary64 type assumptions,
- FE_TONEAREST,
- gradual-subnormal probes,
- contraction/build contract.

### I1 — adjacency

Cross-check nextDown/nextUp against std::nextafter.

### I2 — primitive containment

Addition, subtraction, multiplication, square.

### I3 — composition containment

Cube, determinant, edge sum, F.

### I4 — adversarial range

Subnormal, cancellation, huge offsets, exponent spread.

### I5 — filter sign

Every certified sign agrees with D26QMR1.

### I6 — backend cross-check

D26INT1 vs directed-rounding / exact-rational reference on test builds.

### I7 — performance

Only after I0-I6 qualify should fallback rate and speed drive optimization.

## 46. Research gates

    M6-R91
      one-step adjacent-representable theorem is executable against exact dyadic primitive oracles

    M6-R92
      nextafter reference adjacency covers signed zero, subnormal, normal and boundary cases

    M6-R93
      D26INT1 addition/subtraction intervals contain exact extrema

    M6-R94
      general and non-negative multiplication intervals contain exact extrema

    M6-R95
      square and positive-cube specializations contain exact ranges

    M6-R96
      compound expressions widen per primitive; final-only-widen negative control fails as expected

    M6-R97
      fixed determinant interval tree contains exact determinant

    M6-R98
      fixed balanced edge-sum tree contains exact six-edge S

    M6-R99
      final F interval contains exact cross-polynomial value

    M6-R100
      FE_TONEAREST guard disables fast path under alternate rounding modes

    M6-R101
      gradual-subnormal capability probes detect unsupported FTZ/input-flush behavior

    M6-R102
      scalbn normalization yields the documented [1/2,1) maximum-magnitude target or falls back

    M6-R103
      compiler contract prevents unproved reassociation/contraction in D26INT1 kernel

    M6-R104
      nextDown/nextUp optimized bit-step candidate matches std::nextafter reference if introduced

    M6-R105
      Apple arm64 architecture/environment probe agrees with portable behavior probes where enabled

    M6-R106
      directed-rounding D26INTD1 cross-check never disagrees with D26QMR1

    M6-R107
      EFT/FMA D26INTE1 remains experimental until D26INT1 fallback telemetry justifies it

    M6-R108
      Debug/Release/replay preserve final exact comparison and qualified D26INT1 semantics.

These gates are research/verification design only.

## 47. Current conclusion

The first interval backend should optimize for proof simplicity, not theoretical minimum interval
width.

Leading architecture:

    environment guard
      ->
    RN primitive
      ->
    adjacent representable widening at every arithmetic node
      ->
    fixed interval expression tree
      ->
    certified F sign
      ->
    exact D26QMR1 on uncertainty.

This avoids:
- dynamic rounding-mode mutation,
- one-final-ulp fallacy,
- approximate equality,
- implicit FMA ambiguity.

Directed rounding and EFT/FMA remain valuable independent cross-check/optimization paths.

No production M6 code is authorized by this document.
