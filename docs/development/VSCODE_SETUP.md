# Dynamics26 — VS Code Geliştirme Ortamı

Dynamics26 için birincil IDE **Visual Studio Code** olarak kabul edilir. Proje Modern Fortran, C, C++20, Qt 6, VTK, Open CASCADE, CMake/Ninja ve CTest bileşenlerini aynı kaynak ağacında kullanır.

## 1. macOS / Apple Silicon araçları

Homebrew kuruluysa:

```bash
brew install cmake ninja gcc qt vtk arpack opencascade pipx
pipx install fortls
```

Kontrol:

```bash
which cmake
which ninja
which gfortran-15
which fortls
```

Apple Silicon Homebrew için beklenen kök dizin `/opt/homebrew`'dur.

## 2. Önerilen VS Code eklentileri

Repo açıldığında VS Code bunları otomatik önerecektir:

- Modern Fortran — `fortran-lang.linter-gfortran`
- CMake Tools — `ms-vscode.cmake-tools`
- C/C++ — `ms-vscode.cpptools`
- CodeLLDB — `vadimcn.vscode-lldb`
- GitHub Actions — `github.vscode-github-actions`
- CMake language support — `twxs.cmake`

Terminalden kurmak istersen:

```bash
code --install-extension fortran-lang.linter-gfortran
code --install-extension ms-vscode.cmake-tools
code --install-extension ms-vscode.cpptools
code --install-extension vadimcn.vscode-lldb
code --install-extension github.vscode-github-actions
code --install-extension twxs.cmake
```

## 3. Projeyi açma

Tercih edilen yöntem:

```bash
git clone https://github.com/kavakfatih/Dynamics26.git
cd Dynamics26
code Dynamics26.code-workspace
```

Alternatif olarak repo kökünde `code .` kullanılabilir.

`src/` veya `gui/` klasörünü tek başına workspace olarak açma. CMake proje kökü `Dynamics26/` olmalıdır.

## 4. CMake preset'leri

Repo `CMakePresets.json` ile dört ana preset sağlar:

| Preset | Amaç |
|---|---|
| `macos-debug-core` | Solver/core geliştirme ve test |
| `macos-release-core` | Release optimizasyonu ve doğrulama |
| `macos-debug-gui` | Qt + VTK + OCCT GUI debug |
| `macos-release-gui` | Release `.app` derlemesi |

VS Code içinde Command Palette → **CMake: Select Configure Preset** ile seçim yapılabilir.

Terminal eşdeğeri:

```bash
cmake --preset macos-debug-core
cmake --build --preset build-debug-core --parallel
ctest --preset test-debug-core
```

GUI için:

```bash
cmake --preset macos-debug-gui
cmake --build --preset build-debug-gui --parallel
ctest --preset test-debug-gui
```

## 5. VS Code Tasks

`Terminal → Run Task...` altında hazır görevler bulunur:

- Dynamics26: Configure Debug Core
- Dynamics26: Build Debug Core
- Dynamics26: Test Debug Core
- Dynamics26: Configure Release Core
- Dynamics26: Build Release Core
- Dynamics26: Test Release Core
- Dynamics26: Configure Debug GUI
- Dynamics26: Build Debug GUI
- Dynamics26: Test Debug GUI
- Dynamics26: Release Hardening Tests
- Dynamics26: Clean Build Trees

`Cmd+Shift+B` varsayılan olarak Debug Core build görevini çalıştırır.

## 6. Debug

Run and Debug panelinde iki profil bulunur:

### Dynamics26 CLI — Debug

Önce Debug Core'u configure/build eder, ardından:

```text
build/macos-debug-core/femcae_cli
```

CodeLLDB ile başlatılır.

### Dynamics26 GUI — Debug

Önce Qt/VTK/OCCT GUI build'ini oluşturur, ardından:

```text
build/macos-debug-gui/gui/FEMCAE.app/Contents/MacOS/FEMCAE
```

CodeLLDB ile çalıştırılır.

## 7. Fortran language server

Modern Fortran eklentisi `fortls` kullanır. Workspace ayarları compiler olarak `gfortran-15` seçer ve Debug Core Fortran module dizinini include path'e ekler.

`fortls` bulunamıyorsa:

```bash
pipx install fortls
pipx ensurepath
```

ardından VS Code'u yeniden başlat.

## 8. Kişisel CMake override'ları

Repo tarafından yönetilen ayarlar `CMakePresets.json` içindedir. Makineye özel değişiklik gerekiyorsa `CMakeUserPresets.json` kullan. Bu dosya Git'e eklenmez.

Örneğin farklı bir compiler yolu gerekiyorsa kullanıcı preset'i ile override edebilirsin.

## 9. Geliştirme kuralı

IDE yalnız bir frontend'dir. Referans build/test sözleşmesi her zaman:

```text
CMake → Ninja → CTest
```

olmalıdır. VS Code dışında yapılan build ile GitHub Actions'ın kullandığı build mantığı mümkün olduğunca aynı kalmalıdır.
