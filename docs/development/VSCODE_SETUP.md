# Dynamics26 — VS Code ile macOS Debug

Bu kurulum Dynamics26 kaynak kökünü, `CMakePresets.json` içindeki Apple Silicon
GUI preset'lerini ve CodeLLDB'yi kullanır. Ayrı bir VS Code build ağacı veya
hard-coded CMake konfigürasyonu oluşturmaz.

## 1. Gerekli araçlar ve extensions

Apple Silicon Homebrew araçları:

```bash
brew install cmake ninja gcc qt vtk arpack opencascade pipx
pipx install fortls
```

VS Code proje açıldığında `.vscode/extensions.json` şu gerekli eklentileri önerir:

- Modern Fortran — `fortran-lang.linter-gfortran`
- CMake Tools — `ms-vscode.cmake-tools`
- C/C++ — `ms-vscode.cpptools`
- CodeLLDB — `vadimcn.vscode-lldb`

## 2. Proje kökünü aç

`CMakePresets.json`, `gui/`, `src/` ve `.vscode/` klasörlerini doğrudan içeren
Dynamics26 kökünü aç:

```bash
cd /path/to/FEMCAE-v1.1.0-alpha.3.1.1
code .
```

`gui/` veya `src/` klasörünü tek başına workspace olarak açma.

## 3. Configure, build ve test

`Terminal → Run Task...` altında gerçek preset'leri kullanan görevler vardır:

1. `Dynamics26: Configure Debug GUI`
2. `Dynamics26: Build Debug GUI`
3. `Dynamics26: Test Debug GUI`
4. `Dynamics26: Configure + Build Debug GUI`

Build görevi önce configure görevini çalıştırır. Terminalde aynı sözleşme:

```bash
cmake --preset macos-debug-gui
cmake --build --preset build-debug-gui --parallel
ctest --preset test-debug-gui
```

Build edilen uygulama:

```text
build/macos-debug-gui/gui/FEMCAE.app/Contents/MacOS/FEMCAE
```

## 4. F5 ile GUI debug

1. Sol kenardan **Run and Debug** panelini aç.
2. `Dynamics26 GUI — Debug` profilini seç.
3. `F5` tuşuna bas.

Profil CodeLLDB kullanır. `preLaunchTask` önce Debug GUI configure/build zincirini
çalıştırır, sonra yukarıdaki gerçek `.app` executable'ını proje kökünü çalışma
dizini yaparak başlatır.

## 5. macOS input trace

Normal debug profilinde input log'u kapalıdır. Gerçek trackpad/mouse olaylarını
incelemek için **Run and Debug** içinden
`Dynamics26 GUI — Debug Input Trace` profilini seç. Bu profil yalnız Debug
derlemesinde bulunan `dynamics26.viewport.input` kategorisini şu kuralla açar:

```text
QT_LOGGING_RULES=dynamics26.viewport.input.debug=true
```

Debug Console çıktısı wheel olaylarında `pixelDelta`, `angleDelta`, `phase`,
`inverted`, device type/name/capabilities ile normalize edilmiş sonucu gösterir:

```text
InputSource=PixelScroll Action=Pan
InputSource=AngleWheel Action=Zoom
InputSource=NativeZoom Action=Zoom
```

Native gesture çıktısında gesture type, value ve konum da bulunur. Breakpoint
için uygun giriş noktaları:

- `d26::ViewportInputRouter::classifyWheel`
- `d26::ViewportInputRouter::routeWheel`
- `d26::ViewportInputRouter::routeNativeGesture`

`pixelDelta` bir davranış sinyalidir; tek başına fiziksel cihazın trackpad
olduğunu kanıtlamaz. Unit testler yalnız normalization/arbitration sözleşmesini
doğrular; fiziksel MacBook trackpad, mouse ve Magic Mouse sonucu sayılmaz.

## 6. Makineye özel ayarlar

Repo ayarlarını değiştirmeden yerel compiler veya prefix override'ı gerekiyorsa
Git'e eklenmeyen `CMakeUserPresets.json` kullan. Referans akış her zaman
`CMakePresets.json → Ninja → CTest → CodeLLDB` olarak kalmalıdır.
