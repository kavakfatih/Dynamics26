# M6 Implementation Specification — Frozen Research to Executable Reference

**Status:** IMPLEMENTATION SPEC FROZEN / PRODUCTION CODE NOT STARTED  
**Baseline:** D26-M6-FREEZE-1  
**Repository baseline:** 57935427fd4ac53b538589e261bd2e11583b1adb  
**Date:** 2026-09-06

## 1. Objective

Translate the frozen M6 research contracts into an executable, independently verifiable reference
implementation inside the existing `femcae_meshing` library without:
- changing M6 mathematical semantics,
- exposing internal arbitrary-precision arithmetic as public ABI,
- coupling M6 to GUI/pre-post code before qualification,
- silently changing M1 exact-predicate behavior.

The first implementation target is correctness/replay/qualification, not maximum throughput.

## 2. Frozen semantic dependencies

This specification implements, but does not redefine:

    D26QMR1
    D26QV1
    D26QACC1
    D26QMRF1
    D26INT1
    D26QMRB1
    D26QKEY1
    D26QSCHED1
    D26ACTREF1
    D26OPS1.

Authoritative semantics remain in:

    M6_DESIGN_FREEZE.md.

If implementation convenience conflicts with that document, implementation convenience loses.

## 3. Current repository integration facts

Current production/research foundation already contains:

    include/femcae/meshing/RobustPredicates.h
    src/meshing/RobustPredicates.cpp
    include/femcae/meshing/RobustGeometry.h
    src/meshing/RobustGeometry.cpp
    include/femcae/meshing/TetraTopology.h
    src/meshing/TetraTopology.cpp.

Relevant existing contracts:
- PointId and CanonicalSite are defined by RobustGeometry,
- TetHandle/TetRecord/TetSlot and face/topology validation are defined by TetraTopology,
- M1 exact dyadic BigInt arithmetic is private inside RobustPredicates.cpp,
- `include/` is installed by CMake,
- `femcae_meshing` is the correct library boundary,
- current SimulationMesh/MeshElement remains Hex8/Quad4-oriented and is not the first M6 authority.

Therefore the M6 reference optimizer must not be implemented by prematurely extending GUI/pre-post
SimulationMesh semantics.

## 4. Public-versus-private boundary

### 4.1 First implementation phases Q0-Q9

Keep M6 implementation private to `femcae_meshing`.

Private headers live under:

    src/meshing/internal/
    src/meshing/m6/.

They are:
- not installed,
- not public ABI,
- available only to femcae_meshing and dedicated M6 test targets.

### 4.2 Public API timing

Do not create an installed optimizer API before deterministic/reference qualification.

After Q9, a separately reviewed facade may expose:
- read-only tetra quality diagnostics,
- optimizer request/result/status,
- versioned telemetry.

It must never expose:
- SignedBigInt,
- DyadicValue,
- exact numerator/denominator internals,
- private proposal/cache/scheduler state.

## 5. Proposed source tree

Reference implementation layout:

    src/meshing/
    ├── internal/
    │   └── exact/
    │       ├── SignedBigInt.h
    │       ├── SignedBigInt.cpp
    │       ├── Binary64Dyadic.h
    │       ├── Binary64Dyadic.cpp
    │       ├── ExactIntegerCoordinates.h
    │       └── ExactIntegerCoordinates.cpp
    │
    └── m6/
        ├── quality/
        │   ├── MeanRatioExact.h
        │   ├── MeanRatioExact.cpp
        │   ├── IntervalArithmetic.h
        │   ├── IntervalArithmetic.cpp
        │   ├── MeanRatioFilter.h
        │   ├── MeanRatioFilter.cpp
        │   ├── QualityVector.h
        │   ├── QualityVector.cpp
        │   ├── QualityAcceptance.h
        │   ├── QualityAcceptance.cpp
        │   └── QualityTelemetry.h
        │
        ├── state/
        │   ├── TetraOptimizationState.h
        │   ├── TetraOptimizationState.cpp
        │   ├── ConstraintView.h
        │   ├── QualityKeyCache.h
        │   └── QualityKeyCache.cpp
        │
        └── optimizer/
            ├── OperationTypes.h
            ├── SmartSmoothing.h
            ├── SmartSmoothing.cpp
            ├── Flip23.h
            ├── Flip23.cpp
            ├── EdgeRemovalDP.h
            ├── EdgeRemovalDP.cpp
            ├── OptimizationSmoothing.h
            ├── OptimizationSmoothing.cpp
            ├── BoundedSpr.h
            ├── BoundedSpr.cpp
            ├── ActiveSet.h
            ├── ActiveSet.cpp
            ├── DeterministicScheduler.h
            ├── DeterministicScheduler.cpp
            ├── ReferenceOptimizer.h
            └── ReferenceOptimizer.cpp.

