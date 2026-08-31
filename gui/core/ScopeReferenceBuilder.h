#pragma once

// Dynamics26 Alpha.3.3 — transient CAD selection -> persistent engineering scope.
//
// SelectionItem ekran/oturum durumudur; ScopeReference ise Material/BC/Load/
// Contact/Mesh gibi mühendislik tanimlarinin kalici kapsamini temsil edecek
// data-only kontrattir. Display triangle/line/point veya FEM kimlikleri CAD
// scope gibi kabul edilmez. Yalniz current GeometryDocument revision'indaki
// gercek Body/Face/Edge/Vertex entity'leri persistentKey ile scope'a cevrilir.

#include "SelectionTypes.h"

#include <femcae/geometry/GeometryDocument.h>

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
    MissingGeometryEntity,
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
    UnsupportedDomain,
    UnsupportedKind,
    MissingGeometryEntity,
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

// Persistent scope kullanilacagi anda current GeometryDocument'a karsi tekrar
// dogrulanir. Otomatik topology rebind yapilmaz: revision degismisse scope
// stale'dir. persistentKey gelecekte acik bir rebind/migration islemine temel
// olabilir, fakat stale scope sessizce kabul edilmez.
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

} // namespace d26
