# M4 Design Freeze — Boundary Recovery and CAD-Conforming Volume Meshing

Status: **RESEARCH COMPLETE / DESIGN FROZEN / IMPLEMENTATION NOT STARTED**  
Freeze ID: **D26-M4-FREEZE-1**  
Research package: **M4 — Boundary Recovery + Volume Meshing**  
Date: 2026-09-06

## 1. Input and objective

M4 consumes a watertight M3 surface constraint complex plus a qualified-enough M2 tetra kernel and targets a CAD-conforming tetrahedral volume-mesh candidate.

M4 does not change M2 qualification truth and does not itself make TET4 a product topology.

## 2. Constraint recovery means subcomplex coverage

The frozen requirement is:

```text
original constraint == union of recovered child constraints
```

It is **not** one input M3 triangle -> one output M4 triangle.

A source segment is recovered by an ordered exact subsegment chain. A source surface triangle may become a recovered triangle subcomplex.

Acceptance requires complete coverage, no gaps, no illegal overlaps, no leakage across the source CAD Face, and inherited `GeometryEntityId` provenance.

## 3. First constructed-point domain: SegmentLNC

The first constructed site is:

```text
P = A + t(B-A)
```

where:
- A/B are authoritative constraint endpoints,
- t is an exact dyadic parameter,
- construction identity is authoritative,
- optional binary64 Vec3 is realization/cache only.

Binary64 endpoints are exact dyadic values; exact dyadic t remains compatible with the existing Dynamics26 BigInt/dyadic arithmetic domain.

Canonicalization:
- t=0 and t=1 collapse to the corresponding ExplicitSite,
- interior t uses a canonical reduced dyadic representation,
- source constraint-segment identity is canonical and independent of memory allocation,
- duplicate topological constructions are reconciled before tetra insertion.

Rounded binary64 coordinates are never topology authority.

## 4. Versioned constructed-site symbolic identity

ADR-MESH-0048 introduces:
- **D26SITE2** — canonical site identity and total order,
- **D26LIFT2** — symbolic Delaunay degeneracy policy over that domain.

D26LIFT1 is not silently widened.

```text
ExplicitSite
  -> canonical explicit-site identity

ConstructedSegmentSite
  -> canonical source constraint-segment identity
  -> canonical exact dyadic t
```

The first versioned kind order is `ExplicitSite < ConstructedSegmentSite`; order inside a kind is lexicographic on its canonical fields.

The total order is independent of pointer, allocation order, arena slot, thread, task completion order and wall-clock insertion time. D26LIFT2 participates only when the exact geometric predicate is genuinely zero and never overrides nonzero geometric truth.

## 5. Exact predicates for SegmentLNC

Orient/InCircle/InSphere decisions involving SegmentLNC sites are evaluated from the exact dyadic construction, not rounded realization coordinates.

The first implementation may materialize exact dyadic coordinates internally. Future indirect-predicate filters may optimize the path only if they preserve the same exact oracle.

## 6. General homogeneous future oracle

A future/oracle extension may represent:

```text
P = (X/W, Y/W, Z/W), W > 0
```

with exact Orient3D rows `[X,Y,Z,W]` and InSphere rows:

```text
[XW, YW, ZW, X^2+Y^2+Z^2, W^2]
```

subject to the common positive scaling of the homogeneous determinant formulation.

This is future research architecture, not a first-implementation requirement. SegmentLNC remains the preferred initial scope.

## 7. Recovery sequence

```text
M3 watertight PLC
-> unconstrained tetrahedralization
-> segment recovery
-> facet recovery
-> volume-domain classification
-> constraint coverage validation
-> product coordinate realization
-> exact topology + coverage revalidation
```

Segment recovery ends only with an exact ordered coverage proof.

Facet recovery must permit multiple strategies:
1. ordinary/local cavity recovery,
2. cavity expansion,
3. bounded fallback,
4. robust reference/fallback reconstruction, including modified gift-wrapping-family research.

No missing constraint is silently accepted.

## 8. Domain classification

The universal shortcut `all cells not reachable from outside = inside` is rejected.

Reference semantics:
- ghost/unbounded region starts OUTSIDE,
- crossing an unconstrained facet preserves region state,
- crossing an oriented constraint boundary applies a versioned shell/region transition,
- closed solids, nested hollow shells/internal cavities and multiple shells must be represented explicitly,
- open/non-manifold shells, inconsistent orientation or ambiguous transitions fail explicitly.

The implementation may use labels or shell-state vectors; these semantics remain authoritative.

## 9. Exact -> binary64 realization

If product nodes are binary64:

```text
exact/implicit site
-> deterministic binary64 realization
-> exact topology revalidation
-> constraint coverage revalidation
-> accept OR typed failure
```

Rounded realization may never silently invalidate a topology that was previously exact-valid.

## 10. Output validity and provenance

Every accepted tetra has authoritative exact-positive orientation. Recovered child constraints inherit source CAD provenance; volume cells carry Body/Region provenance for M7.

Resource exhaustion, unsupported geometry and geometric invalidity are distinct typed outcomes.

## 11. Qualification boundary

D26-M4-FREEZE-1 freezes architecture and gates only. It does not claim boundary recovery, arbitrary-CAD tetrahedralization or product TET4 qualification.

Qualification authority: `M4_QUALIFICATION_GATE_MATRIX.md`.
