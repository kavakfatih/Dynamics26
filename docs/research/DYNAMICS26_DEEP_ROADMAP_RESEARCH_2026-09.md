# Dynamics26 Deep Roadmap Research — 2026-09

**Amaç:** Dynamics26 ürün planını nonlinear setup, meshing, solver mathematics, extension architecture ve rubber mechanics açısından yeniden doğrulamak.  
**Kaynak sınırı:** ticari/open-source ürünlerden source code veya UI kopyalanmaz. Vendor docs ürün semantics'i; open-source docs/source architecture/verification pattern'i; bağımsız FEM/continuum mechanics kaynakları fizik/matematik authority'si olarak kullanılır.

# 1. Executive conclusions

1. **V1.1'in doğru hedefi geniş feature coverage değil, capability-bounded nonlinear vertical slice'tır.**
2. Mevcut `MeshService` keyfi STEP volume meshing yapmadığı için arbitrary-part solve V1.1'e yazılmamalıdır.
3. ANSYS/COMSOL benzeri hızlı face-scoped authoring korunmalı; solver load vector geometry/mesh provenance üzerinden consistent integration ile üretilmelidir.
4. Mevcut Fortran nonlinear core yeni Newton demo yazmayı gerektirmiyor; kritik eksik general-model immutable snapshot + product C ABI bridge'dir.
5. Mixed `u-p`, Contact ve rubber yalnız material-card problemi değildir; element formulation + matrix properties + linear backend + tangent contracts birlikte ele alınmalıdır.
6. Full plugin SDK'yı V1.3'e ertelemek mümkündür, fakat typed capability/descriptor/DTO contracts V1.1/V1.2'de başlamalıdır.
7. Keyfi automotive CAD workflow için mature volume-meshing adapter araştırmak, genel 3D mesheri sıfırdan yazmaktan daha doğru kısa/orta vadeli yoldur. Netgen güçlü adaydır ancak adoption kararı değildir.
8. Rubber roadmap'in Definition of Done'u cross-code benzerliği değil, material-point → element → benchmark → physical correlation zinciridir.

# 2. Current Dynamics26 source reality

Repository audit sonucunda planı belirleyen gerçekler:

## 2.1 Meshing

Current `MeshService`:

- `SimulationMesh` ownership taşır,
- `StructuredHexMesher` kullanır,
- parametric axis-aligned box mesher baseline'dır,
- STEP/CAD yalnız `boxDescriptor` ile gerçek box olarak çözümlenebiliyorsa bounding dimensions + gerçek CAD Face provenance devralınır,
- non-box CAD için sahte mesh üretilmez,
- boundary facets geometry IDs taşır,
- scaled-Jacobian quality hesaplanır.

**Roadmap implication:** arbitrary STEP nonlinear solve V1.1 capability değildir.

## 2.2 Nonlinear solver

Core'da:

- Full Newton,
- Modified Newton,
- adaptive load stepping,
- cutback/growth,
- line search,
- residual/displacement criteria,
- rollback/checkpoint,
- typed convergence history,
- mixed/contact diagnostic fields

bulunur.

General GUI model solve ise hâlen ayrı DirectLinear consumer kullanır.

**Roadmap implication:** Beta.3'ün kritik solver işi nonlinear algorithm invention değil, product model bridge ve acceptance'tır.

## 2.3 Linear solver backends

Core registry'de:

- dense reference,
- sparse CG,
- Apple Accelerate sparse direct adapter

bulunur.

Product C ABI paths bazı yerlerde dense reference'a sabitlenmiştir.

**Roadmap implication:** matrix-property / backend compatibility explicit hale gelmeden mixed/contact scale-up yapılmamalıdır.

# 3. ANSYS Mechanical findings

ANSYS Mechanical Force semantics Dynamics26 için özellikle yararlıdır:

- Force vertex/edge/face/node/element-face scope alabilir,
- Face Force selected face total area üzerinden uniform traction'a dönüştürülür,
- geometry değişse bile total force magnitude semantics korunur,
- Scoping Method = Geometry Selection veya Named Selection.

Nonlinear solver tarafında:

- Full Newton,
- Modified Newton,
- Initial stiffness,
- Unsymmetric full Newton,
- line search,
- automatic stepping/cutback,
- convergence controls

ayrı numerical concepts'tir.

ANSYS nonlinear theory/guides ayrıca line search'in force-driven/oscillatory convergence durumlarında yararlı olabileceğini gösterir.

