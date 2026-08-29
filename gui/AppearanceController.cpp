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

#ifdef FEMCAE_GUI_HAS_VTK
#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
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
    p.setColor(QPalette::Base, QColor(QStringLiteral("#141416")));
    p.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#242426")));
    p.setColor(QPalette::Text, QColor(QStringLiteral("#F2F2F7")));
    p.setColor(QPalette::Button, QColor(QStringLiteral("#2C2C2E")));
    p.setColor(QPalette::ButtonText, QColor(QStringLiteral("#F2F2F7")));
    p.setColor(QPalette::Highlight, QColor(QStringLiteral("#0A84FF")));
    p.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#FFFFFF")));
    p.setColor(QPalette::ToolTipBase, QColor(QStringLiteral("#2C2C2E")));
    p.setColor(QPalette::ToolTipText, QColor(QStringLiteral("#F2F2F7")));
    p.setColor(QPalette::Link, QColor(QStringLiteral("#64D2FF")));
    p.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#8E8E93")));

    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(QStringLiteral("#6E6E73")));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(QStringLiteral("#6E6E73")));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(QStringLiteral("#6E6E73")));
    return p;
}

bool paletteIsDark(const QPalette &palette)
{
    return palette.color(QPalette::Window).lightness() < 128;
}

void updateViewportBackground(QMainWindow &window, bool dark)
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

        app_.setPalette(palette);
        window_.setPalette(palette);
        updateViewportBackground(window_, paletteIsDark(palette));

        if (persist) {
            QSettings settings;
            settings.setValue(QStringLiteral("ui/appearance"), setting);
        }
    }

    QApplication &app_;
    QMainWindow &window_;
    QPalette systemPalette_;
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
