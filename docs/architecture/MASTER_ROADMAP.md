# Yeni Nonlinear FEM/CAE Programı
## Mimari Gözden Geçirme ve Sürüm Yol Haritası

**Belge durumu:** Mimari temel / sürüm planı  
**Platform hedefi:** macOS — öncelikli olarak Apple Silicon (`arm64`)  
**Ana solver dili:** Modern Fortran  
**GUI:** Qt 6 / C++  
**3B ve sonuç görselleştirme:** VTK  
**CAD çekirdeği (ilerleyen sürüm):** Open CASCADE Technology (OCCT)  
**Build/Test:** CMake + Ninja + CTest  
**Sürümleme:** Semantic Versioning  
**Kaynak yaklaşımı:** Code_Aster yalnızca teknik/mimari referans; kaynak kod, test girdisi, yorum veya rutin kopyalanmayacak.

---

# 1. Yönetici Özeti

Mevcut yol haritası genel olarak doğru yöndedir; ancak ileride büyük ve pahalı refactor oluşturmaması için bazı mimari kararların daha erken alınması gerekir.

En önemli düzeltmeler:

1. **macOS-only hedefi gerçek bir mimari karara dönüştürülmeli.**
   - Linux CI ve Linux paketleme yapılmayacak.
   - İlk hedef Apple Silicon `arm64`.
   - Apple Accelerate, temel dense ve sparse lineer cebir backend'i olarak kullanılabilir.
   - PETSc/MUMPS doğrudan zorunlu bağımlılık yapılmayacak; ileride opsiyonel yüksek ölçek backend'i olacak.

2. **Mesh, CAD tessellation ve FEM mesh birbirinden kesin olarak ayrılmalı.**
   - CAD B-Rep = geometri.
   - CAD görüntü tessellation'ı = yalnızca ekran görüntüleme ağı.
   - FEM mesh = analiz için bağımsız düğüm/eleman ağı.
   - Bu üç veri modeli hiçbir zaman aynı nesne kabul edilmeyecek.

3. **Project/model veri modeli GUI gelmeden tasarlanmalı.**
   - Node ID, element ID, material ID, section ID ve equation ID farklı kavramlar olacak.
   - Dosya formatı sürümü uygulama sürümünden bağımsız tutulacak.
   - İleride eski projeleri açabilmek için schema versioning kullanılacak.

4. **Mixed `u-p` için çok alanlı DOF sistemi V0.2.0'da hazırlanmalı.**
   - Gerçek mixed elemanlar V0.10.0'da uygulanabilir.
   - Ancak `UX/UY/UZ` ile sınırlı bir DOF mimarisi kurulmayacak.
   - `P`, `RX/RY/RZ`, ileride `T` gibi alanlara uygun Field/DOF altyapısı baştan bulunacak.

5. **Nonlinear material state sistemi V0.1.0'da mimari olarak sabitlenmeli.**
   - `committed state`
   - `trial state`
   - `commit`
   - `revert`
   ayrımı daha ilk çekirdek tasarımında bulunacak.

6. **Sparse matrix altyapısı yalnızca simetrik pozitif tanımlı matris varsaymamalı.**
   - Linear elastic problemlerde SPD olabilir.
   - Mixed `u-p` sistemleri indefinite olabilir.
   - Contact/friction ve bazı constitutive tangent'lar unsymmetric olabilir.
   - Matrix metadata ve solver backend API:
     - symmetric positive definite,
     - symmetric indefinite,
     - unsymmetric
     sistemleri destekleyecek.

7. **Shell ve beam için rotational DOF ve local coordinate system erken tasarlanmalı.**
   - Beam/shell eklendiğinde DOF mimarisi tekrar yazılmamalı.
   - Local axis/orientation sistemi V0.2.0 veri modelinde yer almalı.

8. **Result veri modeli yalnızca nodal sonuçlardan oluşmamalı.**
   - Gauss/integration-point stress ve state değerleri ham sonuçtur.
   - Nodal stress çoğu durumda extrapolation/averaging sonucu oluşur.
   - İkisi ayrı saklanacak ve GUI kullanıcıya hangi değeri gördüğünü açıkça gösterecek.

9. **Restart/checkpoint mimarisi nonlinear çözümden önce düşünülmeli.**
   - Uzun nonlinear çözüm kesildiğinde yeniden başlanabilmeli.
   - İlk gerçek checkpoint implementasyonu V0.8–V0.9 civarında olabilir.
   - Ancak serializable `AnalysisState` kavramı daha erken tasarlanmalı.

10. **Dependency/license politikası V0.1.0 görevi olmalı.**
    - Qt açık kaynak kullanımında LGPL/GPL yükümlülükleri vardır.
    - Qt'nin bazı modülleri yalnızca GPL altında olabilir; kullanılacak Qt modülleri whitelist ile sınırlandırılmalı.
    - VTK BSD-3-Clause olduğu için görselleştirme katmanı açısından uygundur.
    - OCCT LGPL 2.1 + ek istisna ile kullanılabilir; dağıtım yükümlülükleri ayrıca izlenmelidir.
    - Gmsh GPL'dir. Projenin nihai lisansı netleşmeden core bağımlılığı olarak bağlanmamalıdır.
    - Code_Aster GPL kaynaklarından kod alınmayacaktır.

---

# 2. Nihai Mimari İlkeler

## 2.1 Ana katmanlar

```text
┌──────────────────────────────────────────────┐
│                 Qt GUI / C++                 │
│ project tree • properties • dialogs • menus  │
└──────────────────────┬───────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────┐
│              Application Layer               │
│ command model • undo/redo • project service  │
└───────────────┬─────────────────┬────────────┘
                │                 │
                │                 ▼
                │        ┌───────────────────┐
                │        │ VTK Visualization │
                │        │ mesh/results/view │
                │        └───────────────────┘
                │
                ▼
┌──────────────────────────────────────────────┐
│                 Stable C API                 │
│           ISO_C_BINDING boundary             │
└──────────────────────┬───────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────┐
│          Modern Fortran FEM Engine           │
│                                              │
│ Solver                                       │
│   ↓                                          │
│ Assembly ← Contact                           │
│   ↓                                          │
│ Elements ← Constitutive Models               │
│   ↓                                          │
│ Quadrature / Kinematics                      │
│   ↓                                          │
│ Fields / DOF / Constraints / Numbering       │
│   ↓                                          │
│ Mesh / Topology / Sets / Frames              │
│   ↓                                          │
│ Math / Tensor / Foundation                   │
└──────────────────────┬───────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────┐
│            Numerical Backends                │
│ Apple Accelerate • ARPACK-NG • optional PETSc│
└──────────────────────────────────────────────┘
```

## 2.2 Geometri katmanı

V0.12.0 civarında:

```text
STEP / IGES / primitive CAD
           │
           ▼
        OCCT B-Rep
           │
     ┌─────┴──────────┐
     │                │
     ▼                ▼
Display          MesherBackend
Tessellation          │
     │                ▼
     ▼             FEM Mesh
    VTK                │
                       ▼
                  Fortran Solver
```

**Kritik kural:** OCCT'nin görüntüleme için oluşturduğu üçgenleme, doğrudan FEM mesh kabul edilmeyecek.

DXF, özellikle beam/custom section için ayrı bir `GeometryImporter` üzerinden desteklenecek.

---

# 3. Teknoloji Kararları

## 3.1 Fortran

Hedef:

- Fortran 2018 uyumlu kodlama yaklaşımı
- `iso_fortran_env`
- `iso_c_binding`
- explicit interfaces
- module tabanlı tasarım
- mümkün olduğunca global mutable state kullanılmaması
- library kodunda kontrolsüz `stop` kullanılmaması
- hata/status bilgisinin üst katmana taşınması

Numerik türler:

```fortran
use iso_fortran_env, only : real64, int32, int64
```

Önerilen ayrım:

- gerçek sayısal FEM hesapları: `real64`
- kalıcı entity ID: `int64`
- solver-local contiguous index: backend ile uyumlu ayrı `index_kind`
- element-local küçük indeksler: gerektiğinde `int32`

