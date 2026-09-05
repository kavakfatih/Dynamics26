# Dynamics26

macOS / Apple Silicon odaklı, açık kaynak, modern Fortran tabanlı nonlinear FEM/CAE platformu.

> **Engineering core:** `V1.0.2`  
> **GUI baseline:** `V1.1.0-beta.3`  
> **Aktif ürün hedefi:** `V1.1.0-rc.1 — Nonlinear Workflow Hardening, Stability & Diagnostics`

Dynamics26'ın kısa vadeli hedefi özellik sayısını artırmak değil; kullanıcının **desteklenen bir model kapsamı içinde** geometriyi hazırlayıp sınır şartı/yük tanımlayabildiği, nonlinear çözümü çalıştırabildiği, Newton yakınsamasını izleyebildiği ve sonuçları inceleyebildiği ilk gerçek product workflow'u tamamlamaktır.

## Aktif workflow

```text
New Project
→ STEP / Geometry
→ Body / Face Selection
→ Material Assignment
→ Mesh
→ Nonlinear Static Analysis
→ Fixed Support
→ Force / Pressure
→ Preflight / Capability Check
→ Solve
→ Newton / Convergence
→ Results
```

### V1.1 Beta.3 capability envelope

Beta.3 iki şeyi özellikle ayırır:

- **Setup-ready:** UI ve persistent engineering objects geometri/material/mesh/BC/load/analysis tanımını taşıyabilir.
- **Solve-ready:** mevcut solver consumer'ın gerçekten desteklediği element + material + load + formulation kombinasyonu.

Bugünkü repository gerçeğinde product meshing yolu `StructuredHexMesher` tabanlıdır. Keyfi STEP gövdesi için genel volume mesher henüz yoktur. Beta.3 nonlinear product solve baseline'ı **parametric box veya box-compatible CAD + structured HEX8 + doğrulanmış material/formulation subset'i** ile kapatılmıştır. Keyfi STEP modeli sahte bounding-box mesh ile çözülmüş gibi gösterilmez.

## Geliştirme sırası

### V1.1 — Kullanılabilir nonlinear vertical slice

Öncelik:

- hızlı selection/scoping,
- selected Face → Fixed Support / Force authoring,
- Pressure authoring ve açık capability durumu,
- material assignment,
- mevcut structured HEX8 mesh ve quality/preflight,
- consistent surface-load vector,
- general nonlinear product C ABI bridge,
- Full / Modified Newton controls,
- adaptive stepping / cutback / line search,
- typed convergence telemetry,
- displacement / stress / reaction Results MVP,
- save/reopen + Undo/Redo + macOS UX acceptance.

### V1.2 — Geometry-aware meshing + scalable nonlinear foundation

- original Dynamics26 meshing engine M2+ qualification,
- arbitrary B-Rep/STEP volume meshing,
- CAD Face → mesh Facet provenance,
- local sizing / curvature / quality,
- TET/HEX formulation qualification,
- solver matrix-property metadata,
- dense reference → suitable sparse backend routing,
- nonlinear element distortion and mesh-convergence framework.

Production meshing strategy artık **özgün Dynamics26 meshing engine** olarak dondurulmuştur. M1 robust-geometry foundation QUALIFIED, M2 ise M2.0 reference-architecture/experiment-plan adımıyla AUTHORIZED durumdadır. Gmsh/Netgen/TetGen/CGAL/MMG benzeri projeler yalnız clean-room teori, mimari, failure-mode ve benchmark araştırma kaynaklarıdır; production mesher dependency'si değildir.

### V1.3 — Extension / Plugin Architecture & SDK

Tam SDK V1.3'te stabilize edilir; ancak extension-ready internal contracts daha erken kurulur:

- `CapabilityRegistry`,
- versioned solver input/output DTO,
- material-model descriptors,
- element/formulation descriptors,
- solver-backend descriptors,
- result descriptors,
- canonical command boundary.

Plugin hiçbir zaman document internals veya Fortran derived-type ABI'sini doğrudan kullanmaz.

### V1.4 — Rubber / Elastomer Mechanics Foundation

Rubber capability yalnız hyperelastic formül listesi değildir. Gerekli zincir:

```text
Test Data
→ Parameter Fitting
→ Constitutive Model
→ Compressibility Choice
→ Element / Formulation
→ Large-Strain Kinematics
→ Nonlinear Solver
→ Contact
→ Verification
→ Component Test Correlation
```

