#include "services/NamedSelectionService.h"

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

d26::SelectionItem meshItem(const d26::SelectionKind kind,
                            const femcae::meshing::MeshEntityId id,
                            const quint64 generation)
{
    d26::SelectionItem item;
    item.domain = d26::SelectionDomain::Mesh;
    item.kind = kind;
    item.meshEntityId = id;
    item.sourceRevision = generation;
    return item;
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

d26::ScopeReference meshScope(const quint64 generation,
                              const d26::SelectionKind kind,
                              const femcae::meshing::MeshEntityId entityId)
{
    d26::ScopeEntityReference reference;
    reference.domain = d26::SelectionDomain::Mesh;
    reference.kind = kind;
    reference.meshEntityId = entityId;

    d26::ScopeReference scope;
    scope.sourceRevision = generation;
    scope.entities.push_back(reference);
    return scope;
}

void serviceContractTests()
{
    d26::ProjectModel project;
    d26::GeometryService geometry;
    d26::MeshService mesh(&geometry);
    d26::NamedSelectionService service(&project, &geometry, &mesh);

    mesh.setDivisions(1, 1, 1);
    check(mesh.generate(), "baseline FEM mesh can be generated for Named Selection tests");
    check(mesh.generation() != 0 && !mesh.mesh().nodes.empty() && !mesh.mesh().elements.empty(),
          "generated mesh exposes engineering Node/Element identities and generation");

    const auto nodeId = mesh.mesh().nodes.front().id;
    const auto nodeSelection = meshItem(d26::SelectionKind::Node, nodeId, mesh.generation());
    const auto first = service.createFromSelection({nodeSelection}, QStringLiteral("Mesh Scope"));
    check(first.success(), "current FEM Node selection creates a persistent Named Selection");
    check(service.count() == 1 && service.rowOf(first.id) == 0,
          "service owns deterministic Named Selection order");
    check(project.typeOf(first.id) == d26::ObjectType::NamedSelection
              && project.parentOf(first.id) == project.namedSelectionsNode(),
          "ProjectModel stores only Named Selection tree identity under canonical folder");
    check(service.validate(first.id) == d26::ScopeReferenceValidationError::None,
          "fresh FEM Named Selection validates against current mesh generation");
    check(project.object(first.id) != nullptr
              && project.object(first.id)->state == d26::ObjectState::UpToDate,
          "valid persistent scope is reflected as UpToDate ProjectObject state");

    const auto second = service.createFromSelection({nodeSelection}, QStringLiteral("Mesh Scope"));
    check(second.success() && second.id != first.id,
          "same engineering scope can be stored as a separate Named Selection object");
    check(service.byId(first.id)->name == QStringLiteral("Mesh Scope")
              && service.byId(second.id)->name == QStringLiteral("Mesh Scope 2"),
          "duplicate requested names are made unique deterministically");

    service.rename(second.id, QStringLiteral("Updated Scope"));
    check(service.byId(second.id)->name == QStringLiteral("Updated Scope")
              && project.object(second.id)->name == QStringLiteral("Updated Scope"),
          "rename keeps service definition and ProjectModel identity synchronized");

    const auto elementId = mesh.mesh().elements.front().id;
    const d26::ScopeReference elementScope = meshScope(
        mesh.generation(), d26::SelectionKind::Element, elementId);
    service.replaceScope(second.id, elementScope);
    check(service.byId(second.id)->scope.entities.front().kind == d26::SelectionKind::Element
              && service.validate(second.id) == d26::ScopeReferenceValidationError::None,
          "scope update replaces persistent engineering identity without touching display IDs");

    const d26::ScopeReference beforeInvalidReplace = service.byId(second.id)->scope;
    service.replaceScope(second.id, {});
    check(service.byId(second.id)->scope.sourceRevision == beforeInvalidReplace.sourceRevision
              && service.byId(second.id)->scope.entities.size() == beforeInvalidReplace.entities.size(),
          "structurally invalid replacement scope is rejected without mutating stored scope");

    check(service.remove(second.id) && service.byId(second.id) == nullptr
              && project.object(second.id) == nullptr,
          "delete removes service data and matching ProjectObject identity");

    const quint64 previousGeneration = mesh.generation();
    check(mesh.generate() && mesh.generation() != previousGeneration,
          "mesh regenerate advances lifecycle generation");
    check(service.validate(first.id) == d26::ScopeReferenceValidationError::StaleMeshGeneration,
          "old FEM scope never silently rebinds when numeric MeshEntityId is reused");
    check(project.object(first.id)->state == d26::ObjectState::OutOfDate,
          "mesh changed signal automatically refreshes stale Named Selection state to OutOfDate");

    d26::NamedSelectionDefinition geometryDefinition;
    geometryDefinition.name = QStringLiteral("CAD Faces");
    geometryDefinition.scope = geometryFaceScope(
        geometry.document().revision() + 77, 7001, 7000, QStringLiteral("cad/body/1/face/1"));
    const d26::ObjectId geometryId = service.createWithScope(geometryDefinition);
    check(geometryId != d26::InvalidObjectId,
          "persistent Geometry Scope can be restored even when source CAD revision is stale");
    check(service.validate(geometryId) == d26::ScopeReferenceValidationError::StaleGeometryRevision
              && project.object(geometryId)->state == d26::ObjectState::OutOfDate,
          "stale CAD revision is retained for diagnostics and never treated as valid");

    d26::NamedSelectionDefinition emptyDefinition;
    emptyDefinition.name = QStringLiteral("Invalid");
    check(service.createWithScope(emptyDefinition) == d26::InvalidObjectId,
          "empty persistent scope is rejected");

    d26::ScopeReference mixedScope = geometryDefinition.scope;
    d26::ScopeEntityReference meshReference;
    meshReference.domain = d26::SelectionDomain::Mesh;
    meshReference.kind = d26::SelectionKind::Node;
    meshReference.meshEntityId = nodeId;
    mixedScope.entities.push_back(meshReference);
    emptyDefinition.scope = mixedScope;
    check(service.createWithScope(emptyDefinition) == d26::InvalidObjectId,
          "Geometry and Mesh identities cannot be mixed inside one Named Selection");

    d26::ScopeReference mixedKindScope = geometryDefinition.scope;
    d26::ScopeEntityReference edgeReference = geometryDefinition.scope.entities.front();
    edgeReference.kind = d26::SelectionKind::Edge;
    edgeReference.geometryEntityId = 7002;
    edgeReference.persistentKey = QStringLiteral("cad/body/1/edge/1");
    mixedKindScope.entities.push_back(edgeReference);
    emptyDefinition.scope = mixedKindScope;
    check(service.createWithScope(emptyDefinition) == d26::InvalidObjectId,
          "one Geometry Named Selection cannot mix Face and Edge engineering kinds");

    d26::ScopeReference mixedMeshKindScope = meshScope(
        mesh.generation(), d26::SelectionKind::Node, mesh.mesh().nodes.front().id);
    d26::ScopeEntityReference elementReference;
    elementReference.domain = d26::SelectionDomain::Mesh;
    elementReference.kind = d26::SelectionKind::Element;
    elementReference.meshEntityId = mesh.mesh().elements.front().id;
    mixedMeshKindScope.entities.push_back(elementReference);
    emptyDefinition.scope = mixedMeshKindScope;
    check(service.createWithScope(emptyDefinition) == d26::InvalidObjectId,
          "one Mesh Named Selection cannot mix Node and Element engineering kinds");

    d26::SelectionItem projectObjectItem;
    projectObjectItem.domain = d26::SelectionDomain::ProjectObject;
    projectObjectItem.kind = d26::SelectionKind::Object;
    projectObjectItem.projectObjectId = project.modelNode();
    const auto wrongDomain = service.createFromSelection({projectObjectItem});
    check(!wrongDomain.success()
              && wrongDomain.buildError == d26::ScopeReferenceBuildError::UnsupportedDomain,
          "ProjectObject transient identity cannot leak into engineering Named Selection scope");
}

void persistenceTests()
{
    constexpr quint64 hugeObjectId = 9007199254740993ULL;      // 2^53 + 1
    constexpr quint64 hugeGeometryId = 9007199254740995ULL;
    constexpr quint64 hugeParentId = 9007199254740997ULL;
    constexpr quint64 hugeRevision = 9007199254740999ULL;
    constexpr quint64 hugeMeshObjectId = 9007199254741001ULL;
    constexpr quint64 hugeMeshEntityId = 9007199254741003ULL;
    constexpr quint64 hugeMeshGeneration = 9007199254741005ULL;

    d26::ProjectModel sourceProject;
    d26::GeometryService sourceGeometry;
    d26::MeshService sourceMesh(&sourceGeometry);
    d26::NamedSelectionService source(&sourceProject, &sourceGeometry, &sourceMesh);

    d26::NamedSelectionDefinition definition;
    definition.name = QStringLiteral("64-bit CAD Scope");
    definition.scope = geometryFaceScope(
        hugeRevision,
        static_cast<femcae::geometry::GeometryEntityId>(hugeGeometryId),
        static_cast<femcae::geometry::GeometryEntityId>(hugeParentId),
        QStringLiteral("cad/huge/body/face"));

    const d26::ObjectId created = source.createWithScope(definition, -1, hugeObjectId);
    check(created == hugeObjectId,
          "requested ObjectId is restored exactly instead of allocating a replacement identity");

    d26::NamedSelectionDefinition meshDefinition;
    meshDefinition.name = QStringLiteral("64-bit FEM Scope");
    meshDefinition.scope = meshScope(
        hugeMeshGeneration,
        d26::SelectionKind::Facet,
        static_cast<femcae::meshing::MeshEntityId>(hugeMeshEntityId));
    const d26::ObjectId meshCreated = source.createWithScope(
        meshDefinition, -1, hugeMeshObjectId);
    check(meshCreated == hugeMeshObjectId,
          "requested ObjectId restore also preserves a large FEM scope identity");

    const QJsonObject json = source.toJson();
    const QJsonArray items = json.value(QStringLiteral("items")).toArray();
    const QJsonObject item = items.at(0).toObject();
    const QJsonObject entity = item.value(QStringLiteral("entities")).toArray().at(0).toObject();
    const QJsonObject meshItemJson = items.at(1).toObject();
    const QJsonObject meshEntityJson = meshItemJson.value(QStringLiteral("entities")).toArray().at(0).toObject();
    check(item.value(QStringLiteral("object_id")).isString()
              && item.value(QStringLiteral("object_id")).toString() == QString::number(hugeObjectId),
          "ObjectId above IEEE-754 exact integer range is serialized as decimal string");
    check(item.value(QStringLiteral("source_revision")).isString()
              && item.value(QStringLiteral("source_revision")).toString() == QString::number(hugeRevision),
          "geometry revision lifecycle guard is serialized as 64-bit string");
    check(entity.value(QStringLiteral("geometry_entity_id")).isString()
              && entity.value(QStringLiteral("geometry_entity_id")).toString() == QString::number(hugeGeometryId)
              && entity.value(QStringLiteral("parent_geometry_id")).toString() == QString::number(hugeParentId),
          "GeometryEntityId and parent identity preserve full 64-bit precision in JSON");
    check(meshItemJson.value(QStringLiteral("object_id")).toString() == QString::number(hugeMeshObjectId)
              && meshItemJson.value(QStringLiteral("source_revision")).toString() == QString::number(hugeMeshGeneration)
              && meshEntityJson.value(QStringLiteral("mesh_entity_id")).isString()
              && meshEntityJson.value(QStringLiteral("mesh_entity_id")).toString() == QString::number(hugeMeshEntityId),
          "MeshEntityId and mesh generation above 2^53 are serialized as exact decimal strings");

    d26::ProjectModel restoredProject;
    d26::GeometryService restoredGeometry;
    d26::MeshService restoredMesh(&restoredGeometry);
    d26::NamedSelectionService restored(&restoredProject, &restoredGeometry, &restoredMesh);
    QString error;
    check(restored.fromJson(json, &error) && error.isEmpty(),
          "Named Selection JSON round-trip loads without parse diagnostics");

    const d26::NamedSelectionDefinition *roundTrip = restored.byId(hugeObjectId);
    check(roundTrip != nullptr
              && roundTrip->scope.sourceRevision == hugeRevision
              && static_cast<quint64>(roundTrip->scope.entities.front().geometryEntityId) == hugeGeometryId
              && static_cast<quint64>(roundTrip->scope.entities.front().parentGeometryId) == hugeParentId,
          "64-bit CAD engineering identities survive JSON round-trip exactly");
    const d26::NamedSelectionDefinition *meshRoundTrip = restored.byId(hugeMeshObjectId);
    check(meshRoundTrip != nullptr
              && meshRoundTrip->scope.sourceRevision == hugeMeshGeneration
              && static_cast<quint64>(meshRoundTrip->scope.entities.front().meshEntityId) == hugeMeshEntityId,
          "64-bit FEM engineering identity and generation survive JSON round-trip exactly");
    check(restoredProject.object(hugeObjectId) != nullptr
              && restoredProject.typeOf(hugeObjectId) == d26::ObjectType::NamedSelection
              && restoredProject.object(hugeMeshObjectId) != nullptr
              && restoredProject.typeOf(hugeMeshObjectId) == d26::ObjectType::NamedSelection,
          "JSON restore recreates matching ProjectObjects with requested ObjectIds");

    const int countBeforeFailure = restored.count();

    QJsonObject mixedKind = json;
    QJsonArray mixedKindItems = mixedKind.value(QStringLiteral("items")).toArray();
    QJsonObject mixedKindItem = mixedKindItems.at(0).toObject();
    QJsonArray mixedKindEntities = mixedKindItem.value(QStringLiteral("entities")).toArray();
    QJsonObject mixedKindEntity = mixedKindEntities.at(0).toObject();
    mixedKindEntity[QStringLiteral("kind")] = QStringLiteral("edge");
    mixedKindEntity[QStringLiteral("geometry_entity_id")] = QString::number(hugeGeometryId + 2);
    mixedKindEntity[QStringLiteral("persistent_key")] = QStringLiteral("cad/huge/body/edge");
    mixedKindEntities.push_back(mixedKindEntity);
    mixedKindItem[QStringLiteral("entities")] = mixedKindEntities;
    mixedKindItems[0] = mixedKindItem;
    mixedKind[QStringLiteral("items")] = mixedKindItems;
    error.clear();
    check(!restored.fromJson(mixedKind, &error) && !error.isEmpty(),
          "JSON load rejects one Named Selection that mixes Face and Edge kinds");
    check(restored.count() == countBeforeFailure && restored.byId(hugeObjectId) != nullptr
              && restored.byId(hugeMeshObjectId) != nullptr,
          "mixed-kind JSON rejection is atomic and preserves existing persistent scopes");

    QJsonObject malformed = json;
    QJsonArray malformedItems = malformed.value(QStringLiteral("items")).toArray();
    QJsonObject malformedItem = malformedItems.at(0).toObject();
    malformedItem[QStringLiteral("object_id")] = static_cast<double>(hugeObjectId);
    malformedItems[0] = malformedItem;
    malformed[QStringLiteral("items")] = malformedItems;

    error.clear();
    check(!restored.fromJson(malformed, &error) && !error.isEmpty(),
          "numeric/precision-risk ObjectId input is rejected with an explicit diagnostic");
    check(restored.count() == countBeforeFailure && restored.byId(hugeObjectId) != nullptr
              && restored.byId(hugeMeshObjectId) != nullptr,
          "failed JSON parse is atomic and leaves existing Named Selection state unchanged");

    QJsonObject overflow = json;
    QJsonArray overflowItems = overflow.value(QStringLiteral("items")).toArray();
    QJsonObject overflowMeshItem = overflowItems.at(1).toObject();
    QJsonArray overflowEntities = overflowMeshItem.value(QStringLiteral("entities")).toArray();
    QJsonObject overflowMeshEntity = overflowEntities.at(0).toObject();
    overflowMeshEntity[QStringLiteral("mesh_entity_id")] = QStringLiteral("18446744073709551616");
    overflowEntities[0] = overflowMeshEntity;
    overflowMeshItem[QStringLiteral("entities")] = overflowEntities;
    overflowItems[1] = overflowMeshItem;
    overflow[QStringLiteral("items")] = overflowItems;
    error.clear();
    check(!restored.fromJson(overflow, &error) && !error.isEmpty(),
          "unsigned 64-bit overflow in MeshEntityId is rejected explicitly");
    check(restored.count() == countBeforeFailure && restored.byId(hugeMeshObjectId) != nullptr,
          "overflow rejection is atomic and cannot damage restored engineering scope state");
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    serviceContractTests();
    persistenceTests();

    if (failures == 0) {
        std::cout << "Named Selection service tests PASSED\n";
        return 0;
    }
    std::cerr << failures << " Named Selection service test(s) FAILED\n";
    return 1;
}