Kalıcı ID ile solver equation index aynı tip/kavram olmayacak.

---

## 3.2 C / C++ sınırı

Qt hiçbir zaman Fortran derived type adreslerini doğrudan yönetmeyecek.

C API örneği:

```text
fem_context_create()
fem_context_destroy()

fem_model_create()
fem_mesh_set_nodes()
fem_mesh_set_elements()

fem_material_create()
fem_section_create()

fem_analysis_create()
fem_analysis_run()

fem_result_get_field()
```

ABI ayrıca sürümlenecek:

```text
Application Version
Project Schema Version
Solver C-API Version
```

bunlar üç farklı sürüm numarası olacaktır.

---

## 3.3 GUI

V1.0 için öneri:

- Qt 6 / C++
- ağırlıklı olarak Qt Widgets
- native macOS menu bar
- modern sidebar
- inspector/property panel
- analysis tree
- result tree
- undo/redo command sistemi
- dark/light mode
- VTK gömülü viewport

V1.0'a kadar QML zorunlu yapılmayacak. Karmaşık CAE masaüstü uygulaması için Qt Widgets + VTK entegrasyonu daha düşük risklidir.

---

## 3.4 Görselleştirme

VTK kullanılacak.

Görevleri:

- node/element gösterimi
- surface extraction
- deformed shape
- scalar contours
- vector glyphs
- mode-shape animation
- selection/picking
- clipping
- section plane
- contact pressure visualization
- Gauss-to-node extrapolated result visualization

Solver ve VTK birbirine doğrudan bağlanmayacak.

Arada `ResultViewModel` / C++ conversion layer bulunacak.

---

## 3.5 CAD

OCCT, V0.12.0 civarında:

- STEP
- IGES
- B-Rep
- primitive geometry
- topology interrogation
- geometry healing
- tessellation

için kullanılabilir.

Solver OCCT'yi bilmeyecek.

---

## 3.6 Meshing

Mevcut yol haritasındaki en önemli eksiklerden biridir.

Bir `MesherBackend` arayüzü tanımlanacak.

İlk aşamalar:

- 1D structured line mesh
- 2D structured rectangle mesh
- axisymmetric structured mesh
- 3D block structured HEX mesh
- external mesh import

Daha sonraki aşamalar:

- surface meshing
- unstructured triangular/tetrahedral meshing
- local refinement
- quality checks
- element-order elevation

Gmsh doğrudan zorunlu core dependency olmayacak. GPL uyumluluğu proje lisansı ile birlikte ayrıca değerlendirilmeden bağlanmayacak.

---

# 4. Temel Veri Modeli

## 4.1 Entity kimlikleri

Aşağıdakiler birbirinden farklıdır:

```text
Persistent Node ID
Array Position
DOF ID
Equation ID
```

Örnek:

```text
Node ID = 10042

UX  -> equation 185
UY  -> constrained
UZ  -> equation 186
P   -> equation 187
```

Bu ayrım:

- mixed formulation,
- MPC,
- contact multiplier,
- shell rotations,
- restart,
- project save/load

için zorunludur.

---

## 4.2 Field sistemi

Başlangıçta desteklenecek field tanımları:

```text
Displacement
  UX
  UY
  UZ

Rotation
  RX
  RY
  RZ

Pressure
  P
```

İleride:

```text
Temperature
  T

Multiplier
  LM...
```

eklenebilir.

Element hangi field'ları istediğini metadata ile bildirir.

---

## 4.3 Constraint modeli

Yalnızca `fixed DOF` yaklaşımı kullanılmayacak.

Mimari aşağıdaki kısıtları taşıyabilecek:

```text
Essential / Dirichlet
Prescribed displacement
Multi-point constraint
Equation constraint
Rigid coupling
Contact constraint
```

İlk implementasyon daha dar olabilir; veri modeli daha geniş tasarlanır.

---

## 4.4 Coordinate frame

Beam, shell ve anisotropic malzemeler için:

```text
GlobalFrame
LocalElementFrame
SectionFrame
MaterialFrame
```

ayrı kavramlar olacaktır.

Orientation verisi mesh'e gömülmeyecek; model/property katmanında tutulacaktır.

---

# 5. Element Mimarisi

Element:

```text
Topology
+
FieldLayout
+
Kinematics
+
Quadrature
+
Section (gerekiyorsa)
+
Material
+
ElementKernel
```

olarak ele alınacaktır.

Örnek:

```text
QUAD4 topolojisi
```

tek başına bir FEM elemanı değildir.

Aynı topoloji:

```text
PlaneStressQuad4
PlaneStrainQuad4
AxisymmetricQuad4
ShellQuad4
MixedUPQuad4
```

olabilir.

## 5.1 Dispatch performansı

Aşırı object-oriented, her Gauss noktasında dynamic polymorphism kullanılmayacak.

Öneri:

- element/material type runtime'da bir kez resolve edilir,
- elementler type/material blokları halinde gruplanır,
- kernel batch seviyesinde çağrılır,
- Gauss loop içinde procedure lookup yapılmaz.

Bu tasarım ileride paralel assembly ve SIMD için daha uygundur.

---

# 6. Constitutive Model Mimarisi

Tek bir belirsiz `material%evaluate()` arayüzü yerine iki temel kinematik sınıf ayrılmalıdır.

## 6.1 Small strain

Input:

```text
strain
state_committed
material parameters
Δt
```

Output:

```text
stress
consistent tangent
state_trial
energy
status
```

## 6.2 Finite strain

Input:

```text
F
J
state_committed
material parameters
Δt
```

Output:

```text
selected stress measure
consistent material tangent
state_trial
energy
status
```

Stress measure açık metadata ile taşınacak. Bir tensorün Cauchy mi, Kirchhoff mu, 2nd PK mı olduğu isimden tahmin edilmeyecek.

---

## 6.3 Trial / Commit

```text
Committed State
      │
      ▼
Constitutive Evaluation
      │
      ▼
Trial State
      │
      ├── Newton başarısız → discard/revert
      │
      └── Increment converged → commit
```

Plasticity, friction ve history-dependent modeller bu sistemi ortak kullanacaktır.

---

## 6.4 Hyperelastic ayrımı

Mixed `u-p` için ileride refactor gerekmemesi adına hyperelastic yapı kavramsal olarak:

```text
Isochoric / Deviatoric Model
+
Volumetric Response
```

şeklinde ayrılacaktır.

Örneğin:

```text
Neo-Hookean isochoric
Mooney-Rivlin isochoric
Yeoh isochoric
Ogden isochoric
```

ve bağımsız:

```text
Penalty volumetric law
Compressible volumetric law
Mixed pressure constraint
```

katmanları bulunabilir.

Bu karar V0.9–V0.10 arasında büyük mimari değişikliği önler.

---

# 7. Nonlinear Kinematik Standardı

V0.7.0 için ilk finite-strain solid altyapısında **Total Lagrangian** formulation başlangıç referansı olarak kullanılacaktır.

Ana değişkenler:

\[
\mathbf{F} = \mathbf{I} + \nabla_0 \mathbf{u}
\]

\[
J = \det\mathbf{F}
\]

\[
\mathbf{E}
=
\frac{1}{2}
\left(
\mathbf{F}^{T}\mathbf{F}-\mathbf{I}
\right)
\]

İç kuvvet ve consistent tangent kullanılan stress measure ile uyumlu olacaktır.

Mimari `configuration` bilgisini açık tutacağı için ileride Updated Lagrangian eklenebilir.

---

# 8. Residual ve Newton Standardı

Proje genelinde:

\[
\mathbf{R}
=
\mathbf{f}_{ext}
-
\mathbf{f}_{int}
\]

ve equilibrium:

\[
\mathbf{R}=0
\]

Newton düzeltmesi:

\[
\mathbf{K}_T
\Delta\mathbf{u}
=
\mathbf{R}
\]

