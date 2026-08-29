# FEMCAE V0.7.0 — Release Notes

## Başlık

**Finite Strain / Geometric Nonlinearity Foundation**

## Yeni özellikler

- Reference/current configuration.
- `F`, `J`, Green-Lagrange ve Euler-Almansi strain.
- PK2, PK1, Kirchhoff ve Cauchy stress measure dönüşümleri.
- StVK reference constitutive response.
- `TOTAL_LAGRANGIAN_HEX8` formulation.
- Internal force + material tangent + geometric tangent.
- Element ve global consistent-tangent finite-difference verification.
- Sparse global nonlinear system evaluator.
- Global nonlinear displacement trial/commit/revert state.
- Follower-load configuration/external-tangent metadata.
- Pure ve superposed rigid-rotation objectivity testleri.
- Finite-stretch analytic stress verification.

## Neden StVK?

StVK bu sürümde geometric nonlinearity matematiğini constitutive complexity'den ayırmak için kullanılır. Büyük elastomer strain'leri için önerilen model değildir.

Hyperelastic material library V0.9'da, nearly-incompressible mixed `u-p` V0.10'da gelecektir.

## Bilinçli kapsam dışı

- Newton-Raphson
- line search
- adaptive load stepping / cutback
- nonlinear convergence GUI
- surface pressure/follower traction integration
- hyperelasticity/plasticity
- mixed `u-p`
- contact

## Test sonucu

Portable Debug final candidate: **73/73 PASS**.

Clean Release sonucu: **73/73 PASS**.

Ek release gate sonuçları:

- staged install: **PASS**
- installed CLI: **PASS**
- installed C API consumer: **PASS**
- source compiler warning: **0**
