# M1.3 — Degeneracy & Deterministic Symbolic Perturbation

**Program:** Dynamics26 Original Meshing System R&D  
**Parent:** M1 — Robust Geometry Foundation  
**Depends on:** M1.1 exact oracle, M1.2 certified filters  
**State:** RESEARCHING  
**Baseline:** 2026-09-05  
**Production code:** none

## Objective

Define how Dynamics26 meshing algorithms behave when exact robust predicates return `Zero`.

M1.3 does not change the mathematical predicate result. It defines a separate topology policy for cases where a geometric algorithm needs a unique combinatorial decision.

## Core rule

```text
RobustPredicate
→ reports mathematical truth

DegeneracyPolicy
→ decides deterministic algorithmic continuation when truth is Zero
```

The predicate layer never hides degeneracy by inventing a nonzero sign.

## Important separation

Not every degeneracy is resolved by symbolic perturbation.

```text
Exact duplicate coordinates
→ canonicalize / merge as one geometric site

Near duplicate but distinct coordinates
→ keep distinct unless explicit geometry-conditioning policy merges them

All points coplanar in a 3D tetra problem
→ dimension-reduced / unsupported state, not fake tetrahedra

Distinct co-spherical sites
→ deterministic symbolic Delaunay tie-break candidate

Invalid CAD / self-intersection / open shell
→ geometry/meshing diagnostic, not symbolic perturbation
```

## Leading research direction

Use a Simulation-of-Simplicity-style policy for the Delaunay topology layer:

1. exact predicate first,
2. only when exact result is `Zero`,
3. use a stable total order derived from Dynamics26 `PointId`,
4. evaluate a formally defined symbolic perturbation hierarchy,
5. return a deterministic topological sign,
6. never modify stored physical coordinates.

A naive rule such as "if zero, compare IDs and choose a sign" is explicitly rejected unless proved equivalent to the chosen perturbation.

## M1.3 exit

Research can move to executable prototype when:

- degeneracy classes are frozen,
- exact-duplicate policy is frozen,
- stable PointId semantics are frozen,
- symbolic perturbation order is mathematically specified,
- lower-dimensional input behavior is explicit,
- oracle/experiment plan can verify permutation and insertion-order determinism.
