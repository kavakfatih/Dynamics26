# Dynamics26 Original Meshing Engine — Research Closeout

Umbrella ID: **D26-MESH-RESEARCH-CLOSEOUT-1**  
Date: 2026-09-06  
Scope: **RESEARCH CONSOLIDATION + DESIGN FREEZE + DEVELOPMENT HANDOFF**  
Implementation changes in this closeout: **NONE**

> **Post-closeout implementation status (2026-09-07):** P1D remains
> **QUALIFIED IN DECLARED CAVITY-TRANSACTION SUBSCOPE** and P1E is now
> **QUALIFIED IN DECLARED DETERMINISTIC-WALK SUBSCOPE** on source
> `bb232ed3f308375271af07a100adee531444d499`, macOS arm64 workflow #315 SUCCESS.
> P1F is the next development target and is not implemented. M2 OVERALL remains
> **NOT QUALIFIED**. Historical baseline statements below are retained only where
> explicitly labeled as the original research-closeout snapshot.

## 1. Meaning of development ready

The Original Meshing Engine research program is **DEVELOPMENT READY** only in this narrow sense:
- first implementation architecture is sufficiently defined,
- mathematical/topological policies that govern implementation are versioned,
- qualification gates are defined before implementation,
- remaining unknowns can be handled by targeted just-in-time research deltas.

It does **not** mean:
- M2 is qualified,
- the point-cloud tetra constructor is complete,
- the CAD surface mesher is complete,
- boundary recovery is complete,
- arbitrary-CAD TET4 is complete,
- a TET4 solver is complete,
- TET4 is nearly-incompressible or rubber-ready.

## 2. Research package closeout state

| Package | State |
|---|---|
| M0 | STANDING KNOWLEDGE / CONTRACT LIBRARY |
| M1 | QUALIFIED robust-geometry foundation |
| M2 | DESIGN FROZEN; P1A/P1B/P1C/P1D/P1E qualified in declared subscopes; P1F pending; **M2 OVERALL NOT QUALIFIED** |
| M3 | RESEARCH COMPLETE / DESIGN FROZEN — D26-M3-FREEZE-1; implementation not started |
| M4 | RESEARCH COMPLETE / DESIGN FROZEN — D26-M4-FREEZE-1; implementation not started |
| M5 | RESEARCH COMPLETE / DESIGN FROZEN — D26-M5-FREEZE-1; implementation not started |
| M6 | D26-M6-FREEZE-1 retained; frozen mathematics unchanged; product P5 qualification pending |
| M7 | PRODUCT-INTEGRATION RESEARCH COMPLETE / DESIGN FROZEN — D26-M7-FREEZE-1; solver qualification separate |
| M8 | FUTURE DIRECTION DEFINED — D26-M8-DIRECTION-1; not a TET4 blocker |
| M9 | DEFERRED — adaptation/remeshing |
| M10 | DEFERRED — sweep/hex/hybrid |

## 3. P1/M2 executable status

### Historical research-closeout starting baseline

At the original research-closeout baseline:
- P1A semantic Delaunay predicates: **QUALIFIED IN DECLARED SUBSCOPE**,
- P1B typed finite/infinite bootstrap: **QUALIFIED IN BOOTSTRAP SUBSCOPE**,
- P1C brute-force exact point location: **QUALIFIED IN LOCATION SUBSCOPE**,
- P1D cavity oracle + transactional patch: **not yet implemented**.

That list is a historical snapshot, not current status.

### Current post-closeout implementation truth

- P1A semantic Delaunay predicates: **QUALIFIED IN DECLARED SUBSCOPE**,
- P1B typed finite/infinite bootstrap: **QUALIFIED IN BOOTSTRAP SUBSCOPE**,
- P1C brute-force exact point location: **QUALIFIED IN LOCATION SUBSCOPE**,
- P1D cavity oracle + transactional patch: **QUALIFIED IN DECLARED CAVITY-TRANSACTION SUBSCOPE**,
- P1E deterministic adjacency walk: **QUALIFIED IN DECLARED DETERMINISTIC-WALK SUBSCOPE**,
- P1F serial constructor determinism/fingerprint/replay: **NEXT DEVELOPMENT TARGET / NOT IMPLEMENTED**.

Current P1D/P1E-owned gate truth:

**PASS:** M2-G07, M2-G11, M2-G12, M2-G13, M2-G14, M2-G15, M2-G26, M2-G29, M2-G34, M2-G35, M2-G36  
**PARTIAL:** M2-G25  
**DEFERRED:** M2-G20 and P1F/final-constructor gates

