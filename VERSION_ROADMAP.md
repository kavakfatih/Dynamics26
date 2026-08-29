# Dynamics26 — Sürüm Yol Haritası ve Kullanılabilirlik Özeti

Bu belge Dynamics26 için sürüm bazlı ana çalışma planıdır. V0.x–V1.0.2 döneminin ayrıntılı teknik kayıtları `docs/development/*_AUDIT.md`, `CHANGELOG.md` ve mimari belgelerde korunur. V1.1.0 sonrasındaki onaylı uzun dönem planının ayrıntısı `docs/planning/DYNAMICS26_LONG_TERM_PLAN.md` içindedir.

## Proje geliştirme ilkesi

Dynamics26, Code_Aster veya başka bir CAE yazılımının kaynak kodunu kopyalayan/port eden bir proje değildir. Dış yazılımlar özellik, kullanıcı akışı, mühendislik davranışı ve sonuç karşılaştırması için araştırma referansı olabilir. Yeni fizik ve sayısal algoritmalar continuum mechanics, FEM teorisi, akademik literatür ve bağımsız benchmark'lardan türetilerek Dynamics26 mimarisinde özgün olarak geliştirilir.

Standart geliştirme zinciri:

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

Değişmez mimari kural:

```text
CAD Geometry != Display Tessellation != FEM Mesh
```

Ana geliştirme dalı yalnız `main`'dir; sürüm geçmişi version/tag/release ile yönetilir.

## Tamamlanan temel sürümler

| Sürüm | Ana hedef | Durum |
|---|---|---|
| **V0.1.1** | Numerical & Architectural Foundation Hardening | Tamamlandı — Modern Fortran/C ABI/test foundation |
| **V0.2.0** | Model / Mesh / Field / DOF / Numbering | Tamamlandı — gerçek FEM veri modeli temeli |
| **V0.3.0** | Element Kernel / Shape Functions / Quadrature | Tamamlandı — element-local geometri ve kinematik çekirdeği |
| **V0.4.0** | Sparse Assembly / macOS Linear Solver | Tamamlandı — global sparse assembly ve solver facade |
| **V0.5.0** | Linear Structural FEM + İlk Qt GUI | Tamamlandı — lineer core ve ilk Qt/VTK GUI kaynak yapısı |
| **V0.6.0** | Element ailesi genişletme + Modal | Tamamlandı — modal core ve mode-shape yolu |
| **V0.7.0** | Finite Strain / Geometric Nonlinearity | Tamamlandı — TL HEX8 ve consistent tangent foundation |
| **V0.8.0** | Newton / Line Search / Adaptive Stepping | Tamamlandı — nonlinear solver, rollback ve convergence history |
| **V0.9.0** | Hyperelastic + Plastic Constitutive Models | Tamamlandı — hyperelastic global baseline + J2 material-point baseline |
| **V0.10.0** | Mixed `u-p` / Incompressibility | Tamamlandı — HEX8/Q1-P0 mixed baseline + locking verification |
| **V0.11.0** | Contact / Friction | Tamamlandı — rigid-master contact/friction baseline |
| **V0.12.0** | CAD / Geometry / Sections | Tamamlandı — OCCT STEP source integration ve section foundation |
| **V0.13.0** | Meshing + Full Pre/Post Integration | Tamamlandı — structured/external HEX8, provenance ve pre/post baseline |
| **V1.0.0** | Verification / Hardening / macOS Release | Tamamlandı — source verification/hardening foundation |
| **V1.0.1** | macOS Release Engineering Hardening | Tamamlandı — bundle/release engineering source altyapısı |
| **V1.0.2** | Repository / Reproducible Release Hardening | Tamamlandı — reproducibility/repository hardening; strict macOS standalone CI kanıtı ayrıca alınmıştır |

> V1.0.2 döneminde GitHub-hosted Apple Silicon koşusunda standalone `.app`, ad-hoc code-sign, bundle smoke, strict Mach-O dependency audit ve artifact upload başarıyla doğrulanmıştır. Bu kanıt production Developer ID notarization anlamına gelmez.

## Onaylı V1.1.0–V2.0.0 ana yol haritası

