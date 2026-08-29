# FEMCAE V0.2.0 — Model / Mesh / Field / DOF / Numbering

**Tarih:** 2026-08-29

V0.2.0, FEMCAE'in gercek FEM veri modelini kuran ilk minor release'idir. Bu release'in ana hedefi element formulation eklemek degil, element/assembly/nonlinear katmanlarinin daha sonra refactor gerektirmeden kullanabilecegi kimlik, topoloji, field ve denklem numaralandirma sozlesmelerini kurmaktir.

## Ana karar

```text
Node ID != Array Index != DOF ID != Equation ID
```

Bu kural kod ve regression test seviyesinde dogrulanmistir.

## Eklenen moduller

- `src/model/fem_topology.f90`
- `src/model/fem_sets.f90`
- `src/model/fem_mesh.f90`
- `src/model/fem_fields.f90`
- `src/model/fem_dofs.f90`
- `src/model/fem_constraints.f90`
- `src/model/fem_numbering.f90`
- `src/model/fem_coordinate_frames.f90`
- `src/model/fem_model.f90`

## Field altyapisi

Standart structural registry su alanlari tanir:

```text
displacement : 3 component
pressure     : 1 component
rotation     : 3 component
```

Bu, V0.10 mixed `u-p` ve ileride beam/shell rotational DOF'lari icin erken mimari hazirliktir. V0.2.0 mixed formulation cozum yapmaz.

## Numbering

- DOF ID kalicidir ve entity ID'den turetilmez.
- Constraint equation ID'ye degil DOF ID'ye baglanir.
- Equation IDs aktif serbest DOF'lara 0'dan baslayarak deterministik atanir.
- Constraint'li DOF icin equation ID `INVALID_ID` olur.
- Renumbering kalici mesh/DOF kimliklerini degistirmez.

## Test sonucu

Source-level Debug validation:

```text
24/24 CTest passed
18 unit
1 verification
3 regression
1 C API smoke
1 CLI smoke
```

Native macOS/arm64 CI release gate'i repository uzerinde ayrica calistirilmalidir.

## Sonraki release

**V0.3.0 — Element Kernel / Shape Functions / Quadrature**

Ilk hedefler: reference element, natural coordinates, shape functions, gradients, Jacobian, Gauss quadrature ve BAR2/QUAD4/HEX8 matematik prototipleri.
