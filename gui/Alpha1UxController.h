#pragma once

class QMainWindow;

namespace dynamics26::gui {

// Corrective V1.1.0-alpha.1 katmanı yalnız görünür UX davranışını düzenler.
// Solver/mesh/CAD veri modellerine sahip olmaz; mevcut çalışan widget ve servisleri
// bağlama göre sunar. Alpha.2 object architecture gelene kadar geçici ama gerçek
// kullanıcı yüzeyine ait bir orchestration katmanıdır.
void attachAlpha1UxController(QMainWindow &window);

} // namespace dynamics26::gui
