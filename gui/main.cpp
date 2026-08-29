#include "MainWindow.h"
#include "Dynamics26Shell.h"
#include "Alpha1UxController.h"
#include "AppearanceController.h"

#include <QApplication>
#include <QSurfaceFormat>

#include <cstring>
#include <iostream>

#ifdef FEMCAE_GUI_HAS_VTK
#include <QVTKOpenGLNativeWidget.h>
#endif

namespace {
bool hasArgument(int argc, char* argv[], const char* expected)
{
    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr && std::strcmp(argv[i], expected) == 0) {
            return true;
        }
    }
    return false;
}

const char* buildVersion()
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
    // Bu yol QApplication/QPA başlatılmadan önce çalışır. Strict macOS bundle
    // audit geçmiş release kanıtlarıyla uyumlu kalabilsin diye smoke protokolü
    // ve engine build version sözleşmesi V1.1 alpha aşamasında değiştirilmez.
    if (hasArgument(argc, argv, "--bundle-smoke")) {
        std::cout << "FEMCAE bundle smoke PASS version=" << buildVersion() << '\n';
        return 0;
    }

#ifdef FEMCAE_GUI_HAS_VTK
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
#endif

    QApplication app(argc, argv);
    app.setApplicationName("Dynamics26");
    app.setApplicationDisplayName("Dynamics26");
    app.setApplicationVersion(QString::fromLatin1(buildVersion()));
    app.setOrganizationName("Dynamics26");

    MainWindow window;
    dynamics26::gui::applyApplicationShell(window);
    dynamics26::gui::attachAlpha1UxController(window);
    dynamics26::gui::installAppearanceController(app, window);
    window.show();
    return app.exec();
}
