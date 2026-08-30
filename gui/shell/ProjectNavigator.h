#pragma once

// Sol panel — model ağacı.
//
// QTreeWidget değil, QTreeView + ProjectTreeModel kullanır: seçim ve komut
// bağlamı gerçek nesne kimliği üzerinden çözülür.

#include "../core/ProjectTreeModel.h"

#include <QFrame>
#include <QStyledItemDelegate>

class QTreeView;
class QLabel;

namespace d26 {

// Nesne durumunu (hazır / güncel / hata) satırın sağında küçük bir rozet
// olarak çizer. Durum rengi mühendislik anlamıdır, tema rengi değildir.
class NavigatorDelegate final : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    [[nodiscard]] QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const override;
};

class ProjectNavigator final : public QFrame
{
    Q_OBJECT
public:
    ProjectNavigator(ProjectModel *project, QWidget *parent = nullptr);

    void selectObject(ObjectId id);
    [[nodiscard]] ObjectId selectedObject() const;
    void expandAll();
    void collapseAll();
    void refreshDecorations();
    // Ağaç içi yeniden adlandırma başlatır (F2 / context menu).
    void beginInlineRename(ObjectId id);

signals:
    void objectSelected(ObjectId id);
    // Nesne türüne duyarlı bağlam menüsü kabuk tarafından kurulur.
    void contextMenuRequested(ObjectId id, const QPoint &globalPosition);
    void renameCommitted(ObjectId id, const QString &newName);

private:
    ProjectTreeModel *model_{nullptr};
    QTreeView *view_{nullptr};
};

} // namespace d26
