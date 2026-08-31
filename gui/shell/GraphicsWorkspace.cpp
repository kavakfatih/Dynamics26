#include "GraphicsWorkspace.h"

#include "../core/CaeIcons.h"
#include "../core/UiTheme.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QFont>
#include <QLabel>
#include <QToolBar>
#include <QVBoxLayout>

namespace d26 {

GraphicsWorkspace::GraphicsWorkspace(QWidget *parent) : QFrame(parent)
{
    setObjectName(QStringLiteral("Dynamics26GraphicsWorkspace"));
    setFrameShape(QFrame::NoFrame);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    toolbar_ = new QToolBar(this);
    toolbar_->setObjectName(QStringLiteral("Dynamics26GraphicsToolbar"));
    toolbar_->setMovable(false);
    toolbar_->setFloatable(false);
    toolbar_->setIconSize(QSize(16, 16));
    toolbar_->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolbar_->setContentsMargins(0, 0, 0, 0);

    auto *filterGroup = new QActionGroup(this);
    filterGroup->setExclusive(true);
    const auto addFilter = [this, filterGroup](const QString &label,
                                               const QString &toolTip,
                                               const SelectionFilter filter) {
        QAction *action = toolbar_->addAction(label);
        action->setCheckable(true);
        action->setToolTip(toolTip);
        filterGroup->addAction(action);
        connect(action, &QAction::triggered, this, [this, filter] { setSelectionFilter(filter); });
        return action;
    };

    selectBody_ = addFilter(tr("Body"), tr("CAD Body seçimi"), SelectionFilter::Body);
    selectFace_ = addFilter(tr("Face"), tr("CAD Face seçimi"), SelectionFilter::Face);
    selectEdge_ = addFilter(tr("Edge"), tr("CAD Edge seçimi"), SelectionFilter::Edge);
    selectVertex_ = addFilter(tr("Vertex"), tr("CAD Vertex seçimi"), SelectionFilter::Vertex);
    selectBody_->setChecked(true);

    toolbar_->addSeparator();
    fit_ = toolbar_->addAction(tr("Fit"));
    fit_->setToolTip(tr("Görünümü sığdır"));
    isometric_ = toolbar_->addAction(tr("Isometric"));
    isometric_->setToolTip(tr("İzometrik görünüm"));
    toolbar_->addSeparator();

    selectionLabel_ = new ui::SecondaryLabel(QString(), 0.68, 0.82, toolbar_);
    QFont compact = selectionLabel_->font();
    compact.setPointSizeF(qMax(9.0, compact.pointSizeF() - 1.5));
    selectionLabel_->setFont(compact);
    selectionLabel_->setContentsMargins(4, 0, 4, 0);
    toolbar_->addWidget(selectionLabel_);

    auto *spacer = new QWidget(toolbar_);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar_->addWidget(spacer);

    contextLabel_ = new ui::SecondaryLabel(QString(), 0.52, 0.72, toolbar_);
    QFont contextFont = contextLabel_->font();
    contextFont.setPointSizeF(qMax(9.0, contextFont.pointSizeF() - 2.0));
    contextFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    contextLabel_->setFont(contextFont);
    contextLabel_->setContentsMargins(6, 0, 10, 0);
    toolbar_->addWidget(contextLabel_);

    layout->addWidget(toolbar_);
    layout->addWidget(new ui::Hairline(this));

    viewport_ = new ViewportWidget(this);
    layout->addWidget(viewport_, 1);

    refreshIcons();

    connect(fit_, &QAction::triggered, this, &GraphicsWorkspace::fitViewRequested);
    connect(isometric_, &QAction::triggered, this, &GraphicsWorkspace::isometricViewRequested);

    setTopologySelectionAvailable(false, false, false);
}

void GraphicsWorkspace::refreshIcons()
{
    const QColor tint = qApp->palette().color(QPalette::WindowText);
    selectBody_->setIcon(CaeIcons::forCommand(CommandGlyph::SelectBody, tint));
    selectFace_->setIcon(CaeIcons::forCommand(CommandGlyph::SelectFace, tint));
    selectEdge_->setIcon(CaeIcons::forCommand(CommandGlyph::SelectEdge, tint));
    selectVertex_->setIcon(CaeIcons::forCommand(CommandGlyph::SelectVertex, tint));
    fit_->setIcon(CaeIcons::forCommand(CommandGlyph::FitView, tint));
    isometric_->setIcon(CaeIcons::forCommand(CommandGlyph::Isometric, tint));
}

void GraphicsWorkspace::setContextLabel(const QString &text)
{
    contextLabel_->setText(text.toUpper());
}

void GraphicsWorkspace::setSelectionLabel(const QString &text)
{
    selectionLabel_->setText(text);
}

bool GraphicsWorkspace::filterAvailable(const SelectionFilter filter) const noexcept
{
    switch (filter) {
    case SelectionFilter::Body: return selectBody_ != nullptr && selectBody_->isEnabled();
    case SelectionFilter::Face: return selectFace_ != nullptr && selectFace_->isEnabled();
    case SelectionFilter::Edge: return selectEdge_ != nullptr && selectEdge_->isEnabled();
    case SelectionFilter::Vertex: return selectVertex_ != nullptr && selectVertex_->isEnabled();
    }
    return false;
}

void GraphicsWorkspace::syncFilterChecks()
{
    if (selectBody_ != nullptr) selectBody_->setChecked(filter_ == SelectionFilter::Body);
    if (selectFace_ != nullptr) selectFace_->setChecked(filter_ == SelectionFilter::Face);
    if (selectEdge_ != nullptr) selectEdge_->setChecked(filter_ == SelectionFilter::Edge);
    if (selectVertex_ != nullptr) selectVertex_->setChecked(filter_ == SelectionFilter::Vertex);
}

void GraphicsWorkspace::setSelectionFilter(const SelectionFilter filter)
{
    if (!filterAvailable(filter)) {
        syncFilterChecks();
        return;
    }
    if (filter_ == filter) {
        syncFilterChecks();
        return;
    }

    filter_ = filter;
    syncFilterChecks();
    emit selectionFilterChanged(filter_);
}

void GraphicsWorkspace::setTopologySelectionAvailable(const bool faceAvailable,
                                                       const bool edgeAvailable,
                                                       const bool vertexAvailable)
{
    selectBody_->setEnabled(true);
    selectFace_->setEnabled(faceAvailable);
    selectEdge_->setEnabled(edgeAvailable);
    selectVertex_->setEnabled(vertexAvailable);

    selectFace_->setToolTip(faceAvailable
                                ? tr("CAD Face seçimi")
                                : tr("Face seçimi için CAD Face provenance gerekli."));
    selectEdge_->setToolTip(edgeAvailable
                                ? tr("CAD Edge seçimi")
                                : tr("Edge seçimi için canonical CAD Edge provenance gerekli."));
    selectVertex_->setToolTip(vertexAvailable
                                  ? tr("CAD Vertex seçimi")
                                  : tr("Vertex seçimi için canonical CAD Vertex provenance gerekli."));

    // Capability değişikliği mevcut filter niyetini sessizce değiştirmez.
    // Coordinator başarısız scene kurulumunda açıkça Body'ye döner.
    syncFilterChecks();
}

void GraphicsWorkspace::setFaceSelectionAvailable(const bool legacyMeshDerivedAvailability)
{
    // Alpha.3.2 kabuğu CAD Face capability'yi yanlışlıkla FEM mesh varlığına
    // bağlamıştı. Kaynak uyumluluğu için çağrı korunur ama karar artık yalnız
    // SelectionCoordinator'daki gerçek CAD provenance kontrolüne aittir.
    Q_UNUSED(legacyMeshDerivedAvailability)
}

} // namespace d26
