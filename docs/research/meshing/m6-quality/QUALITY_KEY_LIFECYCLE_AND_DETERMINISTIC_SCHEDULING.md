# M6 Research — QualityKey Lifecycle, Cache Invalidation and Deterministic Scheduling

Status: RESEARCHING / optimizer-state contract candidate
Date: 2026-09-06

## 1. Engineering question

D26QMR1 can compare tetra mean-ratio quality exactly.

The next problem is not arithmetic.

It is state ownership:

- when is an exact tetra QualityKey valid?
- what mutation invalidates it?
- may a key survive TetHandle deletion/recreation?
- how should local caches behave during smoothing and reconnection?
- how can proposal evaluation run in parallel without making thread order part of the meshing result?

The leading answer separates:

    numeric geometry identity
    topology storage identity
    operation legality
    optimizer scheduling.

## 2. Repository facts

Current M1/M2 research/source already distinguishes several identities.

### PointId

PointId is stable site identity inside an immutable meshing-request snapshot.

It is independent of:
- pointer address,
- parallel scheduling,
- transient insertion order.

### TetHandle

Current finite topology prototype uses:

    TetHandle
      =
    (slot,generation).

Its purpose is stale-reference detection.

A deleted/reused storage slot can acquire a new generation.

### visitEpoch

Traversal epochs are temporary membership marks.

M1/M2 explicitly state that traversal epochs:
- are not semantic identity,
- do not enter replay fingerprints.

### M2 append-only reference arena

M2.1 research prefers append-only cell storage first so slot reuse cannot obscure correctness.

This is a storage/replay choice, not tetra geometric identity.

## 3. Pure tetra geometry identity

For fixed point coordinates, tetra mean ratio depends only on the four point positions.

For one immutable coordinate state define:

    CanonicalTetKey
      =
    sort(PointId_0,PointId_1,PointId_2,PointId_3).

For D26QMR1 the local vertex ordering does not affect the quality value because:
- six edge lengths are permutation invariant,
- determinant sign disappears in d^2.

Therefore the same four points may reuse one numeric q_MR key even if:
- stored vertex order differs,
- TetHandle slot differs,
- TetHandle generation differs,
- adjacency differs.

Exact positive orientation remains a separate validity requirement before acceptance.

## 4. QualityKey validity domain

A first conceptual cache domain is:

    QualityKeyDomain
      =
    (
      MeshingSnapshotIdentity,
      CoordinateState,
      D26QMR1Version
    ).

Then:

    cache key
      =
    (
      QualityKeyDomain,
      CanonicalTetKey
    ).

The exact representation stored may be:
- (d,s),
- lazy (d,s)->(d^2,s^3),
- another exact D26QMR1-equivalent representation.

The lifecycle contract is independent of that storage choice.

## 5. Why TetHandle is not enough

TetHandle answers:

    does this topology reference still point to the same live storage object?

It does not answer:

    do these four point coordinates have the same quality state?

Examples:

### Same geometry, new handle

A reconnection can delete a tetra and later recreate the identical canonical four-point simplex in a
new slot.

Its q_MR key is unchanged if coordinates are unchanged.

### Same handle semantics, moved coordinate

A smoothing implementation can retain the same logical tetra connectivity while changing one point
coordinate.

The q_MR key changes even though the topological identity may remain logically continuous.

Therefore:

    TetHandle generation
      !=
    Quality coordinate generation.

## 6. Why visitEpoch never enters QualityKey

visitEpoch changes during traversal even when:
- coordinates are unchanged,
- topology is unchanged,
- q_MR is unchanged.

Including it would make cache identity depend on traversal history.

Forbidden:

    QualityKey(...,visitEpoch).

Traversal epochs remain optimizer implementation metadata only.

## 7. Mutation-invalidation matrix

### Connectivity-only 2<->3 / 3<->2 / edge removal / SPR

Coordinates:
    unchanged.

Numeric q_MR key for any identical four-point tetra:
    remains valid.

Topology/proposal handles in affected cavity:
    invalid/revalidate.

Active poor-tet queue entries:
    affected neighborhood invalid/rebuild.

### Accepted smoothing move

One point coordinate:
    changes.

Every tetra containing that PointId:
    numeric q_MR key invalid.

Unrelated tetra keys:
    mathematically remain valid.

First implementation may conservatively discard a larger local cache.

### Point insertion

New point identity:
    introduced.

Old numeric keys not containing the new point remain geometrically valid.

