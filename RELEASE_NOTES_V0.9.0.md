# FEMCAE V0.9.0 — Release Notes

## Release theme

**Hyperelastic + Plastic Constitutive Models**

V0.9, V0.8'in geometric-nonlinear Newton çözümünü gerçek nonlinear constitutive response ile birleştirir.

## Yeni hyperelastic modeller

- Neo-Hookean
- Mooney-Rivlin
- Yeoh
- Ogden, 1–3 terim

Her model:

- strain-energy density,
- isochoric + volumetric contribution,
- Second Piola–Kirchhoff stress,
- consistent `dS/dE` tangent

üretir.

Production tangent'ler finite-difference değildir; finite-difference yalnız verification referansıdır.

## Ogden hardening

Çok-terimli gerçek fitting kullanımını gereksiz kısıtlamamak için aktif `mu_i` katsayıları signed olabilir. Baseline gate:

```text
mu_i != 0
alpha_i != 0
sum(mu_i) > 0
```

şeklindedir. Near-repeated principal stretch spectral tangent ayrıca verification ile korunur.

## Global hyperelastic çözüm

Hyperelastic material registry `model_t` içine eklenmiştir. Total-Lagrangian HEX8 ortak constitutive response üzerinden hyperelastic stress/tangent alır. Global nonlinear assembly ve V0.8 Newton solver bu yolu kullanabilir.

`VER-V090-005`, Neo-Hookean tek-HEX8 modelini gerçek load stepping/Newton çözümüyle hedef finite stretch'e getirir.

## J2 plasticity baseline

Eklenenler:

- small-strain von Mises J2,
- linear isotropic hardening,
- radial return mapping,
- consistent algorithmic tangent,
- committed/trial plastic strain,
- equivalent plastic strain,
- begin-trial / commit / revert.

**Sınır:** V0.9 J2, material-point foundation'dır. Global plastik element solution veya finite-strain multiplicative plasticity tamamlanmış değildir.

## Material Studio

Qt kaynaklarında hyperelastic Material Studio:

- model selection,
- parameter editor,
- unit labels,
- engine validation,
- initial shear modulus `G0`,
- engine-side isochoric uniaxial nominal stress preview

sağlar.

Preview C++ içinde constitutive denklemi kopyalamaz; public C ABI üzerinden Fortran engine'i çağırır.

## Verification

V0.9 release candidate sekiz yeni verification içerir:

- `VER-V090-001` hyperelastic tangent FD,
- `VER-V090-002` homogeneous deformation matrix,
- `VER-V090-003` J2 tangent FD,
- `VER-V090-004` hyperelastic HEX8 tangent FD,
- `VER-V090-005` hyperelastic Newton target stretch,
- `VER-V090-006` `dW/dE = S`,
- `VER-V090-007` J2 loading/return-map/state,
- `VER-V090-008` signed-term Ogden near-repeated spectral tangent.

## Önemli kapsam sınırı

Penalty volumetric energy:

\[
W_{vol}=\frac12K(J-1)^2
\]

mevcuttur fakat bu **mixed u-p değildir**. Nearly incompressible elastomerlerde volumetric locking'i çözmek V0.10'un ana görevidir.

Material parameter gate'leri tüm strain domain'inde global convexity/polyconvexity garantisi olarak yorumlanmamalıdır.


## Final portable validation

```text
Debug                     89/89 PASS
Release                   89/89 PASS
Release source warnings       0
Installed CLI              PASS
Installed C API consumer   PASS
CI YAML parse              PASS
```

## Platform gate

Portable Linux host source-level matematik/test doğrulaması içindir. Native macOS arm64 Accelerate ve Qt/VTK `.app` build/execution sonucu GitHub `macos-15` CI release gate olarak kalır.

## Lisans gate

Nihai `LICENSE` henüz seçilmemiştir. Kamuya açık open-source release öncesi lisans seçimi zorunludur.
