# Test Tiers and CI Integration

## 1. Existing Dynamics26 infrastructure

Current repository already has:

- `add_femcae_meshing_test`,
- CTest labels,
- Python discovery in test CMake,
- macOS arm64 Debug and Release CI,
- explicit `meshing` verification gate.

M1.5 should extend this model.

## 2. Proposed labels

Core labels:

```text
meshing
robust-geometry
predicate
exact-oracle
filter
degeneracy
tet-topology
point-location
determinism
```

Later M2:

```text
delaunay
cavity
bowyer-watson
performance
```

## 3. Test tiers

### T0 — build/platform contract

Very fast.

Runs every relevant CI build.

Checks:
- platform assumptions,
- compile guard,
- fixture parser.

### T1 — committed golden corpus

Fast deterministic unit tests.

Runs:
- every push affecting meshing/core/CMake,
- Debug and Release.

### T2 — deterministic generated corpus

Moderate.

Example:
- 10k–100k cases/predicate.

Runs:
- hosted macOS gate,
- at least Debug and Release until predicate kernel is qualified.

### T3 — deep stress/fuzz corpus

Large:
- millions of cases,
- broad exponent/adversarial generators.

Runs:
- milestone/manual/nightly-style research gate when infrastructure supports it,
- not required for every GUI-only change.

No scheduled automation is introduced by this research document.

### T4 — performance

Release only.

Tracks:
- ns/predicate,
- fallback ratio,
- walk steps,
- insertion throughput,
- bytes/tet.

Performance regression does not override correctness.

## 4. CTest fixture support

CMake/CTest supports setup/required fixtures.

Candidate use:

```text
oracle corpus generation test
    FIXTURES_SETUP d26_predicate_generated

predicate generated-corpus tests
    FIXTURES_REQUIRED d26_predicate_generated
```

This allows a selected consumer test to request its setup automatically.

Alternative:
- generate corpus as a build custom command.

Implementation phase should choose the simpler path after prototype.

## 5. Parallel test safety

Generated corpus files are immutable after setup.

Therefore consumer tests can run in parallel.

If a research test mutates shared output:
- give each test a unique build-directory path,
- or use CTest `RESOURCE_LOCK`.

Avoid one global writable corpus directory.

## 6. Failure semantics

Every test executable returns:
- 0 only on full pass,
- nonzero on mismatch/corruption/invalid topology.

Do not use output regex as the primary correctness signal.

CTest captures output and labels can filter targeted gates.

## 7. CI command concept

Current CI already runs:

```text
ctest -L meshing
```

Future explicit gates can add:

```text
ctest -L robust-geometry
ctest -L predicate
ctest -L determinism
```

once the implementation exists.

## 8. Compiler-mode gates

Predicate implementation target receives explicit compile semantics.

The verification harness additionally tests:
- Debug/Release result parity,
- `__FAST_MATH__` forbidden guard,
- rounding mode,
- binary64 assumptions.

A deliberately unsafe fast-math build, if used, belongs to an isolated research/negative-control target, never the production library.

## 9. Repetition

CTest supports repeated execution modes.

Determinism/stale intermittent failures can be investigated with repeated test execution, but a passing production gate must not depend on "eventually pass" behavior.

For determinism:
- identical run must produce identical canonical digest.

## 10. Canonical digests

For generated topology, hash a canonical representation:

```text
sorted stable PointIds per tet
→ sort tetra records
→ include hull facets if relevant
→ SHA-256
```

Raw allocation slot/TetId order is excluded unless specifically testing allocator determinism.
