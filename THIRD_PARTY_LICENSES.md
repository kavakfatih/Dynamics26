# FEMCAE V1.0 — Dependency / License Inventory

> Bu dosya hukuki görüş değildir. Binary dağıtım öncesi kullanılan **gerçek dependency sürümleri ve lisans metinleri** release paketine dahil edilmelidir.

FEMCAE'nin kendi kaynak kodu **Apache License 2.0** altında yayımlanır (`LICENSE`). Üçüncü taraf kütüphanelerin lisansları değişmez.

| Dependency | FEMCAE kullanımı | Genel lisans modeli | V1.0 dağıtım notu |
|---|---|---|---|
| Apple Accelerate | macOS sparse/direct ve LAPACK backend | macOS sistem framework'ü | Ayrı vendor binary bundle edilmez. |
| ARPACK-NG | modal eigensolver backend | BSD-style open-source | Kullanılan sürümün copyright/license notice'ı binary dağıtıma eklenmeli. |
| Qt 6 | masaüstü GUI | Commercial veya open-source (çoğu temel modül LGPLv3/GPL seçenekleri) | Community/LGPL yolu seçilirse dinamik linking, Qt source/offer ve kullanıcı relink/replace hakları dahil ilgili LGPL yükümlülükleri release sürecinde karşılanmalı. Commercial Qt kullanılıyorsa ilgili ticari sözleşme geçerlidir. |
| VTK | FEM viewport / contour | BSD-style permissive | Kullanılan VTK sürümünün notice/license metni dağıtıma eklenmeli. |
| Open CASCADE Technology | STEP/XDE/OCAF/B-Rep CAD adapter | LGPL-2.1 + OCCT additional exception | OCCT notice/source erişimi ve exception koşulları binary release öncesi doğrulanmalı. |
| CMake / Ninja / compiler toolchain | build-only | Çeşitli | Runtime bundle'a yalnız gerçekten gereken compiler runtime dylib'leri girer. |

## V1.0 release ilkesi

- Third-party kaynak kodu FEMCAE deposuna vendor edilmez; package-manager/system dependency olarak geliştirilir.
- macOS standalone `.app` üretiminde gereken runtime dylib/framework'ler bundle içine alınır ve `THIRD_PARTY_NOTICES.md` ile birlikte dağıtılır.
- Qt açık kaynak kullanımında statik link varsayılmaz; V1.0 bundle akışı shared framework/dylib dağıtımına göre tasarlanmıştır.
- Notarization bir lisans doğrulaması değildir; license-compliance kontrolü release checklist'in ayrı gate'idir.
