#include "ProjectTreeModel.h"

#include "CaeIcons.h"

#include <QApplication>
#include <QCoreApplication>
#include <QPalette>

namespace d26 {
namespace {

QIcon iconForObjectType(const ObjectType type, const QColor &tint)
{
    // Alpha.3.6 Named Selection tipleri ObjectType'a eklendiğinde eski
    // object-glyph switch'inde henüz karşılıkları yoktu; Navigator'da boş bir
    // ikon alanı görünüyordu. Saved scope semantiği zaten komut ikon setinde
    // tanımlı olduğundan aynı vektör glifi kullanılır. Böylece Light/Dark tint
    // davranışı ve tek ikon kaynağı korunur, generic klasör ikonu eklenmez.
    if (type == ObjectType::NamedSelectionsFolder || type == ObjectType::NamedSelection) {
        return CaeIcons::forCommand(CommandGlyph::NamedSelection, tint);
    }
    return CaeIcons::forType(type, tint);
}

} // namespace

ProjectTreeModel::ProjectTreeModel(ProjectModel *project, QObject *parent)
    : QAbstractItemModel(parent), project_(project)
{
    connect(project_, &ProjectModel::aboutToRestructure, this, [this] { beginResetModel(); });
    connect(project_, &ProjectModel::restructured, this, [this] { endResetModel(); });
    connect(project_, &ProjectModel::objectChanged, this, [this](const ObjectId id) {
        const QModelIndex changed = indexForObject(id);
        if (changed.isValid()) {
            emit dataChanged(changed, changed);
        }
    });
}

QModelIndex ProjectTreeModel::index(const int row, const int column, const QModelIndex &parent) const
{
    if (column != 0 || row < 0) {
        return {};
    }
    const ObjectId parentId = objectForIndex(parent);
    const QVector<ObjectId> &children = project_->childrenOf(parentId);
    if (row >= children.size()) {
        return {};
    }
    return createIndex(row, column, static_cast<quintptr>(children.at(row)));
}

QModelIndex ProjectTreeModel::parent(const QModelIndex &child) const
{
    const ObjectId id = objectForIndex(child);
    if (id == InvalidObjectId) {
        return {};
    }
    const ObjectId parentId = project_->parentOf(id);
    if (parentId == InvalidObjectId) {
        return {};
    }
    return indexForObject(parentId);
}

int ProjectTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) {
        return 0;
    }
    return static_cast<int>(project_->childrenOf(objectForIndex(parent)).size());
}

int ProjectTreeModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant ProjectTreeModel::data(const QModelIndex &index, const int role) const
{
    const ObjectId id = objectForIndex(index);
    const ProjectObject *object = project_->object(id);
    if (object == nullptr) {
        return {};
    }
    switch (role) {
    case Qt::DisplayRole:
        return object->name;
    case Qt::DecorationRole:
        return iconForObjectType(object->type, qApp->palette().color(QPalette::Text));
    case Qt::ToolTipRole: {
        QStringList lines;
        lines << object->name;
        if (object->suppressed) {
            lines << QCoreApplication::translate("d26", "Bastırıldı — çözüme katılmıyor");
        }
        if (!object->statusText.isEmpty()) {
            lines << object->statusText;
        }
        return lines.join(QLatin1Char('\n'));
    }
    case ObjectIdRole:
        return QVariant::fromValue<qulonglong>(object->id);
    case ObjectTypeRole:
        return static_cast<int>(object->type);
    case ObjectStateRole:
        return static_cast<int>(object->state);
    case StatusTextRole:
        return object->statusText;
    case SuppressedRole:
        return object->suppressed;
    case EffectivelySuppressedRole:
        return project_->isEffectivelySuppressed(id);
    default:
        return {};
    }
}

bool ProjectTreeModel::setData(const QModelIndex &index, const QVariant &value, const int role)
{
    if (role != Qt::EditRole) {
        return false;
    }
    const ObjectId id = objectForIndex(index);
    const QString name = value.toString().trimmed();
    const ProjectObject *object = project_->object(id);
    if (object == nullptr || name.isEmpty() || name == object->name) {
        return false;
    }
    emit renameRequested(id, name);
    return true;
}

Qt::ItemFlags ProjectTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (supportsRename(project_->typeOf(objectForIndex(index)))) {
        flags |= Qt::ItemIsEditable;
    }
    return flags;
}

QModelIndex ProjectTreeModel::indexForObject(const ObjectId id) const
{
    if (id == InvalidObjectId || project_->object(id) == nullptr) {
        return {};
    }
    const int row = project_->rowOf(id);
    if (row < 0) {
        return {};
    }
    return createIndex(row, 0, static_cast<quintptr>(id));
}

ObjectId ProjectTreeModel::objectForIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return InvalidObjectId;
    }
    return static_cast<ObjectId>(index.internalId());
}

void ProjectTreeModel::refreshDecorations()
{
    beginResetModel();
    endResetModel();
}

} // namespace d26
