# Dynamics26 — Aktif Sürüm Yol Haritası

**Plan revizyonu:** 2026-09-05  
**Platform:** macOS / Apple Silicon (`arm64`)  
**Ana dal:** `main`  
**Engineering core baseline:** `V1.0.2`  
**GUI baseline:** `V1.1.0-beta.3`

Bu roadmap Beta.3 sonrasında Dynamics26 için **kritik yolu** tanımlar. Aktif hedef mevcut nonlinear product vertical slice'ı RC.1 kapsamında sertleştirmek; ardından geometry-aware meshing/scalability, extension SDK ve rubber mechanics'e ilerlemektir.

# 1. Ürün hedefi ve capability envelope

İlk product workflow:

```text
Geometry
→ Selection / Scope
→ Material
→ Mesh
→ Analysis
→ Supports / Loads
→ Preflight / Capability Check
→ Nonlinear Solve
→ Convergence
→ Results
```

Ancak bugünkü repository gerçeğinde keyfi STEP geometry için genel volume mesher yoktur. `MeshService` structured HEX8 box baseline kullanır. Bu nedenle iki durum ayrı tutulur:

- **Setup-ready:** engineering objects ve scopes tanımlanabilir.
- **Solve-ready:** geometry + mesh + element + material + load + solver combination product consumer tarafından doğrulanmıştır.

Beta.3'te unsupported combination hiçbir zaman sessizce başka geometri/formulation/solver'a çevrilmez.

# 2. V1.1.0 — Supported Nonlinear Workflow Vertical Slice

| Milestone | Hedef | Release koşulu |
|---|---|---|
| **V1.1.0-beta.2** | Solver Workspace / typed telemetry / diagnostics | Tamamlandı — automated closeout; USER VALIDATED ayrı |
| **V1.1.0-beta.3** | **Supported Nonlinear Workflow Vertical Slice** | Tamamlandı — exact-HEAD automated gates geçti; USER VALIDATED ayrı |
| **V1.1.0-rc.1** | **hardening / stability / diagnostics / native UX** | Aktif — all V1.1 gates + physical Mac acceptance |
| **V1.1.0** | ilk kullanılabilir nonlinear CAE baseline | capability matrix + user workflow + release evidence tamam |

## B3.0 — Capability Matrix + Preflight Contract

Bu work package Beta.3'ün başlangıç gate'idir.

Capability descriptor aşağıdaki eksenleri ayırmalıdır:

```text
Geometry source
Mesh topology
Element formulation
Material model
Kinematics
Compressibility
Load / BC type
Contact
Linear backend
Nonlinear algorithm
Result fields
```

İlk solve-ready subset:

- parametric box veya `boxDescriptor` ile doğrulanmış box-compatible CAD,
- structured HEX8,
- total-Lagrangian/geometric-nonlinear için doğrulanmış mevcut element path,
- Linear Elastic / StVK-type nonlinear verification material path,
- Fixed Support,
- Total Force reference-area traction,
- Full / Modified Newton,
- adaptive load stepping,
- line search,
- displacement/stress/reaction results.

`Pressure`, hyperelastic, mixed `u-p`, Contact veya arbitrary STEP volume solve ancak kendi product consumer gate'i tamamlanırsa enabled olur.

UI status önerisi:

```text
Ready
Setup only
Unavailable for current formulation
Stale
Invalid
```

## B3.1 — Selection / Scope Productivity

- Geometry Body / Face / Edge / Vertex selection,
- FEM Node / Element / Facet selection,
- click/Shift/Command multi-selection,
- rectangle selection,
- context-aware filters,
- current selection → persistent scope,
- Named Selection create/edit,
- scope edit/apply/cancel,
- hide/show/isolate,
- stale geometry/mesh scope diagnostics,
- Navigator/Inspector/Viewport synchronization.

### Fast authoring

```text
select Face(s)
→ Fixed Support / Force
→ persistent scope created/assigned in one Undo transaction
→ created analysis object selected
```

Kullanıcı manuel Named Selection oluşturmaya zorlanmamalıdır. Mevcut auto-Named-Selection implementation hızlı baseline olarak kullanılabilir; uzun vadede BC/load-owned persistent geometry scope ancak **tek canonical scope resolver'ı bozmadan** tasarlanır.

## B3.2 — Supports / Loads + Surface Integration

### Fixed Support