\[
\mathbf{u}_{i+1}
=
\mathbf{u}_i
+
\alpha_i\Delta\mathbf{u}
\]

olarak tanımlanacaktır.

Bu işaret standardı ADR ile sabitlenecek.

Aynı problem için farklı modüllerde farklı residual işareti kullanılmayacaktır.

---

# 9. Sparse Matrix ve Lineer Solver

## 9.1 Internal sparse representation

Assembly tarafında öncelikle:

```text
COO triplets
      ↓
compressed representation
      ↓
solver backend
```

akışı kullanılabilir.

Internal representation ile backend formatı aynı olmak zorunda değildir.

Matrix özellikleri:

```text
General / Unsymmetric
Symmetric
Symmetric Positive Definite
Symmetric Indefinite
```

metadata olarak taşınacaktır.

---

## 9.2 macOS ana backend

V0.4.0 için önerilen birincil backend:

**Apple Accelerate**

Avantajlar:

- macOS native
- BLAS
- LAPACK
- sparse direct/iterative solver altyapısı
- Apple Silicon optimizasyonu
- ayrı büyük runtime dağıtım bağımlılığını azaltma

Fortran tarafından doğrudan vendor-specific kod yayılmayacak.

```text
Fortran
   │
ISO_C_BINDING
   │
small C bridge
   │
Accelerate
```

şeklinde adapter kullanılacak.

---

## 9.3 Opsiyonel backend

İleride:

```text
PETSc
MUMPS
```

opsiyonel olarak eklenebilir.

Element, assembly ve Newton kodu hangi solver'ın çalıştığını bilmeyecek.

---

## 9.4 Modal backend

V0.6.0 için ARPACK-NG uygun ilk adaydır.

Eigenproblem:

\[
\mathbf{K}\boldsymbol{\phi}
=
\lambda
\mathbf{M}\boldsymbol{\phi}
\]

\[
\lambda=\omega^2
\]

ve:

\[
f=\frac{\omega}{2\pi}
\]

olarak hesaplanacaktır.

---

# 10. Result Mimarisi

Ham ve türetilmiş sonuçlar ayrılacaktır.

## 10.1 Ham sonuç

```text
Nodal displacement
Reaction
Gauss-point stress
Gauss-point strain
Gauss-point pressure
State variables
Plastic strain
Contact pressure
```

## 10.2 Visualization sonucu

```text
Extrapolated nodal stress
Averaged nodal stress
Von Mises
principal values
deformation magnitude
```

GUI, örneğin:

```text
Stress: Sxx
Location: Integration Point
```

veya:

```text
Stress: Sxx
Location: Nodal Averaged
```

bilgisini açıkça gösterecektir.

---

# 11. Project ve Dosya Formatı

Dosya formatı GUI'ye rastgele bırakılmamalıdır.

Önerilen mantık:

```text
Project Metadata
Model Definition
Mesh
Materials
Sections
Analysis Definitions
Results Index
Checkpoints
```

Ağır array verileri ile küçük metadata aynı serialization yaklaşımına zorlanmayacaktır.

V0.5.0 civarında:

- versioned project schema
- versioned result schema
- backward migration altyapısı

bulunacaktır.

Örnek:

```text
application_version: 0.9.0
project_schema_version: 3
solver_api_version: 2
result_schema_version: 2
```

Eski proje dosyasını açmak için migration fonksiyonları yazılabilir.

---

# 12. Restart / Checkpoint

Nonlinear analiz sırasında:

```text
Load Step
Converged DOFs
Material committed state
Contact state
Time/load parameter
Solver configuration
```

checkpoint içine yazılabilir.

İlk gerçek restart desteği V0.8–V0.9 döneminde gelecektir.

Ancak `AnalysisState` ve serializable state prensibi V0.1–V0.2 mimarisinde düşünülmelidir.

---

# 13. Contact Mimarisi

```text
ContactManager
│
├── Surface Extraction
├── Search
│   ├── broad phase
│   └── narrow phase
│
├── Pairing / Projection
├── Contact Kinematics
├── Enforcement
│   ├── Penalty
│   └── Augmented Lagrangian
│
└── Friction
    └── Coulomb
```

Contact katkıları ortak global assembly sistemine girer.

Contact geometry ve constitutive friction state trial/commit mantığına uyar.

Contact nedeniyle global tangent'ın unsymmetric olabileceği baştan kabul edilir.

---

# 14. Test Mimarisi

Sadece `unit test + regression` yeterli değildir.

Testler aşağıdaki katmanlara ayrılacaktır.

## 14.1 Unit Tests

Tek fonksiyon/modül:

- tensor mapping
- determinant
- Jacobian
- shape function
- quadrature
- material point
- sparse structure
- numbering

## 14.2 Element Patch Tests

Her structural element formulation için zorunlu.

Örnek:

- constant strain patch
- rigid body motion
- zero-energy mode kontrolü
- symmetry kontrolü
- coordinate invariance

## 14.3 Verification

Analitik/bağımsız referanslı mühendislik problemi.

## 14.4 Convergence Tests

Mesh inceltildiğinde:

\[
\|e_h\| \rightarrow 0
\]

beklenen yakınsama mertebesi kontrol edilir.

## 14.5 Regression

Daha önce doğru kabul edilen sonuçların yeni commit ile istemeden değişip değişmediğini denetler.

Regression = doğruluk kanıtı değildir.

## 14.6 Restart Tests

Aynı analiz:

```text
continuous run
```

ile:

```text
run → checkpoint → restart
```

sonuçlarının tolerance içinde eşit olması beklenir.

## 14.7 Performance Baseline

Ana release'lerde:

- assembly süresi
- solve süresi
- memory
- DOF/s

takip edilir.

Performans testi normal doğruluk testinden ayrıdır.

---

# 15. V1.0 Hedef Klasör Yapısı

```text
project/
│
├── src/
│   ├── foundation/
│   │   ├── kinds/
│   │   ├── ids/
│   │   ├── status/
│   │   ├── logging/
│   │   ├── tolerances/
│   │   └── version/
│   │
│   ├── math/
│   │   ├── vectors/
│   │   ├── matrices/
│   │   ├── tensors/
│   │   ├── invariants/
│   │   ├── spectral/
│   │   └── numerical/
│   │
│   ├── mesh/
│   │   ├── topology/
│   │   ├── nodes/
│   │   ├── cells/
│   │   ├── sets/
│   │   ├── frames/
│   │   └── quality/
│   │
│   ├── fields/
│   │   ├── field_registry/
│   │   ├── dof/
│   │   ├── constraints/
│   │   └── numbering/
│   │
│   ├── quadrature/
│   │
│   ├── kinematics/
│   │   ├── small_strain/
│   │   └── finite_strain/
│   │
│   ├── elements/
│   │   ├── registry/
│   │   ├── truss/
│   │   ├── beam/
│   │   ├── plane/
│   │   ├── axisymmetric/
│   │   ├── solid/
│   │   ├── shell/
│   │   └── mixed/
│   │
│   ├── sections/
│   │
│   ├── materials/
│   │   ├── interfaces/
│   │   ├── elastic/
│   │   ├── hyperelastic/
│   │   ├── plasticity/
│   │   └── state/
│   │
│   ├── assembly/
│   │   ├── pattern/
│   │   ├── element_assembly/
│   │   └── global/
│   │
│   ├── linalg/
│   │   ├── sparse/
│   │   ├── dense/
│   │   └── backends/
│   │       ├── accelerate/
│   │       ├── arpack/
│   │       └── optional_petsc/
│   │
│   ├── solvers/
│   │   ├── linear_static/
│   │   ├── nonlinear_static/
│   │   │   ├── newton/
│   │   │   ├── line_search/
│   │   │   ├── convergence/
│   │   │   └── load_stepper/
│   │   ├── eigen/
│   │   └── dynamics/
│   │
│   ├── contact/
│   │   ├── surface/
│   │   ├── search/
│   │   ├── pairing/
│   │   ├── enforcement/
│   │   └── friction/
│   │
│   ├── results/
│   │
│   ├── persistence/
│   │
│   └── api/
│       └── c_api/
│
├── bridge/
│   └── macos_accelerate/
│
├── gui/
│   ├── application/
│   ├── project/
│   ├── model_tree/
│   ├── properties/
│   ├── viewport/
│   ├── preprocessing/
│   ├── analysis/
│   └── postprocessing/
│
├── geometry/
│   ├── occt/
│   ├── import/
│   └── sections/
│
├── meshing/
│   ├── interface/
│   ├── structured/
│   └── import/
│
├── tests/
│   ├── unit/
│   ├── patch/
│   ├── verification/
│   ├── convergence/
│   ├── regression/
│   ├── restart/
│   └── performance/
│
├── examples/
├── docs/
│   ├── theory/
│   ├── architecture/
│   ├── adr/
│   ├── verification/
│   ├── references/
│   └── development/
│
├── cmake/
├── tools/
│
├── CMakeLists.txt
├── README.md
├── CHANGELOG.md
├── CONTRIBUTING.md
└── THIRD_PARTY_NOTICES.md
```

