# M6 Research — Local Optimization Traps and Strong Reconnection

Status: RESEARCHING / architecture-and-verification candidate
Date: 2026-09-06

## 1. Engineering question

Why can a valid tetrahedral mesh remain trapped with bad elements even after:
- 2->3 / 3->2 flips,
- edge removal,
- smart smoothing,

and what stronger point-set-preserving reconnection architecture should Dynamics26 research before
implementation?

This document separates:
1. **operation-neighborhood strength**,
2. **quality hill climbing**,
3. **cavity selection**,
4. **optimal retriangulation inside a fixed cavity**,
5. **transactional valley crossing**,
6. **termination and deterministic scheduling**.

Primary literature:
- M6-OPT-004 — Shewchuk edge/multi-face removal,
- M6-OPT-005 — HXT / Growing SPR Cavity,
- M6-OPT-008 — Klingner & Shewchuk aggressive improvement,
- M6-OPT-009 / M6-OPT-010 — Small Polyhedron Reconnection,
- M6-OPT-011 — optimized branch-and-bound SPR.

No external implementation tables or source code are copied into this design.

## 2. Mesh improvement as a reachability graph

Let a mesh state be:

    M.

Let an allowed operation family be:

    O.

Define the operation neighborhood:

    N_O(M)
      = { M' | M' is reachable from M by one legal operation in O }.

Let the versioned quality ordering be:

    M' ≻_Q M

when M' is strictly better under the Dynamics26 quality comparison policy.

Then M is an **O-local optimum** if:

    there is no M' in N_O(M)
    such that M' ≻_Q M.

This definition explains local traps precisely.

If:

    O1 subset O2,

then:

    N_O1(M) subset N_O2(M).

Therefore a mesh can be locally optimal for elementary flips and still be improvable by edge removal,
multi-face removal or a larger cavity reconnection.

The objective function has not changed.

The **reachable neighborhood** has changed.

## 3. Strict hill climbing creates valleys

Suppose three states satisfy:

    Q(M0) > Q(M1)
    Q(M2) > Q(M0).

If the only elementary path is:

    M0 -> M1 -> M2,

a strict hill climber rejects the first step and can never reach M2.

This is an optimization valley.

The classical 4->4 operation gives the important topological pattern:

    4->4
      = 2->3
        followed by
        3->2

for an appropriate cavity.

If the intermediate 2->3 triangulation is worse, a strict 2<->3 hill climber cannot reach an improved
final 4->4 state.

A direct cavity operation can evaluate the final state without committing the bad intermediate state.

### Dynamics26 consequence

Composite/local-cavity search must be:

    Plan
    -> Search
    -> Validate
    -> Compare final state
    -> Commit or discard

not:

    commit a bad intermediate mesh
    -> hope a later operation repairs it.

This reuses the transactional engineering philosophy already frozen for M2 cavity mutation.

## 4. Point-set-preserving operation hierarchy

For the first M6 quality stage, keep the point set fixed.

A useful hierarchy is:

### Level A — elementary bistellar connectivity

    2->3
    3->2.

Strength:
- very small cavity,
- cheap,
- easy to validate.

Weakness:
- very small reachable neighborhood,
- frequent local traps.

### Level B — general edge removal

For an interior edge with N incident tetrahedra:

    N -> 2N-4.

It searches every triangulation of the edge-link polygon that is geometrically valid under the
Dynamics26 cavity validator.

Special cases include:

    N=3 -> 3->2
    N=4 -> 4->4.

Edge removal therefore supersedes an important subset of elementary/composite flips.

But it does **not** cover every point-set-preserving reconnection.

In particular, 2->3 belongs naturally to the inverse multi-face family.

### Level C — multi-face removal

An m-face removal is the inverse family of edge removal.

Classical count:

    2m tetrahedra
      ->
    m+2 tetrahedra.

Its simplest member is:

    m=1
      =>
    2->3.

The operation chooses a connected set of faces sandwiched between two apex vertices and replaces them
with a cavity containing the complementary edge.

Shewchuk derives an optimal targeted multi-face-removal search for a selected face.

### Level D — multi-face retriangulation

This combines:
- multi-face removal,
- edge removal,

or equivalently traverses a broader sequence of 4->4-like changes in the sandwiched triangulation.

Its importance is conceptual:

    one restricted cavity optimum
    can still be separated from a better one by a quality valley.

### Level E — fixed-cavity SPR