Topology/proposal state around insertion cavity:
    invalid.

Cross-request/snapshot reuse:
    not allowed first.

### Point deletion

Keys containing deleted point:
    invalid/unreachable.

Unrelated numeric keys:
    geometrically valid.

Topology active state:
    local rebuild.

### CAD/provenance/protected-constraint change without coordinate motion

Numeric q_MR key:
    may remain valid.

Operation legality/proposal cache:
    invalid.

This proves numeric quality caching and operation-legality caching must be separate.

### D26QMR1 policy/version change

Quality cache:
    invalid unless the new version explicitly declares representation compatibility.

### New canonicalization / MeshingRequest snapshot

All PointId-scoped cache identity:
    new domain.

No cross-snapshot reuse in first design.

## 8. First coordinate-state policy

The simplest safe first implementation does not need a persistent per-point coordinate revision.

Use phase/lifetime restrictions:

### Connectivity-only phase

Coordinates are immutable.

A local/pass/cavity QualityKey cache may use:

    CanonicalTetKey

inside the known immutable coordinate phase.

### Smoothing proposal

Use proposal-local keys.

After an accepted move:
- discard affected-star keys,
- rebuild from the new stored binary64 coordinates.

This is conservative and easy to qualify.

## 9. Future per-point coordinate generation

A later optimization may attach:

    coordinateGeneration(PointId)

incremented on every accepted coordinate mutation.

Then a fully local numeric cache identity can be:

    sorted(
      (PointId_0,coordGen_0),
      ...
      (PointId_3,coordGen_3)
    ).

Benefits:
- unaffected tetra keys survive distant smoothing,
- invalidation becomes local.

Requirements:
- no silent wraparound,
- generation never changes for topology-only mutation,
- generation increment occurs atomically with committed coordinate update,
- replay/fingerprint semantics remain defined separately.

This is not needed for first M6 implementation.

## 10. Cache strata

Do not build one cache that mixes all optimizer state.

Research separates at least four strata.

### C0 — immutable point dyadic decode cache

Potential content:
- binary64 dyadic sign/significand/exponent for x,y,z.

Identity:
- PointId + coordinate state.

This is pure arithmetic input and can be read-only during connectivity phases.

### C1 — tetra exact quality cache

Content:
- (d,s),
- optional promoted powers.

Identity:
- QualityKeyDomain + CanonicalTetKey.

Best first use:
- edge-removal DP,
- SPR/private cavity search.

### C2 — proposal evaluation cache

Content:
- old/new local QualityVector,
- legality checks,
- operation-specific search results.

Identity:
- optimization-round snapshot + CandidateKey.

Lifetime:
- one planning round/proposal.

### C3 — active-set/worklist metadata

Content:
- target poor-tet priority,
- scheduling status,
- retry/defer state.

This is scheduling state, not numeric quality truth.

## 11. Cache purity rule

A cache hit may replace recomputation only.

It may never:
- change candidate ordering,
- change legality,
- change equality semantics,
- change resource budget accounting that is defined as algorithmic truth.

Required metamorphic test:

    cache disabled
      and
    cache enabled

must produce identical:
- exact comparisons,
- selected proposals,
- committed topology,
- exact ties.

Only performance telemetry may differ.

## 12. Cache allocation failure

Cache memory is optional optimization state.

If cache insertion/reservation fails but exact arithmetic can still proceed:

    disable/drop cache entry
      ->
    recompute as needed.

Do not report:
- geometry invalid,
- no better candidate

because a memoization allocation failed.

If exact arithmetic itself cannot obtain required memory:

    explicit resource failure.

Resource failure is not quality equality and not geometry degeneracy.

## 13. Why permanent global cache remains deferred

Previous worst-case D26QMRB1 analysis gives approximately:
- 1316 raw bytes for cached (d,s),
- 3156 raw bytes for cached (d^2,s^3),

per tetra at the absolute binary64 width bound, excluding container overhead.

Therefore first architecture favors:
- proposal-local,
- cavity-local,
- pass-local

memoization.

A global cache requires measured typical widths/hit rates and a memory policy.

## 14. Optimizer round snapshot

Parallel planning needs an immutable semantic view.

Conceptual:

    OptimizationRoundSnapshot
      =
    (
      topology state/revision,
      coordinate state,
      protected-constraint state,
      D26QMR1/D26QV1/D26QACC1 versions,
      operation-schedule version
    ).

