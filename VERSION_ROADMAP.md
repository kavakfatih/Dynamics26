# FEMCAE — Sürüm Yol Haritası ve Kullanılabilirlik Özeti

Bu belge, `MASTER_ROADMAP.md` içindeki ayrıntılı planın kısa ve sürüm bazlı çalışma özetidir.
Her sürüm bağımsız ZIP teslimi, test kaydı ve değişiklik günlüğü ile kapatılacaktır.

| Sürüm | Ana hedef | Kullanıcı açısından durum |
|---|---|---|
| **V0.1.1** | Numerical & Architectural Foundation Hardening | Geliştirici çekirdeği; temel sözleşmeler ve CI/test altyapısı |
| **V0.2.0** | Model / Mesh / Field / DOF / Numbering | **Tamamlandı — gerçek FEM veri modelinin temeli** |
| **V0.3.0** | Element Kernel / Shape Functions / Quadrature | **Tamamlandı — element-local geometri ve kinematik çekirdeği** |
| **V0.4.0** | Sparse Assembly / macOS Linear Solver | **Tamamlandı — ilk global assembled lineer çözüm altyapısı** |
| **V0.5.0** | Linear Structural FEM + İlk Qt GUI | **Core tamamlandı; ilk Qt/VTK GUI kaynak sürümü — native macOS CI gate açık** |
| **V0.6.0** | Element ailesi genişletme + Modal | **Tamamlandı — modal core + GUI source entegrasyonu; native macOS gate açık** |
| **V0.7.0** | Finite Strain / Geometric Nonlinearity | **Tamamlandı — TL HEX8 + consistent tangent + nonlinear assembly foundation** |
| **V0.8.0** | Newton / Line Search / Adaptive Stepping | **Tamamlandı — nonlinear solver + ilk nonlinear GUI source; native macOS gate açık** |
| **V0.9.0** | Hyperelastic + Plastic Constitutive Models | **Tamamlandı — hyperelastic TL-HEX8/Newton + J2 material-point baseline; native macOS GUI gate açık** |
| **V0.10.0** | Mixed `u-p` / Incompressibility | **Tamamlandı — HEX8/Q1-P0 mixed core + locking verification; native macOS GUI gate açık** |
| **V0.11.0** | Contact / Friction | **Tamamlandı — rigid-master node-to-facet contact/friction baseline; native macOS GUI gate açık** |
| **V0.12.0** | CAD / Geometry / Sections | **Tamamlandı — portable geometry/section core + OCCT STEP source integration; native macOS OCCT/GUI gate açık** |
| **V0.13.0** | Meshing + Full Pre/Post Integration | **Tamamlandı — structured/external HEX8 + provenance + generic linear solve + pre/post baseline; native macOS CAD/GUI gate açık** |
| **V1.0.0** | Verification / Hardening / macOS Release | **Portable source verification tamamlandı — 123/123 Debug+Release; native signed/notarized macOS binary gate açık** |
| **V1.0.1** | macOS Release Engineering Hardening | **Portable 123/123 Debug+Release; project migration + signed/notarized workflow hazır; native binary evidence gate açık** |
| **V1.0.2** | Repository / Reproducible Release Hardening | **Portable 124/124 Debug+Release; deterministic source + Git bootstrap hazır; remote/native CI evidence gate açık** |

## Sürüm özetleri

### V0.1.1 — Foundation Hardening

- Modern Fortran 2018 foundation, tensor/Voigt ve state sözleşmeleri korunur.
- C API sürüm sınırı ve schema sürümleri ayrıdır.
- macOS arm64 CI Debug + Release olarak çalışır.
- CI yalnızca mimariyi yazdırmaz; `arm64` değilse başarısız olur.
- Kurulum dizilimi ve kurulu C API consumer smoke testi CI gate'ine eklenir.
- V0.2 öncesi hata-yolu ve tolerans testleri genişletilir.

### V0.2.0 — Model / Mesh / Field / DOF / Numbering ✅

- Node ve element storage.
- Persistent entity ID sistemi.
- Field registry: displacement, pressure ve gelecekte rotational/thermal alanlara açık yapı.
- Constraint ve deterministic equation numbering.
- Local coordinate frame.
- Topology registry, node/element sets ve material/section ID baglantilari.
- Mixed displacement + pressure + rotation numbering regression testi.
- Temel kural: `Node ID != Array Index != DOF ID != Equation ID`.

### V0.3.0 — Element Kernel ✅

