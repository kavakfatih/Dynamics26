# M6 Research — D26INT1 Subnormal Downscale Policy

Status: DECIDED / adopted, and it fixed a soundness bug rather than only a
coverage one
Date: 2026-09-06

Qualification: `tests/meshing/m6_quality/test_m6_filter_coverage.cpp`
(`unit_m6_filter_coverage`), plus `verifyMinNormalTieBoundary` in
`test_m6_interval_backend.cpp` and the enlarged `scale` corpus in
`tools/meshing_oracle/generate_m6_interval_corpus.py`.

## 1. How this started

D26QMRF1 was wired into the D26QV1 comparison path. Once it was being called,
its certificate rate on Dynamics26's own mesh output measured zero.

    subtract(0, 0)          -> [-denorm_min, +denorm_min]   // outward step
    scalePowerOfTwo(_, -1)  -> Range                        // both ends underflow

A relative coordinate is exactly zero whenever the anchor vertex shares an axis
value with another vertex — an axis-aligned edge, or a face on a coordinate
plane. `scalePowerOfTwo` refused to scale any interval whose endpoint
underflowed, so those cells lost their certificate. Today's product meshing path
is `StructuredHexMesher` over box-compatible CAD, so this was the normal case,
not an edge case.

## 2. The first proposal was wrong

The obvious fix was to widen only when the scaling was inexact, deciding
inexactness from the class of the result:

    exact iff  before == 0, or (after != 0 and after is not subnormal)

reasoning that scalbn is exact while the result stays normal.

**That reasoning is false, and both an exhaustive search and two independent
review agents found the same counterexample.** For

    value = (2^53 - 1) * 2^p,   exponent = -1075 - p

the exact product is `2^-1022 - 2^-1075`: precisely the tie between the largest
subnormal and DBL_MIN. Round-to-nearest-**even** resolves that tie *upward* to
DBL_MIN, whose significand field is zero. DBL_MIN is normal, so the predicate
reports "exact", no outward step is taken, and the returned lower bound sits
`2^-1075` above the true value. The smallest instance is
`scalePowerOfTwo(0x1.fffffffffffffp-1021, -2)`.

Exactness is a property of the operation — whether any significand bit was
shifted below the `2^-1074` grid — not of the class the result happens to land
in after rounding. Rounding can promote a subnormal-range exact value into the
normal class.

## 3. The frozen policy had the same bug

The refuted predicate and the frozen one differ only in what they do when the
result *is* subnormal. Neither widens when the result is normal, so the frozen
implementation returns the same too-high bound on the same inputs. Measured
against the shipped library before any change:

    2^-1021 - 2^-1074  (positive)   k=-1     -> ok  enclosure: LOWER BROKEN
    2^-1021 - 2^-1074  (negative)   k=-1     -> ok  enclosure: UPPER BROKEN
    nextafter(2, 0)                 k=-1023  -> ok  enclosure: LOWER BROKEN
    1 - 2^-53                       k=-1022  -> ok  enclosure: LOWER BROKEN

So this was never a choice between a conservative policy and a faster one. The
frozen policy was unsound: `scalePowerOfTwo` could return `IntervalFailure::None`
with an interval that excludes the value it claims to enclose. Reaching it from
D26QMRF1 needs a cell whose coordinates span ~300 orders of magnitude, so there
is no evidence it ever fired in practice — but a certified filter that can
return a wrong bound at all is not certified.

## 4. Adopted rule

`scalePowerOfTwo` becomes an ordinary D26INT1 primitive: evaluate once under
round-to-nearest, then widen outward once, unconditionally, exactly like
`add`, `subtract` and `multiply`.

    const double lo = std::scalbn(value.lo, exponent);
    const double hi = std::scalbn(value.hi, exponent);
    if (!std::isfinite(lo) || !std::isfinite(hi)) return failure(Range);
    return outwardPrimitive(lo, hi);

It costs one ulp on scalings that were exact and removes the entire class of
bug. `std::scalbn` is IEEE-754 `scaleB`, correctly rounded, and
`probeEnvironment` has already established an IEC 559 environment with gradual
underflow, so the error is at most half an ulp and one outward step always
covers it. Overflow is still refused: there is no finite enclosure to return.

The general lesson is the one the module already encoded everywhere else and
this function alone tried to dodge: **the primitive does not get to decide when
it is exempt from widening.**

## 5. Evidence

| check | result |
| --- | --- |
| enclosure over adversarial (lo, hi, k) triples, exact integer comparison | 2,342,087 checked, **0 violations** |
| min-normal tie family, one member per binade, both signs | 2,090 members, all enclosed |
| D26INT1 corpus, now including underflow and tie rows | 264 cases pass |
| certificate coverage, five populations x 4000 cells | **100%** each |
| certified orders agreeing with D26QMRB1 | 19,870 of 19,870 |

The tie family is measure-zero — one significand pattern per binade — so random
corpora never sample it. A 20-million-pair random scan found nothing. It is
enumerated explicitly in `verifyMinNormalTieBoundary` and in the corpus
generator, and that is the only reason it is covered.

Coverage before and after, for the populations that changed:

| population | before | after |
| --- | --- | --- |
| structured lattice corner | 0.0% | 100.0% |
| three vertices on a plane | 0.0% | 100.0% |
| one shared axis value | 11.6% | 100.0% |
| random coordinates | 100.0% | 100.0% |
| graded anisotropic | 100.0% | 100.0% |

On structured lattice tets the filter now certifies 1944 of 1999 comparisons —
the remainder are exact ties, which correctly have no certificate — and runs
6.33x faster than the exact comparator (15.3 ms against 96.7 ms).

## 6. What changed alongside

- `test_m6_interval_backend` no longer asserts `Range` on subnormal downscale;
  it asserts containment, and enumerates the min-normal tie family.
- The `scale` corpus family now spans the underflow regime. It previously could
  not: the C++ verifier requires `actual.ok()`, and the old policy refused every
  such row, so the generator kept exponents in a range that never underflowed.
- `requireContainsScaled` in `test_m6_filtered_qmr` compared against
  `std::scalbn`-scaled oracle endpoints, which rounds and silently underflows to
  zero at the exponents that corpus reaches — an approximate containment check.
  It now compares on integer mantissas.
- D26INT1-E1 became `unit_m6_filter_coverage`. Once production and proposal are
  the same code, comparing them is a tautology; what is worth keeping is the
  coverage floor and the agreement property.

## 7. Re-qualification still owed

Gate M6-R75..R108 was written against the old policy and should be re-run and
re-recorded. Telemetry that qualification evidence may have captured shifts
substantially: `MeanRatioFilterTelemetry::uncertainRange` and
`QualityVectorTelemetry::filterUnavailableCells` go from dominant to near-zero
on structured output.
