# Dynamics26 — GUI Raporu (V1.1)

Bu belge iki turu kapsar:

* **Bölüm I — Alpha.1: GUI Architecture Recovery.** Corrective katman zincirinin
  kaldırılması ve profesyonel CAE kabuğunun kurulması.
* **Bölüm II — Alpha.2: CAE Desktop Interaction Foundation.** Dark Mode
  düzeltmesi, Qt 6.5+ uyumluluğu, tam nesne kalıcılığı, doküman komut sistemi
  (Undo/Redo), bağımlılık motoru, suppression, bağlam menüleri, preflight ve
  yaşam döngüsü komutları.

**Her iki turda da solver çekirdeği (Fortran), geometry/meshing C++ katmanı ve
C ABI değiştirilmedi.**

---

# BÖLÜM I — Alpha.1: GUI Architecture Recovery

**Kapsam:** GUI ve application-shell yeniden kurulumu.

Hedef: "biraz daha güzel Dynamics26" değil; **ANSYS Mechanical / COMSOL
Multiphysics ürün sınıfında, sürdürülebilir mimariye sahip, macOS-native bir CAE
workstation**.

---

## 1. Mevcut GUI'de bulunan problemler

### 1.1 Üst üste binmiş düzeltme katmanları

`gui/main.cpp` şu zinciri kuruyordu:

```
MainWindow  →  applyApplicationShell()      (Dynamics26Shell,      1067 satır)
            →  installCaeWorkbenchController() (CaeWorkbenchController, 599 satır)
            →  installAppearanceController()   (AppearanceController,   228 satır)
```

ve depoda ayrıca `Alpha1UxController` (474 satır) + `Alpha1ProductPolish`
(297 satır) duruyordu. Bu katmanların hepsi **aynı görünür widget'ları** sonradan
değiştiriyordu. Tek sahipli bir kompozisyon yoktu.

### 1.2 Widget keşfi (widget discovery) anti-pattern'i

Kabuk, mühendislik nesnelerini görünen metinden ve widget türünden buluyordu.
Kaldırılan kodda tespit edilen 60'tan fazla örnekten bazıları:

```cpp
// Alpha1UxController.cpp:107
if (label->text().contains(QStringLiteral("CAD B-Rep"), Qt::CaseInsensitive)) …

// Dynamics26Shell.cpp:340
if (label->text().size() > 36 || label->text().contains(QChar(0x2192))) …

// Dynamics26Shell.cpp:449
if (button->text().contains(QStringLiteral("Henüz çözüm yok"), …)) …

// CaeWorkbenchController.cpp:577  — analiz combo'sunu içeriğinden tanıma
for (auto *combo : window.findChildren<QComboBox *>()) {
    const bool linear = combo->findText("Linear Static") >= 0 || …;
```

Bir etiket metni değiştiğinde arayüz sessizce bozuluyordu.

### 1.3 Eski büyük panellerin Details'e gömülmesi

`GeometryPanel` (STEP import + seçim filtresi + geometri ağacı + DXF + açıklama
metni) ve `PrePostPanel` (mesh formu + solve formu + export düğmeleri + notlar)
komple sağ panele taşınıyordu. Details paneli bir "nesne müfettişi" değil,
küçültülmüş bir form yığınıydı.

### 1.4 Ağaç gerçek nesne taşımıyordu

`QTreeWidget` + düz kategori listesi (`Geometri`, `Malzemeler`, `Kesitler`,
`Mesh`, `Analizler`, `Sonuçlar`). Düğüm kimliği **görünen metindi**:

```cpp
// MainWindow.cpp:389 — sonuç ağacı güncellemesi
if (root->text(0) != tr("Sonuçlar")) { continue; }
```

Ayrıca sonuçlar ağaca serbest metin olarak yazılıyordu
(`"Tip Deplasman: 0.123 mm"`), gerçek result object olarak değil.

### 1.5 Global QSS teması

`MainWindow::applyMacStyle()` `QMainWindow`, `QToolBar`, `QTreeWidget`,
`QTabWidget`, `QPushButton`, `QDoubleSpinBox`, `QComboBox`, `QStatusBar`
sınıflarını sabit renklerle boyayan bir stylesheet uyguluyordu
(`background: #f5f5f7`, `border: 1px solid #d7d7dc` …). Bu, macOS Dark Mode'da
kalıcı açık renk kalıntısı üretiyordu. `AppearanceController` bunu sonradan
temizlemeye çalışıyordu — yani bir katman diğerinin hasarını onarıyordu.

### 1.6 Solver implementasyonunun kullanıcıya sızması

Analiz sayfası doğrudan şunları gösteriyordu: `Displacement-only / penalty`,
`Mixed u-p HEX8/P0 verification`, `Mixed Shear γ`, `Contact k_n / E`,
`Full Newton-Raphson`, `Backtracking line search`, `Adaptive increment / cutback`.
Normal kullanıcı için bunlar niyet değil, implementasyon detayıdır.

### 1.7 Viewport'ta semantik yokluğu

`ViewportWidget` her çağrıda `RemoveAllViewProps()` yapıp sabit RGB'lerle yeni
aktörler kuruyordu (`SetColor(0.76, 0.78, 0.82)`). Renkler rol değil, çağrı
yerine gömülüydü; renderer arka planı sabit açık griydi
(`SetBackground(0.965, 0.968, 0.975)`).

### 1.8 Alt panel sürekli açık

`QDockWidget("Sonuçlar ve Çözüm Günlüğü")` başlangıçta ve boşken de açıktı;
3B alanı sürekli tüketiyordu.

### 1.9 Geometri ile mesh birbirine bağlı değildi

`PrePostPanel` kutu ölçülerini kendi spin box'larından alıyordu. İçe aktarılan
STEP gövdesinin sınır kutusu mesh'e hiç aktarılmıyordu; `axisAlignedBoxDescriptor`
API'si mevcut olmasına rağmen GUI'de kullanılmıyordu. Sınır şartları sentetik
kimliklere (101…106) sabitlenmişti.

---

## 2. Yeni mimari

Ayrıntı için **`GUI_ARCHITECTURE.md`**. Özet:

```
Dynamics26MainWindow            ← görünür kompozisyonun TEK sahibi
├─ CommandRegistry              ← tüm komutlar tek QAction kayıt defterinde
├─ ProjectNavigator             ← QTreeView + ProjectTreeModel (QAbstractItemModel)
├─ GraphicsWorkspace            ← ViewportWidget + ince grafik araç çubuğu
├─ DetailsHost                  ← ObjectType → Details sayfası
│   ├─ GeometryDetails   ├─ MaterialDetails  ├─ MeshDetails
│   ├─ AnalysisDetails   ├─ BoundaryConditionDetails
│   ├─ ResultDetails     └─ ObjectDetails
├─ UtilityWorkspace             ← Messages/Convergence/SolverOutput/Results/Timings
└─ EngineeringStatusBar

ServiceContext (explicit bağımlılık) ─► GeometryService · MeshService
                                        MaterialService · AnalysisService
```

