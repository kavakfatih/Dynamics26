# FEMCAE V0.11.0 — Release Notes

## Özet

V0.11.0, V0.10 mixed `u-p` incompressibility çekirdeğinden bağımsız bir **contact / friction subsystem** ekler. Release baseline'ı deformable slave node ile rigid planar QUAD4 master facet arasındaki temas için search, normal enforcement, Coulomb friction, consistent tangent ve nonlinear state rollback zincirini tamamlar.

## Ana ekler

- Contact facet / pair / registry veri modeli.
- Per-slave committed/trial contact-point state.
- Expanded AABB broad-phase search.
- Closest-point narrow-phase projection.
- Signed normal gap ve facet-order normal convention'ı.
- Penalty contact.
- Incremental augmented-Lagrangian contact.
- Coulomb friction, stick/slip state.
- Analytic effective contact tangent.
- Global nonlinear sparse contact assembly.
- Newton, line search, cutback, commit/revert entegrasyonu.
- Contact summary results: active/stick/slip, penetration, normal/tangential resultant.
- Additive C API `fem_demo_contact_hex8(...)`.
- Qt/VTK source contact verification workflow.

## Augmented-Lagrangian hardening

İlk AL denemesinde committed multiplier absolute gap ile her evaluation'da yeniden artırılıyordu. Bu, converged state commit edildikten sonra aynı configuration yeniden evaluate edilirse normal kuvvetin yapay biçimde büyümesine neden olabilirdi.

V0.11 release davranışı committed-gap increment kullanır:

```text
lambda_trial = max(0, lambda_committed - k_n (g_trial - g_committed))
```

Böylece aynı committed configuration'ın tekrar evaluation'ı idempotent'tir. Bu davranış `VER-V110-002` ile doğrulanır.

## Verification

- `VER-V110-001`: Coulomb stick/slip tangent finite-difference check.
- `VER-V110-002`: augmented-Lagrangian state/invariance.
- `VER-V110-003`: TL-HEX8 + rigid plane global Newton equilibrium; 1000 N normal force, yaklaşık 2.5 µm maximum penetration.
- `VER-V110-004`: forced failed increment → contact multiplier/history rollback.
- `VER-V110-005`: 3B global Coulomb solve; `N≈1000 N`, `T=300 N` for `μ=0.30`, Coulomb bound preserved.

## Bilinçli sınırlar

V0.11 aşağıdakileri production-ready olarak sunmaz:

- deformable-deformable contact,
- segment-to-segment veya mortar contact,
- self-contact,
- BVH hierarchy / parallel search,
- curved/higher-order master facet,
- arbitrary contact surface extraction/preprocessing,
- contact pressure contour extrapolation,
- disk tabanlı contact-history restart.

GUI contact preset'i frictionless compression'dır. Coulomb friction core/global verification ile test edilir.

Native macOS arm64 Qt/VTK/Accelerate execution portable Linux validation hostunda çalıştırılmış gibi raporlanmaz; CI release gate'i açık kalır.
