# Dynamics26 — Uzun Vadeli Geliştirme Planı

**Belge durumu:** Onaylı ana plan  
**Başlangıç tabanı:** V1.0.2  
**Platform:** macOS / Apple Silicon (`arm64`)  
**Ana geliştirme dalı:** `main`  
**Ana solver:** Modern Fortran  
**Uygulama katmanı:** C++20 / Qt 6 / VTK / OCCT  

Bu belge V1.0.2 sonrasında Dynamics26 için onaylanan ana ürün yol haritasını tanımlar. `docs/architecture/MASTER_ROADMAP.md` içindeki V0.x–V1.0 mimari temelini devam ettirir; geçmiş mimari kararları geçersiz kılmaz.

## 1. Değişmez proje ilkeleri

1. **Dynamics26 özgün bir FEM/CAE platformudur.** Code_Aster, ANSYS, Marc, Abaqus, Simufact, COMSOL, Altair ve benzeri sistemler yalnız özellik, kullanıcı akışı, mühendislik davranışı ve doğrulama açısından araştırma referansı olabilir. Kaynak kod kopyalanmaz veya uyarlanarak lisans bağı kaldırılmaya çalışılmaz.
2. **Research-first geliştirme uygulanır.** Yeni fizik veya sayısal yöntem önce fizik, continuum mechanics, FEM teorisi, akademik literatür ve benchmark problemleri üzerinden incelenir; sonra Dynamics26'a özgü formülasyon ve kod geliştirilir.
3. **CAD Geometry != Display Tessellation != FEM Mesh.** Bu üç veri modeli mimari olarak ayrı kalır.
4. **GUI solver implementasyon detaylarını doğrudan kullanıcıya dayatmaz.** Kullanıcı mühendislik niyetini tanımlar; element formülasyonu ve solver ayrıntıları gerektiğinde Advanced seviyede açılır.
5. **Modern Fortran solver çekirdeği korunur.** C/C ABI sınırı, C++20 application/geometry/meshing katmanı, Qt 6 GUI, VTK ve OCCT mimarisi korunur.
6. **macOS / Apple Silicon ana ve tek ürün platformudur.** Linux ürün hedefi değildir.
7. **Tek geliştirme dalı `main`'dir.** Sürüm geçmişi version/tag/release ile yönetilir; gereksiz feature/fix branch kullanılmaz.
8. **Her PASS kanıtlanmalıdır.** Derleme, test, benchmark veya CI sonucu çalıştırılmadan başarılı kabul edilmez.
9. **Fortran mühendislik ve matematik açıklamaları Türkçe ve ayrıntılı olmalıdır.** Denklem, varsayım, birim, işaret konvansiyonu ve fiziksel anlam mümkün olduğunda kaynak kodda açıklanır.

## 2. Standart geliştirme zinciri

```text
Research
   ↓
Physics / Mathematics
   ↓
Mathematical Formulation
   ↓
Numerical Algorithm
   ↓
Dynamics26 Architecture
   ↓
Implementation
   ↓
Unit / Verification Tests
   ↓
Independent Benchmark
   ↓
Regression Tests
   ↓
macOS CI
   ↓
Version Close / Tag / Release
```

Her ileri solver özelliğinde en az şu izlenebilirlik hedeflenir:

```text
Teori / denklem
      ↕
Bağımsız akademik kaynak
      ↕
Dynamics26 implementasyonu
      ↕
Verification testi
      ↕
Kabul toleransı / benchmark sonucu
```

## 3. Onaylı V1.1–V2.0 ana yol haritası

| Sürüm | Ana hedef | Ana çıktı |
|---|---|---|
| **V1.1.0** | GUI / UI / UX + Dynamics26 Identity | Profesyonel CAE application shell ve araştırmaya dayalı kullanıcı akışı |
| **V1.2.0** | General CAD & Geometry Platform | Genel CAD import/modelleme, healing, topology/provenance |
| **V1.3.0** | General FEM Meshing | 1D/2D/3D genel mesh altyapısı, quality ve geometry-aware controls |
| **V1.4.0** | Advanced Element Library | Beam/shell/solid/higher-order/mixed element ailesi ve sertifikasyon testleri |
| **V1.5.0** | Physics & Mathematics Verification Audit | Mevcut fiziğin ve matematiğin bağımsız yeniden denetimi |
| **V1.6.0** | Code Quality & Reliability Audit | Fortran/C/C++/ABI kalite, sanitizer, static-analysis ve error-path denetimi |
| **V1.7.0** | Inter-module Communication & Performance | Qt/C++/OCCT/VTK/C ABI/Fortran veri akışı ve performans iyileştirmesi |
| **V1.8.0** | Advanced Nonlinear Mechanics | İleri nonlinear kontrol, finite-strain constitutive ve robust convergence |
| **V1.9.0** | Advanced Contact | Deformable/deformable, surface contact, finite sliding ve gelişmiş friction |
| **V1.10.0** | Advanced Meshing & Adaptivity | Error estimator, adaptive refinement, remesh ve state transfer |
| **V1.11.0** | Large-Scale Solver & Performance | Büyük sparse sistemler, ileri backend ve ölçeklenebilir benchmark |
| **V1.12.0** | Advanced Postprocessing | Profesyonel field/history/result inceleme ve karşılaştırma |
| **V1.13.0** | Dynamics | Modal genişletme, harmonic, transient, spectrum, random ve nonlinear dynamics |
| **V2.0.0** | Integrated CAE Qualification | CAD→mesh→solve→results zincirinin production qualification sürümü |

