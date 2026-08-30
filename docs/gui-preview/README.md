# GUI Preview

Bu klasördeki PNG'ler **elle düzenlenmiş mockup değildir**. Çalışan
Dynamics26 uygulamasının otomatik çıktısıdır; `gui/support/ScreenshotDriver.cpp`
uygulamayı gerçek mühendislik akışından (geometri → mesh → analiz → sınır şartı →
çözüm → sonuç → bağlam menüsü → preflight → out-of-date) geçirip her adımda
pencereyi yakalar. Menüler ayrı üst-düzey pencereler olduğu için gerçek menü
widget'ı ayrıca yakalanıp doğru konuma bindirilir.

Yeniden üretmek için:

```bash
APP=build/macos-release-gui/gui/FEMCAE.app/Contents/MacOS/FEMCAE
"$APP" --capture docs/gui-preview --capture-appearance light
"$APP" --capture docs/gui-preview --capture-appearance dark
```

`--capture-appearance` yalnız bu belgeleme yolunda ve Qt 6.8+ sürüm koruması
altında görünümü sabitler; normal çalıştırmada Dynamics26 macOS System
Appearance'ı takip eder (proje minimum Qt sürümü 6.5'tir).

| Dosya | İçerik |
|---|---|
| `01-initial-*` | Açılış — Geometry seçili, sonuç TANIMLARI çözümden önce mevcut |
| `02-geometry-*` | Body seçili |
| `03-mesh-*` | Mesh üretildikten sonra Mesh Details |
| `04-static-structural-*` | Static Structural — Validation (preflight) bölümü dahil |
| `05-boundary-condition-*` | Fixed Support seçili, kapsam vurgulu |
| `06-solution-result-*` | Equivalent Stress konturu |
| `07-total-deformation-*` | Total Deformation konturu |
| `08-utility-workspace-*` | Alt yardımcı alan açık (Messages) |
| `09-context-menu-*` | Nesne türüne duyarlı bağlam menüsü (Force) |
| `10-undo-redo-*` | Edit menüsü — dinamik Undo metni ("Geri Al Add Force") |
| `11-preflight-*` | Eksik model üzerinde preflight raporu |
| `12-out-of-date-state-*` | Bağımlılık motoru: Mesh/Solution Out of Date rozetleri |
