# Dynamics26 — V1.0.2 Foundation / V1.1 Planning

Modern Fortran tabanlı, macOS / Apple Silicon odaklı açık kaynak nonlinear FEM/CAE platformu.

> **Mevcut durum:** V1.0.2 solver/CAD/meshing/pre-post foundation ve macOS standalone deployment hardening tamamlandı. **V1.1.0-alpha.3.1.1 viewport navigation corrective pass kuruldu**: alpha.3.1 kamera/navigation mimarisi, VS Code Debug GUI akışı, incelenebilir macOS input classification ve capability-tabanlı orientation widget kontrolü. Ayrıntı: [`GUI_ARCHITECTURE.md`](GUI_ARCHITECTURE.md), [`GUI_REDESIGN_REPORT.md`](GUI_REDESIGN_REPORT.md) ve [`docs/development/VSCODE_SETUP.md`](docs/development/VSCODE_SETUP.md).

> **Bağımsız geliştirme ilkesi:** Dynamics26, Code_Aster veya başka bir CAE yazılımının kaynak kodunu kopyalamaz ya da port etmez. Dış projeler yalnız özellik, kullanıcı akışı, mühendislik davranışı ve doğrulama karşılaştırması için referans olabilir. Fizik, matematik, formulasyon, sayısal algoritma ve verification zinciri bağımsız olarak geliştirilir.

## Ana geliştirme yaklaşımı

```text
Research
→ Physics / Mathematics
→ Mathematical Formulation
→ Numerical Algorithm
→ Dynamics26 Architecture
→ Implementation
→ Verification / Benchmark
→ Regression
→ macOS CI
→ Version Close
```

Temel mimari kural:

```text
CAD Geometry != Display Tessellation != FEM Mesh
```

Ana geliştirme dalı `main`'dir. Gereksiz feature/fix branch kullanılmaz; sürüm geçmişi version/tag/release ile yönetilir.

## V1.0 mühendislik temeli

V0.1–V0.13 arasında geliştirilen zincir V1.0.x serisinde verification ve release engineering açısından sertleştirildi:

```text
CAD / Geometry
      ↓
Geometry-aware FEM Mesh
      ↓
Material / Section / BC / Load / Contact
      ↓
Fortran FEM Core
      ↓
Linear / Modal / Nonlinear / Mixed u-p / Contact
      ↓
Results / Probe / Export / Qt-VTK Post
```

Ana solver özellikleri:

- 1D/2D/3D FEM foundation, sparse global assembly ve reactions,
- TRUSS2, 2B Euler–Bernoulli beam, QUAD4 plane stress/strain/axisymmetric, HEX8,
- modal analysis ve generalized eigenproblem backend sınırı,
- finite strain / Total Lagrangian HEX8,
- full ve modified Newton, line search, adaptive load stepping, rollback,
- Neo-Hookean, Mooney-Rivlin, Yeoh ve 1–3 terimli Ogden,
- J2 small-strain material-point plasticity baseline,
- mixed displacement-pressure HEX8/Q1-P0 nearly-incompressible baseline,
- rigid-master contact, augmented Lagrangian ve Coulomb friction baseline,
- STEP/OCCT geometry source integration, DXF custom section core,
- structured HEX8 / external Abaqus C3D8 meshing baseline,
- geometry provenance, assignments, probe, CSV ve VTK export.

## Aktif ve gelecek yol haritası

| Sürüm | Ana hedef |
|---|---|
| **V1.1.0** | GUI / UI / UX + Dynamics26 Identity |
| **V1.2.0** | General CAD & Geometry Platform |
| **V1.3.0** | General FEM Meshing |
| **V1.4.0** | Advanced Element Library |
| **V1.5.0** | Physics & Mathematics Verification Audit |
| **V1.6.0** | Code Quality & Reliability Audit |
| **V1.7.0** | Inter-module Communication & Performance |
| **V1.8.0** | Advanced Nonlinear Mechanics |
| **V1.9.0** | Advanced Contact |
| **V1.10.0** | Advanced Meshing & Adaptivity |
| **V1.11.0** | Large-Scale Solver & Performance |
| **V1.12.0** | Advanced Postprocessing |
| **V1.13.0** | Dynamics |
| **V2.0.0** | Integrated CAE Qualification |

Bu başlıklar ana ürün sütunlarıdır. Ayrıntılı plan için:

- `VERSION_ROADMAP.md`
- `docs/planning/DYNAMICS26_LONG_TERM_PLAN.md`
- `docs/planning/V1.1_GUI_UX_PLAN.md`

## V1.1 GUI hedefi

V1.1 yeni solver fiziğinden önce mevcut çekirdeğin profesyonel CAE uygulamasına dönüştürülmesine odaklanır.

