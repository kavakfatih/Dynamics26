# M2.1-D — Cavity Oracle + Transactional Patch Implementation Record

**Date:** 2026-09-06  
**Development milestone:** DEV-MESH-P1D  
**Identity:** M2.1-D — Cavity Oracle + Transactional Patch  
**Research authority:** M2.0 DESIGN FROZEN  
**Status:** QUALIFIED IN DECLARED P1D CAVITY-TRANSACTION SUBSCOPE  
**M2 OVERALL:** NOT QUALIFIED

## 1. Source identity and exact-head evidence

P1D production development began after the Phase-A research-closeout hardening baseline:

- starting source baseline: `5c3cd085f22321b850eacc38975ea2da8792d880`,
- pre-final P1D qualification baseline: `dfbaa1472520d3345daa031a5f6d81241b1ca205`,
  macOS arm64 workflow #310, SUCCESS,
- final hardened/qualified source: `8d0f7d6e3d685b5b4a9e13cc3a9ffbfda78fc3d3`,
- exact-head macOS arm64 workflow:
  [#311](https://github.com/kavakfatih/Dynamics26/actions/runs/34059005490), completed/success.

Workflow #311 evidence on the exact qualified source SHA:

- Debug full CTest: **161/161 PASS**,
- Release full CTest: **161/161 PASS**,
- Debug meshing-labelled CTest: **38/38 PASS**,
- Release meshing-labelled CTest: **38/38 PASS**,
- `unit_m21a_delaunay_predicates`: PASS in Debug and Release,
- `unit_m21b_delaunay_bootstrap`: PASS in Debug and Release,
- `unit_m21c_delaunay_location`: PASS in Debug and Release,
- `unit_m21d_delaunay_transaction`: PASS in Debug and Release,
- `gui-fast`: SUCCESS,
- `gui-bundle-audit`: SKIPPED by the existing workflow condition.

The P1D executable prints its internal check count when invoked directly, but CI uses
`ctest --output-on-failure`, which suppresses stdout for passing tests. This record therefore does
not invent a runtime check total that is not present in exact-head CI evidence.

## 2. Frozen authority

Implementation and final hardening re-read and preserve:

- `CAVITY_TRANSACTION_SPEC.md`,
- `CELL_STORAGE_AND_MUTATION_MODEL.md`,
- `PATCH_ORIENTATION_AND_STITCHING.md`,
- `LOCAL_CORRECTNESS_AND_SEED_CONTRACT.md`,
- `DETERMINISM_SCOPE_AND_POLICY_VERSIONING.md`,
- `EXPERIMENT_PLAN.md`,
- `M2_1A_IMPLEMENTATION_RECORD.md`,
- `M2_1B_IMPLEMENTATION_RECORD.md`,
- `M2_1C_IMPLEMENTATION_RECORD.md`,
- M1/D26SITE1 canonical-site policy, including signed-zero normalization,
- P1A D26LIFT1 and ADR-MESH-0043 ghost/coplanar-circle semantics.

The implementation remains private under `src/meshing/m2/`. No installed/public mesh API, GUI,
SimulationMesh, CAD-recovery, sizing, optimizer or solver boundary was changed.

## 3. Reference architecture

The qualified P1D path is:

```text
validated canonical sites
-> exact all-live-cell conflict oracle
-> canonical exact-conflicting seed
-> adjacency conflict flood
-> oracle/flood equality
-> cavity internal/boundary extraction
-> 4C = 2I + B + 2-manifold/Euler checks
-> finite/ghost candidate cone construction
-> exact site-snapshot revalidation
-> current all-live exact-oracle recomputation
-> candidate query/base/stitching validation
-> temporary post-state typed validator
-> checked capacity reserve
-> COMMIT BARRIER
-> noexcept deterministic mechanical mutation
```

The all-live conflict oracle is independent of adjacency flood traversal. Finite conflict uses the
qualified P1A positive-tetra InSphere semantic and D26LIFT1 only on exact zero. Ghost conflict uses
the frozen outward exact half-space rule plus exact coplanar 3D-Euclidean circumcircle semantic and
D26LIFT1 only on exact zero. No numeric epsilon is introduced.

## 4. Transaction snapshot identity

A plan is bound to two distinct snapshot domains.

Arena/topology snapshot:

```text
sourceTopologyVersion
sourceSlotCount
sourceLiveCount
current handle generations
```

Canonical geometry/site-catalogue snapshot:

```text
vector<DelaunaySiteSnapshotEntry>
sorted by PointId

entry =
PointId
canonical raw x binary64 bits
canonical raw y binary64 bits
canonical raw z binary64 bits
```

The exact vector equality is the correctness authority. No collision-possible hash is accepted as a
semantic substitute. Binary64 extraction uses C++20 `std::bit_cast<std::uint64_t>`; raw memory byte
order is not a fingerprint authority.

M1/D26SITE1 normalizes both signed-zero encodings to canonical `+0`. P1D snapshots use that same
rule before comparing coordinate bits. Thus `+0.0` and `-0.0` are not incorrectly treated as two
different canonical sites.

`topologyVersion` and the site snapshot are deliberately separate invariants:

- topology version protects against arena mutation,
- exact site snapshot protects against caller-side canonical catalogue/coordinate mutation.

A `reserve()` capacity change is not topology mutation and does not advance either semantic
snapshot.

## 5. Exact oracle invariant at validation

Build-time stored evidence is not trusted by itself.

Every authoritative validation immediately before reserve/commit requires:

```text
current exact all-live-cell conflict oracle
==
stored conflictOracle
==
stored conflictFlood
```

The current oracle is recomputed from the current arena, exact canonical site snapshot and query by
the brute-force semantic all-cell scan. It is not regenerated by adjacency flood.

The qualification corpus explicitly corrupts both stored vectors to the same wrong live-cell set so:

```text
storedOracle == storedFlood
```

remains true while:

```text
storedOracle != currentExactOracle
```

The plan is rejected as stale before the commit barrier and the authoritative topology fingerprint
remains unchanged. Manually forcing `validated=true` and `reserved=true` does not bypass this
semantic revalidation because commit always calls the authoritative validator again.

## 6. Cavity and candidate-patch invariants

Cavity extraction uses canonical face keys and proves:

- all oracle conflict handles are current and unique,
- flood equals oracle,
- every internal cavity face has two reciprocal cavity owners,
- every boundary face has one cavity owner and one reciprocal surviving outside owner,
- `4C = 2I + B`,
- every boundary edge incidence is exactly two,
- the boundary is connected,
- boundary Euler characteristic is two.

For every replacement candidate, validation additionally proves the frozen coning contract:

- inserted query occurs exactly once,
- the candidate face opposite that query equals its recorded cavity base face,
- finite candidate = query + three finite base sites and exact Orient3D Positive,
- ghost candidate = Infinite(slot 0) + query + two finite base sites,
- ghost finite face 0 pairs to a new finite cell and has the frozen outward orientation,
- every candidate base key is unique,
- `set(candidate.baseFace) == set(cavity boundary key)`,
- every new lateral face has exactly two candidate owners,
- every external base has one fully planned surviving outside rewire,
- candidate post-state passes the existing typed topology validator before mutation.

The stronger optional full re-derivation/comparison of every stored internal/boundary record from the
recomputed conflict set is not required by this P1D closeout; the mandatory exact site snapshot,
current conflict oracle and complete candidate/post-state validation are executable.

## 7. Plan -> Validate -> Reserve -> Commit barrier

The frozen transaction order is preserved:

```text
Plan
-> Validate
-> Reserve
-> COMMIT BARRIER
-> Commit
```

All exact site comparison, oracle recomputation, predicate evaluation, candidate validation,
temporary post-state construction, map/set/vector growth and reserve requests occur before the
barrier.

The barrier path is `DelaunayTransactionAccess::applyPreparedCommit(...)` and is `noexcept`.
After the barrier there is no:

- Orient3D/InSphere or symbolic predicate,
- point location,
- conflict discovery or oracle recomputation,
- site snapshot comparison,
- map/hash construction,
- reserve/capacity request,
- new allocation-intended planning storage,
- new topological decision.

The test records arena capacity immediately before commit and proves it is unchanged by the
mechanical commit.

## 8. Failure and stale-plan semantics

Typed/explicit pre-commit failures exercised by P1D include:

- invalid/missing query,
- duplicate live site,
- invalid source topology,
- stale arena topology version/snapshot,
- stale cell generation,
- changed non-query canonical coordinate,
- deleted/extra/changed-PointId site catalogue,
- wrong but self-consistent stored oracle/flood,
- disconnected/oracle-flood disagreement,
- invalid cavity face multiplicity/manifold,
- duplicate/non-manifold candidate,
- missing query incidence / invalid candidate cone,
- invalid exact finite orientation,
- invalid ghost orientation,
- missing/ambiguous stitching,
- resource/capacity limit,
- topology-version exhaustion precondition.

Controlled failure fixtures compare the exact production-topology fingerprint before and after.
They prove no change to live/dead cells, vertices, neighbors, generations, visitEpoch, slot count,
liveCount or topologyVersion for pre-commit rejection.

## 9. Qualification corpus

`unit_m21d_delaunay_transaction` remains the dedicated P1D target with labels:

```text
unit;meshing;m2;delaunay;cavity;transaction;determinism
```

Qualified fixtures include:

- strict bootstrap interior insertion,
- strict outside-hull insertion,
- exact hull-facet insertion,
- five-site co-spherical D26LIFT1 golden connectivity,
- exact shared finite-facet insertion,
- exact finite-edge insertion,
- near-degenerate exact-nonzero scale,
- reversed canonical-site enumeration,
- legal live cell-slot permutation,
- invalid source topology,
- stale handle generation,
- stale topology plan after another legitimate commit,
- resource-limit failure,
- invalid finite candidate,
- invalid ghost candidate,
- non-manifold candidate face incidence,
- exact non-query coordinate snapshot mutation,
- site deletion/extra-site/PointId catalogue mismatch,
- signed-zero canonical-equivalence regression,
- jointly tampered stored oracle/flood,
- forged `validated`/`reserved` flags,
- missing-query candidate cone,
- duplicate/missing candidate base identity.

P1A/P1B/P1C regression targets pass alongside P1D on both Debug and Release exact-head CI.

## 10. Gate mapping

Repository `EXPERIMENT_PLAN.md` definitions are authoritative.

| Gate | P1D outcome | Executable evidence |
| --- | --- | --- |
| M2-G11 | **PASS** | all-cell semantic conflict oracle equals independently traversed adjacency flood across interior/hull/ghost/co-spherical/face/edge/determinism fixtures; current oracle is also recomputed during validation |
| M2-G12 | **PASS** | reciprocal internal cavity facets, connected conflict cavity evidence and `4C=2I+B` checks |
| M2-G13 | **PASS** | boundary edge incidence two, connected boundary, Euler=2, plus explicit non-manifold negative control |
| M2-G14 | **PASS** | controlled invalid/stale/tampered plans preserve exact pre-state fingerprint and never cross commit barrier |
| M2-G15 | **PASS** | interior, shared face, finite edge, hull facet and strict exterior insertion fixtures |
| M2-G25 | **PARTIAL** | typed finite/Infinite validator + ghost convention pass bootstrap and P1D one-step mutation/corruption states; full serial-constructor state-family qualification remains P1F work |
| M2-G26 | **PASS** | append-only pre-reserved path; commit barrier calls only prepared `noexcept` mutation; capacity unchanged after barrier |
| M2-G29 | **PASS** | checked slot arithmetic/resource-limit rejection with old topology unchanged |
| M2-G34 | **PASS** | every finite base cones to exact-positive candidate or typed degeneracy/orientation failure |
| M2-G35 | **PASS** | Infinite base cones to slot-0 ghost; finite face 0 pairing and exact outward witness verified; negative orientation fixture |
| M2-G36 | **PASS** | lateral face-key pairing is exactly two-owner; candidate base set equals boundary set; each base has one surviving outside patch |

M2-G20 remains **DEFERRED**. P1D has exact-head macOS arm64 Debug/Release CI, but M2-G20 is the
complete serial-reference-constructor CI + telemetry gate and is not closed by a P1D package badge.

P1F/final-constructor gates M2-G16..G24 and M2-G30..G33 are not promoted by this record. A single
co-spherical P1D fixture is not the required 120/5!/40,320 permutation qualification program.

## 11. Qualification boundary

The following claim is authorized:

```text
DEV-MESH-P1D
QUALIFIED IN DECLARED SUBSCOPE
```

Specifically, the P1D cavity oracle and transactional replacement patch are qualified against the
frozen reference contract on exact source `8d0f7d6e3d685b5b4a9e13cc3a9ffbfda78fc3d3` and workflow
#311.

This does **not** mean:

- M2 is qualified,
- the complete serial Delaunay constructor exists,
- P1E/P1F are implemented,
- arbitrary point-cloud production meshing is complete,
- CAD-conforming tetra meshing is complete,
- TET4 product/solver qualification exists.

```text
M2 OVERALL = NOT QUALIFIED
```

## 12. Next development target

Only after this qualification documentation receives its own exact-head CI evidence, the next
development target is:

```text
DEV-MESH-P1E
M2.1-E
Deterministic Adjacency Walk
```

P1E is not implemented by this record.
