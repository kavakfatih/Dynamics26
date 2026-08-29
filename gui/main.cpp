#include "MainWindow.h"
#include "Dynamics26Shell.h"

#include <QAction>
#include <QApplication>
#include <QKeySequence>
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

void applyMacCommandShortcuts(MainWindow &window)
{
#ifdef Q_OS_MACOS
    // Qt Apple platformlarında Qt::CTRL kullanıcının Command tuşuna eşlenir.
    // Dynamics26 özel kısayolları Apple'ın standart mapping'ini takip eder.
    const auto actions = window.findChildren<QAction *>();
    for (auto *action : actions) {
        if (action == nullptr) {
            continue;
        }
        if (action->text() == QStringLiteral("Navigator")) {
            action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));
        } else if (action->text() == QStringLiteral("Inspector")) {
            action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_2));
        } else if (action->text() == QStringLiteral("Bottom Utility Area")) {
            action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
        }
    }
#else
    Q_UNUSED(window)
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

#if defined(Q_OS_MACOS) && defined(FEMCAE_GUI_HAS_VTK)
    // Qt, unifiedTitleAndToolBarOnMac ile QOpenGLWidget içeriğini birlikte
    // resmi olarak desteklemiyor. Normal Dynamics26 VTK viewport'u
    // QVTKOpenGLNativeWidget/QOpenGLWidget tabanlı olduğundan Alpha.1'de native
    // window frame korunur fakat Qt unified-toolbar flag'i kapatılır. İleride
    // AppKit/Qt entegrasyonu ayrıca araştırılmadan viewport kararlılığı riske atılmaz.
    window.setUnifiedTitleAndToolBarOnMac(false);
#endif

    applyMacCommandShortcuts(window);
    window.show();
    return app.exec();
}