---

# 16. Sürüm Listesi

| Sürüm | Ana hedef | Kullanıcı durumu |
|---|---|---|
| **V0.1.1** | Numerical & Architectural Foundation Hardening | Geliştirici çekirdeği |
| **V0.2.0** | Model / Mesh / Field / DOF / Numbering | Geliştirici çekirdeği |
| **V0.3.0** | Element Kernel / Shape Functions / Quadrature | Geliştirici çekirdeği |
| **V0.4.0** | Sparse Assembly / macOS Linear Solver | CLI ile analiz altyapısı |
| **V0.5.0** | Linear Structural FEM + İlk Qt GUI | **İlk kullanıcı sürümü** |
| **V0.6.0** | Element ailesi genişletme + Modal | GUI ile lineer/modal kullanım |
| **V0.7.0** | Finite Strain / Geometric Nonlinearity | Nonlinear altyapı |
| **V0.8.0** | Newton / Line Search / Adaptive Stepping | **Tamamlandı — ilk nonlinear GUI source; native macOS gate açık** |
| **V0.9.0** | Hyperelastic + Plastic Constitutive Models | **Tamamlandı — hyperelastic global integration + J2 material-point baseline** |
| **V0.10.0** | Mixed `u-p` / Incompressibility | **Tamamlandı — Q1/P0 mixed core + locking verification** |
| **V0.11.0** | Contact / Friction | **Elastomer temas mühendislik sürümü** |
| **V0.12.0** | CAD / Geometry / Sections | CAE geometri hazırlığı |
| **V0.13.0** | Meshing + Full Pre/Post Integration | **Tam CAE çalışma akışı** |
| **V1.0.0** | Verification / Hardening / macOS Release | **Doğrulanmış ilk mühendislik sürümü** |

---

# 17. V0.1.1 — Numerical & Architectural Foundation Hardening

## Amaç

Projenin değişmesi en pahalı olan temel kararlarını sabitlemek.

## Kapsam

- Modern Fortran proje çekirdeği
- CMake / Ninja / CTest
- macOS build
- Apple Silicon odaklı yapı
- kinds
- IDs
- status/error
- logger
- tolerances
- version
- vector/matrix yardımcıları
- tensor notation
- Voigt convention
- unit policy
- residual/tangent sign convention
- trial/commit state sözleşmesi
- dependency/license policy
- C API mimari sınırı
- ADR altyapısı

## Kritik ADR'lar

```text
ADR-0001 Platform and Toolchain
ADR-0002 Module Dependency Rules
ADR-0003 Numeric Precision
ADR-0004 Persistent ID vs Solver Index
ADR-0005 Residual and Tangent Sign Convention
ADR-0006 Tensor / Voigt Convention
ADR-0007 Trial / Commit State Model
ADR-0008 Linear Solver Backend Policy
ADR-0009 C API Boundary
ADR-0010 Source Independence Policy
ADR-0011 Third-Party License Policy
ADR-0012 Result Location and Stress Measure Policy
```

## Verification

`VER-V010-001`

İki düğümlü cebirsel eksenel çubuk:

\[
u = \frac{FL}{EA}
\]

Bu test henüz gerçek FEM element testi değil; verification harness testidir.

## Çıkış

```text
FEMCAE-v0.1.1-source-macos-arm64.zip
```

---

# 18. V0.2.0 — Model / Mesh / Field / DOF / Numbering

**Durum (2026-08-29): Tamamlandi.** Kaynak uygulama ve regression detaylari `docs/architecture/V0.2.0_ARCHITECTURE.md` icindedir.

## Amaç

Solver'ın gerçek FEM veri modelini oluşturmak.

## Kapsam

- node storage
- element connectivity storage
- topology registry
- node/element sets
- persistent ID sistemi
- field registry
- displacement field
- pressure field altyapısı
- rotational DOF altyapısı
- constraints
- equation numbering
- local coordinate frames
- model ownership/lifetime
- material/section ID bağlantı modeli
- analysis model container

## Çok önemli karar

```text
Node ID ≠ Array Index ≠ DOF ID ≠ Equation ID
```

## Testler

- duplicate ID
- connectivity
- invalid node reference
- constrained DOF numbering
- mixed-field numbering
- local frame orthogonality
- deterministic numbering

## Verification

Henüz gerçek continuum element yoktur; model/numbering doğrulaması yapılır.

---

# 19. V0.3.0 — Element Kernel / Shape Functions / Quadrature

**Durum (2026-08-29): Tamamlandi.** Uygulama ve patch-test detaylari `docs/architecture/V0.3.0_ARCHITECTURE.md` icindedir.

## Amaç

FEM'in element düzeyi matematik çekirdeğini oluşturmak.

## Kapsam

- reference element
- natural coordinates
- shape functions
- shape gradients
- isoparametric mapping
- Jacobian
- inverse Jacobian
- Gauss integration
- element registry
- element kernel interface
- element result container
- element quality/error checks

Başlangıç elemanları:

```text
BAR2 / TRUSS2
QUAD4 plane
QUAD4 axisymmetric prototype
HEX8 solid prototype
```

Beam/shell interface burada hazırlanır; tüm formulation'ların tamamlanması zorunlu değildir.

## Testler

- partition of unity
- Kronecker delta
- derivative sum
- Jacobian
- exact polynomial integration
- orientation/inverted element detection

## Patch Tests

İlk gerçek patch testleri bu sürümde başlar.

---

# 20. V0.4.0 — Sparse Assembly / macOS Linear Solver

**Durum (2026-08-29): Tamamlandi.** Uygulama, solver siniri ve verification detaylari `docs/architecture/V0.4.0_ARCHITECTURE.md` icindedir.

## Amaç

Element katkılarını büyük global sisteme taşıyıp çözmek.

## Kapsam

- DOF map
- sparsity graph
- sparse pattern
- element scatter
- residual vector assembly
- tangent/stiffness assembly
- mass assembly altyapısı
- essential BC treatment
- reaction recovery
- matrix property metadata
- Apple Accelerate adapter
- dense reference solver
- sparse direct solver
- sparse iterative solver interface

## Kritik şart

Assembly kodu Accelerate API'sini doğrudan çağırmaz.

```text
Assembly
  ↓
LinearSolver Interface
  ↓
Accelerate Backend
```

## Verification

- spring/bar assembled system
- symmetric structure check
- constrained solve
- reaction equilibrium

---

# 21. V0.5.0 — Linear Structural FEM + İlk Qt GUI

## Amaç

Lineer structural solver çekirdeğini gerçek material/section/formulation sistemiyle tamamlamak ve ilk macOS Qt kullanıcı arayüzünü solver'a stable C ABI üzerinden bağlamak.

## Tamamlanan core kapsamı

