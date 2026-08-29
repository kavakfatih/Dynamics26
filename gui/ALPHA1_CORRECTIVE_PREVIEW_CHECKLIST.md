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

## 1. CAE workstation shell

- [ ] Ana yerleşim ilk bakışta profesyonel CAE workstation olarak okunuyor: **Model Tree | Graphics | Details**.
- [ ] Açılışta tek ve kompakt global toolbar yüzeyi görülüyor.
- [ ] Toolbar yalnız gerçek backend'e bağlı global komutları taşıyor: Navigator, STEP Import, Mesh, Fit, Solve, Diagnostics, Inspector.
- [ ] `Solve`, toolbar'da kısa metin etiketi taşıyan tek birincil global eylem.
- [ ] Eski ribbon / çok katlı yatay command yüzeyi yok.
- [ ] Graphics alanı pencerenin baskın çalışma alanı.
- [ ] Graphics alanının üstünde düşük profilli context bar bulunuyor; burada sahte selection/section/probe komutları gösterilmiyor.
- [ ] macOS native menu bar normal davranıyor.
- [ ] OpenGL/VTK ile uyumsuz unified-title hack'i kullanılmıyor.
- [ ] Alt engineering status strip düşük dikkat seviyesinde model/solve durumunu gösteriyor.

## 2. Model Tree — Alpha.1 display contract

Alpha.1 gerçek `ProjectTreeModel` değildir; bu model Alpha.2 kapsamıdır. Ancak görünür hiyerarşi ANSYS Mechanical / COMSOL Model Builder mantığını önceden doğru temsil etmelidir.

- [ ] Üst düzeyde `Model` ve en az bir gerçek analysis container (`Static Structural 1`) görülüyor.
- [ ] `Model` altında Geometry, Materials, Sections, Connections ve Mesh bulunuyor.
- [ ] `Static Structural 1` altında Analysis Settings, Loads and Boundary Conditions ve Solution bulunuyor.
- [ ] Solution altında solve sonrasında sayısal değer değil result object adları (`Total Deformation`, `Equivalent Stress`, `Reaction Force` vb.) oluşuyor.
- [ ] Henüz sonuç yoksa `Henüz sonuç yok` empty-state satırı gösteriliyor.
- [ ] Ana ağaçta `Mixed u-p HEX8/P0` gibi solver implementation isimleri görünmüyor.
- [ ] Model Tree seçimi Details ve Graphics bağlamını birlikte değiştiriyor.

## 3. Details / Inspector

- [ ] Sağ panel başlığı `DETAILS` ve seçili engineering object context'i açıkça görünüyor.
- [ ] Sağ panel native macOS palette/style ile doğal görünüyor; clipping/overlap yok.
- [ ] Legacy yatay QTabWidget tab strip görünmüyor.
- [ ] Formlar dar panelde responsive ve gerektiğinde dikey scroll ile kullanılabilir.
- [ ] Material Inspector yalnız seçilen modele ait parametreleri gösteriyor.
- [ ] Analysis Inspector `Analysis Type` seçimine göre contextual davranıyor.
- [ ] Linear Static seçiliyken modal/nonlinear controls görünmüyor.
- [ ] Nonlinear Static seçiliyken `Gelişmiş Çözücü Ayarları` disclosure olarak açılıyor.
- [ ] Nonlinear/Modal Solve disabled olduğunda kullanıcı Details içinde kısa ve görünür neden görüyor; yalnız tooltip'e güvenilmiyor.
- [ ] Normal kullanıcı seviyesinde formulation seçenekleri user-intent dilini kullanıyor (`Automatic / Standard`, `Nearly Incompressible`).
- [ ] Mesh Inspector yalnız mesh tanımı/özeti taşıyor; legacy solve/post/export karmaşası görünmüyor.

## 4. Graphics / Viewport semantics

- [ ] Graphics header seçili bağlamı (`Geometry`, `Mesh`, `Static Structural`, `Results` vb.) doğru gösteriyor.
- [ ] Geometry bağlamı CAD/display tessellation görünümünü kullanıyor.
- [ ] Mesh / Material / Section / Loads-BC / Analysis preprocessing bağlamlarında result contour görünmüyor.
- [ ] Preprocessing sırasında nötr mesh/geometry görünümü kullanılıyor.
- [ ] Koyu görünümde neutral wire/edge geometri arka plana gömülmüyor.
- [ ] Açık görünümde neutral solid shading yüzeyleri aşırı beyaz/siyah kontrasta kaçmıyor.
- [ ] Solution/Results bağlamında contour tekrar görüntüleniyor.
- [ ] VTK viewport arka planı ve neutral geometry/mesh edge renkleri macOS Light/Dark appearance ile birlikte güncelleniyor.
- [ ] Scalar-mapped result contour renkleri theme geçişiyle bozulmuyor.
- [ ] CAD Geometry != Display Tessellation != FEM Mesh ayrımı bozulmamış.

