#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.2 — Contact engineering document service.
//
// ProjectModel ContactRegion için yalnız tree identity (ObjectId/ObjectType/name/
// state) taşır. Source/target engineering scope verisi bu servisin sahibidir.
// Viewport triangle/cell/point indeksleri persistent contact tanımına yazılmaz.
//
// CAD Geometry != Display Tessellation != FEM Mesh
//
// İlk Beta.1 contract yalnız surface-to-surface tanımı kurar:
//   - CAD tarafında Geometry / Face
//   - FEM tarafında Mesh / Facet
// Source ve target aynı engineering domain'de olmak zorundadır. Geometry scope
// GeometryDocument revision + persistentKey ile, Mesh scope Mesh generation ile
// mevcut ScopeReference lifecycle kurallarına göre doğrulanır. Stale scope
// sessizce yeni topology/mesh'e rebind edilmez.

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

// Enum değerleri persistence contract'ın parçasıdır; mevcut değerler gelecekte
// yeniden sıralanmamalıdır. Bonded burada contact authoring formulation intent'ini
// temsil eder; mevcut Static Structural solver'ın contact fiziğini çözdüğü
// anlamına GELMEZ.
enum class ContactFormulation {
    Bonded = 0
};

struct ContactDefinition {
    QString name{QStringLiteral("Contact Region")};
    ScopeReference sourceScope;
    ScopeReference targetScope;
    ContactFormulation formulation{ContactFormulation::Bonded};
};

enum class ContactValidationError {
    None,
    MissingSourceScope,
    MissingTargetScope,
    UnsupportedSourceSurface,
    UnsupportedTargetSurface,
    MixedDomains,
    IdenticalSourceAndTarget,
    SourceScopeInvalid,
    TargetScopeInvalid
};

struct ContactValidationResult {
    ContactValidationError error{ContactValidationError::None};
    ScopeReferenceValidationError sourceScopeError{ScopeReferenceValidationError::None};
    ScopeReferenceValidationError targetScopeError{ScopeReferenceValidationError::None};

    [[nodiscard]] bool valid() const noexcept { return error == ContactValidationError::None; }
};

class ContactService final : public QObject
{
    Q_OBJECT
public:
    ContactService(ProjectModel *project, GeometryService *geometry, MeshService *mesh,
                   QObject *parent = nullptr);

    [[nodiscard]] const QVector<ObjectId> &order() const noexcept { return order_; }
    [[nodiscard]] int count() const noexcept { return static_cast<int>(order_.size()); }
    [[nodiscard]] const ContactDefinition *byId(ObjectId id) const noexcept;
    [[nodiscard]] int rowOf(ObjectId id) const noexcept;

    // Undo/persistence için explicit ObjectId + row restore yolu. Source ve/veya
    // Target canonical unset ScopeReference olabilir (revision=0 + entities=[]):
    // bu, normal Contact authoring sırasında kaydedilebilir EKSİK document
    // state'idir. Tam scope ise CAD Face veya FEM Facet surface contract'ına
    // uymalıdır. revision/entity bakımından yarım veya malformed scope reddedilir.
    // Runtime stale revision/generation ise object korunur ve OutOfDate görünür;
    // proje yüklemede veri sessizce kaybolmaz veya yeni topology'ye rebind edilmez.
    ObjectId createContact(const ContactDefinition &definition, int row = -1,
                           ObjectId requestedId = InvalidObjectId);
    bool remove(ObjectId id);
    void rename(ObjectId id, const QString &name);
    bool replaceSourceScope(ObjectId id, const ScopeReference &scope);
    bool replaceTargetScope(ObjectId id, const ScopeReference &scope);
    void setFormulation(ObjectId id, ContactFormulation formulation);
    void setSuppressed(ObjectId id, bool suppressed);
    void clear();

    [[nodiscard]] ContactValidationResult validate(ObjectId id) const;
    void refreshValidation();

    // 64-bit engineering kimlikleri JSON number/double olarak yazılmaz.
    // ObjectId, GeometryEntityId, MeshEntityId ve revision/generation decimal
    // string olarak saklanır; parse/overflow hataları false ile dışarı bildirilir.
    [[nodiscard]] QJsonObject toJson() const;
    bool fromJson(const QJsonObject &object, QString *errorMessage = nullptr);

signals:
    void changed();

private:
    [[nodiscard]] QString uniqueName(const QString &base, ObjectId excludingId = InvalidObjectId) const;
    [[nodiscard]] ContactValidationResult validateDefinition(const ContactDefinition &definition) const;
    void refreshNode(ObjectId id);

    ProjectModel *project_{nullptr};
    GeometryService *geometry_{nullptr};
    MeshService *mesh_{nullptr};
    QHash<ObjectId, ContactDefinition> definitions_;
    QVector<ObjectId> order_;
};

} // namespace d26