- linear isotropic elasticity,
- truss / plane / 2B beam sections,
- DOF-ID tabanlı nodal loads,
- TRUSS2,
- 2B Euler–Bernoulli beam,
- plane stress QUAD4,
- plane strain QUAD4,
- axisymmetric QUAD4,
- HEX8,
- stress/strain recovery,
- plane-strain out-of-plane `sigma_zz` recovery,
- von Mises helpers,
- generic linear-static analysis driver,
- reaction recovery.

## İlk GUI kapsamı

- New / Open / Save,
- model tree,
- material editor,
- section editor,
- load / BC editor,
- analysis panel,
- result table/tree,
- reaction display,
- solver log,
- optional VTK viewport,
- public C API üzerinden gerçek assembled TRUSS2 axial-bar solve preset'i.

GUI sınırı:

```text
Qt/C++ -> public C ABI -> Fortran engine
```

## V0.5 scope düzeltmesi

İlk roadmap taslağında primitive/structured model, mesh import ve basit 1D/2D/3D mesher V0.5'e fazla geniş biçimde yüklenmişti. Bunlar tam preprocessor/meshing altyapısının parçasıdır ve V0.12–V0.13 ile daha doğru hizalanır.

V0.5, **ilk arayüz ve solver entegrasyon sürümüdür**; arbitrary mesh/model editor tamamlandı iddiası taşımaz. Generic model-handle C API, ihtiyaç halinde V0.5.x/V0.6 entegrasyon hardening adımında geliştirilebilir.

## Verification

- plane-stress QUAD4 analytical/affine verification,
- 2B cantilever beam analytical verification,
- generic LinearStaticAnalysis TRUSS verification,
- plane-strain `sigma_zz` patch test,
- topology/formulation ve section-kind hata yolu testleri.

## Kullanıcı açısından

V0.5.0 kaynak paketi ilk çalıştırılabilir Qt GUI kaynaklarını verir. Native macOS Qt/VTK `.app` build sonucu ayrı CI release gate'idir.

---

# 22. V0.6.0 — Element Ailesi Genişletme + Modal

**Durum (2026-08-29): Core scope tamamlandı; native macOS Accelerate/Qt/VTK execution gate açık.**

## Amaç

Lineer structural kapsamı genişletmek ve modal solver eklemek.

## Uygulanan kapsam

- consistent mass matrix
- lumped mass seçenekleri
- generalized `Kφ = λMφ` eigenproblem
- dense reference backend
- gerçek ARPACK-NG `dsaupd/dseupd` backend
- macOS Accelerate/LAPACK `DSYGV` backend source
- assembled stiffness nullity tabanlı zero/rigid-mode detection
- `φᵀMφ=1` eigenvector normalization
- frequency extraction ve modal residual
- C API modal preset
- Qt mode listesi ve VTK mode animation

Element tarafı:

- TRUSS2, beam, plane/axisymmetric QUAD4 ve HEX8 mass formulations
- beam displacement + rotation multi-field modal desteği
- section/local-frame orientation metadata
- shell baseline **metadata/orientation contract**; shell stiffness/modal formulation henüz yok
- higher-order element için topology/formulation ayrımının korunması

## Verification

- `VER-V060-001`: iki TRUSS2 ayrık FE eigenvalue kapalı-form doğrulaması,
- `VER-V060-002`: 8-element cantilever beam birinci bending frequency analitik karşılaştırması,
- `VER-V060-003`: free-free axial rigid translation detection ve flexible eigenvalue,
- mass normalization,
- eigen residual,
- non-SPD mass / unknown backend / nonzero prescribed displacement hata yolları.

## Performans sınırı

Global `K/M` assembly sparse'dır. V0.6 eigensolver sınırında dense representation kullanılır; ARPACK backend de Cholesky ile standard probleme dönüştürülmüş operator üzerinde çalışır. Production-scale sparse shift-invert ve factorization reuse sonraki hardening kapsamıdır.

---

# 23. V0.7.0 — Finite Strain / Geometric Nonlinearity

## Amaç

Nonlinear continuum kinematiğini kurmak.

## Kapsam

- reference/current configuration
- deformation gradient
- determinant `J`
- Green-Lagrange strain
- finite rotation safe kinematics
- Total Lagrangian solid formulation
- material tangent contribution
- geometric stiffness
- consistent element tangent
- follower-load altyapısı

## V0.7.0 uygulama durumu

**Tamamlandı.** `TOTAL_LAGRANGIAN_HEX8`, global nonlinear residual/tangent evaluator, trial/commit/revert state ve objectivity/consistent-tangent doğrulamaları kaynak kodunda mevcuttur. Follower-load maddesi V0.7 kapsamında configuration/external-tangent metadata sözleşmesi olarak kapatılmıştır; gerçek surface pressure/traction integrasyonu sonraki nonlinear load çalışmalarına bırakılmıştır. StVK yalnız geometrik nonlinearity için referans constitutive modeldir; büyük-strain elastomer malzemesi olarak kullanılmaz.

## Kritik testler

- pure rigid rotation → yapay strain oluşmamalı
- homogeneous deformation
- finite stretch
- tangent finite-difference check

Bu sürümde tam Newton load-step solver henüz ana özellik değildir.

---

# 24. V0.8.0 — Nonlinear Solver

## Amaç

Gerçek nonlinear static analiz çalıştırmak.

## Kapsam

- Newton-Raphson
- modified Newton seçeneği
- residual convergence
- displacement correction convergence
- energy convergence altyapısı
- line search
- load stepping
- adaptive stepping
- step retry
- rollback
- commit
- solver monitor
- convergence history
- checkpoint/restart başlangıcı

## GUI

```text
Analysis Type: Nonlinear Static
Large Displacement: Enabled
Initial Increment
Min/Max Increment
Max Newton Iterations
Line Search
Adaptive Stepping
```

## V0.8.0 uygulama durumu

**Source-level kapsam tamamlandı.** Full/modified Newton, üç convergence criterion altyapısı, backtracking line search, load stepping, adaptive growth/cutback, retry, rollback/commit, convergence history ve in-memory checkpoint continuation uygulanmıştır. C API nonlinear HEX8 preset'i solver history'sini Qt'ye taşır; Qt tarafında Nonlinear Static / Large Displacement paneli ve convergence tablosu vardır.

Kapsam sınırları: V0.8 load-control baseline nonzero prescribed-displacement stepping, arc-length/Riks, gerçek follower surface-pressure integrasyonu, hyperelastic/plastic constitutive modeller, contact veya mixed `u-p` içermez. GUI arbitrary nonlinear mesh/model preprocessor değildir.

## Kullanıcı açısından

**İlk gerçek nonlinear GUI kaynak sürümüdür.** Native macOS/arm64 Qt/VTK execution sonucu CI release gate'idir.

---

# 25. V0.9.0 — Hyperelastic + Plastic Constitutive Models

**Durum (2026-08-29): Source-level kapsam tamamlandı.** Dört hyperelastic model ortak constitutive response üzerinden TL-HEX8 ve V0.8 Newton solver'a bağlanmıştır. J2 small-strain plasticity material-point/state/tangent baseline olarak tamamlanmıştır; global plastic element entegrasyonu bu sürümün kapsam iddiası değildir. Penalty volumetric enerji mixed `u-p` değildir; incompressibility/locking çözümü V0.10 kapsamındadır.

## Amaç

Nonlinear constitutive material library oluşturmak.

## Hyperelastic

- Neo-Hookean
- Mooney-Rivlin
- Yeoh
- Ogden

## Material architecture

- strain-energy density
- isochoric contribution
- volumetric contribution
- stress
- consistent tangent

## Plasticity

İlk hedef:

- J2 von Mises
- isotropic hardening
- return mapping
- consistent algorithmic tangent

Daha gelişmiş modeller sonraki sürümlere bırakılabilir.

## GUI Material Studio

- model selection
- parameter editor
- unit display
- validation
- material curve preview
- parameter consistency check

## Verification

Hyperelastic homogeneous tests:

- uniaxial tension
- planar tension
- equibiaxial tension
- simple shear

Plasticity:

- single material-point loading/unloading
- yield onset
- hardening
- return-map regression

---

# 26. V0.10.0 — Mixed `u-p` / Incompressibility

