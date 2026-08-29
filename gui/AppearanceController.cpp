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
#include <QStyle>
#include <QTimer>
#include <QTreeWidget>
#include <QWidget>

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

struct ThemeTokens {
    QColor window;
    QColor panel;
    QColor raised;
    QColor field;
    QColor viewport;
    QColor text;
    QColor secondary;
    QColor muted;
    QColor border;
    QColor hover;
    QColor selection;
    QColor accent;
    QColor accentPressed;
    QColor disabledText;
    QColor disabledSurface;
    QColor tooltip;
    bool dark = false;
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

ThemeTokens lightTokens()
{
    ThemeTokens t;
    t.window = QColor(QStringLiteral("#EEF1F4"));
    t.panel = QColor(QStringLiteral("#F7F8FA"));
    t.raised = QColor(QStringLiteral("#FFFFFF"));
    t.field = QColor(QStringLiteral("#FFFFFF"));
    t.viewport = QColor(QStringLiteral("#F3F5F8"));
    t.text = QColor(QStringLiteral("#20242A"));
    t.secondary = QColor(QStringLiteral("#626B77"));
    t.muted = QColor(QStringLiteral("#8B94A0"));
    t.border = QColor(QStringLiteral("#D4D9E0"));
    t.hover = QColor(QStringLiteral("#E9EDF2"));
    t.selection = QColor(QStringLiteral("#DCEBFA"));
    t.accent = QColor(QStringLiteral("#007AFF"));
    t.accentPressed = QColor(QStringLiteral("#0064D1"));
    t.disabledText = QColor(QStringLiteral("#9AA2AD"));
    t.disabledSurface = QColor(QStringLiteral("#ECEFF3"));
    t.tooltip = QColor(QStringLiteral("#FFFFFF"));
    t.dark = false;
    return t;
}

ThemeTokens darkTokens()
{
    ThemeTokens t;
    t.window = QColor(QStringLiteral("#181A1D"));
    t.panel = QColor(QStringLiteral("#1F2226"));
    t.raised = QColor(QStringLiteral("#25292E"));
    t.field = QColor(QStringLiteral("#17191C"));
    t.viewport = QColor(QStringLiteral("#0F1418"));
    t.text = QColor(QStringLiteral("#F1F3F5"));
    t.secondary = QColor(QStringLiteral("#A6AFBA"));
    t.muted = QColor(QStringLiteral("#7F8995"));
    t.border = QColor(QStringLiteral("#373D44"));
    t.hover = QColor(QStringLiteral("#2A2F35"));
    t.selection = QColor(QStringLiteral("#223A52"));
    t.accent = QColor(QStringLiteral("#0A84FF"));
    t.accentPressed = QColor(QStringLiteral("#0874DE"));
    t.disabledText = QColor(QStringLiteral("#68717C"));
    t.disabledSurface = QColor(QStringLiteral("#20242A"));
    t.tooltip = QColor(QStringLiteral("#2A2F35"));
    t.dark = true;
    return t;
}

QPalette paletteFor(const ThemeTokens &t, const QPalette &system)
{
    QPalette p(system);
    p.setColor(QPalette::Window, t.window);
    p.setColor(QPalette::WindowText, t.text);
    p.setColor(QPalette::Base, t.field);
    p.setColor(QPalette::AlternateBase, t.panel);
    p.setColor(QPalette::Text, t.text);
    p.setColor(QPalette::BrightText, QColor(QStringLiteral("#FFFFFF")));
    p.setColor(QPalette::Button, t.raised);
    p.setColor(QPalette::ButtonText, t.text);
    p.setColor(QPalette::Highlight, t.accent);
    p.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#FFFFFF")));
    p.setColor(QPalette::ToolTipBase, t.tooltip);
    p.setColor(QPalette::ToolTipText, t.text);
    p.setColor(QPalette::Link, t.accent);
    p.setColor(QPalette::PlaceholderText, t.muted);
    p.setColor(QPalette::Light, t.raised.lighter(t.dark ? 118 : 106));
    p.setColor(QPalette::Midlight, t.border.lighter(t.dark ? 112 : 103));
    p.setColor(QPalette::Mid, t.border);
    p.setColor(QPalette::Dark, t.border.darker(t.dark ? 135 : 108));
    p.setColor(QPalette::Shadow, t.window.darker(t.dark ? 155 : 118));

    p.setColor(QPalette::Disabled, QPalette::WindowText, t.disabledText);
    p.setColor(QPalette::Disabled, QPalette::Text, t.disabledText);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, t.disabledText);
    p.setColor(QPalette::Disabled, QPalette::Base, t.disabledSurface);
    p.setColor(QPalette::Disabled, QPalette::Button, t.disabledSurface);
    return p;
}

