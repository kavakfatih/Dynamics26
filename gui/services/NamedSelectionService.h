#pragma once

// Dynamics26 Alpha.3.6 — Named Selection document service.
//
// Named Selection transient viewport selection DEGILDIR. ProjectModel yalnız
// Navigator'daki ObjectId/ObjectType kimliğini taşır; gerçek engineering scope
// bu serviste ScopeReference olarak yaşar.
//
// CAD Geometry != Display Tessellation != FEM Mesh

#include "../core/ProjectModel.h"
#include "../core/ScopeReferenceBuilder.h"
#include "GeometryService.h"
#include "MeshService.h"

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVector>

namespace d26 {

struct NamedSelectionDefinition {
    QString name{QStringLiteral("Named Selection")};
    ScopeReference scope;
};

struct NamedSelectionCreateResult {
    ObjectId id{InvalidObjectId};
    ScopeReferenceBuildError buildError{ScopeReferenceBuildError::None};

    [[nodiscard]] bool success() const noexcept
    {
        return id != InvalidObjectId && buildError == ScopeReferenceBuildError::None;
    }
};

class NamedSelectionService final : public QObject
{
    Q_OBJECT
public:
    NamedSelectionService(ProjectModel *project, GeometryService *geometry, MeshService *mesh,
                          QObject *parent = nullptr);

    [[nodiscard]] const QVector<ObjectId> &order() const noexcept { return order_; }
    [[nodiscard]] int count() const noexcept { return static_cast<int>(order_.size()); }
    [[nodiscard]] const NamedSelectionDefinition *byId(ObjectId id) const noexcept;
    [[nodiscard]] int rowOf(ObjectId id) const noexcept;

    // Current transient selection'dan persistent scope üretir. Geometry ve Mesh
    // entity'leri tek Named Selection içinde karıştırılmaz.
    [[nodiscard]] NamedSelectionCreateResult
    createFromSelection(const QVector<SelectionItem> &items,
                        const QString &requestedName = QStringLiteral("Named Selection"));

    // Undo/persistence katmanı için explicit scope + ObjectId yolu. Stale scope
    // yüklenebilir; servis onu sessizce valid saymaz, node state'ini OutOfDate
    // veya Error yapar.
    ObjectId createWithScope(const NamedSelectionDefinition &definition, int row = -1,
                             ObjectId requestedId = InvalidObjectId);
    bool remove(ObjectId id);
    void rename(ObjectId id, const QString &name);
    void replaceScope(ObjectId id, const ScopeReference &scope);
    void clear();

    [[nodiscard]] ScopeReferenceValidationError validate(ObjectId id) const;
    void refreshValidation();

    // Alpha.3.6 persistence data contract. Main project save/load wiring bu JSON
    // payload'u dynamics26_document içine bağlayacaktır.
    [[nodiscard]] QJsonObject toJson() const;
    void fromJson(const QJsonObject &object);

signals:
    void changed();

private:
    [[nodiscard]] QString uniqueName(const QString &base) const;
    [[nodiscard]] ScopeReferenceValidationError validateScope(const ScopeReference &scope) const;
    void refreshNode(ObjectId id);

    ProjectModel *project_{nullptr};
    GeometryService *geometry_{nullptr};
    MeshService *mesh_{nullptr};
    QHash<ObjectId, NamedSelectionDefinition> definitions_;
    QVector<ObjectId> order_;
};

} // namespace d26
