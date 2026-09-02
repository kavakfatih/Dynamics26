# Nonlinear CAE Reference Study — ANSYS / Marc / COMSOL / Code_Aster

**Tarih:** 2026-09  
**Amaç:** Dynamics26 nonlinear analysis setup, solver integration, rubber mechanics ve extension architecture öncesi research gate.

Bu çalışma rakip ürünlerin kaynak kodunu veya görsel tasarımını kopyalamak için değildir. Hedef kullanıcı akışlarını, engineering semantics'i, solver kontrol yapılarını, material-model yaklaşımını ve extensibility sınırlarını karşılaştırarak Dynamics26 için `Adopt / Adapt / Reject` kararları üretmektir.

# 1. Executive findings

## 1.1 ANSYS Mechanical

ANSYS Mechanical'ın güçlü tarafı engineering object workflow ve scoping açıklığıdır.

Resmî Mechanical Force dokümantasyonunda Force;

- vertex,
- edge,
- face,
- node,
- element face

seviyelerine uygulanabilmektedir. Face scope'ta kuvvet seçilen yüzeyler üzerinde uniform traction olarak dağıtılır. Scope, doğrudan Geometry Selection veya Named Selection ile tanımlanabilir.

ANSYS nonlinear controls, Static Structural için Program Controlled / Full / Modified / Unsymmetric Newton seçeneklerini ayırır. Mechanical APDL tarafında line search, equilibrium iteration limitleri, automatic stepping ve nonlinear diagnostics ayrı kontrollerdir.

Hyperelastic material fitting dokümantasyonu uniaxial, biaxial, pure shear/simple shear ve volumetric test ailelerini; Neo-Hookean, Mooney-Rivlin, Yeoh, Ogden vb. modelleri açık biçimde ayırır.

### Dynamics26 kararı

**ADAPT**

- object-tree based analysis setup,
- Geometry Selection / Named Selection dual scoping,
- explicit Force vs Pressure semantics,
- Basic / Advanced nonlinear controls,
- convergence controls separate from material models.

Birebir ribbon/tree veya ikon kopyalanmaz.

### Resmî kaynaklar

- ANSYS Mechanical 2026 R1 Force: https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/wb_sim/ds_Force_Load.html
- Nonlinear Controls: https://ansyshelp.ansys.com/public/Views/Secured/corp/v251/en/wb_sim/ds_nl_static_transient.html
- Newton-Raphson theory 2026 R1: https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/ans_thry/thy_tool10.html
- NROPT: https://ansyshelp.ansys.com/public/Views/Secured/corp/v252/en/ans_cmd/Hlp_C_NROPT.html
- Hyperelastic Material Reference 2026 R1: https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/pdf/ANSYS_Mechanical_APDL_Material_Reference.pdf
- ACT customization overview: https://ansyshelp.ansys.com/public/Views/Secured/corp/v242/en/act_dev/act_dev_Overview.html

# 2. COMSOL Multiphysics

COMSOL'un güçlü tarafı physics-aware feature tree ve boundary/domain selection modelidir.

Solid Mechanics Boundary Load için COMSOL 6.4:

- Force per reference area,
- Force per deformed area,
- Total force,
- Pressure,
- Resultant

seçeneklerini ayırır. Total Force seçili boundary alanına dağıtılır. Pressure geometrically nonlinear durumda current surface normal/area semantiğine genişleyebilir.

Results katmanında plot node'larına ayrı Selection attribute eklenebilir. Derived Values global/point/integral sonuçları ve reaction force gibi nicelikleri tablolaştırabilir.

Hyperelastic Material arayüzü compressible, nearly incompressible ve incompressible seçeneklerini ayırır; mixed formulation ve volumetric strain-energy tanımları material modelin parçası olarak görünürdür.

COMSOL add-in/method sistemi Model Builder'a methods/settings forms/ribbon extensions ekleyebilir. Bu, UI/workflow extension ile physics modelin aynı extension host tarafından yönetilebilmesi açısından önemli bir referanstır.

### Dynamics26 kararı

**ADAPT**

- boundary/domain-aware selection,
- explicit reference/deformed load semantics,
- result object scope,
- material compressibility seçeneklerinin açık modellenmesi,
- extension manifest + workflow methods fikri.

Dynamics26'ta Java/Application Builder kopyalanmaz; C++/Qt tabanlı özgün extension host tasarlanır.

### Resmî kaynaklar

