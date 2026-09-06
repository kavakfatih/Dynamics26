# M6 Qualification Gate Matrix

Status: CONTRACT FROZEN / EXECUTABLE EVIDENCE PENDING
Date: 2026-09-06
Applies to: D26-M6-FREEZE-1

## 1. Completeness

EXPERIMENT_PLAN.md defines a complete contiguous gate set:

    M6-R01 .. M6-R170.

Design-freeze repository audit confirmed:
- 170 unique identifiers,
- no missing gate number.

This document groups the gates; it does not mark unimplemented gates as passed.

## 2. Gate groups

| Gates | Domain | Freeze status | Executable evidence |
|---|---|---|---|
| M6-R01..R21 | Core quality/FEM/local-optimization foundations | CONTRACT FROZEN | M6 evidence pending |
| M6-R22..R29 | Multi-metric algebra / conditioning / grading | CONTRACT FROZEN | executable oracle pending |
| M6-R30..R36 | Tetra pathology / angle conditions | CONTRACT FROZEN | fixture suite pending |
| M6-R37..R48 | Local traps / edge removal / strong reconnection | CONTRACT FROZEN | implementation evidence pending |
| M6-R49..R60 | D26QV1 / D26QACC1 | CONTRACT FROZEN | exact-order oracle pending |
| M6-R61..R74 | D26QMR1 exact comparator | CONTRACT FROZEN | M1 arithmetic basis reused; M6 comparator pending |
| M6-R75..R90 | D26QMRF1 filtered comparator | CONTRACT FROZEN | implementation pending |
| M6-R91..R108 | D26INT1 interval backend | CONTRACT FROZEN | implementation pending |
| M6-R109..R124 | D26QMRB1 exact backend / exact-key caching | CONTRACT FROZEN | extraction/integration pending |
| M6-R125..R146 | D26QKEY1 / D26QSCHED1 | CONTRACT FROZEN | implementation pending |
| M6-R147..R170 | Global round monotonicity / D26ACTREF1 / D26OPS1 | CONTRACT FROZEN | implementation pending |

## 3. Recommended qualification sequence

    Q0 M1 exact-validity reuse checks
      ->
    Q1 D26QMRB1 exact comparator
      ->
    Q2 D26INT1 + D26QMRF1
      ->
    Q3 D26QV1/D26QACC1
      ->
    Q4 O1/O2/O3/O4 serial reference operations, including full-D26QV1 O3 edge DP
      ->
    Q5 bounded O5 SPR
      ->
    Q6 D26QKEY1 cache equivalence
      ->
    Q7 D26QSCHED1 deterministic rounds
      ->
    Q8 D26ACTREF1 incremental-active-set equivalence
      ->
    Q9 1/N-thread Debug/Release canonical fingerprint qualification
      ->
    Q10 M7 solver-correlation campaign.

## 4. Claims prohibited before gates pass

Before relevant executable evidence passes, Dynamics26 must not claim:
- M6 optimizer qualified,
- parallel optimizer deterministic,
- bounded SPR exhaustive beyond its declared searched domain,
- universal FEM-good q_MR threshold,
- rubber-ready tetra formulation based on geometry alone.

## 5. M1 reuse boundary

M1 already qualifies canonical finite binary64 geometry input, exact Orient3D truth, the internal dyadic
BigInt mechanism and robust-predicate replay infrastructure.

M6 may reuse mechanisms, but every new M6 expression/cache/schedule/mutation path requires M6-specific
qualification.

## 6. Implementation mapping

Repository-specific implementation ownership and phase exits are frozen in:

    M6_IMPLEMENTATION_SPEC.md
    M6_IMPLEMENTATION_SEQUENCE.md.

These documents map the existing femcae_meshing/M1/M2 foundation to the gate groups without changing
M6 mathematical semantics.

## 7. Maintenance

No gate may be silently removed or weakened.

A future semantic change must update M6_DESIGN_FREEZE.md, add/supersede an ADR, update affected gates
and preserve traceability in EXPERIMENT_PLAN.md.

The contiguous M6-R01..R170 numbering is frozen for D26-M6-FREEZE-1.
