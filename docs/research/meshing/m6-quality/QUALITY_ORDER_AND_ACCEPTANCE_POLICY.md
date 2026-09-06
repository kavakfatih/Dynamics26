# M6 Research — Quality Order and Acceptance Policy

Status: RESEARCHING / engineering-contract candidate
Date: 2026-09-06

## 1. Research question

When several legal tetrahedral mesh-improvement candidates exist, how should Dynamics26 decide:

    candidate A
    vs
    candidate B
    vs
    no operation?

The acceptance rule must simultaneously support:

- strong protection of the worst tetrahedra,
- improvement of the low-quality tail even when the single worst tetrahedron is unchanged,
- deterministic operation selection,
- finite-state termination for connectivity-only hill climbing,
- local comparison without constructing a global mesh-quality object,
- different tetra counts across 2->3 / 3->2 / edge-removal operations,
- compatibility with strong cavity search,
- separation from smooth proposal objectives,
- no epsilon leakage into exact topology truth.

The leading answer is:

    exact validity gates
      ->
    lexicographically ordered local mean-ratio multiset
      ->
    deterministic candidate tie handling.

Aggregate objectives remain valuable for proposal generation and smoothing, but are not the leading
commit-acceptance authority.

Primary literature:
- M6-OPT-008 — Klingner & Shewchuk quality vector,
- M6-OPT-012 — Klingner detailed tetra-mesh-improvement treatment,
- M6-TH-006 — Freitag & Knupp worst-versus-average optimization,
- M6-OPT-006 — Munson inverse-mean-ratio aggregate optimization,
- M6-TH-002 / M6-TH-003 — tetra mean-ratio / algebraic quality foundations.

## 2. Terminology: two different vectors

Dynamics26 already uses a layered engineering quality vector containing different observables:

    q_MR
    q_kappa
    angles
    radius metrics
    FEM context
    nonlinear context
    formulation suitability.

That is a multi-metric diagnostic vector.

This document introduces:

    QMRVector(C)

which is the sorted list of q_MR over a changed local cavity C.

Therefore:

    multi-metric diagnostic vector
    !=
    sorted optimizer order vector.

## 3. Why pure max-min is incomplete

For cavity C:

    q_min(C)
      =
    min_{t in C} q_MR(t).

A strict max-min rule accepts only if:

    q_min(new) > q_min(old).

This strongly protects the worst element, but it can stall.

Example:

    old = [0.20, 0.30, 0.80]
    new = [0.20, 0.31, 0.32].

The worst remains 0.20.

Pure max-min reports a tie.

But the second-worst improves:

    0.30 -> 0.31.

A low-tail order should see that improvement.

## 4. Sorted mean-ratio quality vector

For local tetra set C:

    QMRVector(C)
      =
    sort_ascending(
      q_MR(t_1),...,q_MR(t_n)
    ).

Write:

    [q_(1),q_(2),...,q_(n)]

with:

    q_(1) <= q_(2) <= ... <= q_(n).

Compare from worst to best lexicographically.

The first differing element decides.

Thus:

    [0.20,0.31,0.32]
      >
    [0.20,0.30,0.80].

This strictly refines max-min.

## 5. Variable cell counts

Topology operations can change tetra count.

Research convention:

    compare common entries first;
    if every common entry is exactly equal,
    prefer the shorter vector.

Equivalent conceptual model:

    pad every vector with +infinity.

Example:

    [0.20,0.80]
      >
    [0.20,0.80,0.95].

Interpretation:

    exact quality tie
      ->
    no gratuitous extra tetrahedra.

This is not a general coarsening policy.

All geometry, point-set, sizing and provenance constraints remain active.

## 6. Local-to-global composability theorem

Let:

    U = unchanged quality multiset
    A = old changed-cavity multiset
    B = new changed-cavity multiset.

Global states:

    G_old = U union A
    G_new = U union B.

If:

    QMRVector(B) >_lex QMRVector(A),

then:

    QMRVector(G_new) >_lex QMRVector(G_old).

### Cumulative-count proof

Define:

    N_A(x)
      =
    number of q in A with q <= x.

For ascending lexicographic order with +infinity padding, the better multiset has fewer entries at the
first quality level where cumulative bad-counts differ.

Adding U gives:

    N_(U union A)(x)
      =
    N_U(x)+N_A(x)

and:

    N_(U union B)(x)
      =
    N_U(x)+N_B(x).

The common term cancels.

So local comparison preserves the global direction.

### Consequence

An operation needs only the changed old/new cavity vectors.

The global sorted vector never needs to be materialized.