QString css(const QColor &color)
{
    return color.name(QColor::HexRgb);
}

QString semanticStyleSheet(const ThemeTokens &t)
{
    // Bu katman görsel sistemdir; mühendislik widget'larının layout/ownership
    // mimarisini değiştirmez. Aynı semantic token'lar Açık ve Koyu modda ayrı
    // değer alır. Böylece bir moddan diğerine geçildiğinde eski QSS/palette
    // kalıntıları taşınmaz ve Qt'nin macOS style farkları kontrol altına alınır.
    return QStringLiteral(R"(
QMainWindow {
    background: %1;
    color: %6;
}

QToolBar#Dynamics26MainToolbar {
    background: %2;
    border: 0;
    border-bottom: 1px solid %9;
    spacing: 4px;
    padding: 2px 6px;
}
QToolBar#Dynamics26MainToolbar QToolButton {
    min-width: 26px;
    min-height: 26px;
    max-width: 30px;
    max-height: 30px;
    border: 1px solid transparent;
    border-radius: 4px;
    padding: 2px;
    background: transparent;
    color: %6;
}
QToolBar#Dynamics26MainToolbar QToolButton:hover {
    background: %10;
    border-color: %9;
}
QToolBar#Dynamics26MainToolbar QToolButton:checked {
    background: %11;
    border-color: %13;
}

QFrame#Dynamics26NavigatorPanel,
QFrame#Dynamics26InspectorPanel {
    background: %2;
    color: %6;
    border: 0;
}
QFrame#Dynamics26NavigatorPanel {
    border-right: 1px solid %9;
}
QFrame#Dynamics26InspectorPanel {
    border-left: 1px solid %9;
}
QFrame#Dynamics26NavigatorPanel > QLabel,
QFrame#Dynamics26InspectorPanel > QLabel {
    background: transparent;
    color: %6;
}

QTreeWidget#Dynamics26Navigator {
    background: %2;
    color: %6;
    border: 0;
    outline: 0;
    alternate-background-color: %2;
    selection-background-color: %11;
    selection-color: %6;
}
QTreeWidget#Dynamics26Navigator::item {
    min-height: 24px;
    padding: 1px 5px;
    border: 0;
    color: %6;
}
QTreeWidget#Dynamics26Navigator::item:hover {
    background: %10;
}
QTreeWidget#Dynamics26Navigator::item:selected {
    background: %11;
    color: %6;
}
QTreeWidget#Dynamics26Navigator::branch {
    background: transparent;
}

QScrollArea,
QScrollArea > QWidget > QWidget,
QStackedWidget#Dynamics26EngineeringInspector {
    background: %2;
    color: %6;
    border: 0;
}
QGroupBox {
    background: transparent;
    color: %6;
    border: 0;
    margin-top: 2px;
}
QLabel {
    background: transparent;
    color: %6;
}

QLineEdit,
QSpinBox,
QDoubleSpinBox,
QComboBox {
    background: %4;
    color: %6;
    border: 1px solid %9;
    border-radius: 4px;
    min-height: 24px;
    padding: 1px 6px;
    selection-background-color: %13;
    selection-color: white;
}
QLineEdit:hover,
QSpinBox:hover,
QDoubleSpinBox:hover,
QComboBox:hover {
    border-color: %8;
}
QLineEdit:focus,
QSpinBox:focus,
QDoubleSpinBox:focus,
QComboBox:focus {
    border-color: %13;
}
QLineEdit:disabled,
QSpinBox:disabled,
QDoubleSpinBox:disabled,
QComboBox:disabled {
    background: %16;
    color: %15;
    border-color: %9;
}
QComboBox QAbstractItemView {
    background: %3;
    color: %6;
    border: 1px solid %9;
    selection-background-color: %11;
    selection-color: %6;
}