- COMSOL 6.4 Boundary Load: https://doc.comsol.com/6.4/doc/com.comsol.help.sme/sme_ug_solid.07.076.html
- Hyperelastic Material: https://doc.comsol.com/6.4/doc/com.comsol.help.sme/sme_ug_solid.07.010.html
- Hyperelastic theory: https://doc.comsol.com/6.4/doc/com.comsol.help.sme/sme_ug_theory.06.031.html
- Results Selection: https://doc.comsol.com/6.4/doc/com.comsol.help.comsol/comsol_ref_results.37.216.html
- Derived Values: https://doc.comsol.com/6.4/doc/com.comsol.help.comsol/comsol_ref_results.37.088.html
- Model/Application Methods: https://doc.comsol.com/6.4/doc/com.comsol.help.comsol/application_programming_guide.15.10.html

# 3. Hexagon Marc / Mentat

Marc güçlü nonlinear mechanics, contact ve elastomer/material modeling referansıdır.

Marc 2026.1 ürün duyurusu contact handling ve fixed time stepping convergence iyileştirmeleri ile Mentat'ta meshing/post-processing ve physical material test data kullanım geliştirmelerini vurgular.

Marc material documentation nonlinear elastic/hyperelastic material ailelerini ve user subroutines'i ayırır. Marc ekosisteminde HYPELA2/UELASTOMER gibi user subroutine mekanizmaları custom constitutive davranış için önemli bir extensibility pattern'dir.

Elastomer kullanımında Marc/Herrmann formulation ilişkisi ve incompressibility handling özellikle Dynamics26 rubber roadmap'i için incelenmelidir. Ancak element/formulation seçimi rakip davranışına bakılarak değil, bağımsız benchmark ve akademik teoriyle yapılmalıdır.

### Dynamics26 kararı

**BACKEND / RUBBER REFERENCE**

Marc'tan özellikle şu alanlarda teknik benchmark davranışı araştırılır:

- large strain robustness,
- contact,
- hyperelastic material models,
- nearly incompressible formulation,
- physical material-test driven fitting,
- adaptive stepping/convergence,
- user material extension boundary.

### Resmî kaynaklar

- Marc product / 2026.1: https://nexus.hexagon.com/home/product/marc/
- Marc Volume C Program Input 2024.2: https://documentation-be.hexagon.com/bundle/Marc_2024.2-Volume_C_Program_Input/raw/resource/enus/Marc_2024.2-Volume_C_Program_Input.pdf

# 4. Code_Aster

Code_Aster Dynamics26 için GUI örneğinden çok backend architecture ve verification referansıdır.

Code_Aster command/operator ayrımı şu separation of concerns fikrini güçlendirir:

```text
Model
Material Definition / Assignment
Mechanical Loads / Boundary Conditions
Nonlinear Analysis Command
Solver Controls
Post-processing
```

`AFFE_CHAR_MECA` mechanical load/BC tanımı, `STAT_NON_LINE` nonlinear solve, `DEFI_MATERIAU` material properties/constitutive data ve ayrı solver/behavior dokümanları bu ayrımın örnekleridir.

Mesh/node/element groups, engineering scope'un solver input'a açık şekilde aktarılması açısından faydalı bir referanstır. Dynamics26'ta bu yaklaşım CAD/FEM provenance + Named Selection ile daha GUI-native biçimde uygulanmalıdır.

Code_Aster ayrıca MFront/UMAT coupling belgelerine sahiptir. MFront'ın constitutive law tanımını solver core'dan ayırması Dynamics26 future MaterialModelExtension için güçlü bir architecture study konusudur.

### Dynamics26 kararı

**ARCHITECTURE / VERIFICATION REFERENCE**

- operator separation,
- explicit load/material/solver input boundaries,
- nonlinear verification documentation,
- MFront-style constitutive interface fikri

araştırılır.

Code_Aster source code'u kopyalanmaz veya transliterate edilmez. Matematik akademik/orijinal kaynaklarla yeniden türetilir.

### Resmî kaynaklar

- Documentation index: https://code-aster.org/doc/default/en/index.php
- Code_Aster v17 DEFI_MATERIAU: https://code-aster.org/doc/v17/manuals/man_u/u4/u4.43.01/index.html
- MFront coupling: https://code-aster.org/doc/v17/manuals/man_u/u2/u2.10.02/index.html
- Validation document index: https://code-aster.org/doc/v17/manuals/man_v/other_pages/test_cases/index.html

# 5. Comparative matrix

