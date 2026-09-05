# M2.0 — Determinism Scope and Topology Policy Versioning

Status: PROPOSED / reproducibility contract
Date: 2026-09-05

## 1. Why determinism needs an explicit scope

"Dynamics26 is deterministic" is too vague to be an engineering contract.

M2 uses two different orders:
- canonical site identity / symbolic priority,
- insertion order used by the construction algorithm.

The first defines which weak-Delaunay triangulation is selected in exact degeneracy. The second is an
algorithm/performance choice that must not change that selected topology.

Therefore reproducibility must always state:
- what input representation is held fixed,
- what topology policy version is held fixed,
- what transformations are allowed,
- what fingerprint is compared.

## 2. Current canonical site identity

M1 canonicalization:
1. validates finite binary64 coordinates,
2. normalizes signed zero,
3. groups exact coordinate duplicates,
4. orders canonical raw-bit coordinate keys deterministically,
5. assigns PointId 1..N.

This gives a strong guarantee:

    same exact canonical binary64 site set
    => same PointIds

independent of:
- input enumeration,
- source-record ordering,
- memory address,
- hash-table iteration,
- thread scheduling.

This is the M2.0 identity baseline.

## 3. Current symbolic priority

Leading M2 topology policy:

    SymbolicPriority = ascending canonical PointId

and exact InSphere/InCircle ties are resolved by the Delaunay-specific lift-only policy in
DELAUNAY_MATHEMATICS.md.

Devillers-Teillaud's perturbation definition requires a fixed total order; the chosen order is part of
the mathematical perturbation problem.

Therefore changing symbolic priority is not a performance optimization. It changes the topology
policy for degenerate inputs.

## 4. Guaranteed reproducibility target

For the same:
- exact canonical binary64 coordinates,
- canonicalization policy version,
- symbolic Delaunay policy version,
- set of sites,
- M2 mathematical semantics,

the final canonical finite/hull topology must be identical across:
- arbitrary input enumeration,
- supported insertion order,
- repeated runs,
- Debug/Release builds,
- memory allocation/layout differences,
- future serial optimization paths that claim the same topology policy.

This is the primary M2 determinism contract.

## 5. Insertion order may change, symbolic order may not

Permitted later experiments:
- canonical PointId insertion,
- reverse insertion,
- Morton/Hilbert spatial order,
- deterministic BRIO-like order.

All must produce the same finite and hull fingerprints under the fixed symbolic priority.

A topology difference means:
1. a correctness defect, or
2. the experiment is actually using a different explicitly versioned topology policy.

It cannot be accepted as harmless "ordering noise".

## 6. Coordinate-frame transformations are different inputs

The current PointId priority is derived from canonical coordinate-bit ordering.

A translation, rotation, reflection or scale generally changes the stored binary64 coordinates and
can change their canonical ordering. Therefore it can change PointIds and symbolic priority.

For non-degenerate point sets the ordinary Euclidean Delaunay complex is geometrically equivariant
under exact similarities/isometries; a corresponding transformed topology is expected when the
transformed coordinates represent the exact intended geometry.

For exactly degenerate sets, however, several weak-Delaunay triangulations are valid. If the
coordinate-derived symbolic priority changes after recanonicalization, Dynamics26 may select a
different valid weak-Delaunay subdivision.

This is not automatically a robust-predicate defect. It is a consequence of changing the total order
that defines the perturbation.

## 7. What M2.0 does NOT claim

With the current coordinate-derived PointId priority, M2.0 does not promise:

    exact-degenerate symbolic connectivity
    is invariant under arbitrary coordinate-frame transformations

For example, a co-spherical cube may choose a different legal set of internal diagonals after a
transformation that changes canonical site order.

The transformed result must still satisfy:
- exact topology validity,
- geometric embedding,
- weak-Delaunay legality,
- the symbolic policy computed from the transformed canonical sites,
- determinism for that transformed exact site set.

## 8. Metamorphic test policy

### Non-degenerate fixtures

For selected transformations whose resulting binary64 coordinates are exactly representable and
whose geometry remains non-degenerate:
- transform all sites,
- recanonicalize,
- map vertices by source/test correspondence rather than by new PointId number,
- require equivalent Delaunay connectivity after mapping.

Good exact test transforms include coordinate permutations, sign changes/reflections where values
remain exactly representable, powers-of-two uniform scaling, and selected exactly representable
translations.