- Face scope,
- all displacement DOFs fixed baseline,
- selected-surface highlight,
- area-aware readable support glyph,
- mesh regeneration re-resolution/stale check.

### Total Force

Semantik:

```text
F_total = entire selected-scope resultant
A_ref   = Σ reference surface areas
t_ref   = F_total / A_ref
```

Her boundary facet için:

```text
f_e = ∫Γe Nᵀ t_ref dΓ
```

Acceptance:

- arrow count solver load değerini değiştirmez,
- `Σ f_node` resultant'ı `F_total` ile numerical tolerance içinde eşleşir,
- nodal equivalent forces'in moment resultant'ı surface traction momentiyle uyuşur,
- mesh refinement toplam resultant'ı değiştirmez,
- nonuniform facet boyutlarında `F/node_count` yaklaşımı kullanılmaz.

### Pressure

- ayrı load type,
- sign convention ve surface normal açık,
- reference pressure baseline ayrı capability,
- current/deformed-area follower pressure daha sonra,
- unsupported follower behavior UI'de enabled görünmez.

### Coordinate systems

Beta.3 Global coordinate system kullanabilir; local/cylindrical frame için veri modeli V1.2 öncesi tanımlanır.

## B3.3 — Material + Mesh Readiness

### Material minimum

- body → material assignment,
- Linear Elastic `E`, `ν`, density,
- unit validation,
- missing/duplicate assignment diagnostics,
- capability badge for constitutive model.

### Mesh minimum

Mevcut gerçek capability:

- structured HEX8,
- parametric box,
- gerçek CAD ancak box-compatible ise geometry provenance,
- boundary Facet provenance,
- node/element/facet count,
- scaled Jacobian quality,
- stale mesh lifecycle.

Beta.3 Preflight, arbitrary CAD'in structured-box mesher ile çözülemeyeceğini açıkça bildirmelidir; bounding-box approximation solve değildir.

## B3.4 — Immutable Solver Input + General Nonlinear C ABI

Required chain:

```text
Document State
→ Capability/Preflight
→ Immutable AnalysisSnapshot
→ SolverInputBuilder
→ versioned additive C ABI
→ Fortran model_t construction
→ solve_nonlinear_static()
→ SolverSessionTelemetry
→ ResultSet
```

### Snapshot minimum

- nodes + coordinates,
- element IDs + connectivity,
- formulation IDs,
- material assignments + material parameters,
- constraints,
- assembled equivalent surface loads,
- nonlinear controls,
- requested result fields,
- capability/API version stamp.

### C ABI direction

Demo-specific long argument lists büyütülmez. Yeni product API versioned/POD descriptors veya açık array+count contracts kullanmalıdır.

Kurallar:

- Fortran derived types public ABI değildir,
- all physical solver input SI units,
- caller-provided capacities/counts açık,
- errors typed/status-code based,
- no hidden demo geometry,
- no DirectLinear fallback,
- result/history memory ownership explicit.

### Nonlinear controls

- Full Newton default,
- Modified Newton optional,
- max iterations,
- initial/min/max `Δλ`,
- adaptive stepping,
- cutback/growth,
- line search,
- residual relative tolerance,
- displacement relative tolerance.

Arc-length/Riks ve quasi-Newton/BFGS Beta.3 kapsamı değildir.

## B3.5 — Linear Backend Suitability Gate

Core'da mevcut backend'ler:

- dense reference,
- sparse CG,
- Apple Accelerate sparse direct adapter.

Beta.3 tiny model baseline dense reference kullanabilir ancak DOF size guard ve açık backend telemetry olmalıdır.

V1.1 içinde şu metadata hazırlanır:

```text
Matrix symmetry
Definiteness expectation
Indefinite / saddle-point requirement
Unsymmetric tangent requirement
Backend compatibility
```

CG yalnız SPD-compatible sistemlerde seçilebilir. Gelecekte mixed `u-p` veya friction/contact için indefinite/unsymmetric system gereksinimi solver capability gate'inden geçmeden product support ilan edilmez.

## B3.6 — Convergence + ResultSet MVP

### Telemetry

- attempt,
- accepted step,
- iteration,
- load factor `λ`,
- load increment `Δλ`,
- absolute/relative residual,
- displacement increment/relative displacement,
- line-search alpha,
- cutback reason,
- minimum `J` if available,
- backend/factorization information where available.

### Result architecture

