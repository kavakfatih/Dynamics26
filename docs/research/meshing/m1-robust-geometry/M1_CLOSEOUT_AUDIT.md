# M1 Closeout Audit — Robust Geometry Foundation

**Audit date:** 2026-09-05  
**Audited HEAD:** `f1e3ab433d94912a4b5fd51e539150edfb680022`  
**Decision:** **M1 NOT YET QUALIFIED — M2 BLOCKED**

## 1. Purpose

This audit is the mandatory gate between the M1 robust-geometry program and M2 serial point-cloud Delaunay development.

A green CI run alone is not sufficient. The audit checks:

- research/specification completeness,
- executable mathematical evidence,
- deterministic degeneracy contracts,
- topology primitive readiness,
- test/replay completeness,
- compiler and clean-room constraints,
- commercial benchmark continuity,
- documentation/source-of-truth consistency.

## 2. Verified executable evidence

### Predicate/oracle chain — PASS

Implemented and verified:

```text
Python Oracle A — exact Fraction
Python Oracle B — exact dyadic integer
        ↓
D26PRED raw binary64 fixtures
        ↓
C++ exact dyadic RobustPredicates
        ↓
M1.8 certified fast filter
        ↓
exact fallback
```

Predicates:
- orient2d,
- orient3d,
- incircle,
- insphere.

### CI evidence — PASS

- M1.6 corrected exact-head: `f5fc1eb842fb...`
  - macOS arm64 workflow #231: SUCCESS.
- M1.7 exact kernel exact-head: `4794d68c092b...`
  - macOS arm64 workflow #233: SUCCESS.
- M1.8 certified fast path exact-head: `f1e3ab433d94...`
  - macOS arm64 workflow #234: SUCCESS.
  - Debug build-test: SUCCESS.
  - Release build-test: SUCCESS.
  - full workflow: SUCCESS.

M1.8 tests include committed and deterministically generated exact-oracle corpora.

## 3. Closeout gate matrix

| Gate | Requirement | State | Evidence / blocker |
|---|---|---|---|
| G01 | CAD tolerance != topological predicate | PASS | frozen research/implementation contract |
| G02 | Exact sign conventions for four predicates | PASS | dual oracle + golden fixtures |
| G03 | Independent exact oracle A/B | PASS | executable Python paths |
| G04 | Bit-exact binary64 fixture round-trip | PASS | D26PRED + C++ strict reader |
| G05 | Production/reference exact predicate kernel | PASS | independent C++ dyadic big-integer determinant |
| G06 | Certified fast filter with exact fallback | PASS | M1.8 + generated oracle corpus |
| G07 | Fast-math / FP contraction contract | PASS | compile guard + source compile flags |
| G08 | Apple Silicon Debug/Release equality | PASS | workflow #234 |
| G09 | Clean-room implementation boundary | PASS | no external mesher/predicate runtime/source dependency in kernel |
| G10 | Exact duplicate canonicalization executable test | **FAIL** | policy documented only |
| G11 | Affine dimension classifier executable test | **FAIL** | policy documented only |
| G12 | Formal symbolic perturbation oracle | **FAIL** | theory documented, no executable oracle |
| G13 | Stable PointId perturbation hierarchy frozen | **FAIL** | proposed, not executable/frozen |
| G14 | M1 tetra primitive/handle/face-key validator | **FAIL** | M1.4 data contract only |
| G15 | Failure replay record round-trip | **FAIL** | M1.5 contract only |
| G16 | Structured adversarial/metamorphic corpus | **PARTIAL** | canonical/scale/random exist; permutation/translation/near-degenerate families incomplete |
| G17 | Fast/fallback telemetry and baseline | **PARTIAL** | both paths exercised; ratio/performance baseline not recorded |
| G18 | M1 documentation/status synchronized to code | **FAIL** | M1.6 and implementation spec still stale |
| G19 | ANSYS / COMSOL / Marc benchmark continuity | PASS | public behavior remains external product benchmark, not predicate oracle |
| G20 | M2 starts only after M1 gate closure | PASS policy | ADR-MESH-0011 |

## 4. Commercial benchmark sanity check

The M1 closeout does not use commercial products as mathematical predicate authorities.

### ANSYS

Current ANSYS 2026 R1 public meshing documentation continues to expose explicit mesh/geometry workflow, repair and quality-management behavior. Its commercial internals do not provide a public exact-predicate implementation specification.

### COMSOL

COMSOL 6.4 publicly exposes mesh statistics, minimum/mean quality, quality histograms, growth rates, warning/problem state and Free Tetrahedral quality optimization.

This continues to support:

```text
mesh generated
!=
mesh quality accepted
!=
analysis qualified
```

### Marc / Mentat

Marc remains a benchmark for nonlinear meshing, remeshing/adaptivity and engineering workflow behavior. Public material reviewed during M1 does not disclose an internal exact-predicate implementation suitable as an oracle.

### Audit conclusion

Commercial comparison does not change the M1 mathematical design.

Dynamics26 remains responsible for its own:
- exact predicate verification,
- deterministic degeneracy policy,
- topology validation,
- later solver qualification.

## 5. M1.9 mandatory hardening work

M2 is blocked until all mandatory items below close.

### M1.9-A — Replay + adversarial verification

Implement:
- one-case failure replay record,
- replay reader,
- deterministic structured near-degenerate families,
- permutation tests,
- power-of-two scale tests,
- exact-representable translation tests,
- promotion of failures to permanent regression fixtures.

### M1.9-B — Degeneracy executable foundation

Implement:
- exact duplicate canonicalization,
- signed-zero normalization at site identity boundary,
- affine-dimension classifier,
- stable PointId assignment contract,
- test-only formal symbolic perturbation oracle,
- co-spherical/coplanar symbolic fixtures.

Production Delaunay tie handling remains M2, but M2 must have an independent M1 symbolic oracle before it begins.

### M1.9-C — Tetra primitive foundation

Implement without Delaunay insertion:
- `PointId`,
- `TetHandle {slot,generation}`,
- `TetRecord` local-face convention,
- canonical face key,
- reciprocal-neighbor validator,
- stale-handle tests,
- deliberate corruption corpus.

Point-location walking and cavity insertion remain M2.

### M1.9-D — Predicate telemetry

Record:
- total calls,
- fast-certified,
- exact fallback,
- exact zero,
- invalid input.

Add a Release research baseline for committed/generated corpus.

No performance threshold may weaken correctness.

### M1.9-E — Documentation/ADR synchronization

Update:
- M1/M1.x state board,
- M1 implementation specification,
- ADR-MESH-0005,
- ADR-MESH-0008,
- ADR-MESH-0010,
- current source locations and compiler contract.

## 6. M1 final qualification criteria

M1 becomes `QUALIFIED` only when:

1. G01–G20 mandatory gates are PASS or explicitly deferred to M2 by accepted ADR,
2. M1.9 executable tests pass in Debug and Release on macOS arm64,
3. full repository CI is green at the closeout exact HEAD,
4. clean-room/source-boundary review remains clean,
5. a second closeout audit finds no unresolved M1 blocker.

## 7. M2 gate

Current state:

```text
M1 = VERIFYING / CLOSEOUT HARDENING
M2 = BLOCKED BY M1 CLOSEOUT
```

No Bowyer-Watson, cavity insertion or point-cloud tetrahedralization implementation should be committed before this gate is removed.
