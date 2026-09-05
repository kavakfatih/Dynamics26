# Dynamics26 — Ana Geliştirme Planı

**Plan revizyonu:** 2026-09-03  
**Durum:** Aktif source-of-truth ürün/mimari planı  
**Platform:** macOS / Apple Silicon (`arm64`)  
**Ana dal:** `main`  
**Engineering core:** Modern Fortran  
**Application:** C++20 / Qt 6 / VTK / OCCT  
**Current GUI milestone:** `V1.1.0-beta.3`

Bu planın temel ilkesi **Vertical Slice First, Physics Hardening Next, Extensibility by Contract, Rubber by Verification** yaklaşımıdır.

# 1. Değişmez ilkeler

1. Dynamics26 özgün bir nonlinear FEM/CAE platformudur.
2. ANSYS, Marc, COMSOL, Code_Aster, FEBio ve diğer yazılımlar yalnız research/benchmark/architecture referansıdır; source code kopyalanmaz veya transliterate edilmez.
3. `CAD Geometry != Display Tessellation != FEM Mesh` ayrımı korunur.
4. Document state, solver/result derived state ve transient selection ayrıdır.
5. Persistent engineering mutation canonical command + Undo/Redo yolundan geçer.
6. Solver ve mesh provenance explicit ve test edilebilir olmalıdır.
7. Unsupported capability `Unavailable`/Preflight error olur; sahte değer veya sessiz fallback olmaz.
8. Public solver sınırı additive/versioned C ABI'dir; Fortran module/derived-type ABI public interface değildir.
9. Solver SI unit authority kullanır; GUI/display unit conversion application boundary'dedir.
10. macOS / Apple Silicon tek product platformudur.
11. Ana branch yalnız `main`'dir.
12. Büyük implementation öncesi research + capability contract + verification plan zorunludur.
13. Cross-code correlation doğrulamanın tek kaynağı değildir; bağımsız teori/benchmark ile desteklenir.
14. Extension architecture, document ownership veya physics contracts'i bypass edemez.

# 2. Product architecture hedefi

```text
Qt GUI / Project Tree / Inspector / Viewport
                    ↓
       Canonical Commands + Services
                    ↓
       Capability / Compatibility Layer
                    ↓
          Immutable AnalysisSnapshot
                    ↓
          SolverInputBuilder / DTO
                    ↓
        Stable Additive C/C++ Boundary
                    ↓
             Modern Fortran Core
                    ↓
   Assembly / Elements / Materials / Solvers
                    ↓
       Typed Telemetry + Result Dataset
                    ↓
        VTK Post-processing / Reports
```

Bu mimaride GUI doğrudan element kernel, material state veya Fortran derived type görmez.

# 3. Capability model — yeni merkezi sözleşme

Program büyüdükçe `if material == X && element == Y` zincirleri sürdürülemez. V1.1'den itibaren internal capability descriptors hazırlanmalıdır.

Minimum descriptor axes:

```text
GeometryCapability
MeshTopologyCapability
ElementFormulationCapability
MaterialModelCapability
KinematicsCapability
CompressibilityCapability
BoundaryConditionCapability
LoadCapability
ContactCapability
LinearSolverCapability
NonlinearSolverCapability
ResultCapability
```

Örnek compatibility query:

```text
CanSolve(
  mesh = HEX8,
  element = TotalLagrangianHEX8,
  material = LinearElastic/StVK,
  kinematics = FiniteStrain,
  load = ReferenceAreaTotalForce,
  solver = FullNewton
)
```

Sonuç yalnız `true/false` değil:

```text
Supported
SetupOnly
Unsupported
RequiresRemesh
RequiresDifferentFormulation
RequiresDifferentLinearBackend
Experimental
```

ve kullanıcıya açıklanabilir reason taşımalıdır.

Bu katman Preflight, Inspector enable/disable, solver routing, docs capability matrix ve gelecekte plugin manifest için tek semantik kaynaktır.