**Alpha.2 durumu (uygulandı):** Doküman komut sistemi (QUndoStack + 18 domain
command) · Undo/Redo · dirty/clean doküman durumu · bağımlılık motoru
(Out-of-Date) · Suppress/Unsuppress · Rename/Duplicate/Delete/Cut/Copy/Paste ·
nesne türüne duyarlı bağlam menüleri · Preflight · Clear Generated Mesh · Clear
Solution · tam nesne kalıcılığı (ObjectId/ordering/suppression round-trip) ·
Dark Mode düzeltmesi · Qt 6.5+ uyumluluğu · standart macOS menü ve kısayolları.

**Alpha.1 durumu (uygulandı):** Model Tree (gerçek `ObjectId`/`ObjectType` nesne
grafiği) · baskın 3B grafik alanı · bağlamsal Details paneli · tek kayıt
defterinden yönetilen komut yüzeyi · mühendislik durum çubuğu · başlangıçta
kapalı alt yardımcı alan · macOS System Appearance ile Light/Dark. Eski
`MainWindow` → `Dynamics26Shell` → `CaeWorkbenchController` → `Alpha1UxController`
→ `Alpha1ProductPolish` → `AppearanceController` düzeltme zinciri kaldırıldı
(~4 760 satır). Bkz. [`GUI_REDESIGN_REPORT.md`](GUI_REDESIGN_REPORT.md).

Research kapsamında ANSYS Mechanical, Hexagon Marc/Mentat, Simufact, Abaqus/CAE, COMSOL, Altair HyperMesh/HyperView, gerektiğinde Simcenter ve Apple macOS Human Interface Guidelines modül bazında incelenecektir.

Hedef application shell:

```text
┌────────────────────────────────────────────────────────────┐
│ Dynamics26       Context Toolbar / Search                  │
├──────────────┬─────────────────────────────┬───────────────┤
│ Project      │                             │ Inspector     │
│ Navigator    │        3D Viewport          │               │
│              │                             │ Properties    │
│ Geometry     │                             │ Scope         │
│ Materials    │                             │ Definition    │
│ Connections  │                             │ Advanced      │
│ Mesh         │                             │               │
│ Analyses     │                             │               │
│ Results      │                             │               │
├──────────────┴─────────────────────────────┴───────────────┤
│ Diagnostics / Solve / Convergence / Messages        ▲     │
└────────────────────────────────────────────────────────────┘
```

V1.1 kullanıcıya görünen isimlerin **Dynamics26** olarak güncellenmesi için planlanan migration sürümüdür. `femcae_*`, `libfemcae`, `FEMCAE_*` gibi internal/public API, CMake veya compatibility isimleri ABI/API etkisi denetlenmeden topluca değiştirilmez.

## V1.0 hardening notları

### Disk checkpoint / restart

Nonlinear checkpoint yalnız RAM içinde değildir. `fem_checkpoint_io`:

- sürümlü schema,
- IEEE real64 bit-exact hexadecimal serialization,
- checksum,
- truncated/corrupted file rejection,
- read failure durumunda partial state temizliği

sağlar.

Contact history mevcut V1.0 checkpoint schema'sında henüz serialize edilmez; contact + checkpoint restart bu baseline'da açıkça reddedilir.

### Mesh convergence

`VER-V1000-001` bağımsız Euler–Bernoulli cantilever referansına karşı structured HEX8 convergence çalıştırır:

```text
4×1×1    error = 33.6192 %
8×2×2    error = 10.8368 %
12×3×3   error =  3.93645 %
```

Hata monoton azalmalı ve fine mesh hatası %5 altında kalmalıdır.

### Corrupted input / public API failure handling

V1.0 testleri en az şu hataları explicit olarak reddeder:

- duplicate Abaqus node,
- undefined C3D8 connectivity node,
- malformed C3D8,
- açık/truncated DXF contour,
- unsupported `LWPOLYLINE` bulge,
- invalid Young modulus,
- duplicate C-API node ID,
- missing connectivity node,
- invalid constraint component,
- truncated veya checksum-invalid nonlinear checkpoint.

### Memory-safety gate

V1.0 release testleri ayrı GCC AddressSanitizer + UndefinedBehaviorSanitizer build'inde de çalıştırılmıştır. Bu gate native macOS Instruments/Leaks yerine geçmez; Apple-side memory audit ayrıca ileri code-quality/release çalışmalarındadır.

## V1.0 portable validation kaydı

V1.0.0 validation snapshot'ı:

```text
Debug   : 123/123 PASS
Release : 123/123 PASS
```

V1.0.2 repository/reproducibility hardening snapshot'ında portable Debug/Release **124/124 PASS** kaydı bulunur.

