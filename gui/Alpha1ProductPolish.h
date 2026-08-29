#pragma once

class QMainWindow;

namespace dynamics26::gui {

// Alpha.1 son corrective katmanı yalnız görünür ürün davranışını toparlar.
// Mühendislik veri modelinin, solver'ın, CAD/mesh ayrımının veya ilerideki
// Alpha.2 object architecture'ın sahibi değildir.
void installAlpha1ProductPolish(QMainWindow &window);

} // namespace dynamics26::gui
