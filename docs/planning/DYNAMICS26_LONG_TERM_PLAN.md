# Dynamics26 — Ana Geliştirme Planı

**Plan revizyonu:** 2026-09  
**Durum:** Aktif source-of-truth ürün planı  
**Platform:** macOS / Apple Silicon (`arm64`)  
**Ana dal:** `main`  
**Engineering core:** Modern Fortran  
**Application:** C++20 / Qt 6 / VTK / OCCT  
**Current GUI milestone:** `V1.1.0-beta.2`

Bu plan Beta.2 sonrasında öncelikleri yeniden sıralar. Amaç önce gerçek bir nonlinear analiz workflow'unu kullanılabilir hale getirmek, sonra mesh/material/solver matematiğini derinleştirmek ve ardından elastomer/rubber mechanics altyapısını büyütmektir.

# 1. Değişmez ilkeler

1. Dynamics26 özgün bir nonlinear FEM/CAE platformudur.
2. Code_Aster, ANSYS, Marc, COMSOL ve diğer CAE ürünleri araştırma/benchmark referansıdır; kaynak kod kopyalanmaz.
3. `CAD Geometry != Display Tessellation != FEM Mesh` ayrımı korunur.
4. Document state, derived state ve transient selection ayrımı korunur.
5. Persistent engineering mutation canonical command + Undo/Redo yolundan geçer.
6. Solver telemetry ve results derived state'tir; gereksiz document history oluşturmaz.
7. Unsupported capability `Unavailable` gösterilir; sahte `0` kullanılmaz.
8. C ABI geriye dönük uyumlu tutulur; yeni solver/telemetry ihtiyacı additive API ile çözülür.
9. Fortran core, fizik/matematik açıklamalarını Türkçe ve izlenebilir biçimde taşır.
10. macOS / Apple Silicon tek ürün platformudur.
11. Ana branch yalnız `main`'dir.
12. Büyük feature implementation'dan önce research gate zorunludur.

# 2. Yeni ürün stratejisi — Vertical Slice First

Eski stratejide CAD, meshing, element, verification ve nonlinear gelişim büyük ölçüde ayrı sürüm sütunlarıydı. Yeni strateji kullanıcının tamamlayabileceği dikey workflow'u öne alır.

İlk hedef:

```text
STEP Geometry
→ Select Body / Face
→ Assign Material
→ Generate Mesh
→ Create Static Structural Analysis
→ Enable Nonlinear / Large Deformation
→ Select Fixed Face
→ Select Loaded Face
→ Apply Force / Pressure
→ Preflight
→ Solve
→ Monitor Newton Convergence
→ Inspect Displacement / Stress / Reactions
```

Bu akış gerçek product consumer ile çalışmadan Dynamics26 “nonlinear analysis ready” sayılmaz.

# 3. Research Gate

Her büyük work package için implementation öncesinde `docs/research/` altında bir araştırma notu tutulur.

Minimum şablon:

```text
1. User / Engineering Problem
2. ANSYS Mechanical
3. Hexagon Marc / Mentat
4. COMSOL Multiphysics
5. Code_Aster / relevant open-source references
6. Physics / Mathematics
7. Licensing / source boundary
8. Dynamics26 Adopt / Adapt / Reject decisions
9. Architecture impact
10. Verification / benchmark plan
```

Araştırma yalnız UI screenshot kıyaslaması değildir. Solver feature için denklem/formülasyon ve benchmark, UI feature için engineering semantics ve interaction modeli incelenir.

İlk kayıt:

`docs/research/NONLINEAR_CAE_REFERENCE_STUDY_2026-09.md`

# 4. Phase A — Minimum Usable Nonlinear Analysis Workflow

Bu faz V1.1.0-beta.3'ün ana kapsamıdır.

## A1. Project / Navigator workflow

Project tree minimum modeli:

```text
Model
├─ Geometry
├─ Materials
├─ Connections
├─ Mesh
└─ Analyses
   └─ Static Structural
      ├─ Analysis Settings
      ├─ Fixed Support
      ├─ Force / Pressure
      └─ Solution
         ├─ Total Deformation
         ├─ Equivalent Stress
         └─ Reaction Force
```

