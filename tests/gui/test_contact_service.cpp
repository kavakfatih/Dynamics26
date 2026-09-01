#include "services/ContactService.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>

#include <iostream>
#include <string>

namespace {

int failures = 0;

void check(const bool condition, const std::string &message)
{
    std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
    failures += condition ? 0 : 1;
}

d26::ScopeReference meshFacetScope(const quint64 generation,
                                   const femcae::meshing::MeshEntityId facetId)
{
    d26::ScopeEntityReference reference;
    reference.domain = d26::SelectionDomain::Mesh;
    reference.kind = d26::SelectionKind::Facet;
    reference.meshEntityId = facetId;

    d26::ScopeReference scope;
    scope.sourceRevision = generation;
    scope.entities.push_back(reference);
    return scope;
}

d26::ScopeReference geometryFaceScope(const quint64 revision,
                                      const femcae::geometry::GeometryEntityId faceId,
                                      const femcae::geometry::GeometryEntityId bodyId,
                                      const QString &persistentKey)
{
    d26::ScopeEntityReference reference;
    reference.domain = d26::SelectionDomain::Geometry;
    reference.kind = d26::SelectionKind::Face;
    reference.geometryEntityId = faceId;
    reference.parentGeometryId = bodyId;
    reference.persistentKey = persistentKey;

    d26::ScopeReference scope;
    scope.sourceRevision = revision;
    scope.entities.push_back(reference);
    return scope;
}

void serviceContractTests()
{
    d26::ProjectModel project;
    d26::GeometryService geometry;
    d26::MeshService mesh(&geometry);
    d26::ContactService contacts(&project, &geometry, &mesh);

    mesh.setDivisions(1, 1, 1);
    check(mesh.generate(), "contact fixture generates current FEM mesh");
    check(mesh.generation() != 0 && mesh.mesh().boundaryFacets.size() >= 2,
          "generated mesh exposes at least two real boundary Facet identities");
    if (mesh.mesh().boundaryFacets.size() < 2) {
        return;
    }

    const quint64 generation = mesh.generation();
    const auto sourceFacet = mesh.mesh().boundaryFacets.at(0).id;
    const auto targetFacet = mesh.mesh().boundaryFacets.at(1).id;

    d26::ContactDefinition firstDefinition;
    firstDefinition.name = QStringLiteral("Contact Region");
    firstDefinition.sourceScope = meshFacetScope(generation, sourceFacet);
    firstDefinition.targetScope = meshFacetScope(generation, targetFacet);
    firstDefinition.formulation = d26::ContactFormulation::Bonded;

    const d26::ObjectId firstId = contacts.createContact(firstDefinition);
    check(firstId != d26::InvalidObjectId,
          "current FEM Facet pair creates persistent Contact engineering object");
    check(contacts.count() == 1 && contacts.rowOf(firstId) == 0,
          "ContactService owns deterministic contact definition order");
    check(project.typeOf(firstId) == d26::ObjectType::ContactRegion
              && project.parentOf(firstId) == project.connectionsNode(),
          "ProjectModel stores only ContactRegion tree identity under Connections");
    check(contacts.validate(firstId).valid()
              && project.object(firstId) != nullptr
              && project.object(firstId)->state == d26::ObjectState::Ready,
          "fresh surface pair validates against current Mesh generation");

    const d26::ObjectId secondId = contacts.createContact(firstDefinition);
    check(secondId != d26::InvalidObjectId && secondId != firstId
              && contacts.byId(firstId)->name == QStringLiteral("Contact Region")
              && contacts.byId(secondId)->name == QStringLiteral("Contact Region 2"),
          "duplicate contact names are made unique without changing ObjectId identity");

    contacts.rename(secondId, QStringLiteral("Bonded Interface"));
    check(contacts.byId(secondId)->name == QStringLiteral("Bonded Interface")
              && project.object(secondId)->name == QStringLiteral("Bonded Interface"),
          "rename keeps ContactService definition and ProjectModel name synchronized");

    const d26::ScopeReference originalTarget = contacts.byId(secondId)->targetScope;
    check(!contacts.replaceTargetScope(secondId, {})
              && contacts.byId(secondId)->targetScope.sourceRevision == originalTarget.sourceRevision,
          "structurally empty replacement surface is rejected without state mutation");

    d26::ContactDefinition identicalDefinition = firstDefinition;
    identicalDefinition.name = QStringLiteral("Invalid Same Surface");
    identicalDefinition.targetScope = identicalDefinition.sourceScope;
    const d26::ObjectId identicalId = contacts.createContact(identicalDefinition);
    const d26::ContactValidationResult identicalValidation = contacts.validate(identicalId);
    check(identicalId != d26::InvalidObjectId
              && identicalValidation.error == d26::ContactValidationError::IdenticalSourceAndTarget
              && project.object(identicalId)->state == d26::ObjectState::Error,
          "same engineering surface on Source and Target is retained but diagnosed as Error");

    d26::ContactDefinition crossDomainDefinition = firstDefinition;
    crossDomainDefinition.name = QStringLiteral("Invalid Cross Domain");
    crossDomainDefinition.sourceScope = geometryFaceScope(
        geometry.document().revision() + 1, 7001, 7000, QStringLiteral("cad/body/1/face/1"));
    const d26::ObjectId crossDomainId = contacts.createContact(crossDomainDefinition);
    check(crossDomainId != d26::InvalidObjectId
              && contacts.validate(crossDomainId).error == d26::ContactValidationError::MixedDomains
              && project.object(crossDomainId)->state == d26::ObjectState::Error,
          "Geometry Face and FEM Facet cannot be mixed across one contact pair");

    contacts.setSuppressed(firstId, true);
    check(project.isSuppressed(firstId)
              && project.object(firstId)->state == d26::ObjectState::Suppressed,
          "contact suppression reuses canonical ProjectModel document state");
    contacts.setSuppressed(firstId, false);
    check(!project.isSuppressed(firstId) && contacts.validate(firstId).valid()
              && project.object(firstId)->state == d26::ObjectState::Ready,
          "unsuppress restores current engineering validation state");

    const quint64 oldGeneration = mesh.generation();
    check(mesh.generate() && mesh.generation() != oldGeneration,
          "mesh regeneration advances contact scope lifecycle generation");
    const d26::ContactValidationResult staleValidation = contacts.validate(firstId);
    check(staleValidation.error == d26::ContactValidationError::SourceScopeInvalid
              && staleValidation.sourceScopeError == d26::ScopeReferenceValidationError::StaleMeshGeneration,
          "old contact Facet identity never silently rebinds after mesh regenerate");
    check(project.object(firstId)->state == d26::ObjectState::OutOfDate,
          "MeshService changed signal refreshes stale ContactRegion state to OutOfDate");

    check(contacts.remove(secondId) && contacts.byId(secondId) == nullptr
              && project.object(secondId) == nullptr,
          "contact delete removes engineering definition and matching tree identity");
}

void exactPersistenceTests()
{
    d26::ProjectModel project;
    d26::GeometryService geometry;
    d26::MeshService mesh(&geometry);
    d26::ContactService contacts(&project, &geometry, &mesh);

    constexpr quint64 hugeObjectId = 9007199254741999ULL; // > 2^53
    constexpr quint64 hugeGeneration = 9007199254742111ULL;
    constexpr femcae::meshing::MeshEntityId hugeSourceFacet =
        static_cast<femcae::meshing::MeshEntityId>(9007199254743001ULL);
    constexpr femcae::meshing::MeshEntityId hugeTargetFacet =
        static_cast<femcae::meshing::MeshEntityId>(9007199254743002ULL);

    d26::ContactDefinition definition;
    definition.name = QStringLiteral("Huge Identity Contact");
    definition.sourceScope = meshFacetScope(hugeGeneration, hugeSourceFacet);
    definition.targetScope = meshFacetScope(hugeGeneration, hugeTargetFacet);

    const d26::ObjectId id = contacts.createContact(definition, -1, hugeObjectId);
    check(id == hugeObjectId, "requested Contact ObjectId above IEEE-754 exact range is restored exactly");
    check(contacts.validate(id).sourceScopeError == d26::ScopeReferenceValidationError::StaleMeshGeneration,
          "huge persisted mesh scope remains stale instead of rebinding to current generation");

    const QJsonObject saved = contacts.toJson();
    const QJsonArray items = saved.value(QStringLiteral("items")).toArray();
    check(items.size() == 1 && items.at(0).toObject().value(QStringLiteral("object_id")).isString(),
          "Contact ObjectId is serialized as decimal string, never JSON double");

    const QJsonObject savedItem = items.at(0).toObject();
    const QJsonObject sourceJson = savedItem.value(QStringLiteral("source_scope")).toObject();
    const QJsonObject targetJson = savedItem.value(QStringLiteral("target_scope")).toObject();
    check(sourceJson.value(QStringLiteral("source_revision")).toString()
              == QString::number(hugeGeneration)
              && sourceJson.value(QStringLiteral("entities")).toArray().at(0).toObject()
                     .value(QStringLiteral("mesh_entity_id")).toString()
                     == QString::number(static_cast<qulonglong>(hugeSourceFacet))
              && targetJson.value(QStringLiteral("entities")).toArray().at(0).toObject()
                     .value(QStringLiteral("mesh_entity_id")).toString()
                     == QString::number(static_cast<qulonglong>(hugeTargetFacet)),
          "Contact revision and MeshEntityId values above 2^53 serialize exactly as strings");

    d26::ProjectModel restoredProject;
    d26::GeometryService restoredGeometry;
    d26::MeshService restoredMesh(&restoredGeometry);
    d26::ContactService restored(&restoredProject, &restoredGeometry, &restoredMesh);
    QString error;
    check(restored.fromJson(saved, &error),
          "Contact JSON round-trip restores structurally valid persistent engineering scopes");
    const d26::ContactDefinition *restoredDefinition = restored.byId(hugeObjectId);
    check(restoredDefinition != nullptr
              && restoredDefinition->sourceScope.sourceRevision == hugeGeneration
              && restoredDefinition->sourceScope.entities.front().meshEntityId == hugeSourceFacet
              && restoredDefinition->targetScope.entities.front().meshEntityId == hugeTargetFacet,
          "Contact JSON round-trip preserves ObjectId, generation and MeshEntityId exactly");

    QJsonObject overflow = saved;
    QJsonArray overflowItems = overflow.value(QStringLiteral("items")).toArray();
    QJsonObject overflowItem = overflowItems.at(0).toObject();
    overflowItem[QStringLiteral("object_id")] = QStringLiteral("18446744073709551616");
    overflowItems[0] = overflowItem;
    overflow[QStringLiteral("items")] = overflowItems;

    const int countBeforeFailure = restored.count();
    error.clear();
    check(!restored.fromJson(overflow, &error) && !error.isEmpty()
              && restored.count() == countBeforeFailure
              && restored.byId(hugeObjectId) != nullptr,
          "uint64 overflow is rejected before existing ContactService state is mutated");
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    serviceContractTests();
    exactPersistenceTests();

    std::cout << "Contact service contract: " << (failures == 0 ? "PASS" : "FAIL") << '\n';
    return failures == 0 ? 0 : 1;
}
