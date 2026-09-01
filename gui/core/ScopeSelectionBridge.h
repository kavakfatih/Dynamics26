#pragma once

// Dynamics26 Alpha.3.6 — persistent engineering scope -> transient selection.
//
// Bu dosya yeni bir scope modeli TANIMLAMAZ. Kalıcı kimliğin tek kaynağı
// ScopeReference, doğrulamanın tek kaynağı ScopeReferenceBuilder'dır. Edit Scope
// oturumu yalnız current CAD revision / Mesh generation üzerinde halen geçerli
// olan bir ScopeReference'i SelectionManager öğelerine dönüştürebilir.
//
// En önemli güvenlik kuralı:
//   stale persistent scope -> transient ID preload YOK.
// Eski GeometryEntityId / MeshEntityId sayıları tesadüfen yeniden görünse bile
// kullanıcıya seçilmiş gibi gösterilmez ve sessiz topology/mesh rebind yapılmaz.

#include "ScopeReferenceBuilder.h"

namespace d26 {

struct ScopeSelectionItemsResult {
    QVector<SelectionItem> items;
    ScopeReferenceValidationError error{ScopeReferenceValidationError::None};

    [[nodiscard]] bool success() const noexcept
    {
        return error == ScopeReferenceValidationError::None && !items.isEmpty();
    }
};

[[nodiscard]] inline ScopeSelectionItemsResult
selectionItemsForGeometryScope(const ScopeReference &scope,
                               const femcae::geometry::GeometryDocument &document)
{
    ScopeSelectionItemsResult result;
    result.error = validateGeometryScopeReference(scope, document);
    if (result.error != ScopeReferenceValidationError::None) {
        return result;
    }

    const SelectionKind kind = scope.entities.front().kind;
    result.items.reserve(scope.entities.size());
    for (const ScopeEntityReference &reference : scope.entities) {
        if (reference.domain != SelectionDomain::Geometry) {
            result.items.clear();
            result.error = ScopeReferenceValidationError::UnsupportedDomain;
            return result;
        }
        if (reference.kind != kind) {
            result.items.clear();
            result.error = ScopeReferenceValidationError::UnsupportedKind;
            return result;
        }

        SelectionItem item;
        item.domain = SelectionDomain::Geometry;
        item.kind = reference.kind;
        item.geometryEntityId = reference.geometryEntityId;
        item.parentGeometryId = reference.parentGeometryId;
        item.sourceRevision = document.revision();
        result.items.push_back(item);
    }
    return result;
}

[[nodiscard]] inline ScopeSelectionItemsResult
selectionItemsForMeshScope(const ScopeReference &scope,
                           const femcae::meshing::SimulationMesh &mesh,
                           const quint64 generation)
{
    ScopeSelectionItemsResult result;
    result.error = validateMeshScopeReference(scope, mesh, generation);
    if (result.error != ScopeReferenceValidationError::None) {
        return result;
    }

    const SelectionKind kind = scope.entities.front().kind;
    result.items.reserve(scope.entities.size());
    for (const ScopeEntityReference &reference : scope.entities) {
        if (reference.domain != SelectionDomain::Mesh) {
            result.items.clear();
            result.error = ScopeReferenceValidationError::UnsupportedDomain;
            return result;
        }
        if (reference.kind != kind) {
            result.items.clear();
            result.error = ScopeReferenceValidationError::UnsupportedKind;
            return result;
        }

        SelectionItem item;
        item.domain = SelectionDomain::Mesh;
        item.kind = reference.kind;
        item.meshEntityId = reference.meshEntityId;
        item.sourceRevision = generation;
        result.items.push_back(item);
    }
    return result;
}

} // namespace d26
