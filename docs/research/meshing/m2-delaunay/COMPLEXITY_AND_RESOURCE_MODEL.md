# M2.0 — Complexity and Resource Model

Status: PROPOSED / resource-policy research
Date: 2026-09-05

## 1. Mathematical output complexity is not a software bug

A 3D Delaunay tetrahedralization does not have a universal linear-size bound.

The worst-case combinatorial complexity is quadratic in the number of input points. Current CGAL
documentation gives the classic example of points distributed on two non-coplanar lines; Muecke's
robust 3D implementation paper likewise notes quadratic theoretical complexity and that this is
worst-case optimal.

References:
- E. P. Muecke, A Robust Implementation for Three-Dimensional Delaunay Triangulations,
  IJCGA 8(2), 1998, DOI 10.1142/S0218195998000138.
- J. Erickson, Nice Point Sets Can Have Nasty Delaunay Triangulations,
  DCG 30(1), 2003, DOI 10.1007/s00454-003-2927-4.

Therefore M2 must never use a rule such as:

    tetraCount > K * pointCount => topology corrupted

as a mathematical validator.

Large output can be valid.

## 2. Typical versus worst case

Many practical point distributions produce linear or near-linear Delaunay complexity, but this is an
empirical/distribution-dependent property, not a correctness invariant.

Erickson shows that even geometrically "nice" sampled sets can produce near-quadratic behavior under
certain spread/surface configurations.

Dynamics26 must measure:
- finite tetra / site ratio,
- ghost cell / hull-site ratio,
- peak live cells,
- cumulative tombstone slots,
- cavity-size distribution,

without assuming a fixed ratio is universally valid.

## 3. Resource policy is separate from topology truth

There are three different questions:

1. Is the point set mathematically valid?
2. Is the produced triangulation topologically/geometrically correct?
3. Does the current machine/resource budget allow construction to continue?

Question 3 must not change the answer to 1 or 2.

A resource stop returns a typed operational result such as:

    ResourceLimitExceeded
    - requestedAdditionalCells
    - liveCells
    - allocatedSlots
    - configuredCellBudget
    - approximateBytes
    - insertedSites

It must not masquerade as NonManifoldTopology, PredicateFailure or InvalidGeometry.

## 4. No silent geometric simplification

If a resource limit is reached, M2 must not silently:
- merge near-coincident sites,
- skip an insertion,
- drop a cavity cell,
- reduce symbolic checks,
- change insertion order to an unversioned heuristic,
- emit a partial mesh as complete.

Any future geometry conditioning or point reduction is an explicit upstream operation with provenance
and its own engineering policy.

## 5. Reference arena implication

The M2.1 append-only/tombstoned arena deliberately spends extra memory to simplify correctness.

Therefore its resource envelope is expected to be lower than the eventual optimized production
arena.

Reference qualification corpora should use sizes that:
- expose topology/pathology,
- remain comfortably within CI memory,
- include at least one controlled high-complexity growth family.

The reference arena is not a claim of production scalability.

## 6. Controlled complexity adversaries

Add explicit families:

### Two skew/non-coplanar lines

Distribute roughly n/2 points along each of two non-coplanar lines.

Purpose:
- demonstrate superlinear/quadratic Delaunay growth,
- ensure validators do not reject large-but-valid topology,
- exercise resource telemetry.

### Erickson-style bounded-spread/surface adversaries

Do not reproduce a paper implementation by copying code. Construct independent mathematical
fixtures inspired by the published complexity result.

Purpose:
- prevent assumptions that smooth/surface-like samples always have linear Delaunay size,
- stress walk/cavity/memory metrics.

### Ordinary volumetric random clouds

Purpose:
- establish typical baseline ratios separately from worst-case data.

The benchmark report must label distribution class.

## 7. Budget checks around transaction

Resource checks belong before the destructive commit barrier.

The planner knows cavity boundary size B and therefore the number of candidate unified cells B.

For append-only M2.1:

    requiredNewSlots = B

Before commit:
- check index/slot arithmetic for overflow,
- check configured slot/cell/byte budget,
- reserve capacity,
- if reserve/budget fails, return typed resource failure with old topology unchanged.

No resource allocation is attempted after the commit barrier.

## 8. Integer overflow policy

Counts and products used for:
- slot growth,
- byte estimates,
- telemetry,
- fingerprint serialization lengths,

must use checked arithmetic.

A wraparound is a hard typed failure, never a smaller allocation.

The mathematical input size may be representable while a chosen storage/index width is not.

## 9. Complexity telemetry

At whole-build level:

    sites.input
    sites.canonical
    cells.finite_live
    cells.ghost_live
    cells.peak_live
    slots.total
    slots.dead
    hull.facets
    ratio.finite_tets_per_site
    ratio.unified_cells_per_site

At insertion level:

    cavity.cells
    cavity.boundary_facets
    candidate.cells
    locate.walk_steps
    predicates.calls
    exact_fallbacks

For benchmark aggregation report:
- min/median/p95/max,
- total,
- distribution family,
- point count,
- final topology fingerprint.

## 10. Correctness versus performance acceptance

A slower/reference construction is correct if it passes mathematical gates within configured
resources.

A faster construction is acceptable only if:
- same canonical input,
- same symbolic policy,
- same finite/hull fingerprint,
- same typed success/failure semantics for a budget large enough to complete,
- no oracle mismatch.

Performance improvements cannot weaken exact predicate or transaction gates.

## 11. Product planning consequence

M2 cannot promise a simple "N points = about 6N tetrahedra" memory formula as an upper bound.

Future product UI/resource estimates may use empirical expected ratios for warnings, but hard safety
must be based on:
- actual live/allocated counts,
- checked capacity growth,
- user/system resource budget.

The mathematical worst case remains quadratic.
