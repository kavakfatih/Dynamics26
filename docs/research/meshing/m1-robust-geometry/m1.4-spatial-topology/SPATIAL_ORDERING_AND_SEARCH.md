# Spatial Ordering and Search

## 1. Why ordering matters

Incremental Delaunay algorithms repeatedly:
- locate the next point,
- traverse a local cavity,
- write nearby tetrahedra.

A fully random point order gives poor cache locality on large data.

BRIO and spatial sorting research show that geometric locality can improve practical performance while preserving useful randomized properties.

## 2. Candidate insertion orders

### O0 — PointId order
Purpose:
- deterministic reference,
- easy debugging.

Expected:
- potentially poor spatial locality.

### O1 — deterministic Morton order
Quantize normalized coordinates and sort by interleaved bits.

Pros:
- simple,
- deterministic,
- good locality,
- easy original implementation.

Cons:
- locality weaker than Hilbert/Moore in some cases,
- quantization policy required.

### O2 — deterministic Hilbert/Moore-style order
Pros:
- stronger locality potential.

Cons:
- more complex implementation and orientation logic.

### O3 — BRIO + spatial curve
Use rounds of increasing size with within-round spatial sorting.

Pros:
- strong literature and practice evidence,
- balances randomization with cache locality.

Cons:
- randomness/reproducibility contract must be explicit.

## 3. Determinism requirement

M1.3 symbolic identity order is independent from insertion order.

If O3 uses randomness:
- seed is explicit,
- seed belongs to MeshingAlgorithmVersion/settings,
- repeated runs use the same seed,
- topology should still be canonical under the symbolic degeneracy policy if that target proves achievable.

## 4. Seed/hint strategy

A spatially ordered sequence allows the tetra containing the previous inserted point, or one of the tetrahedra just created, to be a useful seed for the next walk.

Proposed M2 baseline:

```text
seed = one live tetra incident to previous insertion
walk(query, seed)
if failed
    slow fallback
```

Collect walk-step telemetry.

## 5. Optional acceleration indexes

### Dynamic AABB/BVH over tetrahedra
Pros:
- general spatial query.

Cons:
- every insertion/deletion updates many boxes,
- memory overhead,
- tree maintenance competes with cheap adjacency walk.

### kd-tree over vertices
Can find a nearby vertex, then use one incident tetra as a seed.

Pros:
- point set changes only by insertion,
- lower update complexity than tetra BVH.

Cons:
- nearest vertex is not necessarily best tetra seed,
- dynamic kd-tree complexity.

### Delaunay hierarchy
Sampled triangulation levels accelerate point location.

Pros:
- algorithmically elegant,
- good asymptotic/practical behavior documented in literature.

Cons:
- another dynamic topology structure,
- more complex first implementation.

### Spatial hash/grid
Map query coordinate to approximate nearby sites/tetra hints.

Pros:
- simple,
- cache friendly.

Cons:
- scale/anisotropy sensitivity,
- bucket tuning.

## 6. M2 recommendation

Do **not** start with a dynamic BVH.

Start:
1. deterministic spatial ordering,
2. adjacency walk,
3. previous-insertion hint,
4. O(n) exact fallback,
5. telemetry.

Then benchmark.

If fallback frequency or walk steps are unacceptable:
- first add a nearest-site seed structure,
- only then consider heavier tetra spatial indexes.

## 7. Parallel future

HXT research demonstrates a much more advanced path:
- space-filling curve ordering,
- partitioning by curve distance,
- concurrent insertions,
- conflict detection at partition boundaries,
- repartition/resume.

COMSOL publicly states that its 3D tet mesher benefits from shared-memory parallelism across faces/domains, but a single-domain imported CAD part can see little speedup.

ANSYS exposes parallel part meshing and parallelism for selected mesh methods, with explicit CPU/memory guidance.

### Dynamics26 conclusion

Parallelism is **not** M2.0.

First:
- robust serial kernel,
- memory profile,
- hot-loop profile.

Later:
- parallel surface faces/domains,
- domain/space-filling-curve partition research,
- reproducibility ADR.
