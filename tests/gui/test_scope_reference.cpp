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

d26::SelectionItem body(const femcae::geometry::GeometryEntityId id, const quint64 revision)
{
    d26::SelectionItem item;
    item.domain = d26::SelectionDomain::Geometry;
    item.kind = d26::SelectionKind::Body;
    item.geometryEntityId = id;
    item.sourceRevision = revision;
    return item;
}

d26::SelectionItem face(const femcae::geometry::GeometryEntityId id,
                        const femcae::geometry::GeometryEntityId parent,
                        const quint64 revision)
{
    d26::SelectionItem item;
    item.domain = d26::SelectionDomain::Geometry;
    item.kind = d26::SelectionKind::Face;
    item.geometryEntityId = id;
    item.parentGeometryId = parent;
    item.sourceRevision = revision;
    return item;
}

void scopeBuilderTests()
{
    femcae::geometry::GeometryDocument document("scope-reference-test");
    const auto bodyId = document.addEntity(femcae::geometry::GeometryEntityKind::Body,
                                           femcae::geometry::InvalidGeometryId,
                                           "Body 1", "step/body/1");
    const auto faceA = document.addEntity(femcae::geometry::GeometryEntityKind::Face,
                                          bodyId, "Face 1", "step/body/1/face/1");
    const auto faceB = document.addEntity(femcae::geometry::GeometryEntityKind::Face,
                                          bodyId, "Face 2", "step/body/1/face/2");
    const quint64 revision = document.revision();

    const auto result = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{face(faceA, bodyId, revision), face(faceB, bodyId, revision)},
        document);
    check(result.success() && result.scope.entities.size() == 2,
          "two current CAD Faces convert to one persistent ScopeReference");
    check(result.scope.sourceRevision == revision,
          "persistent ScopeReference records the geometry revision it was created from");
    check(result.scope.entities[0].persistentKey == QStringLiteral("step/body/1/face/1")
              && result.scope.entities[1].persistentKey == QStringLiteral("step/body/1/face/2"),
          "scope preserves deterministic CAD persistentKey order");
    check(d26::validateGeometryScopeReference(result.scope, document)
              == d26::ScopeReferenceValidationError::None,
          "new persistent CAD scope validates against its source revision");

    const auto bodyResult = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{body(bodyId, revision)}, document);
    check(bodyResult.success() && bodyResult.scope.entities.front().kind == d26::SelectionKind::Body,
          "current CAD Body converts to persistent Body scope");

    const auto stale = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{face(faceA, bodyId, revision - 1)}, document);
    check(!stale.success()
              && stale.error == d26::ScopeReferenceBuildError::StaleGeometryRevision,
          "stale geometry revision is rejected before persistent scoping");

    const auto wrongParent = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{face(faceA, bodyId + 999, revision)}, document);
    check(!wrongParent.success()
              && wrongParent.error == d26::ScopeReferenceBuildError::ParentBodyMismatch,
          "Face selection cannot be rebound to a different parent Body");

    d26::SelectionItem missing = face(999999, bodyId, revision);
    const auto missingEntity = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{missing}, document);
    check(!missingEntity.success()
              && missingEntity.error == d26::ScopeReferenceBuildError::MissingGeometryEntity,
          "unknown geometry ID is never accepted as engineering scope");

    d26::SelectionItem edge;
    edge.domain = d26::SelectionDomain::Geometry;
    edge.kind = d26::SelectionKind::Edge;
    edge.geometryEntityId = faceA;
    edge.sourceRevision = revision;
    const auto unsupportedKind = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{edge}, document);
    check(!unsupportedKind.success()
              && unsupportedKind.error == d26::ScopeReferenceBuildError::UnsupportedKind,
          "Alpha.3.2 scope builder refuses Edge/Vertex until their selection phase exists");

    d26::SelectionItem object;
    object.domain = d26::SelectionDomain::ProjectObject;
    object.kind = d26::SelectionKind::Object;
    object.projectObjectId = 42;
    const auto wrongDomain = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{object}, document);
    check(!wrongDomain.success()
              && wrongDomain.error == d26::ScopeReferenceBuildError::UnsupportedDomain,
          "ProjectObject identity cannot leak into CAD engineering scope");

    // GeometryDocument public API boş persistentKey üretimini zaten reddeder.
    // Builder savunmasını ayrıca doğrulamak için geçerli entity kontrollü olarak
    // bozulur; findMutable revision'i artırdığı için selection yeni revision ile kurulur.
    femcae::geometry::GeometryDocument noKey("scope-no-key");
    const auto noKeyBody = noKey.addEntity(femcae::geometry::GeometryEntityKind::Body,
                                           femcae::geometry::InvalidGeometryId,
                                           "Body", "step/body/1");
    if (auto *entity = noKey.findMutable(noKeyBody)) {
        entity->persistentKey.clear();
    }
    const auto missingKey = d26::buildGeometryScopeReference(
        QVector<d26::SelectionItem>{body(noKeyBody, noKey.revision())}, noKey);
    check(!missingKey.success()
              && missingKey.error == d26::ScopeReferenceBuildError::MissingPersistentKey,
          "scope persistence requires a non-empty CAD persistentKey");

    d26::ScopeReference tampered = result.scope;
    tampered.entities.front().persistentKey = QStringLiteral("step/body/1/face/wrong");
    check(d26::validateGeometryScopeReference(tampered, document)
              == d26::ScopeReferenceValidationError::PersistentKeyMismatch,
          "persistent scope detects a topology key mismatch in the same revision");

    // Persistent scope sonradan kullanılırken revision yeniden kontrol edilir.
    // Yeni entity eklemek dahi document revision'ini değiştirir; eski scope
    // otomatik rebind edilmez ve açıkça stale sayılır.
    (void)document.addEntity(femcae::geometry::GeometryEntityKind::Edge,
                             bodyId, "Edge 1", "step/body/1/edge/1");
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