During planning:
- no worker mutates authoritative mesh state,
- workers may build private candidate data,
- quality caches are immutable/shared-read-only or worker/proposal local.

## 15. Proposal record

Conceptual proposal:

    QualityProposal
      - CandidateKey
      - ScheduleKey
      - source activation key
      - operation family
      - read footprint
      - write footprint
      - old local QualityVector
      - new local QualityVector
      - exact validity/constraint evidence
      - candidate connectivity/coordinate payload
      - snapshot identity
      - resource/search-result status.

A proposal is not mesh authority.

It is a private plan against one snapshot.

## 16. Semantic read/write footprints

The conflict model is based on semantic state, not memory addresses.

Resource namespaces may include:

    Cell(handle/generation)
    PointCoordinate(PointId)
    ProtectedConstraint(entity/key)
    BoundaryAdjacency/OutsideCell(handle/generation)

A topology operation write footprint must include:
- cavity cells removed/replaced,
- all exterior neighbor records whose reciprocal adjacency is patched,
- any topology index/store records that carry semantic connectivity.

A smoothing write footprint includes:
- moved PointCoordinate.

Its semantic read footprint includes:
- neighboring coordinates,
- incident tetra connectivity/constraints,
- any size/CAD data used in acceptance.

## 17. Conflict definition

For proposals A and B:

    Conflict(A,B)

if:

    W_A intersects (R_B union W_B)

or:

    W_B intersects R_A.

Equivalent symmetric form:

    W_A intersects (R_B union W_B)
      or
    W_B intersects (R_A union W_A).

Read-read overlap is allowed.

For first qualification:
- over-approximate uncertain footprints,
- never under-approximate.

False conflict reduces parallelism.

False non-conflict can corrupt correctness.

## 18. Relation to parallel mesh literature

Parallel tetra improvement literature supports exploiting local-operation independence:
- vertex coloring for smoothing,
- cavity/element locking or atomic ownership for topology-changing operations,
- spatial ordering/partitioning to reduce conflicts.

Recent multithreaded work explicitly treats overlapping operation cavities as the race source and
protects cavity vertices with atomic ownership.

Dynamics26 adopts the locality insight but not the nondeterministic commit semantics.

Its first goal is portable deterministic output.

## 19. Deterministic ScheduleKey

Every active/proposal item must have a total order independent of runtime timing.

Leading research tuple:

    ScheduleKey
      =
    (
      PoorTetKey,
      OperationFamilyRank,
      CanonicalFootprintKey,
      CanonicalCandidateKey
    ).

Where:

    PoorTetKey
      =
    (
      exact q_MR ascending,
      CanonicalTetKey
    ).

OperationFamilyRank is versioned.

CanonicalFootprintKey/candidate keys use stable PointId/topological identity.

Forbidden:
- pointer address,
- thread index,
- completion order,
- randomized hash seed,
- wall-clock arrival.

## 20. Why not rank conflicts by "largest improvement"

Quality improvements from unrelated cavities can involve:
- different old vector lengths,
- different local contexts,
- different operation families.

There is no accepted global scalar "benefit" that makes those proposals commensurate.

First schedule therefore prioritizes the deterministic active target/order policy, not an invented
cross-cavity gain scalar.

Future scheduling experiments may change this only under a versioned policy.

## 21. Reference deterministic winner selection

Build the proposal conflict graph.

Reference algorithm:

    proposals sorted by ScheduleKey

    selected = empty

    for proposal p in order:
        if p conflicts with no selected proposal:
            select p
        else:
            defer/discard p for this round.

This is a deterministic greedy maximal independent set relative to the fixed total order.

It is an oracle/reference schedule, not necessarily the final parallel implementation.

A faster deterministic MIS/round scheduler may replace it only if it returns the same selected set or
defines a newly versioned schedule with its own qualification.

## 22. Parallel plan, deterministic commit

First parallel architecture:

### Phase A — snapshot

Freeze authoritative input for the round.

### Phase B — plan/evaluate in parallel

Workers:
- evaluate operation candidates,
- run exact predicates,
- compute D26QMRF1/D26QMRB1 results,
- use private/local caches,
- produce immutable proposal records.

### Phase C — deterministic selection

Using ScheduleKey + conflict footprints, select a conflict-free winner set.

### Phase D — ordered revalidation

Before commit:
- validate snapshot handles/generations/constraints,
- validate proposal payload,
- verify no unexpected semantic conflict.

### Phase E — canonical commit