Small Polyhedron Reconnection searches the tetrahedralizations of a general small polyhedral cavity,
subject to its fixed boundary and protected internal constraints.

Within a fixed cavity and fixed point set, this operation subsumes the connectivity families above.

It does **not** subsume:
- smoothing,
- point insertion/deletion,
- CAD boundary motion,
- remeshing/state transfer.

So "mother of all flips" should be read as:

    mother of fixed-cavity point-set-preserving retriangulations,

not as a universal mesh optimizer.

## 5. Edge-removal optimality is strong but restricted

For edge:

    e={a,b}

with ring vertices:

    v0,...,v_(N-1),

Dynamics26 already derived a max-min dynamic program over link-polygon triangulations.

Let:

    T_e

be the set of geometrically valid retriangulations obtainable by removing e.

Then edge-removal research solves:

    q*_e
      =
    max over R in T_e
      min over t in R
        q_MR(t).

If the DP and candidate validator are correct, this is an exact optimum over **T_e**.

But:

    T_e
    !=
    all triangulations of the surrounding mesh.

Therefore:

    edge-removal optimum
    !=
    cavity-global optimum
    !=
    mesh-global optimum.

This is the precise limit of the DP contract.

## 6. Multi-face removal adds a complementary search direction

The inverse character matters.

Edge removal asks:

    "Can quality improve if this edge disappears?"

Multi-face removal asks:

    "Can quality improve if this selected internal face-family disappears
     and the complementary edge appears?"

These are not redundant local questions.

Klingner/Shewchuk report additional gains from face-removal operations, although the practical
cost/benefit depends on what other strong transformations are available.

HXT deliberately omits multi-face removal/retriangulation because its Growing SPR Cavity covers their
final connectivity possibilities through a more general operator.

### Dynamics26 implication

Do not rush to production multi-face removal merely because it exists.

Research it as:
- an independent oracle/cross-check for small cavities,
- a way to prove edge-removal neighborhood incompleteness,
- a possible cheaper middle tier if SPR proves too expensive.

## 7. Fixed-cavity SPR objective

Let a cavity C have:
- a fixed closed triangular boundary,
- a fixed finite vertex set,
- optional protected internal vertices/edges/faces.

Let:

    Tri(C)

be the set of all legal tetrahedralizations satisfying those constraints.

For primary quality:

    q(T)
      =
    min over t in T
      q_MR(t).

Define:

    T*_C
      =
    arg max over T in Tri(C)
      q(T).

An exhaustive SPR branch-and-bound search can return T*_C.

This statement is only about the **fixed cavity**.

It does not prove the cavity itself was the right cavity to choose.

## 8. First-principles branch-and-bound bound

Suppose a partial triangulation P has already inserted tetrahedra:

    t1,...,tk.

Define:

    U(P)
      =
    min_i q_MR(t_i).

Any completion of P can only add more tetrahedra.

Therefore the worst quality of any completion satisfies:

    q(completion)
      <=
    U(P).

If the best complete solution found so far has quality:

    q_best,

and:

    U(P) <= q_best,

then the branch cannot produce a strictly better **max-min-only** solution and may be pruned.

This is a simple admissible branch-and-bound upper bound for the max-min objective.

Important later research update:
- equality pruning is **not** sufficient for a full lexicographic quality-vector objective,
- a branch tying q_best at the worst element can still improve the second/third worst elements.

See QUALITY_ORDER_AND_ACCEPTANCE_POLICY.md for the stronger lexicographic bound.

## 9. Why the first SPR objective should remain max-min

The current M6 local QualityKey research includes possible secondary terms such as:
- harmonic mean q_MR,
- arithmetic mean q_MR,
- cell count.

Those are useful for local hill-climbing experiments.

But a branch-and-bound proof is simplest for:

    maximize minimum q_MR.

If the search immediately uses a multi-component QualityKey, pruning requires an admissible bound for
every lexicographic component.

That is possible in principle but creates unnecessary research complexity.

### Leading research candidate

For first SPR experiments:

1. maximize worst q_MR exactly within the cavity,
2. among equal-primary optima, use a deterministic canonical topology tie,
3. evaluate aggregate/angle diagnostics after the primary optimum,
4. research secondary objective optimization separately.

This preserves a transparent correctness oracle.

No production choice is frozen yet.

## 10. Search optimality and cavity selection are separate problems

There are two nested optimization problems.

### Inner problem