## 7. Why aggregate objectives cannot be the authority

### Arithmetic mean counterexample

    old = [0.20,0.20]
    new = [0.19,0.50].

Arithmetic mean:

    old = 0.20
    new = 0.345.

Average improves while the worst tetrahedron degrades.

### Harmonic / inverse-mean counterexample

Harmonic mean:

    H
      =
    n / sum_i(1/q_i).

For the same vectors:

    H_old = 0.2000
    H_new ~= 0.27536.

Again the aggregate improves while the worst degrades.

Therefore aggregate quality does not guarantee low-tail monotonicity.

## 8. Aggregate objectives can also reject low-tail improvement

Use:

    old = [0.20,0.30,0.80]
    new = [0.20,0.31,0.32].

QMRVector improves at the second entry.

But:

    arithmetic mean:
      old ~= 0.43333
      new ~= 0.27667

and:

    harmonic mean:
      old ~= 0.31304
      new ~= 0.26430.

So average objectives can reject a low-tail improvement.

This explains why worst and aggregate objectives are complementary rather than interchangeable.

## 9. Proposal objective versus commit objective

Dynamics26 separates two contracts.

### Proposal objective

Used to create a candidate:
- smart Laplacian direction,
- inverse-mean-ratio smooth optimization,
- condition-number optimization,
- max-min nonsmooth optimization,
- ODT energy,
- future solver-aware objective.

### Commit objective

Used to make the candidate authoritative:

    exact hard validity
      +
    strict QMRVector improvement.

Therefore a proposal optimizer can use a differentiable scalar without allowing aggregate
compensation to worsen the low-quality tail.

## 10. Mean-ratio edge-length form

For a positive tetrahedron let:

    V = volume

and:

    S
      =
    sum over six edges of l_e^2.

The normalized mean ratio is:

    q_MR
      =
    12 (3V)^(2/3)/S.

Regular tetrahedron:

    q_MR = 1.

Degenerate limit:

    q_MR -> 0.

This is equivalent to the weighted-map M6 definition.

## 11. Exact-order transform

Let:

    D
      =
    |OrientDet|
      =
    6V
      >
    0.

Then:

    3V = D/2

and:

    q_MR^3
      =
    432 D^2/S^3.

Define:

    R_MR
      =
    D^2/S^3.

Because cubing is strictly monotone on positive values:

    q_MR(A) > q_MR(B)

iff:

    R_MR(A) > R_MR(B).

No cube root or pow is needed for ordering.

## 12. Exact canonical-binary64 comparator

Every finite binary64 coordinate is dyadic rational.

Evaluated exactly:
- coordinate differences are dyadic,
- squared edge lengths are dyadic,
- S is dyadic,
- D is dyadic,
- D^2 and S^3 are dyadic.

Therefore:

    R_A > R_B

iff:

    D_A^2 S_B^3
      >
    D_B^2 S_A^3.

Define:

    C_Q
      =
    D_A^2 S_B^3
      -
    D_B^2 S_A^3.

The exact sign of C_Q gives exact q_MR order under the canonical binary64 coordinate model.

This removes the need for a pairwise quality epsilon.

## 13. Exact equality

If:

    C_Q = 0,

the tetrahedra have exactly equal q_MR in the canonical coordinate model.

That is a true order tie.

It is not:

    abs(q_A-q_B) < epsilon.

## 14. M1 boundary

M1 owns exact Orient3D sign truth.

M6 q_MR order also needs:
- determinant magnitude D,
- exact squared-edge sum S.

Do not assume the existing public predicate API exposes exact magnitude.

Later options:
- dedicated exact dyadic quality-order oracle,
- audited internal determinant-magnitude helper,
- certified floating filter with exact fallback.

No API choice is frozen.

## 15. Future filtered comparator

Potential architecture:

    fast approximate C_Q
      ->
    certified sign when safely separated
      ->
    exact dyadic fallback near zero.

The error bound must be derived specifically for the q_MR comparison expression.

Orient3D filter bounds cannot be copied.

## 16. Why epsilon equality is unsafe

Suppose:

    a ~= b
    iff
    |a-b| < delta.

Choose:

    a = 0
    b = 0.75 delta
    c = 1.50 delta.

Then:

    a ~= b
    b ~= c
    a !~= c.

So approximate equality is not transitive.

It is unsafe as the equality relation for:
- sorting,
- priority queues,
- canonical candidate order,
- cycle/termination proofs.

A smoothing convergence margin is a separate stop policy, not the q_MR order relation.

## 17. Exact local comparison algorithm candidate

