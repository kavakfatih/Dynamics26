#include "MainWindow.h"

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
    // This path intentionally runs before QApplication/QPA initialization.
    // If dyld cannot resolve Qt/VTK/OCCT/Fortran dependencies, execution never
    // reaches this point; therefore it is useful as a headless bundle smoke.
    if (hasArgument(argc, argv, "--bundle-smoke")) {
        std::cout << "FEMCAE bundle smoke PASS version=" << buildVersion() << '\n';
        return 0;
    }

#ifdef FEMCAE_GUI_HAS_VTK
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
#endif
    QApplication app(argc, argv);
    app.setApplicationName("FEMCAE");
    app.setApplicationVersion(QString::fromLatin1(buildVersion()));
    app.setOrganizationName("FEMCAE");
    MainWindow window;
    window.show();
    return app.exec();
}
