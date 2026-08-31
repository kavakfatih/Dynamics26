#include "core/ScopeReferenceBuilder.h"

#include <femcae/geometry/GeometryDocument.h>

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

d26::SelectionItem geometryItem(const d26::SelectionKind kind,
                                const femcae::geometry::GeometryEntityId id,
                                const femcae::geometry::GeometryEntityId parent,
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

d26::SelectionItem body(const femcae::geometry::GeometryEntityId id, const quint64 revision)
{
    return geometryItem(d26::SelectionKind::Body, id, femcae::geometry::InvalidGeometryId, revision);
}

void scopeBuilderTests()
{
    using femcae::geometry::GeometryEntityKind;
    using femcae::geometry::InvalidGeometryId;

    femcae::geometry::GeometryDocument document("scope-reference-test");
    const auto bodyId = document.addEntity(GeometryEntityKind::Body, InvalidGeometryId,
                                           "Body 1", "step/body/1");
    const auto faceA = document.addEntity(GeometryEntityKind::Face,
                                          bodyId, "Face 1", "step/body/1/face/1");
    const auto faceB = document.addEntity(GeometryEntityKind::Face,
                                          bodyId, "Face 2", "step/body/1/face/2");
    const auto edgeA = document.addEntity(GeometryEntityKind::Edge,
                                          bodyId, "Edge 1", "step/body/1/edge/1");
    const auto vertexA = document.addEntity(GeometryEntityKind::Vertex,
                                            bodyId, "Vertex 1", "step/body/1/vertex/1");
    const quint64 revision = document.revision();

    const auto face = [bodyId, revision](const auto id) {
        return geometryItem(d26::SelectionKind::Face, id, bodyId, revision);
    };
    const auto edge = [bodyId, revision](const auto id) {
        return geometryItem(d26::SelectionKind::Edge, id, bodyId, revision);
    };
    const auto vertex = [bodyId, revision](const auto id) {
        return geometryItem(d26::SelectionKind::Vertex, id, bodyId, revision);
    };

    const auto result = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{face(faceA), face(faceB)}, document);
    check(result.success() && result.scope.entities.size() == 2,
          "two current CAD Faces convert to one persistent ScopeReference");
    check(result.scope.sourceRevision == revision,
          "persistent ScopeReference records the geometry revision it was created from");
    check(result.scope.entities[0].persistentKey == QStringLiteral("step/body/1/face/1")
              && result.scope.entities[1].persistentKey == QStringLiteral("step/body/1/face/2"),
          "scope preserves deterministic CAD persistentKey order");
    check(result.scope.entities[0].parentGeometryId == bodyId,
          "child topology scope preserves parent Body identity");
    check(d26::validateGeometryScopeReference(result.scope, document)
              == d26::ScopeReferenceValidationError::None,
          "new persistent CAD scope validates against its source revision");

    const auto bodyResult = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{body(bodyId, revision)}, document);
    check(bodyResult.success() && bodyResult.scope.entities.front().kind == d26::SelectionKind::Body,
          "current CAD Body converts to persistent Body scope");

    const auto edgeResult = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{edge(edgeA)}, document);
    check(edgeResult.success() && edgeResult.scope.entities.front().kind == d26::SelectionKind::Edge
              && edgeResult.scope.entities.front().parentGeometryId == bodyId,
          "current canonical CAD Edge converts to persistent Edge scope");

    const auto vertexResult = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{vertex(vertexA)}, document);
    check(vertexResult.success() && vertexResult.scope.entities.front().kind == d26::SelectionKind::Vertex
              && vertexResult.scope.entities.front().parentGeometryId == bodyId,
          "current canonical CAD Vertex converts to persistent Vertex scope");

    const auto stale = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{geometryItem(d26::SelectionKind::Face, faceA, bodyId, revision - 1)}, document);
    check(!stale.success()
              && stale.error == d26::ScopeReferenceBuildError::StaleGeometryRevision,
          "stale geometry revision is rejected before persistent scoping");

    const auto wrongParent = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{geometryItem(d26::SelectionKind::Edge, edgeA, bodyId + 999, revision)}, document);
    check(!wrongParent.success()
              && wrongParent.error == d26::ScopeReferenceBuildError::ParentBodyMismatch,
          "Edge/Face/Vertex selection cannot be rebound to a different parent Body");

    const auto kindMismatch = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{geometryItem(d26::SelectionKind::Edge, faceA, bodyId, revision)}, document);
    check(!kindMismatch.success()
              && kindMismatch.error == d26::ScopeReferenceBuildError::GeometryKindMismatch,
          "display or wrong-kind geometry identity cannot masquerade as a CAD Edge");

    const auto missingEntity = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{geometryItem(d26::SelectionKind::Vertex, 999999, bodyId, revision)}, document);
    check(!missingEntity.success()
              && missingEntity.error == d26::ScopeReferenceBuildError::MissingGeometryEntity,
          "unknown geometry ID is never accepted as engineering scope");

    d26::SelectionItem object;
    object.domain = d26::SelectionDomain::ProjectObject;
    object.kind = d26::SelectionKind::Object;
    object.projectObjectId = 42;
    const auto wrongDomain = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{object}, document);
    check(!wrongDomain.success()
              && wrongDomain.error == d26::ScopeReferenceBuildError::UnsupportedDomain,
          "ProjectObject identity cannot leak into CAD engineering scope");

    femcae::geometry::GeometryDocument noKey("scope-no-key");
    const auto noKeyBody = noKey.addEntity(GeometryEntityKind::Body, InvalidGeometryId,
                                           "Body", "step/body/1");
    if (auto *entity = noKey.findMutable(noKeyBody)) {
        entity->persistentKey.clear();
    }
    const auto missingKey = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{body(noKeyBody, noKey.revision())}, noKey);
    check(!missingKey.success()
              && missingKey.error == d26::ScopeReferenceBuildError::MissingPersistentKey,
          "scope persistence requires a non-empty CAD persistentKey");

    d26::ScopeReference tampered = edgeResult.scope;
    tampered.entities.front().parentGeometryId = bodyId + 77;
    check(d26::validateGeometryScopeReference(tampered, document)
              == d26::ScopeReferenceValidationError::ParentBodyMismatch,
          "persistent child-topology scope detects parent Body mismatch");

    tampered = result.scope;
    tampered.entities.front().persistentKey = QStringLiteral("step/body/1/face/wrong");
    check(d26::validateGeometryScopeReference(tampered, document)
              == d26::ScopeReferenceValidationError::PersistentKeyMismatch,
          "persistent scope detects a topology key mismatch in the same revision");

    // Persistent scope sonradan kullanılırken revision yeniden kontrol edilir.
    (void)document.addEntity(GeometryEntityKind::Edge,
                             bodyId, "Edge 2", "step/body/1/edge/2");
    check(d26::validateGeometryScopeReference(result.scope, document)
              == d26::ScopeReferenceValidationError::StaleGeometryRevision,
          "geometry revision change marks an existing persistent scope stale");

    check(!d26::buildGeometryScopeReference({}, document).success(),
          "empty transient selection cannot create a persistent scope");
    check(d26::validateGeometryScopeReference({}, document)
              == d26::ScopeReferenceValidationError::EmptyScope,
          "empty persistent scope fails validation explicitly");
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    scopeBuilderTests();
    if (failures == 0) {
        std::cout << "Scope reference tests PASSED\n";
        return 0;
    }
    std::cerr << failures << " scope reference test(s) FAILED\n";
    return 1;
}