Bu başlıklar ana ürün sütunlarıdır. Sürüm içi alt kapsam research sonuçlarına göre ayrıntılandırılabilir; ancak ana sütunların yol haritasından çıkarılması açık proje kararı gerektirir.

## 4. V1.1.0 — GUI / UI / UX + Dynamics26 Identity

V1.1 ilk aktif fazdır. Yeni solver fiziği eklemek yerine mevcut çekirdeğin gerçek bir mühendisin kullanabileceği CAE uygulamasına dönüştürülmesi hedeflenir.

Ana araştırma referansları:

- ANSYS Mechanical,
- Hexagon Marc / Mentat,
- Simufact,
- Abaqus/CAE,
- COMSOL,
- Altair HyperMesh / HyperView,
- gerektiğinde Siemens Simcenter,
- Apple macOS Human Interface Guidelines.

Araştırmanın amacı ekran kopyalamak değildir. Her gözlem `Adopt / Adapt / Reject` kararıyla Dynamics26 kullanıcı akışına çevrilir.

Hedef application shell:

```text
┌────────────────────────────────────────────────────────────┐
│ Dynamics26        Context Toolbar / Search                 │
├──────────────┬─────────────────────────────┬───────────────┤
│ PROJECT      │                             │ INSPECTOR     │
│ NAVIGATOR    │                             │               │
│              │        3D VIEWPORT          │ Properties    │
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

V1.1 aynı zamanda kullanıcıya görünen **FEMCAE → Dynamics26** geçiş sürümüdür. Uygulama adı, pencere başlığı, menüler, About, dokümantasyon başlıkları, bundle display name ve artifact/release adları gözden geçirilir. Mevcut `femcae_*`, `libfemcae`, `FEMCAE_*` gibi internal/public API veya CMake isimleri ABI/API etkisi incelenmeden topluca değiştirilmez.

Ayrıntılı V1.1 planı: `docs/planning/V1.1_GUI_UX_PLAN.md`.

## 5. V1.2.0 — General CAD & Geometry Platform

Hedef, mevcut OCCT STEP baseline'ını genel CAE geometri platformuna yükseltmektir.

Planlanan araştırma ve geliştirme başlıkları:

- STEP / IGES / BREP import,
- parts, bodies, faces, edges, vertices ve assemblies,
- primitives,
- extrude / revolve / sweep / loft araştırması,
- boolean işlemler,
- fillet / chamfer,
- transformations / patterns,
- shape healing / repair,
- defeaturing,
- partition / imprint,
- named geometry / persistent selection,
- CAD-to-mesh provenance,
- topological naming stratejisi.

Release gate: arbitrary CAD model, display tessellation ve FEM meshing giriş verisi ayrı sözleşmelerle doğrulanmalıdır.

## 6. V1.3.0 — General FEM Meshing

Mevcut structured HEX8 baseline genel mesh platformuna genişletilir.

Hedefler:

- 1D line mesh,
- TRIA / QUAD surface mesh,
- TET / HEX volume mesh altyapısı,
- wedge / pyramid için topology hazırlığı,
- global ve local sizing,
- curvature-aware sizing,
- boundary-layer meshing araştırması,
- mesh quality ölçümleri,
- geometry entity → FEM entity association,
- import/export altyapısı,
- deterministic IDs ve provenance.

V1.3 production-quality otomatik all-hex mesher iddiası taşımaz; ileri remeshing/adaptivity V1.10 kapsamındadır.

## 7. V1.4.0 — Advanced Element Library

Element framework aşağıdaki aileleri taşıyacak şekilde genişletilir:

- 0D: mass, spring, damper,
- 1D: truss, Euler-Bernoulli, Timoshenko beam,
- 2D: plane stress, plane strain, axisymmetric,
- structural surface: membrane, shell, layered shell,
- 3D: tetrahedron, hexahedron, wedge, pyramid,
- higher-order interpolation,
- reduced/selective integration,
- mixed displacement-pressure,
- assumed/enhanced strain,
- hourglass control,
- cohesive/connector araştırması.

Bir element yalnız implement edildiği için production kabul edilmez. Minimum certification zinciri:

```text
Patch Test
→ Analytical Benchmark
→ Distortion Test
→ Locking Test
→ Mesh Convergence
→ Nonlinear Benchmark (uygunsa)
```

## 8. V1.5.0 — Physics & Mathematics Verification Audit

Bu sürüm feature sürümü değil, doğrulama sürümüdür.

Denetim kapsamı:

- birimler ve boyut tutarlılığı,
- tensor ve Voigt convention,
- işaret convention,
- local/global coordinate transformations,
- shape functions ve partition of unity,
- Jacobian ve isoparametric mapping,
- Gauss quadrature,
- strain-displacement matrisleri,
- stress/strain measures,
- finite-strain kinematics (`F`, `J`, `C`, `b` vb.),
- objectivity,
- material ve geometric tangent,
- residual ve equilibrium,
- Newton / line search / step control,
- hyperelastic constitutive modeller,
- mixed `u-p`,
- contact/friction,
- mass/stiffness formulations,
- generalized eigenproblem,
- energy/work checks,
- limiting cases ve mesh convergence.

Her kritik formulasyon için teori-kod-test traceability kaydı oluşturulması hedeflenir.

## 9. V1.6.0 — Code Quality & Reliability Audit

Fortran odakları:

- `implicit none`, explicit interface ve kind tutarlılığı,
- bounds/runtime checks,
- uninitialized/undefined state,
- allocation/deallocation ve ownership,
- error propagation,
- global mutable state,
- module dependency graph,
- gereksiz data copy,
- compiler warning gate.

C/C++ odakları:

- Apple Clang warnings,
- clang-tidy / static analysis,
- RAII ve lifetime,
- sanitizer testleri,
- thread safety,
- malformed/corrupt input,
- C ABI ownership ve lifecycle.

Coverage tek başına fizik doğruluğu ölçütü sayılmaz; risk görünürlüğü amacıyla kullanılır.

## 10. V1.7.0 — Inter-module Communication & Performance

Ana veri yolu:

```text
Qt GUI
   ↓
