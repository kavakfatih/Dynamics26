#include "ContactService.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QSet>

#include <algorithm>

namespace d26 {
namespace {

QString domainToken(const SelectionDomain domain)
{
    switch (domain) {
    case SelectionDomain::Geometry: return QStringLiteral("geometry");
    case SelectionDomain::Mesh: return QStringLiteral("mesh");
    case SelectionDomain::ProjectObject: return QStringLiteral("project_object");
    }
    return {};
}

bool parseDomainToken(const QString &token, SelectionDomain *domain)
{
    if (domain == nullptr) {
        return false;
    }
    if (token == QStringLiteral("geometry")) {
        *domain = SelectionDomain::Geometry;
        return true;
    }
    if (token == QStringLiteral("mesh")) {
        *domain = SelectionDomain::Mesh;
        return true;
    }
    return false;
}

QString kindToken(const SelectionKind kind)
{
    switch (kind) {
    case SelectionKind::Face: return QStringLiteral("face");
    case SelectionKind::Facet: return QStringLiteral("facet");
    default: return {};
    }
}

bool parseKindToken(const QString &token, SelectionKind *kind)
{
    if (kind == nullptr) {
        return false;
    }
    if (token == QStringLiteral("face")) {
        *kind = SelectionKind::Face;
        return true;
    }
    if (token == QStringLiteral("facet")) {
        *kind = SelectionKind::Facet;
        return true;
    }
    return false;
}

bool parseUnsigned64(const QJsonObject &object, const QString &key, quint64 *result,
                     QString *errorMessage)
{
    if (result == nullptr) {
        return false;
    }
    const QJsonValue value = object.value(key);
    if (!value.isString()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("'%1' 64-bit integer string olmalıdır").arg(key);
        }
        return false;
    }
    bool ok = false;
    const qulonglong parsed = value.toString().toULongLong(&ok, 10);
    if (!ok) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("'%1' geçerli bir unsigned 64-bit integer değil").arg(key);
        }
        return false;
    }
    *result = static_cast<quint64>(parsed);
    return true;
}

bool scopeIsSurfaceStructure(const ScopeReference &scope)
{
    if (scope.sourceRevision == 0 || scope.entities.isEmpty()) {
        return false;
    }

    const SelectionDomain domain = scope.entities.front().domain;
    const SelectionKind kind = scope.entities.front().kind;
    if ((domain == SelectionDomain::Geometry && kind != SelectionKind::Face)
        || (domain == SelectionDomain::Mesh && kind != SelectionKind::Facet)
        || (domain != SelectionDomain::Geometry && domain != SelectionDomain::Mesh)) {
        return false;
    }

    for (const ScopeEntityReference &reference : scope.entities) {
        if (reference.domain != domain || reference.kind != kind) {
            return false;
        }
        if (domain == SelectionDomain::Geometry) {
            if (reference.geometryEntityId == femcae::geometry::InvalidGeometryId
                || reference.parentGeometryId == femcae::geometry::InvalidGeometryId
                || reference.persistentKey.isEmpty()) {
                return false;
            }
        } else if (reference.meshEntityId == femcae::meshing::InvalidMeshId) {
            return false;
        }
    }
    return true;
}

bool scopeIsCanonicalUnset(const ScopeReference &scope)
{
    return scope.sourceRevision == 0 && scope.entities.isEmpty();
}

bool scopeIsAuthoringStructure(const ScopeReference &scope)
{
    // Contact authoring iki kontrollü durumu kabul eder:
    // 1) canonical unset: revision=0 + entities=[]
    // 2) tam surface scope: revision>0 + geçerli Face/Facet kimlikleri
    // Bunların arasındaki yarım encoding'ler kalıcı document state değildir.
    return scopeIsCanonicalUnset(scope) || scopeIsSurfaceStructure(scope);
}

