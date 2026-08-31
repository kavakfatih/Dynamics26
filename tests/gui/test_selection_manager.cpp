#include "core/SelectionManager.h"
#include "viewport/GeometrySelectionScene.h"
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

d26::SelectionItem topologyChild(const d26::SelectionKind kind,
                                 const quint64 id,
                                 const quint64 parent,
                                 const quint64 revision)
{
    d26::SelectionItem item;
    item.domain = d26::SelectionDomain::Geometry;
    item.kind = kind;
    item.geometryEntityId = id;
    item.parentGeometryId = parent;
    item.sourceRevision = revision;
    return item;
}

d26::SelectionItem face(const quint64 id, const quint64 parent, const quint64 revision)
{
    return topologyChild(d26::SelectionKind::Face, id, parent, revision);
}

d26::SelectionItem edge(const quint64 id, const quint64 parent, const quint64 revision)
{
    return topologyChild(d26::SelectionKind::Edge, id, parent, revision);
}

d26::SelectionItem vertex(const quint64 id, const quint64 parent, const quint64 revision)
{
    return topologyChild(d26::SelectionKind::Vertex, id, parent, revision);
}

femcae::geometry::TopologyTessellation topologyBody(const quint64 bodyId, const quint64 revision,
                                                     const quint64 faceA, const quint64 faceB,
                                                     const double xOffset)
{
    femcae::geometry::TopologyTessellation topology;
    topology.display.sourceGeometryId = bodyId;
    topology.display.sourceRevision = revision;
    topology.display.points = {{xOffset + 0.0, 0.0, 0.0}, {xOffset + 1.0, 0.0, 0.0},
                               {xOffset + 1.0, 1.0, 0.0}, {xOffset + 0.0, 1.0, 0.0}};
    topology.display.triangles = {{{0, 1, 2}}, {{0, 2, 3}}};
    topology.triangleFaceIds = {faceA, faceB};
    return topology;
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
    const auto edge1 = edge(301, 101, 7);
    const auto vertex1 = vertex(401, 101, 7);

    check(manager.policy().accepts(body1) && manager.policy().accepts(face1)
              && manager.policy().accepts(edge1) && manager.policy().accepts(vertex1),
          "neutral geometry policy accepts Body Face Edge and Vertex");
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

void edgeVertexStateTests()
{
    d26::SelectionManager manager;
    constexpr quint64 bodyId = 501;
    constexpr quint64 revision = 12;
    const auto edge1 = edge(5101, bodyId, revision);
    const auto edge2 = edge(5102, bodyId, revision);
    const auto edge3 = edge(5103, bodyId, revision);
    const auto vertex1 = vertex(5201, bodyId, revision);
    const auto vertex2 = vertex(5202, bodyId, revision);
    const auto vertex3 = vertex(5203, bodyId, revision);

    manager.setPolicy(d26::SelectionPolicy::preset(d26::SelectionPolicyPreset::EdgeScope));
    check(manager.apply(edge1, d26::SelectionOperation::Replace)
              && manager.apply(edge2, d26::SelectionOperation::Add)
              && manager.items().size() == 2 && manager.primary() == edge2,
          "multi-Edge selection supports Replace plus Add with stable primary");
    check(manager.apply(edge3, d26::SelectionOperation::Toggle)
              && manager.items().size() == 3 && manager.primary() == edge3,
          "Edge Toggle adds a missing canonical Edge");
    check(manager.apply(edge3, d26::SelectionOperation::Toggle)
              && manager.items().size() == 2 && manager.primary() == edge2,
          "Edge Toggle removes the current primary deterministically");
    check(manager.setPreselection(edge3) && manager.preselection() == edge3
              && manager.items().size() == 2,
          "Edge hover/preselection remains separate from committed multi-selection");
    check(!manager.apply(vertex1, d26::SelectionOperation::Add),
          "EdgeScope rejects Vertex identity");

    manager.setPolicy(d26::SelectionPolicy::preset(d26::SelectionPolicyPreset::VertexScope));
    check(manager.items().isEmpty() && !manager.preselection().has_value(),
          "switching EdgeScope to VertexScope removes incompatible Edge state");
    check(manager.apply(vertex1, d26::SelectionOperation::Replace)
              && manager.apply(vertex2, d26::SelectionOperation::Add)
              && manager.items().size() == 2 && manager.primary() == vertex2,
          "multi-Vertex selection supports Replace plus Add with stable primary");
    check(manager.apply(vertex3, d26::SelectionOperation::Toggle)
              && manager.items().size() == 3 && manager.primary() == vertex3,
          "Vertex Toggle adds a missing canonical Vertex");
    check(manager.apply(vertex3, d26::SelectionOperation::Toggle)
              && manager.items().size() == 2 && manager.primary() == vertex2,
          "Vertex Toggle removes the current primary deterministically");
    check(manager.setPreselection(vertex3) && manager.preselection() == vertex3
              && manager.items().size() == 2,
          "Vertex hover/preselection remains separate from committed multi-selection");
    check(manager.invalidateGeometryRevision(revision + 1)
              && manager.items().isEmpty() && !manager.preselection().has_value(),
          "geometry revision invalidates stale Edge/Vertex committed and hover state");
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
    release.modifiers = Qt::NoModifier;
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

void geometrySelectionSceneTests()
{
    d26::GeometrySelectionScene scene;
    const auto body1 = topologyBody(1001, 44, 1101, 1102, 0.0);
    const auto body2 = topologyBody(2001, 44, 2101, 2102, 10.0);

    check(scene.append(body1) && scene.append(body2),
          "two CAD Bodies with one revision aggregate into one display scene");
    check(scene.sourceRevision() == 44 && scene.points().size() == 8
              && scene.triangles().size() == 4 && scene.provenance().size() == 4,
          "multi-body scene preserves aggregate geometry and one provenance record per cell");
    check(scene.triangles()[2][0] == 4 && scene.triangles()[2][1] == 5 && scene.triangles()[2][2] == 6,
          "second Body triangle indices are offset into aggregate point storage");
    check(scene.hasFaceProvenance(), "aggregate scene reports complete CAD Face provenance");

    const auto firstBodyCell = scene.provenanceForCell(0);
    const auto secondBodyCell = scene.provenanceForCell(3);
    check(firstBodyCell.has_value() && firstBodyCell->bodyId == 1001 && firstBodyCell->faceId == 1101,
          "cell 0 resolves to Body 1 / Face 1 provenance");
    check(secondBodyCell.has_value() && secondBodyCell->bodyId == 2001 && secondBodyCell->faceId == 2102,
          "cell 3 resolves to Body 2 / Face 2 provenance");

    const auto bodySelection = scene.selectionItemForCell(2, d26::SelectionKind::Body);
    const auto faceSelection = scene.selectionItemForCell(2, d26::SelectionKind::Face);
    check(bodySelection.has_value() && bodySelection->geometryEntityId == 2001
              && bodySelection->sourceRevision == 44,
          "Body filter resolves picked cell to parent CAD Body selection");
    check(faceSelection.has_value() && faceSelection->geometryEntityId == 2101
              && faceSelection->parentGeometryId == 2001 && faceSelection->sourceRevision == 44,
          "Face filter resolves picked cell to CAD Face with parent Body");
    check(!scene.selectionItemForCell(2, d26::SelectionKind::Edge).has_value(),
          "surface scene refuses Edge identity because CAD Edge uses a dedicated line scene");
    check(!scene.provenanceForCell(99).has_value(), "out-of-range display cell is never a valid CAD hit");

    const auto oldPointCount = scene.points().size();
    const auto oldCellCount = scene.triangles().size();
    const auto staleBody = topologyBody(3001, 45, 3101, 3102, 20.0);
    check(!scene.append(staleBody) && scene.points().size() == oldPointCount
              && scene.triangles().size() == oldCellCount && scene.sourceRevision() == 44,
          "mixed CAD revisions are rejected without mutating the existing scene");

    auto broken = topologyBody(4001, 44, 4101, 4102, 30.0);
    broken.display.triangles[1] = {0, 2, 99};
    check(!scene.append(broken) && scene.points().size() == oldPointCount
              && scene.triangles().size() == oldCellCount,
          "invalid display triangle append rolls back atomically");

    scene.clear();
    check(scene.empty() && scene.sourceRevision() == 0 && scene.points().empty()
              && scene.provenance().empty(),
          "clearing display scene also clears provenance and revision state");
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    managerStateTests();
    edgeVertexStateTests();
    singleSelectionAndDomainTests();
    scopeContractTests();
    selectionInputContractTests();
    geometrySelectionSceneTests();
    std::cout << (failures == 0 ? "selection manager PASS\n" : "selection manager FAIL\n");
    return failures == 0 ? 0 : 1;
}
