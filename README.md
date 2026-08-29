# FEMCAE — V1.0.2

Modern Fortran tabanlı, macOS / Apple Silicon odaklı açık kaynak nonlinear FEM/CAE projesi.

> **Release durumu:** V1.0.2 portable/source verification tamamlandı. Bu patch repository bootstrap, reproducible source archive ve GitHub CI/release hygiene katmanını sertleştirir. Native signed/notarized `.app` sonucu yalnız gerçek macOS workflow PASS olduğunda kabul edilir.

> FEMCAE, Code_Aster kaynak kodunu kopyalamaz veya port etmez. Dış projeler yalnız mimari ve mühendislik davranışı açısından referans olabilir; kaynak kod, formülasyon ve doğrulama testleri bağımsız geliştirilir.

## V1.0 mühendislik kapsamı

V1.0 yeni solver özelliği eklemekten çok V0.1–V0.13 arasında geliştirilen zinciri sertleştirir:

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

## V1.0 hardening

### Disk checkpoint / restart

Nonlinear checkpoint artık yalnız RAM içinde değildir. `fem_checkpoint_io`:

- sürümlü schema,
- IEEE real64 bit-exact hexadecimal serialization,
- checksum,
- truncated/corrupted file rejection,
- read failure durumunda partial state temizliği

sağlar.

Contact history bu V1.0 checkpoint schema'sında henüz serialize edilmez; contact + checkpoint restart açıkça reddedilmeye devam eder.

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

V1.0 release testleri ayrı GCC AddressSanitizer + UndefinedBehaviorSanitizer build'inde de çalıştırılır. Portable hostta V1 release label seti **5/5 PASS** olmuştur. Bu gate native macOS Instruments/Leaks yerine geçmez; Apple-side memory audit ayrıca release checklist'indedir.

## Final portable validation

Aynı V1.0 source snapshot:

```text
Debug   : 123/123 PASS
Release : 123/123 PASS
```

Öne çıkan sayılar:

```text
Unit          : 67
Verification  : 39
Release       : 5
Linear        : 15
Nonlinear     : 25
Hyperelastic  : 11
Mixed u-p     : 7
Contact       : 9
Meshing       : 7
Error path    : 7
```

Installed-package gate'leri:

```text
Installed CLI                     PASS
Installed C API consumer          PASS
Installed Geometry/Meshing C++    PASS
macOS workflow YAML parse         PASS
macOS release shell syntax        PASS
Release source compiler warnings  0
```

## Performance smoke

`perf_v1000_001` bir performans iddiası veya donanımlar arası benchmark değildir. Aynı CI/host üzerinde regression alarmı üretmek için 12×3×3 = 108 HEX8, 208 node ve yaklaşık 605 aktif DOF'lu lineer problemi çözer.

Bu doğrulama hostundaki Release örnek koşusu yaklaşık **0.43 s** sürmüştür. Testte geniş bir üst süre sınırı kullanılır; farklı donanımlardaki mutlak süreler karşılaştırılmamalıdır.

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
FC=gfortran-15 cmake -S . -B build-macos -G Ninja \
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

## Lisans

FEMCAE kaynak kodu Apache License 2.0 altında sunulur. Üçüncü taraf kütüphaneler kendi lisansları altındadır; `THIRD_PARTY_LICENSES.md` ve `THIRD_PARTY_NOTICES.md` dosyalarına bakın. Gerçek binary dağıtımında kullanılan upstream lisans metinleri app bundle'a dahil edilmelidir.

## V1.0'da bilinçli olarak production-ready denmeyen alanlar

- arbitrary curved CAD için genel production tetra/hex volume mesher,
- adaptive error-estimator refinement,
- higher-order/stabilized geniş mixed-element ailesi,
- deformable-deformable/mortar/self-contact,
- finite-strain/global J2 plasticity,
- large-scale sparse shift-invert modal optimization,
- interpolated cut-surface contour,
- contact-history disk restart,
- native signed/notarized macOS artifact doğrulaması.

## Temel belgeler

- `VERSION_ROADMAP.md`
- `docs/architecture/V1.0.0_ARCHITECTURE.md`
- `docs/development/V1.0.0_AUDIT.md`
- `docs/development/BUILD_VALIDATION.md`
- `docs/development/PERFORMANCE_BASELINE.md`
- `docs/development/V1.0_RELEASE_CHECKLIST.md`
- `docs/verification/V1.0_VERIFICATION_MATRIX.md`
- `docs/verification/VER-V1000-001.md`
- `docs/verification/VER-V1000-002.md`
- `RELEASE_NOTES_V1.0.0.md`
- `SECURITY.md`

## V1.0.2 repository / reproducible release hardening

- `femcae_geometry` → `femcae_meshing` CMake target bağımlılığı gerçek target sırasıyla kurulur.
- Shared-library sürümü `PROJECT_VERSION` üzerinden gelir.
- `scripts/github/verify_repository_hygiene.py` build/binary/credential artifaktlarını engeller.
- `scripts/release/make_source_archive.py` byte-for-byte deterministik kaynak ZIP üretir.
- `scripts/github/bootstrap_repo.sh` ilk Git commit/origin/push akışını hazırlar.
- CTest geçici kaynak ağacında gerçek no-push Git bootstrap ve reproducible archive doğrulaması yapar.
- Portable Debug/Release: 124/124 PASS.
- Uzak FEMCAE GitHub reposu bu release sırasında bağlantıda görünmediği için remote CI çalıştırılmış sayılmaz.

## V1.0.1 release engineering

- `RELEASE_NOTES_V1.0.1.md`
- `docs/development/V1.0.1_AUDIT.md`
- `docs/development/V1.0.1_NATIVE_RELEASE_GATES.md`

