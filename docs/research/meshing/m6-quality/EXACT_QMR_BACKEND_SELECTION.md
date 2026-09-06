# M6 Research — D26QMR1 Exact Backend Selection

Status: RESEARCHING / backend-contract candidate
Date: 2026-09-06

## 1. Research question

When D26QMRF1 cannot certify a mean-ratio comparison, which exact backend should evaluate D26QMR1?

Candidates considered:

1. reuse/refactor the existing Dynamics26 M1 dyadic BigInt kernel,
2. add a new Shewchuk-style floating-expansion backend,
3. introduce an external arbitrary-precision integer backend such as Boost.Multiprecision cpp_int,
4. retain multiple backends only as independent qualification/benchmark oracles.

The leading first backend is:

    D26QMRB1
      =
    shared internal M1 dyadic BigInt arithmetic.

The reason is not that it is proven fastest.

It is that it already exists in the repository, is exact for stored binary64 coordinates, has passed
M1 qualification, and naturally evaluates the D26QMR1 integer polynomial.

## 2. Repository truth

Current source:

    src/meshing/RobustPredicates.cpp

contains a private BigInt implementation inside the predicates translation unit.

Observed properties:
- signed magnitude,
- vector of 32-bit limbs,
- exact left shift,
- signed addition/subtraction through sign handling,
- schoolbook limb multiplication,
- exact sign,
- binary64 dyadic decoder,
- common-exponent exact integer coordinate conversion,
- generic exact integer determinant.

The public predicate API exposes only:
- predicate sign,
- evaluation path,
- telemetry.

The BigInt/dyadic types are not public API.

This is desirable.

## 3. M1 qualification evidence

M1.7 records the exact production path as:

    finite binary64
      ->
    exact dyadic decomposition
      ->
    common power-of-two integer scale
      ->
    Dynamics26 signed arbitrary-precision integer
      ->
    exact determinant.

M1.8 then adds a certified floating filter with exact fallback.

Therefore Dynamics26 already has the same high-level architecture needed by M6:

    certified fast path
      ->
    exact dyadic integer fallback.

The M6 difference is the exact polynomial, not the arithmetic philosophy.

## 4. Why floating expansions are not "M1 reuse"

Shewchuk expansion arithmetic is an important literature-backed exact-arithmetic technique.

It represents an exact quantity as a non-overlapping expansion of floating components and supports
adaptive exact addition/multiplication.

However the current Dynamics26 M1 production source does not implement that representation.

Therefore adding expansions for M6 would mean:

    new exact backend family,

not:

    reuse existing M1 backend.

This distinction matters for:
- code risk,
- compiler assumptions,
- test burden,
- maintenance,
- clean architecture.

## 5. D26QMR1 does not need gcd/division

For tetra T, let the M1 common-exponent conversion produce integer coordinates P_i with physical
coordinates:

    X_i
      =
    alpha_T P_i

for one exact positive dyadic alpha_T.

Let:

    d_T
      =
    determinant magnitude in the integer coordinate system

and:

    s_T
      =
    six-edge squared-length sum in the same integer coordinate system.

Then:

    D_T
      =
    alpha_T^3 d_T

and:

    S_T
      =
    alpha_T^2 s_T.

Therefore:

    D_T^2/S_T^3
      =
    d_T^2/s_T^3.

No primitive gcd is required.

Tetra A and B may even use different:

    alpha_A
    alpha_B.

The ratios remain independently scale invariant.

### Engineering consequence

The currently implemented M1 BigInt operation set is much closer to sufficient than earlier research
assumed.

No general arbitrary-precision:
- gcd,
- division,
- rational normalization

is required for the first exact comparator.

## 6. Exact key construction

For a valid positive tetra T:

### Step 1 — exact integer coordinates

Use M1-compatible dyadic conversion over the 12 coordinate scalars.

### Step 2 — exact edge differences

Form all six edge vectors:

    e_01
    e_02
    e_03
    e_12
    e_13
    e_23.

There are 18 scalar integer differences.

The first three anchor edges also feed the determinant.

### Step 3 — determinant

Use a fixed exact 3x3 relative determinant:

    d
      =
    det(
      e_01,
      e_02,
      e_03
    ).

Exact-positive validity is established separately by M1 Orient3D.

D26QMR1 may square signed d directly because:

    d^2

