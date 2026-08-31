#include "ProjectNavigator.h"

#include "../core/CaeIcons.h"
#include "../core/UiTheme.h"

#include <QApplication>
#include <QFont>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QStyle>
#include <QTreeView>
#include <QVBoxLayout>

namespace d26 {
namespace {
constexpr int kBadgeDiameter = 7;
constexpr int kBadgeMargin = 8;

QColor stateColor(const ObjectState state)
{
    switch (state) {
    case ObjectState::NotReady:   return ui::statusColor(ui::StatusTone::Neutral);
    case ObjectState::Ready:      return ui::statusColor(ui::StatusTone::Ready);
    case ObjectState::UpToDate:   return ui::statusColor(ui::StatusTone::UpToDate);
    case ObjectState::OutOfDate:  return ui::statusColor(ui::StatusTone::OutOfDate);
    case ObjectState::Warning:    return ui::statusColor(ui::StatusTone::Warning);
    case ObjectState::Error:      return ui::statusColor(ui::StatusTone::Error);
    case ObjectState::Suppressed: return ui::statusColor(ui::StatusTone::Suppressed);
    case ObjectState::Solving:    return ui::statusColor(ui::StatusTone::Solving);
    case ObjectState::None:       break;
    }
    return {};
}
} // namespace

void NavigatorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Status rozeti için metin/ikon alanını daraltıyoruz; fakat macOS native
    // selection arka planı tüm satır boyunca devam etmelidir. Eski kod option
    // rect'ini paint'ten önce daralttığı için seçili satır sağ tarafta kesiliyor
    // ve badge beyaz/boş bir şeritte kalıyordu. Önce yalnız native selection
    // panelini tam rect'e çiz, sonra içerik için güvenli alanı kullan.
    if (option.state.testFlag(QStyle::State_Selected)) {
        QStyleOptionViewItem background(option);
        initStyleOption(&background, index);
        background.text.clear();
        background.icon = {};
        background.state &= ~QStyle::State_HasFocus;
        const QStyle *style = option.widget != nullptr ? option.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &background, painter, option.widget);
    }

    QStyleOptionViewItem adjusted(option);
    adjusted.rect.adjust(0, 0, -(kBadgeDiameter + 2 * kBadgeMargin), 0);

    // Bastırılmış nesne modelde durur ama çözüme katılmaz: soluk ve italik
    // çizilerek görsel olarak da ayrılır (silinmiş gibi görünmez).
    if (index.data(ProjectTreeModel::EffectivelySuppressedRole).toBool()) {
        QFont font = adjusted.font;
        font.setItalic(true);
        adjusted.font = font;
        painter->save();
        painter->setOpacity(0.45);
        QStyledItemDelegate::paint(painter, adjusted, index);
        painter->restore();
    } else {
        QStyledItemDelegate::paint(painter, adjusted, index);
    }

    const auto state = static_cast<ObjectState>(index.data(ProjectTreeModel::ObjectStateRole).toInt());
    const QColor color = stateColor(state);
    if (!color.isValid()) {
        return;
    }
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    const int y = option.rect.center().y() - kBadgeDiameter / 2;
    painter->drawEllipse(QRect(option.rect.right() - kBadgeDiameter - kBadgeMargin, y, kBadgeDiameter, kBadgeDiameter));
    painter->restore();
}

QWidget *NavigatorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const
{
    // Yeniden adlandırma editörü yalnız görünen metni düzenler; DisplayName
    // değişikliği kabuk tarafından RenameObjectCommand olarak uygulanır.
    QWidget *editor = QStyledItemDelegate::createEditor(parent, option, index);
    if (auto *lineEdit = qobject_cast<QLineEdit *>(editor)) {
        lineEdit->setMaxLength(120);
    }
    return editor;
}

QSize NavigatorDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    // Yoğun ama okunur satır yüksekliği: CAE ağaçlarında bilgi yoğunluğu önemlidir.
    size.setHeight(qMax(size.height(), 21));
    return size;
}

ProjectNavigator::ProjectNavigator(ProjectModel *project, QWidget *parent) : QFrame(parent)
{
    setObjectName(QStringLiteral("Dynamics26ProjectNavigator"));
    setFrameShape(QFrame::NoFrame);
    setMinimumWidth(206);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new ui::SecondaryLabel(tr("MODEL"), 0.52, 0.72, this);
    QFont headerFont = header->font();
    headerFont.setPointSizeF(qMax(9.0, headerFont.pointSizeF() - 2.0));
    headerFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    header->setFont(headerFont);
    header->setContentsMargins(10, 9, 10, 6);
    layout->addWidget(header);
    layout->addWidget(new ui::Hairline(this));

    model_ = new ProjectTreeModel(project, this);
    view_ = new QTreeView(this);
    view_->setModel(model_);
    view_->setHeaderHidden(true);
    view_->setFrameShape(QFrame::NoFrame);
    view_->setRootIsDecorated(true);
    view_->setIndentation(15);
    view_->setUniformRowHeights(true);
    view_->setExpandsOnDoubleClick(true);
    view_->setSelectionMode(QAbstractItemView::SingleSelection);
    // Düzenleme yalnız programatik olarak (F2 / Rename komutu) başlar.
    view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view_->setContextMenuPolicy(Qt::CustomContextMenu);
    view_->setItemDelegate(new NavigatorDelegate(view_));
    view_->setAccessibleName(tr("Model Ağacı"));
    view_->setIconSize(QSize(16, 16));
    layout->addWidget(view_, 1);

    connect(view_->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex &current, const QModelIndex &) {
                emit objectSelected(model_->objectForIndex(current));
            });
    connect(model_, &QAbstractItemModel::modelReset, this, [this] { view_->expandAll(); });
    connect(view_, &QWidget::customContextMenuRequested, this, [this](const QPoint &position) {
        const QModelIndex index = view_->indexAt(position);
        const ObjectId id = model_->objectForIndex(index);
        if (id != InvalidObjectId) {
            view_->setCurrentIndex(index);
        }
        emit contextMenuRequested(id, view_->viewport()->mapToGlobal(position));
    });
    // Ağaç içi düzenleme adı doğrudan yazmaz: model isteği yayınlar, kabuk
    // bunu undoable bir RenameObjectCommand'e çevirir.
    connect(model_, &ProjectTreeModel::renameRequested, this, &ProjectNavigator::renameCommitted);
    view_->expandAll();
}

void ProjectNavigator::collapseAll()
{
    view_->collapseAll();
}

void ProjectNavigator::beginInlineRename(const ObjectId id)
{
    const QModelIndex index = model_->indexForObject(id);
    if (!index.isValid()) {
        return;
    }
    view_->setCurrentIndex(index);
    view_->edit(index);
}

void ProjectNavigator::selectObject(const ObjectId id)
{
    const QModelIndex index = model_->indexForObject(id);
    if (!index.isValid()) {
        return;
    }
    view_->setCurrentIndex(index);
    view_->scrollTo(index);
}

ObjectId ProjectNavigator::selectedObject() const
{
    return model_->objectForIndex(view_->currentIndex());
}

void ProjectNavigator::expandAll()
{
    view_->expandAll();
}

void ProjectNavigator::refreshDecorations()
{
    CaeIcons::invalidateCache();
    model_->refreshDecorations();
    view_->expandAll();
}

} // namespace d26
