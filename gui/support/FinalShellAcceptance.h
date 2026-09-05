#pragma once

// Gerçek normal macOS penceresinde layout, komut ve persistent activation kabulü.
// Splitter sinyali taklit edilmez: handle gerçek mouse press/move/release alır.
#include "BoundarySelectionAuthoringAcceptance.h"
#include "../shell/CommandRegistry.h"
#include "../shell/GraphicsWorkspace.h"
#include "../shell/ProjectNavigator.h"
#include "../shell/DetailsHost.h"
#include <QMenu>
#include <QMouseEvent>
#include <QSplitter>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <array>
#include <utility>

namespace d26 {
inline int runFinalShellAcceptance(QApplication &app, Dynamics26MainWindow &window)
{
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        std::cout << (ok ? "PASS  " : "FAIL  ") << message << std::endl;
        if (!ok) ++failures;
    };
    const auto flush = [&] { app.processEvents(); app.processEvents(); };
    auto services = window.services();
    auto *undo = window.documentCommands()->stack();
    auto *registry = window.commandRegistry();
    auto *splitter = window.findChild<QSplitter *>(QStringLiteral("Dynamics26VerticalSplitter"));
    auto *collapse = window.findChild<QToolButton *>(QStringLiteral("Dynamics26UtilityCollapse"));
    auto *status = window.findChild<QToolButton *>(QStringLiteral("Dynamics26StatusDiagnostics"));
    auto *diagnostics = registry->action(QStringLiteral("panel.diagnostics"));
    check(splitter && collapse && status && diagnostics, "final shell has canonical Diagnostics controls");
    if (!splitter || !collapse || !status || !diagnostics) return 1;
    QTemporaryDir temporary;
    const auto original = temporary.filePath(QStringLiteral("original.json"));
    const auto suppressed = temporary.filePath(QStringLiteral("suppressed.json"));
    if (!temporary.isValid() || !window.saveProjectToPath(original)) return 1;
    window.showNormal();
    flush();
    const auto outer = window.geometry();
    const int transientUndo = undo->index();
    check(!window.utility()->isVisible() && !diagnostics->isChecked() && !status->isChecked(),
          "Diagnostics starts closed on all control surfaces");
    check(!splitter->isCollapsible(0) && splitter->isCollapsible(1),
          "only Diagnostics can collapse; viewport cannot collapse");
    diagnostics->trigger(); flush();
    check(window.utility()->isVisible() && diagnostics->isChecked() && status->isChecked()
              && window.geometry() == outer,
          "Diagnostics opens inside unchanged normal-window geometry with synchronized toggles");
    const auto drag = [&](int desiredHeight) {
        auto *handle = splitter->handle(1);
        const QPoint start = handle->mapToGlobal(handle->rect().center());
        const QPoint end = splitter->mapToGlobal(QPoint(splitter->width()/2,
            splitter->height() - desiredHeight - handle->height()/2));
        const auto send = [&](QEvent::Type type, QPoint global, Qt::MouseButton button, Qt::MouseButtons buttons) {
            QMouseEvent event(type, QPointF(handle->mapFromGlobal(global)), QPointF(global), button, buttons, Qt::NoModifier);
            QApplication::sendEvent(handle, &event);
        };
        send(QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
        send(QEvent::MouseMove, end, Qt::NoButton, Qt::LeftButton);
        send(QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);
        flush();
    };
    drag(80);
    check(window.utility()->isVisible() && splitter->sizes().value(1) > 0
              && splitter->sizes().value(1) <= 100 && window.geometry() == outer,
          "real splitter drag shrinks Diagnostics below 100 px without growing window");
    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("Dynamics26UtilityTabs"));
    if (tabs) for (int i = 0; i < tabs->count(); ++i) {
        tabs->setCurrentIndex(i); flush();
        check(window.geometry() == outer, "switching Diagnostics tabs preserves outer window geometry");
    }
    drag(0);
    check(!window.utility()->isVisible() && !diagnostics->isChecked() && !status->isChecked(),
          "drag-to-zero dismisses Diagnostics and synchronizes both toggles");
    status->click(); flush();
    check(window.utility()->isVisible() && diagnostics->isChecked() && window.geometry() == outer,
          "status icon reopens collapsed Diagnostics within existing window");
    collapse->click(); flush();
    check(!window.utility()->isVisible() && !status->isChecked() && !diagnostics->isChecked(),
          "in-panel collapse clears every toggle");
    for (const auto *name : {"Dynamics26RibbonNavigator", "Dynamics26RibbonDetails", "Dynamics26RibbonDiagnostics"}) {
        auto *button = window.findChild<QToolButton *>(QLatin1String(name));
        check(button && button->isVisible() && button->toolButtonStyle() == Qt::ToolButtonIconOnly
                  && !button->icon().isNull() && !button->toolTip().isEmpty() && !button->accessibleName().isEmpty(),
              "panel controls are visible icon-only accessible actions");
    }
    for (const auto *id : {"panel.navigator", "panel.details"}) {
        auto *action = registry->action(QLatin1String(id));
        QWidget *panel = QString::fromLatin1(id) == QStringLiteral("panel.navigator")
            ? static_cast<QWidget *>(window.navigator()) : static_cast<QWidget *>(window.detailsHost());
        action->trigger(); flush();
        check(!panel->isVisible() && !action->isChecked(), "panel icon hides its own panel");
        action->trigger(); flush();
        check(panel->isVisible() && action->isChecked(), "panel icon restores its own panel");
    }
    const auto analysis = window.firstObjectOfType(ObjectType::Analysis);
    const auto *record = services.analysis->analysis(analysis);
    if (!record) return 1;
    const auto solution = record->solutionNode;
    const std::array<std::pair<const char *, ObjectId>, 5> categories{{
        {"Geometry", services.project->geometryNode()}, {"Material", services.project->materialsNode()},
        {"Mesh", services.project->meshNode()}, {"Analysis", analysis}, {"Results", solution}}};
    for (const auto &[suffix, object] : categories) {
        auto *button = window.findChild<QToolButton *>(QStringLiteral("Dynamics26Ribbon") + QLatin1String(suffix));
        if (button) button->click(); flush();
        check(button && button->isChecked() && window.navigator()->selectedObject() == object,
              "ribbon category navigates through canonical ProjectModel selection");
    }
    check(registry->action(QStringLiteral("file.new"))->shortcut() == QKeySequence(QStringLiteral("Ctrl+N"))
              && registry->action(QStringLiteral("file.open"))->shortcut() == QKeySequence(QStringLiteral("Ctrl+O"))
              && registry->action(QStringLiteral("file.save"))->shortcut() == QKeySequence(QStringLiteral("Ctrl+S")),
          "native File command shortcuts remain unchanged");
    window.selectObject(services.project->namedSelectionsNode()); flush();
    for (auto filter : {SelectionFilter::Body, SelectionFilter::Face, SelectionFilter::Edge, SelectionFilter::Vertex})
        check(window.graphics()->filterAvailable(filter), "Named Selections exposes each available CAD topology filter");
    auto *context = window.findChild<QToolBar *>(QStringLiteral("Dynamics26ContextSurface"));
    check(context && context->actions().contains(registry->action(QStringLiteral("selection.beginNamed")))
              && context->actions().contains(registry->action(QStringLiteral("selection.createNamed"))),
          "Named Selections shares New/Create actions without duplicating topology tools");
    check(undo->index() == transientUndo, "panel/ribbon/filter navigation creates no document Undo");