For each valid positive tetra:
1. form exact-order ratio representation D^2/S^3,
2. sort local elements worst-to-best by exact comparison,
3. compare old/new vectors,
4. first difference decides,
5. exact prefix -> shorter wins,
6. identical vector -> quality tie.

This gives a deterministic total order on finite q_MR multisets.

## 18. Candidate selection and no-op ties

If multiple new candidates have the same best improving QMRVector:
- choose canonical resulting connectivity.

But mutation against the current mesh still requires:

    QMRVector(new)
      >
    QMRVector(old).

If vectors are equal:
- no topology mutation.

This prevents quality-neutral canonical changes from cycling.

## 19. Connectivity-only termination

For fixed:
- finite sites,
- coordinates,
- boundary/protected constraints,

the point-set-preserving connectivity state space is finite.

If each accepted mutation strictly improves exact QMRVector:
- no state repeats,
- exact quality ties do not mutate,
- accepted connectivity history terminates.

This is a termination proof, not a useful complexity bound.

## 20. Smoothing termination remains separate

Smoothing changes coordinates continuously.

Exact QMRVector improvement alone does not exclude arbitrarily small infinite improvement sequences.

Smoothing therefore still needs convergence/effort controls such as:
- minimum accepted gain,
- minimum displacement,
- line-search limit,
- pass limit,
- active-set stall.

These values do not define q_MR equality.

## 21. Max-min is the first vector component

For complete triangulation T:

    first(QMRVector(T))
      =
    q_min(T).

So max-min search optimizes exactly the first quality-vector component.

It is a useful independent oracle but does not optimize deeper low-tail entries.

## 22. Max-min pruning limitation

For a partial SPR triangulation P:

    U_min(P)
      =
    minimum q already inserted.

For max-min-only optimization:

    U_min(P) <= q_best

can prune if equal-worst alternatives are irrelevant.

For full lexicographic optimization this is too aggressive.

A branch with:

    U_min(P) == q_best

can still improve:
- second worst,
- third worst,
- later entries.

## 23. Lexicographic branch-and-bound upper bound

For partial P define:

    Q_upper(P)
      =
    QMRVector(P)
    padded with +infinity.

Every completion adds finite q values.

Adding finite entries cannot make the completed vector better than the optimistic +infinity-padded
partial vector.

Therefore:

    QMRVector(completion)
      <=_lex
    Q_upper(P).

If current best complete vector is Q_best and:

    Q_upper(P)
      <=_lex
    Q_best,

the branch may be pruned safely.

This gives a first-principles lexicographic SPR branch-and-bound bound.

## 24. Strong-search objective update

Research should keep two independent oracles:

### Oracle A — max-min

Purpose:
- simple first component,
- easy cross-check.

### Oracle B — full QMRVector

Purpose:
- true low-tail lexicographic optimum,
- alignment with commit acceptance.

They must agree on the optimal worst quality.

They can choose different triangulations when several candidates share that worst value.

## 25. Why q_kappa/angles are not added to the lex order yet

M6 established:
- q_kappa is mathematically related to q_MR,
- radius ratio is related in shape-regularity behavior,
- angles diagnose complementary morphology,
- interpolation and solver suitability are separate evidence layers.

Therefore an arbitrary universal tuple:

    (q_MR,q_kappa,theta_min,-theta_max,...)

would encode unvalidated tradeoffs.

Current policy candidate:
- q_MR defines isotropic shape acceptance,
- other geometry metrics remain diagnostics,
- solver-specific guardrails wait for M7 correlation.

## 26. Reporting

The full conceptual mesh QMRVector does not need to be stored globally.

User-facing quality reports continue to expose:
- minimum,
- low percentiles,
- histograms,
- pathology diagnostics,
- metric distributions.

Local operations compare only changed cavities.

## 27. Candidate policy versions

Research-only placeholders:

    D26QMR1
      exact canonical-binary64 q_MR order

    D26QV1
      sorted local q_MR multiset order
      with shorter-wins exact-prefix rule

    D26QACC1
      hard validity
      +
      strict D26QV1 improvement
      +
      no mutation on exact quality tie.

These are not accepted API/version contracts.

## 28. Verification requirements

Required fixtures:
1. weighted-map/edge-length q_MR equivalence,
2. q_MR^3 = 432 D^2/S^3 goldens,
3. exact comparator versus independent high precision,
4. scale/adversarial near-tie cases,
5. exact equality cases,
6. epsilon non-transitivity negative control,
7. local/global multiset composability,
8. variable-cell-count prefix semantics,
9. max-min tie / second-worst improvement,
10. aggregate worst-regression,
11. aggregate rejection of low-tail improvement,
12. no-cycle connectivity replay,
13. max-min versus full-lex SPR comparison,
14. lex branch-and-bound versus exhaustive cavity oracle,
15. proposal/commit separation for smoothing.