| Sürüm | Ana hedef | Planlanan kullanıcı/teknik kazanım |
|---|---|---|
| **V1.1.0** | GUI / UI / UX + Dynamics26 Identity | Profesyonel CAE application shell, project workflow ve kullanıcıya görünen Dynamics26 adı |
| **V1.2.0** | General CAD & Geometry Platform | Genel CAD import/modelleme, healing/repair ve topology/provenance |
| **V1.3.0** | General FEM Meshing | 1D/2D/3D genel meshing, local controls ve quality framework |
| **V1.4.0** | Advanced Element Library | Beam/shell/solid/higher-order/mixed element ailesi ve certification |
| **V1.5.0** | Physics & Mathematics Verification Audit | Mevcut fiziğin, matematiğin ve tangent/residual zincirinin bağımsız audit'i |
| **V1.6.0** | Code Quality & Reliability Audit | Fortran/C/C++/ABI static/dynamic quality ve reliability hardening |
| **V1.7.0** | Inter-module Communication & Performance | Qt/C++/OCCT/VTK/C ABI/Fortran veri yolunun hız ve mimari iyileştirmesi |
| **V1.8.0** | Advanced Nonlinear Mechanics | İleri nonlinear çözüm kontrolü, finite-strain constitutive ve robust convergence |
| **V1.9.0** | Advanced Contact | Deformable/deformable, surface-to-surface, finite sliding ve ileri friction/contact |
| **V1.10.0** | Advanced Meshing & Adaptivity | Error estimation, adaptive refinement, remesh ve state transfer |
| **V1.11.0** | Large-Scale Solver & Performance | Büyük sparse sistemler, ileri solver backend'leri ve ölçeklenebilir performans |
| **V1.12.0** | Advanced Postprocessing | Profesyonel contour/probe/path/cut/history/comparison ve large-result workflow |
| **V1.13.0** | Dynamics | Modal genişletme, harmonic, transient, spectrum, random ve nonlinear dynamics |
| **V2.0.0** | Integrated CAE Qualification | CAD→mesh→analysis→solve→results zincirinin production qualification sürümü |

Bu başlıklar Dynamics26'nın ana ürün sütunlarıdır. Alt kapsam research sonuçlarına göre değişebilir; ana sütunların kaldırılması açık proje kararı gerektirir.

## V1.1.0 — GUI / UI / UX + Dynamics26 Identity

**Aktif sonraki sürüm.**

Ana hedef mevcut engineering/debug GUI'yi profesyonel macOS-first CAE iş akışına dönüştürmektir. Yeni solver fiziği bu sürümün ana hedefi değildir.

Research kapsamı:

- ANSYS Mechanical,
- Hexagon Marc / Mentat,
- Simufact,
- Abaqus/CAE,
- COMSOL,
- Altair HyperMesh / HyperView,
- gerektiğinde Simcenter,
- Apple macOS Human Interface Guidelines.

Research yöntemi:

```text
Observed Pattern
→ User Problem
→ Engineering/UX Benefit
→ Risk/Trade-off
→ Dynamics26: Adopt / Adapt / Reject
```

Hedef GUI mimarisi:

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

V1.1 aynı zamanda kullanıcıya görünen `FEMCAE → Dynamics26` migration sürümüdür. Internal/public `femcae_*`, `libfemcae`, `FEMCAE_*` gibi API/CMake isimleri ABI/API etkisi denetlenmeden topluca değiştirilmez.

Ayrıntılı plan: `docs/planning/V1.1_GUI_UX_PLAN.md`.

## V1.2.0 — General CAD & Geometry Platform

- arbitrary curved CAD,
- STEP / IGES / BREP,
- parts/bodies/faces/edges/assemblies,
- primitives ve temel modeling operations,
- boolean,
- fillet/chamfer,
- transformations,
- healing/repair,
- defeaturing,
- partition/imprint,
- named geometry,
- persistent selection/topological naming research,
- CAD-to-mesh provenance.

## V1.3.0 — General FEM Meshing

- 1D line mesh,
- TRIA / QUAD,
- TET / HEX,
- wedge/pyramid topology hazırlığı,
- global/local sizing,
- curvature-aware controls,
- boundary-layer research,
- quality metrics,
- geometry-aware entity association,
- mesh import/export.

Tam otomatik production all-hex meshing bu sürüm için zorunlu kapsam değildir.

## V1.4.0 — Advanced Element Library

Planlanan aileler:

- mass/spring/damper,
- truss,
- Euler-Bernoulli ve Timoshenko beam,
- plane stress/strain ve axisymmetric,
- membrane/shell/layered shell,
- tetra/hexa/wedge/pyramid,
- higher-order,
- reduced/selective integration,
- mixed `u-p`,
- assumed/enhanced strain,
- hourglass control.

Her production element için en az patch test, analitik benchmark, distortion/locking kontrolü ve mesh convergence beklenir.

## V1.5.0 — Physics & Mathematics Verification Audit

Feature ekleme yerine aşağıdaki zincir yeniden denetlenir:

```text
Theory
↕
Mathematical Derivation
↕
Numerical Formulation
↕
Dynamics26 Source
↕
Verification Test
↕
Tolerance / Benchmark
```

Ana audit alanları: units, tensor/Voigt conventions, shape functions, Jacobian, quadrature, kinematics, stress/strain measures, objectivity, constitutive tangents, residual, geometric tangent, Newton/convergence, hyperelasticity, mixed `u-p`, contact, mass/stiffness ve eigenproblem.

## V1.6.0 — Code Quality & Reliability Audit

- Fortran explicit interfaces/kinds/runtime checks,
- uninitialized/bounds/allocation/error paths,
- module/global-state audit,
- C/C++ warnings ve static analysis,
- RAII/lifetime,
- sanitizers,
- C ABI ownership/lifecycle,
- malformed/corrupt input tests,
- compiler warning gate.

