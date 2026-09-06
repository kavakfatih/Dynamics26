# M6 Research — D26OPS1 Optimizer Operation Schedule

Status: DESIGN FROZEN / D26OPS1 first implementation schedule
Date: 2026-09-06

## 1. Goal

Freeze the first Dynamics26 tetra-quality improvement operation repertoire and escalation order without
introducing unvalidated product quality thresholds.

The schedule must:
- preserve the fixed point set,
- preserve qualified boundary/CAD constraints,
- commit only strict D26QV1 improvements,
- favor cheap local operations before strong cavity search,
- revisit cheap operations after a strong reconnection unlocks the local topology,
- remain deterministic under D26QSCHED1.

## 2. Literature direction

Freitag/Ollivier-Gooch show that swapping and smoothing are substantially more effective when combined.

Klingner/Shewchuk use an adaptive aggressive schedule built from:
- smoothing,
- topological transformations,
- stronger/composite operations,
and accept final changed regions only when the mesh quality vector improves.

HXT combines:
- smoothing,
- edge removal,
- Growing SPR Cavity.

Its schedule prioritizes smoothing and edge removal because they are much cheaper, then uses the
strong SPR-style operator as a last resort when the cheap loop stalls.

Recent parallel tetra-improvement work likewise separates broad cheap improvement passes from later
targeted low-quality repair stages.

Dynamics26 adopts these high-level lessons, not source code or fixed numeric thresholds.

## 3. First-scope restrictions

D26OPS1 first scope:
- fixed PointId set,
- interior point movement only,
- fixed CAD/boundary coordinates until M3/M4 projection/provenance are qualified,
- point-set-preserving topology,
- exact D26QACC1 commit acceptance.

Deferred:
- point insertion/deletion for quality,
- edge contraction,
- boundary/CAD vertex motion,
- weighted sliver exudation,
- anisotropic metric-space optimization,
- unrestricted large-cavity exhaustive search.

## 4. Operation family O1 — smart interior smoothing

Purpose:
- cheap coordinate repair,
- improve local placement,
- unlock later topology operations.

Proposal generator may use:
- Laplacian centroid direction,
- safe line search,
- other already-researched smart proposal rules.

Commit requires:
- exact-positive incident tets,
- CAD/constraint legality,
- strict incident-star D26QV1 improvement.

A failed smoothing proposal leaves coordinates unchanged.

## 5. Operation family O2 — elementary 2->3 face flip

For each legal interior face candidate:
- Plan,
- exact validity,
- compare changed local D26QV1,
- Commit only if strict.

Why keep 2->3 explicitly:
general edge removal removes an edge and naturally covers 3->2 / 4->4 / larger edge stars, but does
not replace the need for a face-removal 2->3 move.

## 6. Operation family O3 — general edge-removal DP

For a legal interior edge with N tetrahedra:
- build cyclic link polygon,
- dynamic-program the best legal triangulation under D26QV1 semantics.

Special cases:
- N=3 gives 3->2,
- N=4 gives 4->4,
- N>4 gives general N -> 2N-4 edge removal.

Implementation may use specialized fast paths for small N, but qualification semantics remain the same
as the general exact DP.

This tier is stronger than elementary flip-only hill climbing.

## 7. Operation family O4 — optimization-based interior smoothing

If cheap smart smoothing/topology leave a poor star:
- use the stronger optimization-based smoothing proposal mechanism,
- private iterations may use a differentiable aggregate objective,
- final stored binary64 candidate must pass strict exact D26QV1 acceptance.

This is more expensive than O1 and is therefore later in the cheap/local cycle.

## 8. Cheap/local cycle

Leading order:

    O1 SmartSmoothing
      ->
    O2 Flip23
      ->
    O3 EdgeRemovalDP
      ->
    O4 OptimizationSmoothing.

Run deterministic rounds for the active set.

If any operation commits:
- update/rebuild affected activity,
- continue the cheap/local cycle.

The exact interleaving is D26OPS1 policy and must not depend on thread completion order.

## 9. Strong escalation O5 — bounded SPR

When the cheap/local cycle reaches an empty strict-improvement round for unresolved active targets:

    O5 BoundedSPR

may search a larger fixed cavity.

Private strong search may traverse:
- connectivity states not reachable by one accepted elementary move,
- non-improving intermediate states.

Authoritative mesh remains unchanged until:
- final cavity valid,
- same required boundary,
- protected constraints preserved,
- final D26QV1 strict improvement.

If O5 commits any proposal:
- return to O1.

Reason:
strong reconnection can create new smoothing/edge-removal opportunities.

## 10. Why SPR is last

HXT reports smoothing/edge removal as dramatically cheaper than Growing SPR and uses the strong cavity
operation when the cheap loop stalls.

Klingner/Shewchuk likewise reserve stronger/composite transformations for escaping local optima.

Therefore first Dynamics26 policy is:

    cheap monotone repair
      before
    expensive strong reconnection.

This is an engineering schedule choice, not a theorem that SPR can never be profitable earlier.

## 11. Standalone multi-face removal

Multi-face removal remains useful as:
- research oracle,
- small-cavity comparison.

It is not a separate first D26OPS1 production tier because bounded SPR can cover a broader fixed-cavity
reconnection space.

If later measurements show a large cost gap:
- a middle-tier multi-face operator may be added under a new schedule version.

## 12. Activation policy is separate