Belirleyici kararlar:

1. **Tek sahiplik.** `main.cpp` yalnız pencereyi kurar. Sonradan müdahale eden
   ikinci bir katman yoktur.
2. **Nesne kimliği metinden bağımsız.** `ObjectId` + `ObjectType` sistem genelinde
   tek kimlik uzayıdır; `AnalysisService` sınır şartlarını da bu kimlikle saklar.
3. **Mühendislik durumu servislerde.** Widget ömrüne bağlı state kalmadı.
4. **Semantik render rolleri.** Aktörler rolüyle kaydedilir, görünüm değişiminde
   role göre yeniden boyanır.
5. **Niyet ≠ implementasyon.** Kullanıcı `Incompressibility: Automatic` seçer;
   `Mixed u-p / HEX8-P0 / dense direct` yalnız Advanced altında salt-okunur görünür.
6. **Fake yok.** Arka ucu olmayan komut hiç eklenmez; bağlı ama kullanılamaz
   komut pasif kalır ve **nedeni** tooltip + Details'ta yazar.

---

## 3. Eklenen dosyalar

`gui/` ağacı: **60 dosya, ~8 900 satır**.

### `gui/core` (1 408 satır)
| Dosya | Rol |
|---|---|
| `ProjectTypes.h/.cpp` | `ObjectType`, `ObjectState`, `AnalysisType`, `IncompressibilityIntent`, `ViewportContext`, `Severity` |
| `ProjectModel.h/.cpp` | proje nesne grafiğinin sahibi |
| `ProjectTreeModel.h/.cpp` | `QAbstractItemModel` adaptörü |
| `CaeIcons.h/.cpp` | semantik CAE ikon seti (QPainter, ek bağımlılık yok) |
| `UiTheme.h/.cpp` | paletten türetilen ikincil metin/ayraç/satır gölgesi + `SecondaryLabel`, `Hairline` |
| `ServiceContext.h` | explicit bağımlılık taşıyıcısı |

### `gui/services` (1 712 satır)
`GeometryService`, `MeshService`, `MaterialService`, `AnalysisService`.

### `gui/viewport` (1 142 satır)
`RenderRoles.h/.cpp` (rol paleti), `ViewportWidget.h/.cpp` (semantik VTK katmanı,
feature-edge CAD kenarları, BC/yük sembolleri, kontur + legend, `vtkCellPicker`
ile yüz seçimi).

### `gui/details` (1 706 satır)
`DetailsPage` altyapısı + 7 sayfa.

### `gui/shell` (2 430 satır)
`CommandRegistry`, `ProjectNavigator`, `GraphicsWorkspace`, `DetailsHost`,
`UtilityWorkspace`, `EngineeringStatusBar`, `Dynamics26MainWindow`.

### `gui/support` (~380 satır)
- `ScreenshotDriver` — `--capture` ile uygulamayı gerçek akıştan geçirip
  `docs/gui-preview/` görsellerini üretir.
- `SelfTest` — `--selftest` ile aynı akışı çalıştırıp 45 mühendislik kontrolünü
  doğrular (mesh boyutu, kalite, çözüm dengesi, kaydet/aç round-trip, dürüst
  kısıtlar, görünüm geçişi).

---

## 4. Değiştirilen dosyalar

| Dosya | Değişiklik |
|---|---|
| `gui/main.cpp` | tamamen yeniden yazıldı; tek kompozisyon kökü + `--capture` / `--import-step` geliştirici bayrakları. `--bundle-smoke` protokolü **aynen korundu** |
| `gui/CMakeLists.txt` | yeni kaynak ağacı; VTK bileşenlerine `FiltersCore`, `RenderingAnnotation`, `RenderingFreeType` eklendi |
| `.github/workflows/macos-self-hosted.yml` | "Alpha.1 shell contract gate" → **"V1.1 CAE shell architecture gate"** (aşağıda) |
| `SHA256SUMS.txt` | kaynak manifesti yeniden üretildi |

Taşınan (içerik değişmedi): `MaterialCurveWidget.h/.cpp` → `gui/widgets/`.

---

## 5. Kaldırılan legacy GUI parçaları

| Dosya | Satır | Neden |
|---|---:|---|
| `gui/MainWindow.h/.cpp` | 1 030 | monolitik pencere + sekmeli form inspector + global QSS |
| `gui/Dynamics26Shell.h/.cpp` | 1 079 | legacy splitter'ı söküp yeniden etiketleyen düzeltme katmanı |
| `gui/CaeWorkbenchController.h/.cpp` | 625 | ikinci orchestration katmanı, widget keşfi |
| `gui/AppearanceController.h/.cpp` | 242 | önceki katmanların QSS hasarını onaran katman |
| `gui/Alpha1UxController.h/.cpp` | 487 | metin eşleştirmeli üçüncü düzeltme katmanı |
| `gui/Alpha1ProductPolish.h/.cpp` | 309 | dördüncü düzeltme katmanı |
| `gui/GeometryPanel.h/.cpp` | 315 | mühendislik durumunu taşıyan büyük widget → `GeometryService` |
| `gui/PrePostPanel.h/.cpp` | 257 | mesh/solve durumunu taşıyan büyük widget → `MeshService` + `AnalysisService` |
| `gui/ViewportWidget.h/.cpp` | 420 | sabit renkli, rolsüz viewport → `gui/viewport/` |
| `gui/ALPHA1_*.md` | — | kaldırılan corrective mimariyi anlatan notlar |

**Toplam ~4 760 satır legacy GUI kodu kaldırıldı.**

Bu dosyalardaki **çalışan mühendislik mantığı silinmedi**, servislere taşındı:
STEP import + tessellation → `GeometryService`; structured HEX8 + kalite →
`MeshService`; assignment + `fem_solve_linear_hex8_mesh` + export →
`AnalysisService`; hyperelastic doğrulama/önizleme → `MaterialService`.

---

## 6. Backend'de korunan parçalar

Aşağıdakilerin **hiçbiri değiştirilmedi**:

- Modern Fortran solver çekirdeği (`src/**/*.f90`) — tek satır dokunulmadı
- C ABI (`include/femcae/femcae.h`, `src/api/fem_c_api.f90`)
- `femcae_geometry`: `GeometryDocument`, `OcctStepImporter` (STEP/XDE),
  `DxfSectionReader`, `SectionProfile`
- `femcae_meshing`: `StructuredHexMesher`, `evaluateHexMeshQuality`,
  `AssignmentStore`, `AssignmentResolver`, `ResultDatabase` (CSV/legacy-VTK
  export, probe, section cut), `AbaqusInpMeshReader`, `GeometryMeshBridge`
- `gui/ProjectFileMigrator.h/.cpp` ve `gui.project_schema_migration` testi
- Proje JSON şeması (`material` / `section` / `load` / `geometry` / `prepost`)
- CMake / Ninja / CTest yapısı, macOS bundle + deploy + fixup zinciri
- `--bundle-smoke` çıktı sözleşmesi

