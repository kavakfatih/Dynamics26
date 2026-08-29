#pragma once

class QApplication;
class QMainWindow;

namespace dynamics26::gui {

// Dynamics26 görünüm modu uygulama kabuğuna ait bir UX sorumluluğudur.
// Varsayılan "Sistem" macOS görünümünü kullanır; kullanıcı isterse uygulama
// özelinde Açık/Koyu görünümü seçebilir. Solver, model ve proje verisi bu
// tercihten bağımsızdır.
void installAppearanceController(QApplication &app, QMainWindow &window);

} // namespace dynamics26::gui
