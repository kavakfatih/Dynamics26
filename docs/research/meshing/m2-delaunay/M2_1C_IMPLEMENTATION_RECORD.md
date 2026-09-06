# M2.1-C — Brute-force exact point location

Date: 2026-09-06  
Status: IMPLEMENTATION CANDIDATE / exact-head macOS qualification pending  
Development: DEV-MESH-P1C  
M2 OVERALL: NOT QUALIFIED

## Prerequisite evidence and current-source audit

- P1A oblique-circle correction: `6962866dde18fa5dc17c48f49212594b091c2e32`,
  [macOS arm64 #298](https://github.com/kavakfatih/Dynamics26/actions/runs/34037763796), success.
- P1B validator hardening: `6c626f3884aeedcda727f93fc1a4b174cad12d51`,
  [macOS arm64 #299](https://github.com/kavakfatih/Dynamics26/actions/runs/34038076186), success.
  Debug and Release ran 159/159 CTests, including P1A/P1B; gui-fast passed.
- Before P1C, concurrent M6 changes advanced main to
  `742c58d60fe424b42e204e9411ecb29b9dce9c84`. Their interval enclosure correction,
  incidence index and invalid-prior-state changes were reviewed for integration
  impact and preserved. Its workflow 34050837794 completed/success.

P1C began only after these gates were checked; no old SHA was substituted for a
current-source test result. The uploaded prompt's determinism document path is
resolved to `m2-delaunay/DETERMINISM_SCOPE_AND_POLICY_VERSIONING.md`.

## Authority and derivation

Repository authority:
- `DELAUNAY_MATHEMATICS.md` — four exact barycentric numerator signs,
- `LOCAL_CORRECTNESS_AND_SEED_CONTRACT.md` — outside hull witness and seed separation,
- `CELL_STORAGE_AND_MUTATION_MODEL.md` — typed Infinite and opposite-face convention,
- `M2_0_RESEARCH_FREEZE_AUDIT.md` — brute force before walk and mutation,
- `EXPERIMENT_PLAN.md` — M2-G06.

For exact-positive D=O(a,b,c,d), use N0=O(p,b,c,d), N1=O(a,p,c,d),
N2=O(a,b,p,d), N3=O(a,b,c,p). No divisions are needed. Containment requires
all four signs nonnegative; positive-weight vertices identify the minimal
simplex containing the point. Their sorted PointIds define its canonical
identity: four=Cell, three=Facet, two=Edge, one=Vertex. A zero with any negative
numerator is outside that tetra, never a boundary hit.

Every live finite cell is tested independently. Incompatible containing entities
are reported as a logic error rather than resolved by allocation order.

For a query not contained in any cell, scan all live ghosts. Exact outward
O(a,b,c,p)>0 proves strict hull violation. Return the lexicographically smallest
canonical violated face with its oriented triple and adjacent finite/ghost
handles. This is geometric witness evidence, not a ConflictSeed or flood.
No arbitrary ghost is chosen, and no Delaunay symbolic tie enters location.

## Private API and immutable input

`src/meshing/m2/DelaunayLocation.h/.cpp` adds `locateBruteForceExact`.
It accepts const slot/site spans and a finite Vec3 query. It returns owning
result vectors, not raw pointers into storage. Canonical identity and
snapshot-specific generation handles are separate. All live finite **and ghost**
incident cells are returned in canonical connectivity order for a contained
entity. Handle numbers may differ after a storage permutation; identities must not.

Sites may include not-yet-inserted points. A coordinate only becomes a Vertex
location when present in the live complex, not merely in the site catalogue.
Dead slots are ignored. There are no persistent caches, walk hints, BVH/KD-tree,
spatial sorting or parallel traversal. Identity maps are exact PointId lookups.
`finiteCellsTested` exposes that every finite cell was examined.

Invalid coordinates, duplicate IDs/coordinates, invalid topology or an empty
finite complex throw invalid_argument consistently with P1A/P1B private APIs.
A handle-domain overflow throws length_error. Contradictory containment or no
valid outside witness throws logic_error. No error is relabeled Outside.

Precondition: a valid embedded triangulation of its convex hull. Structural
validation and local exact positivity are checked on entry; they do not prove
a general complex is S³ or supply a complete geometric-embedding oracle.
Later constructor qualification owns that stronger precondition. Result handles
must be discarded after the input snapshot changes.

No public/install header, C ABI, GUI, SimulationMesh or document state changes.
CAD geometry, display tessellation and FEM mesh remain distinct.

## Executable oracle and test registration

CTest target: `unit_m21c_delaunay_location`, labels include meshing/m2/location.
It is part of the standard macOS Debug/Release full CTest and meshing-label runs.

Fixtures:
- one exact-positive bootstrap: Cell/Facet/Edge/Vertex/Outside identities,
- two explicitly constructed finite tetra sharing z=0, with six ghost cells,
- independent integer-grid barycentric oracle: x>=0, y>=0, x+y+abs(z)<=1,
- 539 grid queries, each also run with reversed cells and site enumeration,
- complete shared-face/hull-edge incident evidence and generation remapping,
- canonical minimum violated hull-face selection, with exact sign verification,
- face supporting-plane extensions that have zero and negative numerators,
- signed zero and not-yet-inserted catalogue site,
- 2^-500/1/2^500 scaling, face +/- one representable ulp, height 2^-1000,
- invalid/stale/missing/duplicate input rejection,
- vertices, neighbors, liveness, generation and visitEpoch unchanged by queries.

Local GCC Debug/Release output: 5239 checks, 539 grid queries, all five types.
Local execution is supplementary; it is not macOS qualification.

## Gate scope

- M2-G03/G04/G05: PASS for qualified P1B bootstrap (source/CI above).
- M2-G06: candidate; requires this source's exact-head macOS Debug/Release.
- M2-G25: PARTIAL, bootstrap and corrupted-snapshot coverage only.
- M2-G07 and P1D+ mutation/constructor/replay gates: DEFERRED.
- M2-G20 remains the complete serial-constructor gate, not a per-package CI badge.

No CAD-conforming tetra mesher or FEM-qualified TET4 product capability is claimed.
After P1C qualification the next single authorized production target is P1D:
cavity oracle + transactional patch. This work does not implement it.