**CAD ≠ display tessellation ≠ FEM mesh** ayrımı korundu ve güçlendirildi:
`OcctStepImporter::tessellate()` çıktısı yalnız `ViewportWidget::showGeometry()`
içine gider; mesher'a giden tek geometri bilgisi
`axisAlignedBoxDescriptor()`'dan gelen **sınır kutusu + gerçek CAD yüz
kimlikleri**dir.

---

## 7. Yeni kazanılan mühendislik yetenekleri

Yalnız görsel değil, işlevsel iyileştirmeler:

1. **Geometri → mesh provenance zinciri.** STEP gövdesi eksen hizalı kutuysa
   mesh ölçüleri CAD'den devralınır ve sınır şartları **gerçek STEP yüz
   kimliklerine** kapsamlanır (önceden 101…106 sentetik sabitleri kullanılıyordu).
2. **Çoklu sınır şartı ve yük.** `AnalysisService` birden fazla Fixed Support /
   Force taşır; her biri ayrı yüze kapsamlanır, DOF bazında kısıtlanabilir
   (X/Y/Z), yük üç bileşenli verilir. Önceden tek sabit BC + tek X yükü vardı.
3. **Gerçek result nesneleri.** Solve sonrası `Solution` altında
   `Total Deformation` / `Equivalent Stress` / `Reaction Force` nesneleri oluşur;
   her biri kendi konturunu ve Details'ini açar.
4. **Yüz seçimi.** `vtkCellPicker` ile viewport'ta sınır yüzü seçilir; durum
   çubuğu ve kapsam vurgusu güncellenir.
5. **DOF koruması.** `fem_solve_linear_hex8_mesh` çekirdekte
   `LINEAR_SOLVER_DENSE_REFERENCE` kullanır (n×n yoğun matris). GUI bunu
   Advanced'da açıkça yazar ve 6 000 DOF üstünde Solve'u nedeniyle birlikte
   pasifleştirir. Önceden bu sınır hiçbir yerde görünmüyordu.
6. **Doğrulama preset'leri korundu.** Mixed u-p, temas, Total-Lagrangian ve
   eksenel modal çekirdek yolları `Analiz ▸ Solver Doğrulama Preset'leri`
   altında, **model bağımsız oldukları açıkça yazılarak** erişilebilir kaldı.
   Nonlineer preset gerçek yakınsama geçmişini Convergence sekmesine doldurur.

---

## 8. Tamamlanmayan / kapsam dışı bırakılan özellikler

Bunlar arayüzde **çalışıyormuş gibi gösterilmez**; ilgili komut pasiftir ve
nedeni yazar.

| Özellik | Durum | Arayüzdeki karşılığı |
|---|---|---|
| Modal analiz (model tabanlı) | çekirdekte var, GUI çözüm akışına bağlı değil | tree'ye eklenebilir, Solve pasif + açıklama |
| Nonlinear Static (model tabanlı) | aynı | aynı |
| Mixed u-p (keyfi mesh) | yalnız doğrulama preset'i | `Automatic` + ν ≥ 0.475 seçilirse Solve pasif + açıklama |
| Large Deflection | GUI akışı yok | combo var, açık seçilirse Solve pasif + açıklama |
| Contact Region / Joint nesneleri | çekirdekte temas var, model tanımı yok | `Connections` boş, Details nedenini yazar |
| Keyfi STEP hacim meshleme | structured HEX8 yalnız eksen hizalı kutu | kutu olmayan gövdede parametrik kutuya düşer, Details bildirir |
| CAD yüz seviyesinde seçim (Geometry bağlamı) | üçgen→face provenance yok | Geometry'de gövde seviyesi seçim; yüz seçimi mesh üretildikten sonra |
| Hyperelastic malzeme ile Static Structural | yalnız doğrulama/önizleme | Solve pasif + açıklama |
| Named Selection, Measure, Probe, Section Cut komutları | arka uç kısmi | komut hiç eklenmedi (fake buton yok) |
| Kesit (Section) nesnesi atama akışı | DXF okunuyor, atama yok | Sections Details özellikleri gösterir, atama sunmaz |

---

## 9. Bilinen sorunlar

1. **Çözüm eşzamanlıdır (senkron).** Doğrulanmış Fortran çekirdeğine iş parçacığı
   güvenliği varsayımı eklememek için solve, UI thread'inde çalışır. Mesh boyutu
   6 000 DOF ile sınırlandığından pratikte saniyeler mertebesindedir, ancak
   çözüm süresince pencere yanıt vermez (imleç meşgul durumuna geçer).
   Çözüm: worker thread + çekirdek reentrancy denetimi (Alpha.2).
2. **`Stop` komutu yoktur.** Senkron çözümde iptal edilebilir bir nokta olmadığı
   için eklenmedi (fake buton yapmamak adına).
3. **`--capture-appearance`** `QStyleHints::setColorScheme` kullanır. Yalnız
   belgeleme çekimi yolunda çağrılır; normal çalıştırmada görünüm kaynağı
   macOS System Appearance'tır.
4. **Ekran görüntüsü kompoziti.** `QWidget::grab()` QOpenGLWidget içeriğini her
   zaman getirmediğinden VTK karesi ayrıca okunup viewport dikdörtgenine
   bindirilir. Bu yalnız `--capture` yolunu etkiler.
5. **Undo/Redo yoktur.** Komut sistemi `QAction` tabanlıdır; komut nesnesi
   (undo stack) modeli Alpha.2 kapsamındadır.
6. **Tek malzeme ataması.** `MaterialService` çoklu malzemeyi taşıyacak şekilde
   kurgulandı, ancak Alpha.1'de model başına tek atama vardır.
7. **Türkçe locale ondalık ayracı.** Sayısal alanlar sistem locale'ini kullanır;
   Türkçe'de `100,00 mm` görünür. Bilinçli tercihtir (native davranış).

---

## 10. Build / test sonucu

Ortam: macOS 26.6.2, Apple Silicon (arm64), Xcode Command Line Tools,
Qt 6.11.1, VTK 9.7.0, OpenCASCADE 7.9.3, gfortran (Homebrew), CMake + Ninja.

| Adım | Komut | Sonuç |
|---|---|---|
| Configure (Release GUI) | `cmake --preset macos-release-gui` | ✅ başarılı — OCCT 7.9.3, ARPACK-NG, VTK viewport etkin |
| Temiz build | `cmake --build build/macos-release-gui -j 8` | ✅ **0 hata, 0 uyarı** (918 hedef) |
| Tam test matrisi | `ctest --test-dir build/macos-release-gui -j 8` | ✅ **127 / 127 geçti** (35.05 s) |
| Proje şema migration | `gui.project_schema_migration` | ✅ geçti (değişmedi) |
| Bundle smoke | `FEMCAE --bundle-smoke` | ✅ `FEMCAE bundle smoke PASS version=1.0.2` |
| Configure + build (Debug Core, GUI kapalı) | `cmake --preset macos-debug-core` | ✅ başarılı |
| Mimari gate (yerel çalıştırma) | CI gate adımlarının birebir kopyası | ✅ geçti |

