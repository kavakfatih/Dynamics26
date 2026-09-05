# M1 — Robust Geometry Foundation

**Program:** Dynamics26 Original Meshing System R&D  
**Work package:** M1  
**State:** RESEARCHING  
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
