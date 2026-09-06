# M6 Research — Parallel-Round Monotonicity, Active Sets and Termination

Status: PRE-FREEZE RESEARCH
Date: 2026-09-06

## 1. Goal

Close three remaining M6 optimizer questions:

1. Does a round containing several independent strict local improvements strictly improve the global
   D26QV1 quality order?
2. What active-set invalidation rule is authoritative after a local mutation?
3. Can the combined topology+smoothing optimizer terminate without a pairwise quality epsilon?

The answers are:

    yes,
    dependency-based invalidation,
    yes for accepted production states under fixed finite binary64 point coordinates.

## 2. Multiset order

D26QV1 compares the ascending sorted multiset of exact q_MR values.

Variable lengths use the shorter-wins exact-prefix rule, equivalent to padding with +infinity.

Write:

    A > B

for strict D26QV1 improvement.

## 3. Union-compatibility lemma

For any common quality multiset C:

    A > B
      =>
    A union C > B union C.

This was previously established through cumulative bad-count functions.

Intuitively:
the first quality level at which A is better than B remains the first unequal level after adding the
same C to both sides.

## 4. Batch theorem

Suppose a selected round contains m local replacements:

    A_1 -> B_1
    ...
    A_m -> B_m

and every replacement is strict:

    B_i > A_i.

Let U be the unchanged global quality multiset.

Assume the selected winners have a valid changed-cell decomposition so old/new changed tetrahedra are
not double-counted between winners.

Define:

    M_0
      =
    U union A_1 ... union A_m.

For k=1..m:

    M_k
      =
    U
    union B_1 ... B_k
    union A_(k+1) ... A_m.

At step k, all terms except A_k/B_k form one common multiset C_k.

Union compatibility gives:

    M_k > M_(k-1).

By transitivity:

    M_m > M_0.

Therefore:

    QMRVector(global new mesh)
      >
    QMRVector(global old mesh).

## 5. Why scheduler non-conflict matters

The batch theorem is algebraic.

Mesh correctness additionally requires that each local comparison remain valid when the other selected
winners commit.

D26QSCHED1 requires:
- one immutable planning snapshot,
- complete semantic read/write footprints,
- pairwise non-conflicting winners.

Thus winner j cannot change:
- coordinates,
- topology records,
- constraints

read by winner i.

So each strict local D26QV1 proof remains valid through the serializable selected round.

## 6. Stronger non-worsening variant

If:

    B_i >= A_i

for every i and at least one replacement is strict, the same induction gives global strict
improvement.

D26QACC1 is intentionally stronger:

    every committed winner must itself be strict.

This avoids quality-neutral mutations and simplifies replay/termination.

## 7. Non-empty round theorem

A non-empty D26QSCHED1 selected round satisfies:

    GlobalQMRVector_(r+1)
      >
    GlobalQMRVector_r.

An empty selected round satisfies equality and must cause:
- operation-tier escalation,
- or stop.

Repeating an unchanged empty round is forbidden.

## 8. Connectivity-only termination

For fixed:
- finite PointIds,
- fixed coordinates,
- fixed boundary/protected constraints,

the set of legal tetrahedral connectivity states is finite.

Strict global improvement on every non-empty round prevents:
- cycles,
- repeated accepted states.

Therefore connectivity-only rounds terminate.

## 9. Production binary64 smoothing changes the state-space argument

A mathematical smoother over real coordinates has an infinite state space.

Dynamics26 does not store arbitrary real coordinates.

The authoritative candidate/mesh coordinates are finite binary64 bit patterns.

For N fixed PointIds there are finitely many finite canonical binary64 coordinate tuples.

Combining:
- finite coordinate tuples,
- finite valid connectivity states

gives a finite authoritative mesh state space.

## 10. Combined topology+smoothing termination theorem

Assume:
1. fixed finite PointId set,
2. no insertion/deletion,
3. every committed coordinate is finite canonical binary64,
4. every topology/smoothing commit passes exact validity,
5. every commit is strict D26QV1 global improvement,
6. the scheduler stops/escalates on an empty round.

Then the sequence of accepted authoritative mesh states is finite.

Proof:
- finite state space,
- each accepted transition strictly increases global D26QV1,
- hence no state/quality vector can repeat,
- an infinite accepted sequence is impossible.

## 11. Proposal-level termination is separate

The global theorem does not permit an individual search routine to loop forever.

Every proposal generator needs one of:
- finite combinatorial enumeration,
- finite dynamic program,
- count-bounded iteration,
- count-bounded branch-and-bound.