Baseline (değişiklik öncesi) da aynı ortamda derlenip test edilmişti:
**127 / 127**. Yani **regresyon yoktur**.

### GUI öz-testi (`FEMCAE --selftest`)

`gui/support/SelfTest.cpp` uygulamayı gerçek kullanıcı akışından geçirip her
adımın **mühendislik sonucunu** doğrular. macOS'ta çalıştırıldı:

```
SELFTEST PASS   failures=0      (45 kontrol)
```

Kapsanan kontroller:

- başlangıç nesne grafiği (Project / Model / Geometry / Body / Materials / Mesh /
  Static Structural / BC / Load) kuruldu
- **çözüm öncesi sahte sonuç düğümü yok**, alt yardımcı alan kapalı
- Generate Mesh → 11×3×3 = 99 düğüm, 10×2×2 = 40 HEX8, 0 ters eleman,
  min scaled Jacobian ≈ 1
- Solve → deplasman/gerilme pozitif, üç result nesnesi oluştu
- **denge kontrolü:** ΣRx uygulanan yükü 1e-6 bağıl toleransla dengeliyor
  (gerçek Fortran çözümü üzerinde fizik doğrulaması)
- CSV + legacy VTK dışa aktarımı diske yazıldı
- hyperelastic engine doğrulaması geçti, 41 noktalı uniaxial eğri, G₀ > 0
- **dürüst kısıt kontrolleri:** hyperelastic malzeme atanınca Solve pasifleşiyor,
  lineer malzemeye dönünce tekrar aktifleşiyor; `Nearly Incompressible`
  seçilince mixed u-p'ye çözülüyor ve Solve pasifleşiyor; `Automatic` + ν = 0.30
  displacement-based'e çözülüyor
- **proje kaydet → yeni → aç round-trip:** uzunluk, Nx, Young modülü ve yük
  değerleri korunuyor
- Light → Dark → Light geçişi

Bu test **CTest'e kaydedilmedi**: pencere sunucusu gerektirdiği için headless
CI'da çalışamaz. Yerel doğrulama aracıdır.

### Uçtan uca akış doğrulaması (macOS'ta gerçekten çalıştırıldı)

`ScreenshotDriver` uygulamayı otomatik olarak şu akıştan geçirdi ve her adımı
görüntüledi:

| Akış | Sonuç |
|---|---|
| Uygulama açılışı | ✅ parametrik kutu gövdesi + Static Structural 1 hazır |
| Geometry seçimi | ✅ Details + viewport bağlamı doğru |
| **STEP import** (`--import-step`, OCCT ile yazılmış 120×30×25 mm kutu) | ✅ 1 body / 6 face / 12 edge, `Source: From Geometry`, ölçüler CAD'den devralındı |
| Mesh sayfası | ✅ sizing/divisions/statistics/quality gerçek değerlerle |
| **Generate mesh** | ✅ 525 node, 320 HEX8, 352 boundary facet, min scaled Jacobian 1.0000, max aspect 1.250, 0 inverted |
| Analysis seçimi | ✅ `Ready to Solve`, Advanced'da resolved formulation görünür |
| Boundary Condition seçimi | ✅ `X-Min Face · 16 facet · 25 node`, viewport'ta kapsam vurgusu + sembol |
| **Linear solve** (parametrik kutu) | ✅ max \|u\| = 0.00134138 mm, max von Mises = 4.08912 MPa |
| **Linear solve** (STEP kutusu) | ✅ max \|u\| = 0.0008778 mm, max von Mises = 2.23901 MPa |
| Results seçimi | ✅ kontur + legend + result Details |
| Light görünüm | ✅ |
| Dark görünüm | ✅ kalıntı renk yok |
| Alt yardımcı alan | ✅ başlangıçta kapalı, Tanılama ile açılıyor, gerçek log içeriyor |

Görseller: **`docs/gui-preview/`** (19 PNG — light + dark + STEP import akışı).
Bunlar mockup değildir; çalışan uygulamanın otomatik çıktısıdır ve
`FEMCAE --capture <dizin> [--capture-appearance light|dark]` ile yeniden
üretilebilir.

---

## 11. Kabul kriterleri karşılaştırması

| # | Başarısızlık koşulu | Durum |
|---:|---|---|
| 1 | Graphics merkezde baskın değil | ✅ ~%60 genişlik, tek merkezi alan |
| 2 | Eski `GeometryPanel` Details içine gömülmüş | ✅ sınıf tamamen kaldırıldı |
| 3 | Eski `PrePostPanel` Details içine gömülmüş | ✅ sınıf tamamen kaldırıldı |
| 4 | Bottom alan boşken sürekli açık | ✅ başlangıçta kapalı, yalnız gerçek olayla açılır |
| 5 | Tree yalnız kategori isimlerinden oluşuyor | ✅ `ObjectId`/`ObjectType` taşıyan gerçek nesne ağacı |
| 6 | Toolbar rastgele ikonlardan oluşuyor | ✅ semantik CAE ikon seti |
| 7 | Çalışmayan fake button var | ✅ arka ucu olmayan komut eklenmedi; kısıtlı olanlar pasif + gerekçeli |
| 8 | Normal kullanıcı solver implementasyonu görüyor | ✅ yalnız `▸ Advanced Solver Settings` altında |
| 9 | Global dev QSS theme kullanılmış | ✅ `setStyleSheet` gui ağacında sıfır; CI gate ile korunuyor |
| 10 | Light/Dark karışıyor | ✅ tam rol paleti + `SecondaryLabel`/`Hairline` palet takibi |
| 11 | CAD tessellation FEM mesh olarak kullanılıyor | ✅ üçgenleme yalnız `showGeometry()`'ye gider |
| 12 | Solver backend gereksiz yeniden yazılmış | ✅ `src/` altında tek satır değişmedi |
| 13 | UI hâlâ generic Qt form gibi görünüyor | ✅ bkz. `docs/gui-preview/` |

---

## 12. Sürüm numarası hakkında

`CMakeLists.txt` içindeki `PROJECT_VERSION` bilinçli olarak **1.0.2** bırakıldı.
Gerekçe: `--bundle-smoke` çıktısı ve macOS bundle audit zinciri geçmiş release
kanıtlarına bağlıdır ve bu sözleşme V1.1 alpha aşamasında değiştirilmez
(mevcut `gui/main.cpp` yorumunun da belirttiği kural). GUI kilometre taşı ayrı
bir derleme tanımıyla taşınır:

```cmake
DYNAMICS26_GUI_MILESTONE="1.1.0-alpha.3.1"
```

Bu değer Project Details sayfasında `GUI Milestone` satırında ve açılış
mesajında görünür.

---

## 13. Mimari koruma (regresyon önleme)

`.github/workflows/macos-self-hosted.yml` içindeki gate, eski mimarinin geri
gelmesini engeller: kaldırılan corrective katman dosyalarının varlığı,
`findChild/findChildren`, `text().contains(...)`, `setStyleSheet`, uygulama
çapında `setPalette`, `setUnifiedTitleAndToolBarOnMac(true)` ve `Qt::META`
kullanımı build'i kırar.

