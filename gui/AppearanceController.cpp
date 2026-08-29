#include "AppearanceController.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QColor>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPalette>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <QTreeWidget>

#ifdef FEMCAE_GUI_HAS_VTK
#include <QVTKOpenGLNativeWidget.h>
#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#endif

namespace {

enum class AppearanceMode {
    System,
    Light,
    Dark
};

QString normalizedMenuText(QString text)
{
    text.remove(QChar('&'));
    return text.trimmed();
}

QMenu *findViewMenu(QMainWindow &window)
{
    if (window.menuBar() == nullptr) {
        return nullptr;
    }
    for (auto *action : window.menuBar()->actions()) {
        if (action != nullptr && normalizedMenuText(action->text()) == QStringLiteral("Görünüm")) {
            return action->menu();
        }
    }
    return nullptr;
}

QPalette lightPalette(const QPalette &system)
{
    QPalette p(system);
    p.setColor(QPalette::Window, QColor(QStringLiteral("#F5F5F7")));
    p.setColor(QPalette::WindowText, QColor(QStringLiteral("#1D1D1F")));
    p.setColor(QPalette::Base, QColor(QStringLiteral("#FFFFFF")));
    p.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#F2F2F4")));
    p.setColor(QPalette::Text, QColor(QStringLiteral("#1D1D1F")));
    p.setColor(QPalette::Button, QColor(QStringLiteral("#F2F2F4")));
    p.setColor(QPalette::ButtonText, QColor(QStringLiteral("#1D1D1F")));
    p.setColor(QPalette::Highlight, QColor(QStringLiteral("#0A84FF")));
    p.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#FFFFFF")));
    p.setColor(QPalette::ToolTipBase, QColor(QStringLiteral("#FFFFFF")));
    p.setColor(QPalette::ToolTipText, QColor(QStringLiteral("#1D1D1F")));
    p.setColor(QPalette::Link, QColor(QStringLiteral("#0066CC")));
    p.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#8E8E93")));

    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(QStringLiteral("#8E8E93")));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(QStringLiteral("#8E8E93")));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(QStringLiteral("#8E8E93")));
    return p;
}

QPalette darkPalette(const QPalette &system)
{
    QPalette p(system);
    p.setColor(QPalette::Window, QColor(QStringLiteral("#1D1D1F")));
    p.setColor(QPalette::WindowText, QColor(QStringLiteral("#F5F5F7")));
    p.setColor(QPalette::Base, QColor(QStringLiteral("#171719")));
    p.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#242426")));
    p.setColor(QPalette::Text, QColor(QStringLiteral("#F2F2F7")));
    p.setColor(QPalette::BrightText, QColor(QStringLiteral("#FFFFFF")));
    p.setColor(QPalette::Button, QColor(QStringLiteral("#2C2C2E")));
    p.setColor(QPalette::ButtonText, QColor(QStringLiteral("#F2F2F7")));
    p.setColor(QPalette::Light, QColor(QStringLiteral("#48484A")));
    p.setColor(QPalette::Midlight, QColor(QStringLiteral("#3A3A3C")));
    p.setColor(QPalette::Mid, QColor(QStringLiteral("#323234")));
    p.setColor(QPalette::Dark, QColor(QStringLiteral("#202022")));
    p.setColor(QPalette::Shadow, QColor(QStringLiteral("#111113")));
    p.setColor(QPalette::Highlight, QColor(QStringLiteral("#0A84FF")));
    p.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#FFFFFF")));
    p.setColor(QPalette::ToolTipBase, QColor(QStringLiteral("#2C2C2E")));
    p.setColor(QPalette::ToolTipText, QColor(QStringLiteral("#F2F2F7")));
    p.setColor(QPalette::Link, QColor(QStringLiteral("#64D2FF")));
    p.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#8E8E93")));

    // Disabled alanlar okunabilir kalır fakat aktif alanlarla karışmaz. macOS
    // native widget style bazı editable kontrollerde yalnız QPalette'i kısmen
    // uyguladığı için aşağıdaki semantic QSS ile birlikte kullanılır.
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(QStringLiteral("#8E8E93")));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(QStringLiteral("#8E8E93")));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(QStringLiteral("#8E8E93")));
    p.setColor(QPalette::Disabled, QPalette::Base, QColor(QStringLiteral("#222224")));
    return p;
}