For one fixed cavity:

    find best triangulation of C.

SPR can solve this exactly if its search is exhaustive.

### Outer problem

Choose which cavity to optimize:

    C1, C2, C3, ...

This is a different combinatorial search problem.

A perfectly solved small cavity can still leave a poor mesh if a larger/different cavity is needed.

Therefore report separately:

    cavity_search_policy
    retriangulation_search_policy.

Never report:

    "SPR optimal"

without specifying:

    optimal for which cavity?

## 11. Growing-cavity research

HXT's Growing SPR Cavity addresses the outer problem heuristically:

1. start from a bad tetrahedron,
2. grow the cavity by adding nearby connectivity,
3. run SPR after each growth step,
4. stop when an improving retriangulation is found or an effort/size budget is exhausted.

This is important evidence that:
- many local traps require a larger cavity,
- full large-cavity SPR is too expensive to use indiscriminately,
- cheap operations should run first.

But HXT's exact:
- cavity-growth heuristic,
- point cap,
- quality threshold,
- search effort limit

are implementation choices, not Dynamics26 authority.

Dynamics26 must derive and benchmark its own policy.

## 12. Computational complexity boundary

For an edge star, Dynamics26 DP is polynomial in edge valence:

    O(N^3) time
    O(N^2) memory.

General 3D cavity triangulation is fundamentally much harder.

The SPR literature treats the search as branch-and-bound over an enormous triangulation space; even
deciding triangulability of a general polyhedron belongs to hard computational-geometry territory.

Therefore strong reconnection should be a **last-resort bounded search**, not the default operation
for every element.

This naturally motivates a cheap-to-expensive escalation ladder.

## 13. Dynamics26 escalation ladder — research candidate

Not frozen for implementation.

For a low-tail interior tetrahedron in deterministic order:

### Tier 0 — hard guards

Require:
- valid current topology,
- exact positive orientation,
- unprotected interior scope,
- resolved provenance.

### Tier 1 — cheap geometry/connectivity

Try:
- smart Laplacian proposals on free incident vertices,
- elementary 2->3 where useful,
- edge removal on incident interior edges.

If any strict improvement commits:
- update the affected neighborhood,
- return to Tier 1 locally.

### Tier 2 — stronger local optimization

Try:
- optimization-based smoothing,
- edge removal again after coordinates changed,
- research multi-face removal / multi-face retriangulation.

Why repeat edge removal?

Because smoothing changes the candidate tetra qualities and can make a previously rejected
reconnection beneficial.

### Tier 3 — bounded strong cavity reconnection

For residual bad elements:
- construct a deterministic growing cavity,
- invoke bounded SPR branch-and-bound,
- accept only a final validated strict improvement.

After a successful strong reconnection:
- restart cheap smoothing/reconnection on the affected neighborhood.

### Tier 4 — deferred point-changing operations

Only after M5/M7 contracts exist:
- vertex insertion,
- vertex deletion/contraction,
- boundary changes.

These can escape fixed-point-set traps, but they also change:
- size-field realization,
- node identity,
- solver DOF count,
- provenance.

They are explicitly outside the first M6 implementation candidate.

## 14. Why smoothing must be interleaved with topology

Smoothing changes coordinates but not connectivity.

Reconnection changes connectivity but not coordinates.

A topology state that is locally optimal at coordinates X may cease to be locally optimal after a
small accepted vertex relocation.

Conversely, a vertex star that is locally stuck for smoothing can gain a new feasible/quality
landscape after connectivity changes.

So the search is not:

    topology OR smoothing.

It is an alternating optimization problem over:

    connectivity
    x
    vertex coordinates.

The literature repeatedly finds that combinations outperform either class alone.

## 15. Transactional valley crossing

Strong/composite operations may need intermediate states that are not improvements.

Those states must never become externally visible mesh states.

Research contract:

    BeginQualityTransaction
      snapshot logical cavity state
      build temporary candidate sequence
      permit temporary internal quality decrease
      preserve exact candidate validity
      compute final state
      validate final cavity boundary/provenance
      compare final QualityKey
      if strict improvement:
          Commit
      else:
          Discard
    EndQualityTransaction

This is especially important for future:
- composite 4->4 paths,
- multi-face retriangulation,
- vertex insertion + smoothing + reconnection.

No post-hoc "repair after commit" is allowed.

## 16. Termination — fixed coordinates and fixed point set