QString identityToken(const ScopeEntityReference &reference)
{
    if (reference.domain == SelectionDomain::Geometry) {
        return QStringLiteral("g:%1:%2:%3")
            .arg(static_cast<qulonglong>(reference.geometryEntityId))
            .arg(static_cast<qulonglong>(reference.parentGeometryId))
            .arg(reference.persistentKey);
    }
    if (reference.domain == SelectionDomain::Mesh) {
        return QStringLiteral("m:%1").arg(static_cast<qulonglong>(reference.meshEntityId));
    }
    return {};
}

bool sameSurfaceIdentity(const ScopeReference &a, const ScopeReference &b)
{
    if (a.entities.isEmpty() || b.entities.isEmpty()
        || a.entities.front().domain != b.entities.front().domain
        || a.entities.front().kind != b.entities.front().kind
        || a.entities.size() != b.entities.size()) {
        return false;
    }

    QSet<QString> aIds;
    QSet<QString> bIds;
    for (const ScopeEntityReference &reference : a.entities) {
        aIds.insert(identityToken(reference));
    }
    for (const ScopeEntityReference &reference : b.entities) {
        bIds.insert(identityToken(reference));
    }
    return aIds == bIds;
}

ObjectState stateForValidation(const ContactValidationResult &result)
{
    if (result.valid()) {
        return ObjectState::Ready;
    }
    const auto stale = [](const ScopeReferenceValidationError error) {
        return error == ScopeReferenceValidationError::StaleGeometryRevision
            || error == ScopeReferenceValidationError::StaleMeshGeneration;
    };
    if (stale(result.sourceScopeError) || stale(result.targetScopeError)) {
        return ObjectState::OutOfDate;
    }
    return ObjectState::Error;
}

QString scopeErrorText(const ScopeReferenceValidationError error)
{
    switch (error) {
    case ScopeReferenceValidationError::None: return QStringLiteral("geçerli");
    case ScopeReferenceValidationError::EmptyScope: return QStringLiteral("scope boş");
    case ScopeReferenceValidationError::StaleGeometryRevision:
        return QStringLiteral("CAD Geometry revision değişti");
    case ScopeReferenceValidationError::StaleMeshGeneration:
        return QStringLiteral("FEM Mesh yeniden üretildi");
    case ScopeReferenceValidationError::UnsupportedDomain:
        return QStringLiteral("domain desteklenmiyor");
    case ScopeReferenceValidationError::UnsupportedKind:
        return QStringLiteral("entity kind desteklenmiyor");
    case ScopeReferenceValidationError::MissingGeometryEntity:
        return QStringLiteral("CAD topology entity bulunamadı");
    case ScopeReferenceValidationError::MissingMeshEntity:
        return QStringLiteral("FEM Mesh entity bulunamadı");
    case ScopeReferenceValidationError::GeometryKindMismatch:
        return QStringLiteral("CAD topology kind eşleşmiyor");
    case ScopeReferenceValidationError::ParentBodyMismatch:
        return QStringLiteral("parent Body eşleşmiyor");
    case ScopeReferenceValidationError::MissingPersistentKey:
        return QStringLiteral("persistentKey eksik");
    case ScopeReferenceValidationError::PersistentKeyMismatch:
        return QStringLiteral("persistentKey eşleşmiyor");
    }
    return QStringLiteral("scope doğrulanamadı");
}

QString validationText(const ContactValidationResult &result)
{
    switch (result.error) {
    case ContactValidationError::None:
        return QStringLiteral("Contact tanımı geçerli");
    case ContactValidationError::MissingSourceScope:
        return QStringLiteral("Source scope tanımlanmadı");
    case ContactValidationError::MissingTargetScope:
        return QStringLiteral("Target scope tanımlanmadı");
    case ContactValidationError::UnsupportedSourceSurface:
        return QStringLiteral("Source scope surface olmalıdır: CAD Face veya FEM Facet");
    case ContactValidationError::UnsupportedTargetSurface:
        return QStringLiteral("Target scope surface olmalıdır: CAD Face veya FEM Facet");
    case ContactValidationError::MixedDomains:
        return QStringLiteral("Source ve Target aynı engineering domain'de olmalıdır");
    case ContactValidationError::IdenticalSourceAndTarget:
        return QStringLiteral("Source ve Target aynı surface scope olamaz");
    case ContactValidationError::SourceScopeInvalid:
        return QStringLiteral("Source scope geçersiz: %1").arg(scopeErrorText(result.sourceScopeError));
    case ContactValidationError::TargetScopeInvalid:
        return QStringLiteral("Target scope geçersiz: %1").arg(scopeErrorText(result.targetScopeError));
    }
    return QStringLiteral("Contact tanımı doğrulanamadı");
}

