#include "AnalysisService.h"

#include "NamedSelectionService.h"

#include <femcae/meshing/AssignmentResolver.h>

#include <QJsonArray>

#include <algorithm>
#include <set>

using femcae::geometry::GeometryEntityId;
using femcae::geometry::InvalidGeometryId;
using femcae::meshing::MeshEntityId;
using femcae::meshing::boundaryNodeIdsForGeometry;

namespace d26 {
namespace {

QString boundaryScopeMethodName(const BoundaryScopingMethod method)
{
    return method == BoundaryScopingMethod::NamedSelection
        ? QStringLiteral("Named Selection")
        : QStringLiteral("Geometry Selection");
}

} // namespace

BoundaryScopeResolution AnalysisService::resolveBoundaryScope(const SupportDefinition &definition) const
{
    return resolveBoundaryScope(definition.scopingMethod, definition.scope, definition.namedSelectionId);
}

BoundaryScopeResolution AnalysisService::resolveBoundaryScope(const LoadDefinition &definition) const
{
    return resolveBoundaryScope(definition.scopingMethod, definition.scope, definition.namedSelectionId);
}

BoundaryScopeResolution AnalysisService::resolveBoundaryScope(const BoundaryScopingMethod method,
                                                               const BoxFace geometryScope,
                                                               const ObjectId namedSelectionId) const
{
    BoundaryScopeResolution result;
    result.method = method;
    result.namedSelectionId = namedSelectionId;

    if (mesh_ == nullptr) {
        result.error = tr("Mesh servisi kullanılamıyor.");
        return result;
    }

    if (method == BoundaryScopingMethod::GeometrySelection) {
        const GeometryEntityId geometryId = mesh_->geometryIdFor(geometryScope);
        if (geometryId == InvalidGeometryId) {
            result.error = tr("Geometri kapsamı current model üzerinde çözülemedi.");
            return result;
        }
        result.geometryFaceIds.push_back(geometryId);
        result.label = displayName(geometryScope);
        result.valid = true;
        return result;
    }

    if (method != BoundaryScopingMethod::NamedSelection) {
        result.error = tr("Desteklenmeyen scoping method: %1").arg(boundaryScopeMethodName(method));
        return result;
    }
    if (namedSelections_ == nullptr) {
        result.error = tr("Named Selection servisi kullanılamıyor.");
        return result;
    }
    if (namedSelectionId == InvalidObjectId) {
        result.error = tr("Named Selection seçilmedi.");
        return result;
    }

    const NamedSelectionDefinition *named = namedSelections_->byId(namedSelectionId);
    if (named == nullptr) {
        result.error = tr("Referenced Named Selection bulunamadı.");
        return result;
    }

    result.validationError = namedSelections_->validate(namedSelectionId);
    if (result.validationError != ScopeReferenceValidationError::None) {
        result.error = tr("Named Selection current model üzerinde geçerli değil.");
        return result;
    }
    if (named->scope.entities.isEmpty()) {
        result.error = tr("Named Selection boş bir scope içeriyor.");
        return result;
    }

    std::set<GeometryEntityId> uniqueFaces;
    for (const ScopeEntityReference &entity : named->scope.entities) {
        // Surface boundary-condition consumer yalnız CAD Face scope tüketir.
        // Mesh Node/Element/Facet veya CAD Edge/Vertex hiçbir zaman sessizce
        // yüz kapsamına dönüştürülmez.
        if (entity.domain != SelectionDomain::Geometry || entity.kind != SelectionKind::Face
            || entity.geometryEntityId == InvalidGeometryId) {
            result.geometryFaceIds.clear();
            result.error = tr("Fixed Support / Force için Named Selection Geometry Face kapsamı olmalıdır.");
            return result;
        }
        uniqueFaces.insert(entity.geometryEntityId);
    }

    if (uniqueFaces.empty()) {
        result.error = tr("Named Selection çözülebilir CAD Face içermiyor.");
        return result;
    }

    result.geometryFaceIds.reserve(static_cast<qsizetype>(uniqueFaces.size()));
    for (const GeometryEntityId id : uniqueFaces) {
        result.geometryFaceIds.push_back(id);
    }
    result.label = named->name;
    result.valid = true;
    return result;
}

std::vector<MeshEntityId>
AnalysisService::resolvedBoundaryNodeIds(const BoundaryScopeResolution &scope) const
{
    if (!scope.valid || mesh_ == nullptr || !mesh_->hasMesh()) {
        return {};
    }

    // Bir Named Selection birden fazla Face içerdiğinde her Face için ayrı Load
    // assignment üretmek toplam kuvveti N kez uygulardı. Burada bütün yüzlerin
    // node'ları önce TEK birleşik kümeye alınır. Constraint ve toplam kuvvet
    // yalnız bu birleşik scope üzerinde bir kez uygulanır.
    std::set<MeshEntityId> uniqueNodes;
    for (const GeometryEntityId geometryId : scope.geometryFaceIds) {
        const auto ids = boundaryNodeIdsForGeometry(mesh_->mesh(), geometryId);
        uniqueNodes.insert(ids.begin(), ids.end());
    }
    return {uniqueNodes.begin(), uniqueNodes.end()};
}

int AnalysisService::resolvedBoundaryNodeCount(const SupportDefinition &definition) const
{
    return static_cast<int>(resolvedBoundaryNodeIds(resolveBoundaryScope(definition)).size());
}

int AnalysisService::resolvedBoundaryNodeCount(const LoadDefinition &definition) const
{
    return static_cast<int>(resolvedBoundaryNodeIds(resolveBoundaryScope(definition)).size());
}

QJsonObject AnalysisService::boundaryScopeSignature(const BoundaryScopingMethod method,
                                                    const BoxFace geometryScope,
                                                    const ObjectId namedSelectionId) const
{
    QJsonObject signature;
    signature[QStringLiteral("method")] = static_cast<int>(method);

    if (method == BoundaryScopingMethod::GeometrySelection) {
        signature[QStringLiteral("geometry_scope")] = static_cast<int>(geometryScope);
        const BoundaryScopeResolution resolution = resolveBoundaryScope(method, geometryScope, InvalidObjectId);
        signature[QStringLiteral("valid")] = resolution.valid;
        if (resolution.valid && !resolution.geometryFaceIds.isEmpty()) {
            signature[QStringLiteral("geometry_face_id")] =
                QString::number(static_cast<qulonglong>(resolution.geometryFaceIds.front()));
        }
        return signature;
    }

    // ObjectId ve engineering identity'ler JSON number olarak yazılmaz; solver
    // input signature da persistence ile aynı exact-64-bit kuralını korur.
    signature[QStringLiteral("named_selection_id")] = QString::number(namedSelectionId);
    const BoundaryScopeResolution resolution = resolveBoundaryScope(method, geometryScope, namedSelectionId);
    signature[QStringLiteral("valid")] = resolution.valid;
    signature[QStringLiteral("validation_error")] = static_cast<int>(resolution.validationError);
    if (!resolution.valid || namedSelections_ == nullptr) {
        signature[QStringLiteral("error")] = resolution.error;
        return signature;
    }

    const NamedSelectionDefinition *named = namedSelections_->byId(namedSelectionId);
    if (named == nullptr) {
        signature[QStringLiteral("valid")] = false;
        return signature;
    }

    signature[QStringLiteral("source_revision")] = QString::number(named->scope.sourceRevision);
    QJsonArray entities;
    for (const ScopeEntityReference &entity : named->scope.entities) {
        QJsonObject item;
        item[QStringLiteral("domain")] = static_cast<int>(entity.domain);
        item[QStringLiteral("kind")] = static_cast<int>(entity.kind);
        item[QStringLiteral("geometry_entity_id")] =
            QString::number(static_cast<qulonglong>(entity.geometryEntityId));
        item[QStringLiteral("parent_geometry_id")] =
            QString::number(static_cast<qulonglong>(entity.parentGeometryId));
        item[QStringLiteral("persistent_key")] = entity.persistentKey;
        entities.append(item);
    }
    signature[QStringLiteral("entities")] = entities;
    return signature;
}

} // namespace d26
