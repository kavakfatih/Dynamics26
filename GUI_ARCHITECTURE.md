# Dynamics26 — GUI Mimarisi (V1.1.0-alpha.3.1 — Viewport Navigation & Camera Foundation)

Bu belge Dynamics26 masaüstü arayüzünün mimarisini, katman sınırlarını ve
mühendislik çekirdeğiyle olan sözleşmesini tanımlar.

Hedef platform: **macOS / Apple Silicon**. Araç seti: **Qt 6 Widgets + VTK 9 + OCCT 7**.

---

## 1. Neden Qt Widgets (QML değil)

Alpha.1 kapsamında QML'e geçilmedi. Gerekçe teknik:

- VTK'nın Qt entegrasyonu (`QVTKOpenGLNativeWidget`) bir **QWidget**'tir.
  QML'e taşımak `QQuickFramebufferObject` üzerinden özel bir render node yazmayı
  ve VTK'nın interactor/picking zincirini yeniden kurmayı gerektirir.
- OCCT ve meshing katmanı zaten C++ nesne modelidir; QML'e geçiş bunlar için
  ayrı bir `QAbstractItemModel` + property köprüsü katmanı ekler, mühendislik
  değeri üretmez.
- Details paneli **yoğun, iki kolonlu, sayısal bir tablo**dur. Bu, Qt Widgets'in
  native macOS kontrolleriyle en iyi çözdüğü problemdir; QML'de aynı native
  görünümü yeniden üretmek gerekir.

Sonuç: görsel modernlik uğruna doğrulanmış VTK/OCCT yolunu riske atmak yerine
Qt Widgets ile devam edildi.

---

## 2. Katman haritası

```
Dynamics26MainWindow                    ← görünür kompozisyonun TEK sahibi
│
├─ CommandRegistry                      ← tüm komutlar tek QAction kayıt defterinde
│
├─ DocumentCommandManager               ← model mutasyonlarının TEK giriş kapısı
│  └─ QUndoStack                           Undo/Redo · clean/dirty · macro
│
├─ DependencyEngine                     ← nesne DURUMLARININ tek yazarı
│                                          (UpToDate / OutOfDate / Ready / …)
│
├─ ProjectNavigator                     ← QTreeView + bağlam menüleri + inline rename
│  └─ ProjectTreeModel : QAbstractItemModel
│
├─ GraphicsWorkspace                    ← ViewportWidget + ince grafik araç çubuğu
│
├─ DetailsHost                          ← ObjectType → Details sayfası
│
├─ UtilityWorkspace                     ← Messages/Convergence/SolverOutput/…
│
├─ EngineeringStatusBar                 ← model · seçim · doküman · güncellik · çözücü
│
└─ ServiceContext                       ← explicit bağımlılık taşıyıcısı
   ├─ GeometryService                      CAD B-Rep + display tessellation
   ├─ MaterialService                      malzeme nesneleri + atama
   ├─ MeshService                          FEM mesh + kalite + güncellik
   └─ AnalysisService                      analiz · BC · sonuç tanımları · çözüm

           femcae_geometry (OCCT/XDE, DXF) · femcae_meshing (StructuredHexMesher,
           AssignmentResolver, ResultDatabase) · femcae_api (C ABI → Fortran)
```

### Mutasyon akışı

Model üzerindeki her anlamlı değişiklik AYNI yoldan geçer:

```
   UI (Details / menü / bağlam menüsü / ağaç)
        │
        ▼
   Domain Command            (QUndoCommand — "Add Force", "Change Mesh Divisions")
        │
        ▼
   Service                   (AnalysisService / MeshService / MaterialService)
        │
        ▼
   Domain Object + ProjectModel
        │
        ▼
   DependencyEngine::evaluate()          ← nesne durumlarını hesaplar
        │
        ▼
   ProjectTreeModel · DetailsHost · GraphicsWorkspace · EngineeringStatusBar
```

Details sayfaları servisleri DOĞRUDAN çağırmaz; CI gate bunu zorlar.

**Kural:** oklar tek yönlüdür. Servisler kabuğu tanımaz; kabuk servisleri
`ServiceContext` üzerinden **açıkça** alır. Hiçbir katman widget ağacında arama
yaparak veya görünen metne bakarak nesne bulmaz.