QJsonObject scopeToJson(const ScopeReference &scope)
{
    QJsonObject object;
    object[QStringLiteral("source_revision")] = QString::number(scope.sourceRevision);
    QJsonArray entities;
    for (const ScopeEntityReference &reference : scope.entities) {
        QJsonObject entity;
        entity[QStringLiteral("domain")] = domainToken(reference.domain);
        entity[QStringLiteral("kind")] = kindToken(reference.kind);
        if (reference.domain == SelectionDomain::Geometry) {
            entity[QStringLiteral("geometry_entity_id")] =
                QString::number(static_cast<qulonglong>(reference.geometryEntityId));
            entity[QStringLiteral("parent_geometry_id")] =
                QString::number(static_cast<qulonglong>(reference.parentGeometryId));
            entity[QStringLiteral("persistent_key")] = reference.persistentKey;
        } else if (reference.domain == SelectionDomain::Mesh) {
            entity[QStringLiteral("mesh_entity_id")] =
                QString::number(static_cast<qulonglong>(reference.meshEntityId));
        }
        entities.push_back(entity);
    }
    object[QStringLiteral("entities")] = entities;
    return object;
}

bool scopeFromJson(const QJsonObject &object, ScopeReference *scope, QString *errorMessage)
{
    if (scope == nullptr) {
        return false;
    }

    quint64 sourceRevision = 0;
    if (!parseUnsigned64(object, QStringLiteral("source_revision"), &sourceRevision, errorMessage)) {
        return false;
    }

    const QJsonValue entitiesValue = object.value(QStringLiteral("entities"));
    if (!entitiesValue.isArray()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("entities array olmalıdır");
        }
        return false;
    }
    const QJsonArray entities = entitiesValue.toArray();

    // Canonical unset authoring state yalnız revision=0 + entities=[] biçimidir.
    // revision=0 + entity veya revision>0 + boş entity listesi yarım/malformed
    // encoding'dir ve mevcut servis durumunu değiştirmeden reddedilir.
    if (sourceRevision == 0) {
        if (!entities.isEmpty()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("source_revision 0 iken entities boş olmalıdır");
            }
            return false;
        }
        *scope = ScopeReference{};
        return true;
    }
    if (entities.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("source_revision sıfır değilse entities boş olamaz");
        }
        return false;
    }

    ScopeReference parsed;
    parsed.sourceRevision = sourceRevision;
    parsed.entities.reserve(entities.size());
    for (qsizetype index = 0; index < entities.size(); ++index) {
        if (!entities.at(index).isObject()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("entities[%1] object olmalıdır").arg(index);
            }
            return false;
        }
        const QJsonObject entityObject = entities.at(index).toObject();
        ScopeEntityReference reference;
        if (!parseDomainToken(entityObject.value(QStringLiteral("domain")).toString(), &reference.domain)
            || !parseKindToken(entityObject.value(QStringLiteral("kind")).toString(), &reference.kind)) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("entities[%1] domain/kind surface contract'a uymuyor").arg(index);
            }
            return false;
        }

        quint64 rawId = 0;
        if (reference.domain == SelectionDomain::Geometry) {
            if (!parseUnsigned64(entityObject, QStringLiteral("geometry_entity_id"), &rawId, errorMessage)) {
                return false;
            }
            reference.geometryEntityId = static_cast<femcae::geometry::GeometryEntityId>(rawId);
            if (!parseUnsigned64(entityObject, QStringLiteral("parent_geometry_id"), &rawId, errorMessage)) {
                return false;
            }
            reference.parentGeometryId = static_cast<femcae::geometry::GeometryEntityId>(rawId);
            reference.persistentKey = entityObject.value(QStringLiteral("persistent_key")).toString();
        } else {
            if (!parseUnsigned64(entityObject, QStringLiteral("mesh_entity_id"), &rawId, errorMessage)) {
                return false;
            }
            reference.meshEntityId = static_cast<femcae::meshing::MeshEntityId>(rawId);
        }
        parsed.entities.push_back(reference);
    }

    if (!scopeIsSurfaceStructure(parsed)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("scope yapısal olarak geçersiz");
        }
        return false;
    }
    *scope = parsed;
    return true;
}

} // namespace