No wording promotes M2 overall beyond **NOT QUALIFIED**.

ADR-MESH-0043 remains authoritative for oblique coplanar ghost-circle 3D Euclidean lift semantics and is neither overwritten nor renumbered.

## 4. Frozen global architecture invariant

```text
CAD Geometry
!=
Display Tessellation
!=
FEM Mesh

CAD / B-Rep
-> GeometryEntityId
-> Mesh provenance
-> GeometryAssociationMap
-> Named Selection
-> BC / Load
-> Solver
-> Results
```

Display triangle/line/point indices are not persistent engineering identity.

## 5. Research taxonomy

```text
M0  Knowledge / Contracts
M1  Robust Geometry
M2  Point-Cloud Delaunay
M3  CAD Curve + Surface Meshing
M4  Boundary Recovery + Volume Meshing
M5  Size Fields / Refinement
M6  Quality Optimization
M7  Product TET4 Integration
M8  TET10 / Curved Boundaries
M9  Adaptation / Remeshing
M10 Sweep / Hex / Hybrid
```

Research M-numbers and development P-numbers are separate namespaces.

## 6. Frozen development dependency

```text
DEV-MESH-P1
M1 + M2
serial point-cloud Delaunay
current: P1A PASS / P1B PASS / P1C PASS / P1D PASS / P1E PASS / P1F NEXT
    |
    v
DEV-MESH-P2
M3 CAD Edge + Face surface meshing
    |
    v
DEV-MESH-P3
M4 boundary recovery + CAD-conforming tetra volume mesh
    |
    v
DEV-MESH-P4
M5 sizing + refinement + provenance lifecycle
    |
    v
DEV-MESH-P5
M6 quality optimization
    |
    v
DEV-MESH-P6
M7 TET4 product integration
    |
    v
dedicated TET4 solver/formulation qualification
    |
    v
DEV-MESH-P7
M8 TET10 / curved boundaries
```

## 7. Closeout authorities

- M3: `m3-surface/M3_DESIGN_FREEZE.md` and `M3_QUALIFICATION_GATE_MATRIX.md`
- M4: `m4-boundary-recovery/M4_DESIGN_FREEZE.md` and `M4_QUALIFICATION_GATE_MATRIX.md`
- M5: `m5-sizing/M5_DESIGN_FREEZE.md` and `M5_QUALIFICATION_GATE_MATRIX.md`
- M6: existing `D26-M6-FREEZE-1` remains authoritative without semantic change
- M7: `m7-product-tet4/M7_DESIGN_FREEZE.md` and `M7_QUALIFICATION_GATE_MATRIX.md`
- M8: `m8-tet10/M8_FUTURE_DIRECTION.md`

## 8. Just-in-time research policy

```text
concrete implementation question
-> targeted research delta
-> SOURCE_REGISTRY update
-> ADR if frozen semantics change
-> independent oracle
-> implementation
-> qualification
```

Broad architecture research is not reopened without a concrete blocker.

## 9. Solver boundary

Dedicated authority remains `docs/research/fem/tet4-nearly-incompressible/`.

```text
TET4 mesh generated
!= compressible TET4 solver qualified
!= nearly-incompressible TET4 qualified
!= rubber-ready
```

M7 selects no solver formulation.

## 10. Closeout self-audit

- [x] Historical P1A/P1B/P1C research-closeout truth is preserved and clearly labeled.
- [x] Current post-closeout status records P1D and P1E qualified in their declared subscopes and P1F next.
- [x] ADR-MESH-0043 preserved.
- [x] M3 FaceUse/pcurve ownership and chart aliases are explicit.
- [x] M4 subcomplex coverage and exact constructed sites are explicit.
- [x] D26SITE2/D26LIFT2 are versioned; rounded doubles are not topology authority.
- [x] M5 min-composition, Lipschitz gradation and minimum-size limitation are explicit.
- [x] M6 frozen semantics are unchanged.
- [x] M7 topology/formulation separation and provenance lifecycle are explicit.
- [x] TET4 mesh and rubber solver qualification remain separate.
- [x] M8 is explicitly non-blocking for TET4.
- [x] This closeout changes documentation only.
- [ ] Final closeout commit exact-head CI — checked externally after commit and reported in the closeout report.

The final CI checkbox is repository evidence, not a reason to rewrite the freeze after the workflow run.
