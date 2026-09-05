# Symbolic Perturbation Theory

## Simulation of Simplicity

Edelsbrunner and Mücke introduced Simulation of Simplicity (SoS) as a systematic way to handle degenerate geometric input by conceptually applying arbitrarily small, ordered perturbations.

The perturbation is symbolic:
- stored coordinates are not modified,
- a hierarchy of infinitesimals defines a consistent general-position topology,
- exact nondegenerate results remain unchanged.

## Why a simple ID tie-break is not enough

A locally deterministic rule such as:

```text
if predicate == Zero:
    choose sign from two IDs
```

need not correspond to one globally coherent perturbation.

That can lead to:
- contradictory orientation relations,
- inconsistent cavities,
- insertion-order dependence,
- invalid topology.

The tie-break must therefore be derived from a formal perturbation or another proven degeneracy-resolution method.

## Conceptual perturbation family

A generic SoS-style model can be written as:

```text
x_(i,j)' = x_(i,j) + epsilon^(rank(i,j))
```

where:
- `i` is stable point rank,
- `j` is coordinate dimension,
- exponents are unique and totally ordered,
- `epsilon` is symbolic and tends to zero from the positive side.

A predicate determinant becomes:

```text
D(epsilon)
= c0 + c1 epsilon^k1 + c2 epsilon^k2 + ...
```

If:
```text
c0 != 0
```
the ordinary exact predicate sign wins.

If:
```text
c0 = 0
```
the first nonzero symbolic coefficient determines the tie.

## Dynamics26 boundary

The robust predicate layer remains:

```text
Negative / Zero / Positive
```

A separate degeneracy policy is invoked only on exact `Zero`.

The policy:
- consumes stable point identity,
- never changes stored coordinates,
- is part of Delaunay/topology behavior rather than geometry repair.

## Alternative research families

Besides general SoS, Delaunay-specific literature includes perturbation/completion methods such as:
- Devillers–Teillaud 3D Delaunay symbolic perturbation,
- Dillencourt–Smith degeneracy completion.

These remain comparison candidates.

## Leading choice

For M2 point-insertion research:

**S0 — SoS-style predicate-level symbolic perturbation**

Alternative:

**S1 — degeneracy completion/postprocessing**

S0 is preferred for initial study because it integrates naturally with exact predicates and cavity decisions.

## Exact duplicates

Symbolic perturbation is not used to make two identical coordinate records into different geometric sites.

They are canonicalized before the Delaunay site set is created.

## Near degeneracy

If the exact predicate is nonzero, symbolic perturbation is not invoked.

An extremely small exact nonzero sign is still the topology truth.

## Proof requirement

Before S0 can be accepted:

1. symbolic ranking must be fixed,
2. coefficient-evaluation rules must be defined,
3. consistency must be supported by primary-source theory,
4. exact symbolic oracle fixtures must pass,
5. input/insertion permutation tests must pass.