removes sign, but a debug/qualification build should cross-check the determinant sign against M1.

### Step 4 — squared-edge sum

    s
      =
    sum over six edges
      (dx^2+dy^2+dz^2).

For a non-degenerate finite tetra:

    s > 0.

### Step 5 — quality rational key

Conceptual exact key:

    K(T)
      =
    (n,m)

where:

    n=d^2
    m=s^3.

The key need not be reduced.

### Step 6 — compare

    K(A) > K(B)

iff:

    n_A m_B
      >
    n_B m_A.

The sign of:

    n_A m_B
      -
    n_B m_A

is the exact D26QMR1 result.

## 7. Existing BigInt operations are almost sufficient

The current private BigInt already provides:
- signed addition,
- multiplication,
- negation,
- left shift,
- sign.

Therefore a first exact comparison can be expressed without a new compare API:

    delta
      =
    n_A*m_B
      +
    negate(n_B*m_A).

Then:

    sign(delta)

is the result.

A future internal compareAbs/nonnegative compare could avoid materializing the final subtraction, but
that is an optimization, not a correctness dependency.

## 8. Exact arithmetic operation graph

One tetra key requires conceptually:

### Edge differences
- 18 signed BigInt differences.

### Edge norm
- 18 BigInt squares,
- fixed exact addition tree.

### Determinant
Using a cofactor 3x3 tree:
- 9 BigInt multiplications,
- 5 signed additions/subtractions.

### Powers
- one determinant square,
- one s square,
- one multiply to form s^3.

Thus the key graph is small and fixed.

The expensive part is limb width, not graph size.

Pair comparison then needs:
- two cross multiplications,
- one signed subtraction/comparison.

No root or division is involved.

## 9. Why the current generic determinant should not define final performance architecture

M1 currently contains a generic recursive determinant routine.

For tiny predicate matrices this favored simplicity/correctness.

D26QMR1 needs only one exact 3x3 determinant.

Leading research direction:
- reuse the same BigInt arithmetic,
- use a fixed explicit 3x3 exact expression tree,
- cross-check against the generic determinant in qualification tests.

This avoids:
- recursive minor construction,
- temporary matrix allocation,
- arithmetic graph ambiguity.

No implementation is authorized yet.

## 10. Shared-kernel architecture

Do not:
- copy the BigInt class into M6,
- expose BigInt through public FEM/meshing API,
- make M1 depend on M6 quality policy.

Leading future internal architecture:

    meshing/exact/
      Binary64Dyadic
      SignedBigInt
      ExactIntegerHelpers

consumed by:

    M1 RobustPredicates
    M6 D26QMR1 exact comparator.

Policy ownership remains separate:

    exact arithmetic mechanism
      shared

but:

    predicate semantics
      M1-owned

and:

    quality-order semantics
      M6-owned.

## 11. Refactoring risk

Extracting a currently private qualified arithmetic implementation is not mathematically neutral from a
software-verification perspective.

Required before any future refactor is accepted:
- all M1 exact predicate fixtures unchanged,
- all M1 adversarial/replay tests unchanged,
- exact-zero behavior unchanged,
- fast/fallback result semantics unchanged,
- no public ABI expansion,
- source-level fast-math guard retained or strengthened,
- macOS arm64 Debug/Release exact-head CI.

Until then, research documentation does not authorize extraction.

## 12. BigInt performance characteristics

Current M1 BigInt uses:
- 32-bit limbs,
- vector storage,
- schoolbook multiplication.

If operands have:

    p
    q

limbs, schoolbook multiplication is:

    O(pq)

limb products.

This is acceptable for:
- reference exactness,
- rare filtered fallbacks,
- small local cavities

until telemetry proves otherwise.

It should not be assumed optimal for millions of repeated quality comparisons.

## 13. Conservative key-width bounds

Using the previous binary64 worst-case edge-component bound:

    B <= 2099.

Research bounds:

    bitlen(d)
      <=
    3B+3
      =
    6300

    bitlen(s)
      <=
    2B+5
      =
    4203

    bitlen(d^2)
      <=
    12600

    bitlen(s^3)
      <=
    12609.

With 32-bit limbs:

    d
      <=
    197 limbs

    s
      <=
    132 limbs

    d^2
      <=
    394 limbs

    s^3
      <=
    395 limbs.

