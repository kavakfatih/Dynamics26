#include "DetailsHost.h"

#include "../core/ProjectModel.h"
#include "../core/UiTheme.h"
#include "../details/AnalysisDetails.h"
#include "../details/BoundaryConditionDetails.h"
#include "../details/ConnectionsDetails.h"
#include "../details/ContactDetails.h"
#include "../details/DetailsPage.h"
#include "../details/GeometryDetails.h"
#include "../details/MaterialDetails.h"
#include "../details/MeshDetails.h"
#include "../details/ObjectDetails.h"
#include "../details/ResultDetails.h"
#include "../details/SelectionDetails.h"

#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace d26 {

DetailsHost::DetailsHost(const ServiceContext &services, QWidget *parent)
    : QFrame(parent), services_(services)
{
    setObjectName(QStringLiteral("Dynamics26DetailsHost"));
    setFrameShape(QFrame::NoFrame);
    setMinimumWidth(268);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new QWidget(this);
    auto *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(10, 8, 10, 7);
    headerLayout->setSpacing(1);
    subtitle_ = new ui::SecondaryLabel(tr("DETAILS"), 0.52, 0.72, header);
    QFont subtitleFont = subtitle_->font();
    subtitleFont.setPointSizeF(qMax(9.0, subtitleFont.pointSizeF() - 2.0));
    subtitleFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    subtitle_->setFont(subtitleFont);
    headerLayout->addWidget(subtitle_);

    title_ = new QLabel(tr("Seçim yok"), header);
    QFont titleFont = title_->font();
    titleFont.setBold(true);
    title_->setFont(titleFont);
    title_->setWordWrap(true);
    headerLayout->addWidget(title_);

    // Document object ile transient viewport selection ayni şey değildir.
    // Selection özeti bu nedenle title/subtitle yerine ayrı, ikincil bir satırda
    // yaşar; hover burada gösterilmez, yalnız committed seçim görünür.
    selectionSummary_ = new ui::SecondaryLabel(QString(), 0.58, 0.78, header);
    selectionSummary_->setObjectName(QStringLiteral("Dynamics26SelectionSummary"));
    QFont selectionFont = selectionSummary_->font();
    selectionFont.setPointSizeF(qMax(9.0, selectionFont.pointSizeF() - 1.5));
    selectionSummary_->setFont(selectionFont);
    selectionSummary_->setWordWrap(true);
    selectionSummary_->setVisible(false);
    headerLayout->addWidget(selectionSummary_);

    layout->addWidget(header);

    layout->addWidget(new ui::Hairline(this));

    scroll_ = new QScrollArea(this);
    scroll_->setWidgetResizable(true);
    scroll_->setFrameShape(QFrame::NoFrame);
    scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    stack_ = new QStackedWidget(scroll_);
    scroll_->setWidget(stack_);
    layout->addWidget(scroll_, 1);

    emptyState_ = new QWidget(stack_);
    auto *emptyLayout = new QVBoxLayout(emptyState_);
    emptyLayout->setContentsMargins(18, 26, 18, 18);
    auto *emptyLabel = new ui::SecondaryLabel(tr("Özelliklerini görmek için model ağacından bir nesne seçin."),
                                              0.55, 0.74, emptyState_);
    emptyLabel->setWordWrap(true);
    emptyLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    emptyLayout->addWidget(emptyLabel);
    emptyLayout->addStretch(1);
    stack_->addWidget(emptyState_);

    geometry_ = new GeometryDetails(services_, stack_);
    mesh_ = new MeshDetails(services_, stack_);
    material_ = new MaterialDetails(services_, stack_);
    analysis_ = new AnalysisDetails(services_, stack_);
    boundary_ = new BoundaryConditionDetails(services_, stack_);
    connections_ = new ConnectionsDetails(services_, stack_);
    contact_ = new ContactDetails(services_, stack_);
    result_ = new ResultDetails(services_, stack_);
    selection_ = new SelectionDetails(services_, stack_);
    stack_->addWidget(selection_);
    object_ = new ObjectDetails(services_, stack_);
    for (DetailsPage *page : {static_cast<DetailsPage *>(geometry_), static_cast<DetailsPage *>(mesh_),
                              static_cast<DetailsPage *>(material_), static_cast<DetailsPage *>(analysis_),
                              static_cast<DetailsPage *>(boundary_), static_cast<DetailsPage *>(connections_),
                              static_cast<DetailsPage *>(contact_), static_cast<DetailsPage *>(result_),
                              static_cast<DetailsPage *>(object_)}) {
        stack_->addWidget(page);
        connectPage(page);
    }
    stack_->setCurrentWidget(emptyState_);
}

void DetailsHost::connectPage(DetailsPage *page)
{
    connect(page, &DetailsPage::modelEdited, this, &DetailsHost::modelEdited);
    connect(page, &DetailsPage::requestCommand, this, &DetailsHost::commandRequested);
}

DetailsPage *DetailsHost::pageFor(const ObjectType type) const
{
    switch (type) {
    case ObjectType::GeometryFolder:
        return geometry_;
    case ObjectType::Mesh:
        return mesh_;
    case ObjectType::Material:
        return material_;
    case ObjectType::Analysis:
    case ObjectType::AnalysisSettings:
        return analysis_;
    case ObjectType::FixedSupport:
    case ObjectType::Force:
        return boundary_;
    case ObjectType::ConnectionsFolder:
        return connections_;
    case ObjectType::ContactRegion:
        return contact_;
    case ObjectType::TotalDeformation:
    case ObjectType::EquivalentStress:
    case ObjectType::ReactionForce:
        return result_;
    default:
        return object_;
    }
}

void DetailsHost::showObject(const ObjectId id)
{
    current_ = id;
    const ProjectObject *object = services_.project->object(id);
    if (object == nullptr) {
        title_->setText(tr("Seçim yok"));
        subtitle_->setText(tr("DETAILS"));
        stack_->setCurrentWidget(emptyState_);
        return;
    }
    title_->setText(object->name);
    subtitle_->setText(displayName(object->type).toUpper());
    DetailsPage *page = pageFor(object->type);
    page->setObject(id);
    stack_->setCurrentWidget(page);
    scroll_->verticalScrollBar()->setValue(0);
}

void DetailsHost::showSelection(const QVector<SelectionItem> &items, CommandRegistry *commands)
{
    if (items.isEmpty() || items.front().domain != SelectionDomain::Geometry
        || items.front().kind == SelectionKind::Body) {
        if (stack_->currentWidget() == selection_) showObject(current_);
        return;
    }
    selection_->showSelection(items, commands);
    stack_->setCurrentWidget(selection_);
}

void DetailsHost::setSelectionSummary(const QString &text)
{
    if (selectionSummary_ == nullptr) {
        return;
    }
    const QString normalized = text.trimmed();
    selectionSummary_->setText(normalized);
    selectionSummary_->setVisible(!normalized.isEmpty());
}

void DetailsHost::refresh()
{
    const ProjectObject *object = services_.project->object(current_);
    if (object == nullptr) {
        return;
    }
    title_->setText(object->name);
    // Kaydırma konumu ve odak korunur; kullanıcı bir alanı düzenlerken sayfa
    // yeniden kurulmaz, yalnız değerler güncellenir.
    if (auto *page = qobject_cast<DetailsPage *>(stack_->currentWidget())) {
        page->refresh();
    }
}

} // namespace d26