---

# BÖLÜM II — Alpha.2: CAE Desktop Interaction Foundation

Alpha.1 mimarisi **korundu ve üzerine inşa edildi**. Hiçbir corrective katman
geri gelmedi; `QTreeWidget`, `findChildren`, metin eşleştirme ve global QSS
yasakları CI gate ile hâlâ zorunlu.

`gui/` ağacı: **68 dosya, ~13 260 satır** (Alpha.1: 60 dosya, ~8 900 satır).

## II.1 — Dark Mode blocker (P0.2) · ÇÖZÜLDÜ

### Kök neden

`ui::SecondaryLabel::applyColor()` ikincil metin rengini **widget'ın kendi
paletinden** türetiyordu:

```cpp
// HATALI (Alpha.1)
QPalette labelPalette = palette();                                   // ← zaten mutasyona uğramış
labelPalette.setColor(QPalette::WindowText, secondaryText(labelPalette, …));
QLabel::setPalette(labelPalette);
```

Bir widget'a açıkça renk atandığında Qt o rolü **"resolved"** işaretler ve
görünüm değişiminde native paletten güncellemez. Dolayısıyla Light → Dark
geçişinde:

1. `QPalette::Window` güncellenir → `isDark()` doğru şekilde `true` döner,
2. fakat `QPalette::WindowText` **eski (koyu) değerinde kalır**,
3. o koyu renge yeniden alfa uygulanır → **koyu zemin üzerinde koyu metin**.

`GEOMETRY`, `DEFINITION`, `DISPLAY`, `ACTIONS`, `MESH`, `STATISTICS`,
`QUALITY`, `ANALYSIS` gibi bölüm başlıkları bu yüzden Dark Mode'da okunmuyordu.

### Çözüm

Türetmenin tek kaynağı artık **`QApplication::palette()`**:

```cpp
QPalette applicationPalette() { return QApplication::palette(); }   // tek doğruluk kaynağı

void SecondaryLabel::applyColor() {
    if (applying_) return;                     // setPalette yeni PaletteChange üretir
    applying_ = true;
    const QColor color = secondaryText(lightAlpha_, darkAlpha_);   // app paletinden
    QPalette p = palette();
    p.setColor(QPalette::WindowText, color);
    p.setColor(QPalette::Text, color);         // QLabel bağlama göre bu rolü çözebilir
    p.setColor(QPalette::ButtonText, color);
    QLabel::setPalette(p);
    applying_ = false;
}
```

Ek olarak:

- `hairlineColor()` / `rowShadeColor()` de app paletinden türetilir ve
  `Hairline::paintEvent` her boyamada yeniden okur (saklanan renk yok).
- Koyu görünümde ikincil metin alfası yükseltildi (0.62 → 0.82); aynı alfa iki
  modda aynı okunabilirliği vermiyor.
- Yeni `ui::statusColor(StatusTone)` — durum rozeti renkleri de görünüme göre
  ton alır. `CaeIcons::forState` ve `NavigatorDelegate` artık sabit RGB yerine
  bunu kullanır.

Global QSS veya `QApplication::setPalette` **yine kullanılmıyor**; CI gate
`UiTheme.cpp` içinde `QApplication::palette()` çağrısını zorunlu kılar.

### Doğrulama

Öz-testte otomatik: `Light → Dark → Light` çevrimi sonunda ikincil metin
renginin ilk değerine döndüğü ve Dark'ta Light'takinden daha açık olduğu
ölçülür. Ekran görüntüsü: `docs/gui-preview/04-static-structural-dark.png`.

## II.2 — Qt 6.5+ uyumluluğu (P0.3)

`QStyleHints::setColorScheme()` / `unsetColorScheme()` Qt **6.8+** API'sidir ve
Alpha.1'de korumasız kullanılıyordu. CMake minimum sürüm 6.5 olarak **korundu**;
çağrılar sürüm koruması altına alındı:

```cpp
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#else
    std::cout << "note: --capture-appearance requires Qt 6.8+; using system appearance\n";
#endif
```

Bu yol yalnız `--capture` (belgeleme) ve `--selftest` (görünüm regresyon testi)
içinde çağrılır. Normal çalıştırmada görünüm kaynağı macOS System Appearance'tır.
CI gate korumasız `setColorScheme` kullanımını ve Qt minimum sürümünün
yükseltilmesini engeller.

## II.3 — Tam nesne kalıcılığı (P0.4)

Proje dosyası artık **iki katmanlıdır**: V1.0 uyumluluk anahtarları + tam nesne
grafiğini taşıyan `dynamics26_document` bölümü (ayrıntı: `GUI_ARCHITECTURE.md` §15).

Round-trip garantisi: ObjectId · DisplayName · ordering · analiz türü · large
deflection · incompressibility intent · suppression · Fx/Fy/Fz · scope · birden
fazla malzeme/mesnet/yük · sonuç tanımları · ağaç hiyerarşisi.

Bu turda çözülen **iki gerçek kalıcılık hatası**:

1. **ObjectId çakışması.** Kimlik sayacı yükleme *sonrasında* rezerve ediliyordu;
   ara düğümlere otomatik atanan kimlikler dosyadan gelen açık ObjectId'lerle
   çakışıp nesne kaybına yol açıyordu. Sayaç artık yüklemeden **önce** rezerve
   edilir ve `AnalysisSettings` / `Solution` düğüm kimlikleri de dosyada saklanır.
2. **Sıralama sadakati.** Sınır şartları ve yükler ayrı dizilerde saklanınca
   ağaçtaki gerçek sıra (Support, Force, Support, Force) kayboluyordu. Artık
   `boundary_conditions` adlı **tek sıralı liste** kullanılır ve
   `AnalysisService::resyncChildLists()` iç listeleri her zaman ağaç sırasından
   türetir.

Eski V1.0 dosyaları açılmaya devam eder; tek malzeme ve tek skaler yük
varsayılan nesne grafiğine uygulanır ve kullanıcı uyarılır. Yeni nesne modeli
eski `force_n` alanına **sıkıştırılmaz**.

## II.4 — Doküman komut sistemi (P0.5–P0.7)

Undo/Redo iki araç çubuğu düğmesi olarak değil, **gerçek domain command
mimarisi** olarak uygulandı: `DocumentCommandManager` (QUndoStack) + **18 domain
command sınıfı** (`gui/commands/DomainCommands.h`).

- Mutasyon akışı: `UI → Domain Command → Service → Domain Object → DependencyEngine → UI`
- Oluşturma komutları redo'da **aynı ObjectId'yi** yeniden kullanır.
- Silme komutları nesnenin tam durumunu **ve ağaçtaki satırını** saklar.
- Özellik komutları **700 ms'lik zaman penceresi** içinde birleşir (spinbox
  sürükleme tek adım; ayrı düzenlemeler ayrı adım).
- Edit menüsünün en üstünde `⌘Z` / `⇧⌘Z`, **dinamik metinle**:
  *"Geri Al Add Force"*, *"Geri Al Change Mesh Divisions"*.