Target model:

```text
SolveDataset
└─ ResultSet(step, substep, loadFactor, converged)
   ├─ nodal displacement
   ├─ nodal reaction
   ├─ element / integration-point stress
   └─ derived fields
```

Beta.3 Results MVP:

- Total Deformation,
- Directional Deformation,
- Equivalent Stress,
- Reaction Force,
- deformed + undeformed overlay,
- scale factor,
- min/max,
- probe,
- final accepted increment at minimum.

Integration-point, element-averaged ve nodal-averaged stress aynı field gibi sunulmaz.

## B3.7 — Vertical-Slice Acceptance

Deterministic first benchmark:

- box/cantilever geometry,
- structured HEX8,
- Linear Elastic/StVK-like finite-strain product path,
- Fixed Support on one face,
- distributed Total Force on opposite face,
- geometric nonlinearity,
- multiple load increments,
- convergence history,
- displacement/stress/reaction ResultSet.

Numerical gates:

- completed load factor = target,
- configured convergence criteria satisfied,
- applied-force/reaction equilibrium within benchmark tolerance,
- surface-load resultant conservation,
- no hidden solver fallback,
- no NaN/Inf,
- no negative/inverted `J` accepted silently,
- save/reopen engineering setup reproducible,
- Undo/Redo setup mutations canonical,
- derived results/telemetry not document Undo state.

USER VALIDATED additionally tests real pointer/trackpad/keyboard/Light/Dark workflow.

# 3. V1.2.0 — Geometry-Aware Meshing + Scalable Nonlinear Foundation

V1.2 begins with the blocker that currently prevents real arbitrary-part workflows.

## M1 — Original Dynamics26 Meshing Engine Foundation

Production meshing strategy için karar kapanmıştır: Dynamics26 kendi özgün meshing
engine'ini clean-room araştırma ve doğrulama zinciriyle geliştirir. External mesher
entegrasyonu production stratejisi değildir.

Repository'deki meshing R&D source of truth:

- `docs/research/meshing/DECISION_LOG.md`,
- `docs/research/meshing/CLEAN_ROOM_POLICY.md`,
- `docs/research/meshing/ROADMAP_AND_EXPERIMENTS.md`.

Güncel gate durumu:

- M1 robust geometry foundation: **QUALIFIED**,
- exact predicates / certified fast path / deterministic degeneracy foundation: qualified M1 evidence,
- tetra topology primitives/validator: qualified M1 scope,
- M2: **AUTHORIZED — M2.0 REFERENCE ARCHITECTURE & EXPERIMENT PLAN FIRST**.

Gmsh, Netgen, TetGen, CGAL, MMG ve benzeri projeler teori, architecture,
failure-mode ve benchmark araştırma kaynağı olarak kullanılabilir; kaynak kodu
kopyalama/transliteration veya production mesher dependency adoption yapılmaz.

## M2 — Arbitrary B-Rep Volume Meshing

- surface triangulation owned by mesher pipeline, not display mesh,
- tetra volume mesh baseline,
- boundary facet provenance,
- local/global sizing,
- curvature refinement,
- sliver/quality controls,
- regenerate/stale lifecycle,
- Named Selection remapping via CAD authority.

TET4 availability does **not** automatically mean nonlinear/rubber qualification. TET4/TET10/HEX strategies receive separate element and locking/convergence gates.

## M3 — Element/Formulation Qualification

- TET4/TET10,
- HEX8/higher-order roadmap,
- integration rules,
- geometric stiffness,
- finite-strain tangent,
- distortion/inversion monitoring,
- patch tests,
- bending/shear/volumetric locking checks,
- mesh convergence.

## M4 — Scalable Linear Solve Layer

- matrix-property metadata,
- sparse direct/iterative benchmark,
- backend suitability rules,
- factorization reuse where mathematically valid,
- nonlinear tangent refactorization telemetry,
- DOF/memory guardrails.

# 4. V1.3.0 — Extension / Plugin Architecture & SDK

## Extension-ready contracts start before V1.3

V1.1/V1.2 internal architecture should already expose typed descriptors:

- `CapabilityDescriptor`,
- `MaterialModelDescriptor`,
- `ElementFormulationDescriptor`,
- `SolverBackendDescriptor`,
- `MeshProviderDescriptor`,
- `ResultDescriptor`,
- versioned Solver/Input DTO.