## 29. Current conclusion

The leading M6 acceptance architecture is:

    generate candidate with an operation-appropriate objective
      ->
    exact validity/protected-geometry validation
      ->
    exact sorted q_MR local comparison
      ->
    commit only strict lexicographic improvement.

This supplies:
- worst-element protection,
- deeper low-tail improvement,
- local-to-global monotonicity,
- deterministic exact ties,
- connectivity termination,
- compatibility with smooth proposal objectives,
- a route to full lexicographic SPR search.

No production M6 code is authorized.


## 30. D26QMR1 research refinement — exact order, not exact scalar value

The exact comparator does not need to construct the real number q_MR itself.

In general:

    q_MR
      =
    cbrt(
      432 D^2/S^3
    )

can be irrational even when D and S are exact rationals.

The optimizer only needs:

    compare(q_MR(A), q_MR(B)).

Because cubing is strictly monotone on q_MR > 0:

    compare(q_MR(A), q_MR(B))
      =
    compare(
      D_A^2/S_A^3,
      D_B^2/S_B^3
    ).

Therefore D26QMR1 is defined conceptually as an **exact ordering predicate**.

It is not an exact real-valued q_MR evaluator.

User-facing q_MR values may remain floating diagnostics while authoritative ordering is exact.

## 31. Binary64 lattice reduction

Every finite IEEE-style binary64 value is a dyadic rational.

Using the smallest binary64 lattice unit:

    lambda_64
      =
    2^-1074,

every finite coordinate x can be written exactly as:

    x
      =
    I_x lambda_64

for an integer I_x.

For normal binary64 with raw exponent E in [1,2046] and fraction field f:

    I_x
      =
    sign
    (2^52 + f)
    2^(E-1).

For a subnormal:

    I_x
      =
    sign f.

Canonical signed zero maps to:

    I_x = 0.

NaN and infinity are outside the D26 canonical finite-site contract.

The largest finite binary64 coordinate has a lattice integer requiring at most 2098 bits.

A difference of two finite coordinates therefore requires at most 2099 bits.

## 32. Primitive tetra lattice

Decode the four tetra vertices to lattice integers:

    I_0, I_1, I_2, I_3.

Translation is removed by exact differences.

Define G as the positive gcd of all non-zero integer coordinate differences between tetra vertices.

Then define primitive integer relative coordinates:

    U_i
      =
    (I_i - I_0)/G.

At least one component is primitive with respect to the common integer scale.

The physical relative coordinates are:

    X_i - X_0
      =
    alpha U_i

with:

    alpha
      =
    G 2^-1074.

For:

    d
      =
    |det(U_1,U_2,U_3)|

and:

    s
      =
    sum over six edges
      ||U_j-U_i||^2,

we obtain exactly:

    D
      =
    alpha^3 d

and:

    S
      =
    alpha^2 s.

Hence:

    q_MR^3
      =
    432 d^2/s^3.

The complete binary64 exponent structure cancels.

The exact reference comparison is therefore pure integer arithmetic:

    sign(
      d_A^2 s_B^3
      -
      d_B^2 s_A^3
    ).

## 33. Primitive-scale invariance

The gcd normalization is optional for correctness but valuable for compactness.

Any exact common integer factor h applied to every coordinate difference gives:

    d -> h^3 d
    s -> h^2 s

and thus:

    d^2/s^3
      unchanged.

Therefore dividing by G:
- preserves q_MR order,
- preserves exact equality,
- reduces exact arithmetic sizes,
- makes the fallback representation independent of a gratuitous common lattice scale.

The mathematical G is independent of the chosen anchor vertex because the pairwise-difference set is
generated by the differences to any one tetra vertex.

## 34. Conservative exact-arithmetic size bound

Let B be the maximum bit length of any primitive pairwise edge-coordinate component.

Then:

    B <= 2099

for arbitrary finite binary64 input coordinates.

There are 18 squared scalar edge components in s.

Therefore:

    s
      <
    18 2^(2B)
      <
    2^(2B+5).

The 3x3 determinant has six triple products:

    |d|
      <
    6 2^(3B)
      <
    2^(3B+3).

One cross-comparison product satisfies:

    d_A^2 s_B^3
      <
    2^(12B+21).