Exact filenames may be consolidated during implementation, but semantic ownership must remain equivalent.

## 6. Shared exact arithmetic extraction

### 6.1 Goal

Extract arithmetic mechanism from RobustPredicates.cpp without changing M1 predicate behavior.

Current private M1 mechanism includes:
- signed 32-bit-limb BigInt,
- binary64 dyadic decode,
- common-exponent exact integer coordinate conversion,
- exact integer determinant helpers.

### 6.2 Frozen extraction rule

Move mechanism, not policy.

Allowed shared internal layer:

    femcae::meshing::internal::exact

must know nothing about:
- Orient3D semantics,
- q_MR semantics,
- optimizer policy,
- topology mutation.

M1 RobustPredicates continues to own:
- predicate expression construction,
- sign semantics,
- exact-zero behavior,
- telemetry.

M6 MeanRatioExact owns:
- d,
- s,
- d^2/s^3 comparison graph.

### 6.3 No public leakage

Because CMake installs `include/` wholesale, exact arithmetic private headers must not be placed under
`include/femcae`.

### 6.4 Extraction acceptance

The extraction commit is accepted only if:
- M1 predicate golden corpus is unchanged,
- fast/exact path counts remain within the documented semantic contract,
- exact zeros remain exact zeros,
- Debug/Release M1 tests pass,
- no public header gains BigInt types.

## 7. M6 tetra optimization state

The first optimizer should own an internal state derived from the M1/M2 tetra foundation.

Conceptual:

    QualityPointState
      - PointId id
      - geometry::Vec3 point

    TetraOptimizationState
      - vector<QualityPointState> points
      - vector<TetSlot> tetraSlots
      - immutable/explicit constraint view
      - round-local metadata.

### 7.1 PointId lookup

Do not assume:

    PointId == vector index + 1

inside M6 algorithms, even if current canonicalization happens to assign sequential IDs.

Provide one validated point-coordinate lookup abstraction.

Reason:
- keeps M6 independent of future PointId allocation implementation,
- improves fixture corruption testing,
- avoids accidental ABI/persistence assumptions.

### 7.2 CanonicalSite copy boundary

Initialize QualityPointState from CanonicalSite.

Do not recanonicalize after smoothing.

PointId remains stable inside the optimization request while current coordinates move.

### 7.3 SimulationMesh boundary

Do not mutate SimulationMesh directly in the first M6 reference implementation.

A later M2/M4 volume-mesh adapter may translate:
- qualified volume sites,
- qualified TetSlot topology,
- CAD/provenance constraints

into TetraOptimizationState.

## 8. Constraint view

First M6 operation code needs an explicit legality input rather than hidden GUI/CAD access.

Reference internal constraint representation may contain:

    PointMobility
      - InteriorFree
      - Fixed

    ProtectedEdgeKey
      - sorted PointId pair

    ProtectedFaceKey
      - CanonicalFaceKey.

First implementation rule:
- CAD/boundary point coordinates are fixed,
- protected edges/faces cannot be replaced,
- unknown/unresolved provenance is conservative ConstraintBlocked.

M3/M4 can later supply authoritative CAD mobility/provenance through an adapter.

## 9. D26QMRB1 — exact mean-ratio key

### 9.1 Internal key

Conceptual private representation:

    ExactMeanRatioKey
      - BigInt determinantMagnitude
      - BigInt edgeSum

or lazily promoted:

    d
    s
      ->
    d^2
    s^3.

It is not public API.

### 9.2 Construction

For one positive tetra:
1. gather four current binary64 Vec3 coordinates,
2. use shared exact dyadic common-exponent integer conversion,
3. form exact relative edge differences,
4. compute exact 3x3 determinant magnitude d,
5. compute exact six-edge squared sum s,
6. require d>0 and s>0 for quality-key construction.

Geometry validity remains checked independently with M1 Orient3D.

### 9.3 Comparison

For keys A,B:

    lhs = d_A^2 * s_B^3
    rhs = d_B^2 * s_A^3.