QPushButton {
    background: %3;
    color: %6;
    border: 1px solid %9;
    border-radius: 4px;
    min-height: 25px;
    padding: 2px 10px;
}
QPushButton:hover {
    background: %10;
    border-color: %8;
}
QPushButton:pressed {
    background: %11;
}
QPushButton:disabled {
    background: %16;
    color: %15;
    border-color: %9;
}
QPushButton#Dynamics26IntegratedSolve:enabled {
    background: %13;
    color: white;
    border-color: %13;
    font-weight: 600;
}
QPushButton#Dynamics26IntegratedSolve:enabled:hover {
    background: %14;
    border-color: %14;
}

QCheckBox,
QRadioButton {
    color: %6;
    spacing: 6px;
}
QCheckBox:disabled,
QRadioButton:disabled {
    color: %15;
}

QToolButton {
    color: %6;
    background: transparent;
    border: 1px solid transparent;
    border-radius: 4px;
    padding: 3px 5px;
}
QToolButton:hover {
    background: %10;
    border-color: %9;
}
QToolButton#Dynamics26AdvancedSolverDisclosure {
    background: %3;
    color: %6;
    border: 1px solid %9;
    border-radius: 4px;
    padding: 5px 7px;
    text-align: left;
    font-weight: 500;
}
QToolButton#Dynamics26AdvancedSolverDisclosure:hover {
    background: %10;
}
QToolButton#Dynamics26DiagnosticsHandle {
    color: %7;
    padding: 1px 6px;
}

QDockWidget#Dynamics26UtilityArea {
    background: %2;
    color: %6;
    border-top: 1px solid %9;
}
QTabWidget#Dynamics26UtilityTabs::pane {
    background: %2;
    border: 0;
    border-top: 1px solid %9;
}
QTabWidget#Dynamics26UtilityTabs QTabBar::tab {
    background: transparent;
    color: %7;
    border: 0;
    border-bottom: 2px solid transparent;
    min-height: 24px;
    padding: 3px 12px;
}
QTabWidget#Dynamics26UtilityTabs QTabBar::tab:hover {
    color: %6;
    background: %10;
}
QTabWidget#Dynamics26UtilityTabs QTabBar::tab:selected {
    color: %6;
    border-bottom-color: %13;
    font-weight: 600;
}

QTableWidget,
QTableView,
QPlainTextEdit,
QTextEdit {
    background: %3;
    color: %6;
    border: 0;
    gridline-color: %9;
    selection-background-color: %11;
    selection-color: %6;
}
QHeaderView::section {
    background: %2;
    color: %7;
    border: 0;
    border-right: 1px solid %9;
    border-bottom: 1px solid %9;
    padding: 4px 7px;
    font-weight: 600;
}

QStatusBar {
    background: %2;
    color: %7;
    border-top: 1px solid %9;
    min-height: 22px;
}
QStatusBar QLabel,
QStatusBar QToolButton {
    background: transparent;
    color: %7;
}

QSplitter::handle {
    background: %9;
}
QSplitter::handle:horizontal {
    width: 1px;
}
QSplitter::handle:vertical {
    height: 1px;
}

QScrollBar:vertical {
    background: transparent;
    width: 10px;
    margin: 2px 1px;
}
QScrollBar::handle:vertical {
    background: %8;
    min-height: 28px;
    border-radius: 4px;
}
QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical,
QScrollBar::add-page:vertical,
QScrollBar::sub-page:vertical {
    background: transparent;
    height: 0;
}
QScrollBar:horizontal {
    background: transparent;
    height: 10px;
    margin: 1px 2px;
}
QScrollBar::handle:horizontal {
    background: %8;
    min-width: 28px;
    border-radius: 4px;
}
QScrollBar::add-line:horizontal,
QScrollBar::sub-line:horizontal,
QScrollBar::add-page:horizontal,
QScrollBar::sub-page:horizontal {
    background: transparent;
    width: 0;
}

QToolTip {
    background: %17;
    color: %6;
    border: 1px solid %9;
    padding: 4px 6px;
}
)")
        .arg(css(t.window))
        .arg(css(t.panel))
        .arg(css(t.raised))
        .arg(css(t.field))
        .arg(css(t.viewport))
        .arg(css(t.text))
        .arg(css(t.secondary))
        .arg(css(t.muted))
        .arg(css(t.border))
        .arg(css(t.hover))
        .arg(css(t.selection))
        .arg(css(t.accent))
        .arg(css(t.accent))
        .arg(css(t.accentPressed))
        .arg(css(t.disabledText))
        .arg(css(t.disabledSurface))
        .arg(css(t.tooltip));
}