- Shape functions, doğal koordinatlar ve Jacobian.
- Gauss quadrature ve isoparametric mapping.
- BAR/TRUSS, QUAD4, axisymmetric QUAD4 ve HEX8 prototipleri.
- İlk gerçek patch testleri ve inverted-element kontrolleri.
- Ölçekten bağımsız Jacobian singularity kontrolü.
- Beam/shell için rotation + section metadata prototype arayüzü.

### V0.4.0 — Sparse Assembly / Linear Solver ✅

- Element DOF/equation map ve dense adjacency kullanmayan sparsity graph.
- CSR global matrix, generic matrix/vector scatter ve stiffness/tangent/mass assembly altyapısı.
- Sıfır olmayan prescribed displacement RHS correction ve DOF-ID tabanlı reaction recovery.
- Backend bağımsız `LinearSolver` facade.
- Dense reference + sparse Jacobi-CG solver.
- macOS için ISO_C_BINDING/C adapter üzerinden Apple Accelerate Sparse direct backend.
- İlk gerçek assembled two-TRUSS2 verification ve force-equilibrium testi.

### V0.5.0 — Linear FEM + İlk Qt GUI ✅ core / GUI source

- Linear elastic material, sections, nodal loads ve generic LinearStaticAnalysis driver.
- Truss, 2B beam, plane stress/strain, axisymmetric QUAD4 ve HEX8 lineer formulation'ları.
- Stress/strain recovery, von Mises ve reaction sonuç altyapısı.
- İlk Qt 6 GUI: model tree, material/section/load/BC/analysis panelleri, results/log ve optional VTK viewport.
- GUI → public C ABI → Fortran engine sınırı korunur.
- V0.5 GUI solve preview axial-bar C-API preset'i kullanır; arbitrary mesh/model preprocessor tamamlandı iddiası yoktur.
- Native macOS Qt/VTK app build sonucu release gate olarak açıktır.

### V0.6.0 — Modal + Element Ailesi ✅

- TRUSS2, 2B beam, plane/axisymmetric QUAD4 ve HEX8 consistent/lumped mass matrix.
- Genel `Kφ = λMφ` modal analysis driver.
- Dense reference, gerçek ARPACK-NG ve macOS Accelerate/LAPACK backend kaynakları.
- Frequency, mass-normalized mode shape ve modal residual.
- Beam/shell için `orientation_frame_id` metadata ve frame validation.
- C API modal preset; Qt mode seçimi ve VTK mode-shape animasyonu.
- `VER-V060-001` axial discrete-FE ve `VER-V060-002` cantilever beam doğrulaması.
- Büyük model sparse shift-invert performansı V0.6 kapsam iddiası değildir; native macOS backend/GUI execution gate açıktır.

### V0.7.0 — Finite Strain ✅

- Reference/current configuration, deformation gradient `F`, `J`, Green-Lagrange ve Euler-Almansi strain.
- Stress-measure dönüşümleri: PK2 → PK1 / Kirchhoff / Cauchy.
- Total Lagrangian HEX8 baseline ve StVK reference constitutive law.
- Material + geometric tangent ayrımı ve 24-DOF finite-difference consistent-tangent verification.
- Global sparse nonlinear `f_int`, `R=f_ext-f_int` ve tangent evaluator; Newton iterasyonu V0.8'e bırakılır.
- Pure rigid rotation ve superposed rigid rotation objectivity verification.
- Follower-load reference/current configuration metadata; gerçek surface follower-load integrasyonu sonraki sürümdedir.

### V0.8.0 — Nonlinear Solver ✅

- Full Newton-Raphson ve modified Newton.
- Residual, displacement-correction ve opsiyonel energy convergence kriterleri.
- Backtracking line search.
- Load stepping, adaptive increment growth, cutback ve retry.
- Trial/commit/revert ile başarısız step rollback semantiği.
- Iteration/step convergence history.
- In-memory checkpoint/restart foundation ve continuation verification.
- C API üzerinden gerçek TL-HEX8 nonlinear solve + history export.
- Qt `Nonlinear Static / Large Displacement` paneli, Newton ayarları ve convergence tablosu.
- VTK nonlinear deformed HEX8 preset görünümü.
- V0.8 load-control baseline nonzero prescribed-displacement stepping veya arc-length/Riks içermez.
- StVK yalnız verification material'ıdır; rubber hyperelasticity V0.9 kapsamındadır.

### V0.9.0 — Nonlinear Constitutive Models ✅