## 5. Command authenticity

- [ ] Project Navigator toggle çalışıyor (`⌘1`).
- [ ] Details/Inspector toggle çalışıyor (`⌘2`).
- [ ] Diagnostics/utility toggle çalışıyor (`⌘J` ve status strip handle).
- [ ] STEP Import toolbar komutu gerçek GeometryPanel import yoluna bağlı.
- [ ] Mesh toolbar komutu gerçek structured HEX8 yoluna bağlı.
- [ ] Fit View çalışıyor.
- [ ] Solve yalnız entegre Linear Static workflow için enabled.
- [ ] Henüz entegre olmayan modal/nonlinear GUI yollarında Solve disabled ve gerekçesi görünür.
- [ ] Demo/verification komutları normal kullanıcı command surface'inde görünmüyor.
- [ ] Search / Undo / Redo / Contact Wizard / Probe / Section gibi henüz gerçek command-registry'si olmayan sahte kontroller görünmüyor.

## 6. Results & Diagnostics workspace

- [ ] Drawer uygulama ilk açıldığında varsayılan kapalı.
- [ ] Kullanıcı açtığında Results / Convergence / Messages-Solver sekmeleri korunuyor.
- [ ] Çözüm tamamlandığında Results sekmesi açılıyor ve sayısal tablo okunabilir durumda.
- [ ] Error/hata durumunda Messages / Solver alanı görünür hale geliyor.
- [ ] Drawer kendiliğinden kapanmıyor; kullanıcı kapatıyor.
- [ ] Klasik QDockWidget title chrome yerine engineering status strip çekmecenin görünür kontrolü olarak çalışıyor.

## 7. Composition-root recovery contract

- [ ] Startup composition root artık `MainWindow -> Dynamics26Shell -> CaeWorkbenchController -> AppearanceController` zincirini kullanıyor.
- [ ] Eski `Alpha1UxController` ve `Alpha1ProductPolish` startup'ta çalıştırılmıyor ve aynı widget'ı üst üste yeniden biçimlendirmiyor.
- [ ] `CaeWorkbenchController` engineering backend verisinin sahibi değil; yalnız görünür workbench orchestration yapıyor.
- [ ] Alpha.2 başlamadan gerçek `QTreeView + ProjectTreeModel` object architecture kapsamına geçilmiyor.

## 8. macOS appearance — recovery contract

- [ ] Dynamics26 görünümü macOS System Appearance tarafından belirleniyor; uygulama global `QPalette` skin'i zorlamıyor.
- [ ] Uygulama genelinde `QLabel/QPushButton/QLineEdit/...` gibi generic widget sınıflarını boyayan global QSS skin'i kullanılmıyor.
- [ ] macOS Light appearance altında Model Tree, Details, controls ve status text eksiksiz okunuyor.
- [ ] macOS Dark appearance altında Model Tree, Details, controls ve status text eksiksiz okunuyor.
- [ ] Light → Dark → Light sistem geçişinde eski temadan renk/foreground/background kalıntısı kalmıyor.
- [ ] Gereksiz shadow / renkli chrome yok.
- [ ] Renk yalnız durum/önem/result anlamı taşıdığı yerde kullanılıyor.

## 9. Engineering yollarının korunması

- [ ] VTK viewport render ediyor ve etkileşim bozulmamış.
- [ ] STEP / OCCT geometry yolu çalışıyor.
- [ ] Structured HEX8 mesh üretimi çalışıyor.
- [ ] Material editor ve hyperelastic preview yolu korunuyor.
- [ ] Lineer mesh solve sonucu Results alanına geliyor.
- [ ] Backend modal/nonlinear/contact verification yolları kaynak kodda ve regression testlerinde korunuyor.
- [ ] Project Open / Save / migration davranışı bozulmamış.

## 10. Kabul kanıtı

Alpha.1 ancak aşağıdaki kanıtların tümü mevcut olduğunda kapanabilir:

1. `gui-fast on trusted Mac` başarılı.
2. Project migration gate başarılı.
3. arm64 GUI architecture gate başarılı.
4. İlgili clean hosted core/reproducibility gate başarılı.
5. Gerektiğinde manual strict standalone bundle audit başarılı.
6. macOS sistem Light ve Dark görünümünde gerçek ekran görüntüleri incelenmiş.
7. Light → Dark → Light geçişi gerçek macOS üzerinde kalıntısız doğrulanmış.
8. Model Tree / Graphics / Details / Utility davranışı gerçek macOS interaction incelemesinde kabul edilmiş.
9. Kullanıcı gerçek macOS uygulamasını açıp ekran görüntüsü/interaction incelemesini kabul etmiş.

**Alpha.2 — gerçek ProjectTreeModel + Navigator/Inspector object architecture bu kabulden önce başlamaz.**
