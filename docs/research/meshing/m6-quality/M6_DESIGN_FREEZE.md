# M6 Design Freeze — Tetra Quality and Deterministic Optimizer

Program: Dynamics26 Original Meshing Engine
Status: RESEARCH COMPLETE / DESIGN FROZEN / IMPLEMENTATION NOT QUALIFIED
Date: 2026-09-06
Freeze ID: D26-M6-FREEZE-1

## 1. Purpose

This document is the authoritative closeout of the M6 tetra-quality / FEM-suitability / local-optimizer
research package.

It freezes mathematical quality-order semantics, numerical comparator architecture, exact/filtered
backend roles, local optimization repertoire, cache/state lifetime rules, deterministic parallel
scheduling, active-set semantics, termination/status boundaries and deferred scope.

It does not claim that production M6 code exists or that M6-R01..M6-R170 executable qualification has
passed.

## 2. Precedence

If an older research paragraph conflicts with this document:

    M6_DESIGN_FREEZE.md
      ->
    ACCEPTED M6 ADRs in DECISION_LOG.md
      ->
    frozen detailed contract documents
      ->
    historical research-candidate text.

Any future change to a frozen invariant requires a new ADR, explicit supersession and affected
qualification-plan/policy-version updates.

## 3. Frozen scope

First M6 implementation scope:
- fixed finite PointId set,
- finite canonical binary64 coordinates,
- fixed CAD/boundary coordinates,
- interior point smoothing,
- point-set-preserving topology improvement,
- exact positive tetra validity,
- deterministic local optimizer rounds.

Not in first scope:
- point insertion/deletion,
- edge contraction,
- CAD boundary/surface motion,
- weighted sliver exudation,
- anisotropic metric-space optimization,
- unrestricted large-cavity exhaustive search.

## 4. Frozen quality-layer separation

Dynamics26 keeps separate:
1. topology/positive-orientation validity,
2. initial isotropic shape quality,
3. sizing/gradation,
4. FEM numerical context,
5. nonlinear current-configuration distortion,
6. formulation suitability.

A geometrically good tetra does not imply locking-free nearly incompressible TET4, good global
conditioning, adequate interpolation or rubber-ready formulation.

## 5. D26QMR1 — exact primary element order

For positive TET4:

    q_MR = 12 (3V)^(2/3) / S

where S is the sum of six squared edge lengths.

With D=6V:

    q_MR^3 = 432 D^2/S^3.

For tetra A,B the sign of:

    D_A^2 S_B^3 - D_B^2 S_A^3

defines exact q_MR order for the canonical binary64 coordinate model.

D26QMR1 is an exact ordering predicate, not an exact irrational scalar evaluator. It runs only after
exact positive validity is established. No pairwise q_MR epsilon is used.

## 6. D26QV1 and D26QACC1

For changed region C:

    QMRVector(C) = sort ascending exact q_MR values.

Compare worst-to-best lexicographically. If one vector is an exact prefix of the other, shorter wins.

Every authoritative M6 mutation requires:

    exact geometry/topology validity
      ->
    protected/CAD/provenance legality
      ->
    strict D26QV1 improvement
      ->
    commit.

Exact quality tie means no mutation. Aggregate/smooth objectives may generate proposals but cannot
override D26QACC1.

## 7. Local-to-global / batch monotonicity

D26QV1 is compatible with multiset union.

If each selected non-conflicting winner satisfies:

    B_i > A_i,

then repeated union compatibility gives:

    GlobalNew > GlobalOld.

Thus every non-empty D26QSCHED1 committed round is strict global D26QV1 improvement.

## 8. D26QMRF1 / D26INT1

First fast comparator targets:

    F(A,B)=D_A^2 S_B^3-D_B^2 S_A^3.

Architecture:

    dynamic certified interval
      ->
    exact D26QMR1 fallback.

Fast outcomes are Less, Greater or Uncertain. Fast Equal is forbidden.

First interval backend D26INT1 uses:
- FE_TONEAREST primitive evaluation,
- adjacent-representable outward widening after every arithmetic primitive,
- binary radix-2 / 53-bit double assumptions,
- qualified gradual-subnormal behavior,
- frozen non-contracted expression tree,
- exact fallback on environment/range uncertainty.

Directed-rounding and EFT/FMA backends remain optional future optimizations.

## 9. D26QMRB1

First exact fallback reuses the qualified M1 mechanism:

    binary64 dyadic decode
      ->
    per-call common exponent
      ->
    internal signed BigInt
      ->
    exact integer arithmetic.

