# Dynamics26 — Aktif Sürüm Yol Haritası

**Plan revizyonu:** 2026-09  
**Platform:** macOS / Apple Silicon (`arm64`)  
**Ana dal:** `main`  
**Engineering core baseline:** `V1.0.2`  
**GUI baseline:** `V1.1.0-beta.2`

Bu belge Beta.2 sonrasında Dynamics26'ın aktif ürün önceliklerini yeniden sıralar. V0.x–V1.0.2 ve Alpha/Beta tarihçesi Git history, CHANGELOG ve milestone belgelerinde korunur.

## 1. Yeni ana hedef

En yakın hedef, kullanıcının nonlinear analiz için gerekli setup'ı yapabildiği, çözümü çalıştırabildiği ve sonucu inceleyebildiği minimum ama gerçek bir CAE workflow'udur.

```text
Geometry
→ Selection / Scope
→ Material
→ Mesh
→ Analysis
→ Supports / Loads
→ Preflight
→ Nonlinear Solve
→ Convergence
→ Results
```

Bu dikey tamamlandıktan sonra mesh, materials, constitutive models ve nonlinear solver fiziği/matematiği derinleştirilir.

## 2. V1.1.0 kalan sıra — usable nonlinear workflow

| Milestone | Hedef | Release koşulu |
|---|---|---|
| **V1.1.0-beta.2** | Solver Workspace / typed telemetry / diagnostics | **Tamamlandı — automated closeout; USER VALIDATED ayrı** |
| **V1.1.0-beta.3** | **Minimum Usable Nonlinear Analysis Workflow** | Setup → solve → results dikeyi gerçek product consumer ile çalışmalı |
| **V1.1.0-rc.1** | Workflow hardening / macOS UX / persistence / error recovery | native Light/Dark + keyboard/mouse + full regression |
| **V1.1.0** | İlk kullanılabilir nonlinear CAE application baseline | USER workflow dokümante, unsupported capability dürüst, release gates green |

### Beta.3 work packages

#### B3.1 — Selection / Scope Productivity

- Geometry Body / Face / Edge / Vertex selection,
- FEM Node / Element / Facet selection,
- current selection → Named Selection,
- scope edit/apply/cancel,
- hide/show/isolate minimum workflow,
- stale geometry/mesh scope diagnostics,
- scope-aware contextual toolbar.

#### B3.2 — Supports / Loads

- seçili yüzeyden `Fixed Support` oluşturma,
- seçili yüzeyden `Force` oluşturma,
- `Pressure` oluşturma,
- Geometry Selection / Named Selection scoping,
- global/local coordinate system hazırlığı,
- Total Force ile traction/pressure ayrımı,
- selected-face highlight,
- support glyph,
- viewport load arrows.

Load glyph sayısı solver nodal force sayısı değildir. Gerçek yük, element-face quadrature ile consistent load vector olarak oluşturulur.

#### B3.3 — Material / Mesh Minimum Setup

- body → material assignment,
- minimum material card validation,
- Linear Elastic production path,
- mevcut hyperelastic model kartlarının capability durumunu açık gösterme,
- mesh generate,
- global sizing,
- element/node count,
- minimum Jacobian/quality diagnostics,
- stale mesh lifecycle.

#### B3.4 — General Nonlinear Product Solve Consumer

Mevcut verification solver ile product model consumer birbirine karıştırılmaz.

Beta.3'te desteklenen model subset'i açıkça tanımlanarak:

- nonlinear analysis intent,
- geometric nonlinearity / large deformation,
- Full / Modified Newton,
- maximum iterations,
- adaptive stepping,
- initial/min/max increment,
- line search,
- residual/displacement tolerances,
- product model → C ABI → Fortran nonlinear solver bridge,
- rollback/cutback failure handling,
- typed telemetry

bağlanır.

`Nonlinear` seçilmiş bir analysis sessizce `DirectLinear` çözücüye düşemez. Consumer uygun değilse Solve `Unavailable` / Preflight error vermelidir.

#### B3.5 — Results MVP

Minimum sonuç nesneleri:

- Total Deformation,
- Directional Deformation,
- von Mises Stress,
- principal stress hazırlığı,
- reaction force,
- deformed + undeformed overlay,
- deformation scale,
- min/max,
- point/face probe,
- converged load step/substep seçimi,
- result scope.

#### B3.6 — Product Acceptance

- save/reopen,
- Undo/Redo state sınırları,
- derived solver/result state sınırları,
- Light/Dark,
- keyboard reachability,
- mouse/trackpad selection,
- failure recovery,
- representative nonlinear benchmark model.

## 3. V1.2.0 — Nonlinear Engineering Core Hardening

V1.1 kullanılabilir dikeyi kurar; V1.2 fiziği ve numeriği sertleştirir.

### Mesh

