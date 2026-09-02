# Dynamics26

macOS / Apple Silicon odaklı, açık kaynak, modern Fortran tabanlı nonlinear FEM/CAE platformu.

> **Güncel engineering core:** `V1.0.2`  
> **Güncel GUI milestone:** `V1.1.0-beta.2`  
> **Aktif öncelik:** `V1.1.0-beta.3 — Minimum Usable Nonlinear Analysis Workflow`

Dynamics26'ın kısa vadeli hedefi artık ayrı ayrı özellik biriktirmek değil, kullanıcının gerçek bir modeli baştan sona hazırlayıp çözebildiği **kullanılabilir nonlinear analiz dikeyini** mümkün olan en kısa sürede tamamlamaktır.

## Ana kullanıcı hedefi

İlk büyük ürün eşiği şu workflow'un kesintisiz çalışmasıdır:

```text
New Project
→ STEP / Geometry
→ Body / Face Selection
→ Material Assignment
→ Mesh
→ Static Structural Analysis
→ Nonlinear / Large Deformation Setup
→ Fixed Support
→ Force / Pressure
→ Preflight
→ Solve
→ Newton / Convergence Monitor
→ Displacement / Stress / Reaction Results
```

Bu akış tamamlanmadan yeni ve geniş solver özellikleri varmış gibi gösterilmez.

## Güncel geliştirme stratejisi

### Faz 1 — Hızlı nonlinear setup / solve / results

Öncelik GUI'nin mühendislik açısından kullanılabilir hale gelmesidir:

- Body / Face / Edge / Vertex ve FEM selection araçlarının tamamlanması,
- transient selection ile persistent scope ayrımının korunması,
- seçili yüzeyden doğrudan Fixed Support / Force / Pressure oluşturma,
- Named Selection ile yeniden kullanılabilir scope,
- seçili yüzeyde doğru ve okunabilir load/support glyph gösterimi,
- Material assignment,
- temel Mesh generate + quality görünümü,
- Static Structural analysis setup,
- nonlinear / large deformation intent,
- gerçek general nonlinear product solve consumer,
- typed convergence telemetry,
- temel post-processing.

### Faz 2 — Mesh / Material / Solver fiziği ve matematiği

Kullanılabilir dikey kurulduktan sonra mevcut baseline sistematik biçimde sertleştirilecektir:

- mesh formulation ve quality,
- material data architecture,
- material model registry,
- constitutive equations ve consistent tangent,
- residual / tangent assembly,
- Full ve Modified Newton-Raphson,
- line search,
- automatic load stepping / cutback,
- convergence norms,
- rollback ve restart,
- bağımsız verification / benchmark.

### Faz 3 — Extensible CAE architecture

Programın büyümeden önce extension sınırları tanımlanacaktır:

- versioned extension manifest,
- capability registry,
- UI/workflow extensions,
- material-model extensions,
- solver backend extensions,
- importer/exporter extensions,
- result/postprocessor extensions,
- stable C/C++ API boundary,
- Fortran core'a doğrudan bağımlılığı engelleyen adapter katmanı,
- ileride MFront benzeri constitutive-law entegrasyonuna uygun architecture.

### Faz 4 — Rubber / Elastomer mechanics

Hedef ANSYS, Marc ve COMSOL seviyesindeki temel elastomer analiz altyapısına yaklaşmaktır:

- Neo-Hookean,
- Mooney-Rivlin,
- Yeoh,
- Ogden,
- compressible / nearly incompressible / incompressible davranış,
- mixed `u-p`, Herrmann, selective/F-bar benzeri formulation araştırması,
- large strain,
- robust contact,
- hyperelastic parameter fitting,
- uniaxial / biaxial / planar / volumetric test correlation,
- viscoelasticity,
- Mullins / cyclic softening research,
- temperature / frequency dependence,
- rubber component benchmark suite.

## Research-first kuralı

Yeni bir önemli özellik doğrudan kodlanmaz. Önce:

```text
Engineering Need
→ ANSYS / Marc / COMSOL Research
→ Code_Aster / Open-Source Architecture Review
→ Physics & Mathematics
→ Dynamics26 Adopt / Adapt / Reject Decision
→ Implementation
→ Verification
→ Regression
→ macOS CI
```

