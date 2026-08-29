#include "AppearanceController.h"

#include "GeometryPanel.h"
#include "PrePostPanel.h"
#include "ViewportWidget.h"

#include <QApplication>
#include <QGuiApplication>
#include <QLabel>
#include <QMainWindow>
#include <QPalette>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <QTreeWidget>

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

    void scheduleViewportRefresh()
    {
        if (viewport_ == nullptr) {
            return;
        }
        QTimer::singleShot(0, viewport_, [this] {
            if (viewport_ != nullptr) {
                viewport_->refreshSystemAppearance();
            }
        });
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
                [this](const QPalette &) { scheduleViewportRefresh(); });

        // Context değişimi veya backend'in yeni geometry/mesh/result scene'i
        // oluşturması mevcut VTK actor'larını değiştirebilir. Appearance katmanı
        // actor'ları yorumlamaz; yalnız ViewportWidget'tan semantic görünümünü
        // yeniden uygulamasını ister.
        if (auto *navigator = window_.findChild<QTreeWidget *>(QStringLiteral("Dynamics26Navigator"))) {
            connect(navigator, &QTreeWidget::currentItemChanged, this,
                    [this](QTreeWidgetItem *, QTreeWidgetItem *) { scheduleViewportRefresh(); });
        }
        if (auto *geometry = window_.findChild<GeometryPanel *>()) {
            connect(geometry, &GeometryPanel::message, this,
                    [this](const QString &) { scheduleViewportRefresh(); });
        }
        if (auto *prePost = window_.findChild<PrePostPanel *>()) {
            connect(prePost, &PrePostPanel::message, this,
                    [this](const QString &) { scheduleViewportRefresh(); });
            connect(prePost, &PrePostPanel::solveCompleted, this,
                    [this](double, double, double, qlonglong, double) { scheduleViewportRefresh(); });
        }

        scheduleViewportRefresh();
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