Amaç ANSYS benzeri okunabilir engineering object hiyerarşisi ile COMSOL benzeri physics-aware settings yaklaşımını birleştirmektir; görsel kopyalama yapılmaz.

## A2. Selection / scoping

Tamamlanacak davranışlar:

- Body / Face / Edge / Vertex,
- FEM Node / Element / Facet,
- visible-only rectangle selection,
- current selection count / primary entity,
- context-aware selection filter,
- Named Selection create/edit,
- Geometry Selection ↔ Named Selection scope method,
- stale scope detection,
- hide/show/isolate minimum productivity,
- Esc / Shift / Command interaction,
- viewport / Navigator / Inspector synchronization.

### Load/BC creation workflow

Hedef kullanıcı davranışı:

```text
Select face(s)
→ Insert Fixed Support / Force / Pressure
→ current selection persistent scope'a aktarılır
→ object Inspector açılır
→ scope ve definition aynı yerde görünür
```

Kullanıcı isterse önce object oluşturup sonra `Apply Selection` ile scope atayabilir.

## A3. Fixed Support

Minimum production semantics:

- face scope,
- selected face persistent reference,
- UX'de fixed support glyph,
- scope validity,
- mesh regeneration sonrası re-resolution/stale behavior,
- solve consumer'a doğru constrained DOF seti.

İleri constraint family daha sonra gelir:

- displacement,
- remote displacement,
- symmetry,
- cylindrical support,
- elastic support.

## A4. Force / Pressure semantics

ANSYS ve COMSOL'da total force seçili geometry üzerine dağıtılabilir; COMSOL ayrıca reference/deformed area ve pressure semantiğini açık ayırır. Dynamics26'ta da UI ve solver semantiği ayrık olmalıdır.

### Force

`Total Force` başlangıç semantiği:

```text
F_total = prescribed resultant vector
A_ref   = total selected reference surface area
t_ref   = F_total / A_ref
```

Uniform traction baseline yalnız desteklenen geometry/load türlerinde kullanılacaktır.

### Pressure

```text
traction = -p n
```

İşaret convention açıkça dokümante edilmelidir. Large-deformation follower pressure için current normal/current area kullanımına geçiş ayrı formulation/verification work package'ıdır.

### Consistent face load vector

Gerçek FEM yükü viewport'taki arrow glyph sayısına bağlı değildir.

Her element face için:

```text
f_e = ∫_Γ N^T t dΓ
```

uygun surface quadrature ile hesaplanır. Çoklu yüzey scope'ta element-face katkıları global load vector'a assemble edilir.

### Arrow visualization

VTK glyph sistemi:

- yalnız kullanıcı feedback'idir,
- selected geometry üzerinde area-aware sample noktaları üretir,
- yüzey normali / user vector yönünü gösterir,
- arrow density zoom ve alanla ölçeklenebilir,
- magnitude renk/uzunluk semantiği tutarlı olur,
- arrow count fiziksel yük bölme sayısı değildir,
- curved surface üzerinde local normals gerekirse ayrı pressure glyph modu kullanır.

## A5. Material assignment minimum

GUI'nin ilk hedefi her constitutive modeli aynı anda production ilan etmek değildir.

Minimum:

- Material object,
- Material Assignment,
- Body scope,
- density,
- Linear Elastic `E`, `ν`,
- unit validation,
- duplicate/missing material diagnostics.

Mevcut hyperelastic backend modelleri material cards'ta capability-aware gösterilebilir. General product consumer doğrulanmadıysa `Unavailable for current analysis consumer` açıkça yazılır.

## A6. Mesh minimum

- Generate Mesh,
- global element size,
- node/element count,
- selected geometry provenance,
- minimum Jacobian quality,
- invalid/inverted element rejection,
- stale state,
- clear/regenerate,
- boundary facet provenance for loads/supports.