V1.0.2 döneminde GitHub-hosted Apple Silicon CI üzerinde standalone `.app`, ad-hoc code-sign, strict Mach-O dependency audit, bundle smoke ve artifact upload da başarıyla doğrulanmıştır. Bu, Developer ID ile production notarization kanıtı değildir.

## Performance smoke

`perf_v1000_001` donanımlar arası performans iddiası değildir. Aynı CI/host üzerinde regression alarmı üretmek için 12×3×3 = 108 HEX8, 208 node ve yaklaşık 605 aktif DOF'lu lineer problemi çözer.

Large-scale solver performansı V1.11.0'da ayrı research/benchmark programı olarak ele alınacaktır.

## Portable build

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DFEMCAE_BUILD_TESTING=ON \
  -DFEMCAE_BUILD_GUI=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## macOS arm64 GUI/CAD build

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
cmake --install build-macos --prefix stage
```

> `FEMCAE_*` build option adları şimdilik compatibility/internal interface olarak korunmaktadır. V1.1 identity audit'i sonrasında API/ABI etkisine göre migration kararı verilecektir.

`.app` deployment kaynakları Qt deployment + CMake `BundleUtilities::fixup_bundle()` kullanır. `scripts/macos/audit_bundle.sh` Mach-O architecture, absolute Homebrew/build-path dependencies ve code-sign verification kontrolleri yapar.

Developer ID/notarization için `scripts/macos/sign_and_notarize.sh` altyapısı bulunur; gerçek Apple Developer kimliği ve notary profile gerektirir.

## Kurulu C++ meshing API örneği

```cpp
#include <femcae/meshing/StructuredHexMesher.h>

using namespace femcae::meshing;
StructuredHexMesher mesher;
BoxBoundaryGeometry geometry{100, 101, 102, 103, 104, 105, 106};
StructuredHexMesherOptions options;
options.nx = options.ny = options.nz = 1;

auto mesh = mesher.meshBox({{0,0,0}, {1,1,1}}, geometry, 1, options);
auto quality = evaluateHexMeshQuality(mesh);
```

Bu `femcae` namespace/include path'leri şimdilik compatibility API'sidir; kullanıcıya görünen ürün adı Dynamics26'dır.

## Lisans

Dynamics26 proje kaynakları mevcut repository lisans politikası kapsamında Apache License 2.0 altında sunulur. Üçüncü taraf kütüphaneler kendi lisansları altındadır; `THIRD_PARTY_LICENSES.md` ve `THIRD_PARTY_NOTICES.md` dosyalarına bakın. Gerçek binary dağıtımında kullanılan upstream lisans metinleri app bundle'a dahil edilmelidir.

## Mevcut baseline'da production-ready denmeyen ana alanlar

Bu alanlar artık doğrudan V1.2–V1.13 roadmap sütunlarına bağlanmıştır:

- arbitrary curved CAD için general geometry/modeling workflow,
- general 1D/2D/3D FEM meshing,
- higher-order ve gelişmiş element ailesi,
- finite-strain/global plasticity ve ileri nonlinear çözüm kontrolü,
- deformable-deformable/mortar/self-contact,
- adaptive refinement/remeshing/state transfer,
- large-scale sparse solver/eigensolver optimizasyonu,
- advanced result interpolation/comparison/postprocess,
- tam dynamics analysis family,
- production Developer ID signed/notarized distribution qualification.

## Temel belgeler

- `VERSION_ROADMAP.md`
- `docs/planning/DYNAMICS26_LONG_TERM_PLAN.md`
- `docs/planning/V1.1_GUI_UX_PLAN.md`
- `docs/architecture/MASTER_ROADMAP.md`
- `docs/architecture/V1.0.0_ARCHITECTURE.md`
- `docs/development/V1.0.0_AUDIT.md`
- `docs/development/BUILD_VALIDATION.md`
- `docs/development/PERFORMANCE_BASELINE.md`
- `docs/development/V1.0_RELEASE_CHECKLIST.md`
- `docs/verification/V1.0_VERIFICATION_MATRIX.md`
- `RELEASE_NOTES_V1.0.0.md`
- `SECURITY.md`

## CI yönü

V1.1 itibarıyla CI üç katmanlı ele alınacaktır:

1. **Fast main CI:** core build/tests + GUI compile + small GUI smoke.
2. **Self-hosted MacBook engineering CI:** trusted/manual incremental GUI, dependency reuse, profiling ve uzun testler.
3. **Clean GitHub-hosted release CI:** clean arm64 build, full tests, deploy, strict bundle audit, codesign, bundle smoke ve artifact.

`gui-build` yavaşlığı step-bazlı süre ölçümü yapıldıktan sonra optimize edilecektir; strict bundle audit yalnız hız veya CI geçişi için gevşetilmeyecektir.