bool paletteIsDark(const QPalette &palette)
{
    return palette.color(QPalette::Window).lightness() < 128;
}

QString darkSemanticStyleSheet()
{
    // Alpha.1'de QSS bir skin değildir. Yalnız macOS native widget style'ın
    // custom palette'i uygulamadığı yüzeylerde semantic palette rollerinin
    // görünür karşılığını garanti eder. Layout/geometry bu katmanda değişmez.
    return QStringLiteral(R"(
        QMainWindow,
        QFrame#Dynamics26InspectorPanel,
        QScrollArea,
        QScrollArea > QWidget > QWidget {
            background-color: #1D1D1F;
            color: #F2F2F7;
        }

        QTreeWidget#Dynamics26Navigator {
            background-color: #1B1B1D;
            color: #EDEDF2;
            border: 0;
            outline: 0;
            selection-background-color: #0A5EA8;
            selection-color: #FFFFFF;
        }
        QTreeWidget#Dynamics26Navigator::item {
            min-height: 22px;
            padding: 1px 4px;
            background: transparent;
            color: #EDEDF2;
        }
        QTreeWidget#Dynamics26Navigator::item:hover {
            background-color: #2A2A2D;
        }
        QTreeWidget#Dynamics26Navigator::item:selected {
            background-color: #0A5EA8;
            color: #FFFFFF;
        }

        QLineEdit,
        QSpinBox,
        QDoubleSpinBox,
        QComboBox {
            background-color: #2C2C2E;
            color: #F2F2F7;
            border: 1px solid #48484A;
            border-radius: 4px;
            min-height: 24px;
            padding: 1px 6px;
            selection-background-color: #0A84FF;
            selection-color: #FFFFFF;
        }
        QLineEdit:focus,
        QSpinBox:focus,
        QDoubleSpinBox:focus,
        QComboBox:focus {
            border-color: #0A84FF;
        }
        QLineEdit:disabled,
        QSpinBox:disabled,
        QDoubleSpinBox:disabled,
        QComboBox:disabled {
            background-color: #222224;
            color: #8E8E93;
            border-color: #3A3A3C;
        }
        QComboBox QAbstractItemView {
            background-color: #2C2C2E;
            color: #F2F2F7;
            border: 1px solid #48484A;
            selection-background-color: #0A5EA8;
            selection-color: #FFFFFF;
        }

        QPushButton {
            background-color: #323234;
            color: #F2F2F7;
            border: 1px solid #48484A;
            border-radius: 5px;
            min-height: 24px;
            padding: 2px 10px;
        }
        QPushButton:hover {
            background-color: #3A3A3C;
        }
        QPushButton:pressed {
            background-color: #242426;
        }
        QPushButton:disabled {
            background-color: #252527;
            color: #77777C;
            border-color: #343436;
        }

        QToolButton {
            color: #F2F2F7;
            background-color: transparent;
        }
        QToolButton#Dynamics26AdvancedSolverDisclosure {
            background-color: #2C2C2E;
            color: #F2F2F7;
            border: 1px solid #48484A;
            border-radius: 4px;
            padding: 4px 6px;
            text-align: left;
        }
        QToolButton#Dynamics26AdvancedSolverDisclosure:hover {
            background-color: #343436;
        }

        QGroupBox {
            color: #F2F2F7;
            border: 0;
            background-color: transparent;
        }
        QLabel {
            color: #F2F2F7;
            background-color: transparent;
        }

        QTableWidget,
        QPlainTextEdit {
            background-color: #161618;
            color: #EDEDF2;
            border: 1px solid #3A3A3C;
            gridline-color: #343436;
            selection-background-color: #0A5EA8;
            selection-color: #FFFFFF;
        }
        QHeaderView::section {
            background-color: #2C2C2E;
            color: #D1D1D6;
            border: 0;
            border-right: 1px solid #3A3A3C;
            border-bottom: 1px solid #3A3A3C;
            padding: 4px 6px;
        }

        QTabWidget::pane {
            background-color: #1D1D1F;
            border: 1px solid #3A3A3C;
        }
        QTabBar::tab {
            background-color: #242426;
            color: #AEAEB2;
            border: 0;
            padding: 5px 11px;
        }
        QTabBar::tab:selected {
            background-color: #343436;
            color: #FFFFFF;
        }

        QStatusBar {
            background-color: #1D1D1F;
            color: #D1D1D6;
            border-top: 1px solid #3A3A3C;
        }
        QStatusBar QLabel,
        QStatusBar QToolButton {
            color: #D1D1D6;
            background: transparent;
        }

        QToolTip {
            background-color: #2C2C2E;
            color: #F2F2F7;
            border: 1px solid #48484A;
            padding: 4px 6px;
        }
    )");
}