Thus cached unreduced:

    (d^2,s^3)

can require up to approximately:

    789 * 4
      =
    3156 bytes

of raw limb payload per tetra in the absolute input-format worst case.

This excludes:
- vector capacity overhead,
- allocator metadata,
- object metadata.

## 14. Global exact-key cache is not the first policy

At the worst-case raw bound:

    1,000,000 tetra
      * 3156 bytes
      ~
    3.16 GB

before container/object overhead.

Typical engineering meshes will usually be far smaller in exact integer width, but the theoretical
cost demonstrates that a permanent global exact-key cache is not a safe default.

Leading policy:

    no global persistent exact QMR key cache
    before telemetry.

## 15. Local memoization policy

Exact key memoization is most valuable where the same candidate tetra is compared repeatedly.

### Connectivity-only local optimization

Coordinates are fixed.

A pass/cavity-scoped cache keyed by canonical tetra vertex identity is safe.

### Edge-removal DP

The same candidate pole tetra can appear in repeated recurrence comparisons.

Local memoization can avoid rebuilding d/s.

### SPR / cavity retriangulation

This is the strongest caching candidate.

Many legal triangulations reuse the same candidate tetra subsets.

Cache:

    canonical tetra identity
      ->
    exact key.

### Smoothing

Coordinates change.

A PointId-only cache is unsafe across moves.

Any cache must be:
- proposal scoped,
- or keyed by exact coordinate bits / coordinate generation.

Leading first policy:
- do not retain exact keys across accepted vertex movement.

## 16. Cache representation alternatives

### A. Cache (d,s)

Worst-case raw limbs:

    197 + 132
      =
    329 limbs
      ~
    1316 bytes.

Pros:
- smaller.

Cons:
- repeated comparisons recompute d^2 and s^3.

### B. Cache (d^2,s^3)

Worst-case raw:

    ~3156 bytes.

Pros:
- pair comparison becomes only cross products.

Cons:
- larger local memory.

### C. Lazy promoted cache

Store:

    d,s

first.

Compute/cache:

    d^2,s^3

only after the tetra actually reaches exact comparison repeatedly.

This is the leading research candidate for SPR/local search.

No threshold is frozen.

## 17. Optional binary-content normalization

General gcd is unnecessary.

A cheaper future optimization may remove a common power of two.

For example:
- count common trailing zero bits in integer edge components,
- right-shift them by the shared power.

This preserves the exact q_MR ratio and can substantially reduce limb widths for highly aligned dyadic
coordinates.

The current private BigInt does not provide this right-shift/valuation API.

Therefore binary-content normalization is deferred until telemetry demonstrates value.

It is not part of D26QMR1 truth.

## 18. Floating-expansion backend candidate

Research placeholder:

    D26QMRE1.

Shewchuk expansion arithmetic is attractive because:
- it is designed for exact polynomial decisions from binary floating inputs,
- it can adapt precision to uncertainty,
- robust geometric predicates demonstrate strong performance.

However D26QMR1 differs from standard orientation predicates:
- the final comparison polynomial includes squared determinant and cubic edge sum,
- exact expansion lengths can grow through repeated products,
- a new exact expression implementation and environment qualification are required,
- current M1 code provides no expansion type to reuse.

Therefore:

    D26QMRE1
      is not the first exact fallback.

It may later be benchmarked if BigInt fallback cost is material.

## 19. External arbitrary-precision alternative

Boost.Multiprecision provides:
- arbitrary-precision cpp_int,
- a Boost-licensed C++ backend,
- current C++20 compatibility,
- no GMP runtime dependency for cpp_int.

This makes it a credible independent implementation/benchmark candidate.

Research placeholder:

    D26QMRX1.

But current Dynamics26:
- does not use Boost.Multiprecision,
- already has a qualified in-project BigInt,
- would add a new dependency surface solely for convenience/performance evaluation.

Therefore Boost cpp_int is not selected as the first production fallback.

It is suitable for:
- benchmark builds,
- independent cross-checks,
- a possible later replacement if maintenance/performance evidence justifies it.

## 20. GMP-backed arithmetic

GMP-class backends can be substantially faster for large integers.

But D26QMR1 does not yet have evidence that:
- exact fallback frequency is high,
- operand sizes are large enough,
- BigInt multiplication dominates optimizer wall time.

