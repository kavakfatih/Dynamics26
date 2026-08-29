# FEMCAE V0.4.0 Release Notes

## Sparse Assembly / macOS Linear Solver

V0.4.0, FEMCAE'nin ilk global assembled lineer cozum altyapisini ekler.

### Yeni

- Element DOF/equation map.
- Dense adjacency kullanmayan sparsity graph.
- CSR sparse matrix storage.
- Generic global matrix ve vector scatter.
- Stiffness/tangent/mass assembly icin ortak matrix yolu.
- Nonzero prescribed displacement RHS correction.
- Persistent DOF-ID tabanli reaction recovery.
- Matrix symmetry/definiteness metadata.
- Backend-bagimsiz LinearSolver facade.
- Partial-pivoting dense reference solver.
- Jacobi-preconditioned sparse CG.
- Apple Accelerate Sparse icin ISO_C_BINDING + C adapter tabanli direct solver backend.
- TRUSS2 linear local stiffness verification helper.
- `VER-V040-001` assembled two-bar verification.
- CLI assembled sparse solve demonstrasyonu.

### Hardening

- Pattern disi sparse scatter explicit hata verir.
- Singular dense solve numerical failure olarak raporlanir.
- Unknown solver backend explicit hata verir.
- Debug runtime checks, sparsity duplicate loop'unda Fortran short-circuit varsayimindan kaynaklanabilecek out-of-bounds riskini yakaladi; explicit branch ile giderildi.

### Mimari kontrat

```text
Assembly -> LinearSolver Interface -> Backend
```

Platform/vendor kodu assembly katmanina sizdirilmaz.

### Verification

Iki seri 1 m TRUSS2, `E=210 GPa`, `A=1e-4 m^2`, `F=1000 N`:

```text
u2 = 4.7619047619e-5 m
u3 = 9.5238095238e-5 m
R_support = -1000 N
```

Dense reference ve sparse CG bu assembled sistemi analitik degerle dogrular.

### Native macOS notu

Accelerate adapter source-level olarak hazirdir ve macOS CTest akisi backend available oldugunda gercek direct solve yolunu calistirir. Bu gelistirme ortaminda macOS/Accelerate binary calistirilamadigi icin native arm64 GitHub Actions sonucu release gate olarak acik kalir.

### Korunan kontratlar

- `Node ID != Array Index != DOF ID != Equation ID`
- `R = f_ext - f_int`
- `K_T * du = R`
- Voigt `XX, YY, ZZ, XY, YZ, XZ`
- engineering shear strain convention
- trial/commit/revert state semantics
- C API / project schema / result schema surumleri `1`

### Sonraki sürüm

V0.5.0 — Linear Structural FEM + İlk Qt GUI.