# 4. Research methodology

## 4.1 Kaynak otoritesi

**Tier A — Physics authority**
- continuum mechanics/FEM textbooks,
- peer-reviewed papers,
- standards,
- analytical/closed-form benchmarks.

**Tier B — Product semantics**
- ANSYS/Marc/COMSOL resmi documentation.

**Tier C — Open architecture/verification**
- Code_Aster,
- FEBio,
- Netgen ve ilgili open-source projects.

**Tier D — Troubleshooting only**
- forums/community posts.

Forum cevabı material law veya numerical formulation seçiminin ana dayanağı olamaz.

## 4.2 Research record şablonu

```text
R-XXX Title
1. Engineering need
2. Current Dynamics26 source reality
3. Theory/equations
4. ANSYS behavior
5. Marc behavior
6. COMSOL behavior
7. Code_Aster/FEBio/open-source architecture
8. License/source boundary
9. Alternatives
10. Adopt / Adapt / Reject
11. Capability contract
12. Implementation boundaries
13. Verification matrix
14. Acceptance thresholds
15. Deferred risks
```

# 5. Phase A — V1.1 Supported Nonlinear Workflow

Amaç kullanıcının desteklenen model üzerinde kesintisiz product workflow tamamlamasıdır.

## A0. Source-reality gate

Mevcut gerçekler:

- geometry: OCCT/STEP topology ve selection altyapısı,
- mesh: structured HEX8 box baseline,
- product general solve: DirectLinear,
- nonlinear core: gerçek `solve_nonlinear_static`, Full/Modified Newton, stepping, cutback, line search, history,
- nonlinear general product C ABI: henüz yok,
- contact/mixed nonlinear: verification paths mevcut, general product consumer değil.

V1.1 planı bu gerçekleri genişletmeden çalışır.

## A1. Setup UX

Hedef tree:

```text
Model
├─ Geometry
├─ Materials
├─ Connections
├─ Coordinate Systems
├─ Mesh
└─ Analyses
   └─ Nonlinear Static
      ├─ Analysis Settings
      ├─ Fixed Support
      ├─ Force / Pressure
      └─ Solution
         ├─ Total Deformation
         ├─ Directional Deformation
         ├─ Equivalent Stress
         └─ Reaction Force
```

Coordinate Systems klasörü minimum skeleton olarak erken eklenebilir; local BC/load semantics daha sonra enable edilir.

## A2. Selection / persistent scopes

- Geometry Body/Face/Edge/Vertex,
- FEM Node/Element/Facet,
- click/multi/rectangle,
- filter,
- selection feedback,
- Named Selection,
- edit/apply/cancel,
- hide/show/isolate,
- stale scope diagnostics.

Persistent scope her zaman geometry/mesh provenance guard taşır. Mesh IDs generation-specific'tir; CAD geometry-based scope remesh sonrası current boundary facets'e yeniden çözülür.

## A3. BC / Load authoring

Fast path:

```text
Select Face(s)
→ Fixed Support / Force
→ persistent scope + analysis object one Undo macro
```

Alternative:

```text
Insert load/support
→ enter scope edit mode
→ select entities
→ Apply
```

### Fixed Support

Beta.3: Ux=Uy=Uz=0 on face scope.

Later:
- Displacement,
- Symmetry,
- Cylindrical/Remote/Elastic supports.

### Load object model

Future-ready minimum fields:

```text
LoadKind
Scope
CoordinateSystemId
DefinitionMode
Vector / magnitude
Configuration = Reference | Current
Follower = false/true
AmplitudeFunctionId
```

Beta.3 only validated combinations enabled.

## A4. Surface load mathematics

### Total Force

```text
A_ref = ∫Γ dA
traction_ref = F_total / A_ref
f_e = ∫Γe Nᵀ traction_ref dΓ
```

### Verification

For each test surface:

```text
Σ f_i = F_total
Σ (x_i × f_i) = ∫Γ x × traction dΓ
```

within numerical tolerance.

For planar QUAD4 + constant traction this naturally yields area-weighted equivalent nodal forces. Shared-node facet contributions are assembled; node count is not the weighting basis.

### Pressure

```text
traction = -p n
```

Reference/current surface and follower semantics are explicit. Large-deformation follower pressure is separate from initial reference-area Pressure capability.

## A5. Mesh readiness

V1.1 product solve supports only actual current mesher capability.

Preflight checks:

- generated mesh exists,
- mesh current with geometry/settings,
- supported topology,
- supported formulation,
- no inverted/degenerate elements,
- minimum quality threshold according to benchmark policy,
- boundary facets resolvable for every surface BC/load.

Arbitrary STEP geometry does not become solve-ready by using its bounding box.

## A6. Material readiness

Minimum product material:

```text
Material
├─ Identity
├─ Density
└─ Linear Elastic
   ├─ E
   └─ ν
```

Nonlinear geometric behavior may use existing finite-strain StVK-like element/material path only after product bridge verification.

Material assignment is body-scoped and capability checked against element formulation.

## A7. Nonlinear SolverInput architecture

### Immutable snapshot

Before solver call, all persistent state is resolved to immutable POD-like input:

```text
AnalysisSnapshot
├─ source revisions / hashes
├─ mesh nodes/elements/facets
├─ formulation assignments
├─ material model cards
├─ constraints
├─ equivalent load vectors
├─ nonlinear controls
├─ output requests
└─ capability stamp
```

Once Solving starts, document edits do not mutate this snapshot.

### C ABI

Recommended direction:

```text
fem_nonlinear_model_input_v1
fem_nonlinear_options_v1
fem_nonlinear_result_v1
fem_nonlinear_history_entry_v1
```

C-interoperable descriptors should include structure/version size or explicit API version so additive evolution can be controlled.

Pointers/array capacities, ownership and lifetime must be explicit. Fortran adapter validates every ID/count/range before model construction.

## A8. Newton-Raphson product mapping

Core equation convention must be single-source documented:

```text
R(u, λ) = λ f_ext - f_int(u)
K_T Δu = R
u_{i+1} = u_i + α Δu
```

Beta.3 mapping:

- Full Newton,
- Modified Newton,
- adaptive stepping,
- increment min/initial/max,
- cutback/growth,
- line search,
- residual criterion,
- displacement correction criterion.

ANSYS's Full/Modified distinction and line-search/automatic stepping behavior are UX references; Dynamics26 solver semantics remain its own implementation.

No arc-length until standard load control and failure diagnostics are mature.

## A9. Matrix-property / linear-backend contract

Nonlinear solver depends on a linearized system, so element/material/contact choices must declare matrix properties.

Required metadata:

```text
Symmetric / Unsymmetric
SPD expected / Indefinite / General
Real
Pressure saddle-point present
Contact/friction contribution
```

Routing rules:

- dense reference: tests/small models,
- sparse CG: only compatible SPD systems,
- Accelerate sparse direct: macOS candidate, capability benchmark required,
- mixed `u-p`: requires indefinite-capable backend,
- friction/contact may require unsymmetric-compatible backend depending formulation.

A solver backend cannot be selected because it is “fast”; it must be mathematically compatible.

## A10. Convergence / solver state

Document state stores solver intent, not current Newton iteration.

Derived `SolverSession` owns:

- state machine,
- current attempt/step/iteration,
- load factor,
- residuals,
- line-search alpha,
- cutbacks,
- warnings,
- cancellation request,
- final status.

Rollback restores solver trial state, not document state.

## A11. Result architecture

Separate:

```text
ResultDefinition = document request
ResultDataset    = derived solve output
ViewportSelection = transient presentation state
```

Result field metadata:

- field name,
- location: Node / Element / IntegrationPoint / Facet,
- components,
- unit,
- step/substep/load factor,
- averaging/derivation policy.

