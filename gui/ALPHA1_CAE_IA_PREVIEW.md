# Dynamics26 V1.1.0-alpha.1 — CAE Information Architecture Preview

Bu preview, ANSYS Mechanical / COMSOL Model Builder araştırması sonrasında Alpha.1 görünür arayüzünü yeniden kuran recovery turudur.

## Görsel kabul odağı

- Merkezde **Graphics** alanı baskın olmalı; Model Tree ve Detaylar ikincil panellerdir.
- Sol ağaç `Model -> Geometri / Malzemeler / Kesitler / Bağlantılar / Mesh` ve `Statik Yapısal 1 -> Analiz Ayarları / Sabit Mesnet / Kuvvet / Çözüm` iş akışını göstermelidir.
- `Henüz sonuç yok` gibi sahte tree node'ları kullanılmamalıdır.
- Sağ **Detaylar** alanı legacy `GeometryPanel` / `PrePostPanel` formunun tamamını göstermemeli; seçili CAE nesnesinin kompakt özelliklerini göstermelidir.
- Geometri Detayları yalnız kaynak, gövde sayısı, durum ve görüntüleme sözleşmesini göstermelidir; STEP/DXF büyük legacy formu görünmemelidir.
- Mesh Detayları yöntem, boyutlar, bölüntüler ve mesh istatistiklerini kompakt biçimde göstermelidir.
- Analiz Detayları normal kullanıcı seviyesinde analiz türü / Automatic formülasyon / solve durumu göstermelidir.
- Çözüm Detayları solve sonrasında temel sonuç özetini göstermelidir.
- Alt Sonuçlar / Yakınsama / Mesajlar alanı normal çalışma sırasında kapalı kalmalı; hata veya kullanıcı isteğiyle açılmalıdır.
- Graphics bağlamı küçük viewport overlay'i ile gösterilmeli; viewport üstünde ikinci büyük tam-genişlik toolbar oluşturulmamalıdır.
- macOS system appearance korunmalı; global QSS/QPalette skin geri getirilmemelidir.

CI başarısı yalnız teknik gate'tir. Alpha.1 kullanıcı gerçek macOS ekranını görüp kabul etmeden PASS değildir.
