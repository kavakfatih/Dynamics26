# Meshing Decision Log

## ADR-MESH-0001 — Dynamics26 will develop an original meshing engine

**Status:** ACCEPTED  
**Date:** 2026-09-05

### Context

The initial V1.2 roadmap considered integrating a mature external volume mesher. Research confirmed that Netgen, Gmsh, MMG, TetGen and CGAL provide valuable algorithms, architectures and benchmark ideas. The project direction has now changed: Dynamics26 should own the production meshing implementation.

### Decision

- Do not make Netgen, Gmsh, MMG, TetGen or CGAL the V1.2 production meshing engine.
- Develop original Dynamics26 meshing algorithms under the existing \`femcae::meshing\` architecture.
- Use peer-reviewed papers and mathematical references as primary algorithm sources.
- Use open-source code only for architecture/failure/test study under \`CLEAN_ROOM_POLICY.md\`.
- Preserve \`GeometryEntityId\` as CAD authority and \`SimulationMesh\` as solver mesh authority.
- Keep \`StructuredHexMesher\` as a validated baseline while the unstructured engine matures.

### Rationale

Advantages:

- full control of geometry/provenance contracts,
- no product dependence on a third-party meshing API,
- no inherited external implementation/license constraints in the core,
- ability to tune specifically for nonlinear/rubber automotive workflows,
- meshing research becomes an owned Dynamics26 competency.

Costs:

- substantially larger R&D effort,
- robust computational geometry is difficult,
- quality and failure handling must be earned through a large verification corpus,
- arbitrary CAD meshing arrives later than an adapter-based approach.

### Consequence

V1.2 should be treated as a staged meshing R&D program, not a single feature.

---

## ADR-MESH-0002 — Delaunay-first tetrahedral research track

**Status:** PROPOSED  
**Date:** 2026-09-05

### Context

Strong literature exists for incremental Delaunay, constrained boundary recovery and Delaunay refinement. Commercial/open systems also demonstrate Delaunay and advancing-front hybrids.

### Proposed direction

Start original volume-meshing research with:

1. robust geometric predicates,
2. incremental Delaunay tetrahedralization on point clouds,
3. constrained boundary recovery,
4. Delaunay refinement,
5. separate quality optimization.

Keep advancing-front tetra meshing as an independent research track and potential hybrid component.

### Why not accepted yet

Boundary recovery, sliver control, sizing behavior and CAD-surface strategy need experiments before the production algorithm family is fixed.

---

## ADR-MESH-0003 — Named Selection is a meshing scope

**Status:** PROPOSED  
**Date:** 2026-09-05

### Context

ANSYS and COMSOL both reuse persistent selections for mesh controls. Dynamics26 already owns a persistent Named Selection system.

### Proposed direction

Allow future mesh-control definitions such as local size to scope through persistent Named Selection ObjectIds, resolved into GeometryEntityIds before meshing.

### Constraint

The low-level meshing kernel receives resolved immutable geometry identities; it does not depend on Qt/ProjectModel/NamedSelectionService.

---

## Open decisions

| ID | Topic | Needed evidence |
|---|---|---|
| ADR-MESH-0002 | Delaunay-first volume track | M1/M2/M4 experiments |
| ADR-MESH-0003 | Named Selection mesh-control scope | mesh-control document model design |
| ADR-MESH-0004 | First surface mesher | parameter-space CDT vs advancing front experiments |
| ADR-MESH-0005 | Robust predicate implementation strategy | license/source-boundary review + M1 benchmark |
| ADR-MESH-0006 | Quality metric release gates | nonlinear TET4 sensitivity study |
| ADR-MESH-0007 | Curvature/proximity size-field formulas | M3/M5 experiments |


---

## ADR-MESH-0005 — Robust predicate implementation strategy

**Status:** PROPOSED  
**Date:** 2026-09-05

### Research finding

M1 establishes that CAD/modeling tolerance and topological predicate correctness are separate concerns. A fixed epsilon on determinant magnitude is not accepted as a general topology policy.

### Leading design

```text
finite-input validation
→ fast binary64 determinant
→ certified error filter
→ adaptive exact fallback when uncertain
→ PredicateSign {Negative, Zero, Positive}
```

True exact degeneracy remains `Zero`. Symbolic perturbation/tie-breaking belongs to M2 or the consuming topology algorithm.

### Verification authority

An independent exact-rational oracle generated from the exact binary64 input values must exist before the production adaptive path can be qualified.

### Compiler constraint

The predicate target must not be compiled under floating-point modes that invalidate the arithmetic/error-bound assumptions. Fast-math is therefore disallowed for this target; FP contraction policy must match the eventual filter proof.

### Why still PROPOSED

The arithmetic implementation strategy is not accepted until:

1. independent oracle fixtures exist,
2. candidate filter/fallback designs are benchmarked,
3. Apple Silicon Debug/Release behavior is tested,
4. clean-room/source-boundary review is complete.


### M1.1 exact-oracle research update — 2026-09-05

Evidence now supports the following parts of ADR-MESH-0005:

- predicate truth is exact with respect to stored finite binary64 inputs,
- a test-only exact-rational oracle is feasible using standard integer/rational arithmetic,
- binary64 inputs can be converted to common dyadic integers without changing predicate sign,
- filter architecture should permit conservative fallback but never false certification,
- commercial ANSYS/COMSOL/Marc tolerance and repair controls are geometry/meshing policy, not evidence for an epsilon-based predicate design.

ADR-MESH-0005 remains **PROPOSED** until executable oracle and filter experiments are complete.


### M1.2 certified-filter research update — 2026-09-05
The first Dynamics26 F0 filter specification is now derived independently:
- expanded-monomial evaluation graph,
- gamma_n error accumulation,
- fast sign certification only outside the computed error envelope,
- no fast-path Zero,
- explicit normal-range gate,
- initial `-fno-fast-math -ffp-contract=off` compiler contract.

Leading conservative first-order factors are approximately:
- orient2d: 4u,
- orient3d: 10u,
- incircle: 18u,
- insphere: 80u,
times the documented computed absolute-monomial sum and denominator corrections.

ADR-MESH-0005 remains **PROPOSED** until executable M1.1/M1.2 experiments pass.
