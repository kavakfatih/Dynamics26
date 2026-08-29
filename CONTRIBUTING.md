# Contributing

## Temel kurallar

1. Identifier ve API isimleri Ingilizce yazilir.
2. Muhendislik ve matematik aciklamalari Turkce yazilir.
3. FEM formulu ekleyen her PR, `docs/theory/` altinda turetim ve kaynak bilgisi icermelidir.
4. Code_Aster veya baska copyleft projelerden kaynak kod/test/yorum kopyalanmaz.
5. Library kodunda kontrolsuz `stop` / `error stop` kullanilmaz.
6. Yeni davranis unit test ve uygun oldugunda verification/patch test ile gelir.
7. Mimari olarak dondurulmus karar degisiyorsa yeni ADR yazilir.
8. Release hedefi macOS/Apple Silicon'dur.

## PR kontrol listesi

- [ ] Matematik teorisi/aciklamasi var
- [ ] Kaynak/provenance belirtilmis
- [ ] Unit test var
- [ ] Verification/patch test gerekli ise var
- [ ] Regression etkisi degerlendirildi
- [ ] CTest geciyor
- [ ] CHANGELOG guncellendi
- [ ] Yeni third-party dependency varsa lisans incelemesi yapildi