The orientation sign may reverse under reflections; the stored cell ordering must be reoriented
positive without changing connectivity.

### Exact-degenerate fixtures

Run the same transforms, but require:
- valid weak-Delaunay geometry,
- correct symbolic topology under the new canonical priority,
- deterministic repeated result.

Do not require the old degenerate diagonal pattern unless the symbolic priority itself is preserved.

### Priority-preserving metamorphic fixtures

Separately construct tests where a stable external test priority is explicitly held fixed across a
coordinate transform. Under that experimental policy the symbolic topology should map exactly.

This is a future-policy experiment, not the M2.0 production default.

## 9. Why provenance-based symbolic priority is a future option

M3/M9 may create sites with stable CAD/refinement provenance that survives model transformations.

A future topology policy could define:

    SymbolicPriorityKey
    = stable provenance-derived unique key

instead of coordinate-derived PointId.

Potential advantages:
- exact-degenerate topology can remain stable under rigid model transforms,
- adaptation lineage can preserve deterministic tie identity.

But such a key must be:
- globally unique within the immutable meshing snapshot,
- stable under supported transformations,
- independent of insertion order/thread scheduling,
- reproducible after persistence/reopen if product semantics require it.

M2.0 does not introduce this complexity before point-cloud construction is qualified.

## 10. Versioned topology policy

Every canonical topology fingerprint/replay should carry explicit policy identifiers.

Leading identifiers:

    D26SITE1
    - finite binary64 validation
    - signed-zero normalization
    - exact coordinate duplicate grouping
    - canonical raw-bit site-key ordering
    - PointId 1..N

    D26LIFT1
    - real coordinates unchanged
    - lift-only InSphere/InCircle exact-zero resolution
    - symbolic priority = ascending D26SITE1 PointId

    D26DT1
    - finite tetra connectivity records sorted by PointId
    - hull triangle records sorted by PointId
    - lexicographic record ordering
    - policy header included in serialization

Names are research candidates until M2.0 freeze, but the versioning concept is mandatory.

## 11. Fingerprint record

A diagnostic canonical serialization should remain human-diffable before hashing.

Conceptual header:

    D26DT1
    site_policy=D26SITE1
    symbolic_policy=D26LIFT1
    canonical_sites=<N>
    finite_tets=<T>
    hull_facets=<H>

followed by:
- canonical site raw-bit records,
- sorted finite tetra PointId tuples,
- sorted hull PointId triples.

A SHA-256 or other digest may be emitted for compact regression comparison, but the digest does not
replace the canonical record.

## 12. Replay policy

D26DT-REPLAY records:
- site-policy version,
- symbolic-policy version,
- fingerprint-schema version,
- exact raw site bits,
- PointIds / canonical site list,
- insertion order,
- predicate and symbolic decisions.

A replay created under one symbolic policy is not silently interpreted under a newer policy.

## 13. Policy migration rule

Changing any of these requires an explicit research/ADR decision and new golden evidence:
- duplicate canonicalization,
- PointId/canonical ordering,
- symbolic priority,
- lift perturbation sign/order,
- finite/hull fingerprint schema.

Old golden records remain labeled with the policy that produced them.

A policy migration is allowed to change degenerate connectivity while preserving weak-Delaunay
geometry, but the change must be deliberate and reviewable.

## 14. Determinism matrix

| Change | Same topology required under D26SITE1/D26LIFT1? |
|---|---|
| input enumeration | YES |
| source-record enumeration | YES |
| repeated run | YES |
| Debug vs Release | YES |
| cell allocation order | YES |
| supported insertion order | YES |
| deterministic spatial ordering optimization | YES |
| coordinate translation/rotation that changes PointId priority, non-degenerate | mapped geometric topology expected |
| coordinate transformation that changes priority, exact-degenerate | NO; new policy result must be valid/deterministic |
| symbolic priority policy change | NO; requires new policy version |
| topology fingerprint schema change | semantic topology may be same, schema version must change |

## 15. Engineering consequence

A regression report must never say only "mesh hash changed".

It must answer:
- Did the canonical exact site set change?
- Did D26SITE policy change?
- Did D26LIFT policy change?
- Did only insertion/performance order change?
- Is the point set degenerate?
- Did finite connectivity change?
- Did hull connectivity change?
- Does weak-Delaunay validity still hold?

This makes determinism scientifically auditable rather than cosmetic.