This prevents an integration-point stress from being silently treated as a nodal stress.

# 6. Phase B — V1.2 Geometry-Aware Meshing and Numerical Hardening

## B1. Meshing engine strategy

Building a robust general 3D volume mesher from scratch is not on the V1.1 critical path.

Primary research candidate: Netgen.

Reasons to evaluate:

- automatic tetrahedral volume meshing,
- B-Rep/STEP through geometry kernel,
- optimization/refinement,
- library API,
- macOS,
- LGPL-2.1.

Adoption gate:

- legal/license review,
- deterministic arm64 builds,
- no display-tessellation-as-FEM-mesh shortcut,
- OCCT/B-Rep transfer,
- CAD Face provenance,
- local sizing,
- quality metrics,
- error reporting,
- acceptable performance.

If a third-party mesher cannot preserve required provenance/quality contract it is rejected regardless of mesh visual quality.

## B2. Mesh data model evolution

`SimulationMesh` should grow without losing stable concepts:

```text
MeshNode
VolumeElement
BoundaryFacet
ElementTopology
PolynomialOrder
SourceGeometryPersistentKey
Material/Region assignment
Quality metrics
Generation/revision
```

Surface Facet must remain first-class because loads/contact/result scopes depend on it.

## B3. Element qualification

Do not equate mesh generation with element correctness.

For every topology/formulation:

1. shape-function partition of unity,
2. Jacobian mapping,
3. rigid-body mode test,
4. constant-strain patch,
5. stiffness symmetry where expected,
6. geometric tangent check,
7. material tangent check,
8. distortion/inversion detection,
9. mesh convergence,
10. locking benchmarks where relevant.

## B4. Newton certification

Additional gates:

- analytical residual checks,
- finite-difference Jacobian/tangent comparison,
- manufactured-solution tests,
- deterministic iteration history,
- step cutback recovery,
- line-search regression,
- checkpoint/restart equivalence,
- singular-system failure paths.

## B5. Scalability

Performance work begins only after correctness gates:

- sparse assembly profiling,
- factorization costs,
- memory estimate before solve,
- size-based backend routing,
- cancellation responsiveness,
- optional parallel assembly research.

# 7. Phase C — Extension-ready architecture then V1.3 SDK

## C0. Internal contracts before dynamic plugins

V1.1/V1.2 establish:

- capability registry,
- typed descriptors,
- immutable DTOs,
- canonical command mutation,
- versioned APIs,
- service ownership.

## C1. Plugin host

V1.3 dynamic extension host:

```text
ExtensionManifest
├─ id
├─ name/version
├─ host API range
├─ platform/arch
├─ capabilities
├─ dependencies
└─ entry library
```

Extension categories:

- Workflow/UI,
- Geometry Importer,
- Mesh Provider,
- Material Model,
- Solver Backend,
- Result Evaluator,
- Exporter/Report.

## C2. Material model plugin contract

Finite strain interface must explicitly define:

- input kinematics (`F`, strain measure),
- stress measure returned,
- algorithmic tangent measure/layout,
- state variables and commit/revert,
- temperature/time fields if supported,
- thread safety,
- initialization,
- error/status behavior,
- SI units.

Code_Aster/MFront integration experience is a warning against ambiguous strain/stress/tangent conventions; Dynamics26 interface makes these explicit in the descriptor.

## C3. Conformance tests

A plugin cannot become production-capable solely because it loads.

Required:

- manifest validation,
- ABI compatibility,
- lifecycle test,
- error-path test,
- unit consistency,
- capability declaration verification,
- numerical benchmark for physics extensions.

# 8. Phase D — V1.4 Rubber / Elastomer Mechanics

## D1. Rubber architecture

```text
Experimental Data
→ Fit Dataset
→ Constitutive Parameters
→ Material Point Law
→ Compressibility Strategy
→ Element/Formulation
→ Nonlinear Solve
→ Contact
→ Result
→ Physical Correlation
```