- geometry-aware volume meshing roadmap,
- TET/HEX strategy research,
- Jacobian determinant,
- aspect ratio,
- skew/distortion,
- element orientation,
- integration-point quality,
- face provenance,
- nonlinear distortion monitoring,
- mesh convergence framework.

### Materials / Material Models

- typed material-property schema,
- SI unit authority,
- model registry,
- parameter validation,
- model-specific required test data,
- constitutive state/history ownership,
- material-point verification harness,
- consistent tangent finite-difference check.

Öncelikli modeller:

1. Linear Elastic,
2. Neo-Hookean,
3. Mooney-Rivlin 2 parameter,
4. Yeoh,
5. Ogden 1–3 term.

### Nonlinear solver / Newton-Raphson

Ana denklem:

```text
R(u, λ) = F_ext(u, λ) - F_int(u) = 0
```

Newton increment'i consistent işaret convention ile:

```text
K_T Δu = R
u_(i+1) = u_i + α Δu
```

Sertleştirilecek alanlar:

- tangent matrix derivation,
- geometric tangent,
- material tangent,
- Full Newton,
- Modified Newton,
- line search,
- automatic increment growth/cutback,
- force residual,
- displacement increment norm,
- energy norm research,
- convergence scaling,
- divergence/stagnation detection,
- rollback,
- checkpoint/restart,
- deterministic telemetry.

Her özellik önce independent theory + benchmark ile doğrulanır.

## 4. V1.3.0 — Extension / Plugin Architecture & SDK

Amaç monolitik büyümeyi engellemektir.

Extension sınıfları:

- `WorkflowExtension`,
- `MaterialModelExtension`,
- `SolverBackendExtension`,
- `MeshExtension`,
- `GeometryImporterExtension`,
- `ResultExtension`,
- `ExporterExtension`.

Temel kurallar:

- versioned manifest,
- API/ABI compatibility number,
- capability declaration,
- explicit lifecycle/ownership,
- no direct GUI → Fortran internals access,
- no plugin → mutable document internals bypass,
- document mutations canonical commands üzerinden,
- solver/material extension'ları stable adapter/C ABI üzerinden,
- extension failure ana uygulamayı mümkün olduğunca izole etmelidir.

İlham kaynakları: ANSYS ACT extension modeli, COMSOL add-in/method modeli, Marc user subroutines ve Code_Aster MFront/UMAT coupling. Bunların kaynak kodu kopyalanmaz.

## 5. V1.4.0 — Rubber Mechanics Foundation

Ana hedef kauçuk/parça analizidir.

### Constitutive

- Neo-Hookean,
- Mooney-Rivlin,
- Yeoh,
- Ogden,
- volumetric response,
- nearly incompressible/incompressible options,
- parameter fitting.

### Element/formulation research

Aşağıdakiler karşılaştırmalı araştırılır; sonuç kanıta göre seçilir:

- mixed `u-p`,
- Herrmann formulation,
- selective/B-bar,
- F-bar,
- reduced integration + stabilization.

Tek bir formulation önceden dogma olarak seçilmez.

### Test-data workflow

- uniaxial tension,
- planar/pure shear,
- biaxial tension,
- volumetric/compression data,
- least-squares fitting,
- weighting/scaling,
- stability checks,
- extrapolation warning,
- test vs material-point overlay.

## 6. V1.5.0 — Advanced Elastomer / Contact

- deformable ↔ deformable contact,
- finite sliding,
- robust friction,
- contact pressure/opening/slip results,
- viscoelasticity,
- Prony-series research,
- Mullins effect / cyclic softening,
- temperature dependence,
- frequency dependence,
- preload + nonlinear response,
- rubber component benchmark library.

## 7. V1.6+ — Advanced CAE growth

Sonraki sürümler ihtiyaca ve verification maturity'ye göre:

- advanced meshing/adaptivity,
- shells/higher-order elements,
- plasticity/damage,
- large-scale sparse solving,
- advanced postprocessing,
- dynamics/harmonic/transient,
- production SDK ecosystem,
- full CAE qualification

başlıklarına genişler.

## 8. Research Gate — zorunlu

Her önemli work package için koddan önce kısa bir research record hazırlanır:

```text
Problem / User Need
Reference Products
ANSYS observation
Marc observation
COMSOL observation
Code_Aster/open-source observation
Physics / equations
License/source boundary
Dynamics26 Adopt / Adapt / Reject
Verification plan
```

Araştırmasız büyük solver/material/mesh özelliği implementation'a alınmaz.

## 9. Capability gate

Bir feature yalnız aşağıdaki zincirle ilerler:

```text
CODE EXISTS
→ TEST EXISTS
→ TEST PASSED
→ FEATURE WORKS
→ USER VALIDATED
```

Verification-only consumer, general product capability olarak pazarlanmaz.