## Amaç

Nearly incompressible elastomer problemleri için displacement-pressure alanlarını,
blok tangent yapısını ve locking'e dayanıklı formulation altyapısını tek başına
doğrulamak. Contact bu sürümde özellikle kapsam dışı tutulur; böylece mixed
formulation hataları ile contact-search/constraint hataları birbirine karışmaz.

## Kapsam — V0.10 kapanış durumu

- element-associated `pressure_p0` field/DOF: **tamamlandı**
- HEX8/Q1 displacement + P0 pressure mixed interpolation: **tamamlandı**
- perturbed-Lagrangian mixed residual `R_u/R_p`: **tamamlandı**
- block tangent:

\[
\begin{bmatrix}
K_{uu} & K_{up}\\
K_{pu} & K_{pp}
\end{bmatrix}
\]

- pressure result data path: **tamamlandı**
- volumetric locking benchmark: **tamamlandı**
- symmetric-indefinite solver requirement / CG rejection: **tamamlandı**
- stabilized equal-order Q1/Q1 veya higher-order stable pairs: **V0.10 baseline kapsamı dışında**
- contact: **V0.11**

## Verification

- `VER-V100-001`: local 25x25 mixed tangent FD kontrolü
- `VER-V100-002`: global sparse mixed tangent FD kontrolü
- `VER-V100-003`: `J=1` manufactured simple-shear coupled Newton
- `VER-V100-004`: penalty-only Q1 vs mixed Q1/P0 nearly-incompressible locking benchmark
- ayrıca mixed DOF map, pressure recovery ve error-path unit testleri

## Kullanıcı açısından

**Nearly incompressible rubber için ilk ciddi mühendislik sürümüdür.**

---

# 27. V0.11.0 — Contact / Friction

## Amaç

Contact search, constraint enforcement ve friction state yönetimini mixed `u-p`
formulation'dan bağımsız bir subsystem olarak geliştirip Newton/cutback zincirine
bağlamak.

## V0.11 kapanış kapsamı

Baseline temas çifti:

- deformable **slave node** tarafı,
- rigid planar **QUAD4 master facet** tarafı,
- master facet node sırası ile tanımlanan normal,
- signed gap convention: `g_n < 0` penetrasyon.

Search:

- expanded AABB broad phase,
- QUAD4'ün iki üçgene ayrılmasıyla closest-point narrow phase,
- nearest eligible rigid facet seçimi.

Enforcement:

- penalty normal contact,
- committed-gap incremental augmented Lagrangian,
- Coulomb friction,
- tangential penalty predictor,
- stick/slip active set,
- analytic/consistent contact tangent.

State ve nonlinear solution:

- contact-point committed/trial master facet, gap, normal multiplier, tangential traction ve position,
- line-search candidate değerlendirmelerinde history accumulation yok,
- converged step `commit`, failed/cutback step `revert`,
- global contact force ve tangent sparse nonlinear sisteme assemble edilir,
- friction/contact sistemi için CG reddedilir; direct/reference backend baseline kullanılır,
- contact history restart snapshot formatı gelene kadar contact + checkpoint restart açıkça reddedilir.

## Verification

- `VER-V110-001`: stick/slip analytic tangent ↔ central finite difference,
- `VER-V110-002`: augmented-Lagrangian multiplier, committed-gap increment ve aynı-state re-evaluation invariance,
- `VER-V110-003`: TL-HEX8 + rigid plane, 1000 N global contact equilibrium,
- `VER-V110-004`: zorla başarısız load step'te multiplier/history rollback,
- `VER-V110-005`: global Coulomb friction, normal equilibrium ve `||T|| <= μN`.

## GUI / C ABI

- `fem_demo_contact_hex8(...)` gerçek nonlinear contact preset'i,
- Qt nonlinear panelinde contact verification formulation seçimi,
- penalty / augmented-Lagrangian seçimi,
- penetration / normal contact force / active contact / Newton correction sonucu,
- VTK kaynaklarında rigid plane + deformed HEX8 contact görünümü.

GUI baseline frictionless compression preset'idir. Global Coulomb friction core verification
ile kapatılmıştır; arbitrary contact-pair preprocessor bu sürümün kapsamı değildir.

## Bilinçli sınırlar

V0.11 aşağıdakileri tamamlanmış saymaz:

- deformable-deformable contact,
- segment-to-segment / mortar formulation,
- self-contact,
- BVH hierarchy veya production-scale parallel search,
- curved master surface / higher-order facet,
- arbitrary surface extraction ve contact meshing,
- nodal/face contact-pressure contour extrapolation,
- disk tabanlı contact-state restart formatı.

## Kullanıcı açısından

**Rubber + contact/friction için ilk çalışan nonlinear çekirdek baseline'ıdır.**
Nearly-incompressible mixed `u-p` V0.10 ve contact/friction V0.11 ayrı subsystem olarak
korunur; birleşik production pre/post workflow V0.13 hardening'ine kadar genişletilecektir.

# 28. V0.12.0 — CAD / Geometry / Sections ✅

## Amaç

Solver'dan ayrı bir CAE geometri katmanı kurmak ve geometry/visualization/FEM mesh
ayrımını gerçek uygulama kodunda hayata geçirmek.

## Tamamlanan kaynak kapsamı

- Ayrı C++20 `femcae_geometry` library.
- `GeometryDocument`: assembly/body/face/edge/vertex hierarchy.
- 64-bit deterministic geometry ID ve document namespace/persistent-key modeli.
- Geometry/FEM provenance association map.
- `GeometryTessellation`: yalnız display point/triangle verisi; FEM mesh değildir.
- Optional OpenCASCADE XDE/STEPCAF STEP adapter.
- `FEMCAE_REQUIRE_OCCT` native release gate.
- OCCT B-Rep -> display tessellation yolu.
- Qt GeometryPanel + geometry tree + body/face/edge/vertex selection filter.
- ASCII DXF custom-section import: LWPOLYLINE, LINE, CIRCLE, ARC.
- Closed contour validation ve outer/hole nesting classification.
- Section area, centroid, Ixx/Iyy/Ixy, principal moments/axes ve polar area moment.
- STEP/DXF source path project persistence.

## Verification

Portable V0.12 candidate 112/112 test geçer. Yeni gate'ler:

- deterministic persistent geometry IDs,
- CAD != tessellation != FEM mesh contract,
- analytic rectangle section,
- analytic hollow rectangle section,
- DXF closed/open contour,
- DXF LINE/CIRCLE/ARC paths,
- OCCT adapter availability contract.

OpenCASCADE bulunan native build ayrıca generated STEP box import+tessellation verification çalıştırır.

## Kritik gate

OCCT display tessellation hiçbir koşulda doğrudan FEM mesh kabul edilmez.
Geometry entity ID ile FEM mesh entity ID ayrı namespace/ownership modelinde kalır.

## Bilinçli sınırlar

- Persistent ID sistemi CAD edit sonrası tam semantic/topological naming çözümü değildir.
- Nested assembly semantics baseline root/body seviyesindedir.
- LWPOLYLINE bulge desteklenmez.
- Polar area moment `Jp`, genel Saint-Venant torsion constant değildir.
- IGES, geometry healing UI ve CAD-to-mesher production integration tamamlanmış sayılmaz.

# 29. V0.13.0 — Meshing + Full Pre/Post Integration ✅

## Amaç

Geometri, FEM mesh, analiz ayarları ve sonuç görüntülemeyi tek CAE workflow baseline'ında birleştirmek.

## Tamamlanan baseline

- ayrı `femcae_meshing` C++20 library,
- structured axis-aligned HEX8 volume meshing,
- Abaqus ASCII NODE/C3D8 external mesh import,
- center scaled-Jacobian + aspect-ratio quality,
- global ve CAD-face local structured sizing,
- CAD body/face provenance ve `GeometryAssociationMap` bridge,
- material/section/load/constraint/contact assignment metadata,
- boundary-facet provenance üzerinden geometry assignment resolution,
- generic linear HEX8 C ABI solve,
- displacement/reaction/von-Mises result path,
- probes, plane-cut element selection, CSV ve legacy VTK export,
- Qt `Mesh / Pre-Post` source paneli ve VTK deformed/von-Mises view,
- native OCCT axis-aligned STEP box -> HEX8 provenance verification source.