- Dirty/Clean `QUndoStack::setClean()` / `isClean()` üzerinden; kaydedilen
  noktaya Undo ile dönmek dokümanı tekrar **temiz** yapar.
- Pencere başlığı: `Dynamics26 — model.femcae.json — Düzenlendi`.
- Durum çubuğunda `Düzenlendi` göstergesi.

**View state** (kamera, Fit View, panel görünürlüğü, seçim, aktif sekme) ve
**derived data** (üretilmiş mesh, hesaplanmış sonuç alanları) yığına girmez.

## II.5 — Bağımlılık motoru (P0.11)

`DependencyEngine` nesne durumlarının **tek yazarıdır**. Servisler durum yazmayı
tamamen bıraktı (yalnız ad senkronlar).

Bu turda çözülen **iki bayatlık hatası** — her ikisi de monoton sayaç
kullanımından kaynaklanıyordu:

1. **Mesh.** `settingsRevision` sayacı, bir ayar değişikliğini Undo ile geri
   almayı da "değişiklik" sayıyordu; mesh sonsuza dek bayat kalıyordu. Artık
   üretim anındaki `Definition` saklanır ve **içerik karşılaştırması** yapılır —
   ayarları geri almak mesh'i yeniden geçerli kılar.
2. **Çözüm.** Aynı sorun çözüm için de vardı; ayrıca mesh *ayarı* değişince
   çözüm bayat sayılmıyordu. Artık `solverInputSignature()` solver'ın gerçekten
   tükettiği girdilerin (mesh + malzeme E/ν + **aktif** BC/yükler + formülasyon)
   JSON imzasını üretir. İmza değişirse sonuç `OutOfDate`; imza aynıysa yeniden
   geçerli. **Nesneyi yeniden adlandırmak sonuçları bayatlatmaz** — çünkü solver
   girdisi değildir.

## II.6 — Nesne düzenleme ve bağlam menüleri (P0.8–P0.10)

- **Rename** — `F2`, Edit menüsü, bağlam menüsü ve ağaç içi düzenleme.
  `ProjectTreeModel::setData()` adı doğrudan yazmaz; `renameRequested` sinyali
  yayınlar, kabuk bunu `RenameObjectCommand`'e çevirir. Domain nesnesinin
  DisplayName'i değişir, yalnız ağaç metni değil.
- **Duplicate** (`⇧⌘D`) — yeni ObjectId üretir.
- **Delete** (`Del`) — undo nesneyi aynı kimlik, ad, kapsam ve **konumla** geri getirir.
- **Cut / Copy / Paste** (`⌘X` / `⌘C` / `⌘V`) — sistem panosu üzerinden,
  `application/x-dynamics26-object+json` MIME tipiyle. Yapıştırma yeni kimlik atar.
- **Suppress / Unsuppress** — `Body`, `FixedSupport`, `Force` ve sonuç
  tanımlarında. Bastırılmış nesne modelde kalır, soluk+italik çizilir, solver
  girdisine ve preflight'a **girmez**, undoable'dır.
- **Select All** — çoklu seçim bu sürümde yok; komut **pasif ve gerekçeli**
  (sahte komut eklenmedi).
- **Bağlam menüleri** nesne türüne duyarlıdır ve menü/araç çubuğuyla **aynı
  QAction'ları** paylaşır; bir komutun etkin/pasif durumu tek yerden yönetilir.
- Ağaç üretkenliği: Expand All / Collapse All.

## II.7 — Yaşam döngüsü: Preflight, Clear (P0.12–P0.14)

**Preflight (`⌘R`)** — Solve doğrudan solver'ı çağırmaz. 12 kontrol
(analiz türü, geometri, malzeme, mesh varlığı/güncelliği/kalitesi, aktif sınır
şartı, aktif yük, formülasyon, large deflection, çözücü DOF kapasitesi, sonuç
tanımı) çalışır; rapor `AnalysisDetails` içindeki **`VALIDATION`** bölümünde ve
Messages sekmesinde gösterilir. Her kontrol bir `subject` ObjectId taşır.
Rapor geçmezse çekirdeğe hiç gidilmez.

**Clear Generated Mesh** — yöntem/ölçü/bölme korunur, üretilmiş düğüm-eleman-
kalite verisi silinir.

**Clear Solution** — analiz, ayarlar, BC/yükler ve **sonuç tanımları** korunur,
yalnız hesaplanmış alan değerleri silinir.

**Sonuç tanımları artık model durumudur.** `Total Deformation`,
`Equivalent Stress` ve `Reaction Force` analiz oluşturulduğunda **gerçek nesne
olarak** doğar (ANSYS'teki gibi), çözümden önce `NotReady` rozeti taşır ve
undoable şekilde eklenip silinebilir. Hesaplanmış değerler türetilmiş veridir.

**Stop / Cancel eklenmedi**: çözüm eşzamanlı çalıştığı için iptal edilebilir bir
nokta yoktur ve sahte bir komut üretilmez.

## II.8 — Standart macOS menüleri ve kısayolları (P0.14)

`Dosya` (Yeni · Aç · **Son Kullanılanlar** · Kaydet · **Farklı Kaydet** ·
**Kaydedilene Dön** · Import Geometry · **Kapat**) · `Düzenle` (Undo · Redo ·
Kes · Kopyala · Yapıştır · Çoğalt · Sil · Yeniden Adlandır · Bastır · Tümünü
Seç) · `Geometri` · `Malzeme` · `Mesh` · `Analiz` · `Sonuçlar` · `Görünüm` ·
**`Yardım`** (Klavye Kısayolları · Sistem Bilgisi · Hakkında).

Kısayollar: `⌘N ⌘O ⌘S ⇧⌘S ⌘W ⌘Z ⇧⌘Z ⌘X ⌘C ⌘V ⇧⌘D ⌘A F2 Del F7 ⌘R F5 ⌘0 ⌘1`.
Standart macOS kısayolları başka işlere atanmadı.

Kaydedilmemiş değişiklik varken Yeni / Aç / Kapat işlemleri **Kaydet / Vazgeç /
İptal** sorar.

## II.9 — Bu turda düzeltilen gerçek hatalar

Öz-test bu hataları **bulup** doğruladı:

| # | Hata | Etki |
|---|---|---|
| 1 | `SecondaryLabel` rengini kendi mutasyona uğramış paletinden türetiyordu | Dark Mode'da bölüm başlıkları okunmuyordu |
| 2 | Mesh bayatlığı monoton sayaçla ölçülüyordu | ayar değişikliğini Undo ile geri almak mesh'i geçerli kılmıyordu |
| 3 | Çözüm bayatlığı mesh *ayarı* değişimini görmüyordu | bayat mesh üzerindeki sonuçlar "güncel" görünüyordu |
| 4 | Kimlik sayacı yüklemeden *sonra* rezerve ediliyordu | proje açılışında ObjectId çakışması → nesne kaybı |
| 5 | BC/yükler iki ayrı dizide saklanıyordu | ağaç sıralaması round-trip'te bozuluyordu |
| 6 | `QToolBar::clear()` bağlam başlığı widget'ını yok ediyordu (Alpha.1'den kalma) | asılı işaretçi |
| 7 | Bayat sonuç durum çubuğunda "çözüme hazır değil" gösteriliyordu | yanıltıcı: bayat sonuç çözümü engellemez |