Examples:

    edge-removal DP
      finite polygon triangulation search

    optimization smoothing
      bounded iterations / bounded line-search evaluations

    bounded SPR
      cavity/branch-node budgets.

Budget exhaustion is:

    ResourceOrSearchLimit

not:

    ExhaustiveNoImprovement.

## 12. Exact equality remains exact

No termination rule requires:

    abs(q_new-q_old) < epsilon.

If the stored binary64 candidate produces exactly equal D26QV1:
- it is not committed.

If it produces a strict exact improvement:
- it may commit.

Convergence tolerances can remain proposal-generation heuristics, not authoritative quality equality.

## 13. Active-set dependency oracle

For a committed write set W:

    stale(item)

iff:

    item.R intersects W
      or
    item.W intersects W.

This is the authoritative dependency definition.

It works for:
- topology mutations,
- smoothing coordinate writes,
- constraint writes.

## 14. Why a fixed ring radius is not architecture truth

A 2->3 flip can create opportunities across new faces/edges.

An edge removal can change stars on cavity-boundary edges.

A smoothing move changes:
- all incident tetra quality,
- flip/reconnection quality involving those tets,
- some proposals whose target tetra itself did not move.

Therefore:
- one-ring may be sufficient for some operators,
- insufficient for others,
- larger than necessary for still others.

The semantic footprint is the contract.

A ring/star closure is an implementation over-approximation.

## 15. Reference active-set oracle

Qualification mode:

    D26ACTREF1

recomputes eligible active targets globally from all live finite tetrahedra after each committed round.

It is expensive but simple.

Incremental active-set implementations must match its:
- active membership,
- exact priority order,
- final result.

Over-invalidation/recomputation is acceptable.

Under-invalidation is not.

## 16. Exhaustive versus targeted activation

Reference exhaustive mode may activate every finite tetrahedron.

Production performance mode may activate only a low-quality subset.

D26OPS1 does not freeze a universal q_MR cutoff.

Therefore a targeted run may claim:

    no D26OPS1 improvement found for the activated set

but may not claim global operation-family local optimality unless every eligible tetrahedron was
examined.

## 17. Parallel-round verification

Required small-mesh oracle:
1. enumerate several pairwise non-conflicting strict proposals,
2. compute each local old/new vector,
3. commit selected winners in every permutation,
4. compare global D26QV1.

Require:
- every permutation yields the same semantic mesh fingerprint,
- every non-empty selected round is strict global improvement.

## 18. Negative controls

### Neutral commit

Permit one equal-quality topology mutation.

Expected:
- finite-state strict termination proof no longer applies to that transition.

### Epsilon equality

Use approximate q_MR equality in sorting.

Expected:
- non-transitive ordering fixture breaks deterministic total order.

### Under-invalidated active set

Omit a proposal whose read footprint intersects the committed write set.

Expected:
- D26ACTREF1 mismatch.

### Infinite private search

Remove branch/iteration bound from a synthetic proposal generator.

Expected:
- demonstrates why state-space termination does not prove proposal routine termination.

## 19. Research gates

    M6-R147
      D26QV1 union compatibility is cross-checked with exhaustive finite multiset fixtures

    M6-R148
      batch theorem holds for random sets of strict local replacements

    M6-R149
      non-empty conflict-free round always gives strict global D26QV1 improvement

    M6-R150
      selected-winner commit permutations preserve the same semantic fingerprint

    M6-R151
      empty round escalates/stops and cannot spin unchanged

    M6-R152
      connectivity-only finite-state no-cycle theorem is executable on small exhaustive meshes

    M6-R153
      binary64 topology+smoothing state model is finite under fixed PointIds

    M6-R154
      strict exact smoothing commits cannot repeat an accepted global QualityVector

    M6-R155
      equal D26QV1 smoothing/topology candidates never commit

    M6-R156
      proposal search limits report budget/resource status separately from no-improvement

    M6-R157
      dependency-based invalidation matches global D26ACTREF1 rebuild

    M6-R158
      deliberate under-invalidation is detected by the reference active-set oracle

    M6-R159
      targeted activation does not claim global local optimality

    M6-R160
      Debug/Release and 1/N-thread rounds preserve strict global monotonicity.

## 20. Current conclusion

The M6 round architecture has a strong monotonicity invariant:

    every committed winner strict locally
      +
    selected winners non-conflicting
      =>
    every non-empty round strict globally.

For actual fixed-point-set binary64 production state, this also gives a finite accepted mutation
history.

This closes the main mathematical gap between:
- exact local acceptance,
- deterministic parallel rounds,
- global optimizer progress.

No production implementation is authorized by this document.
