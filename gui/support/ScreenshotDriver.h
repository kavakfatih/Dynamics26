#pragma once

// Belgeleme ekran görüntüsü sürücüsü (--capture <dizin>).
//
// Uygulamayı gerçek mühendislik akışından geçirir (geometri → mesh → analiz →
// sınır şartı → çözüm → sonuç) ve her adımda pencerenin görüntüsünü kaydeder.
// Böylece dokümantasyondaki görseller elle düzenlenmiş mockup değil, çalışan
// uygulamanın gerçek çıktısıdır.

#include <QString>

class QApplication;

namespace d26 {

class Dynamics26MainWindow;

// forcedAppearance: boş bırakılırsa macOS System Appearance kullanılır.
// "light" / "dark" yalnız BELGELEME çekimi için görünümü sabitler; normal
// çalıştırmada bu yol hiç çağrılmaz, dolayısıyla uygulamanın tek görünüm
// kaynağı sistem olmayı sürdürür.
int runScreenshotDriver(QApplication &app, Dynamics26MainWindow &window, const QString &outputDirectory,
                        const QString &forcedAppearance = QString());

} // namespace d26
