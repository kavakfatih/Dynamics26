# M6 Early Research — Local Topology Improvement

Status: DESIGN FROZEN INPUT / implementation not started
Date: 2026-09-06

## 1. Engineering question

Once M2/M4 have produced a valid tetrahedralization, which local connectivity changes should
Dynamics26 use to improve FEM-oriented tetrahedral shape without silently changing:
- the point set,
- CAD boundary topology,
- protected interfaces,
- size-field intent,
- geometry provenance?

The answer is not "keep the mesh Delaunay at all costs". Classical experiments show that an
in-sphere/Delaunay face-swap pass can be useful as an intermediate connectivity cleanup, but a final
in-sphere-only objective can leave very poor extremal dihedral angles. Swapping and smoothing are
more effective when combined.

Primary sources:
- Barry Joe, Three-Dimensional Triangulations from Local Transformations, 1989.
- Freitag & Ollivier-Gooch, Tetrahedral Mesh Improvement Using Swapping and Smoothing, 1997.
- Marot & Remacle, Quality tetrahedral mesh generation with HXT, 2020.
- Shewchuk, Two Discrete Optimization Algorithms for the Topological Improvement of Tetrahedral
  Meshes, 2002 research manuscript.

## 2. Topology operations and point-set semantics

The fundamental 3D bistellar/Pachner operations are:
- 1 -> 4: add a vertex inside one tetrahedron,
- 4 -> 1: remove a degree-4 interior vertex,
- 2 -> 3: replace two tetrahedra sharing a face by three sharing a new edge,
- 3 -> 2: inverse of 2 -> 3.

For a quality stage that should initially preserve the point distribution and size field:

    first candidates = 2<->3 and their generalizations.

1<->4 / 4<->1 change the point set and therefore belong behind an explicit refinement/coarsening
policy, not the first M6 quality pass.

A 4->4 operation is useful in practice but is not a fundamental 3D Pachner move; it can be viewed as
a composition of 2->3 followed by 3->2, or as the N=4 case of a general edge-removal cavity.

## 3. 2 -> 3 local reconnection

Current cavity:

    T0 = (a,b,c,d)
    T1 = (a,c,b,e)

with common internal face {a,b,c} and opposite vertices d,e on opposite sides.

Candidate:

    (a,b,d,e)
    (b,c,d,e)
    (c,a,d,e)

sharing new interior edge {d,e}.

A flip is not legal merely because the connectivity can be written.

Reference validation must require:
1. old two-cell cavity is a valid topological 3-ball,
2. shared face is not protected,
3. complementary edge {d,e} is not already present elsewhere in a way that violates the simplicial
   complex,
4. all three candidate tetrahedra have exact non-zero orientation and can be stored positive,
5. candidate internal faces pair exactly twice,
6. candidate exterior boundary is exactly the old cavity boundary,
7. exact total signed/absolute cavity volume is conserved,
8. no CAD/interface/provenance constraint is modified,
9. quality acceptance policy approves the candidate.

Using a generic local-retriangulation validator is safer than relying on a diagram-specific
"convexity epsilon".

## 4. 3 -> 2 local reconnection

Current cavity contains exactly three tetrahedra around an interior edge {d,e} whose link is the
triangle (a,b,c).

Candidate removes {d,e} and creates:

    (a,b,c,d)
    (a,c,b,e)

The move requires:
- the edge star relevant to the move has valence exactly three,
- complementary face {a,b,c} is not already an incompatible simplex,
- same boundary / volume / positive-candidate invariants as 2->3,
- protected feature checks,
- quality improvement.

This is the N=3 edge-removal case.

## 5. 4 -> 4 as a composite quality operation

An interior edge with four incident tetrahedra has a quadrilateral link.

There are two triangulations of that link polygon, so removing the old edge and choosing the other
diagonal yields four replacement tetrahedra.

This is the quality-relevant 4->4 operation.

Dynamics26 should not hard-code a separate topological universe for it. It falls naturally out of
the general edge-removal formulation in EDGE_REMOVAL_DYNAMIC_PROGRAMMING.md.

## 6. Delaunay legality versus FEM quality

For a reconfigurable five-point 2/3 cavity, the in-sphere criterion chooses the locally Delaunay
configuration.

But M6's final objective is FEM-oriented quality, not ordinary Delaunay identity.

Therefore:
- M2 owns Delaunay correctness for construction,
- M6 may deliberately accept a non-Delaunay local reconnection if it improves the versioned quality
  objective and preserves all hard constraints,
- no M6 operation is allowed to weaken M1 exact orientation/topology validity.

This separation is supported experimentally by Freitag & Ollivier-Gooch: in-sphere swapping was
useful as an early pass, but using in-sphere as the final face-swap criterion performed poorly for
extremal angles.

## 7. Cavity quality comparison

For a local cavity C with positive TET4s and primary element score q_MR:

    q_min(C) = min_{T in C} q_MR(T).