## II.10 — Bu turda tamamlanmayan / ertelenen (P1/P2)

Hiçbiri sahte kontrol olarak gösterilmiyor.

| Özellik | Durum |
|---|---|
| Çoklu seçim + Select All | `edit.selectAll` pasif + gerekçeli |
| Stop / Cancel solve | komut yok (senkron çözüm) |
| Named Selection UI | scope kimliği mimarisi hazır, UI yok |
| Face/Edge/Vertex tam OCCT picking | mesh yüzü seçimi var; CAD yüz seviyesi yok |
| Node/Element seçimi | yok |
| Ağaç arama / filtre / grup / drag-drop | yok |
| Measure · Section Plane · Probe | komut eklenmedi |
| Gelişmiş mesh sizing (local/refinement) | yok |
| Contact / Joint tanımı | `Connections` boş, Details nedenini yazar |
| Model tabanlı Modal / Nonlinear | preflight nedenini yazar, Solve pasif |
| Autosave / crash recovery | yok |
| Charts / modal animasyon | yok |

## II.11 — Bilinen sorunlar (Alpha.2)

1. **Çözüm eşzamanlıdır.** Doğrulanmış Fortran çekirdeğine iş parçacığı
   güvenliği varsayımı eklenmedi; mesh 6 000 DOF ile sınırlı. Çözüm süresince
   pencere yanıt vermez (imleç meşgul durumuna geçer).
2. **Undo yığını 200 adımla sınırlı.**
3. **Pano dahili MIME tipi kullanır**; başka uygulamalara yapıştırılamaz
   (metin temsili nesne adıdır).
4. **`Body` bastırması geometri yeniden içe aktarıldığında sıfırlanır**
   (Body düğümleri yeniden kurulur).
5. **`--capture-appearance` Qt 6.8+ gerektirir**; 6.5–6.7 ile derlenirse
   belgeleme çekimi sistem görünümünü kullanır ve bunu bildirir.
6. **Türkçe locale ondalık ayracı** — sayısal alanlar sistem locale'ini kullanır
   (`100,00 mm`). Bilinçli tercih (native davranış).

## II.12 — Build / test sonucu (Alpha.2)

Ortam: macOS 26.6.2, Apple Silicon (arm64), Qt 6.11.1, VTK 9.7.0,
OpenCASCADE 7.9.3, gfortran (Homebrew), CMake + Ninja.

| Adım | Komut | Sonuç |
|---|---|---|
| Configure (Release GUI) | `cmake --preset macos-release-gui` | ✅ başarılı |
| **Temiz build** | `cmake --build build/macos-release-gui -j 8` | ✅ **0 hata, 0 uyarı** (922 hedef) |
| **CTest tam matris** | `ctest --test-dir build/macos-release-gui -j 8` | ✅ **127 / 127** (44.8 s) |
| Proje şema migration | `gui.project_schema_migration` | ✅ geçti (mevcut test korundu, silinmedi) |
| **GUI öz-testi** | `FEMCAE --selftest` | ✅ **118 / 118** |
| Bundle smoke | `FEMCAE --bundle-smoke` | ✅ `PASS version=1.0.2` |
| Mimari gate (yerel) | CI gate adımlarının birebir kopyası | ✅ geçti |

Alpha.1 baseline'ı da aynı ortamda doğrulandı: **127/127 CTest, 45/45 öz-test**.
Alpha.2 sonrası **127/127 CTest, 118/118 öz-test** — **regresyon yoktur**.

### Öz-test kapsamı (118 kontrol, 11 bölüm)

| Bölüm | Kapsam |
|---|---|
| 1 | başlangıç nesne grafiği · sonuç TANIMLARININ çözümden önce varlığı · temiz doküman |
| 2 | mesh üretimi · komut tabanlı bölme değişikliği · dirty state |
| 3 | çözüm · sonuç nesneleri · **ΣRx denge kontrolü** (gerçek Fortran çıktısı üzerinde fizik doğrulaması) |
| 4 | **bağımlılık (§43)** — yük/Nx/malzeme değişimi · Undo ile yeniden geçerlilik · rename'in sonucu bayatlatmaması |
| 5 | **Undo/Redo (§41)** — create · property · rename · delete (kimlik + konum) · malzeme · birleştirme penceresi |
| 6 | **Suppress/Unsuppress** — modelde kalma · preflight etkisi · undo |
| 7 | **Preflight (§46)** — hyperelastic malzeme · mixed u-p · geçerli model |
| 8 | **Clear Generated Mesh / Clear Solution (§45)** — neyin korunduğu, neyin silindiği |
| 9 | **Dirty/Clean (§42)** — kaydet · düzenle · kaydedilen noktaya Undo |
| 10 | **Kalıcılık round-trip (§44)** — 2 malzeme, 2 mesnet, 2 yük, 1 bastırılmış BC, analiz ayarları, 3 sonuç tanımı → SAVE → RESET → OPEN |
| 11 | **Görünüm regresyonu (§47)** — Light · Dark · Light → Dark → Light renk kalıntısı yok |

### Uçtan uca akış (macOS'ta gerçekten çalıştırıldı)

`ScreenshotDriver` uygulamayı otomatik olarak geometri → mesh → analiz → sınır
şartı → çözüm → sonuç → **bağlam menüsü → Edit menüsü (dinamik Undo metni) →
preflight → out-of-date** akışından geçirdi ve 24 PNG üretti
(`docs/gui-preview/`, light + dark). Menüler ayrı üst-düzey pencere olduğu için
gerçek menü widget'ı ayrıca yakalanıp doğru konuma bindirilir.

Ölçülen değerler: 525 node · 320 HEX8 · 1 575 DOF ·
max |u| = 0.00134138 mm · max von Mises = 4.08912 MPa.

---

# BÖLÜM III — Alpha.3.1: Viewport Navigation & Camera Foundation

## III.1 — Current-state audit

Kod değiştirilmeden önce source-of-truth ağaç ve kurulu toolchain incelendi:

| Alan | Audit sonucu |
|---|---|
| Camera API | `resetCamera()` + world-origin odaklı `setIsometricView()`; standard-view enum/API yoktu |
| VTK interactor | QVTK varsayılan trackball style; application-owned style yoktu |
| Mouse | VTK left drag kamerayı döndürebiliyordu; 3 px threshold yalnız pick'i ayırıyordu |
| Wheel/native gesture | Qt arbitration yoktu; `QWheelEvent` / `QNativeGestureEvent` ele alınmıyordu |
| Fit | `vtkRenderer::ResetCamera()`; explicit empty/bounds/overlay sözleşmesi yoktu |
| Representation | Shaded / Shaded+Edges / Wireframe çalışıyordu; view state olarak korundu |
| Axis/orientation | `vtkAxesActor` include edilmiş fakat widget oluşturulmamıştı; camera cube yoktu |
| Qt | 6.11.1 (CMake minimum 6.5 korunuyor) |
| VTK | 9.7.0; `vtkOrientationMarkerWidget` ve `vtkCameraOrientationWidget` mevcut |