Return exact:
- Less,
- Equal,
- Greater.

Do not compute:
- cube root,
- division,
- decimal q_MR,
- gcd for correctness.

## 10. D26QMRF1 / D26INT1

### 10.1 Result type

Private comparison result:

    MeanRatioOrder
      - Less
      - Equal
      - Greater

    MeanRatioEvaluationPath
      - IntervalCertified
      - ExactFallback.

D26INT1 itself never returns Equal as a certified fast result.

### 10.2 Certified source isolation

Certified floating-expression source files require:
- no fast-math,
- no unproved reassociation,
- explicit `-ffp-contract=off` or qualified equivalent,
- FE_TONEAREST environment guard,
- gradual-subnormal capability guard.

Current CMake applies certified FP flags to RobustPredicates.cpp.

Implementation should generalize this into a source list such as:

    FEMCAE_CERTIFIED_FP_SOURCES

including:
- RobustPredicates.cpp,
- IntervalArithmetic.cpp,
- MeanRatioFilter.cpp.

### 10.3 Fast path

The interval expression tree is frozen by D26INT1 research.

If:
- environment unsupported,
- non-finite intermediate,
- interval contains zero,

return Uncertain and call D26QMRB1.

## 11. QualityVector implementation

### 11.1 Concept

A D26QV1 vector is a sorted sequence of tetra quality entries.

It need not expose an irrational scalar.

Reference entry can contain:

    CanonicalTetKey
    ExactMeanRatioKey.

Sort primarily by D26QMR1 exact order.

If exact quality tie:
- canonical tetra identity may stabilize representation,
- canonical identity does not make equal quality a strict improvement.

### 11.2 Vector comparison

Compare entry qualities worst-to-best.

If all compared entries tie and one vector ends:
- shorter vector is better.

This exact-prefix rule must be implemented explicitly and tested independently.

## 12. D26QACC1

Reference API concept:

    AcceptanceDecision evaluateReplacement(
        oldCells,
        newCells,
        pointState,
        constraints,
        telemetry);

Decision sequence:
1. validate old/new structural assumptions,
2. exact-positive Orient3D for every new tetra,
3. topology/cavity boundary equivalence,
4. constraint/provenance legality,
5. construct old/new D26QV1,
6. strict new>old only,
7. otherwise no commit.

Acceptance must be side-effect free.

Mutation occurs only after a positive decision.

### Decision outcomes

    Accept
    NoImprovement
    InvalidCandidate
    ConstraintBlocked
    InvalidPriorState

`InvalidPriorState` reports that the cells being replaced were already invalid,
and is step 1's answer for the old side. Before it existed, an invalid prior
cell was noticed only when the D26QV1 build threw at step 5; that throw was
caught and reported as `InvalidCandidate`, so "your replacement is bad" and "the
mesh handed in was already bad" were one answer under one counter, with the
second attributed to the first.

Refusing either way is the frozen policy — SMOOTHING_UNTANGLING_AND_CAD_CONSTRAINTS.md
section 5 keeps untangling outside the normal M6 path, so an invalid mesh from
M2/M4 is a construction failure and is not silently healed — but that same
section requires its provenance and failure semantics to be explicit, and one
shared outcome does not satisfy that. This follows the M6_DESIGN_FREEZE.md
section 14 rule that a failure of one kind is never reported as a failure of
another.

When both sides are invalid the prior state is reported, because it names the
condition that has to be fixed first.

Old-side validation is skipped when the caller supplies an already-built
old-side D26QV1 vector: constructing that vector performed the same check, and
re-running it per candidate is the per-sample cost the reuse path exists to
remove. Supplying a prepared vector therefore transfers this validation duty to
the caller.

## 13. O1 smart smoothing

Reference O1 proposal generator:
1. select one InteriorFree PointId,
2. gather incident tetra star,
3. form one deterministic smart/Laplacian proposal,
4. optional count-bounded binary64 line-search samples,
5. reject any sample that creates non-positive incident tetra,
6. evaluate incident-star D26QV1,
7. retain best strict candidate under frozen deterministic proposal ordering.

No authoritative coordinate mutation occurs during search.

## 14. O2 2->3 flip

Plan:
- identify legal interior face shared by exactly two tetra,
- build three candidate tetra around complementary edge,
- exact-positive orientation,
- preserve cavity boundary/protected topology,
- evaluate 2-cell versus 3-cell D26QV1,
- produce immutable proposal only if strict.