## Doğrulama

Portable final snapshot:

```text
Debug   118/118 PASS
Release 118/118 PASS
```

`VER-V130-001`, structured mesher çıktısını generic C ABI üzerinden gerçek Fortran sparse assembly/linear solver/stress recovery yoluna verir ve `u=FL/EA`, reaction equilibrium ve von Mises'i doğrular. Native `VER-V130-002` macOS/OCCT gate'inde STEP box geometry face ID'lerinin mesh boundary provenance'a taşınmasını kontrol eder.

## Scope sınırı

V0.13 production-grade arbitrary curved CAD tetra/hex mesher, adaptive error-estimator refinement, full Abaqus model import veya interpolated cut-surface post-processing iddiası değildir. Production hardening ve release engineering V1.0 kapsamıdır.

## Kullanıcı açısından

**İlk uçtan uca mesh -> solve -> post-processing CAE workflow baseline'ıdır.**

---

# 30. V1.0.0 — Verified Engineering Release ✅ Portable Source Candidate

## Amaç

Yeni solver özelliği eklemekten çok V0.x boyunca geliştirilen zinciri verification, failure handling, restart ve macOS release engineering açısından sertleştirmek.

## Portable kapanış

```text
Debug   123/123 PASS
Release 123/123 PASS
V1 ASan+UBSan release gates 5/5 PASS
```

Kapanan release gate'leri:

- bütün mevcut unit/regression/patch/verification matrisi,
- independent HEX8 cantilever mesh-convergence study,
- versioned/checksum-protected disk checkpoint/restart,
- corrupted Abaqus/DXF/checkpoint input rejection,
- public C API invalid-input failure handling,
- deterministic performance smoke,
- installed CLI/C API/C++ consumer tests,
- source compiler warning gate = 0,
- Apache-2.0 LICENSE/NOTICE ve third-party inventory,
- macOS deployment/audit/sign/notarize script infrastructure.

## Convergence verification

`VER-V1000-001` bağımsız Euler-Bernoulli cantilever referansına karşı:

```text
4x1x1    error 33.6192 %
8x2x2    error 10.8368 %
12x3x3   error  3.93645 %
```

Hata monoton azalır ve fine-mesh hata %5'in altındadır.

## Restart verification

`VER-V1000-002`, %25 load factor'da checkpoint alır, disk round-trip sonrası aynı converged state'ten devam eder ve final finite-stretch sonucu uninterrupted solution ile eşleştirir. Truncated ve checksum-corrupted checkpoint'ler reddedilir.

## macOS release engineering

Kaynakta:

- Qt deploy script,
- `BundleUtilities::fixup_bundle()`,
- app-local Frameworks RPATH,
- Mach-O arm64/dependency audit,
- ad-hoc CI codesign,
- Developer ID + Hardened Runtime + `notarytool` + stapler script altyapısı

bulunur.

## Açık native release gate'leri

Bu Linux hostta aşağıdakiler PASS sayılmaz:

- native macOS arm64 full CI execution,
- real Qt/VTK/OCCT/Accelerate `.app` run,
- final `otool` dependency closure,
- GUI project migration checks,
- Apple Instruments/Leaks native audit,
- actual binary third-party license bundle,
- real Developer ID signing/notarization/Gatekeeper assessment.

Bu nedenle V1.0 source/portable engineering candidate tamamlanmış, **signed/notarized native macOS distribution gate'i açık** olarak raporlanır.

## Kullanıcı açısından

**İlk 1.0 engineering source candidate.** Native binary release checklist tamamlandıktan sonra dağıtım artifact'ı ayrıca kapatılmalıdır.

---

# 31. V1.0 Sonrası Planlanan Ana Özellikler

V1.0 kapsamını büyütmemek için aşağıdakiler V1.x'e bırakılabilir:

```text
Harmonic response
Transient dynamics
Newmark / generalized-alpha
Nonlinear transient
Damping models
Buckling
Thermal field
Thermo-mechanical coupling
Advanced shell formulations
Advanced beam warping
Cohesive elements
Damage
Viscoelasticity
Advanced plasticity
Parallel assembly
GPU exploration
```

Bu özellikler için çekirdek mimari V1.0 öncesinde kapıyı kapatmamalıdır; ancak implementasyonları V1.0 release gate'ine dahil edilmemelidir.

---

# 32. Her Sürümde Zorunlu Geliştirme Döngüsü

Her sürüm:

1. Matematiksel teori
2. Kaynak/literatür listesi
3. Architecture Decision Record
4. Kod
5. Ayrıntılı Türkçe mühendislik/matematik açıklamaları
6. Unit test
7. Element patch test — uygunsa
8. FEM verification problemi
9. Convergence test — uygunsa
10. Regression kontrolü
11. Numerical review
12. Source/reference provenance review
13. CHANGELOG
14. Git commit
15. Pull Request
16. macOS build/test
17. ZIP release

adımlarından geçecektir.

---

# 33. ZIP Teslim Standardı

V0.1–V0.4:

```text
Project-v0.x.0-source-macos-arm64.zip
```

V0.5 ve sonrası:

```text
Project-v0.x.0-source-macos-arm64.zip
Project-v0.x.0-app-macos-arm64.zip
```

Kaynak ZIP:

- source
- tests
- docs
- CMake
- CHANGELOG
- license/third-party notices
- build instructions

App ZIP:

- `.app`
- runtime dependencies
- release notes
- third-party notices

içerecektir.

---

# 34. Kod Açıklama Standardı

Identifier'lar İngilizce olacaktır:

```fortran
compute_internal_force
assemble_tangent
deformation_gradient
material_state
gauss_point
```

Mühendislik açıklamaları Türkçe olacaktır:

```fortran
! ---------------------------------------------------------------------------
! Bu rutin elemanın iç kuvvet vektörünü hesaplar.
!
! Küçük şekil değiştirme durumunda iç kuvvet:
!
!     f_int = ∫ B^T sigma dV
!
! ifadesinden elde edilir.
!
! Buradaki B matrisi kullanılan element formülasyonuna bağlıdır.
! ---------------------------------------------------------------------------
```

Formül açıklamalarında:

- kullanılan stress/strain measure,
- koordinat sistemi,
- unit assumption,
- sign convention,
- referans kaynak

belirtilmelidir.

---

# 35. Unit Politikası

Solver içinde gizli:

```text
mm → m
MPa → Pa
```

dönüşümü olmayacak.

Core yalnızca **tutarlı birim sistemi** varsayacaktır.

GUI:

- unit metadata,
- input display conversion,
- unit consistency check

yapacaktır.

Örneğin kullanıcı:

```text
N – mm – MPa
```

sistemini tutarlı kullandığında solver bunu doğal olarak çözebilir.

---

# 36. License / Source Independence Politikası

## Code_Aster

- kaynak kod kopyalanmayacak
- subroutine/function satır satır port edilmeyecek
- yorumlar çevrilmeyecek
- test input dosyaları kopyalanmayacak
- reference output kopyalanmayacak
- özel veri yapı isimleri ve dispatch mekanizmaları bire bir taklit edilmeyecek

Kullanılabilecek şey:

- yüksek seviye mimari fikir
- genel FEM yöntemleri
- literatürde kamuya açık matematiksel formülasyonlar

## FEM matematik kaynakları

Ana kaynak sınıfları:

- FEM textbooks
- nonlinear continuum mechanics textbooks
- original constitutive-model papers
- peer-reviewed articles
- public standards
- independent analytical derivations

Her formulation için `docs/theory/...` altında derivation dokümanı oluşturulacaktır.

---

# 37. Başlıca Riskler ve Kontroller

