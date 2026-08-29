# FEMCAE V0.1.1 — Foundation Hardening

## Release status

V0.1.1, V0.2.0 veri modeline geçmeden önce V0.1.0 temelini sertleştiren patch sürümüdür.
Yeni bir FEM element ailesi veya solver özelliği eklemez; build/test/release sözleşmelerini güçlendirir.

## Ana değişiklikler

- Uygulama sürümü `0.1.1` olarak güncellendi; project schema, result schema ve C API sürümü `1` olarak korundu.
- macOS GitHub Actions akışı Debug + Release matrisine dönüştürüldü.
- Runner ve üretilen binary'ler için gerçek `arm64` fail gate eklendi.
- CI install-layout smoke testi eklendi.
- Kurulu public header + dylib üzerinden bağımsız C consumer smoke testi eklendi.
- Foundation hata yollarını kapsayan yeni unit test eklendi.
- Tolerance metadata için unit test eklendi.
- Kaynak doğrulama test sayısı 13'ten 15'e çıkarıldı.
- Kısa ve sürüm bazlı `VERSION_ROADMAP.md` eklendi.
- Roadmap'te aşırı geniş iki release bölündü:
  - V0.10: mixed `u-p` / incompressibility
  - V0.11: contact / friction
  - V0.12: CAD / geometry / sections
  - V0.13: meshing + full pre/post integration

## Değişmeyen mimari sözleşmeler

- `R = f_ext - f_int`
- `K_T * du = R`
- Voigt sırası: `XX, YY, ZZ, XY, YZ, XZ`
- Strain Voigt'te engineering shear
- Trial / commit / revert nonlinear state semantiği
- GUI ↔ solver sınırı: stable C API
- Project/result/API sürümlerinin bağımsızlığı
- `Node ID != Array Index != DOF ID != Equation ID` ilkesi

## Bilinen release gate

Native macOS/Apple Silicon GitHub Actions çalışması repository üzerinde henüz yürütülmelidir.
Linked GitHub hesabında bu çalışma sırasında erişilebilir bir FEMCAE repository bulunmadığı için bu paket içinde yalnızca workflow güçlendirmesi ve Linux kaynak-seviyesi doğrulaması yapılmıştır.

Açık kaynak `LICENSE` seçimi hâlâ proje sahibi kararı beklemektedir. Lisans seçilmeden public open-source release yapılmamalıdır.