General gcd/division is not required; independent tetra scales cancel in d^2/s^3.

BigInt remains internal and must not leak into installed/public API.

## 10. D26QKEY1

Numeric tetra quality identity is coordinate-state identity, not TetHandle storage identity.

For fixed coordinate state:

    CanonicalTetKey = sorted four PointIds.

TetHandle slot/generation, visitEpoch and adjacency are not numeric q_MR identity.

Accepted smoothing invalidates every incident tetra quality key. Constraint/provenance changes may
leave numeric q_MR unchanged while invalidating proposal legality.

First mutable exact caches are proposal/cavity/pass/worker-local. No permanent global exact-key cache
is frozen.

## 11. D26QSCHED1

Frozen first scheduling architecture:

    immutable round snapshot
      ->
    parallel private proposal planning/evaluation
      ->
    explicit semantic read/write footprints
      ->
    deterministic ScheduleKey
      ->
    deterministic conflict-free winner selection
      ->
    ordered revalidation/commit
      ->
    affected activity rebuild.

Thread id, pointer address, completion time, unordered hash traversal and wall-clock arrival are never
semantic priority.

Proposals conflict on write/write or write/read overlap. Topology write footprints include exterior
adjacency records patched by commit.

## 12. D26ACTREF1

Reference active-set qualification oracle:

    rebuild all eligible active targets from all live finite tetrahedra after each committed round.

Incremental invalidation must match the reference for the same activation policy.

No fixed one-ring/two-ring radius is architectural truth; semantic dependency owns invalidation.

## 13. D26OPS1

Frozen first operation schedule:

    O1 Smart interior smoothing
      ->
    O2 2->3 face flip
      ->
    O3 General edge-removal DP
      ->
    O4 Optimization-based interior smoothing
      ->
    repeat while strict progress exists
      ->
    O5 Bounded fixed-cavity SPR on stall.

If O5 commits, return to O1. Otherwise stop with explicit result/status.

General edge removal covers N=3 -> 3->2, N=4 -> 4->4 and larger legal edge stars.

O5 is transactional: private exploration may cross non-improving states; only a final exact-valid
strict D26QV1 improvement commits. Full D26QV1 is authoritative; max-min remains a first-component
verification oracle.

## 14. Termination and statuses

For fixed finite PointIds, finite canonical binary64 committed coordinates and finite valid
connectivity state, every accepted commit is strict global D26QV1 improvement.

Therefore accepted authoritative mutation history is finite.

Private search routines still require finite enumeration or count-bounded search.

The optimizer distinguishes Improved, ExhaustiveNoImprovement, SearchBudgetExhausted, ResourceFailure
and ConstraintBlocked/InvalidCandidate. Budget/resource failure is never geometry failure or quality
equality.

## 15. Threshold boundary

D26OPS1 does not freeze a universal q_MR cutoff.

Reference exhaustive qualification may activate every live finite tetrahedron. Targeted modes must
scope optimum claims to the activated set.

M6 freezes metric roles, not generic claims such as q_MR > constant => FEM-good. Solver-correlated
thresholds remain M7 evidence. Nearly incompressible rubber suitability remains a formulation/solver
question in addition to geometry.

## 16. Deferred scope

Deferred from first implementation:
- point insertion/deletion,
- edge contraction,
- CAD boundary vertex movement/projection,
- weighted regular-Delaunay sliver exudation,
- anisotropic metric-space quality optimization,
- unrestricted large-cavity exhaustive SPR,
- global persistent exact-key cache,
- semi-static D26QMR filter,
- EFT/FMA interval backend,
- simultaneous authoritative parallel commit.

Future parallel commit requires deterministic storage-range preassignment and equivalence to ordered
commit reference.

## 17. Qualification boundary

M6-R01..M6-R170 are complete and contiguous in EXPERIMENT_PLAN.md.

At freeze:
- qualification contract: FROZEN,
- executable M6 implementation evidence: PENDING,
- production M6 qualification: NOT CLAIMED.

Existing M1 exact-predicate evidence supports reused mechanisms but does not automatically qualify new
M6 expressions, caches, schedules or mutations.

See M6_QUALIFICATION_GATE_MATRIX.md.

## 18. Closeout

M6 status:

    RESEARCH COMPLETE
    DESIGN FROZEN
    IMPLEMENTATION NOT STARTED / NOT QUALIFIED.

Next action is implementation specification and qualification-harness planning, not unconstrained M6
architecture research.