- Ortak constitutive material-point response sınırı; stress/tangent measure explicit metadata.
- Neo-Hookean, Mooney-Rivlin, Yeoh ve 1–3 terimli Ogden.
- Isochoric + volumetric penalty strain-energy ayrımı ve production analytic/consistent tangent.
- Ogden signed `mu_i` fitting desteği; `sum(mu_i)>0` initial shear gate ve near-repeated principal-stretch tangent regression.
- Hyperelastic Total-Lagrangian HEX8 ve V0.8 Newton solver ile gerçek global entegrasyon.
- J2 von Mises small-strain + isotropic hardening + radial return + consistent algorithmic tangent.
- J2 committed/trial state ve rollback-compatible state lifecycle; global plastic element entegrasyonu henüz kapsam dışıdır.
- Qt Material Studio source: hyperelastic model seçimi, parameter editor, birimler, engine validation, `G0` ve engine-side isochoric uniaxial preview.
- Sekiz V0.9 verification: energy/stress, material tangent, HEX8 tangent, Newton equilibrium, J2 return-map ve Ogden spectral hardening.
- Penalty volumetric response mixed `u-p` değildir; nearly incompressible locking çözümü V0.10'a aittir.

### V0.10.0 — Mixed `u-p` / Incompressibility ✅

- `pressure_p0` element-associated pressure field ve Q1/P0 HEX8 mixed DOF map.
- Perturbed-Lagrangian `W_iso + p(J-1) - p^2/(2K)` residual/tangent formulation.
- `K_uu`, `K_up`, `K_pu`, `K_pp` bloklarının analytic/consistent assembly'si.
- Global sparse mixed system ve displacement/pressure blok-bilinçli nonlinear convergence.
- Symmetric-indefinite sistemlerde CG backend'inin explicit rejection'ı; direct backend baseline.
- Element-ID tabanlı P0 pressure result recovery ve C API mixed simple-shear verification preset'i.
- Local + global tangent finite-difference verification, isochoric mixed Newton ve penalty-vs-mixed locking benchmark.
- Q1/P0 controlled baseline; stabilized Q1/Q1/higher-order mixed element veya universal inf-sup stability iddiası yoktur.
- Contact bilinçli olarak V0.11'e ayrılmıştır.

### V0.11.0 — Contact / Friction ✅

- Element formulation'dan bağımsız contact registry ve contact-point committed/trial state.
- Baseline: deformable slave node ↔ rigid planar QUAD4 master facet.
- Expanded AABB broad-phase ve triangle-split closest-point narrow-phase search.
- Facet node sırasına bağlı master normal ve signed gap; `g_n<0` penetrasyon convention'ı.
- Penalty ve committed-gap incremental augmented-Lagrangian normal enforcement.
- Coulomb friction; tangential penalty predictor, stick/slip active set ve analytic contact tangent.
- Global sparse nonlinear residual/tangent assembly ve direct-solver contact gate'i.
- Newton/line-search/cutback ile trial/commit/revert; failed-step contact history rollback verification.
- Contact summary: active/stick/slip sayıları, maksimum penetrasyon, normal/tangential resultant.
- C ABI frictionless compression preset'i; Qt/VTK contact verification source workflow.
- Beş verification: local tangent, AL state invariance, global 1000 N contact equilibrium, rollback ve global Coulomb `T≤μN`.
- V0.11 baseline deformable-deformable/mortar/self-contact/BVH hierarchy veya arbitrary contact GUI preprocessor iddiası değildir.
- Contact history disk/restart snapshot formatı henüz yoktur; contact + checkpoint restart açıkça reddedilir.

### V0.12.0 — CAD / Geometry / Sections ✅

- Ayrı C++20 `femcae_geometry` library ve solver'dan bağımsız `GeometryDocument`.
- 64-bit deterministic geometry ID; document namespace + persistent key sözleşmesi.
- Assembly/body/face/edge/vertex geometry hierarchy ve geometry-to-FEM provenance map.
- OCCT XDE/STEPCAF STEP adapter; OCCT yoksa kontrollü stub, macOS CI'da `FEMCAE_REQUIRE_OCCT=ON`.
- B-Rep display tessellation ayrı veri yapısında; FEM node/element ID taşımaz.
- Qt GeometryPanel: STEP import, geometry tree, body/face/edge/vertex filter ve VTK display path.
- Portable ASCII DXF custom-section reader: LWPOLYLINE/LINE/CIRCLE/ARC.
- Green-theorem section properties: A, centroid, Ixx/Iyy/Ixy, principal moments/axis ve polar area moment.
- Rectangle, hollow section, DXF contour ve geometry-separation verification testleri.
- Persistent ID tam CAD-edit topological naming iddiası değildir; `Jp` genel Saint-Venant torsion constant olarak kullanılmaz.
- CAD-to-mesher, local sizing/refinement ve tam assignment/pre-post V0.13'e bırakılmıştır.

