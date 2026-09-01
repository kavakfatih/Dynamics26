// Dynamics26 GUI giriş noktası — TEK kompozisyon kökü.
//
// Dynamics26MainWindow görünür pencere yerleşiminin tek sahibidir.
// SelectionCoordinator ikinci bir shell/pencere katmanı değildir; yalnız
// transient CAD selection state'ini mevcut servis/navigator/viewport arasında
// açıkça bağlar.
//
// Geliştirici bayrakları (normal kullanımda gerekmez):
//   --bundle-smoke                       macOS bundle audit protokolü
//   --selftest                           genel GUI öz-testi
//   --selection-selftest                 Alpha.3.3 CAD topology shell acceptance
//   --capture <dizin>                    belgeleme ekran görüntüleri
//   --capture-appearance light|dark      çekim için görünümü sabitler
//   --import-step <dosya>                dosya diyaloğu olmadan STEP yükler

#include "core/NamedSelectionCompositionContract.h"
#include "shell/Dynamics26MainWindow.h"
#include "shell/SelectionCoordinator.h"
#include "support/ScreenshotDriver.h"
#include "support/SelectionAcceptanceTest.h"
#include "support/SelfTest.h"

#include <QApplication>
#include <QSurfaceFormat>

#include <cstring>
#include <iostream>
#include <string>

#ifdef FEMCAE_GUI_HAS_VTK
#include <QVTKOpenGLNativeWidget.h>
#endif

namespace {

bool hasArgument(const int argc, char *argv[], const char *expected)
{
    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr && std::strcmp(argv[i], expected) == 0) {
            return true;
        }
    }
    return false;
}

std::string argumentValue(const int argc, char *argv[], const char *expected)
{
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] != nullptr && std::strcmp(argv[i], expected) == 0) {
            return argv[i + 1];
        }
    }
    return {};
}

const char *buildVersion()
{
#ifdef FEMCAE_APP_VERSION
    return FEMCAE_APP_VERSION;
#else
    return "unknown";
#endif
}

} // namespace

int main(int argc, char *argv[])
{
    // Bu yol QApplication/QPA başlatılmadan önce çalışır. macOS bundle audit
    // protokolü geçmiş release kanıtlarıyla uyumlu kalmalıdır; değiştirilmez.
    if (hasArgument(argc, argv, "--bundle-smoke")) {
        std::cout << "FEMCAE bundle smoke PASS version=" << buildVersion() << '\n';
        return 0;
    }

#ifdef FEMCAE_GUI_HAS_VTK
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
#endif

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Dynamics26"));
    app.setApplicationDisplayName(QStringLiteral("Dynamics26"));
    app.setApplicationVersion(QString::fromLatin1(buildVersion()));
    app.setOrganizationName(QStringLiteral("Dynamics26"));

    // Uygulama macOS System Appearance'ı kullanır. Global QPalette veya
    // uygulama çapında QSS ayarlanmaz; Light/Dark tek kaynaktan gelir.
    d26::Dynamics26MainWindow window;

    // Alpha.3.6 persistent scope servisi document lifetime boyunca tek örnektir.
    // Transient SelectionManager/SelectionCoordinator'dan önce kurulur; böylece
    // application-level consumer'lar ServiceContext üzerinden aynı servise bakar.
    auto *namedSelections = d26::createNamedSelectionComposition(window.services(), &window);
    window.installNamedSelectionService(namedSelections);
    if (window.services().namedSelections == nullptr) {
        std::cerr << "FEMCAE composition FAIL: NamedSelectionService unavailable\n";
        return 2;
    }

    auto *selectionCoordinator = new d26::SelectionCoordinator(&window, &window);
    Q_UNUSED(selectionCoordinator);

    // Hosted shell acceptance pencereyi göstermek zorunda değildir; QObject,
    // model ve widget-state signal zinciri görünürlükten bağımsız çalışır. Bu
    // ayrım QVTK/OpenGL'i headless CI'da gereksiz yere ekrana bağlamaz.
    const bool selectionSelfTest = hasArgument(argc, argv, "--selection-selftest");
    if (!selectionSelfTest) {
        window.show();
    }

    // Otomasyon kolaylığı: dosya diyaloğu olmadan STEP yükler.
    const std::string stepPath = argumentValue(argc, argv, "--import-step");
    if (!stepPath.empty()) {
        window.importGeometryFromPath(QString::fromStdString(stepPath));
    }

    if (selectionSelfTest) {
        // Alpha.3.3+: gerçek Body/Face/Edge/Vertex ve FEM selection coordinator
        // zincirini çalıştırır; fiziksel pointer kabulünün yerine geçmez.
        return d26::runSelectionAcceptanceTest(app, window);
    }

    if (hasArgument(argc, argv, "--selftest")) {
        // Geliştirici öz-testi: gerçek akışı çalıştırır ve sonucu doğrular.
        return d26::runSelfTest(app, window);
    }

    const std::string captureDirectory = argumentValue(argc, argv, "--capture");
    if (!captureDirectory.empty()) {
        // Geliştirici modu: belgelenmiş ekran görüntülerini üretir ve çıkar.
        const std::string appearance = argumentValue(argc, argv, "--capture-appearance");
        return d26::runScreenshotDriver(app, window, QString::fromStdString(captureDirectory),
                                        QString::fromStdString(appearance));
    }

    return app.exec();
}