ContactService::ContactService(ProjectModel *project, GeometryService *geometry, MeshService *mesh,
                               QObject *parent)
    : QObject(parent), project_(project), geometry_(geometry), mesh_(mesh)
{
    if (geometry_ != nullptr) {
        connect(geometry_, &GeometryService::changed, this, &ContactService::refreshValidation);
    }
    if (mesh_ != nullptr) {
        connect(mesh_, &MeshService::changed, this, &ContactService::refreshValidation);
    }
}

const ContactDefinition *ContactService::byId(const ObjectId id) const noexcept
{
    const auto it = definitions_.constFind(id);
    return it == definitions_.constEnd() ? nullptr : &it.value();
}

int ContactService::rowOf(const ObjectId id) const noexcept
{
    return static_cast<int>(order_.indexOf(id));
}

ObjectId ContactService::createContact(const ContactDefinition &definition, const int row,
                                       const ObjectId requestedId)
{
    if (project_ == nullptr || project_->connectionsNode() == InvalidObjectId
        || !scopeIsAuthoringStructure(definition.sourceScope)
        || !scopeIsAuthoringStructure(definition.targetScope)) {
        return InvalidObjectId;
    }

    // Contact Region Source/Target seçilmeden önce de gerçek document state'tir.
    // Canonical boş scope eksik authoring state olarak saklanır; cross-domain,
    // source==target veya runtime-stale tam surface scope'lar da kaybolmadan
    // oluşturulur ve refreshNode() gerçek Error/OutOfDate durumunu gösterir.
    ContactDefinition stored = definition;
    const QString baseName = stored.name.trimmed().isEmpty()
        ? QStringLiteral("Contact Region") : stored.name.trimmed();
    stored.name = uniqueName(baseName);

    const ObjectId id = project_->addObjectAt(project_->connectionsNode(), row,
                                              ObjectType::ContactRegion, stored.name, 0,
                                              requestedId);
    if (id == InvalidObjectId) {
        return InvalidObjectId;
    }
    const int insertRow = (row < 0 || row > order_.size()) ? order_.size() : row;
    order_.insert(insertRow, id);
    definitions_.insert(id, stored);
    refreshNode(id);
    emit changed();
    return id;
}

bool ContactService::remove(const ObjectId id)
{
    if (!definitions_.contains(id)) {
        return false;
    }
    definitions_.remove(id);
    order_.removeAll(id);
    if (project_ != nullptr) {
        project_->removeObject(id);
    }
    emit changed();
    return true;
}

void ContactService::rename(const ObjectId id, const QString &name)
{
    auto it = definitions_.find(id);
    if (it == definitions_.end()) {
        return;
    }
    const QString baseName = name.trimmed().isEmpty()
        ? QStringLiteral("Contact Region") : name.trimmed();
    const QString finalName = uniqueName(baseName, id);
    if (it->name == finalName) {
        return;
    }
    it->name = finalName;
    if (project_ != nullptr) {
        project_->setName(id, finalName);
    }
    emit changed();
}

bool ContactService::replaceSourceScope(const ObjectId id, const ScopeReference &scope)
{
    auto it = definitions_.find(id);
    if (it == definitions_.end() || !scopeIsAuthoringStructure(scope)) {
        return false;
    }
    it->sourceScope = scope;
    refreshNode(id);
    emit changed();
    return true;
}