## D2. Hyperelastic laws

Priority:

1. Neo-Hookean,
2. Mooney-Rivlin 2P,
3. Yeoh,
4. Ogden 1–3 term.

For each model document:

- strain-energy density,
- invariants/principal-stretch convention,
- volumetric part,
- stress derivation,
- consistent tangent,
- parameter constraints,
- zero-strain checks,
- infinitesimal shear modulus relation.

## D3. Fitting

Experimental test families:

- uniaxial tension/compression,
- planar/pure shear,
- biaxial tension,
- volumetric/compression.

Fit architecture stores raw test data and transformation metadata separately from fitted model parameters.

Acceptance:

- reproducible objective function,
- normalized residuals/weighting,
- train/validation ranges,
- model stability warnings,
- extrapolation display,
- parameter covariance/uncertainty research.

## D4. Incompressibility qualification

COMSOL and Marc references reinforce that near-incompressibility is a formulation problem, not only a material parameter.

Compare:

- mixed `u-p`,
- Herrmann,
- B-bar/selective,
- F-bar,
- reduced integration + stabilization.

Mixed `u-p` adds pressure DOFs and changes linear-system structure; backend suitability is part of formulation qualification.

## D4.1. TET4 nearly-incompressible research track

Research package:

`docs/research/fem/tet4-nearly-incompressible/`

TET4 does not inherit the current HEX8 Q1/P0 mixed formulation by topology substitution.

Candidate comparison:
- pure P1 and P1/P0 negative controls,
- MINI P1+bubble/P1 as stable mixed reference candidate,
- pressure-projection stabilized P1/P1,
- patch F-bar displacement-only alternative,
- later hybrid/mixed TET10.

Qualification includes:
- numerical inf-sup mesh sequence where applicable,
- pressure/spurious-mode checks,
- exact consistent tangent,
- K/G locking sweep,
- nonlinear local-state rollback,
- M6 geometry-quality cross matrix,
- higher-order comparison.

No candidate is selected for product use before this evidence exists.

## D5. Verification pyramid

```text
Material point analytical limits
→ stress/tangent numerical differentiation
→ single HEX/TET deformation modes
→ patch tests
→ volumetric locking benchmark
→ Cook-type bending/shear benchmark where relevant
→ distorted-mesh test
→ cross-code benchmark
→ rubber block/shear/torsion physical correlation
→ engine mount / crank pulley component correlation
```

# 9. Phase E — V1.5 Advanced Rubber / Contact / Time Dependence

- finite sliding surface-to-surface contact,
- contact search + active-set robustness,
- penalty / augmented-Lagrangian comparison,
- friction + stick/slip state,
- contact consistent/algorithmic tangent,
- viscoelastic Prony framework,
- Mullins/cyclic softening,
- temperature and frequency dependence,
- preload/history,
- component test correlation database.

# 10. Deferred advanced algorithms

Only after previous gates:

- arc-length/Riks,
- quasi-Newton/BFGS,
- trust-region research,
- adaptive remeshing,
- nonlinear dynamics,
- distributed/HPC solving.

# 11. Verification and release hierarchy

Every physics feature:

```text
RESEARCHED
→ EQUATIONS / CONTRACT DOCUMENTED
→ CODE EXISTS
→ MATERIAL-POINT / UNIT TEST
→ ELEMENT TEST
→ SYSTEM BENCHMARK
→ CROSS-CODE COMPARISON
→ FEATURE WORKS IN PRODUCT
→ USER VALIDATED
→ COMPONENT CORRELATED (where applicable)
```

Every source milestone:

- Debug core regression,
- Release core regression,
- C ABI consumer smoke,
- feature-specific numerical benchmark,
- GUI application acceptance,
- native arm64 gate,
- selection/scope regression,
- Light/Dark audit.

`USER VALIDATED` remains physical Mac acceptance, not CI.