Adding an external binary/runtime dependency before those measurements would invert the project
priority:

    performance dependency
    before
    demonstrated performance problem.

Therefore no GMP backend is selected.

## 21. Independent oracle policy

Avoid a common-mode error where:
- production BigInt,
- test oracle

share the same arithmetic bug.

Qualification should retain independent paths:

### Oracle A — Python Fraction

Exact stored binary64 rational coordinates.

### Oracle B — Python/dyadic integer

Independent exact integer construction and D26QMR ratio.

### Optional Oracle C — Boost cpp_int test/benchmark build

Independent C++ implementation if introduced.

Production D26QMRB1 must agree with accepted fixtures from independent oracles.

## 22. Exact fallback telemetry

Required counters/measurements after implementation:

    qmr_exact_fallback_calls
    qmr_exact_key_builds
    qmr_exact_key_cache_hits
    qmr_exact_key_cache_misses
    qmr_exact_max_d_bits
    qmr_exact_max_s_bits
    qmr_exact_max_power_bits
    qmr_exact_max_cross_bits
    qmr_exact_compare_equal
    qmr_exact_time_by_operation_class.

Stratify by:
- 2->3,
- 3->2,
- edge removal,
- smoothing,
- SPR.

The backend should not be optimized from one global average.

## 23. Backend promotion rule

D26QMRB1 remains the production-leading exact backend unless evidence shows a material problem.

A replacement/preceding exact backend must demonstrate:

1. identical D26QMR1 results,
2. identical exact ties,
3. deterministic replay,
4. no new unsafe compiler assumptions,
5. materially better end-to-end optimizer performance,
6. acceptable dependency/licensing/maintenance cost.

Microbenchmark speed alone is insufficient.

## 24. Candidate exact cascade

Leading research architecture:

    D26INT1
      ->
    D26QMRB1
      ->
    result.

Possible later optimization:

    D26INT1
      ->
    optional D26QMRE1
      ->
    D26QMRB1 reference backstop

is **not** currently justified.

An exact backend should not become another uncertainty layer unless it provides a certified exact
result.

## 25. Research gates

    M6-R109
      repository audit confirms M1 exact backend is dyadic BigInt, not floating expansions

    M6-R110
      M1 common-exponent scaling is proved sufficient for D26QMR1 without gcd/division

    M6-R111
      D26QMRB1 exact keys agree with primitive-gcd-normalized oracle keys

    M6-R112
      fixed exact 3x3 determinant agrees with M1 generic exact determinant

    M6-R113
      exact determinant sign agrees with M1 Orient3D for the same ordered tetra

    M6-R114
      d/s and d^2/s^3 width telemetry never exceeds the derived binary64 bounds

    M6-R115
      D26QMRB1 agrees with independent Python Fraction and dyadic-integer corpora

    M6-R116
      extraction of shared exact arithmetic, if implemented, leaves all M1 qualification fixtures unchanged

    M6-R117
      no public BigInt or M6 quality type leaks into installed/public predicate ABI

    M6-R118
      local cavity cache returns identical results with caching disabled

    M6-R119
      smoothing invalidates/prohibits stale PointId-only exact-key reuse

    M6-R120
      SPR/edge-removal cache telemetry measures hit rate and memory separately

    M6-R121
      global persistent exact-key cache remains disabled until explicit memory/performance evidence exists

    M6-R122
      optional Boost cpp_int oracle/benchmark, if introduced, agrees bit-for-bit in comparison results

    M6-R123
      expansion backend remains experimental until exact-fallback telemetry demonstrates a need

    M6-R124
      Debug/Release/replay preserve final D26QMR1 result and exact-cache semantics.

These are research/verification-design gates.

## 26. Current conclusion

The first exact D26QMR1 fallback should reuse the arithmetic mechanism Dynamics26 already trusts:

    M1 dyadic BigInt
      ->
    shared internal exact kernel
      ->
    fixed D26QMR exact key
      ->
    exact cross comparison.

Key refinements:
- no general gcd/division is required,
- BigInt stays internal,
- no permanent global exact-key cache,
- local/lazy memoization is preferred for repeated cavity search,
- expansion arithmetic is a later alternative, not current M1 reuse,
- external multiprecision is a benchmark/oracle candidate, not a first dependency.

No production refactor or M6 implementation is authorized by this document.
