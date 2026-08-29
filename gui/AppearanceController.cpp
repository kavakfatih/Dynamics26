#include "AppearanceController.h"

#include "GeometryPanel.h"
#include "PrePostPanel.h"
#include "ViewportWidget.h"

#include <QApplication>
#include <QColor>
#include <QGuiApplication>
#include <QLabel>
#include <QMainWindow>
#include <QPalette>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <QTreeWidget>

#ifdef FEMCAE_GUI_HAS_VTK
#include <QVTKOpenGLNativeWidget.h>
#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#endif

namespace {

#ifdef FEMCAE_GUI_HAS_VTK
struct ViewportTheme
{
    double bg[3];
    double surface[3];
    double edge[3];
    double wire[3];
};

ViewportTheme currentViewportTheme()
{
    const QPalette palette = qApp->palette();
    const QColor window = palette.color(QPalette::Window);
    const bool dark = window.lightnessF() < 0.5;

    if (dark) {
        return {
            {0.060, 0.071, 0.082},
            {0.43, 0.47, 0.53},
            {0.72, 0.76, 0.82},
            {0.78, 0.82, 0.88}
        };
    }

    return {
        {0.960, 0.968, 0.977},
        {0.70, 0.74, 0.80},
        {0.28, 0.32, 0.38},
        {0.24, 0.28, 0.34}
    };
}

void setActorColor(vtkProperty *property, const double rgb[3])
{
    if (property != nullptr) {
        property->SetColor(rgb[0], rgb[1], rgb[2]);
    }
}
#endif

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

void ViewportWidget::refreshSystemAppearance()
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (renderer_ == nullptr || vtkWidget_ == nullptr || vtkWidget_->renderWindow() == nullptr) {
        return;
    }

    const ViewportTheme theme = currentViewportTheme();
    renderer_->SetBackground(theme.bg[0], theme.bg[1], theme.bg[2]);

    // VTK, Qt/macOS görünüm motorunun parçası değildir. Bu nedenle viewport kendi
    // semantic rendering rollerini yeniler. Scalar-mapped result contour yüzeyi
    // korunur; yalnız neutral geometry/mesh ve referans çizgileri temaya uyar.
    auto *actors = renderer_->GetActors();
    if (actors != nullptr) {
        actors->InitTraversal();
        while (auto *actor = actors->GetNextActor()) {
            auto *property = actor->GetProperty();
            if (property == nullptr) {
                continue;
            }

            auto *mapper = actor->GetMapper();
            const bool scalarMapped = mapper != nullptr && mapper->GetScalarVisibility() != 0;

            if (property->GetRepresentation() == VTK_WIREFRAME) {
                setActorColor(property, theme.wire);
                property->SetLineWidth(1.35);
            }

            if (property->GetEdgeVisibility()) {
                property->SetEdgeColor(theme.edge[0], theme.edge[1], theme.edge[2]);
                if (!scalarMapped) {
                    setActorColor(property, theme.surface);
                }
            }
        }
    }

    vtkWidget_->renderWindow()->Render();
#endif
}

namespace dynamics26::gui {

void installAppearanceController(QApplication &app, QMainWindow &window)
{
    new AppearanceController(app, window);
}

} // namespace dynamics26::gui