### V0.13.0 — Meshing + Full Pre/Post ✅

- Ayrı C++20 `femcae_meshing` library ve solver'dan bağımsız `SimulationMesh`.
- Structured axis-aligned HEX8 box mesher; deterministic node/element/facet ID ve CAD body/face provenance.
- Global + geometry-face local structured sizing baseline; center scaled-Jacobian/aspect-ratio quality report.
- Portable Abaqus ASCII `*NODE` + `C3D8` external mesh import baseline.
- Geometry-targeted material/section/load/constraint/contact metadata ve boundary-facet provenance tabanlı assignment resolution.
- V0.12 `GeometryAssociationMap` ile CAD body/face -> FEM element/facet/node köprüsü.
- Generic `fem_solve_linear_hex8_mesh` C ABI: arbitrary linear HEX8 mesh -> Fortran DOF/sparse assembly/solve/reaction/stress recovery.
- `ResultDatabase`: displacement, element scalar, nearest-node probe, plane-cut element selection, CSV ve legacy VTK export.
- Qt `Mesh / Pre-Post` source paneli ve VTK deformed HEX8 + von Mises contour path.
- Native OCCT build'de axis-aligned STEP box -> bounds/six face IDs -> structured HEX8 provenance verification.
- General arbitrary curved CAD volume meshing, adaptive unstructured refinement ve interpolated cut-surface contours production-ready iddiası değildir.

### V1.0.0 — Verified Engineering Release ✅ Portable Source Candidate

- Debug ve Release full matrix: **123/123 PASS**.
- V1 hardening gate'leri: mesh convergence, disk checkpoint/restart, corrupted input, public C API error paths ve performance smoke.
- V1 release testleri GCC AddressSanitizer + UndefinedBehaviorSanitizer altında **5/5 PASS**.
- Versioned/checksum-protected nonlinear checkpoint disk formatı; truncated ve checksum-bozuk dosya rejection.
- HEX8 cantilever convergence: 4×1×1 → 8×2×2 → 12×3×3 hata monoton azalır, fine mesh ~%3.94.
- Installed CLI, C API ve public geometry/meshing C++ consumer PASS; final source compiler warning gate 0.
- Apache-2.0 source license baseline + NOTICE + third-party license inventory.
- macOS app deployment: Qt deploy script + BundleUtilities dependency fixup + arm64/rpath audit + signing/notarization script altyapısı.
- Native Apple Silicon Qt/VTK/OCCT/Accelerate app execution, Developer ID signing/notarization, GUI migration ve native memory audit **açık release gate**; Linux hostta PASS sayılmaz.

## Kullanıcı açısından ana kilometre taşları

- **İlk GUI:** V0.5.0
- **İlk nonlinear GUI:** V0.8.0
- **Nearly incompressible rubber:** V0.10.0
- **Rubber + contact/friction:** V0.11.0
- **CAD model hazırlama:** V0.12.0
- **Tam CAE akışı:** V0.13.0
- **Doğrulanmış mühendislik release'i:** V1.0.0

### V1.0.1 — macOS Release Engineering Hardening ✅ Source/Automation Complete

- Project JSON schema migration boundary.
- GUI version single-source hardening.
- arm64 Mach-O + dependency + LC_RPATH bundle audit.
- Headless dyld `--bundle-smoke`.
- Protected manual Developer ID + `notarytool` + stapler + Gatekeeper workflow.
- Native signed/notarized artifact remains an execution gate, not a portable claim.

### V1.0.2 — Repository / Reproducible Release Hardening ✅ Source/Automation Complete

- Deterministic byte-for-byte source ZIP + internal SHA256 manifest.
- Repository hygiene gate ve credential/build-artifact rejection.
- First Git commit/origin/push bootstrap helper.
- CMake target-order ve shared-library version single-source düzeltmesi.
- GitHub CI source-integrity/reproducibility gate ve concurrency.
- Portable Debug/Release 124/124 PASS.
- Remote FEMCAE repository/native GitHub Actions evidence remains open until repository creation/access.