| Risk | Sonuç | Kontrol |
|---|---|---|
| DOF'ları yalnızca displacement olarak tasarlamak | Mixed/contact/shell refactor | V0.2 field registry |
| Node ID = equation ID kabul etmek | constraint ve restart kırılması | Ayrı ID/numbering |
| Yalnızca symmetric matrix kabul etmek | contact/mixed solver problemi | matrix property metadata |
| Malzeme state'ini doğrudan overwrite etmek | Newton rollback hatası | trial/commit |
| CAD triangulation = FE mesh kullanmak | yanlış analiz modeli | ayrı mesh katmanı |
| Gauss stress = nodal stress gibi göstermek | yanlış sonuç yorumlama | result location metadata |
| GUI'yi solver data structure'a bağlamak | GUI değişiminde solver kırılması | C API |
| Qt'nin rastgele modüllerini kullanmak | lisans riski | module whitelist |
| Gmsh'i erken hard dependency yapmak | lisans/mimari kilitlenme | MesherBackend |
| PETSc'i ilk günden zorunlu yapmak | macOS packaging karmaşıklığı | Accelerate primary backend |
| Shell'i basit solid türevi gibi görmek | hatalı shell formulation | ayrı element family |
| Finite strain stress measure'ı belirsiz bırakmak | tangent/stress hataları | explicit measure metadata |
| Regression'ı verification sanmak | yanlış güven | ayrı verification/convergence |
| Project schema versioning yapmamak | eski proje dosyaları açılamaz | bağımsız schema version |
| Checkpoint'i sonradan eklemek | state serialization refactor | AnalysisState erken tasarım |
| Per-Gauss dynamic polymorphism | performans kaybı | block/batch dispatch |

---

# 38. Mimari Dondurma Kararları

Aşağıdaki kararların V0.1.0 sonrası değiştirilmesi çok dikkatli ADR ve migration gerektirmelidir:

1. Residual sign convention
2. Tensor/Voigt ordering
3. Engineering shear convention
4. Persistent ID semantics
5. Field/DOF model
6. Trial/commit material state model
7. Stress-measure metadata
8. Solver C API ownership rules
9. Matrix symmetry classification
10. Unit policy
11. Project schema versioning policy
12. CAD mesh / visualization mesh / FEM mesh separation
13. Core dependency direction
14. Source-independence policy

---

# 39. Son Değerlendirme

Revize edilen yapı ile yol haritası teknik olarak tutarlıdır.

Özellikle aşağıdaki noktalar gelecekteki büyük sorunların önünü kesmektedir:

- mixed `u-p` gelmeden field/DOF altyapısının hazırlanması,
- plasticity gelmeden trial/commit state sisteminin hazırlanması,
- contact gelmeden unsymmetric sparse matrix desteğinin düşünülmesi,
- GUI gelmeden solver C API'sinin tanımlanması,
- CAD gelmeden geometry/mesh ayrımının tanımlanması,
- beam/shell gelmeden rotational DOF ve local frame sisteminin hazırlanması,
- nonlinear analiz gelmeden restart edilebilir AnalysisState modelinin düşünülmesi,
- sonuç ekranı gelmeden Gauss/nodal result semantiğinin tanımlanması,
- macOS-only kararından yararlanarak Apple Accelerate'in ilk native solver backend'i yapılması.

Bu nedenle bundan sonraki geliştirmede temel roadmap:

```text
V0.1  Foundation
  ↓
V0.2  Model / Mesh / Field / DOF
  ↓
V0.3  Element / Quadrature
  ↓
V0.4  Sparse Assembly / Accelerate
  ↓
V0.5  Linear FEM + First Qt GUI
  ↓
V0.6  Modal + Element Family Expansion
  ↓
V0.7  Finite Strain
  ↓
V0.8  Nonlinear Solver
  ↓
V0.9  Hyperelastic / Plasticity
  ↓
V0.10 Mixed u-p
  ↓
V0.11 Contact / Friction
  ↓
V0.12 CAD / Geometry / Sections
  ↓
V0.13 Meshing / Full Pre-Post
  ↓
V1.0  Verified Engineering Release
```

olarak kullanılmalıdır.

---

# 40. Teknik Bağımlılık Doğrulama Notları — 28 Ağustos 2026

Aşağıdaki bilgiler güncel resmi/ana kaynaklardan kontrol edilmiştir:

- Qt 6.11 dokümantasyonu macOS üzerinde `arm64` mimarisini desteklenen desktop target olarak listeler.
- Qt'nin açık kaynak sürümünde LGPLv3/GPL yükümlülükleri vardır; bazı modüller yalnızca GPL altında olabilir.
- `macdeployqt`, Qt uygulamalarının deploy edilebilir macOS `.app` bundle'ı hazırlanmasına yardımcı olur.
- Apple Accelerate; BLAS, LAPACK ve sparse solver kütüphaneleri içerir. Sparse katmanı symmetric ve unsymmetric sistemleri temsil edebilir.
- PETSc resmi kurulum dokümantasyonu macOS geliştirmesini ve Homebrew üzerinden `gfortran` kullanımını destekler.
- VTK BSD-3-Clause lisanslıdır ve Qt ile gömülü native OpenGL widget entegrasyonu sunar.
- OCCT macOS üzerinde kullanılabilir ve LGPL 2.1 + ek istisna ile dağıtılır.
- Gmsh GPL altında dağıtılır; bu nedenle proje lisansı kararı verilmeden hard dependency yapılmamalıdır.
- ARPACK-NG büyük sparse eigenvalue problemleri için güncel bir ARPACK devam projesidir; BLAS/LAPACK ile çalışır ve CMake/ISO_C_BINDING desteği bulunur.

## Referans bağlantılar

- Qt Supported Platforms: https://doc.qt.io/qt-6/supported-platforms.html
- Qt Licensing: https://doc.qt.io/qt-6/licensing.html
- Qt macOS Deployment: https://doc.qt.io/qt-6/macos-deployment.html
- Apple Accelerate: https://developer.apple.com/accelerate/
- Apple Sparse Solvers: https://developer.apple.com/documentation/accelerate/sparse-solvers-library
- PETSc Install: https://petsc.org/main/install/
- VTK About / License: https://docs.vtk.org/en/latest/about.html
- VTK Qt Widget: https://vtk.org/doc/nightly/html/QVTKOpenGLNativeWidget_8h_source.html
- OCCT Documentation: https://dev.opencascade.org/doc/overview/html/
- OCCT License: https://dev.opencascade.org/doc/occt-7.7.0/overview/html/occt_public_license.html
- Gmsh: https://gmsh.info/
- ARPACK-NG: https://github.com/opencollab/arpack-ng

---

**V0.10.0 mixed incompressibility source scope tamamlanmıştır: V0.9 isochoric hyperelastic response HEX8/Q1-P0 displacement-pressure formulation'a bağlanmış, local/global consistent block tangent finite-difference ile doğrulanmış, mixed Newton `J=1` simple shear çözümü ve penalty-vs-mixed locking benchmark'ı geçmiştir. Element pressure result data path ve GUI/C ABI verification başlangıcı vardır. Q1/P0 baseline arbitrary meshler için evrensel inf-sup stability iddiası değildir; stabilized/higher-order mixed element hardening ve contact sonraki sürümlere bırakılmıştır. Native macOS/arm64 Accelerate ve Qt/VTK execution CI release gate olarak açık kalır.**


**V0.11.0 contact/friction source scope tamamlanmıştır: rigid planar QUAD4 master + deformable slave-node baseline, AABB/closest-point search, penalty ve incremental augmented-Lagrangian normal enforcement, Coulomb stick/slip friction, analytic contact tangent, trial/commit/revert rollback, global Newton equilibrium ve global Coulomb verification geçmiştir. Contact subsystem mixed `u-p` ve constitutive element kodundan ayrı tutulmuştur. Deformable-deformable/mortar/self-contact, BVH hierarchy ve contact-state disk restart bu release'in kapsam iddiası değildir; native macOS/arm64 Qt/VTK execution CI gate olarak açık kalır.**
