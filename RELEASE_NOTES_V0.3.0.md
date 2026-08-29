# FEMCAE V0.3.0 Release Notes

## Element Kernel / Shape Functions / Quadrature

V0.3.0, FEMCAE'nin ilk gerçek element-local FEM matematiğini ekler.

### Yeni

- BAR2 / QUAD4 / HEX8 reference elements.
- Lagrange shape functions ve natural derivatives.
- 1B/2B/3B Gauss-Legendre tensor-product quadrature.
- Isoparametric mapping.
- 2B/3B Jacobian + inverse Jacobian.
- Embedded 3B BAR2/TRUSS2 line metric ve unit direction.
- Physical shape gradients.
- Plane / solid / axisymmetric small-strain B matrices.
- Axisymmetric `2*pi*r` integration measure.
- Element result container.
- Element quality ve inverted/degenerate detection.
- Topology/formulation ayrımlı element registry.
- Beam/shell prototype rotation/section metadata.
- 4 gerçek element patch testi.

### Sayısal hardening

Jacobian singularity kontrolü birim ölçeğine bağımlı sabit determinant eşiğinden dimensionless scaled determinant kontrolüne geçirildi.

### Korunan kontratlar

- `Node ID != Array Index != DOF ID != Equation ID`
- `R = f_ext - f_int`
- `K_T * du = R`
- Voigt `XX, YY, ZZ, XY, YZ, XZ`
- engineering shear strain convention
- trial/commit/revert state semantics
- C API / project schema / result schema sürümleri `1`

### Sonraki sürüm

V0.4.0 — Sparse Assembly / macOS Linear Solver.