Coverage fizik doğruluğunun yerine geçmez; yalnız risk görünürlüğü için kullanılır.

## V1.7.0 — Inter-module Communication & Performance

Ana zincir:

```text
Qt GUI
→ C++ Application Model
→ OCCT / Meshing / VTK
→ Bulk Stable C ABI
→ Modern Fortran Solver
```

Hedefler: chatty API azaltma, bulk transfer, copy azaltma, explicit ownership, reusable buffers, sparse-pattern/factorization reuse, memory layout/cache locality, async solver worker, result streaming ve profiling.

## V1.8.0 — Advanced Nonlinear Mechanics

Research ve geliştirme adayları:

- full/modified/quasi Newton,
- advanced line search,
- automatic increment/cutback,
- displacement control,
- arc-length / Riks,
- follower loads,
- finite-strain plasticity,
- viscoelasticity,
- creep,
- damage foundation,
- consistent tangent certification.

## V1.9.0 — Advanced Contact

- deformable ↔ deformable,
- surface-to-surface,
- finite sliding,
- self-contact,
- penalty / augmented Lagrangian geliştirmeleri,
- mortar ve diğer advanced enforcement research,
- friction/stick-slip,
- broad-phase acceleration/BVH research,
- curved surface contact,
- contact postprocess,
- contact history restart.

## V1.10.0 — Advanced Meshing & Adaptivity

```text
Solve
→ Error Estimation
→ Refinement / Remesh
→ Solution + State Transfer
→ Continue Solve
```

Kapsam: error estimators, h-refinement, adaptive refinement, distortion monitoring, rezoning/remeshing, solution/history mapping ve contact recreation.

## V1.11.0 — Large-Scale Solver & Performance

Research adayları: Apple Accelerate, ARPACK-NG, SLEPc, PETSc, MUMPS ve uygun diğer sparse backend'ler. Dependency kararı license/distribution/API/benchmark değerlendirmesi sonrası alınır.

Örnek ölçek basamakları:

```text
10k DOF
100k DOF
500k DOF
1M DOF
5M DOF
```

Assembly time, solve/factorization time, peak RAM, iterations ve parallel efficiency kaydedilir.

## V1.12.0 — Advanced Postprocessing

- raw integration-point vs derived/averaged result ayrımı,
- contour/deformation/vector glyph,
- principal/invariant results,
- probe/path/section cut/clip/iso-surface,
- XY/history plots,
- force/displacement/energy curves,
- contact pressure/opening/slip,
- case comparison,
- modal/transient animation,
- lazy large-result loading,
- export/report.

## V1.13.0 — Dynamics

- modal ve prestressed modal,
- harmonic response,
- transient structural,
- modal superposition,
- damping,
- base excitation,
- response spectrum,
- random vibration / PSD,
- nonlinear dynamics.

İleri araştırma hattı: rotordynamics, gyroscopic effects, Campbell diagram, critical speeds, complex modes ve imbalance response.

## V2.0.0 — Integrated CAE Qualification

Ana qualification workflow:

```text
CAD
→ Mesh
→ Materials / Sections / Connections
→ Analysis / Loads / BC
→ Solve
→ Results
→ Save / Restart / Reopen
```

V2.0; independent benchmarks, regression, performance qualification, project compatibility, crash/error recovery, documentation/examples ve production macOS signing/notarization zincirini birlikte değerlendiren ana ürün yeterlilik sürümüdür.

## CI stratejisi

### Fast main CI

```text
Configure
→ Core Build
→ Core Tests
→ GUI Compile
→ Small GUI Smoke
```

### Self-hosted MacBook runner

`self-hosted / macOS / ARM64` runner; trusted/manual engineering workflow için Qt/VTK/OCCT dependency reuse, incremental GUI build, profiling ve uzun doğrulamalarda kullanılabilir. Untrusted PR kodu kişisel runner üzerinde otomatik çalıştırılmaz.

### Clean release CI

```text
Clean macOS ARM64
→ Release Build
→ Full Tests
→ GUI
→ Install / Deploy
→ Bundle Fixup
→ Strict Mach-O Audit
→ Codesign
→ Bundle Smoke
→ Artifact
```

Strict bundle audit CI'yı geçirmek için gevşetilmez.

`gui-build` optimizasyonu için dependency setup, configure, compile, test, deploy, fixup, audit/sign ve upload süreleri önce ölçülür; caching/incremental/target-splitting kararları ölçüm sonucuna göre verilir.

## Ana belgeler

- `docs/architecture/MASTER_ROADMAP.md` — V0.x/V1.0 mimari temel ve tarihsel plan
- `docs/planning/DYNAMICS26_LONG_TERM_PLAN.md` — V1.1–V2.0 ayrıntılı onaylı plan
- `docs/planning/V1.1_GUI_UX_PLAN.md` — aktif GUI research ve implementation planı
- `CHANGELOG.md` — tamamlanan değişiklik geçmişi
- `docs/development/*_AUDIT.md` — sürüm teknik audit kayıtları
