#pragma once

// Dynamics26 Alpha.3.4 — transient selection -> persistent engineering scope.
//
// Geometry scope gerçek CAD topology kimliği + persistentKey ile, FEM scope ise
// gerçek MeshEntityId + mesh generation ile saklanır. Display triangle/line/
// point/cell indeksleri hiçbir zaman persistent engineering identity değildir.
//
// CAD Geometry != Display Tessellation != FEM Mesh

#include "SelectionTypes.h"

#include <femcae/geometry/GeometryDocument.h>
#include <femcae/meshing/MeshTypes.h>

#include <QString>
#include <QVector>

#include <optional>

namespace d26 {

enum class ScopeReferenceBuildError {
    None,
    EmptySelection,
    UnsupportedDomain,
    UnsupportedKind,
    StaleGeometryRevision,
    StaleMeshGeneration,
    MissingGeometryEntity,
    MissingMeshEntity,
    GeometryKindMismatch,
    ParentBodyMismatch,
    MissingPersistentKey
};

struct ScopeReferenceBuildResult {
    ScopeReference scope;
    ScopeReferenceBuildError error{ScopeReferenceBuildError::None};

    [[nodiscard]] bool success() const noexcept {
        return error == ScopeReferenceBuildError::None && !scope.isEmpty();
    }
};

enum class ScopeReferenceValidationError {
    None,
    EmptyScope,
    StaleGeometryRevision,
    StaleMeshGeneration,
    UnsupportedDomain,
    UnsupportedKind,
    MissingGeometryEntity,
    MissingMeshEntity,
    GeometryKindMismatch,
    ParentBodyMismatch,
    MissingPersistentKey,
    PersistentKeyMismatch
};

[[nodiscard]] inline std::optional<femcae::geometry::GeometryEntityKind>
geometryEntityKindForSelectionKind(const SelectionKind kind)
{
    using femcae::geometry::GeometryEntityKind;
    switch (kind) {
    case SelectionKind::Body: return GeometryEntityKind::Body;
    case SelectionKind::Face: return GeometryEntityKind::Face;
    case SelectionKind::Edge: return GeometryEntityKind::Edge;
    case SelectionKind::Vertex: return GeometryEntityKind::Vertex;
    default: return std::nullopt;
    }
}

[[nodiscard]] inline bool geometrySelectionKindHasBodyParent(const SelectionKind kind) noexcept
{
    return kind == SelectionKind::Face || kind == SelectionKind::Edge || kind == SelectionKind::Vertex;
}

[[nodiscard]] inline bool isMeshSelectionKind(const SelectionKind kind) noexcept
{
    return kind == SelectionKind::Node || kind == SelectionKind::Element || kind == SelectionKind::Facet;
}

[[nodiscard]] inline bool meshEntityExists(const femcae::meshing::SimulationMesh &mesh,
                                           const SelectionKind kind,
                                           const femcae::meshing::MeshEntityId id) noexcept
{
    using femcae::meshing::InvalidMeshId;
    if (id == InvalidMeshId) {
        return false;
    }
    if (kind == SelectionKind::Node) {
        return mesh.findNode(id) != nullptr;
    }
    if (kind == SelectionKind::Element) {
        return mesh.findElement(id) != nullptr;
    }
    if (kind == SelectionKind::Facet) {
        for (const auto &facet : mesh.boundaryFacets) {
            if (facet.id == id) {
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] inline ScopeReferenceBuildResult
buildGeometryScopeReference(const QVector<SelectionItem> &items,
                            const femcae::geometry::GeometryDocument &document)
{
    ScopeReferenceBuildResult result;
    if (items.isEmpty()) {
        result.error = ScopeReferenceBuildError::EmptySelection;
        return result;
    }

    QVector<QString> persistentKeys;
    persistentKeys.reserve(items.size());

    for (const SelectionItem &item : items) {
        if (item.domain != SelectionDomain::Geometry) {
            result.scope.entities.clear();
            result.error = ScopeReferenceBuildError::UnsupportedDomain;
            return result;
        }
        const auto expectedKind = geometryEntityKindForSelectionKind(item.kind);
        if (!expectedKind.has_value()) {
            result.scope.entities.clear();
            result.error = ScopeReferenceBuildError::UnsupportedKind;
            return result;
        }
        if (item.sourceRevision == 0 || item.sourceRevision != document.revision()) {
            result.scope.entities.clear();
            result.error = ScopeReferenceBuildError::StaleGeometryRevision;
            return result;
        }

        const femcae::geometry::GeometryEntity *entity = document.find(item.geometryEntityId);
        if (entity == nullptr) {
            result.scope.entities.clear();
            result.error = ScopeReferenceBuildError::MissingGeometryEntity;
            return result;
        }
        if (entity->kind != *expectedKind) {
            result.scope.entities.clear();
            result.error = ScopeReferenceBuildError::GeometryKindMismatch;
            return result;
        }

        if (geometrySelectionKindHasBodyParent(item.kind)
            && (item.parentGeometryId == femcae::geometry::InvalidGeometryId
                || entity->parentId != item.parentGeometryId)) {
            result.scope.entities.clear();
            result.error = ScopeReferenceBuildError::ParentBodyMismatch;
            return result;
        }

        if (entity->persistentKey.empty()) {
            result.scope.entities.clear();
            result.error = ScopeReferenceBuildError::MissingPersistentKey;
            return result;
        }

        const QString persistentKey = QString::fromStdString(entity->persistentKey);
        if (persistentKeys.contains(persistentKey)) {
            continue;
        }
        persistentKeys.push_back(persistentKey);

        ScopeEntityReference reference;
        reference.domain = SelectionDomain::Geometry;
        reference.kind = item.kind;
        reference.geometryEntityId = item.geometryEntityId;
        reference.parentGeometryId = item.parentGeometryId;
        reference.persistentKey = persistentKey;
        result.scope.entities.push_back(reference);
    }

    if (result.scope.isEmpty()) {
        result.error = ScopeReferenceBuildError::EmptySelection;
        return result;
    }
    result.scope.sourceRevision = document.revision();
    return result;
}

[[nodiscard]] inline ScopeReferenceBuildResult
buildMeshScopeReference(const QVector<SelectionItem> &items,
                        const femcae::meshing::SimulationMesh &mesh,
                        const quint64 generation)
{
    ScopeReferenceBuildResult result;
    if (items.isEmpty()) {
        result.error = ScopeReferenceBuildError::EmptySelection;
        return result;
    }
    if (generation == 0) {
        result.error = ScopeReferenceBuildError::StaleMeshGeneration;
        return result;
    }

    for (const SelectionItem &item : items) {
        if (item.domain != SelectionDomain::Mesh) {
            result.scope.entities.clear();
            result.error = ScopeReferenceBuildError::UnsupportedDomain;
            return result;
        }
        if (!isMeshSelectionKind(item.kind)) {
            result.scope.entities.clear();
            result.error = ScopeReferenceBuildError::UnsupportedKind;
            return result;
        }
        if (item.sourceRevision != generation) {
            result.scope.entities.clear();
            result.error = ScopeReferenceBuildError::StaleMeshGeneration;
            return result;
        }
        if (!meshEntityExists(mesh, item.kind, item.meshEntityId)) {
            result.scope.entities.clear();
            result.error = ScopeReferenceBuildError::MissingMeshEntity;
            return result;
        }

        bool duplicate = false;
        for (const ScopeEntityReference &existing : result.scope.entities) {
            if (existing.domain == SelectionDomain::Mesh && existing.kind == item.kind
                && existing.meshEntityId == item.meshEntityId) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            continue;
        }

        ScopeEntityReference reference;
        reference.domain = SelectionDomain::Mesh;
        reference.kind = item.kind;
        reference.meshEntityId = item.meshEntityId;
        result.scope.entities.push_back(reference);
    }

    if (result.scope.isEmpty()) {
        result.error = ScopeReferenceBuildError::EmptySelection;
        return result;
    }
    result.scope.sourceRevision = generation;
    return result;
}

// CAD scope'un oturumlar arasi kalici kimligi raw revision sayaci DEGILDIR.
// Bu helper yalnız stabil topology kimliğini doğrular: GeometryEntityId,
// entity kind, parent Body ve persistentKey. Project-load rebind bu kontrolü
// geçmeden sourceRevision'i current document revision'ina taşıyamaz.
[[nodiscard]] inline ScopeReferenceValidationError
validateGeometryScopeIdentity(const ScopeReference &scope,
                              const femcae::geometry::GeometryDocument &document)
{
    if (scope.isEmpty()) {
        return ScopeReferenceValidationError::EmptyScope;
    }

    for (const ScopeEntityReference &reference : scope.entities) {
        if (reference.domain != SelectionDomain::Geometry) {
            return ScopeReferenceValidationError::UnsupportedDomain;
        }
        const auto expectedKind = geometryEntityKindForSelectionKind(reference.kind);
        if (!expectedKind.has_value()) {
            return ScopeReferenceValidationError::UnsupportedKind;
        }
        if (reference.persistentKey.isEmpty()) {
            return ScopeReferenceValidationError::MissingPersistentKey;
        }

        const femcae::geometry::GeometryEntity *entity = document.find(reference.geometryEntityId);
        if (entity == nullptr) {
            return ScopeReferenceValidationError::MissingGeometryEntity;
        }
        if (entity->kind != *expectedKind) {
            return ScopeReferenceValidationError::GeometryKindMismatch;
        }
        if (geometrySelectionKindHasBodyParent(reference.kind)
            && (reference.parentGeometryId == femcae::geometry::InvalidGeometryId
                || entity->parentId != reference.parentGeometryId)) {
            return ScopeReferenceValidationError::ParentBodyMismatch;
        }
        if (QString::fromStdString(entity->persistentKey) != reference.persistentKey) {
            return ScopeReferenceValidationError::PersistentKeyMismatch;
        }
    }
    return ScopeReferenceValidationError::None;
}

// Yalnız kontrollü proje yükleme aşamasında kullanılır. Scope'un stabil CAD
// kimliği current document ile birebir uyuşuyorsa runtime revision guard yeniden
// bağlanır. Runtime GeometryService::changed akışında bu helper çağrılmaz; böylece
// topology değişiminde sessiz rebind yapılmaz.
[[nodiscard]] inline bool
rebindLoadedGeometryScopeReference(ScopeReference &scope,
                                   const femcae::geometry::GeometryDocument &document)
{
    if (document.revision() == 0
        || validateGeometryScopeIdentity(scope, document) != ScopeReferenceValidationError::None) {
        return false;
    }
    scope.sourceRevision = document.revision();
    return true;
}

// Persistent CAD scope kullanilacagi anda current GeometryDocument'a karsi
// tekrar dogrulanir. Otomatik topology rebind yapilmaz.
[[nodiscard]] inline ScopeReferenceValidationError
validateGeometryScopeReference(const ScopeReference &scope,
                               const femcae::geometry::GeometryDocument &document)
{
    if (scope.isEmpty()) {
        return ScopeReferenceValidationError::EmptyScope;
    }
    if (scope.sourceRevision == 0 || scope.sourceRevision != document.revision()) {
        return ScopeReferenceValidationError::StaleGeometryRevision;
    }
    return validateGeometryScopeIdentity(scope, document);
}

// FEM scope yalnız oluşturulduğu mesh generation üzerinde geçerlidir. Mesh
// regenerate/clear/reset sonrası ID sayıları tesadüfen aynı olsa bile eski scope
// yeni mesh'e sessizce bağlanmaz.
[[nodiscard]] inline ScopeReferenceValidationError
validateMeshScopeReference(const ScopeReference &scope,
                           const femcae::meshing::SimulationMesh &mesh,
                           const quint64 generation)
{
    if (scope.isEmpty()) {
        return ScopeReferenceValidationError::EmptyScope;
    }
    if (generation == 0 || scope.sourceRevision != generation) {
        return ScopeReferenceValidationError::StaleMeshGeneration;
    }

    for (const ScopeEntityReference &reference : scope.entities) {
        if (reference.domain != SelectionDomain::Mesh) {
            return ScopeReferenceValidationError::UnsupportedDomain;
        }
        if (!isMeshSelectionKind(reference.kind)) {
            return ScopeReferenceValidationError::UnsupportedKind;
        }
        if (!meshEntityExists(mesh, reference.kind, reference.meshEntityId)) {
            return ScopeReferenceValidationError::MissingMeshEntity;
        }
    }
    return ScopeReferenceValidationError::None;
}

} // namespace d26
