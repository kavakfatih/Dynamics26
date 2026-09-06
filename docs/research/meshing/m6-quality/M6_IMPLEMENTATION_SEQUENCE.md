# M6 Implementation and Qualification Sequence

**Status:** FROZEN EXECUTION PLAN / CODE NOT STARTED  
**Applies to:** D26-M6-FREEZE-1  
**Date:** 2026-09-06

## 1. Rule

Do not implement the whole optimizer in one change.

Each phase must leave:
- build green,
- M1 regression green,
- new M6 evidence attributable to that phase,
- no hidden semantic widening.

## 2. I0 — extract shared exact arithmetic

Production changes:
- create private `src/meshing/internal/exact`,
- move/refactor M1 BigInt/dyadic mechanism,
- reconnect RobustPredicates to it.

No M6 comparator yet.

Tests:
- all M1.7-M1.9 predicates,
- exact-zero corpus,
- invalid inputs,
- telemetry partition,
- Debug/Release.

Exit:
- no M1 semantic change.

Gate focus:
- M6-R109,
- M6-R116,
- M6-R117.

## 3. I1 — D26QMRB1 exact comparator

Production:
- MeanRatioExact,
- fixed exact d/s construction,
- exact pair compare,
- private telemetry.

Test:
- Python Fraction/dyadic oracle,
- all 24 vertex permutations,
- translations/scales,
- regular/sliver/needle/wedge,
- exact ties,
- invalid/degenerate rejection,
- bit-width telemetry.

Gate focus:
- M6-R61..R74,
- M6-R110..R115.

Exit:
- exact comparator authoritative and independently verified.

## 4. I2 — D26INT1 and D26QMRF1

Production:
- interval primitive backend,
- environment guard,
- fixed determinant/S/F trees,
- exact fallback wiring.

Tests:
- primitive exact containment,
- final-only-widen negative control,
- alternate rounding-mode rejection,
- subnormal probes,
- wide exponent range,
- exact fallback agreement.

Gate focus:
- M6-R75..R108.

Exit:
- no certified fast result disagrees with D26QMRB1.

## 5. I3 — D26QV1 and D26QACC1

Production:
- exact sorted quality-vector representation,
- vector comparator,
- local replacement acceptance.

Tests:
- pure max-min tie / deeper-vector improvement,
- variable cell count,
- exact-prefix shorter-wins,
- local-to-global union fixtures,
- aggregate-worse-low-tail negative controls.

Gate focus:
- M6-R49..R60,
- M6-R147..R149.

Exit:
- strict local acceptance theorem is executable.

## 6. I4 — serial local operators O1-O4

Order of subphases:

    I4a Smart smoothing
    I4b 2->3 flip
    I4c full D26QV1 edge-removal DP
    I4d optimization smoothing.

### I4c special rule

Do not implement historical scalar max-min recurrence as production O3 authority.

Implement:

    V[i,j]
      =
    max_D26QV1
      merge(
        V[i,k],
        V[k,j],
        W(i,k,j)
      ).

Cross-check against exhaustive Catalan enumeration.

Max-min remains first-component oracle.

Gate focus:
- M6-R13..R21,
- M6-R37..R40,
- M6-R47..R48,
- M6-R59,
- M6-R161..R163.

Exit:
- O1-O4 serial deterministic fixture corpus green.

## 7. I5 — bounded O5 SPR

Production:
- deterministic cavity selector,
- private branch-and-bound,
- D26QV1 optimistic upper bound,
- count-based resource limits,
- transaction payload.

Tests:
- exhaustive tiny cavity oracle,
- local valley fixtures,
- nested cavity scope,
- BudgetExhausted != ExhaustiveNoImprovement,
- O5 success restarts O1.

Gate focus:
- M6-R38,
- M6-R40..R48,
- M6-R57..R58,
- M6-R164..R166.

Exit:
- bounded SPR qualification claims are correctly scoped.