C++ Application Model
   ├── OCCT Geometry
   ├── Meshing
   └── VTK Visualization
   ↓
Stable / Bulk C ABI
   ↓
Modern Fortran Solver
```

Araştırılacak ve ölçülecek konular:

- chatty API yerine bulk transfer,
- gereksiz array/data copy azaltma,
- açık ownership/lifetime,
- reusable buffers,
- sparse pattern ve factorization reuse,
- cache locality / memory layout,
- element batching,
- asynchronous solver worker,
- UI thread izolasyonu,
- result streaming,
- VTK topology reuse,
- profiler ve timing telemetry.

Bu sürüm V1.11 large-scale solver çalışmasının mimari ön hazırlığıdır.

## 11. V1.8.0 — Advanced Nonlinear Mechanics

Research başlıkları:

- robust full Newton,
- modified Newton,
- quasi-Newton,
- advanced line search,
- automatic increments ve cutback,
- displacement control,
- arc-length / Riks,
- follower loads,
- finite-strain plasticity,
- viscoelasticity,
- creep,
- damage foundation,
- consistent tangent verification,
- history/state management.

Her yeni model akademik formülasyon ve bağımsız benchmark ile sertifikalandırılır.

## 12. V1.9.0 — Advanced Contact

Mevcut rigid-master baseline'dan aşağıdaki seviyeye ilerleme hedeflenir:

- deformable ↔ deformable,
- surface-to-surface,
- finite sliding,
- self-contact,
- edge/contact özel durumları,
- penalty ve augmented Lagrangian geliştirmeleri,
- mortar ve diğer ileri enforcement araştırmaları,
- Coulomb/friction genişletmeleri,
- stick/slip robust active set,
- BVH veya benzeri broad-phase acceleration,
- curved master surfaces,
- contact pressure/postprocess,
- disk restart/history.

## 13. V1.10.0 — Advanced Meshing & Adaptivity

V1.3 mesh üretimidir; V1.10 çözüm odaklı adaptivity'dir.

```text
Solve
  ↓
Error Estimation
  ↓
Refinement / Remesh Decision
  ↓
Mesh Update
  ↓
Solution + State Transfer
  ↓
