# Dynamics26 V1.1.0-alpha.1 Corrective — macOS Preview Acceptance Checklist

Bu kontrol listesi Alpha.1 corrective iteration'ın gerçek macOS kullanıcı incelemesi içindir.
CI başarıları görsel/interaction kabulünün yerine geçmez. Alpha.2 bu liste kullanıcı tarafından
kabul edilmeden başlamaz.

## Ön koşullar

- Build: Apple Silicon `arm64`
- Platform: macOS
- GUI milestone: `1.1.0-alpha.1`
- Engineering preview yalnız UX/interaction incelemesi içindir.
- Engineering preview standalone release değildir; Homebrew Qt/VTK/OCCT runtime bağımlılıklarını kullanabilir.
- Dağıtılabilir build için ayrıca manual `native-release` / strict bundle audit zorunludur.

## 1. Application shell

- [ ] Açılışta tek ve kompakt üst toolbar yüzeyi görülüyor.
- [ ] Eski ribbon / çok katlı yatay command yüzeyi yok.
- [ ] 3D viewport pencerenin baskın çalışma alanı.
- [ ] macOS native menu bar normal davranıyor.
- [ ] OpenGL/VTK ile uyumsuz unified-title hack'i kullanılmıyor.
- [ ] Alt engineering status strip düşük dikkat seviyesinde model/solve durumunu gösteriyor.

## 2. Project Navigator

- [ ] Geometri, Malzemeler, Kesitler, Mesh, Analizler ve Sonuçlar anlaşılır biçimde görünüyor.
- [ ] Ana ağaçta `Mixed u-p HEX8/P0` gibi solver implementation isimleri görünmüyor.
- [ ] `Analizler` altında Yükler ve Sınır Şartları / Analiz Ayarları erişilebilir.
- [ ] Sonuç ağacında sayısal değer yerine sonuç nesnesi adları bulunuyor.
- [ ] `Sonuçlar` bağlamına geçildiğinde gerçek result contour görünümü geri geliyor.

## 3. Inspector

- [ ] Sağ panel sistem palette ile doğal görünüyor; clipping/overlap yok.
- [ ] Legacy yatay QTabWidget tab strip görünmüyor.
- [ ] Formlar dar panelde responsive ve gerektiğinde dikey scroll ile kullanılabilir.
- [ ] Material Inspector yalnız seçilen modele ait parametreleri gösteriyor.
- [ ] Analysis Inspector `Analiz Türü` seçimine göre contextual davranıyor.
- [ ] Lineer Statik seçiliyken modal/nonlinear controls görünmüyor.
- [ ] Nonlineer Statik seçiliyken `Gelişmiş Çözücü Ayarları` disclosure olarak açılıyor.
- [ ] Mesh Inspector yalnız mesh tanımı/özeti taşıyor; legacy solve/post/export karmaşası görünmüyor.

## 4. Viewport semantics

- [ ] Geometri bağlamı CAD/display tessellation görünümünü kullanıyor.
- [ ] Mesh / Malzeme / Kesit / Yük-BC / Analiz preprocessing bağlamlarında result contour görünmüyor.
- [ ] Preprocessing sırasında nötr mesh/geometry görünümü kullanılıyor.
- [ ] Sonuçlar bağlamında contour tekrar görüntüleniyor.
- [ ] CAD Geometry != Display Tessellation != FEM Mesh ayrımı bozulmamış.

## 5. Command authenticity

- [ ] Project Navigator toggle çalışıyor (`⌘1`).
- [ ] Inspector toggle çalışıyor (`⌘2`).
- [ ] Diagnostics/utility toggle çalışıyor (`⌘J` ve status strip handle).
- [ ] Fit View çalışıyor.
- [ ] Mesh Oluştur gerçek structured HEX8 yoluna bağlı.
- [ ] `Çöz` yalnız entegre Lineer Statik workflow için enabled.
- [ ] Henüz entegre olmayan modal/nonlinear GUI yollarında `Çöz` disabled ve gerekçeli tooltip veriyor.
- [ ] Demo/verification komutları normal kullanıcı command surface'inde görünmüyor.
- [ ] Search / Undo / Redo gibi henüz gerçek command-registry'si olmayan sahte kontroller görünmüyor.

## 6. Results & Diagnostics drawer

- [ ] Drawer varsayılan kapalı.
- [ ] Kullanıcı açtığında Sonuçlar / Yakınsama / Günlük sekmeleri korunuyor.
- [ ] Çözüm tamamlandığında Sonuçlar sekmesi açılabiliyor ve sayısal tablo kullanıcı dilinde görünüyor.
- [ ] Error/hata durumunda Günlük alanı görünür hale geliyor.
- [ ] Drawer kendiliğinden kapanmıyor; kullanıcı kapatıyor.
- [ ] Klasik QDockWidget title chrome yerine engineering status strip çekmecenin görünür kontrolü olarak çalışıyor.

## 7. macOS appearance

- [ ] Light Mode'da okunabilirlik doğru.
- [ ] Dark Mode'da hard-coded beyaz/siyah legacy yüzey oluşmuyor.
- [ ] Inspector section'ları property-grid gibi görünmüyor; gereksiz group-box chrome azaltılmış.
- [ ] İnce separator ve küçük radius yaklaşımı doğal görünüyor.
- [ ] Gereksiz shadow / renkli chrome yok.
- [ ] Renk yalnız durum/önem/result anlamı taşıdığı yerde kullanılıyor.

## 8. Engineering yollarının korunması

- [ ] VTK viewport render ediyor ve etkileşim bozulmamış.
- [ ] STEP / OCCT geometry yolu çalışıyor.
- [ ] Structured HEX8 mesh üretimi çalışıyor.
- [ ] Material editor ve hyperelastic preview yolu korunuyor.
- [ ] Lineer mesh solve sonucu Results alanına geliyor.
- [ ] Backend modal/nonlinear/contact verification yolları kaynak kodda ve regression testlerinde korunuyor.
- [ ] Project Open / Save / migration davranışı bozulmamış.

## 9. Kabul kanıtı

Alpha.1 ancak aşağıdaki kanıtların tümü mevcut olduğunda kapanabilir:

1. `gui-fast on trusted Mac` başarılı.
2. Project migration gate başarılı.
3. arm64 GUI architecture gate başarılı.
4. İlgili clean hosted core/reproducibility gate başarılı.
5. Gerektiğinde manual strict standalone bundle audit başarılı.
6. Light ve Dark Mode gerçek macOS ekran görüntüleri incelenmiş.
7. Kullanıcı gerçek macOS uygulamasını açıp ekran görüntüsü/interaction incelemesini kabul etmiş.

**Alpha.2 — Navigator + Inspector Architecture bu kabulden önce başlamaz.**