## 8. I6 — D26QKEY1 cache lifecycle

Production:
- proposal/cavity local exact-key memoization,
- no global persistent cache,
- smoothing invalidation.

Tests:
- cache disabled/enabled identity,
- TetHandle generation versus quality identity,
- stale smoothing cache negative control,
- allocation-failure fallback.

Gate focus:
- M6-R118..R131.

Exit:
- caching proven semantically pure.

## 9. I7 — D26QSCHED1 deterministic rounds

First version may evaluate proposals serially but must use final scheduler data model.

Production:
- snapshot,
- footprints,
- ScheduleKey,
- deterministic greedy conflict selection,
- ordered commit.

Tests:
- enumeration permutation,
- completion-order simulation,
- read/write conflict oracle,
- commit permutation serializability.

Gate focus:
- M6-R132..R146,
- M6-R149..R151.

Exit:
- round result deterministic before concurrency is introduced.

## 10. I8 — D26ACTREF1 then incremental active set

Production:
1. global reference rebuild,
2. incremental dependency-based invalidation.

Tests:
- compare active membership/order after every round,
- deliberate under-invalidation negative control,
- constraint-only invalidation,
- smoothing and topology mutations.

Gate focus:
- M6-R157..R159.

Exit:
- incremental active set is an optimization, not new semantics.

## 11. I9 — parallel proposal evaluation and replay

Only now introduce worker concurrency.

Production:
- parallel private proposal planning,
- same deterministic selection,
- same ordered commit.

Do not parallelize authoritative commit first.

Tests:
- 1/2/4/N worker counts supported by runner,
- Debug/Release,
- repeated runs,
- canonical final fingerprint,
- telemetry partition sanity.

Gate focus:
- M6-R139,
- M6-R145..R146,
- M6-R160,
- M6-R169..R170.

Exit:
- deterministic multicore M6 reference qualified.

## 12. I10 — integration and M7 correlation

After M6 reference qualification:
- define public read-only diagnostics facade,
- integrate with qualified tetra volume mesh state,
- add GUI/reporting separately,
- run solver-conditioning/interpolation/nonlinear/formulation correlation.

Do not derive product thresholds before M7 evidence.

## 13. CTest target plan

Recommended target names:

    unit_m6_exact_qmr
    unit_m6_interval
    unit_m6_filtered_qmr
    unit_m6_quality_vector
    unit_m6_acceptance
    unit_m6_edge_removal
    unit_m6_smoothing
    unit_m6_spr
    unit_m6_quality_key
    unit_m6_scheduler
    unit_m6_active_set
    ver_m6_full_schedule
    ver_m6_thread_replay
    unit_m6_exact_oracle.

Use stable labels:

    m6
    meshing
    exact
    quality
    optimizer
    determinism
    performance.

## 14. Phase status table

| Phase | Deliverable | Source change authorized now? | Qualification before next phase |
|---|---|---:|---|
| I0 | shared exact mechanism | after spec CI | mandatory |
| I1 | D26QMRB1 | after I0 | mandatory |
| I2 | D26INT1/F1 | after I1 | mandatory |
| I3 | D26QV1/QACC1 | after I2 | mandatory |
| I4 | O1-O4 serial | after I3 | mandatory |
| I5 | O5 SPR | after I4 | mandatory |
| I6 | cache | after I5 reference | mandatory |
| I7 | scheduler serial semantics | after I6 | mandatory |
| I8 | active-set optimization | after I7 | mandatory |
| I9 | parallel planning | after I8 | mandatory |
| I10 | product/solver integration | after M6 qualification | M7 evidence |

## 15. Stop rule

If any phase exposes a contradiction with D26-M6-FREEZE-1:
- stop implementation,
- document the contradiction,
- add a new ADR if semantics must change,
- update affected qualification gates,
- only then resume.

Do not silently patch around a frozen-contract mismatch.