Continue Solve
```

Kapsam:

- error estimators,
- h-refinement,
- local adaptive refinement,
- mesh distortion monitoring,
- remeshing / rezoning,
- solution mapping,
- history/state-variable mapping,
- contact recreation,
- adaptivity convergence criteria.

## 14. V1.11.0 — Large-Scale Solver & Performance

Research adayları Apple Accelerate, ARPACK-NG, SLEPc, PETSc, MUMPS ve uygun diğer sparse backend'lerdir. Yeni dependency kararı lisans, macOS dağıtımı, Fortran/C sınırı ve benchmark sonuçları birlikte değerlendirilmeden verilmez.

Örnek benchmark kademeleri:

```text
10k DOF
100k DOF
500k DOF
1M DOF
5M DOF
```

Ölçümler:

- assembly time,
- factorization/preconditioner time,
- solve time,
- peak RAM,
- nonlinear/linear iteration count,
- eigen solve time,
- parallel efficiency,
- model/result transfer cost.

## 15. V1.12.0 — Advanced Postprocessing

Raw solver result ile kullanıcıya sunulan derived/averaged result ayrı tutulur.

Hedefler:

- nodal ve integration-point fields,
- extrapolation / averaging seçenekleri,
- stress/strain invariants ve principal values,
- deformed/undeformed overlay,
- vector glyphs,
- probe / path,
- section cut / clipping / iso-surface,
- XY/history plots,
- load-displacement, force-time, energy-time,
- contact pressure/opening/slip,
- case comparison,
- modal/transient animation,
- large-result lazy loading,
- export/report altyapısı.

GUI, kullanıcının `Integration Point`, `Nodal Extrapolated` veya `Nodal Averaged` sonuç gördüğünü açıkça göstermelidir.

## 16. V1.13.0 — Dynamics

Planlanan analiz ailesi:

- modal,
- prestressed modal,
- harmonic response,
- transient structural,
- direct time integration,
- modal superposition,
- Rayleigh/modal/structural damping,
- base excitation,
- response spectrum,
- random vibration / PSD,
- nonlinear dynamics.

İleri araştırma hattı:

- rotordynamics,
- gyroscopic effects,
- Campbell diagram,
- critical speeds,
- complex eigenvalues,
- imbalance response.

## 17. V2.0.0 — Integrated CAE Qualification

V2.0 yeni özellik sayısından çok bütün zincirin yeterliliğine odaklanır:

```text
CAD
→ Mesh
→ Materials / Sections / Connections
→ Analysis / Loads / BC
→ Solve
→ Results
→ Project Save / Restart / Reopen
```

Qualification kapsamı:

- independent benchmark suite,
- regression suite,
- performance qualification,
- project schema/backward compatibility,
- crash/error recovery,
- documentation/examples,
- native macOS distribution,
- Developer ID signing / notarization / stapling,
- third-party license inventory.

## 18. CI stratejisi

### 18.1 Fast main CI

Her değişiklikte hızlı feedback hedefi:

```text
Configure
→ Core Build
→ Core Tests
→ GUI Compile
→ Small GUI Smoke
```

### 18.2 Self-hosted MacBook runner

Self-hosted `macOS` / `ARM64` runner yalnız güvenilir proje kodunda ve kontrollü workflow ile kullanılır. Özellikle:

- GUI geliştirme,
- Qt/VTK/OCCT dependency cache,
- incremental build,
- profiling,
- uzun verification,
- developer-triggered engineering CI

için kullanılabilir. Untrusted PR kodu otomatik olarak kişisel runner üzerinde çalıştırılmaz.

### 18.3 Clean GitHub-hosted release CI

Release kanıtı temiz ortamda korunur:

```text
Clean macOS ARM64
→ Release Build
→ Full Tests
→ GUI
→ Install/Deploy
→ Bundle Fixup
→ Strict Mach-O Audit
→ Codesign
→ Bundle Smoke
→ Artifact
```

Strict bundle audit yalnız CI geçsin diye gevşetilmez.

### 18.4 GUI build performans çalışması

`gui-build` optimizasyonu tahminle değil ölçümle yapılır. En az şu adımların süreleri kaydedilir:

- Homebrew/dependency setup,
- configure,
- compile,
- CTest,
- deploy/macdeployqt,
- BundleUtilities/fixup,
- Mach-O audit/sign,
- artifact compression/upload.

Sonra caching, incremental build, ccache/sccache uygunluğu, target ayrıştırma ve packaging sıklığı ölçüm sonuçlarına göre değerlendirilir.

## 19. Sürüm kapatma kuralı

Bir sürüm ancak kapsamına uygun acceptance kriterleri ve test kanıtları tamamlandığında kapanır. Dokümantasyonda `PASS`, `verified`, `production-ready` gibi ifadeler gerçek kanıt olmadan kullanılmaz.