First version commits winners in ScheduleKey order.

### Phase F — rebuild affected activity

Invalidate/requeue affected neighborhood and begin a new snapshot round.

This allows substantial parallelism in expensive planning/search without making authoritative mutation
concurrent first.

## 23. Conflict-free serializability theorem

Suppose proposals A and B:
- were evaluated against the same immutable snapshot,
- have exact declared read/write sets,
- have no read/write or write/write conflict.

Then:
- A does not modify any state observed by B,
- B does not modify any state observed by A.

At the semantic-state level their updates commute:

    A(B(M))
      =
    B(A(M))

for the declared state.

Therefore a selected pairwise non-conflicting set is serializable in any order.

### Important boundary

This does **not** prove equivalence to a different serial optimizer that:
1. commits one proposal,
2. regenerates all candidates,
3. chooses a new best item,
4. repeats.

D26QSCHED1 is its own round-based algorithm.

The guarantee target is deterministic result for that frozen algorithm.

## 24. Why ordered commit remains first

Even if semantic operations commute, storage side effects may not:
- append-only slot allocation order,
- diagnostic IDs,
- telemetry order,
- future free-list allocation.

M2 already values deterministic write order.

Therefore first M6 parallel qualification keeps:

    proposal evaluation parallel
    authoritative commit ordered.

Later parallel commit requires:
- deterministic slot/range preassignment,
- no allocation races,
- canonical output fingerprint equivalence,
- exact same semantic result.

## 25. Future deterministic parallel commit

A later research path may:

1. sort selected winners,
2. precompute required new-cell counts,
3. assign deterministic disjoint append ranges by prefix sum,
4. preallocate capacity,
5. commit independent winners in parallel to assigned ranges,
6. rebuild/validate global indices deterministically.

This can retain stable storage output while removing serial mutation cost.

It is deferred until serial ordered commit is qualified.

## 26. Smoothing conflict model

For smoothing one vertex v:

Write:
    coordinate(v).

Reads:
- neighbor coordinates,
- incident tetra connectivity,
- incident constraints,
- size/CAD state used by proposal.

Quality invalidation:
- all tetra incident to v.

Two smoothing moves are compatible only if neither writes a coordinate read by the other and their
semantic affected state is disjoint under the frozen policy.

A deterministic vertex coloring is one way to construct compatible groups.

Classical mesh literature uses graph coloring for parallel smoothing because same-color vertices can be
chosen to avoid mutual interference.

D26QSCHED1 may use the generic footprint model instead of a special smoothing-only scheduler.

## 27. Topology-changing operations need dynamic footprints

A static vertex coloring is less natural for:
- 2<->3,
- 3<->2,
- general edge removal,
- growing strong cavities.

Their cavity size/shape depends on current topology and search.

Therefore:
- plan actual cavity,
- derive actual semantic footprint,
- conflict-check proposals per round.

This follows the mesh-improvement literature observation that topology-changing operations require
runtime cavity conflict protection.

## 28. Proposal invalidation

A proposal is stale if any state in its semantic read set changed since its snapshot.

First round architecture avoids complicated incremental validation:
- all selected winners come from the same snapshot,
- losers/deferred proposals are not carried blindly into the next round,
- rebuild/replan after the commit round.

This trades recomputation for simple correctness.

Future carry-over optimization requires per-resource revisions.

## 29. Revision hierarchy candidate

Do not overload one integer for all semantics.

Research conceptual revisions:

    MeshingSnapshotId
      canonical site/provenance input domain

    TopologyRevision
      authoritative connectivity mutation

    CoordinateRevision
      authoritative coordinate mutation

    ConstraintRevision
      protected/CAD/size legality state

    ScheduleVersion
      D26QSCHED1 algorithm version

A global CoordinateRevision is safe but coarse.

First implementation can avoid persisting q_MR keys across smoothing instead of introducing this
entire revision system.

## 30. Handle generation versus topology revision

TetHandle generation:
- validates one slot reference.

TopologyRevision:
- identifies a broader authoritative mesh state.

A proposal may contain valid handles whose surrounding topology no longer matches its snapshot.

Therefore handle generation alone is insufficient for long-lived proposal validity.

First round design simply discards old-round proposals.

## 31. Quality cache in parallel planning

Leading first policy:

### Shared immutable

Allowed:
- point coordinate arrays,
- optional fully prebuilt dyadic decode table,
- immutable input/provenance tables.

### Proposal/thread local mutable