Assume:
- finite site set,
- fixed coordinates,
- fixed boundary/protected constraints,
- point-set-preserving connectivity operations only.

There are only finitely many possible tetrahedra:

    at most binomial(n,4),

hence finitely many possible tetrahedralizations.

If every committed operation strictly improves a deterministic quality ordering:

    M_(k+1) ≻_Q M_k,

then no mesh state can repeat.

Therefore the connectivity-only hill-climbing process terminates after finitely many accepted
mutations.

This is a mathematical termination argument, not a performance bound.

The number of possible states can still be enormous.

## 17. Termination — smoothing breaks the finite-state proof

Vertex smoothing changes floating coordinates continuously.

Therefore the connectivity-only finite-state argument no longer applies.

An infinite sequence of ever-smaller accepted improvements is conceptually possible.

Smoothing needs explicit optimizer stop semantics, such as versioned combinations of:
- minimum quality improvement,
- minimum coordinate displacement,
- maximum line-search iterations,
- maximum local/global passes,
- stalled active set.

No numerical value is accepted yet.

These are optimizer convergence controls.

They are not geometry predicates.

## 18. Termination — point insertion/deletion changes the state space again

If vertex insertion is unrestricted, even the number of sites is no longer fixed.

Strict quality improvement alone does not provide a useful resource bound.

Future point-changing optimization therefore requires:
- size-field policy,
- point-count/resource budgets,
- insertion/deletion provenance,
- rollback,
- deterministic point identity,
- solver-workflow consequences.

This is another reason to defer it from first M6 implementation.

## 19. Equal-quality ties and cycles

If a mutation with exactly equal quality is allowed merely because its connectivity key is different,
two or more equal-quality states could cycle unless a second monotone ordering is defined.

Simplest first research policy:

    equal primary quality
      =>
    no mutation.

Alternative future policy:

    accept equal quality only if a globally defined canonical topology rank strictly improves.

That rank must itself be:
- deterministic,
- well-founded over the finite connectivity state space,
- versioned.

Do not use container iteration order as a tie breaker.

## 20. Floating quality comparison and strictness

Exact predicates decide:
- orientation,
- intersection/topological legality,
- cavity validity.

Floating q_MR decides:
- optimization ranking.

If two quality values are nearly indistinguishable, the optimizer may use a versioned comparison
margin or no-op tie policy.

That margin:
- must be dimensionless for q_MR,
- must never leak into Orient3D/topology truth,
- must be tested for deterministic Debug/Release behavior.

No margin value is selected here.

## 21. Active-set scheduling

A full sweep over every possible cavity is wasteful.

Research candidate active item:

    PoorTetKey
      =
    (
      q_MR ascending,
      CanonicalTetKey
    ).

After a mutation:
- remove invalidated local candidates,
- recompute quality only in affected stars/cavities,
- enqueue newly poor/changed tetrahedra deterministically.

For each poor tetrahedron:
- operation types have fixed versioned order,
- edges/faces/vertices are enumerated by canonical keys,
- successful mutation restarts the affected neighborhood.

This gives reproducibility independent of hash/container order.

## 22. Effort budgets are not quality truth

Strong cavity search needs operational limits.

Possible future budget dimensions:
- cavity vertex count,
- cavity tetra count,
- branch-and-bound nodes explored,
- predicate/quality evaluations,
- wall-clock budget only for non-deterministic performance modes.

For deterministic reference qualification, prefer count-based budgets over wall time.

If a budget is exhausted:

    StrongSearchBudgetExceeded

is not:

    NoBetterTriangulationExists.

This distinction must be represented in telemetry/results.

## 23. Strong-search result semantics

A bounded strong reconnection should distinguish at least:

### Improved

A validated better final cavity was committed.

### ExhaustiveNoImprovement

The entire allowed triangulation search for the fixed cavity was exhausted and no better state exists
under the objective.

### BudgetExhausted

Search stopped before completeness.

### InvalidCavity

Boundary/protected/topological contract failed.

### NoTriangulation

For a fixed cavity search proven exhaustive, no legal tetrahedralization satisfying constraints exists.

These states are materially different for debugging and future qualification.

## 24. Search monotonicity versus exploration non-monotonicity

Committed mesh history should be monotone:

    Q(M_0)
      <
    Q(M_1)
      <
    Q(M_2)
      < ...

under the versioned accepted quality order.

Temporary search states need not be monotone.

This is the correct way to cross valleys:

    non-monotone private exploration
    +
    monotone public commits.