A conservative first comparison is:

    q_min(new) > q_min(old).

This cannot sacrifice the old worst element for a better average.

However pure max-min hill climbing can stall even when the second-worst or third-worst tetrahedron can
improve without changing q_min.

The current leading research order is therefore:

    QMRVector(C)
      =
    sort_ascending(q_MR over C)

compared lexicographically from worst to best.

For different cell counts, if one vector is an exact prefix of the other, the shorter vector wins.

Consequences:
- q_min remains the first and strongest component,
- second/third/... worst improvements are visible,
- aggregate means cannot compensate for a worse low-tail element,
- local cavity comparison implies global quality-vector improvement.

Harmonic/inverse-mean-ratio and arithmetic/average objectives remain valuable proposal/smoothing
objectives, but they are not the leading commit-acceptance order.

See QUALITY_ORDER_AND_ACCEPTANCE_POLICY.md.

## 8. Quality ordering is exact but remains separate from geometric validity

M1 exact predicates decide Orient3D validity, degeneracy and topology/cavity truth.

M6 D26QMR1 decides only tetra mean-ratio ordering for already-valid positive tetrahedra.

Authoritative quality order is not an approximate floating epsilon test. D26QMR1 reduces the canonical
binary64 model to exact dyadic/integer comparison.

Displayed/reporting q_MR values may remain floating diagnostics.

Therefore:
- geometry truth remains M1-owned,
- quality ordering is exact M6-owned,
- exact quality ties produce no mutation under D26QACC1,
- no q_MR comparison tolerance leaks into topology predicates.

## 9. Protected topology policy

Early M6 connectivity changes operate on interior tetrahedral topology only.

Blocked initially:
- CAD boundary triangle replacement,
- CAD feature edge replacement,
- seam/interface triangle changes,
- bonded/shared interface connectivity changes,
- topology with unresolved GeometryEntityId provenance.

A future surface optimizer may allow a boundary 2->2 triangle diagonal change only when:
- both triangles belong to the same authoritative CAD Face,
- no protected CAD Edge is changed,
- the new triangle geometry satisfies surface/chord/normal constraints,
- provenance is rebuilt exactly.

This is separate future research.

## 10. Deterministic operation scheduling — superseded by D26QSCHED1

The earlier serial active-queue candidate is superseded by the frozen scheduler contracts:

    QUALITY_KEY_LIFECYCLE_AND_DETERMINISTIC_SCHEDULING.md
    PARALLEL_ROUND_MONOTONICITY_AND_TERMINATION.md
    OPTIMIZER_OPERATION_SCHEDULE.md.

Frozen first semantics:
- immutable round snapshot,
- parallel/private proposal evaluation,
- explicit semantic read/write footprints,
- deterministic ScheduleKey,
- deterministic conflict-free winner set,
- ordered authoritative commit,
- dependency-based active invalidation/rebuild.

Thread completion order, pointer identity and hash iteration never define operation priority.

## 11. Historical schedule — superseded

The earlier flip/edge/smoothing ordering is superseded by frozen D26OPS1.

Authoritative first order:

    O1 smart interior smoothing
      ->
    O2 2->3 face flips
      ->
    O3 general edge-removal DP
      ->
    O4 optimization-based interior smoothing
      ->
    repeat cheap/local cycle while progress
      ->
    O5 bounded SPR on stall
      ->
    if O5 succeeds, return to O1
      ->
    otherwise stop/status.

See OPTIMIZER_OPERATION_SCHEDULE.md.

## 12. Operations deliberately deferred after Design Freeze

Outside the first frozen D26OPS1 scope:
- point insertion/deletion for quality,
- edge contraction,
- boundary/CAD surface vertex motion,
- weighted/regular-Delaunay sliver exudation,
- anisotropic metric-space reconnection,
- unrestricted large-cavity exhaustive reconnection,
- simultaneous authoritative parallel mesh mutation.

Not deferred anymore:
- bounded fixed-cavity SPR is O5,
- parallel proposal planning/evaluation is part of D26QSCHED1.

Parallel authoritative commit may be researched later with deterministic slot/range preassignment.

## 13. Reachability definition of a local optimum

For operation family O:

    N_O(M)
      = one-operation legal neighbors of M.

A mesh is O-locally optimal when no M' in N_O(M) strictly improves the versioned quality order.

Expanding the operation family enlarges the reachable neighborhood.

Therefore:

    elementary-flip local optimum
    != edge-removal local optimum
    != general-cavity local optimum.

See LOCAL_OPTIMIZATION_TRAPS_AND_STRONG_RECONNECTION.md.

## 14. Composite operations must be transactional

A final 4->4 or stronger reconnection can be representable as a sequence of elementary flips while
requiring a non-improving intermediate state.

Dynamics26 must not commit such an intermediate state.