bool ContactService::replaceTargetScope(const ObjectId id, const ScopeReference &scope)
{
    auto it = definitions_.find(id);
    if (it == definitions_.end() || !scopeIsAuthoringStructure(scope)) {
        return false;
    }
    it->targetScope = scope;
    refreshNode(id);
    emit changed();
    return true;
}

void ContactService::setFormulation(const ObjectId id, const ContactFormulation formulation)
{
    auto it = definitions_.find(id);
    if (it == definitions_.end() || it->formulation == formulation) {
        return;
    }
    it->formulation = formulation;
    refreshNode(id);
    emit changed();
}

void ContactService::setSuppressed(const ObjectId id, const bool suppressed)
{
    if (!definitions_.contains(id) || project_ == nullptr || project_->object(id) == nullptr
        || project_->isSuppressed(id) == suppressed) {
        return;
    }
    project_->setSuppressed(id, suppressed);
    refreshNode(id);
    emit changed();
}

void ContactService::clear()
{
    if (definitions_.isEmpty() && order_.isEmpty()) {
        return;
    }
    if (project_ != nullptr) {
        const QVector<ObjectId> ids = order_;
        for (const ObjectId id : ids) {
            project_->removeObject(id);
        }
    }
    definitions_.clear();
    order_.clear();
    emit changed();
}

ContactValidationResult ContactService::validate(const ObjectId id) const
{
    const ContactDefinition *definition = byId(id);
    if (definition == nullptr) {
        ContactValidationResult result;
        result.error = ContactValidationError::MissingSourceScope;
        return result;
    }
    return validateDefinition(*definition);
}

void ContactService::refreshValidation()
{
    for (const ObjectId id : order_) {
        refreshNode(id);
    }
    if (!order_.isEmpty()) {
        emit changed();
    }
}

QJsonObject ContactService::toJson() const
{
    QJsonObject root;
    root[QStringLiteral("schema")] = QStringLiteral("dynamics26.contacts");
    root[QStringLiteral("schema_version")] = 1;

    QJsonArray items;
    for (const ObjectId id : order_) {
        const ContactDefinition *definition = byId(id);
        if (definition == nullptr) {
            continue;
        }
        QJsonObject item;
        item[QStringLiteral("object_id")] = QString::number(id);
        item[QStringLiteral("name")] = definition->name;
        item[QStringLiteral("formulation")] = static_cast<int>(definition->formulation);
        item[QStringLiteral("suppressed")] = project_ != nullptr && project_->isSuppressed(id);
        item[QStringLiteral("source_scope")] = scopeToJson(definition->sourceScope);
        item[QStringLiteral("target_scope")] = scopeToJson(definition->targetScope);
        items.push_back(item);
    }
    root[QStringLiteral("items")] = items;
    return root;
}