---

## 3. Doküman komut sistemi (`gui/core` + `gui/commands`)

### DocumentCommandManager

`QUndoStack` sahibi. Sorumlulukları:

| Sorumluluk | Ayrıntı |
|---|---|
| Undo / Redo | `createUndoAction()` / `createRedoAction()` dinamik metin üretir: *"Geri Al Add Force"* |
| Dirty / Clean | `markSaved()` → `setClean()`; `isDirty()` → `!isClean()`; kaydedilen noktaya Undo ile dönmek dokümanı tekrar temiz yapar |
| Transaction | `beginMacro()` / `endMacro()` — birden fazla komut tek Undo adımı |
| Yeniden değerlendirme | `documentMutated()` sinyali → `DependencyEngine::evaluate()` + arayüz tazeleme |

Yığın derinliği 200 adımla sınırlıdır.

### Domain Command'ler

18 komut sınıfı, tek generic "set property" komutu yok. Undo metni kullanıcıya
ne geri alacağını mühendislik diliyle söyler.

| Alan | Komutlar |
|---|---|
| Ad | `RenameObjectCommand` |
| Malzeme | `CreateMaterialCommand`, `DeleteMaterialCommand`, `SetMaterialPropertiesCommand`, `AssignMaterialCommand` |
| Mesh | `SetMeshDefinitionCommand` |
| Analiz | `CreateAnalysisCommand`, `DeleteAnalysisCommand`, `SetIncompressibilityCommand`, `SetLargeDeflectionCommand` |
| Sınır şartı / yük | `CreateFixedSupportCommand`, `CreateForceCommand`, `DeleteBoundaryConditionCommand`, `SetSupportCommand`, `SetForceCommand` |
| Sonuç tanımı | `CreateResultDefinitionCommand`, `DeleteResultDefinitionCommand` |
| Bastırma | `SuppressObjectCommand` |

**Kimlik sözleşmesi.** Oluşturma komutları ürettikleri `ObjectId`'yi saklar ve
redo tekrarında AYNI kimliği yeniden kullanır. Silme komutları nesnenin tam
durumunu **ve ağaçtaki satırını** saklar; undo nesneyi aynı kimlik ve aynı
konumla geri getirir.

**Birleştirme (merge).** Özellik komutları `mergeWith()` uygular ve **700 ms'lik
bir zaman penceresi** içindeki ardışık düzenlemeleri tek Undo adımında birleştirir.
Spinbox sürükleme tek adım olur; kullanıcı durup ayrı bir düzenleme yaptığında
yeni bir adım başlar.

### Neyin Undo'ya girmediği

| Kategori | Örnek | Neden |
|---|---|---|
| **View state** | kamera, Fit View, panel görünürlüğü, aktif sekme, ağaç seçimi | dokümanı değiştirmez |
| **Derived data** | üretilmiş mesh, hesaplanmış sonuç alanları | model durumu değil; büyük ikili anlık görüntüler yığına konmaz |
| **Derived komutlar** | Generate Mesh, Solve, Clear Generated Mesh, Clear Solution | türetilmiş veriyi üretir/siler, proje tanımını değiştirmez |

`Nx = 10 → Generate Mesh → Nx = 20` durumunda Mesh **OutOfDate** olur; `Undo`
`Nx = 10`'a döner ve mesh **yeniden geçerli** hale gelir (aşağıdaki içerik
karşılaştırması sayesinde).

---

## 4. Bağımlılık motoru (`gui/core/DependencyEngine`)

Nesne durumlarının **tek yazarı**. Servisler yalnız kendi verilerini ve nesne
adlarını yönetir.

### Bayatlık içerik karşılaştırmasıyla belirlenir

Monoton sayaç kullanılmaz — sayaç, bir değişikliği Undo ile geri almayı
"hâlâ bayat" olarak gösterirdi.

**Mesh:** `MeshService` üretim anındaki `Definition`'ı saklar; `isUpToDate()`
mevcut tanımla karşılaştırır (+ CAD sürüklüyorsa geometri revizyonu).

**Çözüm:** `AnalysisService::solverInputSignature()` solver'ın **gerçekten
tükettiği** girdilerin JSON imzasını üretir:

```
mesh üretimi + mesh güncelliği + ölçü/bölme tanımı
atanmış malzeme modeli + E + ν
AKTİF (bastırılmamış) sınır şartlarının kapsam ve DOF'ları
AKTİF yüklerin kapsam ve bileşenleri
çözülen formülasyon + large deflection + analiz türü
```

Çözüm bu imzayla damgalanır. İmza değişirse sonuç `OutOfDate` olur; imza aynıysa
— örneğin bir yük değişikliği Undo ile geri alındıysa — sonuç **yeniden
geçerlidir**. Nesneyi yeniden adlandırmak imzayı değiştirmez, dolayısıyla
sonuçları bayatlatmaz.

### Kural tablosu

| Değişiklik | Mesh | Solution |
|---|---|---|
| Geometri değişti | OutOfDate | OutOfDate |
| Mesh ayarı değişti | OutOfDate | OutOfDate |
| Malzeme parametresi değişti | geçerli kalır | OutOfDate |
| Sınır şartı / yük değişti | geçerli kalır | OutOfDate |
| Nesne yeniden adlandırıldı | geçerli kalır | geçerli kalır |
| Bastırma değişti | geçerli kalır | OutOfDate |

---

## 5. Bastırma (Suppress)

`Body`, `FixedSupport`, `Force` ve sonuç tanımları bastırılabilir.
Bastırılmış nesne:

- projede **kalır** ve ağaçta görünür (Delete ile aynı şey değildir),
- ağaçta soluk + italik çizilir ve `Suppressed` rozeti taşır,
- `solverInputSignature`'a **girmez**,
- preflight ve solve tarafından **dikkate alınmaz**,
- undoable'dır (`SuppressObjectCommand`).

`ProjectModel::isEffectivelySuppressed()` ata zincirini de kontrol eder:
bastırılmış bir analizin altındaki her şey etkin olarak bastırılmıştır.

---

## 6. Yaşam döngüsü: Preflight → Solve → Clear

```
Idle ─► Preflight ─┬─► Ready ─► Solving ─┬─► Completed
                   │                     └─► Failed
                   └─► Failed (çözüm hiç başlamaz)
```

**Solve doğrudan solver'ı çağırmaz.** `AnalysisService::solve()` önce
`preflight()` çalıştırır, raporu Solver Output'a yazar ve rapor geçmezse
çekirdeğe hiç gitmez.

`PreflightReport` şu kontrolleri üretir: analiz türü · geometri · malzeme
ataması · mesh varlığı · mesh güncelliği · mesh kalitesi (ters eleman) · aktif
sınır şartı · aktif yük · çözülen formülasyon · large deflection · çözücü DOF
kapasitesi · sonuç tanımı. Her kontrol bir `subject` ObjectId taşır, böylece
hata hangi nesnede olduğu ağaçta gösterilebilir.

Rapor büyük bir modal yerine **AnalysisDetails içindeki `VALIDATION` bölümünde**
ve Messages sekmesinde gösterilir.

| Komut | Korunan | Silinen |
|---|---|---|
| **Clear Generated Mesh** | yöntem, ölçüler, Nx/Ny/Nz | üretilmiş düğüm/eleman/kalite |
| **Clear Solution** | analiz, ayarlar, BC/yükler, **sonuç tanımları** | hesaplanmış deplasman/gerilme/reaksiyon |

`Stop / Cancel` **yoktur**: çözüm eşzamanlı çalıştığı için iptal edilebilir bir
nokta bulunmaz ve sahte bir komut eklenmez.

---

## 7. Proje nesne modeli (`gui/core`)

### ProjectModel