Önceki corrective maddeler de native GUI öz-testinde doğrulandı: Undo/Redo,
dirty/clean, kalıcılık, DependencyEngine, preflight, geometry, mesh, Fortran
solve, results ve Light/Dark geçişleri PASS. Solver, mesher, geometry backend ve
proje şeması değiştirilmedi.

## III.2 — Uygulanan mimari

Yeni viewport dosyaları:

| Dosya | Sorumluluk |
|---|---|
| `ViewportNavigation.h` | `StandardView`, normalized action/source/phase kontratı |
| `ViewportInputRouter.h/.cpp` | Qt mouse/wheel/native/key normalization ve duplicate arbitration |
| `ViewportCameraController.h/.cpp` | Orbit, Pan, Zoom, Fit, zoom-to-bounds, standard views, rotation center |
| `Dynamics26InteractorStyle.h/.cpp` | tek VTK interaction owner; competing default camera bindings inert |

`ViewportWidget` kompozisyon sahibi olarak kaldı; renderer/actor yaşam döngüsü
yeniden tasarlanmadı. Giriş akışı:

```
Qt event → ViewportInputRouter → NavigationAction → ViewportCameraController → vtkCamera
```

Mouse modeli tek ve sabittir: left Selection için ayrılmış, middle Orbit,
Shift+middle Pan, wheel Zoom, Option+left Orbit fallback, right context menu.
`vtkInteractorStyleRubberBandPick` veya başka style'a runtime switching yoktur.

macOS wheel sınıflandırması tek `pixelDelta` koşuluna bağlanmadı. Device type,
device adı, capability, `pixelDelta`, `angleDelta`, `phase` ve `inverted`
birlikte değerlendirilir. Natural Scrolling için delta ikinci kez terslenmez.
Momentum clamp/scale uygulanır. Native gesture aktifken compatibility wheel
tüketilir ama action yayınlanmaz; VTK style wheel metotları da inert'tir.

## III.3 — Kamera, views ve overlay'ler

- Global convention: `+X Right`, `+Y Back`, `+Z Up`.
- `StandardView`: Isometric / Front / Back / Top / Bottom / Left / Right.
- Viewport-only kısayollar: `F`, `0`, `1`, `Shift+1`, `2`, `Shift+2`, `3`,
  `Shift+3`. Editör focus'unda ve application-global olarak çalışmaz.
- Fit görünür model bounds'u üzerinden çalışır; empty scene güvenlidir.
- `zoomToBounds` gerçek API olarak hazırdır; SelectionManager binding'i sonraki
  turdadır ve fake toolbar button eklenmemiştir.
- Rotation center varsayılanı görünür bounds merkezi; set-to-highlight/reset
  komutları viewport sağ-tık menüsünde keşfedilebilirdir.
- Sol-alt axis triad: non-interactive `vtkOrientationMarkerWidget`, 0.13×0.13.
- Sağ-üst orientation cube: mevcut VTK 9.7 API'siyle etkin, model kamera'sına
  bağlı ve kendi input'unu tüketir. Minimum VTK version yükseltilmedi.
- Representation üçlüsü korunmuştur ve Undo/dirty state'e girmez.

## III.4 — Kapsam sınırı

Bu turda `SelectionManager`, CAD Body/Face/Edge/Vertex picking, Named Selection,
Section Plane, material/unit/contact/nonlinear/mesher/solver çalışması
yapılmadı. Mevcut body-level provenance pick bridge yalnız left click için
korundu. CAD Geometry ≠ Display Tessellation ≠ FEM Mesh ayrımı değişmedi.

## III.5 — Automated validation

Yeni `gui.viewport_navigation` CTest'i şunları doğrudan doğrular:

- plain left drag kamera action üretmiyor,
- middle orbit ve Shift+middle pan lifecycle,
- angle-wheel zoom ve high-resolution trackpad pixel-scroll pan,
- Natural Scrolling double inversion yok,
- native gesture + compatibility wheel duplicate yok,
- Front/Back/Top/Bottom/Left/Right/Isometric camera directions,
- deterministic Top/Bottom `viewUp`,
- Fit visible bounds + empty scene güvenliği,
- zoom-to-bounds API,
- application VTK style left/wheel ile kamerayı değiştiremiyor,
- non-interactive axis triad creation,
- orientation cube availability, enabled state ve camera binding.

Native `FEMCAE --selftest` ayrıca gerçek application shell üzerinde axis triad,
orientation cube, representation aktör durumları, representation'ın Undo'ya
girmemesi ve önceki 118 corrective/regression kontrolünü çalıştırır.

## III.6 — Real Mac input report

Otomatik testler gerçek Mac üzerinde çalıştırılmış olsa da bu teslimat sırasında
fiziksel trackpad/mouse hareketi insan eliyle uygulanmadı. PASS uydurulmadı:

| Kontrol | Sonuç |
|---|---|
| Built-in Trackpad Two-Finger Pan | **NOT TESTED** |
| Trackpad Pinch Zoom | **NOT TESTED** |
| Physical Mouse Wheel Zoom | **NOT TESTED** |
| Natural Scrolling | **NOT TESTED** (classifier semantics automated PASS) |
| Magic Mouse | **NOT TESTED** |
| Duplicate Qt/VTK Handling | **NONE** (router + inert VTK style + automated de-duplication test) |

## III.7 — Build / test sonucu (Alpha.3.1)

Ortam: macOS 26.6.2, Apple Silicon arm64, Qt 6.11.1, VTK 9.7.0,
OpenCASCADE 7.9.3, CMake + Ninja.

| Adım | Sonuç |
|---|---|
| Debug GUI configure + build | **PASS**, VTK viewport ve orientation widgets etkin |
| Focused GUI tests | **2 / 2 PASS** (`gui.viewport_navigation`, schema migration) |
| Debug CTest tam matris | **128 / 128 PASS** (36.30 s) |
| Release GUI configure + full build | **PASS** (932 build adımı; VTK + OCCT etkin) |
| Release CTest tam matris | **128 / 128 PASS** (clean build sonrası 36.51 s; final incremental revalidation 2.50 s) |
| Native macOS Release GUI self-test | **122 / 122 PASS** |

`QT_QPA_PLATFORM=offscreen` ile GUI öz-testinde OpenGL context ve native
appearance switching bulunmadığından 118/120 kontrol geçti, iki Dark Mode
kontrolü environment-limited FAIL oldu. Bu sonuç PASS sayılmadı. Aynı executable
native Cocoa/OpenGL ortamında yeniden çalıştırılmıştır. Final Release build
axis triad, orientation cube, üç representation modu, Undo dışı view-state ve
önceki corrective/regression kontrolleriyle **122/122 PASS** vermiştir.