“Mesh generated” tek başına nonlinear-ready değildir. Preflight kalite ve formulation uyumluluğunu denetler.

## A7. Analysis Settings

Basic görünüm:

- Analysis Type: Static Structural,
- Linear / Nonlinear intent,
- Large Deformation,
- Number of load steps or Automatic,
- End load factor.

Advanced görünüm:

- Newton method,
- maximum iterations,
- adaptive stepping,
- initial/min/max increment,
- line search,
- residual relative tolerance,
- displacement relative tolerance.

Backend tarafından tüketilmeyen property enabled görünemez.

## A8. Product nonlinear solve bridge

Şu anki DirectLinear general solve ile verification-only nonlinear solver birbirinden ayrıdır.

Beta.3 hedefi mevcut nonlinear core'un doğrulanmış subset'ini gerçek model consumer'a bağlamaktır.

Required chain:

```text
Document Analysis State
→ Preflight
→ Immutable Solver Input Snapshot
→ C++/C ABI Adapter
→ Fortran Nonlinear Solver
→ Typed Session Telemetry
→ Result Dataset
```

Kurallar:

- nonlinear intent → DirectLinear fallback yasak,
- unsupported element/material/contact → solve blocked,
- telemetry derived state,
- document mutation solver thread'den yapılmaz,
- failure reason typed olmalı,
- rollback/cutback state ana modele kısmi yazılmamalı.

## A9. Convergence UX

Normal kullanıcıya:

- current load factor,
- current increment,
- iteration,
- convergence state,
- residual trend,
- warning/failure reason

gösterilir.

Advanced:

- absolute/relative residual,
- displacement increment,
- line-search alpha,
- cutback provenance,
- minimum J,
- contact/mixed diagnostics destekleniyorsa.

## A10. Results MVP

İlk production post-processing:

- Total Deformation,
- directional displacement,
- Equivalent (von Mises) Stress,
- reaction forces,
- deformed shape,
- undeformed overlay,
- deformation scale,
- min/max,
- probe,
- result step/substep selector,
- result entity scope.

Raw integration-point result ile averaged/nodal derived result etiketleri karıştırılmaz.

# 5. Phase B — Mesh, Materials, Material Models, Solver Physics & Mathematics

V1.2.0'ın ana konusu budur.

## B1. Mesh engineering

Araştırma ve geliştirme:

- TET4/TET10 vs HEX8/higher-order stratejisi,
- geometry curvature,
- local sizing,
- transition elements,
- Jacobian quality,
- distortion,
- aspect ratio,
- skewness,
- reduced/full/selective integration compatibility,
- boundary integration orientation,
- mesh convergence automation,
- nonlinear element distortion monitor.

Her mesh quality metric'in solver anlamı dokümante edilir; sadece renkli kalite barı eklenmez.

## B2. Material architecture

Material data üç seviyeye ayrılır:

```text
Material Identity
→ Physical Properties
→ Constitutive Models
```

Her constitutive model:

- input schema,
- units,
- parameter constraints,
- state variables,
- stress update,
- tangent,
- supported kinematics,
- supported element/formulation,
- required test data,
- verification cases

taşır.

## B3. Hyperelastic equations

Öncelik:

1. Neo-Hookean,
2. Mooney-Rivlin 2P,
3. Yeoh,
4. Ogden 1–3 term.

Her model için:

```text
W(F or C)
→ stress measure
→ consistent material tangent
→ volumetric/isochoric split
→ parameter sanity
→ material-point test
→ single-element test
→ component benchmark
```

## B4. Newton-Raphson certification

Temel denge:

```text
R(u, λ) = F_ext(u, λ) - F_int(u) = 0
```

Iterasyon:

```text
K_T(u_i, λ_i) Δu_i = R_i
u_(i+1) = u_i + α_i Δu_i
```

Sertifikasyon kapsamı:

- residual sign/unit conventions,
- tangent consistency,
- finite-difference tangent check,
- Full Newton,
- Modified Newton,
- tangent reuse,
- line search,
- automatic stepping,
- cutback,
- convergence tolerances,
- singular/negative-J handling,
- iteration cap,
- rollback,
- reproducibility.

