# FEMCAE V0.10.0 — Release Notes

## Mixed u-p / Incompressibility

V0.10, nearly-incompressible hyperelastic solids için ilk gerçek displacement-pressure mixed formulation sürümüdür.

### Yeni formulation

`MIXED_UP_HEX8_P0`:

- Q1 trilinear displacement
- element başına bir P0 pressure DOF
- 24 + 1 = 25 local unknown

Mixed functional:

```text
Psi = W_iso + p(J-1) - p^2/(2K)
```

### Solver / assembly

- analytic `K_uu/K_up/K_pu/K_pp`
- global CSR mixed assembly
- block-aware displacement/pressure convergence
- symmetric-indefinite solver contract
- CG mixed sistemde explicit error
- element P0 pressure result recovery

### Verification

Dört V0.10 ana verification:

1. local mixed tangent finite difference
2. global sparse mixed tangent finite difference
3. `J=1` manufactured simple-shear coupled Newton
4. penalty-only vs mixed nearly-incompressible locking benchmark

Locking benchmark'ta `K/G0≈10^4` için mixed reference-tangent tip displacement magnitude, penalty-only Q1 cevabının yaklaşık **25.19 katıdır**.

### C API / GUI

- `fem_demo_mixed_up_hex8_shear`
- Qt analysis formulation selector
- mixed simple-shear pressure/recovered-gamma result display
- VTK source path for sheared HEX8 wireframe

### Bilinçli sınırlar

- Q1/P0 baseline universal inf-sup stability garantisi değildir.
- Stabilized Q1/Q1 ve higher-order mixed elements yoktur.
- Exact `K=∞` incompressibility yoktur.
- Contact/friction V0.11'dedir.
- Native macOS Qt/VTK/Accelerate execution CI release gate olarak kalır.
