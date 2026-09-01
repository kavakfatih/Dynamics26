#include "NamedSelectionService.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QSet>

namespace d26 {
namespace {

QString validationText(const ScopeReferenceValidationError error)
{
    switch (error) {
    case ScopeReferenceValidationError::None:
        return QStringLiteral("Scope geçerli");
    case ScopeReferenceValidationError::EmptyScope:
        return QStringLiteral("Scope boş");
    case ScopeReferenceValidationError::StaleGeometryRevision:
        return QStringLiteral("CAD Geometry revision değişti; scope güncel değil");
    case ScopeReferenceValidationError::StaleMeshGeneration:
        return QStringLiteral("FEM Mesh yeniden üretildi; scope güncel değil");
    case ScopeReferenceValidationError::UnsupportedDomain:
        return QStringLiteral("Scope domain desteklenmiyor veya domain'ler karışık");
    case ScopeReferenceValidationError::UnsupportedKind:
        return QStringLiteral("Scope entity kind desteklenmiyor");
    case ScopeReferenceValidationError::MissingGeometryEntity:
        return QStringLiteral("CAD topology entity bulunamadı");
    case ScopeReferenceValidationError::MissingMeshEntity:
        return QStringLiteral("FEM Mesh entity bulunamadı");
    case ScopeReferenceValidationError::GeometryKindMismatch:
        return QStringLiteral("CAD topology kind eşleşmiyor");
    case ScopeReferenceValidationError::ParentBodyMismatch:
        return QStringLiteral("CAD topology parent Body eşleşmiyor");
    case ScopeReferenceValidationError::MissingPersistentKey:
        return QStringLiteral("CAD persistentKey eksik");
    case ScopeReferenceValidationError::PersistentKeyMismatch:
        return QStringLiteral("CAD persistentKey eşleşmiyor");
    }
    return QStringLiteral("Scope doğrulanamadı");
}

ObjectState objectStateForValidation(const ScopeReferenceValidationError error)
{
    if (error == ScopeReferenceValidationError::None) {
        return ObjectState::UpToDate;
    }
    if (error == ScopeReferenceValidationError::StaleGeometryRevision
        || error == ScopeReferenceValidationError::StaleMeshGeneration) {
        return ObjectState::OutOfDate;
    }
    return ObjectState::Error;
}

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
    case SelectionKind::Object: return QStringLiteral("object");
    case SelectionKind::Body: return QStringLiteral("body");
    case SelectionKind::Face: return QStringLiteral("face");
    case SelectionKind::Edge: return QStringLiteral("edge");
    case SelectionKind::Vertex: return QStringLiteral("vertex");
    case SelectionKind::Node: return QStringLiteral("node");
    case SelectionKind::Element: return QStringLiteral("element");
    case SelectionKind::Facet: return QStringLiteral("facet");
    }
    return {};
}