### Dynamics26 decisions

**ADAPT**

- direct geometry-scoped load/support UX,
- Named Selection reusable scope,
- Basic/Advanced nonlinear controls,
- total force as one resultant,
- numerical settings separate from material settings.

**REJECT**

- product-specific command/UI identity copying,
- every solver option'u ilk sürümde expose etmek.

# 4. COMSOL findings

COMSOL Boundary Load modelinde aşağıdaki ayrım önemlidir:

- Force per reference area,
- Force per deformed area,
- Total force,
- Pressure,
- Resultant.

Total force selected boundaries area'ya bölünür; coordinate system explicit'tir.

Mixed formulation documentation:

- near incompressibility displacement-only formulation'da locking/stress sorunları oluşturabilir,
- pressure mixed formulation auxiliary pressure DOF ekler,
- resulting matrix indefinite olabilir ve bazı iterative solvers bununla iyi çalışmaz.

Hyperelastic UI:

- compressible,
- nearly incompressible,
- incompressible

modes ayırır ve mixed formulation/material volumetric response ilişkisini açık tutar.

Physics Builder architecture:

- supported dimensions,
- domain/boundary features,
- dependent variables,
- reusable building blocks,
- Study/Solver Defaults,
- Mesh Defaults,
- Result Defaults,
- preview/testing

tek physics interface descriptor'ında birlikte ele alınır.

### Dynamics26 decisions

**ADAPT**

- explicit load configuration semantics,
- capability-aware material compressibility,
- physics feature + solver/mesh/result default contract idea,
- extension descriptor before dynamic plugin host.

**IMPORTANT ARCHITECTURE LESSON**

Material/formulation capability, solver suitability ve result availability aynı compatibility system içinde sorgulanmalıdır.

# 5. Marc / Mentat findings

Marc'ın resmi product positioning'i özellikle:

- hyperelasticity,
- contact,
- geometric nonlinearities,
- robust nonlinear solve,
- physical material test data,
- nonlinear/contact convergence

alanlarını öne çıkarır. 2026.1 sürümü contact handling, fixed-time-stepping convergence ve Mentat meshing/postprocessing/material-test usability iyileştirmelerini vurgular.

### Dynamics26 decisions

Marc primary technical comparison reference for:

- rubber large-strain robustness,
- Contact,
- nearly incompressible formulation behavior,
- material-test driven workflow,
- nonlinear convergence.

Marc behavior tek başına formulation seçimi için proof değildir. Mixed/Herrmann/F-bar vb. karar bağımsız numerical benchmarks ile alınır.

# 6. Code_Aster findings

Code_Aster architecture aşağıdaki separation'ı güçlendirir:

```text
Model
Material
Load / BC
Nonlinear operator
Solver
Post-processing
```

Current source tree'de solver implementation ayrı `IterationSolver`, `Operators`, `StepSolvers`, `TimeIntegrators`, `LinearAlgebra` alanlarına bölünmüştür. Repository source/test separation ve büyük validation culture taşır.

MFront/MGIS finite-strain integration tartışması özellikle önemlidir: external constitutive interface'te deformation gradient, stress measure ve tangent operator convention açık değilse integration hatalarına çok açıktır.

### Dynamics26 decisions

**ADAPT**

- solver operator separation,
- validation documentation discipline,
- external material law interface'te kinematics/stress/tangent explicit contract.

**DO NOT COPY**

- commands/source implementation,
- Fortran/C++ algorithms,
- material-law implementation.

# 7. FEBio findings

FEBio, nonlinear finite-element solver olarak open-source ve macOS desteklidir. Özellikle biomechanics/material-rich domain'de solver + plugin ecosystem geliştirmiştir. Public repositories material plugin örnekleri ve plugin build pattern'leri sunar.

### Dynamics26 decisions

FEBio secondary open-source reference for:

- nonlinear application architecture,
- material plugin conformance,
- plugin build/test ergonomics,
- long-lived solver extensibility.

Dynamics26 solver algorithm roadmap'ı FEBio'nun yöntemlerini kopyalamaz; Full Newton baseline stabilize edilmeden quasi-Newton/BFGS öncelik olmaz.

# 8. Volume meshing strategy research

Current structured box mesher useful verification/product vertical slice baseline'dır fakat real automotive parts için yeterli değildir.

## Netgen candidate

Public Netgen project:

- automatic 3D tetrahedral meshing,
- geometry-kernel-based STEP/IGES path,
- mesh optimization,
- hierarchical refinement,
- library API,
- Unix/Windows/OSX,
- LGPL-2.1.

### Adoption gate

Before adding dependency:

1. **License:** linking/distribution/package review.
2. **Build:** Apple Silicon reproducibility, CMake integration, CI cost.
3. **Geometry:** OCCT/B-Rep direct transfer; display tessellation never solver mesh source.
4. **Provenance:** CAD Face persistent key → generated surface facets.
5. **Topology:** first/second-order tetra support and data extraction.
6. **Quality:** Jacobian/sliver/aspect metrics exposed or recomputed internally.
7. **Controls:** global/local size, curvature, narrow-region behavior.
8. **Determinism:** stable test behavior; raw mesh IDs need not be stable if provenance/remap is.
9. **Failure:** typed diagnostics for invalid CAD/mesh failure.
10. **Benchmark:** automotive/rubber geometry corpus.

### Decision

**RESEARCH CANDIDATE — NOT YET ADOPTED**

A full general 3D mesher should not be built from scratch before this evaluation.

# 9. Solver matrix-property research

A nonlinear iteration solves a linearized system. Therefore `solver backend` is part of physics compatibility.

Required matrix metadata:

```text
symmetry
positive-definiteness expectation
indefinite/saddle-point
unsymmetric terms
pressure DOF presence
contact/friction terms
```

Examples:

- basic displacement-only elastic tangent may be SPD-compatible under suitable conditions,
- mixed `u-p` commonly yields indefinite saddle-point structure,
- friction/contact or certain follower-load tangents may become unsymmetric,
- CG must never be generic fallback for indefinite/unsymmetric systems.

### Dynamics26 decision

Create backend suitability layer before rubber mixed formulation and general Contact product support.

# 10. Nonlinear algorithm roadmap

## Production baseline

1. Full Newton.
2. Modified Newton.
3. Automatic load increment.
4. Cutback/growth.
5. Line search.
6. Force/displacement convergence.
7. Rollback/checkpoint.

## Later

- energy convergence after scaling study,
- predictor,
- numerical stabilization,
- arc-length/Riks for limit-point/path-following,
- quasi-Newton/BFGS after independent research.

Arc-length is not a generic “better Newton” toggle; it solves a different path-following need and interacts with load control/line search strategy.

# 11. Product nonlinear C ABI research decision

Existing demo APIs are useful verification surfaces but should not become the general model ABI by adding ever more scalar arguments.

Recommended evolution:

```text
versioned C-interoperable model descriptor
versioned nonlinear options descriptor
versioned result descriptor
versioned history entry
```

Each descriptor defines:

- API version / struct size,
- pointer + count pairs,
- element/formulation IDs,
- material card IDs/parameters,
- load/constraint arrays,
- result capacities,
- ownership/lifetime,
- SI units,
- status codes.

Fortran adapter builds internal `model_t`; C++ never knows Fortran storage/layout.

# 12. Surface load research decision

Bad baseline:

```text
node_force = total_force / selected_node_count
```

Correct general direction:

```text
f_e = ∫ Nᵀ t dΓ
```

For Total Force uniform reference traction:

```text
A = Σ facet area
traction = F_total / A
```

Then each facet contribution is integrated and assembled.

Verification must check both resultant and moment, not only summed scalar magnitude.

Arrow/glyph rendering is independent presentation sampling.

# 13. Results architecture research decision

Nonlinear solver output is a sequence, not one flattened final number.

Separate:

```text
SolverHistory = all attempts/iterations/cutbacks
AcceptedResultSets = accepted physical states
ResultDefinition = user request
DerivedField = presentation/postprocessing
```

Result field metadata must include location and averaging policy.

Minimum field locations:

- Node,
- Element,
- IntegrationPoint,
- BoundaryFacet.

# 14. Extension architecture research decision

Do not begin V1.3 by loading arbitrary dylibs into application internals.

## Stage 1 — internal descriptors

During V1.1/V1.2:

- capability registry,
- material descriptors,
- formulation descriptors,
- solver backend descriptors,
- mesh provider descriptors,
- result descriptors,
- versioned DTOs.

## Stage 2 — trusted in-process extensions

V1.3:

- manifest,
- host API range,
- explicit capabilities/dependencies,
- canonical command mutation,
- lifecycle/error reporting,
- conformance tests.

## Material plugin special contract

Must declare:

