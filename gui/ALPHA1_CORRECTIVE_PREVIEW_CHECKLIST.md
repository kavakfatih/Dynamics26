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
- [ ] Eski Home / Geometry / Materials / Connections benzeri ribbon katmanı yok.
- [ ] İkinci text-only command-hint/ribbon satırı yok.
- [ ] 3D viewport pencerenin baskın çalışma alanı.
- [ ] macOS native menu bar normal davranıyor.
- [ ] Pencere başlığı/toolbar alanında OpenGL ile problemli unified-title hack'i yok.

## 2. Project Navigator

- [ ] Sol panel boş/debug panel görünümünde değil.
- [ ] Geometri, Malzemeler, Kesitler, Mesh, Analizler ve Sonuçlar anlaşılır biçimde görünüyor.
- [ ] Ana ağaçta `Mixed u-p HEX8/P0` gibi solver implementation isimleri görünmüyor.
- [ ] `Analizler` altında Yükler ve Sınır Şartları / Analiz Ayarları erişilebilir.
- [ ] `Sonuçlar` seçimi alt Results & Diagnostics alanını açıyor.

## 3. Inspector

- [ ] Sağ panel sistem palette ile doğal görünüyor; siyah/debug tablo alanı yok.
- [ ] Başlangıçta anlamlı `Seçim yok` empty-state görülüyor.
- [ ] Navigator seçimi Inspector içeriğini contextually değiştiriyor.
- [ ] Legacy yatay QTabWidget tab strip görünmüyor.
- [ ] Geometri / Mesh / Malzeme / Kesit / Yük-BC / Analiz engineering sayfaları çalışmaya devam ediyor.

## 4. Toolbar ve gerçek komutlar

- [ ] Project Navigator toggle çalışıyor (`⌘1`).
- [ ] Inspector toggle çalışıyor (`⌘2`).
- [ ] Results & Diagnostics toggle çalışıyor (`⌘J`).
- [ ] Fit View çalışıyor.
- [ ] Linear Verification gerçek solver yolunu çalıştırıyor.
- [ ] Search / Undo / Redo gibi henüz gerçek command-registry'si olmayan sahte kontroller görünmüyor.
- [ ] Toolbar'daki komutların karşılığı menu bar'da bulunuyor.

## 5. Engineering yollarının korunması

- [ ] VTK viewport render ediyor ve etkileşim bozulmamış.
- [ ] STEP / OCCT geometry yolu çalışıyor.
- [ ] Mesh / Pre-Post akışı açılıyor.
- [ ] Material editor açılıyor ve mevcut preview davranışı korunuyor.
- [ ] Linear verification sonucu viewport + Results alanına geliyor.
- [ ] Modal verification sonucu çalışıyor.
- [ ] Nonlinear verification / convergence alanı çalışıyor.
- [ ] Project Open / Save / migration davranışı bozulmamış.

## 6. macOS appearance

- [ ] Light Mode'da okunabilirlik doğru.
- [ ] Dark Mode'da hard-coded beyaz/siyah legacy yüzey oluşmuyor.
- [ ] İnce separator ve küçük radius yaklaşımı doğal görünüyor.
- [ ] Gereksiz shadow / renkli chrome yok.
- [ ] Renk yalnız durum/önem taşıdığı yerde kullanılıyor.

## 7. Kabul kanıtı

Alpha.1 ancak aşağıdaki kanıtların tümü mevcut olduğunda kapanabilir:

1. `gui-fast on trusted Mac` başarılı.
2. Project migration gate başarılı.
3. arm64 GUI architecture gate başarılı.
4. İlgili clean hosted core/reproducibility gate başarılı.
5. Gerektiğinde manual strict standalone bundle audit başarılı.
6. Kullanıcı gerçek macOS uygulamasını açıp ekran görüntüsü/interaction incelemesini kabul etmiş.

**Alpha.2 — Navigator + Inspector Architecture bu kabulden önce başlamaz.**