void updateViewportAppearance(QMainWindow &window, bool dark)
{
#ifdef FEMCAE_GUI_HAS_VTK
    const auto vtkWidgets = window.findChildren<QVTKOpenGLNativeWidget *>();
    for (auto *widget : vtkWidgets) {
        if (widget == nullptr || widget->renderWindow() == nullptr) {
            continue;
        }
        auto *renderers = widget->renderWindow()->GetRenderers();
        if (renderers == nullptr) {
            continue;
        }
        renderers->InitTraversal();
        while (auto *renderer = renderers->GetNextItem()) {
            if (dark) {
                renderer->SetBackground(0.055, 0.067, 0.075);
            } else {
                renderer->SetBackground(0.965, 0.968, 0.975);
            }

            // Result scalar map'lerine dokunulmaz. Yalnız neutral/wireframe
            // geometri ve mesh çizgileri dark viewport üzerinde okunabilir hale
            // getirilir; böylece Geometry/Mesh/Analysis semantiği korunur.
            auto *actors = renderer->GetActors();
            if (actors != nullptr) {
                actors->InitTraversal();
                while (auto *actor = actors->GetNextActor()) {
                    auto *property = actor->GetProperty();
                    auto *mapper = actor->GetMapper();
                    if (property == nullptr) {
                        continue;
                    }

                    if (property->GetEdgeVisibility()) {
                        if (dark) {
                            property->SetEdgeColor(0.70, 0.74, 0.80);
                        } else {
                            property->SetEdgeColor(0.18, 0.20, 0.24);
                        }
                    }

                    const bool scalarMapped = mapper != nullptr && mapper->GetScalarVisibility() != 0;
                    if (!scalarMapped && property->GetRepresentation() == VTK_WIREFRAME) {
                        if (dark) {
                            property->SetColor(0.48, 0.68, 0.92);
                        }
                    }
                }
            }
        }
        widget->renderWindow()->Render();
    }
#else
    Q_UNUSED(window)
    Q_UNUSED(dark)
#endif
}

class AppearanceController final : public QObject
{
public:
    AppearanceController(QApplication &app, QMainWindow &window)
        : QObject(&window), app_(app), window_(window), systemPalette_(app.palette())
    {
        installMenu();
        installPreviewIdentity();
        watchViewportContextChanges();

        QSettings settings;
        const QString saved = settings.value(QStringLiteral("ui/appearance"), QStringLiteral("system")).toString();
        if (saved == QStringLiteral("dark")) {
            apply(AppearanceMode::Dark, false);
        } else if (saved == QStringLiteral("light")) {
            apply(AppearanceMode::Light, false);
        } else {
            apply(AppearanceMode::System, false);
        }
    }

private:
    void installMenu()
    {
        QMenu *viewMenu = findViewMenu(window_);
        if (viewMenu == nullptr) {
            return;
        }

        auto *appearanceMenu = viewMenu->addMenu(QStringLiteral("Görünüm Modu"));
        appearanceMenu->setObjectName(QStringLiteral("Dynamics26AppearanceMenu"));

        auto *group = new QActionGroup(appearanceMenu);
        group->setExclusive(true);

        systemAction_ = appearanceMenu->addAction(QStringLiteral("Sistem"));
        lightAction_ = appearanceMenu->addAction(QStringLiteral("Açık"));
        darkAction_ = appearanceMenu->addAction(QStringLiteral("Koyu"));
        for (auto *action : {systemAction_, lightAction_, darkAction_}) {
            action->setCheckable(true);
            group->addAction(action);
        }

        systemAction_->setToolTip(QStringLiteral("Dynamics26 görünümünü macOS sistem görünümüyle başlat"));
        lightAction_->setToolTip(QStringLiteral("Dynamics26 için açık görünümü kullan"));
        darkAction_->setToolTip(QStringLiteral("Dynamics26 için koyu görünümü kullan"));

        connect(systemAction_, &QAction::triggered, this, [this] { apply(AppearanceMode::System, true); });
        connect(lightAction_, &QAction::triggered, this, [this] { apply(AppearanceMode::Light, true); });
        connect(darkAction_, &QAction::triggered, this, [this] { apply(AppearanceMode::Dark, true); });
    }