bool ContactService::fromJson(const QJsonObject &object, QString *errorMessage)
{
    const auto fail = [errorMessage](const QString &message) {
        if (errorMessage != nullptr) {
            *errorMessage = message;
        }
        return false;
    };

    if (object.value(QStringLiteral("schema")).toString() != QStringLiteral("dynamics26.contacts")) {
        return fail(QStringLiteral("Contact JSON schema tanınmıyor"));
    }
    if (object.value(QStringLiteral("schema_version")).toInt(-1) != 1) {
        return fail(QStringLiteral("Contact JSON schema_version desteklenmiyor"));
    }
    const QJsonValue itemsValue = object.value(QStringLiteral("items"));
    if (!itemsValue.isArray()) {
        return fail(QStringLiteral("Contact JSON 'items' array içermelidir"));
    }

    struct PendingContact {
        ObjectId id{InvalidObjectId};
        ContactDefinition definition;
        bool suppressed{false};
    };
    QVector<PendingContact> pending;
    QSet<ObjectId> ids;
    QSet<QString> names;

    const QJsonArray items = itemsValue.toArray();
    pending.reserve(items.size());
    for (qsizetype index = 0; index < items.size(); ++index) {
        if (!items.at(index).isObject()) {
            return fail(QStringLiteral("Contact item[%1] object olmalıdır").arg(index));
        }
        const QJsonObject item = items.at(index).toObject();
        quint64 rawId = 0;
        QString parseError;
        if (!parseUnsigned64(item, QStringLiteral("object_id"), &rawId, &parseError)
            || rawId == InvalidObjectId) {
            return fail(QStringLiteral("item[%1]: %2").arg(index).arg(
                parseError.isEmpty() ? QStringLiteral("object_id sıfır olamaz") : parseError));
        }
        const ObjectId id = static_cast<ObjectId>(rawId);
        if (ids.contains(id)) {
            return fail(QStringLiteral("item[%1]: duplicate ObjectId").arg(index));
        }
        ids.insert(id);

        const QString name = item.value(QStringLiteral("name")).toString().trimmed();
        if (name.isEmpty()) {
            return fail(QStringLiteral("item[%1]: name boş olamaz").arg(index));
        }
        const QString foldedName = name.toCaseFolded();
        if (names.contains(foldedName)) {
            return fail(QStringLiteral("item[%1]: Contact adı benzersiz olmalıdır").arg(index));
        }
        names.insert(foldedName);

        const int formulationValue = item.value(QStringLiteral("formulation")).toInt(-1);
        if (formulationValue != static_cast<int>(ContactFormulation::Bonded)) {
            return fail(QStringLiteral("item[%1]: formulation desteklenmiyor").arg(index));
        }
        if (!item.value(QStringLiteral("source_scope")).isObject()
            || !item.value(QStringLiteral("target_scope")).isObject()) {
            return fail(QStringLiteral("item[%1]: source_scope/target_scope object olmalıdır").arg(index));
        }

        PendingContact record;
        record.id = id;
        record.definition.name = name;
        record.definition.formulation = ContactFormulation::Bonded;
        record.suppressed = item.value(QStringLiteral("suppressed")).toBool(false);
        parseError.clear();
        if (!scopeFromJson(item.value(QStringLiteral("source_scope")).toObject(),
                           &record.definition.sourceScope, &parseError)) {
            return fail(QStringLiteral("item[%1].source_scope: %2").arg(index).arg(parseError));
        }
        parseError.clear();
        if (!scopeFromJson(item.value(QStringLiteral("target_scope")).toObject(),
                           &record.definition.targetScope, &parseError)) {
            return fail(QStringLiteral("item[%1].target_scope: %2").arg(index).arg(parseError));
        }

        // Geometry revision runtime guard'dır. Yalnız stabil CAD identity
        // (entity id + parent + persistentKey) current document ile birebir
        // eşleşirse project-load sırasında kontrollü şekilde current revision'a
        // taşınır. Mesh generation için otomatik rebind YOKTUR. Canonical boş
        // authoring scope rebind edilmez; incomplete state aynen korunur.
        if (geometry_ != nullptr) {
            if (!record.definition.sourceScope.entities.isEmpty()
                && record.definition.sourceScope.entities.front().domain == SelectionDomain::Geometry) {
                ScopeReference rebound = record.definition.sourceScope;
                if (rebindLoadedGeometryScopeReference(rebound, geometry_->document())) {
                    record.definition.sourceScope = rebound;
                }
            }
            if (!record.definition.targetScope.entities.isEmpty()
                && record.definition.targetScope.entities.front().domain == SelectionDomain::Geometry) {
                ScopeReference rebound = record.definition.targetScope;
                if (rebindLoadedGeometryScopeReference(rebound, geometry_->document())) {
                    record.definition.targetScope = rebound;
                }
            }
        }
        pending.push_back(record);
    }

    if (project_ == nullptr || project_->connectionsNode() == InvalidObjectId) {
        return fail(QStringLiteral("ProjectModel Connections container hazır değil"));
    }
    for (const PendingContact &record : pending) {
        const ProjectObject *existing = project_->object(record.id);
        if (existing != nullptr && !definitions_.contains(record.id)) {
            return fail(QStringLiteral("ObjectId %1 mevcut başka bir ProjectObject ile çakışıyor")
                            .arg(record.id));
        }
    }

    clear();
    for (const PendingContact &record : pending) {
        const ObjectId id = createContact(record.definition, -1, record.id);
        if (id == InvalidObjectId) {
            return fail(QStringLiteral("ObjectId %1 Contact restore edilemedi").arg(record.id));
        }
        if (record.suppressed) {
            setSuppressed(id, true);
        }
    }
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }
    return true;
}

