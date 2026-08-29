#include "AppearanceController.h"

#include "ViewportWidget.h"

#include <QApplication>
#include <QGuiApplication>
#include <QLabel>
#include <QMainWindow>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>

namespace {

class AppearanceController final : public QObject
{
public:
    AppearanceController(QApplication &app, QMainWindow &window)
        : QObject(&window), app_(app), window_(window)
    {
        // Alpha.1 recovery kararı:
        // Dynamics26, macOS görünüm motoruyla yarışan global QPalette/QSS skin'i
        // uygulamaz. Qt'nin native macOS style'ı Light/Dark/System görünümünün
        // tek kaynağıdır. Önceki engineering-preview tema tercihi artık geçersizdir.
        QSettings settings;
        settings.remove(QStringLiteral("ui/appearance"));

        // Legacy MainWindow constructor'ından gelebilecek pencere-seviyesi QSS
        // shell tarafından da temizlenir; burada ikinci kez boşaltmak bilinçli bir
        // güvenlik ağıdır. QApplication palette'i veya style'ı zorlanmaz.
        window_.setStyleSheet(QString());

        installPreviewIdentity();
        attachViewportToSystemAppearance();
    }

private:
    void installPreviewIdentity()
    {
        auto *status = window_.statusBar();
        if (status == nullptr
            || status->findChild<QLabel *>(QStringLiteral("Dynamics26PreviewIdentity")) != nullptr) {
            return;
        }

        auto *label = new QLabel(QStringLiteral("V1.1 · α1"), status);
        label->setObjectName(QStringLiteral("Dynamics26PreviewIdentity"));
        label->setToolTip(QStringLiteral("Dynamics26 V1.1.0-alpha.1 engineering preview"));
        status->addPermanentWidget(label);
    }

    void attachViewportToSystemAppearance()
    {
        viewport_ = window_.findChild<ViewportWidget *>();
        if (viewport_ == nullptr) {
            return;
        }

        // macOS Light/Dark değiştiğinde Qt paletteChanged yayınlar. Qt widget'ları
        // native style tarafından güncellenir; VTK ayrı bir render sistemi olduğu
        // için yalnız viewport kendi semantic rendering renklerini yeniler.
        connect(&app_, &QGuiApplication::paletteChanged, this,
                [this](const QPalette &) {
                    if (viewport_ != nullptr) {
                        viewport_->refreshSystemAppearance();
                    }
                });

        QTimer::singleShot(0, viewport_, [this] {
            if (viewport_ != nullptr) {
                viewport_->refreshSystemAppearance();
            }
        });
    }

    QApplication &app_;
    QMainWindow &window_;
    ViewportWidget *viewport_ = nullptr;
};

} // namespace

namespace dynamics26::gui {

void installAppearanceController(QApplication &app, QMainWindow &window)
{
    new AppearanceController(app, window);
}

} // namespace dynamics26::gui
