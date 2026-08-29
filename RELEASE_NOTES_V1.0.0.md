# FEMCAE V1.0.0 — Release Notes

## Release tanımı

V1.0.0, V0.1–V0.13 özellik zincirinin **verification ve hardening** sürümüdür. Portable source verification tamamlanmıştır. Native macOS signed/notarized binary gate'i henüz bu Linux hostta çalıştırılmamıştır.

## Ana V1 değişiklikleri

### 1. Disk checkpoint / restart

- `fem_checkpoint_io` eklendi.
- Schema version: 1.
- Real64 değerler IEEE bit-pattern hex olarak serialize edilir.
- Checksum doğrulaması vardır.
- Truncated ve checksum-bozuk checkpoint reddedilir.
- Read failure partial state bırakmaz.

### 2. Convergence study

Bağımsız Euler–Bernoulli referansı ile structured HEX8 cantilever:

| Mesh | Eleman | Relative error |
|---|---:|---:|
| 4×1×1 | 4 | 33.6192% |
| 8×2×2 | 32 | 10.8368% |
| 12×3×3 | 108 | 3.93645% |

Gate: hata monoton azalmalı ve fine mesh <%5 olmalı.

### 3. Failure handling

Corrupted Abaqus/DXF/checkpoint dosyaları ve public C API invalid arguments açık status/hata ile reddedilir.

### 4. Memory-safety hardening

V1 release testlerinin beşi GCC ASan + UBSan altında PASS olmuştur. Bu native Apple Instruments/Leaks audit'inin yerini almaz.

### 5. macOS release engineering

- `qt_generate_deploy_app_script`
- `BundleUtilities::fixup_bundle()`
- app-local `@executable_path/../Frameworks`
- `audit_bundle.sh`
- `sign_and_notarize.sh`
- GitHub macOS arm64 release gate'leri

kaynakta hazırdır.

## Portable test sonucu

```text
Debug   123/123 PASS
Release 123/123 PASS
ASan+UBSan V1 gates 5/5 PASS
```

Installed CLI/C API/C++ consumer: PASS.

## Kaynak warning gate

Checkpoint checksum rotate-index integer-kind uyarıları release kapanışında düzeltilmiş; son rebuild warning scan **0** olmuştur.

## Açık native gate'ler

V1.0 kaynak paketinin “signed/notarized macOS binary” olduğu iddia edilmez. Şunlar gerçek macOS/Apple Developer ortamı gerektirir:

1. `macos-15` arm64 CI tam çalışması,
2. native Qt/VTK/OCCT/Accelerate `.app` execution,
3. `otool` / dependency closure audit,
4. Developer ID signing,
5. Apple `notarytool` submission + stapling,
6. Gatekeeper `spctl` assessment,
7. binary bundle third-party license files,
8. GUI migration + native memory audit.
