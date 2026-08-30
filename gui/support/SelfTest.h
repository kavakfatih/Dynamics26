#pragma once

// Geliştirici öz-testi (--selftest).
//
// Uygulamayı gerçek kullanıcı akışından geçirir ve her adımın mühendislik
// sonucunu doğrular: mesh üretimi, lineer çözüm, proje kaydet/aç round-trip,
// malzeme doğrulama önizlemesi, görünüm geçişi.
//
// CTest'e KAYITLI DEĞİLDİR: pencere sunucusu gerektirdiği için headless CI'da
// çalıştırılamaz. Yerel doğrulama ve regresyon kontrolü içindir.

class QApplication;

namespace d26 {

class Dynamics26MainWindow;

int runSelfTest(QApplication &app, Dynamics26MainWindow &window);

} // namespace d26
