#include "ProjectModel.h"

namespace d26 {

ProjectModel::ProjectModel(QObject *parent) : QObject(parent)
{
    resetToEmptyProject();
}

const ProjectObject *ProjectModel::object(const ObjectId id) const noexcept
{
    const auto it = objects_.constFind(id);
    return it == objects_.constEnd() ? nullptr : &it.value();
}

ObjectType ProjectModel::typeOf(const ObjectId id) const noexcept
{
    const auto *item = object(id);
    return item != nullptr ? item->type : ObjectType::Model;
}

ObjectId ProjectModel::parentOf(const ObjectId id) const noexcept
{
    const auto *item = object(id);
    return item != nullptr ? item->parent : InvalidObjectId;
}

const QVector<ObjectId> &ProjectModel::childrenOf(const ObjectId id) const
{
    if (id == InvalidObjectId) {
        return roots_;
    }
    const auto it = objects_.constFind(id);
    return it == objects_.constEnd() ? empty_ : it.value().children;
}

int ProjectModel::rowOf(const ObjectId id) const
{
    const auto *item = object(id);
    if (item == nullptr) {
        return -1;
    }
    const QVector<ObjectId> &siblings = childrenOf(item->parent);
    return static_cast<int>(siblings.indexOf(id));
}

void ProjectModel::resetToEmptyProject()
{
    emit aboutToRestructure();
    objects_.clear();
    roots_.clear();
    nextId_ = 1;

    const auto create = [this](const ObjectId parent, const ObjectType type, const QString &name) {
        ProjectObject object;
        object.id = nextId_++;
        object.parent = parent;
        object.type = type;
        object.name = name;
        objects_.insert(object.id, object);
        if (parent == InvalidObjectId) {
            roots_.push_back(object.id);
        } else {
            objects_[parent].children.push_back(object.id);
        }
        return object.id;
    };

    project_ = create(InvalidObjectId, ObjectType::Project, QStringLiteral("Project"));
    model_ = create(project_, ObjectType::Model, QStringLiteral("Model"));
    geometry_ = create(model_, ObjectType::GeometryFolder, QStringLiteral("Geometry"));
    materials_ = create(model_, ObjectType::MaterialsFolder, QStringLiteral("Materials"));
    sections_ = create(model_, ObjectType::SectionsFolder, QStringLiteral("Sections"));
    connections_ = create(model_, ObjectType::ConnectionsFolder, QStringLiteral("Connections"));
    mesh_ = create(model_, ObjectType::Mesh, QStringLiteral("Mesh"));

    objects_[geometry_].state = ObjectState::NotReady;
    objects_[geometry_].statusText = QStringLiteral("Geometri içe aktarılmadı");
    objects_[mesh_].state = ObjectState::NotReady;
    objects_[mesh_].statusText = QStringLiteral("Mesh üretilmedi");

    emit restructured();
}

ObjectId ProjectModel::addObject(const ObjectId parent, const ObjectType type, const QString &name, const qint64 tag)
{
    return addObjectAt(parent, -1, type, name, tag, InvalidObjectId);
}

ObjectId ProjectModel::addRoot(const ObjectType type, const QString &name, const qint64 tag)
{
    return addRootAt(-1, type, name, tag, InvalidObjectId);
}

ObjectId ProjectModel::addObjectAt(const ObjectId parent, const int row, const ObjectType type, const QString &name,
                                   const qint64 tag, const ObjectId requestedId)
{
    if (!objects_.contains(parent)) {
        return InvalidObjectId;
    }
    if (requestedId != InvalidObjectId && objects_.contains(requestedId)) {
        return InvalidObjectId; // kimlik çakışması: sessizce üzerine yazma
    }
    emit aboutToRestructure();
    ProjectObject object;
    object.id = requestedId != InvalidObjectId ? requestedId : nextId_++;
    object.parent = parent;
    object.type = type;
    object.name = name;
    object.tag = tag;
    objects_.insert(object.id, object);
    QVector<ObjectId> &siblings = objects_[parent].children;
    const int index = (row < 0 || row > siblings.size()) ? siblings.size() : row;
    siblings.insert(index, object.id);
    reserveIdsUpTo(object.id);
    emit restructured();
    return object.id;
}

ObjectId ProjectModel::addRootAt(const int row, const ObjectType type, const QString &name, const qint64 tag,
                                 const ObjectId requestedId)
{
    if (requestedId != InvalidObjectId && objects_.contains(requestedId)) {
        return InvalidObjectId;
    }
    emit aboutToRestructure();
    ProjectObject object;
    object.id = requestedId != InvalidObjectId ? requestedId : nextId_++;
    object.parent = InvalidObjectId;
    object.type = type;
    object.name = name;
    object.tag = tag;
    objects_.insert(object.id, object);
    const int index = (row < 0 || row > roots_.size()) ? roots_.size() : row;
    roots_.insert(index, object.id);
    reserveIdsUpTo(object.id);
    emit restructured();
    return object.id;
}

void ProjectModel::reserveIdsUpTo(const ObjectId highestUsedId)
{
    if (highestUsedId >= nextId_) {
        nextId_ = highestUsedId + 1;
    }
}

void ProjectModel::collectSubtree(const ObjectId id, QVector<ObjectId> &out) const
{
    out.push_back(id);
    for (const ObjectId child : childrenOf(id)) {
        collectSubtree(child, out);
    }
}

void ProjectModel::detach(const ObjectId id)
{
    const auto *item = object(id);
    if (item == nullptr) {
        return;
    }
    if (item->parent == InvalidObjectId) {
        roots_.removeAll(id);
    } else if (objects_.contains(item->parent)) {
        objects_[item->parent].children.removeAll(id);
    }
}

void ProjectModel::removeObject(const ObjectId id)
{
    if (!objects_.contains(id)) {
        return;
    }
    emit aboutToRestructure();
    QVector<ObjectId> doomed;
    collectSubtree(id, doomed);
    detach(id);
    for (const ObjectId victim : doomed) {
        objects_.remove(victim);
    }
    emit restructured();
}

void ProjectModel::removeChildren(const ObjectId id)
{
    if (!objects_.contains(id) || objects_[id].children.isEmpty()) {
        return;
    }
    emit aboutToRestructure();
    QVector<ObjectId> doomed;
    for (const ObjectId child : objects_[id].children) {
        collectSubtree(child, doomed);
    }
    objects_[id].children.clear();
    for (const ObjectId victim : doomed) {
        objects_.remove(victim);
    }
    emit restructured();
}

void ProjectModel::setName(const ObjectId id, const QString &name)
{
    if (!objects_.contains(id) || objects_[id].name == name) {
        return;
    }
    objects_[id].name = name;
    emit objectChanged(id);
}

void ProjectModel::setSuppressed(const ObjectId id, const bool suppressed)
{
    if (!objects_.contains(id) || objects_[id].suppressed == suppressed) {
        return;
    }
    objects_[id].suppressed = suppressed;
    emit objectChanged(id);
}

bool ProjectModel::isSuppressed(const ObjectId id) const
{
    const auto *item = object(id);
    return item != nullptr && item->suppressed;
}

bool ProjectModel::isEffectivelySuppressed(ObjectId id) const
{
    while (id != InvalidObjectId) {
        const auto *item = object(id);
        if (item == nullptr) {
            return false;
        }
        if (item->suppressed) {
            return true;
        }
        id = item->parent;
    }
    return false;
}

void ProjectModel::setState(const ObjectId id, const ObjectState state, const QString &statusText)
{
    if (!objects_.contains(id)) {
        return;
    }
    if (objects_[id].state == state && objects_[id].statusText == statusText) {
        return;
    }
    objects_[id].state = state;
    objects_[id].statusText = statusText;
    emit objectChanged(id);
}

ObjectId ProjectModel::ancestorOfType(ObjectId id, const ObjectType type) const
{
    while (id != InvalidObjectId) {
        const auto *item = object(id);
        if (item == nullptr) {
            return InvalidObjectId;
        }
        if (item->type == type) {
            return id;
        }
        id = item->parent;
    }
    return InvalidObjectId;
}

QVector<ObjectId> ProjectModel::childrenOfType(const ObjectId parent, const ObjectType type) const
{
    QVector<ObjectId> result;
    for (const ObjectId child : childrenOf(parent)) {
        if (typeOf(child) == type) {
            result.push_back(child);
        }
    }
    return result;
}

QVector<ObjectId> ProjectModel::analyses() const
{
    QVector<ObjectId> result;
    for (const ObjectId root : roots_) {
        if (typeOf(root) == ObjectType::Analysis) {
            result.push_back(root);
        }
    }
    return result;
}

} // namespace d26