Code_Aster yalnız teknik, mimari ve verification referansıdır. Kaynak kodu kopyalanmaz, port edilmez veya lisans yükümlülüğünü kaldırmak için yeniden yazılmaz.

İlk araştırma özeti:

- `docs/research/NONLINEAR_CAE_REFERENCE_STUDY_2026-09.md`

## Force / Pressure yüzey davranışı

Load görüntüsü ile gerçek FEM yük dağılımı birbirine karıştırılmaz.

- **Total Force:** seçili yüzey kümesine ait toplam resultant kuvvettir.
- İlk baseline'da uniform reference-area traction olarak dağıtılabilir: `t = F / A_ref`.
- **Pressure:** yüzey normaline göre traction üretir.
- VTK okları yalnız görsel glyph'tir; solver yükü ok sayısına göre nodal force bölerek oluşturulmaz.
- Gerçek FEM load vector, seçili element face üzerinde shape function / quadrature ile consistent olarak entegre edilmelidir.
- Large-deformation follower/deformed-area yükleri ayrı doğrulama ile etkinleştirilir.

Bu ayrım ANSYS ve COMSOL'daki geometry-scoped load davranışına benzer bir kullanıcı semantiği sağlar; implementasyon Dynamics26'a özgüdür.

## Capability doğruluk kuralı

```text
Verification demo != General product capability
Typed telemetry   != Solver capability
UI control        != Solver consumer
```

Unsupported alan UI'de `Unavailable` görünmelidir. Eksik metrik sahte `0` ile doldurulmaz.

Validation sınıflandırması:

```text
CODE EXISTS
TEST EXISTS
TEST PASSED
FEATURE WORKS
USER VALIDATED
```

`USER VALIDATED` yalnız gerçek Mac üzerinde fiziksel kullanıcı doğrulamasından sonra verilir.

## Mimari

```text
Modern Fortran Solver Core
        ↓
Stable / Additive C ABI
        ↓
C++20 Application + Geometry / Meshing
        ↓
Qt 6 GUI
        ↓
VTK Viewport + Results
        ↓
OCCT CAD Geometry
```

Değişmez kural:

```text
CAD Geometry != Display Tessellation != FEM Mesh
```

## Aktif belgeler

- `VERSION_ROADMAP.md` — güncel sürüm sırası
- `docs/planning/DYNAMICS26_LONG_TERM_PLAN.md` — ana stratejik plan
- `docs/planning/V1.1_GUI_UX_PLAN.md` — hızlı nonlinear workflow planı
- `docs/planning/V1.1_RELEASE_SEQUENCE.md` — V1.1 milestone sırası
- `docs/research/NONLINEAR_CAE_REFERENCE_STUDY_2026-09.md` — ANSYS / Marc / COMSOL / Code_Aster araştırması

## Build — macOS arm64

```bash
brew install cmake ninja gcc arpack qt vtk opencascade

prefix_path="$(brew --prefix qt);$(brew --prefix vtk);$(brew --prefix opencascade)"
FC=gfortran cmake -S . -B build-macos -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_PREFIX_PATH="$prefix_path" \
  -DFEMCAE_BUILD_TESTING=ON \
  -DFEMCAE_BUILD_GUI=ON \
  -DFEMCAE_GUI_WITH_VTK=ON \
  -DFEMCAE_ENABLE_OCCT_GEOMETRY=ON \
  -DFEMCAE_REQUIRE_OCCT=ON

cmake --build build-macos --parallel
ctest --test-dir build-macos --output-on-failure
```

`FEMCAE_*`, `libfemcae` ve benzeri internal/compatibility isimleri ABI/API etkisi denetlenmeden topluca değiştirilmez.

## Lisans

Dynamics26 repository kaynakları Apache License 2.0 kapsamında geliştirilir. Üçüncü taraf bağımlılıklar kendi lisansları altındadır. Kaynak veya algoritma referansı kullanılırken lisans ve attribution sınırı ayrıca doğrulanır.