ANSYS, Marc, COMSOL ve Code_Aster davranışları yalnız reference behavior olarak karşılaştırılır; denklemler bağımsız FEM/continuum mechanics kaynaklarıyla türetilir.

# 6. Phase C — Extension / Plugin Architecture

Bu fazın architecture skeleton'ı V1.2 sırasında başlatılır, SDK V1.3.0'da stabilize edilir.

## C1. Extension principles

- plugin host = C++ application layer,
- Fortran internals plugin ABI değildir,
- versioned manifest,
- semantic capability IDs,
- explicit dependencies,
- load/unload lifecycle,
- errors isolated and reportable,
- document changes command bus üzerinden,
- solver extension immutable input/output DTO kullanır,
- UI extension arbitrary global patch yapamaz.

## C2. Extension types

```text
UI / Workflow
Geometry Importer
Mesh Generator
Material Model
Solver Backend
Result Evaluator
Exporter / Report
```

## C3. Material extension direction

Marc user subroutine modeli ve Code_Aster MFront/UMAT coupling yaklaşımı, “constitutive model core solver'dan ayrılabilir mi?” sorusu açısından referanstır.

Dynamics26 hedefi:

```text
Material Model Plugin
→ stable constitutive interface
→ stress + tangent + state update
→ core element formulation
```

İleride MFront adapter araştırılabilir; lisans/distribution/API uygunluğu ayrıca denetlenir.

# 7. Phase D — Rubber / Elastomer Mechanics

V1.4.0 ve sonrası.

## D1. Nearly incompressible formulation research

Kauçukta volumetric locking ana risklerden biridir. Aşağıdaki yöntemler akademik benchmark ile karşılaştırılır:

- mixed `u-p`,
- Herrmann,
- selective/B-bar,
- F-bar,
- reduced integration + stabilization.

Karar kriterleri:

- locking,
- pressure oscillation,
- distortion sensitivity,
- contact compatibility,
- consistent tangent complexity,
- performance,
- 2D/axisymmetric/3D genişleyebilirlik.

## D2. Parameter fitting

Test families:

- uniaxial tension,
- planar/pure shear,
- biaxial tension,
- volumetric/compression.

Fit sisteminde:

- engineering → true measures dönüşümü açık,
- least-squares objective,
- data weighting,
- parameter bounds,
- stability checks,
- fit quality,
- extrapolation warning,
- model comparison

olmalıdır.

## D3. Rubber product analyses

Representative benchmark components:

- simple rubber block compression,
- bonded rubber shear,
- torsion annulus,
- engine mount stiffness,
- crank pulley rubber ring torsion,
- contact-heavy elastomer case.

Son hedef yalnız solver benchmark değil, fiziksel test korelasyonudur.

# 8. Phase E — Advanced Rubber / Contact / Time Dependence

- deformable-deformable finite sliding,
- friction,
- viscoelasticity,
- Prony series,
- Mullins effect,
- temperature dependence,
- frequency-dependent modulus research,
- preload/history,
- cyclic analysis,
- fatigue/damage araştırması.

# 9. Verification hierarchy

Her solver/material özelliği şu piramitten geçer:

```text
Material Point
→ Single Element
→ Patch / Limiting Case
→ Analytical Benchmark
→ Published Benchmark
→ Cross-Code Comparison
→ Component Test Correlation
```

Cross-code comparison tek başına doğruluk kanıtı değildir; ANSYS/Marc/COMSOL sonuçları independent reference ile birlikte kullanılır.

# 10. CI / release gates

Her source milestone:

- Debug core regression,
- Release core regression,
- C ABI consumer smoke,
- GUI application acceptance,
- native arm64 architecture,
- selection/scope regression,
- Light/Dark audit,
- feature-specific benchmark

geçmeden kapatılmaz.

`USER VALIDATED` yalnız kullanıcının fiziksel Mac doğrulamasıdır.