Commit:
- uses M2-style Plan -> Validate -> Commit transaction,
- patches outside reciprocal adjacency explicitly.

## 15. O3 full D26QV1 edge-removal DP

The frozen O3 implementation does **not** use scalar max-min as its authoritative recurrence.

For edge star link polygon v_i..v_j:

    W(i,k,j)
      =
    sorted exact quality vector
    of the two pole tetra contributed by triangle (i,k,j).

Base:

    V[i,i+1] = empty.

Candidate split:

    C(i,k,j)
      =
    merge_sorted(
      V[i,k],
      V[k,j],
      W(i,k,j)
    ).

Recurrence:

    V[i,j]
      =
    max_D26QV1 over valid k
      C(i,k,j).

The recurrence has optimal substructure because D26QV1 is compatible with multiset union.

Reference implementation may store full vectors for clarity.

Naive complexity:
- O(N^4) with O(N) vector merge/compare inside O(N^3) split enumeration.

Do not optimize representation until exhaustive Catalan-oracle tests pass.

Historical max-min DP is retained as:
- first-component cross-check,
- negative-control/benchmark oracle.

## 16. O4 optimization smoothing

O4 may use a differentiable smooth proposal objective, such as aggregate inverse mean ratio, inside
private count-bounded optimization.

Authoritative acceptance remains:
- final binary64 coordinate,
- exact positive Orient3D,
- strict incident-star D26QV1.

The proposal objective never becomes commit truth.

## 17. O5 bounded SPR

Reference O5:
- deterministic cavity selection,
- fixed boundary/protected constraints,
- fixed point set,
- private triangulation search,
- full D26QV1 final objective,
- lexicographic optimistic upper bound from +infinity padding,
- count-based cavity/branch budgets,
- explicit result status.

Reference search favors correctness over speed.

No production claim may exceed the actually searched cavity/domain.

## 18. Proposal structure

Private conceptual:

    OperationProposal
      - OperationFamily
      - CandidateKey
      - ScheduleKey
      - readFootprint[]
      - writeFootprint[]
      - oldQualityVector
      - newQualityVector
      - proposed point changes
      - proposed tetra delete/create/patch data
      - planning round id
      - search/resource status.

Proposal contains no live pointer identity in semantic ordering.

## 19. Semantic resource keys

Reference deterministic resource namespaces:

    PointCoordinate(PointId)
    TetraCell(TetHandle)
    BoundaryAdjacency(TetHandle,localFace)
    ProtectedEdge(CanonicalEdgeKey)
    ProtectedFace(CanonicalFaceKey).

Resource keys are:
- canonical,
- sorted,
- deterministic.

Read/write footprint construction is part of each operation's qualification.

## 20. D26QSCHED1 reference scheduler

Round:
1. freeze read-only snapshot,
2. construct active targets in canonical order,
3. evaluate proposals; first implementation may be serial even though interface is parallel-ready,
4. sort proposals by ScheduleKey,
5. deterministic greedy conflict-free selection,
6. revalidate selected proposals,
7. commit in ScheduleKey order,
8. discard all old-round proposals,
9. rebuild activity.

Parallel proposal evaluation is introduced only after serial round fingerprint qualification.

## 21. D26ACTREF1

Implement the expensive global reference first:

    rebuild eligible targets from every live finite tetra.

Incremental active invalidation is a later optimization.

Qualification:
- incremental set/order must match D26ACTREF1 for same activation policy.

This avoids embedding one-ring/two-ring assumptions into correctness.

## 22. Activation policy

Reference M6 qualification mode:

    activate every live finite tetra.

Only after correctness qualification may a low-tail performance activation policy be added.

A targeted activation mode must report its scope and cannot claim global local optimality.

## 23. CMake integration

Keep all M6 sources in `femcae_meshing`.

Recommended CMake structure:

    set(FEMCAE_M6_QUALITY_SOURCES ...)
    set(FEMCAE_M6_OPTIMIZER_SOURCES ...)
    list(APPEND FEMCAE_MESHING_SOURCES ...)

Private source headers are not installed.

Dedicated M6 tests receive:

    target_include_directories(test PRIVATE
        ${CMAKE_SOURCE_DIR}/src)

only when they require private reference headers.

Certified floating source options are applied explicitly and audited.

## 24. Test tree