The final difference therefore requires fewer than approximately:

    12B + 22

bits.

At the absolute binary64 worst case:

    B=2099
      =>
    < 25210 bits

for the comparison magnitude.

This is only a conservative mathematical bound.

Primitive normalization can make ordinary local mesh cases much smaller; telemetry is required before
any storage backend is optimized around a typical width.

## 35. Validity must precede quality

Because the comparator uses d^2, an inverted tetrahedron would otherwise have the same shape score as
its positive mirror.

Therefore the order is always:

    finite/distinct topology checks
      ->
    exact Orient3D positive validity
      ->
    D26QMR1 quality comparison.

D26QMR1 never decides:
- inversion,
- degeneracy,
- topology validity.

A zero determinant is a geometry-validity result, not a quality tie.

## 36. Filtered-exact research architecture

The reference oracle can be exact-only.

A later performance path may use:

    certified fast filter
      ->
    exact primitive-lattice fallback.

The filter is allowed to return only:
- Less,
- Equal only if exact stage proves it,
- Greater,
- Uncertain.

An uncertain filter result must fall back to exact arithmetic.

No approximate-only quality decision is permitted.

See EXACT_QMR_COMPARATOR.md for the detailed interval candidate and qualification rules.


## 37. Batch-union monotonicity theorem

Let the quality-multiset order be ascending lexicographic D26QV1 with the shorter-wins exact-prefix
rule, equivalently conceptual +infinity padding.

The already-established union compatibility states:

    B > A
      =>
    B union C > A union C

for any common multiset C.

Now consider m selected local replacements:

    A_i -> B_i

with:

    QMRVector(B_i)
      >
    QMRVector(A_i)

for every i.

Assume the selected round admits a valid decomposition of the mesh into:
- unchanged multiset U,
- pairwise distinct old changed-cell multisets A_i,
- pairwise distinct new changed-cell multisets B_i.

Define intermediate global quality multisets:

    M_0
      =
    U union A_1 union ... union A_m

    M_k
      =
    U
    union B_1 ... union B_k
    union A_(k+1) ... union A_m.

For step k, all terms except A_k/B_k are one common multiset C_k.

Therefore:

    B_k > A_k
      =>
    M_k > M_(k-1).

By transitivity:

    M_m > M_0.

Hence a non-empty batch of strict local improvements is a strict global improvement.

## 38. Parallel-round consequence

For D26QSCHED1 selected winners:
- all proposals were evaluated on one immutable snapshot,
- semantic footprints are pairwise non-conflicting,
- each proposal passed exact validity/constraint checks,
- each proposal is a strict D26QV1 improvement.

Non-conflict guarantees that committing one selected winner does not change the quality inputs used by
another selected winner.

Therefore the batch-union theorem applies to the whole selected round:

    GlobalQMRVector(after round)
      >
    GlobalQMRVector(before round).

Thread count and worker completion order do not enter this proof.

## 39. Empty round

If deterministic selection produces no legal strict-improvement winner:

    global quality does not change.

The round scheduler must either:
- escalate to the next operation tier,
- or stop according to D26OPS1.

It must not spin indefinitely on an unchanged snapshot.

## 40. Binary64 finite-state termination refinement

Earlier research separated connectivity termination from smoothing because continuous real-coordinate
smoothing has an infinite state space.

Dynamics26 production state is more specific.

Assume:
- fixed finite PointId set,
- no point insertion/deletion,
- every stored coordinate is a finite canonical binary64 value,
- topology belongs to the finite valid tetrahedralization state space for those sites/boundary rules,
- every accepted topology or smoothing commit strictly improves global D26QV1.

Then the authoritative mesh state space is finite:
- finitely many coordinate bit patterns per scalar,
- finitely many point-coordinate tuples,
- finitely many valid connectivity states for a fixed finite point set.

Every accepted commit gives a strict global QMRVector increase.

Therefore:
- no accepted state can repeat,
- no accepted global quality vector can repeat,
- the accepted mutation sequence is finite.

This applies to actual binary64 smoothing commits as well as connectivity changes.

## 41. What still needs bounded termination

Finite accepted state does not make every proposal generator automatically terminating.

Each private search must terminate independently.

Examples:
- line search,
- optimization-smoothing iterations,
- edge-removal DP,
- SPR branch-and-bound.

Reference qualification uses finite/count-based limits or finite exact enumeration.

A resource/budget stop is reported explicitly and is not equivalent to:

    ExhaustiveNoImprovement.

No minimum pairwise q_MR epsilon is required for the global no-cycle proof.