D26OPS1 does not define:

    q_MR < constant
      =>
    active.

Reference exhaustive qualification:
- all live finite tetrahedra are eligible targets.

Performance/product modes may use:
- low-tail rank,
- user target,
- future solver-correlated threshold.

Those belong to an activation policy, not operation-order truth.

## 13. Canonical target order

Within one operation family, reference target order:

    PoorTetKey
      =
    (
      exact q_MR ascending,
      CanonicalTetKey
    ).

Proposal conflicts are resolved by D26QSCHED1 ScheduleKey.

No target priority may depend on:
- memory address,
- hash iteration,
- thread id,
- completion time.

## 14. Acceptance is uniform across families

For O1..O5:

    exact validity
      ->
    constraint/provenance legality
      ->
    final changed-region D26QV1
      ->
    strict better
      ->
    commit.

No family receives a private quality threshold that can worsen the global low tail.

## 15. Strong-search resource semantics

Bounded SPR can end as:
- Improved,
- ExhaustiveNoImprovement,
- SearchBudgetExhausted,
- ResourceFailure,
- InvalidCandidate/ConstraintBlocked.

Only:

    ExhaustiveNoImprovement

supports a claim that the searched cavity/range had no better legal final triangulation.

Budget exhaustion does not.

## 16. Stop state

Reference D26OPS1 stops when:
1. cheap/local cycle yields no strict commit,
2. bounded SPR escalation yields no strict commit,
3. no higher enabled operation tier remains.

If SPR is budget-exhausted rather than exhaustive:
- optimizer may stop due policy/resource limit,
- result status records the limit,
- it must not claim strong local optimality.

## 17. Local-optimum claim levels

### L0

No O1 smart-smoothing improvement on activated targets.

### L1

No O1/O2/O3/O4 cheap/local improvement on activated targets.

### L2

L1 plus bounded SPR searched exhaustively over its declared cavity/budget domain with no improvement.

If activation is exhaustive over all live tetrahedra, the claim can be global for the enabled
operation family.

If activation is targeted, the claim is only for the activated set.

## 18. Parallel round behavior

Each operation tier uses D26QSCHED1:
- immutable snapshot,
- parallel proposal evaluation,
- deterministic conflict-free selection,
- ordered commit.

A non-empty selected round is strict global D26QV1 improvement by the batch theorem.

After commit:
- old proposals are discarded,
- active set is rebuilt/replanned.

## 19. Termination

For first-scope fixed finite PointIds and finite binary64 coordinates:
- every accepted O1..O5 commit is strict global D26QV1 improvement,
- authoritative state space is finite.

Thus accepted mutation history is finite.

Proposal routines themselves remain:
- finite exact algorithms,
- or count-bounded private searches.

## 20. Why no universal quality threshold is frozen

M6 research has already shown:
- mean ratio is a strong isotropic baseline,
- q_kappa/angles/radius metrics provide complementary diagnostics,
- FEM conditioning/interpolation/formulation suitability are context-dependent,
- nearly incompressible rubber suitability cannot be reduced to one q_MR cutoff.

Therefore D26OPS1 freezes operation behavior before freezing a product label such as:

    q_MR > 0.2 => FEM-good.

Solver correlation remains required.

## 21. First implementation family summary

| Rank | Family | Point set | Coordinates | Topology | Relative cost intent |
|---|---|---|---|---|---|
| O1 | Smart interior smoothing | preserved | moved | unchanged | low |
| O2 | 2->3 face flip | preserved | unchanged | changed | low |
| O3 | General edge-removal DP | preserved | unchanged | changed | low/medium |
| O4 | Optimization interior smoothing | preserved | moved | unchanged | medium |
| O5 | Bounded SPR | preserved | unchanged | changed | high |

Relative cost labels are architectural intent and require benchmark confirmation.

## 22. Research gates

    M6-R161
      O1/O2/O3/O4 family order replays identically across target enumeration permutations

    M6-R162
      general edge-removal DP special cases agree with 3->2 and 4->4 independent oracles

    M6-R163
      every O1..O5 commit is strict D26QV1 local and global improvement

    M6-R164
      a successful O5 round returns scheduling to O1 and exposes unlock fixtures

    M6-R165
      cheap-cycle stall fixtures are improved by bounded SPR where strong reconnection is necessary

    M6-R166
      bounded-SPR budget exhaustion is never reported as exhaustive no-improvement

    M6-R167
      exhaustive activation and targeted activation produce correctly scoped optimum claims

    M6-R168
      disabled deferred operators cannot silently mutate point count or boundary coordinates

    M6-R169
      1/N-thread D26QSCHED1 execution preserves D26OPS1 canonical final fingerprint

    M6-R170
      full reference schedule terminates on finite binary64 fixed-point-set fixture corpus.

These are pre-freeze gates.

## 23. Current conclusion

Leading first M6 optimizer:

    O1 smart smoothing
      ->
    O2 2->3
      ->
    O3 general edge removal
      ->
    O4 optimization smoothing
      ->
    repeat while progress
      ->
    O5 bounded SPR on stall
      ->
    if progress, return to O1
      ->
    otherwise stop with an explicit claim/status.

This schedule is:
- point-set preserving,
- boundary-conservative,
- exact-quality monotone,
- deterministic-round compatible,
- escalation based rather than threshold hard-coded.

No production optimizer implementation is authorized by this document.
