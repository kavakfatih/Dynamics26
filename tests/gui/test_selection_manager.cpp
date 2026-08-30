#include "core/SelectionManager.h"
#include "viewport/ViewportSelectionController.h"

#include <QCoreApplication>

#include <iostream>
#include <string>

namespace {

int failures = 0;

void check(const bool condition, const std::string &message)
{
    std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
    failures += condition ? 0 : 1;
}

d26::SelectionItem body(const quint64 id, const quint64 revision)
{
    d26::SelectionItem item;
    item.domain = d26::SelectionDomain::Geometry;
    item.kind = d26::SelectionKind::Body;
    item.geometryEntityId = id;
    item.sourceRevision = revision;
    return item;
}

d26::SelectionItem face(const quint64 id, const quint64 parent, const quint64 revision)
{
    d26::SelectionItem item;
    item.domain = d26::SelectionDomain::Geometry;
    item.kind = d26::SelectionKind::Face;
    item.geometryEntityId = id;
    item.parentGeometryId = parent;
    item.sourceRevision = revision;
    return item;
}

void managerStateTests()
{
    d26::SelectionManager manager;
    int selectionSignals = 0;
    int preselectionSignals = 0;
    int policySignals = 0;
    QObject::connect(&manager, &d26::SelectionManager::selectionChanged,
                     [&selectionSignals] { ++selectionSignals; });
    QObject::connect(&manager, &d26::SelectionManager::preselectionChanged,
                     [&preselectionSignals] { ++preselectionSignals; });
    QObject::connect(&manager, &d26::SelectionManager::policyChanged,
                     [&policySignals] { ++policySignals; });

    const auto body1 = body(101, 7);
    const auto face1 = face(201, 101, 7);
    const auto face2 = face(202, 101, 7);
    const auto face3 = face(203, 101, 7);

    check(manager.policy().accepts(body1) && manager.policy().accepts(face1),
          "neutral geometry policy accepts Body and Face");
    check(manager.apply(body1, d26::SelectionOperation::Replace)
              && manager.items().size() == 1 && manager.primary() == body1,
          "Replace creates one primary selection");

    check(manager.apply(face1, d26::SelectionOperation::Add)
              && manager.items().size() == 2 && manager.items()[0] == body1
              && manager.items()[1] == face1 && manager.primary() == face1,
          "Add preserves deterministic insertion order and updates primary");

    const int signalsBeforeDuplicate = selectionSignals;
    check(!manager.apply(face1, d26::SelectionOperation::Add)
              && manager.items().size() == 2 && selectionSignals == signalsBeforeDuplicate,
          "duplicate Add does not create duplicate state or signal noise");

    check(manager.apply(face2, d26::SelectionOperation::Toggle)
              && manager.items().size() == 3 && manager.primary() == face2,
          "Toggle adds a missing entity");
    check(manager.apply(face2, d26::SelectionOperation::Toggle)
              && manager.items().size() == 2 && manager.primary() == face1,
          "Toggle removes an existing primary and falls back to newest remaining");

    check(manager.apply(body1, d26::SelectionOperation::Remove)
              && manager.items().size() == 1 && manager.items().front() == face1
              && manager.primary() == face1,
          "Remove deletes non-primary selection without disturbing primary");

    check(manager.setPreselection(face3) && manager.preselection() == face3,
          "preselection is independent from committed selection");
    check(manager.items().size() == 1 && manager.items().front() == face1,
          "preselection does not mutate committed selection");

    manager.setPolicy(d26::SelectionPolicy::preset(d26::SelectionPolicyPreset::SurfaceScope));
    check(policySignals == 1 && manager.items().size() == 1 && manager.items().front() == face1,
          "SurfaceScope policy preserves compatible Face selection");
    check(manager.preselection() == face3,
          "policy change preserves compatible Face preselection");

    check(!manager.apply(body1, d26::SelectionOperation::Add)
              && manager.items().size() == 1,
          "policy rejects incompatible Body selection");

    d26::SelectionPolicy bodyOnly = d26::SelectionPolicy::preset(d26::SelectionPolicyPreset::BodyMeshControl);
    manager.setPolicy(bodyOnly);
    check(manager.items().isEmpty() && !manager.primary().has_value()
              && !manager.preselection().has_value(),
          "policy change removes incompatible selection and preselection");

    manager.setPolicy(d26::SelectionPolicy::preset(d26::SelectionPolicyPreset::NeutralGeometry));
    check(manager.apply(face1, d26::SelectionOperation::Add)
              && manager.apply(face2, d26::SelectionOperation::Add),
          "multi-selection can be rebuilt after policy transition");
    check(manager.invalidateGeometryRevision(8) && manager.items().isEmpty()
              && !manager.primary().has_value(),
          "geometry revision change invalidates stale committed selection");

    check(selectionSignals > 0 && preselectionSignals > 0,
          "selection and preselection changes publish explicit signals");
}

void singleSelectionAndDomainTests()
{
    d26::SelectionManager manager;
    d26::SelectionPolicy singleFace = d26::SelectionPolicy::preset(d26::SelectionPolicyPreset::SurfaceScope);
    singleFace.allowMultiple = false;
    manager.setPolicy(singleFace);

    const auto face1 = face(301, 111, 4);
    const auto face2 = face(302, 111, 4);
    check(manager.apply(face1, d26::SelectionOperation::Add)
              && manager.apply(face2, d26::SelectionOperation::Add)
              && manager.items().size() == 1 && manager.items().front() == face2
              && manager.primary() == face2,
          "single-selection policy replaces previous entity on Add");

    check(manager.clear() && manager.items().isEmpty() && !manager.primary().has_value(),
          "Clear removes committed selection");
    check(!manager.clear(), "clearing already-empty selection is a no-op");

    d26::SelectionPolicy objectPolicy;
    objectPolicy.domain = d26::SelectionDomain::ProjectObject;
    objectPolicy.allowedKinds = {d26::SelectionKind::Object};
    objectPolicy.allowMultiple = false;
    manager.setPolicy(objectPolicy);

    d26::SelectionItem object;
    object.domain = d26::SelectionDomain::ProjectObject;
    object.kind = d26::SelectionKind::Object;
    object.projectObjectId = 42;
    check(manager.apply(object, d26::SelectionOperation::Replace),
          "ProjectObject domain uses its own ObjectId identity");
    check(!manager.invalidateGeometryRevision(999) && manager.items().size() == 1
              && manager.items().front() == object,
          "geometry revision invalidation never clears ProjectObject selection");
}

void scopeContractTests()
{
    d26::ScopeEntityReference entity;
    entity.domain = d26::SelectionDomain::Geometry;
    entity.kind = d26::SelectionKind::Face;
    entity.geometryEntityId = 9001;
    entity.persistentKey = QStringLiteral("step/root/1/body/1/face/3");

    d26::ScopeReference scope;
    check(scope.isEmpty(), "ScopeReference starts empty");
    scope.entities.push_back(entity);
    check(!scope.isEmpty() && scope.entities.front().persistentKey.endsWith(QStringLiteral("face/3")),
          "ScopeReference carries stable-key foundation without migrating BC schema");
}

void selectionInputContractTests()
{
    d26::ViewportSelectionController controller;

    check(d26::ViewportSelectionController::operationForModifiers(Qt::NoModifier)
              == d26::SelectionOperation::Replace,
          "plain click maps to Replace");
    check(d26::ViewportSelectionController::operationForModifiers(Qt::ShiftModifier)
              == d26::SelectionOperation::Add,
          "Shift click maps to Add");
    check(d26::ViewportSelectionController::operationForModifiers(Qt::ControlModifier)
              == d26::SelectionOperation::Toggle,
          "Qt Control semantic modifier maps macOS Command click to Toggle");
    check(d26::ViewportSelectionController::operationForModifiers(Qt::ControlModifier | Qt::ShiftModifier)
              == d26::SelectionOperation::Toggle,
          "Command toggle has deterministic priority over Shift add");

    d26::SelectionPointerInput hover;
    hover.type = d26::SelectionPointerEventType::Move;
    hover.position = QPointF(12.0, 18.0);
    const auto hoverAction = controller.routePointer(hover);
    check(hoverAction.has_value() && hoverAction->type == d26::SelectionInputActionType::Hover
              && hoverAction->position == hover.position,
          "idle pointer move emits hover/preselection request");

    d26::SelectionPointerInput press;
    press.type = d26::SelectionPointerEventType::Press;
    press.position = QPointF(20.0, 20.0);
    press.button = Qt::LeftButton;
    press.buttons = Qt::LeftButton;
    press.modifiers = Qt::ShiftModifier;
    check(!controller.routePointer(press).has_value() && controller.clickInProgress(),
          "left press starts selection click candidate without committing");

    d26::SelectionPointerInput moveWhilePressed;
    moveWhilePressed.type = d26::SelectionPointerEventType::Move;
    moveWhilePressed.position = QPointF(21.0, 20.0);
    moveWhilePressed.buttons = Qt::LeftButton;
    check(!controller.routePointer(moveWhilePressed).has_value(),
          "pressed pointer move never emits hover");

    d26::SelectionPointerInput release;
    release.type = d26::SelectionPointerEventType::Release;
    release.position = QPointF(22.0, 21.0);
    release.button = Qt::LeftButton;
    release.buttons = Qt::NoButton;
    release.modifiers = Qt::NoModifier; // press semantics must win
    const auto commit = controller.routePointer(release);
    check(commit.has_value() && commit->type == d26::SelectionInputActionType::Commit
              && commit->operation == d26::SelectionOperation::Add,
          "small Shift-left gesture commits Add using press-time modifier state");

    press.position = QPointF(30.0, 30.0);
    press.modifiers = Qt::NoModifier;
    (void)controller.routePointer(press);
    release.position = QPointF(40.0, 30.0);
    const auto draggedRelease = controller.routePointer(release);
    check(!draggedRelease.has_value() && !controller.clickInProgress(),
          "pointer travel beyond click tolerance does not commit selection");

    press.position = QPointF(50.0, 50.0);
    press.modifiers = Qt::AltModifier;
    check(!controller.routePointer(press).has_value() && !controller.clickInProgress(),
          "Option-left is reserved for navigation and never starts selection");
    release.position = QPointF(50.0, 50.0);
    check(!controller.routePointer(release).has_value(),
          "Option navigation release cannot leak into selection commit");

    const auto escape = controller.routeKey(Qt::Key_Escape, Qt::NoModifier);
    check(escape.has_value() && escape->type == d26::SelectionInputActionType::Clear
              && escape->operation == d26::SelectionOperation::Clear,
          "Escape emits clear-selection intent");
    check(!controller.routeKey(Qt::Key_Escape, Qt::ShiftModifier).has_value(),
          "modified Escape is not silently reinterpreted");

    d26::SelectionPointerInput leave;
    leave.type = d26::SelectionPointerEventType::Leave;
    const auto leaveAction = controller.routePointer(leave);
    check(leaveAction.has_value() && leaveAction->type == d26::SelectionInputActionType::ClearPreselection,
          "viewport leave clears hover/preselection only");
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    managerStateTests();
    singleSelectionAndDomainTests();
    scopeContractTests();
    selectionInputContractTests();
    std::cout << (failures == 0 ? "selection manager PASS\n" : "selection manager FAIL\n");
    return failures == 0 ? 0 : 1;
}