Proposed:

    tests/meshing/m6_quality/
    ├── fixtures/
    │   ├── exact_qmr/
    │   ├── quality_vectors/
    │   ├── edge_stars/
    │   ├── smoothing/
    │   ├── spr/
    │   └── scheduler/
    ├── test_m6_exact_qmr.cpp
    ├── test_m6_interval_backend.cpp
    ├── test_m6_filtered_qmr.cpp
    ├── test_m6_quality_vector.cpp
    ├── test_m6_acceptance.cpp
    ├── test_m6_edge_removal.cpp
    ├── test_m6_smoothing.cpp
    ├── test_m6_spr.cpp
    ├── test_m6_quality_key.cpp
    ├── test_m6_scheduler.cpp
    ├── test_m6_active_set.cpp
    ├── test_m6_full_schedule.cpp
    └── test_m6_thread_replay.cpp.

Independent exact test authority:

    tools/meshing_oracle/m6_quality_oracle.py

using:
- Python exact binary64 ratio decoding,
- Fraction / arbitrary integers,
- no production C++ code reuse.

## 25. Test labels

Use CTest labels such as:

    unit;meshing;m6;quality
    verification;meshing;m6;exact
    verification;meshing;m6;optimizer
    verification;meshing;m6;determinism
    performance;meshing;m6.

Keep exact correctness gates distinct from performance telemetry.

## 26. Qualification mapping

Implementation sequence follows M6_QUALIFICATION_GATE_MATRIX.md.

Minimum phase exits:

### Q0
Shared exact extraction does not regress M1.

### Q1
D26QMRB1 exact comparator passes independent oracle.

### Q2
D26INT1/D26QMRF1 never disagree with exact fallback.

### Q3
D26QV1/D26QACC1 exact local/global order passes.

### Q4
Serial O1-O4 reference operations pass exact cavity/star fixtures.

### Q5
O5 bounded SPR matches exhaustive small-cavity oracle.

### Q6
Cache on/off is semantically identical.

### Q7
Deterministic scheduler produces same selected rounds under enumeration permutations.

### Q8
Incremental active set matches D26ACTREF1.

### Q9
Debug/Release and 1/N thread runs match canonical final fingerprint.

### Q10
M7 solver correlation informs user-facing numeric quality thresholds.

## 27. Telemetry

Caller-owned/reference optimizer telemetry should include:
- D26QMR compare calls,
- interval-certified count,
- exact-fallback count,
- exact-equality count,
- max exact operand width,
- quality-key cache hits/misses,
- proposals by operation family,
- conflicts,
- winners,
- active rebuilds,
- SPR branch nodes,
- budget/resource statuses,
- rounds,
- final low-tail quality distribution.

Telemetry must not alter semantic ordering.

## 28. Error/resource semantics

Separate:
- invalid geometry,
- invalid topology,
- constraint blocked,
- search budget exhausted,
- allocation/resource failure,
- exhaustive no improvement.

Never convert memory/budget failure into:
- quality equality,
- degeneracy,
- successful local optimum.

## 29. Determinism

First reference result must be independent of:
- unordered container iteration,
- pointer values,
- allocation order,
- thread completion order,
- Debug/Release floating variation,
- worker count after Q9.

Canonical output fingerprint should include:
- current point coordinate bits,
- canonical live tetra set,
- relevant protected/provenance state,
- frozen M6 policy versions.

## 30. Implementation non-goals

Do not implement in the first M6 reference phase:
- public GUI quality controls,
- tetra rendering integration,
- adaptive refinement/coarsening,
- CAD boundary smoothing,
- point insertion/deletion,
- anisotropic metrics,
- weighted triangulation,
- solver-driven remeshing,
- GPU kernels.

## 31. Commit discipline

Recommended:
- one phase per small atomic commit,
- exact-head CI after each meaningful phase,
- no branch required; continue directly on main under repository policy,
- do not mix GUI/RC work with M6 core commits when avoidable.

## 32. Implementation readiness conclusion

Repository architecture supports M6 without replacing current meshing library boundaries.

The first executable path should be:

    shared exact arithmetic extraction
      ->
    exact q_MR comparator
      ->
    certified interval filter
      ->
    exact quality-vector acceptance
      ->
    serial O1-O4
      ->
    bounded O5
      ->
    cache
      ->
    deterministic scheduler
      ->
    active-set optimization
      ->
    parallel replay qualification.

Production source changes are authorized only after this specification itself is committed and passes
exact-head CI.
