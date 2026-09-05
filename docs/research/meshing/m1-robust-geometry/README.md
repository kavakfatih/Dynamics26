# M1 — Robust Geometry Foundation

**Program:** Dynamics26 Original Meshing System R&D  
**Work package:** M1  
**State:** VERIFYING — FINAL CLOSEOUT CANDIDATE  
**Research baseline:** 2026-09-05

## Objective

Establish the numerical and topological foundation required by every later unstructured-meshing algorithm.

M1 is deliberately completed before Delaunay, surface or volume meshing implementation begins.

## Scope

- robust geometric predicates,
- exact/filtered decision semantics,
- floating-point failure mechanisms,
- degeneracy handling policy,
- CAD tolerance vs topological-predicate separation,
- primitive topology/data contracts,
- deterministic behavior requirements,
- adversarial/property-test specification,
- compiler floating-point contract for macOS/Apple Silicon.

## Documents

| Document | Purpose |
|---|---|
| `THEORY.md` | Why robust predicates are required and how they fit meshing |
| `PREDICATE_MATHEMATICS.md` | orient/incircle/insphere mathematics and sign semantics |
| `NUMERICAL_ROBUSTNESS.md` | floating-point, filtering, exact fallback, compiler policy |
| `DATA_STRUCTURES.md` | implementation-neutral primitive/topology contracts |
| `TEST_SPECIFICATION.md` | deterministic, adversarial, metamorphic and oracle tests |
| `COMMERCIAL_OPEN_SOURCE_COMPARISON.md` | product/source architecture comparison |
| `M1_IMPLEMENTATION_SPEC.md` | proposed original Dynamics26 implementation contract |

## Core conclusion

A mesher cannot use one global coordinate tolerance to decide topology.

```text
CAD/geometry tolerance
→ "are these geometric entities close enough for modeling/healing policy?"

Robust predicate
→ "what is the mathematically correct sign of this orientation/incircle/insphere decision?"
```

These are different problems and will be implemented as different subsystems.

## M1 exit

M1 research can move to implementation when:

1. predicate sign conventions are frozen,
2. compiler floating-point contract is documented,
3. exact-zero/degeneracy policy is frozen,
4. independent exact test oracle is specified,
5. public kernel interface is implementation-neutral,
6. no external predicate source code is required by the design.


## M1.1 subprogram

`m1.1-exact-oracle/` defines the independent exact predicate oracle, filtering research, commercial CAE sanity check and experiment plan. Production robust-predicate code remains intentionally deferred until the oracle is executable and independently verified.


## M1.2 subprogram
`m1.2-certified-filters/` derives the first original conservative floating-point certification filters from the general gamma_n rounding-error model. Every certified sign must match the independent M1.1 exact oracle.


## M1.3 subprogram

`m1.3-degeneracy/` separates exact duplicates, affine-dimension deficiency, local coplanarity, co-spherical Delaunay ambiguity and invalid CAD/domain topology. The leading Delaunay policy is a stable PointId-driven formal symbolic perturbation invoked only on exact predicate `Zero`.


## M1.4 subprogram

`m1.4-spatial-topology/` defines the serial M2 point-location and tetra-combinatorics foundation: generation-checked handles, opposite-face neighbor convention, walk-first location with typed states, cavity buffers, spatial insertion ordering and performance/memory experiments.


## M1.5 subprogram

`m1.5-verification-harness/` converts M1 theory into an executable evidence architecture: dual exact oracles, raw-bit fixtures, CTest tiers, deterministic generated corpora, failure replay, topology reference checks and commercial mesh-quality/reporting benchmarks.


## M1.6 subprogram

`m1.6-executable-verification/` is QUALIFIED verification infrastructure. `m1.7-exact-kernel/` is the QUALIFIED exact C++ predicate kernel. `m1.8-certified-fast-path/` is the QUALIFIED filtered fast path. `M1_CLOSEOUT_AUDIT.md` records the first failed closeout. M1.9-A through M1.9-D are now executable and green; the synchronized documentation commit must pass CI before the second audit can qualify M1.


## Executable M1.9 closeout evidence

- M1.9-A: adversarial/metamorphic fixtures + one-case replay — workflow #236 SUCCESS.
- M1.9-B: duplicate canonicalization + stable PointId + affine dimension + symbolic oracle — workflow #237 SUCCESS.
- M1.9-C: TetHandle / opposite-face / canonical-face / reciprocal-neighbor validator — workflow #238 SUCCESS.
- M1.9-D: telemetry + homogeneous fast-filter proof — workflow #239 SUCCESS.
- Release predicate baseline: 1049 cases, 1043 fast-certified, 6 exact fallback, 4 exact zero, 2 invalid input, 0 mismatch.

Production point location, cavity insertion and Bowyer-Watson remain intentionally unimplemented until M1 final audit passes.