QString ContactService::uniqueName(const QString &base, const ObjectId excludingId) const
{
    const auto isUsed = [this, excludingId](const QString &candidate) {
        for (auto it = definitions_.constBegin(); it != definitions_.constEnd(); ++it) {
            if (it.key() != excludingId && it->name.compare(candidate, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
        return false;
    };
    if (!isUsed(base)) {
        return base;
    }
    for (int suffix = 2; ; ++suffix) {
        const QString candidate = QStringLiteral("%1 %2").arg(base).arg(suffix);
        if (!isUsed(candidate)) {
            return candidate;
        }
    }
}

ContactValidationResult ContactService::validateDefinition(const ContactDefinition &definition) const
{
    ContactValidationResult result;
    if (definition.sourceScope.entities.isEmpty()) {
        result.error = ContactValidationError::MissingSourceScope;
        return result;
    }
    if (definition.targetScope.entities.isEmpty()) {
        result.error = ContactValidationError::MissingTargetScope;
        return result;
    }
    if (!scopeIsSurfaceStructure(definition.sourceScope)) {
        result.error = ContactValidationError::UnsupportedSourceSurface;
        return result;
    }
    if (!scopeIsSurfaceStructure(definition.targetScope)) {
        result.error = ContactValidationError::UnsupportedTargetSurface;
        return result;
    }
    if (definition.sourceScope.entities.front().domain != definition.targetScope.entities.front().domain) {
        result.error = ContactValidationError::MixedDomains;
        return result;
    }
    if (sameSurfaceIdentity(definition.sourceScope, definition.targetScope)) {
        result.error = ContactValidationError::IdenticalSourceAndTarget;
        return result;
    }

    const SelectionDomain domain = definition.sourceScope.entities.front().domain;
    if (domain == SelectionDomain::Geometry) {
        result.sourceScopeError = geometry_ != nullptr
            ? validateGeometryScopeReference(definition.sourceScope, geometry_->document())
            : ScopeReferenceValidationError::UnsupportedDomain;
        if (result.sourceScopeError != ScopeReferenceValidationError::None) {
            result.error = ContactValidationError::SourceScopeInvalid;
            return result;
        }
        result.targetScopeError = geometry_ != nullptr
            ? validateGeometryScopeReference(definition.targetScope, geometry_->document())
            : ScopeReferenceValidationError::UnsupportedDomain;
    } else if (domain == SelectionDomain::Mesh) {
        result.sourceScopeError = mesh_ != nullptr
            ? validateMeshScopeReference(definition.sourceScope, mesh_->mesh(), mesh_->generation())
            : ScopeReferenceValidationError::UnsupportedDomain;
        if (result.sourceScopeError != ScopeReferenceValidationError::None) {
            result.error = ContactValidationError::SourceScopeInvalid;
            return result;
        }
        result.targetScopeError = mesh_ != nullptr
            ? validateMeshScopeReference(definition.targetScope, mesh_->mesh(), mesh_->generation())
            : ScopeReferenceValidationError::UnsupportedDomain;
    } else {
        result.error = ContactValidationError::MixedDomains;
        return result;
    }

    if (result.targetScopeError != ScopeReferenceValidationError::None) {
        result.error = ContactValidationError::TargetScopeInvalid;
        return result;
    }
    return result;
}

void ContactService::refreshNode(const ObjectId id)
{
    if (project_ == nullptr || project_->object(id) == nullptr) {
        return;
    }
    if (project_->isSuppressed(id)) {
        project_->setState(id, ObjectState::Suppressed, QStringLiteral("Bastırıldı"));
        return;
    }
    const ContactValidationResult result = validate(id);
    project_->setState(id, stateForValidation(result), validationText(result));
}

} // namespace d26