Model ağacındaki nesnelerin tek sahibi. Bir orman tutar: `Project` kökü ve her
analiz ayrı bir üst düzey köktür (ANSYS Mechanical'daki `Model` / `Static
Structural` ayrımı).

Her nesne şunu taşır:

| Alan | Anlam |
|---|---|
| `ObjectId` | kararlı sayısal kimlik (görünen metinden bağımsız) |
| `ObjectType` | `Body`, `Mesh`, `FixedSupport`, `EquivalentStress`, … |
| `name` | kullanıcıya görünen ad |
| `parent` / `children` | hiyerarşi |
| `state` | `NotReady` / `Ready` / `UpToDate` / `Obsolete` / `Error` |
| `statusText` | durumun nedeni (tooltip + Details) |
| `tag` | alan yükü (geometri gövde kimliği, analiz türü …) |

`ObjectId` sistem genelinde tek kimlik uzayıdır: `AnalysisService` sınır
şartlarını ve analizleri de bu kimlikle saklar, dolayısıyla ağaç ile mühendislik
verisi arasında ikinci bir eşleme tablosu yoktur.

### ProjectTreeModel

`QAbstractItemModel` adaptörü. `index.internalId()` doğrudan `ObjectId` taşır.
`NavigatorDelegate` satırın sağına durum rozetini çizer.

### CaeIcons

Her nesne türü ve komut için `QPainter` ile çizilen semantik CAE ikonu
(gövde, mesh ızgarası, ankastre mesnet sembolü, yük oku, kontur bantları …).
Ek bir kaynak dosyası veya bağımlılık yoktur; ikonlar mevcut palet rengiyle
üretilir ve görünüm değişiminde yeniden oluşturulur.

---

## 8. Servisler (`gui/services`) — mühendislik durumunun sahipliği

| Servis | Sahip olduğu | Kullandığı çekirdek |
|---|---|---|
| `GeometryService` | `GeometryDocument`, STEP yolu, DXF kesit özeti | `OcctStepImporter`, `DxfSectionReader`, `SectionProfile` |
| `MeshService` | `SimulationMesh`, sizing/divisions, `BoxBoundaryGeometry` | `StructuredHexMesher`, `evaluateHexMeshQuality` |
| `MaterialService` | malzeme tanımları, atama | `fem_hyperelastic_validate`, `fem_hyperelastic_isochoric_uniaxial_preview` |
| `AnalysisService` | analizler, sınır şartları, yükler, `ResultDatabase` | `AssignmentStore`, `resolveAssignments`, `fem_solve_linear_hex8_mesh` |

Servisler `QObject`'tir ve yalnız `changed()` / `message()` sinyalleri yayınlar.
Widget ömrüne bağlı mühendislik durumu **yoktur** — eski `GeometryPanel` ve
`PrePostPanel` widget'ları bu rolü üstlendiği için kaldırıldı.

### Geometry → Mesh sözleşmesi

`MeshService`, `GeometryService`'ten **yalnız eksen hizalı sınır kutusunu ve
gerçek CAD yüz kimliklerini** devralır (`OcctStepImporter::axisAlignedBoxDescriptor`).

```
STEP  ──►  GeometryDocument (B-Rep)  ──►  bounding box + face IDs  ──►  StructuredHexMesher
                    │
                    └──►  tessellate()  ──►  ViewportWidget (yalnız GÖSTERİM)
```

Görüntüleme üçgenleri hiçbir zaman mesher'a girmez. CAD gövdesi kutu değilse
`MeshService` parametrik kutuya döner ve bunu Details'ta açıkça bildirir.

### Birim politikası (ADR-0014)

GUI mm/MPa/GPa gösterir, çekirdeğe SI (m, Pa, N) gönderir. STEP model birimi
mm kabul edilir ve Geometry Details'ta `Length Unit` satırında açıkça yazar.

---

## 9. Viewport (`gui/viewport`) — semantik render

Aktörler property incelemesiyle değil, **taşıdıkları rolle** renklendirilir:

```
GeometrySurface   GeometryEdge
MeshSurface       MeshEdge        MeshNode
Selection
BoundaryCondition LoadGlyph
ReferenceShape
ResultContour     ResultVector
Background        BackgroundGradient   OverlayText
```

`ViewportPalette::forAppearance(dark)` her rol için **tam** bir palet döndürür.
Görünüm değiştiğinde `applyPalette()` tüm kayıtlı aktörleri rolüne göre yeniden
boyar; hiçbir aktör önceki paletten renk taşımaz.

### Bağlamlar

| Bağlam | Gösterim |
|---|---|
| Geometry / Materials / Connections | nötr CAD gölgelemesi + feature-edge aktörü |
| Mesh | nötr FEM yüzeyi + element kenarları (+ opsiyonel düğümler) |
| Loads / Analysis | nötr model + mesnet/yük sembolleri + kapsam vurgusu |
| Results | `vtkLookupTable` konturu + `vtkScalarBarActor` legend + deforme şekil + referans tel kafes |

**Kontur yalnız Results bağlamında açılır.** Preprocessing ekranlarında sonuç
renk skalası kullanılmaz.

### Yüzey verisi

Mesh, Loads ve Results bağlamları **tek bir sınır yüzeyi polydata**'sı üzerinden
çalışır: noktalar mesh düğümleri (gerekirse deforme), hücreler `boundaryFacets`.
Her hücre `sourceGeometryId` taşır — bu hem `vtkCellPicker` ile yüz seçimini hem
de kapsam vurgusunu mümkün kılar.

---

## 10. Details Host (`gui/shell` + `gui/details`)

`DetailsHost::pageFor(ObjectType)` seçilen nesnenin **türüne** göre sayfa seçer:

| ObjectType | Sayfa |
|---|---|
| `GeometryFolder` | `GeometryDetails` |
| `Mesh` | `MeshDetails` |
| `Material` | `MaterialDetails` |
| `Analysis`, `AnalysisSettings` | `AnalysisDetails` |
| `FixedSupport`, `Force` | `BoundaryConditionDetails` |
| `TotalDeformation`, `EquivalentStress`, `ReactionForce` | `ResultDetails` |
| diğerleri | `ObjectDetails` (Project / Model / Body / Sections / Connections / Solution) |

Yapı taşları: `DetailsSection` (DEFINITION / SCOPE / STATISTICS / ADVANCED …) ve
`DetailsRow` (sabit etiket kolonu + değer). Düzenlenebilir sayfalar widget'larını
kurucuda bir kez kurar; `refresh()` yalnız değerleri günceller (odak ve kaydırma
konumu korunur).

### Kullanıcı niyeti ≠ solver implementasyonu

`AnalysisDetails` normal kullanıcıya yalnız niyeti gösterir:

```
FORMULATION
  Incompressibility     Automatic | Compressible | Nearly Incompressible
```

Çözülen implementasyon `AnalysisService::resolvedFormulation()` tarafından
türetilir ve **yalnız** `▸ Advanced Solver Settings` altında salt-okunur
gösterilir:

```
Resolved Formulation   Displacement-based (u)  |  Mixed displacement–pressure (u–p)
Element Technology     HEX8 — full integration |  HEX8/P0 — element-associated pressure DOF
Linear Solver          Direct — dense reference
Practical DOF Limit    6000 DOF
```

Türetme kuralı: `Automatic` + ν ≥ 0.475 → mixed u–p. Mixed u–p keyfi mesh için
GUI çözüm akışına bağlı olmadığından bu durumda **Solve pasif kalır ve nedeni
yazılır** — çalışmayan bir yol çalışıyor gibi gösterilmez.

---

## 11. Komut yüzeyi (`CommandRegistry`)

Tüm komutlar tek bir kayıt defterinde `QAction` olarak tanımlanır ve string
kimlikle adreslenir (`mesh.generate`, `analysis.solve`, `geometry.import` …).
Menü, üst komut yüzeyi, bağlam şeridi ve Details düğmeleri **aynı** `QAction`'ı
paylaşır. Dolayısıyla:

- bir komutun etkin/pasif durumu tek yerden yönetilir,
- pasif bir komutun **nedeni** tooltip'te görünür (`setEnabled(id, false, reason)`),
- arka ucu olmayan komut hiç eklenmez.

### Komut yüzeyi

```
satır 1 (global):  Yeni · Aç · Kaydet │ Import Geometry · Generate Mesh │ Solve │ Fit View     … Navigator · Details · Diagnostics
satır 2 (bağlam):  GEOMETRY  Import Geometry · Replace · Import Section
                   MESH      Generate Mesh · Nodes
                   ANALYSIS  Insert Support · Insert Force │ Solve
                   SOLUTION  Export CSV · Export VTK
```

Grafik alanının kendi ince araç çubuğu ayrıdır ve yalnız görüntü/seçim
komutlarını taşır (Body / Face / Fit / Isometric) — ikinci bir ribbon değildir.

---

## 12. Alt yardımcı alan (`UtilityWorkspace`)

Sekmeler: **Messages · Convergence · Solver Output · Results Table · Timings**.

Davranış:

| Durum | Sonuç |
|---|---|
| normal modelleme | kapalı |
| `Severity::Error` mesajı | Messages açılır (zorlayıcı) |
| nonlineer doğrulama preset'i | Convergence gerçek yakınsama geçmişiyle açılır |
| kullanıcı kapatır | `userDismissed_` işaretlenir; yalnız gerçek hata tekrar açar |

---

## 13. Durum çubuğu (`EngineeringStatusBar`)

```
1 body • 320 HEX8 • 1.575 DOF     X-Min Face • 16 facet • Global Coordinate System     max |u| 0.001341 mm · Çözüldü  [Tanılama]
```

Sol: model büyüklükleri. Orta: seçim bağlamı. Sağ: çözücü durumu + tanılama
paneli anahtarı.

---

## 14. Light / Dark

- Özel tema motoru **yoktur**. `QApplication::setPalette` ve global QSS
  kullanılmaz; kaynak macOS System Appearance'tır.
- `gui/core/UiTheme.h` yalnız Dynamics26'nın kendi çizdiği ikincil metin, ayraç
  ve satır gölgesi renklerini mevcut paletten **türetir**. `SecondaryLabel` ve
  `Hairline` palet değişimini dinleyip yeniden uygular.
- VTK viewport native bir Qt widget'ı olmadığından kendi semantik paletini taşır;
  `QEvent::ApplicationPaletteChange` alındığında tüm roller yeniden uygulanır.
- Ekran görüntüsü sürücüsü (`--capture-appearance`) `QStyleHints::setColorScheme`
  ile görünümü sabitler. Bu yol **yalnız** belgeleme çekimi içindir; normal
  çalıştırmada hiç çağrılmaz.

---

## 15. Proje kalıcılığı

Dosya iki katman taşır:

**1) V1.0 uyumluluk katmanı** — `material` / `section` / `load` / `geometry` /
`prepost` / `project_schema`. Eski sürümler dosyayı açabilir. Açma yolu hâlâ
`femcae::gui::ProjectFileMigrator::migrate()` üzerinden geçer ve
`gui.project_schema_migration` CTest'i değişmeden çalışır.

**2) `dynamics26_document`** — TAM nesne grafiği:

```json
{
  "dynamics26_document": {
    "version": 1,
    "next_object_id": 42,
    "materials": {
      "materials": [ { "object_id": 8, "name": "...", "model_index": 0, ... } ],
      "assigned_object_id": 8
    },
    "analyses": {
      "analyses": [{
        "object_id": 9, "name": "Static Structural 1", "analysis_type": 0,
        "large_deflection": false, "incompressibility": 0, "suppressed": false,
        "settings_object_id": 10, "solution_object_id": 11,
        "boundary_conditions": [
          { "kind": "fixed_support", "object_id": 12, "scope": 0, "fix_x": true, ..., "suppressed": false },
          { "kind": "force",         "object_id": 13, "scope": 1, "fx_n": 100.0, ..., "suppressed": false }
        ],
        "result_definitions": [ { "object_id": 14, "kind": 0, "name": "Total Deformation" } ]
      }],
      "analysis_counter": 1
    }
  }
}
```

Round-trip garantileri: **ObjectId**, **DisplayName**, **ordering**, analiz
ayarları, Fx/Fy/Fz, kapsam, **suppression**, sonuç tanımları ve ağaç hiyerarşisi.

İki tasarım kararı bunu mümkün kılar:

1. **Sınır şartları ve yükler TEK sıralı listede** (`boundary_conditions`)
   saklanır. İki ayrı dizi, ağaçtaki gerçek sırayı (Support, Force, Support)
   kaybederdi.
2. **Kimlik sayacı yüklemeden ÖNCE rezerve edilir** (`next_object_id`). Aksi
   halde otomatik üretilen ara düğüm kimlikleri, dosyadan gelen açık
   ObjectId'lerle çakışıp nesne kaybına yol açardı.

Eski (V1.0) dosyalarda `dynamics26_document` yoktur; bu durumda tek malzeme ve
tek skaler yük **varsayılan nesne grafiğine uygulanır** ve kullanıcı uyarılır.
Yeni nesne modeli eski `force_n` alanına sıkıştırılmaz.

Ayrıca dosya menüsü `Son Kullanılanlar` listesini `QSettings` içinde tutar ve
`Kaydedilene Dön` diskteki son kaydı yeniden yükler (Undo ile aynı şey değildir).

---

## 16. Mimari koruma (CI gate)

`.github/workflows/macos-self-hosted.yml` içindeki **V1.1 CAE shell architecture
gate** şunları zorlar:

- `gui/shell/Dynamics26MainWindow.cpp` üç bölgeli yerleşimi kendisi kurar,
- `gui/core/ProjectTreeModel` gerçek bir `QAbstractItemModel`'dir,
- kaldırılan corrective katmanlar geri eklenemez,
- `findChild/findChildren` ile widget keşfi yasaktır,
- `text().contains(...)` ile kontrol tanıma yasaktır,
- `setStyleSheet` ve uygulama çapında `setPalette` yasaktır,
- `setUnifiedTitleAndToolBarOnMac(true)` ve `Qt::META` yasaktır,
- **`DocumentCommandManager` / `DependencyEngine` / `DomainCommands` kaldırılamaz**,
- **Details sayfaları servisleri doğrudan mutasyona uğratamaz**
  (`services_.analysis->set…` vb. yasaktır; domain command kullanılmalıdır),
- **`UiTheme` ikincil renkleri `QApplication::palette()` üzerinden türetmelidir**
  (widget'ın kendi paletinden türetmek stale foreground üretir),
- **Qt minimum sürümü 6.5'tir**; 6.8+ API'leri sürüm koruması olmadan kullanılamaz.

---

## 17. Viewport navigation ve camera foundation (Alpha.3.1)

Alpha.3.1, mevcut `GraphicsWorkspace → ViewportWidget` sınırını değiştirmeden
viewport girişini ve kamera davranışını üç ayrı sorumluluğa böler:

```
Qt mouse / wheel / native gesture / key event
                    │
                    ▼
         ViewportInputRouter             cihaz + phase + delta sınıflandırması
                    │                    event kabulü / duplicate arbitration
                    ▼
           NavigationAction              Orbit · Pan · Zoom · Fit · StandardView
                    │
                    ▼
      ViewportCameraController           tek kamera mutasyon noktası
                    │
                    ▼
             vtkCamera
```

`Dynamics26InteractorStyle`, `vtkInteractorStyleTrackballCamera` tabanlı **tek
application-owned VTK interaction owner**'dır. Qt router'ın yönettiği left
drag, middle drag, right drag ve wheel için VTK'nın varsayılan kamera
metotlarını bilinçli olarak etkisiz bırakır. Runtime style switching yoktur;
gelecekte Selection / Measure / Section aynı owner üzerinde genişletilecektir.

### Giriş sözleşmesi

| Fiziksel giriş | Normalized action | Not |
|---|---|---|
| Left click / drag | kamera action yok | Selection için ayrılmıştır; mevcut body-level pick bridge korunur |
| Middle drag | Orbit | görünür model merkezli rotation center |
| Shift + Middle drag | Pan | kamera, focal point ve rotation center birlikte taşınır |
| Option + Left drag | Orbit | macOS fallback |
| Right click | context menu | standard views + rotation-center komutları keşfedilebilir |
| Physical angle wheel | Zoom | `angleDelta / 120` normalize edilir |
| validated high-resolution trackpad scroll | Pan | `pixelDelta`, device type/name, phase ve angle sinyalleri birlikte kullanılır |
| Native pan | Pan | gesture lifecycle Begin / Update / End |
| Native pinch | Zoom | `value()` incremental delta olarak normalize edilir |

`pixelDelta != 0 → trackpad` bir domain invariant **değildir**. Sınıflandırma
`pixelDelta`, `angleDelta`, `phase`, `device()->type()`, device adı ve capability
bilgisini birlikte değerlendirir. Açıkça mouse olan ve angular tick taşıyan
cihaz zoom'dur; typed/named trackpad veya phased pixel-only akış pan adayıdır.
`inverted()` nedeniyle ikinci kez yön çevrilmez: Qt'nin teslim ettiği delta
doğrudan kullanılır. `ScrollMomentum` pan büyüklüğü 0.35 ile sınırlandırılır ve
tek event delta'sı aşırı inertial sıçramayı önlemek için clamp edilir.

Native gesture stream aktifken uyumluluk amacıyla gelen `QWheelEvent` kabul
edilir fakat ikinci action yayınlanmaz. Tüm tüketilen wheel/native/navigation
event'leri `accept()` edilir ve event filter `true` döndürür; böylece **bir
fiziksel gesture = bir navigation action** olur.

### Kamera ve global koordinat sözleşmesi

`StandardView` API'si:

```cpp
enum class StandardView { Isometric, Front, Back, Top, Bottom, Left, Right };
void ViewportWidget::setStandardView(StandardView view);
```

Global convention sabittir: `+X = Right`, `+Y = Back`, `+Z = Up`.

| View | Camera side | Looking direction | View up |
|---|---|---|---|
| Front | −Y | +Y | +Z |
| Back | +Y | −Y | +Z |
| Right | +X | −X | +Z |
| Left | −X | +X | +Z |
| Top | +Z | −Z | +Y |
| Bottom | −Z | +Z | −Y |
| Isometric | +X, −Y, +Z | bounds center | deterministic +Z seed |

Top/Bottom `viewUp` değerleri bilinçli olarak zıt seçilir; böylece her iki
görünümde ekran sağı +X kalır. Standard view uygulaması görünür model bounds'unu
sığdırır ve rotation center'ı bounds merkezine taşır.

Viewport focus aktifken `F`, `0`, `1`, `Shift+1`, `2`, `Shift+2`, `3`,
`Shift+3` sırasıyla Fit, Isometric, Front, Back, Top, Bottom, Right ve Left'tir.
Router yalnız viewport event filter'ında çalışır; `QLineEdit`, spinbox ve text
editor kontrollerini ayrıca dışlar. Application-global `Command+F`, `Command+H`
ve screenshot kısayolu eklenmemiştir.

### Fit, zoom-to-bounds ve rotation center

- `fitView()` yalnız ana renderer'ın görünür 3B model bounds'unu kullanır.
  Axis triad ve orientation cube ayrı overlay renderer'larda olduğundan bounds'a
  katılmaz. Empty scene güvenli biçimde `false` döner ve kamerayı bozmaz.
- `zoomToBounds(std::array<double, 6>)`, gelecekteki `SelectionManager` için
  tamamlanmış camera API'sidir. Bu turda fake Zoom to Selection kontrolü yoktur.
- Varsayılan rotation center görünür model bounds merkezidir. Public
  `setRotationCenter`, `setRotationCenterToBounds` ve `resetRotationCenter`
  API'leri vardır. Sağ tık menüsü, mevcut highlight için **Set Rotation Center
  to Selection** ve **Reset Rotation Center to Model** sunar.

### Overlay'ler

- Sol-alt: `vtkAxesActor + vtkOrientationMarkerWidget`, viewport
  `(0, 0, 0.13, 0.13)`, non-interactive. Label rengi semantik
  `OverlayText` rolünden Light/Dark görünümünde yeniden uygulanır.
- Sağ-üst: build-time mevcutsa `vtkCameraOrientationWidget`. Bu source-of-truth
  build'de VTK 9.7.0 ile compile-time mevcut ve model renderer kamera'sına
  bağlıdır. Widget kendi left events'lerini `AbortFlagOn` ile tüketir; cube
  click geometri picking'e düşmez. API yoksa `orientationCubeAvailable()`
  false döner; yalnız bunun için VTK minimum sürümü yükseltilmez.

### View state sınırı

Kamera, Fit, standard views, rotation center ve
`Shaded / Shaded + Edges / Wireframe` **view state**'tir. Hiçbiri
`DocumentCommandManager` / `QUndoStack` içine girmez ve proje dirty state'ini
değiştirmez. CAD Geometry ≠ Display Tessellation ≠ FEM Mesh sözleşmesi bu
değişikliklerden etkilenmemiştir.
