#pragma once

// Real application composition: Qt mouse/context events -> VTK provenance pick
// -> one visible menu -> canonical document command. No bridge signals injected.
#include "BoundarySelectionAuthoringAcceptance.h"
#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPointer>
#include <QSet>
#include <QPushButton>
#include <QToolBar>
#include "../shell/CommandRegistry.h"
#ifdef FEMCAE_GUI_HAS_VTK
#include <QVTKOpenGLNativeWidget.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#endif

namespace d26 {
inline int runViewportInteractionAcceptance(QApplication &app, Dynamics26MainWindow &window)
{
#ifndef FEMCAE_GUI_HAS_VTK
    Q_UNUSED(app)
    Q_UNUSED(window)
    std::cout << "FAIL  viewport interaction acceptance requires VTK\n";
    return 1;
#else
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        std::cout << (ok ? "PASS  " : "FAIL  ") << message << '\n';
        if (!ok) ++failures;
    };
    const auto flush = [&] {
        app.processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        app.processEvents();
    };
    auto services = window.services();
    auto *coordinator = boundary_selection_acceptance_detail::coordinator(window);
    auto *bridge = window.findChild<ViewportSelectionBridge *>();
    auto *graphics = window.graphics();
    auto *widget = graphics->viewport()->findChild<QVTKOpenGLNativeWidget *>();
    auto *undo = window.documentCommands()->stack();
    check(coordinator && bridge && widget, "interaction acceptance has real Qt/VTK application composition");
    if (!coordinator || !bridge || !widget) return 1;
    QTemporaryDir temporary;
    const auto baseline = temporary.filePath(QStringLiteral("interaction-baseline.json"));
    const auto saved = temporary.filePath(QStringLiteral("interaction-authored.json"));
    if (!temporary.isValid() || !window.saveProjectToPath(baseline)) return 1;
    window.selectObject(services.project->geometryNode());
    flush();
    window.selectObject(services.project->namedSelectionsNode()); flush();
    auto *newSelection = window.detailsHost()->findChild<QPushButton *>(QStringLiteral("Dynamics26NewNamedSelection"));
    check(newSelection && newSelection->isVisible(), "Named Selections folder has visible New Named Selection CTA");
    const int beforeBegin = undo->index();
    if (newSelection) newSelection->click();
    flush();
    check(undo->index() == beforeBegin && graphics->selectionFilter() == SelectionFilter::Face,
          "folder CTA starts Face selection without creating an empty scope or Undo entry");
    auto *createCommand = window.commandRegistry()->action(QStringLiteral("selection.createNamed"));
    check(createCommand && !createCommand->isEnabled(), "Create Named Selection is disabled without selection");
    graphics->setSelectionFilter(SelectionFilter::Face);
    graphics->viewport()->setIsometricView();
    flush();
    auto *renderer = widget->renderWindow()->GetRenderers()->GetFirstRenderer();
    const auto toScreen = [&](const femcae::geometry::Vec3 &p) {
        renderer->SetWorldPoint(p.x, p.y, p.z, 1.0);
        renderer->WorldToDisplay();
        const auto *display = renderer->GetDisplayPoint();
        const auto *size = widget->renderWindow()->GetSize();
        return QPoint(qRound(display[0] * widget->width() / size[0]),
                      qRound((size[1] - 1 - display[1]) * widget->height() / size[1]));
    };
    QVector<QPoint> positions;
    QSet<quint64> faceIds;
    for (const auto &surface : services.mesh->displaySelectionTopologyScene(0.15)) {
        for (const auto &tri : surface.display.triangles) {
            femcae::geometry::Vec3 p;
            for (auto index : tri) {
                const auto &v = surface.display.points[index];
                p.x += v.x / 3; p.y += v.y / 3; p.z += v.z / 3;
            }
            const auto position = toScreen(p);
            const auto hit = bridge->pickAtGlobalPosition(widget->mapToGlobal(position));
            if (hit && !faceIds.contains(hit->geometryEntityId)) {
                positions.push_back(position);
                faceIds.insert(hit->geometryEntityId);
            }
            if (positions.size() >= 2) break;
        }
        if (positions.size() >= 2) break;
    }
    check(positions.size() >= 2, "real visible geometry provides two distinct Face picks");
    if (positions.size() < 2) return 1;
    const auto mouse = [&](QEvent::Type type, QPoint pos, Qt::MouseButton button,
                           Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        QMouseEvent event(type, QPointF(pos), QPointF(widget->mapToGlobal(pos)), button, buttons, modifiers);
        QApplication::sendEvent(widget, &event);
    };
    const auto click = [&](QPoint pos, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        mouse(QEvent::MouseButtonPress, pos, Qt::LeftButton, Qt::LeftButton, modifiers);
        mouse(QEvent::MouseButtonRelease, pos, Qt::LeftButton, Qt::NoButton, modifiers);
        flush();
    };
    const auto secondaryClick = [&](QPoint pos) -> QPointer<QMenu> {
        mouse(QEvent::MouseButtonPress, pos, Qt::RightButton, Qt::RightButton);
        QContextMenuEvent event(QContextMenuEvent::Mouse, pos, widget->mapToGlobal(pos));
        QApplication::sendEvent(widget, &event);
        mouse(QEvent::MouseButtonRelease, pos, Qt::RightButton, Qt::NoButton);
        flush();
        QPointer<QMenu> menu;
        int visible = 0;
        for (auto *top : QApplication::topLevelWidgets()) {
            if (auto *candidate = qobject_cast<QMenu *>(top); candidate && candidate->isVisible()) {
                ++visible; menu = candidate;
            }
        }
        check(visible == 1 && menu && menu->objectName() == QStringLiteral("Dynamics26ViewportContextMenu"),
              "one physical secondary-click sequence opens exactly one canonical menu");
        return menu;
    };
    const auto action = [](QMenu *menu, const char *label) -> QAction * {
        if (menu) for (auto *a : menu->actions())
            if (a->text().section(QLatin1Char('\t'), 0, 0) == QString::fromUtf8(label)) return a;
        return nullptr;
    };
    click(positions[0]);
    check(coordinator->selectionManager()->items().size() == 1,
          "real Face click commits one transient selection");
    bool visibleCommand = false;
    for (auto *toolbar : window.findChildren<QToolBar *>())
        visibleCommand |= toolbar->isVisible() && toolbar->actions().contains(createCommand);
    check(createCommand && createCommand->isEnabled() && visibleCommand,
          "Face selection enables visible shared Create Named Selection command");
    const int before = undo->index();
    auto menu = secondaryClick(positions[0]);
    check(action(menu, "Create Named Selection") && action(menu, "Fixed Support from Selection")
              && action(menu, "Force from Selection") && action(menu, "Fit View")
              && action(menu, "Standard View"),
          "visible Face menu exposes scope authoring and camera navigation together");
    auto *pivot = action(menu, "Set Rotation Center to Selection");
    check(pivot && pivot->isEnabled(), "Face selection enables its rotation center without BC highlight");
    if (pivot) pivot->trigger();
    check(undo->index() == before, "right click and rotation center leave document Undo unchanged");
    auto *create = action(menu, "Create Named Selection");
    if (create) create->trigger();
    if (menu) menu->close();
    flush();
    const ObjectId created = window.navigator()->selectedObject();
    const auto *definition = services.namedSelections->byId(created);
    check(definition && definition->scope.entities.size() == 1 && undo->index() == before + 1
              && window.detailsHost()->currentObject() == created,
          "visible menu command creates one persistent Named Selection, tree and Details transaction");
    if (definition) {
        const auto scope = definition->scope;
        undo->undo(); flush();
        check(!services.namedSelections->byId(created), "one Undo removes created Named Selection");
        undo->redo(); flush();
        definition = services.namedSelections->byId(created);
        check(definition && definition->scope.entities.size() == scope.entities.size()
                  && services.namedSelections->validate(created) == ScopeReferenceValidationError::None,
              "Redo restores the same ObjectId and valid engineering relationship");
        check(window.saveProjectToPath(saved) && window.openProjectFromPath(saved),
              "interaction-authored model saves and reopens");
        flush();
        check(services.namedSelections->byId(created)
                  && services.namedSelections->validate(created) == ScopeReferenceValidationError::None,
              "save/reopen preserves selected Face identities");
    }
    window.selectObject(services.project->geometryNode()); flush();
    graphics->setSelectionFilter(SelectionFilter::Face);
    graphics->viewport()->setIsometricView(); flush();
    click(positions[0]); click(positions[1], Qt::ShiftModifier);
    check(coordinator->selectionManager()->items().size() == 2, "Shift-click preserves two distinct Faces");
    menu = secondaryClick(positions[1]);
    create = action(menu, "Create Named Selection");
    check(create == createCommand, "context menu and command surface share the identical QAction");
    if (menu) menu->close();
    check(window.runCommand(QStringLiteral("selection.createNamed")), "visible command creates multi-Face Named Selection");
    if (menu) menu->close();
    flush();
    definition = services.namedSelections->byId(window.navigator()->selectedObject());
    check(definition && definition->scope.entities.size() == 2,
          "multi-Face menu creates one scope containing both Faces");
    window.selectObject(services.project->geometryNode()); flush();
    menu = secondaryClick(QPoint(2, widget->height()/2));
    check(menu && !action(menu, "Create Named Selection") && !action(menu, "Force from Selection")
              && action(menu, "Fit View") && !action(menu, "Set Rotation Center to Selection"),
          "empty viewport menu only exposes navigation, never stale engineering actions");
    if (menu) menu->close();
    flush();
    for (const bool supportKind : {true, false}) {
        window.selectObject(services.project->geometryNode()); flush();
        graphics->setSelectionFilter(SelectionFilter::Face);
        graphics->viewport()->setIsometricView(); flush();
        click(positions[0]);
        if (!supportKind) click(positions[1], Qt::ShiftModifier);
        const int beforeBoundary = undo->index();
        const auto commandId = supportKind ? QStringLiteral("selection.createSupport") : QStringLiteral("selection.createForce");
        menu = secondaryClick(positions[0]);
        auto *boundaryAction = action(menu, supportKind ? "Fixed Support from Selection" : "Force from Selection");
        check(boundaryAction && boundaryAction == window.commandRegistry()->action(commandId) && boundaryAction->isEnabled(),
              "Face menu shares enabled canonical boundary authoring command");
        if (boundaryAction) boundaryAction->trigger();
        if (menu) menu->close();
        flush();
        const ObjectId boundaryId = window.navigator()->selectedObject();
        const auto *support = services.analysis->support(boundaryId);
        const auto *force = services.analysis->load(boundaryId);
        const ObjectId scopeId = supportKind ? (support ? support->namedSelectionId : InvalidObjectId)
                                            : (force ? force->namedSelectionId : InvalidObjectId);
        const auto *scope = services.namedSelections->byId(scopeId);
        check(scope && scope->scope.entities.size() == (supportKind ? 1 : 2) && undo->index() == beforeBoundary + 1,
              "physical Face menu creates scope plus BC/load in one Undo transaction");
        if (scope) {
            undo->undo(); flush();
            check(!services.project->object(boundaryId) && !services.namedSelections->byId(scopeId),
                  "one Undo removes boundary object and its owned Named Selection");
            undo->redo(); flush();
            support = services.analysis->support(boundaryId); force = services.analysis->load(boundaryId);
            check((supportKind ? support && support->namedSelectionId == scopeId : force && force->namedSelectionId == scopeId)
                      && services.namedSelections->validate(scopeId) == ScopeReferenceValidationError::None,
                  "Redo restores the same boundary and scope ObjectId relationship");
            check(window.saveProjectToPath(saved) && window.openProjectFromPath(saved), "boundary relationship saves and reopens");
            flush();
            check(services.namedSelections->validate(scopeId) == ScopeReferenceValidationError::None,
                  "reopened boundary keeps persistent Face identities");
        }
    }
    check(window.openProjectFromPath(baseline), "interaction acceptance restores original project");
    flush();
    std::cout << "Viewport interaction acceptance " << (failures ? "FAIL" : "PASS") << '\n';
    return failures ? 1 : 0;
#endif
}
} // namespace d26