Strong/composite search follows:

    Plan -> Search -> Validate -> Compare final -> Commit/Discard.

This preserves monotone committed mesh quality while permitting non-monotone private exploration.

## 15. Multi-face / SPR final position

Multi-face removal remains a useful independent oracle/middle-tier experiment.

Frozen D26OPS1 does not add it as a separate first operation tier.

Instead:

    O5 = bounded fixed-cavity SPR

is the strong last-resort operator after O1..O4 stall.

SPR searches privately and transactionally, commits only a final exact-valid strict D26QV1
improvement, and returns scheduling to O1 after success.

Unrestricted/exhaustive large-cavity search remains deferred.

## 16. Termination — final frozen distinction

For fixed finite PointIds and finite canonical binary64 stored coordinates:
- connectivity state is finite,
- coordinate state is finite,
- every accepted O1..O5 commit is strict exact global D26QV1 improvement.

Therefore accepted authoritative topology+smoothing mutation history is finite.

Private proposal routines still require finite/count-bounded search semantics.

Point insertion/deletion remains outside this theorem because it changes the fixed point set.

## 17. Exact mean-ratio ordering update

For positive tetrahedra:

    q_MR
      =
    12 (3V)^(2/3)
      / sum_edges l^2.

With:

    D = 6V
    S = sum_edges l^2

we have:

    q_MR^3
      =
    432 D^2/S^3.

Therefore q_MR ordering can be reduced to the exact sign of:

    D_A^2 S_B^3
      -
    D_B^2 S_A^3.

For canonical binary64 coordinates all factors admit exact dyadic evaluation.

This is the leading research path for a deterministic quality comparator without pairwise epsilon
equality.


## 18. Deterministic parallel scheduling research update

Parallel optimization is path-dependent, so thread scheduling must not become an implicit quality
policy.

Leading first parallel architecture:

    immutable optimization-round snapshot
      ->
    build active items in canonical order
      ->
    plan/evaluate proposals in parallel
      ->
    record explicit read/write footprints
      ->
    select a deterministic conflict-free winner set
      ->
    commit winners in canonical order
      ->
    invalidate affected active items
      ->
    begin next round.

The first version deliberately parallelizes expensive proposal work before parallelizing topology
mutation.

### Conflict rule

For proposals A and B with semantic read/write sets:

    conflict(A,B)

when either proposal writes state read or written by the other.

Conceptually:

    W_A intersects (R_B union W_B)
      or
    W_B intersects (R_A union W_A).

The write set includes not only cavity cells but any outside neighbor record that commit must patch.

For first qualification, over-approximating the footprint is preferred to a false non-conflict.

### Deterministic winner selection

Reference scheduling uses a total ScheduleKey and deterministic greedy conflict selection.

Candidate ScheduleKey components:
1. source poor-tet key,
2. fixed operation-family rank,
3. canonical cavity/footprint key,
4. canonical candidate-connectivity key.

Do not use:
- thread id,
- completion time,
- pointer address,
- hash iteration,
- wall-clock budget

as semantic priority.

### Serializability boundary

If selected proposal read/write footprints are pairwise non-conflicting and all proposals were planned
against the same immutable snapshot, their semantic mutations commute over the declared state.

However Dynamics26 does **not** yet claim that one round is identical to the fully sequential
algorithm that regenerates the entire proposal set after every single commit.

The first guarantee target is:
- deterministic round output,
- serializable conflict-free selected mutations,
- thread-count-independent final result for the frozen D26QSCHED1 round algorithm.

See QUALITY_KEY_LIFECYCLE_AND_DETERMINISTIC_SCHEDULING.md.


## 19. D26OPS1 operation schedule pre-freeze

The earlier research-candidate schedule is refined/superseded by the dedicated:

    OPTIMIZER_OPERATION_SCHEDULE.md.

Leading first implementation schedule for fixed point set and fixed CAD boundary coordinates:

    cheap cycle
      1. smart interior smoothing
      2. elementary 2->3 face-flip proposals
      3. general edge-removal DP
           - N=3 covers 3->2
           - N=4 covers 4->4
           - larger legal edge stars use the same exact DP semantics
      4. optimization-based interior smoothing

    repeat cheap cycles while any strict commit occurs

    if cheap cycle stalls:
      5. bounded SPR / strong fixed-cavity reconnection on unresolved targets

    if bounded SPR commits:
      return to cheap cycle

    otherwise:
      stop.

Every committed operation:
- passes exact validity,
- respects protected/CAD/provenance rules,
- strictly improves D26QV1.

Private composite/SPR exploration may traverse non-improving intermediate states but only the final
transaction can commit.

Deferred from first D26OPS1:
- point insertion/deletion,
- edge contraction,
- boundary/CAD surface motion,
- weighted sliver exudation,
- anisotropic metric-space optimization.

Numeric activation thresholds are not frozen by D26OPS1.
