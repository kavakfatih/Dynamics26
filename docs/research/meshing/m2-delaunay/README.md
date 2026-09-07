# M2 — Delaunay Point-Cloud Tetrahedralization Research


> 2026-09-06 independent audit: ADR-MESH-0043 corrects the oblique coplanar
> circle metric. Corrected source `6962866dde18fa5dc17c48f49212594b091c2e32`
> passed macOS arm64 workflow [#298](https://github.com/kavakfatih/Dynamics26/actions/runs/34037763796).
> M2-G09 / coplanar G10 are requalified for the expanded P1A corpus.
> Historical evidence below remains scoped to its original commit.

Program: Dynamics26 Original Meshing Engine
Work package: M2.0 — Delaunay Reference Architecture & Experiment Plan
Status: M2.0 DESIGN FROZEN / M2.1-A QUALIFIED IN SUBSCOPE / M2.1-B QUALIFIED IN BOOTSTRAP SUBSCOPE / M2.1-C QUALIFIED IN LOCATION SUBSCOPE / M2.1-D QUALIFIED IN CAVITY-TRANSACTION SUBSCOPE / M2.1-E QUALIFIED IN DETERMINISTIC-WALK SUBSCOPE; M2 OVERALL NOT QUALIFIED
Date: 2026-09-05

## Purpose

This package freezes the mathematical, topological and verification contracts that must exist before
Dynamics26 writes the first production/reference Bowyer-Watson insertion kernel.

M2.0 is intentionally research-first. It does not add a product mesher and it does not change the
current StructuredHexMesher product path.

The production implementation remains original Dynamics26 code. Academic literature is the
algorithmic authority; public CGAL/TetGen/Gmsh/Netgen material is used only for architecture,
failure-mode and verification study under ../CLEAN_ROOM_POLICY.md.

## Engineering traceability rule

For M2 and later solver/physics work, implementation must remain reviewable from first principles:

    source / theorem / physical assumption
    -> Dynamics26 derivation
    -> sign / unit / identity convention
    -> algorithmic invariant
    -> executable fixture / oracle
    -> implementation symbol
    -> regression / CI evidence

A production function that makes a non-trivial geometric, mathematical or physical decision should
be traceable to a committed derivation or contract. Chat history is not engineering authority.

## M2.0 document map

- M2_0_REFERENCE_ARCHITECTURE.md — serial reference-construction architecture and data-flow decisions
- DELAUNAY_MATHEMATICS.md — Orient3D, point location, InSphere, ghost-hull and symbolic-tie mathematics
- CAVITY_TRANSACTION_SPEC.md — conflict cavity topology, special insertion cases and transaction rules
- LOCAL_CORRECTNESS_AND_SEED_CONTRACT.md — verified conflict seeds, local/global Delaunay legality and S^3 invariants
- CELL_STORAGE_AND_MUTATION_MODEL.md — typed finite/infinite cell storage, face conventions, handle and commit-barrier policy
- COMPLEXITY_AND_RESOURCE_MODEL.md — 3D worst-case output complexity, typed resource limits and growth telemetry
- DETERMINISM_SCOPE_AND_POLICY_VERSIONING.md — reproducibility scope, transform semantics, policy IDs and fingerprint/replay versioning
- PATCH_ORIENTATION_AND_STITCHING.md — finite/ghost cone construction, positive orientation and new-new/new-old neighbor pairing
- M2_0_RESEARCH_FREEZE_AUDIT.md — closure matrix, frozen contracts, gate classification and M2.1 authorization boundary
- EXPERIMENT_PLAN.md — golden cases, brute-force oracles, determinism gates, replay and telemetry
- M2_1A_IMPLEMENTATION_RECORD.md — semantic-predicate implementation and exact-head qualification evidence
- M2_1B_IMPLEMENTATION_RECORD.md — bootstrap hardening and exact-head qualification evidence
- M2_1C_IMPLEMENTATION_RECORD.md — brute-force location contract and exact-source workflow #302 qualification
- M2_1D_IMPLEMENTATION_RECORD.md — exact cavity oracle, transaction snapshot/barrier and exact-source workflow #311 qualification
- M2_1E_IMPLEMENTATION_RECORD.md — deterministic adjacency walk, P1C oracle agreement and exact-source workflow #315 qualification

## Current leading decisions

These remain PROPOSED until executable evidence closes their acceptance gates.

- serial incremental Bowyer-Watson reference constructor,
- no numeric super-tetrahedron,
- finite + infinite/ghost-cell hull topology,
- infinite vertex is a tagged topological value, never a PointId and never a coordinate,
- deterministic four-site affine-basis bootstrap from canonical sites,
- brute-force point-location oracle plus deterministic adjacency walk,
- finite conflict through the existing exact/filtered insphere semantic,
- ghost conflict through exact hull half-space plus coplanar circumcircle semantic,
- Delaunay-specific lift-only symbolic perturbation for exact co-spherical/cocircular ties,
- symbolic identity order is separate from insertion order,
- cavity mutation is transactional: Plan -> Validate -> Commit,
- canonical tetra/facet fingerprints are regression identities,
- Delaunay correctness and FEM element quality are separate gates.

## Scope boundary

M2 is unconstrained point-cloud tetrahedralization only.

Not M2:
- CAD Face/Edge recovery,
- constrained Delaunay volume meshing,
- size fields,
- sliver optimization,
- TET4 product qualification,
- TET10,
- remeshing/adaptation,
- solver material/contact physics.

Those remain M3+ work packages.
