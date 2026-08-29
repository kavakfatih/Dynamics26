#pragma once

class QMainWindow;

namespace dynamics26::gui {

// V1.1.0-alpha.1 yalnızca application-shell katmanını kurar. Mevcut
// geometry/mesh/solver widget'ları yeniden yazılmaz; ileriki V1.1 paketleri
// Navigator, Inspector ve command modelini bu iskelet üzerinde değiştirecektir.
void applyApplicationShell(QMainWindow &window);

} // namespace dynamics26::gui