```text
kinematics input
strain/deformation measure
stress measure
algorithmic tangent type/layout
state variable size
commit/revert semantics
thread-safety
units
supported dimensions/formulations
```

This is more important than plugin loading mechanism itself.

# 15. Rubber mechanics research decision

A robust rubber solver requires four independent but coupled qualifications:

1. constitutive law,
2. incompressibility formulation,
3. element/mesh behavior,
4. nonlinear/contact solver behavior.

## Constitutive order

- Neo-Hookean,
- Mooney-Rivlin 2P,
- Yeoh,
- Ogden 1–3 term.

## Data order

- uniaxial,
- planar/pure shear,
- biaxial,
- volumetric/compression.

## Incompressibility candidates

- mixed `u-p`,
- Herrmann,
- B-bar/selective,
- F-bar,
- reduced integration + stabilization.

No method becomes default until locking, pressure, distortion, tangent, contact and performance benchmarks are complete.

# 16. Verification pyramid

Every material/formulation/solver capability:

```text
Theory identity / analytical limit
→ material point
→ finite-difference tangent/Jacobian
→ single element
→ patch / locking / distortion
→ mesh refinement
→ published benchmark
→ cross-code comparison
→ product workflow acceptance
→ physical component correlation
```

Suggested rubber component ladder:

1. homogeneous rubber cube uniaxial.
2. bonded shear block.
3. confined compression.
4. rubber annulus torsion.
5. simple contact compression.
6. crank pulley rubber ring torsion.
7. engine mount stiffness.

# 17. Revised critical path

```text
B3.0 Capability/Preflight
        ↓
B3.1 Selection/Scope
        ↓
B3.2 BC/Load + consistent surface integration
        ↓
B3.3 Material/Mesh readiness
        ↓
B3.4 SolverInputSnapshot + nonlinear product C ABI
        ↓
B3.5 backend suitability baseline
        ↓
B3.6 convergence + ResultSet
        ↓
B3.7 full vertical-slice acceptance
        ↓
V1.1 RC / USER VALIDATED
        ↓
V1.2 volume meshing + scalable numerical foundation
        ↓
V1.3 plugin SDK
        ↓
V1.4 rubber mechanics
```

Parallel research tracks may study V1.2+ topics, but implementation should not destabilize the V1.1 critical path.

# 18. Research-backed references used for this revision

## ANSYS

- Mechanical 2026 R1 Force — face Force distribution and scoping.
- Newton-Raphson theory / NROPT — Full/Modified/Unsymmetric strategies.
- Structural Analysis Guide — nonlinear solve/line-search guidance.
- ACT Customization Guide — Mechanical extension architecture patterns.

## COMSOL 6.4

- Boundary Load — reference/deformed/total/pressure semantics.
- Mixed Formulation — near incompressibility and indefinite system warning.
- Hyperelastic Material — compressibility/mixed formulation.
- Physics Builder — feature definitions, supported dimensions, mesh/solver/result defaults, testing.

## Marc 2026.1

- official product page — nonlinear/contact/hyperelasticity and current convergence/material-test improvements.

## Code_Aster

- source repository and `Solvers` organization.
- DEFI_MATERIAU hyperelastic/nonlinear material categories.
- MFront/MGIS finite-strain integration design discussion.

## FEBio

- open-source nonlinear solver repository.
- plugin/example repositories and current developer-manual/plugin updates.

## Netgen

- official GitHub project — automatic 3D tetra meshing, STEP/IGES geometry-kernel path, optimization/refinement, macOS, LGPL-2.1.

# 19. Final Adopt / Adapt / Reject summary

| Topic | Decision |
|---|---|
| Object-tree + face-scoped engineering setup | **ADAPT** |
| Total Force as entire selected-scope resultant | **ADOPT SEMANTICS / OWN IMPLEMENTATION** |
| Consistent surface integration | **ADOPT FEM PRINCIPLE** |
| Full/Modified Newton + stepping + line search | **ADAPT UX / OWN CORE** |
| Generic arbitrary STEP solve in V1.1 | **REJECT CLAIM** |
| Netgen integration | **RESEARCH CANDIDATE** |
| Mixed `u-p` as immediate universal rubber default | **REJECT PREMATURE DECISION** |
| Capability registry before plugin SDK | **ADOPT ARCHITECTURE** |
| Direct Fortran ABI plugin access | **REJECT** |
| MFront-style explicit kinematics/stress/tangent contracts | **ADAPT** |
| Cross-code match as sole verification | **REJECT** |
