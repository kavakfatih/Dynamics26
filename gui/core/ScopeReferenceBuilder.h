#pragma once

// Dynamics26 Alpha.3.2 — transient CAD selection -> persistent engineering scope.
//
// SelectionItem ekran/oturum durumudur; ScopeReference ise Material/BC/Load/
// Contact gibi mühendislik tanimlarinin kalici kapsamini temsil edecek data-only
// kontrattir. Bu builder display triangle veya FEM facet kimligini CAD scope gibi
// kabul etmez. Yalniz current GeometryDocument revision'indaki gercek Body/Face
// entity'lerini persistentKey ile scope'a cevirir.

#include "SelectionTypes.h"

#include <femcae/geometry/GeometryDocument.h>

#include <QString>
#include <QVector>

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
    MissingPersistentKey,
    PersistentKeyMismatch
};

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
        if (item.kind != SelectionKind::Body && item.kind != SelectionKind::Face) {
            result.scope.entities.clear();
            result.error = ScopeReferenceBuildError::UnsupportedKind;
            return result;
        }
        if (item.sourceRevision != document.revision()) {
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

        const auto expectedKind = item.kind == SelectionKind::Body
            ? femcae::geometry::GeometryEntityKind::Body
            : femcae::geometry::GeometryEntityKind::Face;
        if (entity->kind != expectedKind) {
            result.scope.entities.clear();
            result.error = ScopeReferenceBuildError::GeometryKindMismatch;
            return result;
        }

        if (item.kind == SelectionKind::Face
            && item.parentGeometryId != femcae::geometry::InvalidGeometryId
            && entity->parentId != item.parentGeometryId) {
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
        // SelectionManager zaten identity bazinda duplicate tutmaz; builder yine
        // de harici/programatik caller icin persistent scope'u deterministik ve
        // tekil tutar.
        if (persistentKeys.contains(persistentKey)) {
            continue;
        }
        persistentKeys.push_back(persistentKey);

        ScopeEntityReference reference;
        reference.domain = SelectionDomain::Geometry;
        reference.kind = item.kind;
        reference.geometryEntityId = item.geometryEntityId;
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

// Persistent scope daha sonra kullanilacagi anda current GeometryDocument'a
// karsi tekrar dogrulanir. Alpha.3.2 bilincli olarak otomatik topology rebind
// yapmaz: revision degismisse scope stale'dir. persistentKey gelecekte acik bir
// rebind/migration islemine temel olabilir, fakat stale scope sessizce kabul edilmez.
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
        if (reference.kind != SelectionKind::Body && reference.kind != SelectionKind::Face) {
            return ScopeReferenceValidationError::UnsupportedKind;
        }
        if (reference.persistentKey.isEmpty()) {
            return ScopeReferenceValidationError::MissingPersistentKey;
        }

        const femcae::geometry::GeometryEntity *entity = document.find(reference.geometryEntityId);
        if (entity == nullptr) {
            return ScopeReferenceValidationError::MissingGeometryEntity;
        }
        const auto expectedKind = reference.kind == SelectionKind::Body
            ? femcae::geometry::GeometryEntityKind::Body
            : femcae::geometry::GeometryEntityKind::Face;
        if (entity->kind != expectedKind) {
            return ScopeReferenceValidationError::GeometryKindMismatch;
        }
        if (QString::fromStdString(entity->persistentKey) != reference.persistentKey) {
            return ScopeReferenceValidationError::PersistentKeyMismatch;
        }
    }
    return ScopeReferenceValidationError::None;
}

} // namespace d26
