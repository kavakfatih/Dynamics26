# Dynamics26 V1.1.0-alpha.3.1 Handoff

## Tamamlanan iş

Professional CAE viewport navigation ve camera foundation, mevcut Alpha.2
application shell üzerinde uygulandı. Ana mimari, Undo/Redo, dirty/clean,
DependencyEngine, ServiceContext, persistence, semantic VTK roles, Light/Dark,
geometry, mesh, solver ve results akışları korunmuştur.

- `ViewportInputRouter`: Qt mouse/wheel/native gesture/key normalization.
- `ViewportCameraController`: Orbit, Pan, Zoom, Fit, zoom-to-bounds,
  standard views ve rotation center.
- Tek `Dynamics26InteractorStyle`; runtime style switching ve duplicate VTK
  camera handling yok.
- Mouse: left Selection, middle Orbit, Shift+middle Pan, wheel Zoom,
  Option+left Orbit, right context menu.
- `StandardView`: Isometric / Front / Back / Top / Bottom / Left / Right.
- Viewport-local shortcuts: `F`, `0`, `1`, `Shift+1`, `2`, `Shift+2`, `3`,
  `Shift+3`.
- Sol-alt non-interactive axis triad; sağ-üst orientation cube.
- Visible-bounds Fit, empty-scene safety, rotation center to highlight/reset,
  SelectionManager için `zoomToBounds` API.
- Shaded / Shaded+Edges / Wireframe view-state ve Undo dışı kalır.

SelectionManager, tam Body/Face/Edge/Vertex picking, Named Selection, Measure ve
Section Plane bu turun kapsamında değildir.

## Doğrulama

- Debug CTest: **128/128 PASS**.
- `gui.viewport_navigation`: routing, wheel/trackpad sınıflandırması,
  native/wheel de-duplication, 7 view direction, Fit, overlays PASS.
- Release CTest: **128/128 PASS**.
- Native macOS Release `FEMCAE --selftest`: **122/122 PASS**.

Fiziksel built-in trackpad, mouse wheel ve Magic Mouse insan eliyle test
edilmemiştir; final raporda **NOT TESTED** olarak tutulur. Otomatik input
contract testleri fiziksel-device PASS olarak sunulmaz.

## Sonraki viewport işi

1. SelectionManager ve tek interaction owner üzerinde selection mode.
2. CAD tessellation triangle → face provenance ve Body/Face/Edge/Vertex pick.
3. Zoom to Selection'ın gerçek selection binding'i.
4. Named Selection ve scope persistence.
5. Measure / Section Plane / Probe interaction modes.