This avoids rebuilding core boundaries when dynamic plugins arrive.

## V1.3 plugin classes

- Workflow / UI,
- Geometry Importer,
- Mesh Provider,
- Material Model,
- Solver Backend,
- Result Evaluator,
- Exporter / Report.

Rules:

- C++ extension host,
- versioned manifest,
- host API min/max compatibility,
- declared capabilities/dependencies,
- platform/architecture declaration,
- plugin cannot bypass command/document mutation boundary,
- plugin cannot depend on compiler-specific Fortran module ABI,
- material plugin must explicitly declare kinematics, stress measure, tangent measure and state ownership,
- extension conformance tests required.

Initial dynamic plugins are trusted/in-process. Stronger isolation or out-of-process execution is a later security/reliability enhancement.

# 5. V1.4.0 — Rubber / Elastomer Mechanics Foundation

Rubber roadmap is a coupled stack, not a model-name checklist.

## R1 — Material-Point Framework

- deformation gradient `F`,
- `J = det(F)`,
- objective stress measure contract,
- energy/stress/tangent consistency,
- state variables,
- finite-difference tangent verification,
- SI units.

## R2 — Hyperelastic Models

Order:

1. Neo-Hookean,
2. Mooney-Rivlin 2P,
3. Yeoh,
4. Ogden 1–3 term.

Each:

```text
W
→ stress
→ consistent tangent
→ compressible/isochoric-volumetric split
→ material-point tests
→ single-element tests
→ component benchmark
```

## R3 — Experimental Fitting

- uniaxial,
- planar/pure shear,
- biaxial,
- volumetric/compression,
- engineering/true measure conversion,
- least-squares objective,
- weighting,
- parameter bounds,
- stability checks,
- extrapolation warning,
- confidence/fit-quality report.

## R4 — Nearly Incompressible / Incompressible Formulations

Research candidates:

- mixed `u-p`,
- Herrmann,
- selective/B-bar,
- F-bar,
- reduced integration + stabilization.

Gate metrics:

- volumetric locking,
- pressure oscillation,
- patch tests,
- bending/shear response,
- distortion sensitivity,
- contact compatibility,
- tangent consistency,
- linear-system definiteness/symmetry,
- computational cost.

`ν≈0.5` displacement-only penalty behavior tek başına “incompressible support” değildir.

## R5 — Rubber Verification Pyramid

```text
Analytical material point
→ finite-difference tangent
→ single-element deformation modes
→ patch / locking tests
→ mesh refinement
→ published benchmark
→ ANSYS / Marc / COMSOL cross-code comparison
→ physical component test correlation
```

Cross-code equality tek başına proof değildir.

# 6. V1.5.0 — Contact / Time Dependence / Advanced Elastomer

- deformable-deformable finite sliding,
- penalty + augmented-Lagrangian qualification,
- friction/stick-slip,
- contact pressure/opening/slip results,
- viscoelasticity / Prony,
- Mullins/cyclic softening,
- temperature dependence,
- frequency dependence,
- preload/history,
- engine mount / crank pulley / torsion annulus benchmark library.

# 7. V1.6+ — Advanced CAE

- arc-length/path following,
- advanced adaptivity/remeshing,
- shells/higher-order elements,
- plasticity/damage,
- dynamics/harmonic/transient,
- large-scale sparse/HPC,
- production extension ecosystem,
- qualification/correlation suites.

# 8. Mandatory Research Gate

Every major work package gets a record under `docs/research/`:

```text
Problem / user need
Theory / equations
Reference-product semantics
ANSYS official observation
Marc official observation
COMSOL official observation
Code_Aster / FEBio / open-source architecture observation
Current Dynamics26 source reality
License/source boundary
Adopt / Adapt / Reject
Capability contract
Architecture impact
Verification cases
Acceptance thresholds
```

Source authority tiers:

1. theory/papers/standards/analytical reference,
2. official vendor documentation,
3. open-source docs/source for architecture and independent test ideas,
4. forums only for troubleshooting clues.

# 9. Capability + Release Gate

A capability advances only through:

```text
RESEARCHED
→ CONTRACT DEFINED
→ CODE EXISTS
→ TEST EXISTS
→ TEST PASSED
→ FEATURE WORKS
→ USER VALIDATED
```

`FEATURE WORKS` applies only to the documented capability envelope. Verification-only consumers never become general product capability by implication.
