# Determinism and Identity Policy

## Stable PointId

A Delaunay site requires identity independent of memory address.

Proposed properties:

- unique inside one immutable MeshingRequest snapshot,
- deterministic for identical resolved input,
- not derived from pointer address,
- unaffected by parallel scheduling.

## Canonicalization

Input records pass through:

```text
finite validation
→ exact-coordinate duplicate grouping
→ canonical site creation
→ stable PointId assignment
```

## Duplicate provenance

One site may represent multiple input records.

Conceptual metadata:

```text
CanonicalSite
- PointId
- coordinate
- sourceRecordIds[]
- optional CAD provenance[]
```

## Near-coincident sites

M1/M2 does not merge mathematically distinct points.

Any tolerance-based merge happens before the meshing snapshot and records:
- tolerance,
- scope,
- geometry revision,
- provenance change.

## Stable symbolic order

The symbolic perturbation needs a total order.

Leading candidate:
- stable `PointId` order,
- plus a fixed coordinate-component order defined by the SoS specification.

Forbidden inputs to symbolic order:
- pointer addresses,
- hash-table iteration,
- transient insertion index,
- thread scheduling.

## Insertion order is separate

For performance M2 may later use spatial or seeded orderings.

But:

```text
symbolic identity order
!=
insertion order
```

Changing insertion order should not change the canonical final topology when the symbolic policy is fixed.

## Output determinism

Raw creation-order TetIds need not be used for regression comparison.

Prefer a canonical topology fingerprint built from:
- sorted stable site IDs per tetra,
- sorted tetra connectivity records.

## Identity domains

`PointId` is an algorithm/site identity.

It does not replace:
- `GeometryEntityId`,
- CAD persistent key,
- Project ObjectId.

## Failure over hidden nondeterminism

If a stable symbolic identity cannot be established for an input class, report unsupported/ambiguous input rather than falling back to transient pointer ordering.