That is the central M6 transactional optimization principle.

## 25. Point-set preservation and size field

The current strong-reconnection research uses exactly the existing cavity vertices.

Therefore:
- no new point spacing is introduced,
- no point is deleted,
- the M5 target-size field is not directly resampled.

However connectivity can still alter:
- local edge lengths,
- edge directions,
- grading relationships.

So even point-set-preserving reconnection must eventually recheck M5 sizing/gradation acceptance.

Point-set preservation is not equivalent to size-field preservation.

## 26. Boundary and CAD constraints

First M6 strong reconnection remains interior-only.

A cavity touching:
- CAD Face triangulation,
- CAD Edge,
- CAD Vertex,
- protected bonded/shared interface

is blocked unless a future boundary-specific policy explicitly authorizes modification.

For a future constrained SPR cavity, protected triangles/edges can conceptually be required in every
candidate triangulation.

But CAD geometric fidelity requires M3/M4 authoritative projection/provenance contracts first.

## 27. Relation to sliver treatment

A strong fixed-point reconnection can eliminate some slivers by finding a better connectivity around
the same sites.

It cannot guarantee all slivers disappear.

Some point configurations may have no sufficiently good tetrahedralization without:
- moving points,
- changing weights/regular triangulation,
- inserting/deleting points.

Therefore the architecture remains:

    strong unweighted reconnection
      before
    finite-weight sliver treatment research.

D26LIFT1 remains unrelated to quality weights.

## 28. Proposed Dynamics26 research architecture

The leading non-production architecture is:

    valid fixed-boundary mesh
      ->
    deterministic low-tail active set
      ->
    cheap smoothing + elementary/edge reconnection
      ->
    optimization smoothing
      ->
    repeat cheap reconnection
      ->
    bounded strong cavity reconnection for residual traps
      ->
    restart cheap local passes after every strong success
      ->
    final exact topology validation
      ->
    quality-vector / solver-correlation report.

Deferred:
- point insertion/deletion,
- boundary modification,
- finite weighted regular triangulation,
- parallel strong search.

## 29. Verification questions before any implementation freeze

Research fixtures must answer:

1. Can a 4->4 final improvement exist while every required elementary first step is non-improving?
2. Does edge-removal DP find the optimum over every legal triangulation of its link polygon?
3. Can a cavity exist where no incident edge removal improves quality but multi-face/SPR does?
4. Does fixed-cavity exhaustive SPR match an independent enumeration oracle for small cavities?
5. Does branch-and-bound pruning preserve the true max-min optimum?
6. Do growing-cavity policies expose cases solved only after cavity expansion?
7. Does every accepted connectivity-only sequence terminate without cycles?
8. Can equal-quality tie policies be shown cycle-free?
9. Do count-based effort limits reproduce identical results across runs/build modes?
10. Does a successful strong reconnection unlock further cheap smoothing/edge removal?
11. Does a point-set-preserving reconnection ever violate future size/gradation acceptance?
12. Are budget exhaustion and exhaustive no-improvement reported distinctly?

## 30. Current conclusion

The next Dynamics26 implementation should **not** begin with SPR.

The research supports a staged architecture:

    cheap deterministic operations first
      ->
    stronger fixed-point cavity search only for residual local traps.

The key mathematical separation is:

    operation local optimum
    != fixed-cavity optimum
    != global mesh optimum.

And the key engineering separation is:

    private search may cross quality valleys
    while committed mesh states remain strictly improving and valid.

This document authorizes no production M6 code.


## 31. Quality-order research update — lexicographic strong search

The initial SPR research deliberately used max-min because its pruning bound is simple.

M6 quality-order research now derives a stronger objective:

    QMRVector(T)
      =
    sorted q_MR values, worst to best,

with exact-prefix shorter-vector semantics.

For a partial triangulation P:

    Q_upper(P)
      =
    QMRVector(P) padded with +infinity

is an optimistic lexicographic upper bound because any completion only adds finite-quality
tetrahedra.

Therefore if:

    Q_upper(P)
      <=_lex
    Q_best,

the branch may be pruned safely for a full lexicographic search.

This means:
- max-min remains an independent first-component oracle,
- full QMRVector branch-and-bound is mathematically feasible,
- branches with equal worst quality must not be discarded if their remaining low-tail entries can
  improve.

This update supersedes any implication that max-min must remain the final SPR objective.