Preferred:
- exact tetra quality memoization,
- SPR branch caches,
- temporary QualityVectors,
- local search state.

Benefits:
- no lock ordering,
- no cache race,
- no semantic dependence on eviction timing.

Duplicate computation is acceptable before shared-cache complexity is justified.

## 32. Shared mutable cache boundary

A future shared exact-key cache is a performance optimization only.

Requirements:
- cache contents are pure functions of key domain,
- miss/eviction never changes algorithm result,
- hash/randomization does not affect scheduling,
- memory pressure never becomes a quality decision,
- thread-count changes may alter hit-rate telemetry but not output.

If deterministic cache telemetry is required, use a fixed sharding/eviction policy.

It is not required for semantic reproducibility.

## 33. Active-set lifecycle

Active queue entries are not permanent.

After a committed mutation:
- removed/dead tetra targets are dropped,
- changed/new tetra in affected neighborhood get new exact quality,
- neighboring operation opportunities are regenerated,
- unchanged distant entries remain valid only if their priority basis did not depend on changed state.

First implementation may conservatively rebuild a bounded affected neighborhood.

Do not retain an active item only because its old TetHandle generation remains live.

## 34. Count-based budgets

Parallel scheduling must not use wall time as deterministic algorithm truth.

For reference/qualification modes use count-based limits:
- proposals planned,
- cavity size,
- branch-and-bound nodes,
- exact comparisons,
- optimization iterations.

Wall-clock budgets may exist only in explicitly nondeterministic/performance modes or as an external
abort signal with result semantics that do not claim exhaustive optimization.

## 35. Determinism levels

Research distinguishes:

### D0 — arithmetic determinism

Same exact input -> same D26QMR1 result.

### D1 — candidate determinism

Same snapshot -> same candidate set and ScheduleKeys.

### D2 — round determinism

Same snapshot -> same conflict graph/winner set.

### D3 — commit determinism

Same winner set -> same canonical committed semantic mesh state.

### D4 — portable optimizer determinism

Same canonical input/policy -> same final canonical mesh fingerprint across supported thread counts and
Debug/Release.

M6 should qualify these levels separately.

## 36. Literature relevance

### Parallel tetra improvement

Wang et al. (2024) combine atomic cavity ownership and graph coloring, explicitly treating overlapping
local-operation regions as conflict sources and reporting substantial shared-memory speedup.

Use:
- evidence that local cavity parallelism is practical,
- evidence that topology-changing operations need conflict management.

Do not copy:
- memory layout,
- exact ownership token protocol,
- scheduling heuristics.

### Parallel local reconnection

Shang et al. (2016) use Hilbert ordering/decomposition to reduce interference between local
reconnection regions.

Use:
- evidence that spatial separation can reduce conflicts.

Do not treat Hilbert ordering as Dynamics26 deterministic authority.

### Deterministic Galois

Nguyen, Lenharth and Pingali (2014) emphasize portable deterministic execution independent of
machine-dependent scheduling parameters.

Galois documentation describes deterministic rounds selecting conflict-free active elements.

Use:
- architecture evidence for deterministic round-based irregular parallelism.

Dynamics26 derives its own schedule/keys and does not depend on Galois runtime/source.

## 37. Proposed telemetry

    quality_key_builds
    quality_key_cache_hits
    quality_key_cache_misses
    quality_key_invalidations_coordinate
    proposal_count
    proposal_conflict_edges
    winner_count
    deferred_conflict_count
    planning_parallelism_ratio
    exact_fallback_by_operation
    round_count
    active_rebuild_count
    max_read_footprint
    max_write_footprint.

For deterministic reference:
- counts should be reproducible for fixed algorithm/thread-independent planning if specified,
- wall time is performance telemetry only.

## 38. Negative controls

### Stale PointId-only smoothing cache

Reuse old q_MR key after moving one vertex.

Expected:
- mismatch with exact recomputation.

### Handle-only proposal validity

Keep a proposal after unrelated/local topology revision with handles still live.

Expected:
- snapshot/revision policy rejects or replans if any read-set state changed.

### Thread-completion-order scheduler

Assign winner priority by which worker finishes first.

Expected:
- repeated/thread-count runs can choose different conflict winners.

This policy is forbidden.

### Hash-iteration candidate order

Build proposal order from unordered container traversal.

Expected:
- permutation/seed implementation can change result.

This policy is forbidden.

### False non-conflict

Deliberately omit an outside-neighbor patch record from topology write footprint.