    void installPreviewIdentity()
    {
        auto *status = window_.statusBar();
        if (status == nullptr || status->findChild<QLabel *>(QStringLiteral("Dynamics26PreviewIdentity")) != nullptr) {
            return;
        }
        auto *label = new QLabel(QStringLiteral("V1.1 · α1"), status);
        label->setObjectName(QStringLiteral("Dynamics26PreviewIdentity"));
        label->setToolTip(QStringLiteral("Dynamics26 V1.1.0-alpha.1 engineering preview"));
        status->addPermanentWidget(label);
    }

    void watchViewportContextChanges()
    {
        auto *navigator = window_.findChild<QTreeWidget *>(QStringLiteral("Dynamics26Navigator"));
        if (navigator == nullptr) {
            return;
        }
        connect(navigator, &QTreeWidget::currentItemChanged, this,
                [this](QTreeWidgetItem *, QTreeWidgetItem *) {
                    // Navigator selection handler önce viewport içeriğini değiştirir.
                    // Bir event-loop tick sonra active theme'in neutral actor renkleri
                    // yeni VTK actor'larına tekrar uygulanır.
                    QTimer::singleShot(0, this, [this] {
                        updateViewportAppearance(window_, currentDark_);
                    });
                });
    }

    void apply(AppearanceMode mode, bool persist)
    {
        QPalette palette;
        QString setting;
        if (mode == AppearanceMode::Dark) {
            palette = darkPalette(systemPalette_);
            setting = QStringLiteral("dark");
            if (darkAction_ != nullptr) darkAction_->setChecked(true);
        } else if (mode == AppearanceMode::Light) {
            palette = lightPalette(systemPalette_);
            setting = QStringLiteral("light");
            if (lightAction_ != nullptr) lightAction_->setChecked(true);
        } else {
            palette = systemPalette_;
            setting = QStringLiteral("system");
            if (systemAction_ != nullptr) systemAction_->setChecked(true);
        }

        currentDark_ = paletteIsDark(palette);
        app_.setPalette(palette);
        window_.setPalette(palette);

        // System/Light için native Qt/macOS görünümü korunur. Forced Dark'ta ise
        // yalnız palette'i görmezden gelen native-style kontroller hedefli QSS ile
        // düzeltilir; bu Alpha.1'in eski global light-only QSS yaklaşımına dönüşmez.
        window_.setStyleSheet(mode == AppearanceMode::Dark ? darkSemanticStyleSheet() : QString());
        updateViewportAppearance(window_, currentDark_);

        if (persist) {
            QSettings settings;
            settings.setValue(QStringLiteral("ui/appearance"), setting);
        }
    }

    QApplication &app_;
    QMainWindow &window_;
    QPalette systemPalette_;
    bool currentDark_ = false;
    QAction *systemAction_ = nullptr;
    QAction *lightAction_ = nullptr;
    QAction *darkAction_ = nullptr;
};

} // namespace

namespace dynamics26::gui {

void installAppearanceController(QApplication &app, QMainWindow &window)
{
    new AppearanceController(app, window);
}

} // namespace dynamics26::gui