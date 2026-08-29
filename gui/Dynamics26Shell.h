#pragma once

class QMainWindow;

namespace dynamics26::gui {

// V1.1.0-alpha.1 corrective shell, mevcut engineering widget'larını yeniden
// yazmadan pencerenin görünür layout/menu/toolbar sahipliğini üstlenir.
// Navigator/Inspector gerçek object/property modeline alpha.2'de taşınacaktır.
void applyApplicationShell(QMainWindow &window);

} // namespace dynamics26::gui