void refreshWidgetStyles(QMainWindow &window)
{
    const auto widgets = window.findChildren<QWidget *>();
    for (auto *widget : widgets) {
        if (widget == nullptr || widget->style() == nullptr) {
            continue;
        }
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();
    }
    window.update();
}

void updateViewportAppearance(QMainWindow &window, const ThemeTokens &t)
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
            if (t.dark) {
                renderer->SetBackground(0.055, 0.071, 0.083);
            } else {
                renderer->SetBackground(0.953, 0.961, 0.973);
            }

            // Result scalar map'leri korunur. Neutral Geometry/Mesh/Analysis
            // actor'ları ise her tema geçişinde iki yönde de açıkça yeniden
            // renklendirilir. Bu, Dark -> Light geçişinde kalan koyu/blue actor
            // kalıntılarını da ortadan kaldırır.
            auto *actors = renderer->GetActors();
            if (actors == nullptr) {
                continue;
            }
            actors->InitTraversal();
            while (auto *actor = actors->GetNextActor()) {
                auto *property = actor->GetProperty();
                auto *mapper = actor->GetMapper();
                if (property == nullptr) {
                    continue;
                }

                const bool scalarMapped = mapper != nullptr && mapper->GetScalarVisibility() != 0;
                if (scalarMapped) {
                    continue;
                }

                if (property->GetRepresentation() == VTK_WIREFRAME) {
                    if (t.dark) {
                        property->SetColor(0.72, 0.78, 0.86);
                    } else {
                        property->SetColor(0.24, 0.29, 0.35);
                    }
                    property->SetLineWidth(1.35);
                }

                if (property->GetEdgeVisibility()) {
                    if (t.dark) {
                        property->SetColor(0.32, 0.36, 0.42);
                        property->SetEdgeColor(0.62, 0.67, 0.74);
                    } else {
                        property->SetColor(0.72, 0.76, 0.82);
                        property->SetEdgeColor(0.27, 0.31, 0.37);
                    }
                    property->SetLineWidth(1.0);
                }
            }
        }
        widget->renderWindow()->Render();
    }
#else
    Q_UNUSED(window)
    Q_UNUSED(t)
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

        systemAction_->setToolTip(QStringLiteral("Dynamics26 görünümünü macOS sistem görünümüyle eşleştir"));
        lightAction_->setToolTip(QStringLiteral("Dynamics26 açık görünümünü kullan"));
        darkAction_->setToolTip(QStringLiteral("Dynamics26 koyu görünümünü kullan"));

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
                    QTimer::singleShot(0, this, [this] {
                        updateViewportAppearance(window_, currentTheme_);
                    });
                });
    }

    void apply(AppearanceMode mode, bool persist)
    {
        QString setting;
        if (mode == AppearanceMode::Dark) {
            currentTheme_ = darkTokens();
            setting = QStringLiteral("dark");
            if (darkAction_ != nullptr) darkAction_->setChecked(true);
        } else if (mode == AppearanceMode::Light) {
            currentTheme_ = lightTokens();
            setting = QStringLiteral("light");
            if (lightAction_ != nullptr) lightAction_->setChecked(true);
        } else {
            currentTheme_ = systemPalette_.color(QPalette::Window).lightness() < 128
                ? darkTokens() : lightTokens();
            setting = QStringLiteral("system");
            if (systemAction_ != nullptr) systemAction_->setChecked(true);
        }

        const QPalette palette = paletteFor(currentTheme_, systemPalette_);
        app_.setPalette(palette);
        window_.setPalette(palette);

        // Her geçişte stylesheet tamamen yeniden kurulur. Özellikle Dark -> Light
        // dönüşünde önceki modun field/tree/table kuralları hiçbir widget üzerinde
        // kalmaz. Bu yaklaşım Alpha.1'in görünür shell'ini tek bir design-system
        // otoritesine bağlar.
        window_.setStyleSheet(QString());
        window_.setStyleSheet(semanticStyleSheet(currentTheme_));
        refreshWidgetStyles(window_);
        updateViewportAppearance(window_, currentTheme_);

        if (persist) {
            QSettings settings;
            settings.setValue(QStringLiteral("ui/appearance"), setting);
        }
    }

    QApplication &app_;
    QMainWindow &window_;
    QPalette systemPalette_;
    ThemeTokens currentTheme_ = lightTokens();
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