Expected:
- verification detects overlapping semantic writes or topology mismatch.

## 39. Research gates

    M6-R125
      CanonicalTetKey quality identity is invariant to TetHandle slot/generation for fixed coordinates

    M6-R126
      visitEpoch never changes/cache-keys exact q_MR identity

    M6-R127
      connectivity-only mutation preserves reusable canonical tetra quality keys

    M6-R128
      accepted smoothing invalidates every incident tetra quality key

    M6-R129
      constraint-only mutation leaves numeric q_MR cache separate from proposal legality cache

    M6-R130
      cache-enabled/disabled runs produce identical exact comparisons and topology

    M6-R131
      cache allocation failure cannot become geometry/quality truth

    M6-R132
      round snapshot freezes topology/coordinate/constraint/policy state for proposal planning

    M6-R133
      every proposal declares complete semantic read/write footprints

    M6-R134
      conflict relation detects all write-read/write-write overlaps in fixture oracle

    M6-R135
      deterministic ScheduleKey is independent of pointer/thread/hash/completion order

    M6-R136
      greedy reference conflict selection returns identical winner set across proposal enumeration permutations

    M6-R137
      selected non-conflicting proposals satisfy semantic commutativity/serializability fixtures

    M6-R138
      no claim of equivalence is made to regenerate-after-every-commit serial optimizer without separate proof

    M6-R139
      ordered commit reproduces canonical semantic fingerprint across thread counts

    M6-R140
      smoothing conflict fixtures agree with affected-star/read-coordinate oracle

    M6-R141
      topology-changing cavity footprints include all exterior adjacency writes

    M6-R142
      old-round proposals are discarded/replanned after commit round

    M6-R143
      proposal-local cache strategy is race-free and result-equivalent to no cache

    M6-R144
      deterministic reference budgets use count units rather than wall time

    M6-R145
      Debug/Release and 1/N-thread runs produce the same final canonical optimizer fingerprint

    M6-R146
      any future parallel commit with deterministic slot preassignment matches ordered-commit reference.

These are research/verification-design gates only.

## 40. Policy placeholders

Research-only names:

    D26QKEY1
      exact quality-key validity/cache domain

    D26QSCHED1
      round-based deterministic optimizer schedule.

They are not public API contracts.

## 41. Current conclusion

The safest path to parallel M6 is not:

    multiple threads mutate the mesh and resolve races afterward.

It is:

    immutable snapshot
      ->
    parallel private planning
      ->
    deterministic conflict-free selection
      ->
    ordered validated commit
      ->
    local invalidation/rebuild.

Likewise, the safest first cache is not a global handle-keyed store.

It is:

    coordinate-state-aware local memoization.

This keeps:
- exact quality truth,
- topology storage identity,
- constraint legality,
- scheduling determinism

as separate engineering concerns.

No production M6 optimizer or cache implementation is authorized by this document.


## 42. Active-set invalidation is dependency-based, not radius folklore

A committed mutation has a semantic write set:

    W_commit.

A previously planned item/proposal is stale when its semantic state depends on a changed resource.

Reference invalidation oracle:

    stale(i)

if:

    R_i intersects W_commit

or:

    W_i intersects W_commit.

This is the same dependency logic used by conflict detection.

It avoids encoding an unproved geometric rule such as:
- always one ring,
- always two rings.

A local star/ring implementation is allowed only as a conservative realization of the semantic
dependency oracle.

## 43. Reference active-set rebuild

For qualification, define an expensive but simple oracle:

    D26ACTREF1
      =
    rebuild eligible active targets from all live finite tetrahedra
    after every committed round.

This is deterministic and independent of an incremental invalidation implementation.

A production incremental active-set strategy must match D26ACTREF1 in:
- eligible target set,
- canonical target ordering,
- final optimizer result

for the same activation/schedule policy.

Over-invalidation is allowed.

Under-invalidation is a correctness bug.

## 44. Activation threshold boundary

Operation scheduling and target activation are separate policies.

D26OPS1 defines:
- operation order/escalation.

It does not freeze a universal numeric statement such as:

    q_MR < 0.2 => bad.

Reference exhaustive qualification may activate all finite tetrahedra.

Product/solver-aware low-tail activation thresholds remain future evidence-driven policy.

If a targeted activation policy is used, the final claim is:

    no accepted improvement among activated targets under D26OPS1,

not necessarily:

    globally locally optimal over every tetrahedron.