bool parseKindToken(const QString &token, SelectionKind *kind)
{
    if (token == QStringLiteral("body")) {
        *kind = SelectionKind::Body;
        return true;
    }
    if (token == QStringLiteral("face")) {
        *kind = SelectionKind::Face;
        return true;
    }
    if (token == QStringLiteral("edge")) {
        *kind = SelectionKind::Edge;
        return true;
    }
    if (token == QStringLiteral("vertex")) {
        *kind = SelectionKind::Vertex;
        return true;
    }
    if (token == QStringLiteral("node")) {
        *kind = SelectionKind::Node;
        return true;
    }
    if (token == QStringLiteral("element")) {
        *kind = SelectionKind::Element;
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

bool scopeStructureIsValid(const ScopeReference &scope)
{
    if (scope.sourceRevision == 0 || scope.entities.isEmpty()) {
        return false;
    }

    const SelectionDomain domain = scope.entities.front().domain;
    if (domain != SelectionDomain::Geometry && domain != SelectionDomain::Mesh) {
        return false;
    }

    for (const ScopeEntityReference &reference : scope.entities) {
        if (reference.domain != domain) {
            return false;
        }
        if (domain == SelectionDomain::Geometry) {
            if (!geometryEntityKindForSelectionKind(reference.kind).has_value()
                || reference.geometryEntityId == femcae::geometry::InvalidGeometryId
                || reference.persistentKey.isEmpty()) {
                return false;
            }
            if (geometrySelectionKindHasBodyParent(reference.kind)
                && reference.parentGeometryId == femcae::geometry::InvalidGeometryId) {
                return false;
            }
        } else {
            if (!isMeshSelectionKind(reference.kind)
                || reference.meshEntityId == femcae::meshing::InvalidMeshId) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

NamedSelectionService::NamedSelectionService(ProjectModel *project, GeometryService *geometry,
                                             MeshService *mesh, QObject *parent)
    : QObject(parent), project_(project), geometry_(geometry), mesh_(mesh)
{
    // Persistent scope state kaynak servislerin yaşam döngüsünü kendisi izler.
    // UI veya consumer'ın refreshValidation() çağırmasını beklemek stale rozeti
    // geciktirir ve aynı servisin farklı consumer'larda farklı davranmasına yol
    // açar. Geometry revision / Mesh generation değişimi bu nedenle doğrudan
    // servis seviyesinde yeniden doğrulama tetikler; otomatik rebind YAPILMAZ.
    if (geometry_ != nullptr) {
        connect(geometry_, &GeometryService::changed,
                this, &NamedSelectionService::refreshValidation);
    }
    if (mesh_ != nullptr) {
        connect(mesh_, &MeshService::changed,
                this, &NamedSelectionService::refreshValidation);
    }
}

const NamedSelectionDefinition *NamedSelectionService::byId(const ObjectId id) const noexcept
{
    const auto it = definitions_.constFind(id);
    return it == definitions_.constEnd() ? nullptr : &it.value();
}

int NamedSelectionService::rowOf(const ObjectId id) const noexcept
{
    return static_cast<int>(order_.indexOf(id));
}

NamedSelectionCreateResult
NamedSelectionService::createFromSelection(const QVector<SelectionItem> &items,
                                           const QString &requestedName)
{
    NamedSelectionCreateResult result;
    if (items.isEmpty()) {
        result.buildError = ScopeReferenceBuildError::EmptySelection;
        return result;
    }

    ScopeReferenceBuildResult build;
    switch (items.front().domain) {
    case SelectionDomain::Geometry:
        if (geometry_ == nullptr) {
            result.buildError = ScopeReferenceBuildError::UnsupportedDomain;
            return result;
        }
        build = buildGeometryScopeReference(items, geometry_->document());
        break;
    case SelectionDomain::Mesh:
        if (mesh_ == nullptr) {
            result.buildError = ScopeReferenceBuildError::UnsupportedDomain;
            return result;
        }
        build = buildMeshScopeReference(items, mesh_->mesh(), mesh_->generation());
        break;
    case SelectionDomain::ProjectObject:
        result.buildError = ScopeReferenceBuildError::UnsupportedDomain;
        return result;
    }

    result.buildError = build.error;
    if (!build.success()) {
        return result;
    }

    NamedSelectionDefinition definition;
    definition.name = requestedName;
    definition.scope = build.scope;
    result.id = createWithScope(definition);
    if (result.id == InvalidObjectId) {
        result.buildError = ScopeReferenceBuildError::UnsupportedDomain;
    }
    return result;
}

ObjectId NamedSelectionService::createWithScope(const NamedSelectionDefinition &definition,
                                                const int row,
                                                const ObjectId requestedId)
{
    if (project_ == nullptr || project_->namedSelectionsNode() == InvalidObjectId
        || !scopeStructureIsValid(definition.scope)) {
        return InvalidObjectId;
    }

    NamedSelectionDefinition stored = definition;
    const QString baseName = stored.name.trimmed().isEmpty()
        ? QStringLiteral("Named Selection") : stored.name.trimmed();
    stored.name = uniqueName(baseName);

    const ObjectId id = project_->addObjectAt(project_->namedSelectionsNode(), row,
                                              ObjectType::NamedSelection, stored.name, 0,
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

bool NamedSelectionService::remove(const ObjectId id)
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

void NamedSelectionService::rename(const ObjectId id, const QString &name)
{
    auto it = definitions_.find(id);
    if (it == definitions_.end()) {
        return;
    }

    const QString baseName = name.trimmed().isEmpty()
        ? QStringLiteral("Named Selection") : name.trimmed();
    QString finalName;
    if (it->name.compare(baseName, Qt::CaseInsensitive) == 0) {
        finalName = baseName;
    } else {
        finalName = uniqueName(baseName);
    }
    if (it->name == finalName) {
        return;
    }

    it->name = finalName;
    if (project_ != nullptr) {
        project_->setName(id, finalName);
    }
    emit changed();
}

void NamedSelectionService::replaceScope(const ObjectId id, const ScopeReference &scope)
{
    auto it = definitions_.find(id);
    if (it == definitions_.end() || !scopeStructureIsValid(scope)) {
        return;
    }
    it->scope = scope;
    refreshNode(id);
    emit changed();
}

void NamedSelectionService::clear()
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

ScopeReferenceValidationError NamedSelectionService::validate(const ObjectId id) const
{
    const NamedSelectionDefinition *definition = byId(id);
    return definition != nullptr
        ? validateScope(definition->scope)
        : ScopeReferenceValidationError::EmptyScope;
}

void NamedSelectionService::refreshValidation()
{
    for (const ObjectId id : order_) {
        refreshNode(id);
    }
    if (!order_.isEmpty()) {
        emit changed();
    }
}

QJsonObject NamedSelectionService::toJson() const
{
    QJsonObject root;
    root[QStringLiteral("schema")] = QStringLiteral("dynamics26.named_selections");
    root[QStringLiteral("schema_version")] = 1;

    QJsonArray items;
    for (const ObjectId id : order_) {
        const NamedSelectionDefinition *definition = byId(id);
        if (definition == nullptr) {
            continue;
        }

        QJsonObject item;
        item[QStringLiteral("object_id")] = QString::number(id);
        item[QStringLiteral("name")] = definition->name;
        // source_revision runtime stale guard'dır. Diagnostic olarak yazılır;
        // project-load sırasında CAD stabil identity doğrulanırsa current
        // GeometryDocument revision'ına kontrollü olarak yeniden bağlanır.
        item[QStringLiteral("source_revision")] = QString::number(definition->scope.sourceRevision);

        QJsonArray entities;
        for (const ScopeEntityReference &reference : definition->scope.entities) {
            QJsonObject entity;
            entity[QStringLiteral("domain")] = domainToken(reference.domain);
            entity[QStringLiteral("kind")] = kindToken(reference.kind);
            if (reference.domain == SelectionDomain::Geometry) {
                entity[QStringLiteral("geometry_entity_id")] = QString::number(
                    static_cast<qulonglong>(reference.geometryEntityId));
                entity[QStringLiteral("parent_geometry_id")] = QString::number(
                    static_cast<qulonglong>(reference.parentGeometryId));
                entity[QStringLiteral("persistent_key")] = reference.persistentKey;
            } else if (reference.domain == SelectionDomain::Mesh) {
                entity[QStringLiteral("mesh_entity_id")] = QString::number(
                    static_cast<qulonglong>(reference.meshEntityId));
            }
            entities.push_back(entity);
        }
        item[QStringLiteral("entities")] = entities;
        items.push_back(item);
    }
    root[QStringLiteral("items")] = items;
    return root;
}

bool NamedSelectionService::fromJson(const QJsonObject &object, QString *errorMessage)
{
    const auto fail = [errorMessage](const QString &message) {
        if (errorMessage != nullptr) {
            *errorMessage = message;
        }
        return false;
    };

    if (object.value(QStringLiteral("schema")).toString()
        != QStringLiteral("dynamics26.named_selections")) {
        return fail(QStringLiteral("Named Selection JSON schema tanınmıyor"));
    }
    if (object.value(QStringLiteral("schema_version")).toInt(-1) != 1) {
        return fail(QStringLiteral("Named Selection JSON schema_version desteklenmiyor"));
    }

    const QJsonValue itemsValue = object.value(QStringLiteral("items"));
    if (!itemsValue.isArray()) {
        return fail(QStringLiteral("Named Selection JSON 'items' array içermelidir"));
    }

    struct PendingRecord {
        ObjectId id{InvalidObjectId};
        NamedSelectionDefinition definition;
    };
    QVector<PendingRecord> pending;
    QSet<ObjectId> ids;
    QSet<QString> names;

    const QJsonArray items = itemsValue.toArray();
    pending.reserve(items.size());
    for (qsizetype itemIndex = 0; itemIndex < items.size(); ++itemIndex) {
        if (!items.at(itemIndex).isObject()) {
            return fail(QStringLiteral("Named Selection item[%1] object olmalıdır").arg(itemIndex));
        }
        const QJsonObject item = items.at(itemIndex).toObject();

        quint64 rawObjectId = 0;
        QString parseError;
        if (!parseUnsigned64(item, QStringLiteral("object_id"), &rawObjectId, &parseError)
            || rawObjectId == InvalidObjectId) {
            return fail(QStringLiteral("item[%1]: %2").arg(itemIndex).arg(
                parseError.isEmpty() ? QStringLiteral("object_id sıfır olamaz") : parseError));
        }
        const ObjectId objectId = static_cast<ObjectId>(rawObjectId);
        if (ids.contains(objectId)) {
            return fail(QStringLiteral("item[%1]: duplicate ObjectId").arg(itemIndex));
        }
        ids.insert(objectId);

        const QJsonValue nameValue = item.value(QStringLiteral("name"));
        if (!nameValue.isString() || nameValue.toString().trimmed().isEmpty()) {
            return fail(QStringLiteral("item[%1]: name boş olamaz").arg(itemIndex));
        }
        const QString name = nameValue.toString().trimmed();
        const QString foldedName = name.toCaseFolded();
        if (names.contains(foldedName)) {
            return fail(QStringLiteral("item[%1]: Named Selection adı benzersiz olmalıdır").arg(itemIndex));
        }
        names.insert(foldedName);

        quint64 sourceRevision = 0;
        parseError.clear();
        if (!parseUnsigned64(item, QStringLiteral("source_revision"), &sourceRevision, &parseError)
            || sourceRevision == 0) {
            return fail(QStringLiteral("item[%1]: %2").arg(itemIndex).arg(
                parseError.isEmpty() ? QStringLiteral("source_revision sıfır olamaz") : parseError));
        }

        const QJsonValue entitiesValue = item.value(QStringLiteral("entities"));
        if (!entitiesValue.isArray() || entitiesValue.toArray().isEmpty()) {
            return fail(QStringLiteral("item[%1]: entities boş olmayan array olmalıdır").arg(itemIndex));
        }

        ScopeReference scope;
        scope.sourceRevision = sourceRevision;
        const QJsonArray entities = entitiesValue.toArray();
        scope.entities.reserve(entities.size());
        for (qsizetype entityIndex = 0; entityIndex < entities.size(); ++entityIndex) {
            if (!entities.at(entityIndex).isObject()) {
                return fail(QStringLiteral("item[%1].entities[%2] object olmalıdır")
                                .arg(itemIndex).arg(entityIndex));
            }
            const QJsonObject entityObject = entities.at(entityIndex).toObject();
            ScopeEntityReference reference;
            if (!parseDomainToken(entityObject.value(QStringLiteral("domain")).toString(),
                                  &reference.domain)) {
                return fail(QStringLiteral("item[%1].entities[%2]: domain desteklenmiyor")
                                .arg(itemIndex).arg(entityIndex));
            }
            if (!parseKindToken(entityObject.value(QStringLiteral("kind")).toString(),
                                &reference.kind)) {
                return fail(QStringLiteral("item[%1].entities[%2]: kind desteklenmiyor")
                                .arg(itemIndex).arg(entityIndex));
            }

            quint64 rawId = 0;
            if (reference.domain == SelectionDomain::Geometry) {
                parseError.clear();
                if (!parseUnsigned64(entityObject, QStringLiteral("geometry_entity_id"),
                                     &rawId, &parseError)) {
                    return fail(QStringLiteral("item[%1].entities[%2]: %3")
                                    .arg(itemIndex).arg(entityIndex).arg(parseError));
                }
                reference.geometryEntityId = static_cast<femcae::geometry::GeometryEntityId>(rawId);

                parseError.clear();
                if (!parseUnsigned64(entityObject, QStringLiteral("parent_geometry_id"),
                                     &rawId, &parseError)) {
                    return fail(QStringLiteral("item[%1].entities[%2]: %3")
                                    .arg(itemIndex).arg(entityIndex).arg(parseError));
                }
                reference.parentGeometryId = static_cast<femcae::geometry::GeometryEntityId>(rawId);
                reference.persistentKey = entityObject.value(QStringLiteral("persistent_key")).toString();
            } else {
                parseError.clear();
                if (!parseUnsigned64(entityObject, QStringLiteral("mesh_entity_id"),
                                     &rawId, &parseError)) {
                    return fail(QStringLiteral("item[%1].entities[%2]: %3")
                                    .arg(itemIndex).arg(entityIndex).arg(parseError));
                }
                reference.meshEntityId = static_cast<femcae::meshing::MeshEntityId>(rawId);
            }
            scope.entities.push_back(reference);
        }

        if (!scopeStructureIsValid(scope)) {
            return fail(QStringLiteral("item[%1]: scope yapısal olarak geçersiz").arg(itemIndex));
        }

        // sourceRevision oturum-içi stale guard'dır; permanent CAD identity
        // değildir. Project load sırasında yalnız GeometryEntityId + kind +
        // parent Body + persistentKey current document ile birebir uyuşuyorsa
        // revision guard current oturuma taşınır. Stable identity doğrulanamazsa
        // saved revision korunur ve normal validation stale/error durumunu gösterir.
        // Mesh scope için böyle bir rebind YOKTUR: generation persistent değildir.
        if (scope.entities.front().domain == SelectionDomain::Geometry && geometry_ != nullptr) {
            ScopeReference rebound = scope;
            if (rebindLoadedGeometryScopeReference(rebound, geometry_->document())) {
                scope = rebound;
            }
        }

        PendingRecord record;
        record.id = objectId;
        record.definition.name = name;
        record.definition.scope = scope;
        pending.push_back(record);
    }

    if (project_ == nullptr || project_->namedSelectionsNode() == InvalidObjectId) {
        return fail(QStringLiteral("ProjectModel Named Selections container hazır değil"));
    }
    for (const PendingRecord &record : pending) {
        const ProjectObject *existing = project_->object(record.id);
        if (existing != nullptr && !definitions_.contains(record.id)) {
            return fail(QStringLiteral("ObjectId %1 mevcut başka bir ProjectObject ile çakışıyor")
                            .arg(record.id));
        }
    }

    clear();
    for (const PendingRecord &record : pending) {
        if (createWithScope(record.definition, -1, record.id) == InvalidObjectId) {
            return fail(QStringLiteral("ObjectId %1 restore edilemedi").arg(record.id));
        }
    }

    if (errorMessage != nullptr) {
        errorMessage->clear();
    }
    return true;
}

QString NamedSelectionService::uniqueName(const QString &base) const
{
    const auto isUsed = [this](const QString &candidate) {
        for (auto it = definitions_.constBegin(); it != definitions_.constEnd(); ++it) {
            if (it->name.compare(candidate, Qt::CaseInsensitive) == 0) {
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

ScopeReferenceValidationError
NamedSelectionService::validateScope(const ScopeReference &scope) const
{
    if (scope.entities.isEmpty()) {
        return ScopeReferenceValidationError::EmptyScope;
    }

    const SelectionDomain domain = scope.entities.front().domain;
    if (domain == SelectionDomain::Geometry) {
        return geometry_ != nullptr
            ? validateGeometryScopeReference(scope, geometry_->document())
            : ScopeReferenceValidationError::UnsupportedDomain;
    }
    if (domain == SelectionDomain::Mesh) {
        return mesh_ != nullptr
            ? validateMeshScopeReference(scope, mesh_->mesh(), mesh_->generation())
            : ScopeReferenceValidationError::UnsupportedDomain;
    }
    return ScopeReferenceValidationError::UnsupportedDomain;
}

void NamedSelectionService::refreshNode(const ObjectId id)
{
    if (project_ == nullptr || project_->object(id) == nullptr) {
        return;
    }
    const ScopeReferenceValidationError error = validate(id);
    project_->setState(id, objectStateForValidation(error), validationText(error));
}

} // namespace d26
