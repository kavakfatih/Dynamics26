#include "core/SelectionManager.h"

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

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    managerStateTests();
    singleSelectionAndDomainTests();
    scopeContractTests();
    std::cout << (failures == 0 ? "selection manager PASS\n" : "selection manager FAIL\n");
    return failures == 0 ? 0 : 1;
}
