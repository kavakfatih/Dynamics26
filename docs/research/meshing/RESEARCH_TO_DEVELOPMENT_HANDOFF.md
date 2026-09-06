# Original Meshing Engine — Research to Development Handoff

**Date:** 2026-09-06  
**Purpose:** convert existing meshing R&D into an implementation-ready engineering baseline without
misrepresenting prototypes as product milestones.

## 1. Current product boundary

The UI/UX/GUI hardening line is V1.1.0-rc.1 and is under feature freeze except for engineering
blockers/serious defects. The next major engineering program is V1.2 Original Meshing Engine.

Production meshing development is considered to start only when DEV-MESH-P1 begins.

## 2. What the research has established

### M0 — Knowledge, clean-room and architecture

Available:

- source registry,
- algorithm taxonomy,
- commercial behavior benchmarks,
- open-source architecture studies,
- clean-room policy,
- original Dynamics26 mesher architecture,
- decision log,
- benchmark/experiment framework.

Development consequence:

- Dynamics26 owns its production mesher implementation,
- third-party meshers are research/benchmark references,
- no source copying/transliteration,
- CAD/display/FEM representations remain separate.

### M1 — Robust geometry foundation

Status: QUALIFIED in its declared scope.

Established:

- finite binary64 input contract,
- exact dyadic oracle model,
- exact/certified Orient2D/Orient3D/InCircle/InSphere,
- conservative fast path with exact fallback,
- exact-zero behavior,
- deterministic identity/degeneracy foundation,
- tetra handle/opposite-face/canonical-face primitives,
- replay/adversarial/CI harness,
- caller-owned predicate telemetry.

Development reuse:

- P1 must reuse these qualified mechanisms,
- topology decisions must not introduce epsilon heuristics,
- M1 point-location/cavity text is design input only; production search/mutation belongs to P1/M2.

### M2 — Delaunay point-cloud tetrahedralization research

Status: M2.0 DESIGN FROZEN; P1A/P1B/P1C QUALIFIED IN DECLARED SUBSCOPES.
M2 OVERALL NOT QUALIFIED; serial insertion/constructor qualification remains open.
See `m2-delaunay/M2_1C_IMPLEMENTATION_RECORD.md` for source SHA and macOS CI evidence.

Established:

- reference incremental architecture,
- semantic predicate boundary,
- point-location contracts,
- cavity transaction model,
- cell storage/mutation model,
- deterministic policy/versioning,
- patch orientation/stitching,
- resource/complexity model,
- experiment plan.

Development reuse:

- P1A semantic Delaunay predicates (qualified; oblique metric correction ADR-MESH-0043),
- P1B typed finite/ghost bootstrap (qualified in bootstrap scope),
- P1C brute-force exact point location (qualified; M2-G06),
- P1D cavity oracle + Plan -> Validate -> Commit transactional patch (next),
- P1E deterministic adjacency walk,
- P1F serial constructor determinism / fingerprint / replay.

This sequence supersedes the earlier abbreviated handoff list and matches the
frozen M2 phase boundaries and current development plan; no phase is skipped.

### M3 — CAD curve/surface meshing research

Status: not yet mature enough for production implementation.

Known direction:

- canonical CAD-edge discretization,
- Face meshing owned by mesher, not display tessellation,
- shared-edge conformity,
- persistent Face provenance,
- parameter-space constrained Delaunay as primary candidate,
- advancing-front as possible independent comparison.

Research required during P2:

- OCCT p-curve/trim-loop failure modes,
- singular/periodic surfaces,
- seam handling,
- geometric deviation metrics,
- robust constrained triangulation policy.

### M4 — Boundary recovery and volume meshing research

Status: development concept defined; detailed production research still required.

Known direction:

- watertight triangulated boundary,
- tetra initialization,
- segment/facet recovery,
- volume classification,
- exact boundary provenance,
- explicit failure instead of silent repair.

Research required during P3:

- recovery operation ordering,
- recovery termination/failure classes,
- non-manifold/near-coincident CAD boundary handling,
- robust interior classification,
- recovery-quality interaction.

### M5 — Size-field research

Status: scoped, not implemented as original unstructured mesher capability.

Known direction:

- global/local/curvature/proximity sizing,
- gradation limiter,
- Named Selection / geometry-scoped controls.

Research required during P4:

- field-composition algebra,
- curvature/proximity sampling policy,
- gradation enforcement,
- complexity bounds,
- deterministic insertion/refinement order.

### M6 — Tetra quality / deterministic optimizer research

Status: DESIGN FROZEN under `D26-M6-FREEZE-1`.

Established research decisions:

- D26QMR1 exact mean-ratio order,
- D26INT1 certified interval backend,
- D26QMRF1 exact-fallback filter architecture,
- D26QV1 exact sorted quality vector,
- D26QACC1 strict replacement acceptance,
- local-to-global monotonicity,
- O1..O5 operation schedule,
- full-D26QV1 edge-removal DP,
- deterministic scheduler and active-set contracts,
- finite-state termination boundary,
- 170 contiguous qualification gates,
- no universal FEM-good qMR threshold,
- nonlinear/rubber suitability remains solver/formulation dependent.

