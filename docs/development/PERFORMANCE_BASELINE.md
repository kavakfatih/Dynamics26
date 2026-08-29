# V1.0 Performance Baseline

## Amaç

Bu belge bir donanım benchmark sıralaması değildir. Amaç aynı CI/host üzerinde algoritmik/performance regresyonlarını yakalayacak tekrar edilebilir bir smoke problem tanımlamaktır.

## Problem

`perf_v1000_001`:

- structured HEX8 mesh: 12×3×3,
- elements: 108,
- nodes: 208,
- approximate active DOF: 605,
- linear elastic generic HEX8 C ABI solve.

## Portable Release örnek sonucu

```text
elapsed_seconds ≈ 0.434
```

Bu sayı yalnız mevcut doğrulama hostunun gözlemidir. CPU, compiler, BLAS/backend, thermal state ve CI contention farklılıkları nedeniyle başka makinelerle doğrudan karşılaştırılmamalıdır.

## Gate

- fiziksel sanity: tip deplasmanı analitik axial referansın %5'i içinde,
- reaction equilibrium exact tolerance içinde,
- geniş smoke ceiling: 30 s.

Accuracy/convergence iddiası ayrı `VER-V1000-001` testine aittir.
