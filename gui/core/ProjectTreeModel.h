#pragma once

// ProjectModel nesne grafiğini QTreeView'e sunan adaptör.
//
// QTreeWidget yerine QAbstractItemModel kullanılır: düğüm kimliği görünen
// metinden bağımsızdır, internalId doğrudan ObjectId taşır.

#include "ProjectModel.h"

#include <QAbstractItemModel>

namespace d26 {

class ProjectTreeModel final : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Roles {
        ObjectIdRole = Qt::UserRole + 1,
        ObjectTypeRole,
        ObjectStateRole,
        StatusTextRole,
        SuppressedRole,
        EffectivelySuppressedRole
    };

    explicit ProjectTreeModel(ProjectModel *project, QObject *parent = nullptr);

    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex &child) const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;

    [[nodiscard]] QModelIndex indexForObject(ObjectId id) const;
    [[nodiscard]] ObjectId objectForIndex(const QModelIndex &index) const;

    // İkonlar palet rengiyle üretildiğinden görünüm değişiminde yenilenmelidir.
    void refreshDecorations();

signals:
    // Ağaç içi düzenleme adı DOĞRUDAN yazmaz; kabuk bunu undoable bir
    // RenameObjectCommand'e çevirir.
    void renameRequested(ObjectId id, const QString &newName);

private:
    ProjectModel *project_;
};

} // namespace d26