### M6 executable research evidence already present

The following code was created while still in the research/qualification program:

| Evidence | Representative commit |
|---|---|
| private shared exact arithmetic extraction | `c13a3d63...` |
| exact qMR comparator | `43ea99aa...`, hardening `16b84ed8...` |
| interval backend | `e55e20a6...` |
| certified qMR filter | `a67b0454...` |
| D26QV1 / D26QACC1 | `d0f442ae...`, fix `57910c0b...` |
| tetra optimizer state | `9e822f17...`, validation fix `88a53f7c...` |
| smart-smoothing proposal | `0a6b3a9b...` |
| transactional smoothing commit | `01b9241b...` |

Exact-head workflow #287 on `01b9241b242d7141f51bf8a72185d3603312b4df` completed successfully.

Classification rule:

> This is reusable research implementation evidence, not proof that the production quality-optimizer
> phase or the general volume mesher is complete.

When DEV-MESH-P5 begins, this code is re-audited against the qualified P3/P4 mesh state and
provenance/constraint model. It may be reused, adapted or replaced. Relevant M6 gates must still be
closed with executable evidence.

### M7 — Product TET4 qualification

Status: not started.

Research/product questions intentionally deferred:

- TET4 solver formulation suitability,
- patch/convergence/distortion behavior,
- geometric quality vs conditioning/interpolation correlation,
- arbitrary CAD end-to-end scope/provenance,
- GUI diagnostics,
- nonlinear/rubber qualification boundaries.

## 3. Frozen implementation invariants safe to carry forward

These are development inputs unless superseded by explicit ADR:

1. `CAD Geometry != Display Tessellation != FEM Mesh`.
2. CAD/B-Rep identity is authoritative for persistent geometry scope.
3. Mesh topology decisions use qualified exact/certified predicates, not geometric epsilons.
4. Mutation uses Plan -> Validate -> Commit.
5. PointId/GeometryEntityId/TetHandle have distinct semantic roles.
6. No assumption that PointId equals storage index.
7. Determinism is semantic: no pointer/thread/completion-order priority.
8. Constraint/provenance uncertainty fails conservatively.
9. Quality optimization cannot override exact validity or protected topology.
10. Solver suitability is not inferred from geometric quality alone.
11. Unsupported geometry/operation fails explicitly; no hidden bounding-box or repair substitution.
12. Every product capability requires implementation + tests + qualification + integration evidence.

## 4. Research artifacts by development phase

| Development phase | Primary research authority | Research status at handoff |
|---|---|---|
| P1 Point-cloud Delaunay | M1 + M2 | strong / implementation-ready |
| P2 CAD surface | M3 + M1 predicates | partial; just-in-time research required |
| P3 Boundary recovery | M4 + M2 topology | partial; deep research required |
| P4 Sizing/provenance | M5 + existing Geometry/Named Selection contracts | scoped; research required |
| P5 Quality optimizer | M6 | deep/frozen; partial executable evidence exists |
| P6 Product TET4 | M7 + solver/FEM research | not started |
| P7 TET10 | M8 | deferred |

## 5. Current production-development scope

P0 has closed and P1A/P1B/P1C have passed their declared qualification gates.
The next single implementation target is:

```text
DEV-MESH-P1D
Cavity oracle + transactional patch
```

P1D is not implemented by the P1C closeout. M2 overall remains NOT QUALIFIED.

Reason:

- P1 is on the critical dependency path to a real tetra volume mesher,
- M6 prototypes are ahead of their integration dependency,
- continuing O2/O3/O5 now would deepen an optimizer without an authoritative production volume-mesh
  state to optimize.

M6 research may continue in parallel when it answers a concrete future integration/qualification
question, but production coding priority stays on P1 -> P2 -> P3 -> P4 -> P5.

## 6. Historical development-entry checklist

The following records the original handoff checklist before the first P1 source
commit; its unchecked items are historical, not current P1C status. Current
source/CI evidence is recorded in the P1A/P1B/P1C implementation records.
For every next package, re-read authority and verify current main and CI again.

- [x] M1 qualified in declared scope,
- [x] M2.0 reference architecture frozen,
- [x] clean-room/source boundary documented,
- [x] benchmark/experiment framework exists,
- [x] M6 research preserved separately,
- [ ] P0 plan/handoff exact-head CI green,
- [ ] current P1A authority documents re-read against main,
- [ ] exact P1A gate list selected,
- [ ] no newer main commit invalidates the starting assumptions.

## 7. Research loop during development

Development does not end research.

For each unresolved implementation question:

1. state the exact engineering question,
2. inspect current repository reality,
3. research theory/vendor/open-source evidence as appropriate,
4. update source registry,
5. record Adopt/Adapt/Reject,
6. add or supersede ADR only if semantics change,
7. define oracle/fixture,
8. implement,
9. qualify on exact HEAD,
10. record what remains unknown.

This is the default Dynamics26 meshing workflow from P1 onward.