Öncelik Neo-Hookean → Mooney-Rivlin → Yeoh → Ogden; ardından nearly-incompressible/incompressible formulation qualification'dır. Mixed `u-p`, Herrmann, selective/B-bar, F-bar ve reduced-integration/stabilization yaklaşımları benchmark ile karşılaştırılır; tek yöntem önceden dogma olarak seçilmez.

## Force / Pressure fizik kontratı

Viewport glyph sayısı solver yükü değildir.

`Total Force` için başlangıç reference-configuration semantiği:

```text
A_ref = toplam seçili reference yüzey alanı
t_ref = F_total / A_ref
f_e   = ∫Γe Nᵀ t_ref dΓ
```

- çoklu facet katkıları global load vector'a assemble edilir,
- resultant ve moment korunumu test edilir,
- Pressure ayrı bir load type'tır,
- follower/deformed-area pressure yalnız verified backend capability olduğunda açılır.

## Nonlinear solver yönü

Fortran core'da gerçek `solve_nonlinear_static()` altyapısı, Full/Modified Newton, adaptive stepping, cutback, line search, rollback/checkpoint ve convergence history mevcuttur. Beta.3'ün ana solver işi yeni bir demo Newton algoritması yazmak değil; **immutable real-model input snapshot → additive/versioned C ABI → mevcut nonlinear core → typed ResultSet** zincirini kurmaktır.

`Nonlinear` analysis hiçbir koşulda sessizce `DirectLinear` solve'a düşemez.

## Research-first kuralı

Büyük bir feature için sıra:

```text
Engineering Need
→ Theory / Independent References
→ ANSYS / Marc / COMSOL Official Documentation
→ Code_Aster / FEBio / Relevant Open-Source Architecture Review
→ Licensing Boundary
→ Dynamics26 Adopt / Adapt / Reject
→ Capability Contract
→ Implementation
→ Verification
→ Regression
→ macOS CI
→ USER VALIDATED
```

Kaynak hiyerarşisi:

1. continuum/FEM theory, papers, standards ve analytical benchmarks,
2. resmi vendor documentation,
3. open-source docs/source — architecture ve verification pattern için,
4. forum/community — yalnız troubleshooting, physics authority değil.

Code_Aster veya başka bir projeden source code kopyalanmaz/transliterate edilmez.

## Capability doğruluk kuralı

```text
Verification demo != General product capability
UI control        != Solver consumer
Telemetry         != Physics support
Mesh exists       != Nonlinear-ready mesh
```

Unsupported combination UI'de `Unavailable` veya blocking Preflight olarak görünür.

Validation zinciri:

```text
CODE EXISTS
→ TEST EXISTS
→ TEST PASSED
→ FEATURE WORKS
→ USER VALIDATED
```

`USER VALIDATED` yalnız gerçek Mac üzerinde fiziksel kullanıcı doğrulamasından sonra verilir.

## Mimari

```text
Modern Fortran Solver Core
        ↓
Stable / Additive / Versioned C ABI
        ↓
C++20 Application + Solver Input Builders
        ↓
Geometry / Meshing / Capability Services
        ↓
Qt 6 GUI + VTK Results
        ↓
OCCT CAD Geometry
```

Değişmez kural:

```text
CAD Geometry != Display Tessellation != FEM Mesh
```

## Aktif belgeler

- `VERSION_ROADMAP.md` — aktif kritik yol ve sürüm gate'leri
- `docs/planning/DYNAMICS26_LONG_TERM_PLAN.md` — ürün/solver/mimari ana planı
- `docs/planning/V1.1_GUI_UX_PLAN.md` — Beta.3 vertical-slice çalışma planı
- `docs/research/NONLINEAR_CAE_REFERENCE_STUDY_2026-09.md` — ilk ANSYS/Marc/COMSOL/Code_Aster kıyası
- `docs/research/DYNAMICS26_DEEP_ROADMAP_RESEARCH_2026-09.md` — derin roadmap araştırması ve teknik karar kayıtları

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

`FEMCAE_*`, `libfemcae` ve diğer internal/compatibility isimleri ABI/API etkisi denetlenmeden topluca değiştirilmez.

## Lisans

Dynamics26 repository kaynakları Apache License 2.0 kapsamında geliştirilir. Üçüncü taraf bağımlılıklar kendi lisansları altındadır; dependency adoption öncesinde source/distribution/linking koşulları ayrıca incelenir.