    window.selectObject(analysis); flush();
    QMenu *menu = window.buildContextMenu(analysis, &window);
    auto *deactivate = registry->action(QStringLiteral("edit.suppress"));
    check(menu && menu->actions().contains(deactivate) && deactivate->isEnabled(),
          "Analysis context menu exposes enabled Pasife Al");
    const int before = undo->index();
    deactivate->trigger(); flush(); delete menu;
    check(services.project->isSuppressed(analysis) && services.project->isEffectivelySuppressed(solution)
              && !services.analysis->preflight(analysis).passed() && undo->index() == before + 1,
          "deactivation suppresses analysis and children, blocks solve, creates one Undo");
    undo->undo(); flush();
    check(!services.project->isEffectivelySuppressed(solution), "Undo restores active analysis relationship");
    undo->redo(); flush();
    check(services.project->isEffectivelySuppressed(solution), "Redo restores inherited suppression");
    check(window.saveProjectToPath(suppressed) && window.openProjectFromPath(suppressed),
          "suppressed analysis saves and reopens");
    check(services.project->isSuppressed(analysis) && services.project->isEffectivelySuppressed(solution),
          "analysis and child ObjectIds preserve suppression after reopen");
    window.selectObject(analysis); flush();
    menu = window.buildContextMenu(analysis, &window);
    auto *activate = registry->action(QStringLiteral("edit.unsuppress"));
    check(menu && menu->actions().contains(activate) && activate->text() == QStringLiteral("Aktifleştir"),
          "suppressed Analysis context menu exposes Aktifleştir");
    activate->trigger(); flush(); delete menu;
    check(!services.project->isSuppressed(analysis) && !services.project->isEffectivelySuppressed(solution),
          "reactivation restores canonical analysis and children");
    check(window.openProjectFromPath(original), "final shell acceptance restores original authoring model");
    flush();
    return failures;
}
} // namespace d26