| Area | ANSYS | Marc | COMSOL | Code_Aster | Dynamics26 decision |
|---|---|---|---|---|---|
| Setup tree | Very strong engineering-object tree | Mentat pre/post workflow | Physics/feature tree | Command/operator workflow | ANSYS + COMSOL pattern adapted |
| Selection / scope | Geometry + Named Selection | sets/groups | boundary/domain selections | mesh groups | geometry IDs + Named Selection + mesh provenance |
| Force | face/edge/etc.; geometry-scoped | strong loadcase mechanics | total force / traction / pressure | FORCE_FACE etc. | explicit Total Force / Traction / Pressure |
| Load distribution | distributed across scoped topology | solver/loadcase dependent | total force divided over selected boundaries | FE load definitions | consistent face integration; glyph independent |
| Nonlinear controls | Full/Modified NR, line search, stepping | robust nonlinear controls | Automatic/Newton solver methods | STAT_NON_LINE + NEWTON/CONVERGENCE | Basic/Advanced controls mapped to real consumer |
| Hyperelastic | broad models + fitting | major strength | broad models + compressibility modes | nonlinear behaviors/material definitions | own verified constitutive framework |
| Incompressibility | element/material options | Herrmann etc. strong reference | mixed formulation / incompressible variants | formulation/behavior combinations | comparative formulation research |
| Contact | mature | major strength | mature multiphysics contact | DEFI_CONTACT/nonlinear | later product consumer after workflow |
| Results | object tree + contours/probes | postprocessor | plot/result nodes + derived values | result fields/post operators | result objects + selection + probe |
| Extensions | ACT | user subroutines | methods/add-ins/API | MFront/UMAT/source extensibility | versioned C++ plugin host + stable solver/material adapters |

# 6. Immediate Dynamics26 UX adoption

## 6.1 Scope pattern

Adopt:

```text
Scope Method
- Geometry Selection
- Named Selection
```

Do not force users to create Named Selection for every support/load.

## 6.2 Insert-from-selection

Fast path:

```text
select face(s)
→ Fixed Support / Force / Pressure
```

This is essential for setup speed.

## 6.3 Total Force semantics

One user-entered `F_total` is one resultant over the complete selected scope.

For initial uniform reference-area traction:

```text
A = Σ selected face reference areas
t = F_total / A
```

FE nodal-equivalent contribution must be integrated with element face shape functions/quadrature.

Do not implement:

```text
node_force = F_total / number_of_visible_arrows
```

or any similar visualization-dependent load logic.

## 6.4 Pressure semantics

Pressure follows surface normals. Reference/current configuration distinction becomes important under geometric nonlinearity. UI should not expose follower/deformed pressure until backend support is verified.

# 7. Nonlinear solver research priorities

The next solver-hardening study will compare, but not copy, how the references expose:

- Full Newton,
- Modified Newton,
- tangent update frequency,
- line search,
- load/time stepping,
- cutback,
- force/displacement/energy convergence,
- contact convergence,
- failure diagnostics.

Dynamics26 formulation starts from:

```text
R(u, λ) = F_ext(u, λ) - F_int(u)
K_T Δu = R
u_(i+1) = u_i + α Δu
```

Exact residual/tangent sign conventions must be documented and verified with finite differences.

# 8. Rubber mechanics research priorities

ANSYS/Marc/COMSOL comparison shows a mature rubber workflow needs more than a strain-energy formula.

Required stack:

```text
Test Data
→ Parameter Fitting
→ Material Model
→ Compressibility Choice
→ Element/Formulation
→ Large-Strain Kinematics
→ Contact
→ Nonlinear Controls
→ Verification
→ Component Correlation
```

Priority research:

- Neo-Hookean / Mooney-Rivlin / Yeoh / Ogden fitting,
- uniaxial / biaxial / planar test information content,
- volumetric response,
- nearly incompressible locking,
- mixed `u-p` / Herrmann / F-bar / selective approaches,
- tangent consistency,
- contact under large strain,
- viscoelastic extensions.

# 9. Extension architecture findings

Reference patterns suggest extension points should be capability-specific rather than a generic “run arbitrary code inside everything” system.

Recommended Dynamics26 boundary:

```text
Qt/C++ Extension Host
├─ WorkflowExtension
├─ ImporterExtension
├─ MeshExtension
├─ MaterialModelExtension
├─ SolverBackendExtension
├─ ResultExtension
└─ ExporterExtension
        ↓
Versioned stable interfaces / DTO
        ↓
C ABI adapters where Fortran interaction is required
```

For material laws, MFront/Marc user-subroutine concepts justify a future stress/tangent/state update plugin boundary. The first version should remain internal/experimental until ABI, thread safety, state ownership and error isolation are proven.

# 10. Research conclusions

1. **First priority is workflow completeness, not more isolated features.**
2. ANSYS provides the clearest scoping/object-tree reference.
3. COMSOL provides the clearest boundary-load semantics and physics-aware settings reference.
4. Marc is the primary commercial technical reference for difficult nonlinear/contact/rubber behavior.
5. Code_Aster is most useful for open architecture, explicit solver operators, verification culture and constitutive extension study.
6. Dynamics26 should combine these lessons without copying UI or source.
7. The immediate product milestone must connect the existing nonlinear core to a real model consumer; verification-only solver paths are insufficient.
8. Rubber development must couple constitutive models with incompressibility formulation, element behavior, contact and experimental fitting.
