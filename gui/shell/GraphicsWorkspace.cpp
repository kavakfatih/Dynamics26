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
namespace {

bool isGeometryFilter(const SelectionFilter filter) noexcept
{
    return filter == SelectionFilter::Body || filter == SelectionFilter::Face
        || filter == SelectionFilter::Edge || filter == SelectionFilter::Vertex;
}

bool isMeshFilter(const SelectionFilter filter) noexcept
{
    return filter == SelectionFilter::Node || filter == SelectionFilter::Element
        || filter == SelectionFilter::Facet;
}

} // namespace

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
    selectNode_ = addFilter(tr("Node"), tr("FEM Node seçimi"), SelectionFilter::Node);
    selectElement_ = addFilter(tr("Element"), tr("FEM Element seçimi"), SelectionFilter::Element);
    selectFacet_ = addFilter(tr("Facet"), tr("FEM boundary facet seçimi"), SelectionFilter::Facet);
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

    setSelectionFilterDomain(SelectionDomain::Geometry);
    setTopologySelectionAvailable(false, false, false);
    setMeshSelectionAvailable(false, false, false);
}

void GraphicsWorkspace::refreshIcons()
{
    const QColor tint = qApp->palette().color(QPalette::WindowText);
    selectBody_->setIcon(CaeIcons::forCommand(CommandGlyph::SelectBody, tint));
    selectFace_->setIcon(CaeIcons::forCommand(CommandGlyph::SelectFace, tint));
    selectEdge_->setIcon(CaeIcons::forCommand(CommandGlyph::SelectEdge, tint));
    selectVertex_->setIcon(CaeIcons::forCommand(CommandGlyph::SelectVertex, tint));
    // FEM ve CAD entity seviyeleri ayni semantik geometri glif ailesini kullanir;
    // label/tooltip domain ayrimini acikca belirtir.
    selectNode_->setIcon(CaeIcons::forCommand(CommandGlyph::SelectVertex, tint));
    selectElement_->setIcon(CaeIcons::forCommand(CommandGlyph::SelectBody, tint));
    selectFacet_->setIcon(CaeIcons::forCommand(CommandGlyph::SelectFace, tint));
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
    QAction *action = nullptr;
    switch (filter) {
    case SelectionFilter::Body: action = selectBody_; break;
    case SelectionFilter::Face: action = selectFace_; break;
    case SelectionFilter::Edge: action = selectEdge_; break;
    case SelectionFilter::Vertex: action = selectVertex_; break;
    case SelectionFilter::Node: action = selectNode_; break;
    case SelectionFilter::Element: action = selectElement_; break;
    case SelectionFilter::Facet: action = selectFacet_; break;
    }
    return action != nullptr && action->isVisible() && action->isEnabled();
}

void GraphicsWorkspace::syncFilterChecks()
{
    if (selectBody_ != nullptr) selectBody_->setChecked(filter_ == SelectionFilter::Body);
    if (selectFace_ != nullptr) selectFace_->setChecked(filter_ == SelectionFilter::Face);
    if (selectEdge_ != nullptr) selectEdge_->setChecked(filter_ == SelectionFilter::Edge);
    if (selectVertex_ != nullptr) selectVertex_->setChecked(filter_ == SelectionFilter::Vertex);
    if (selectNode_ != nullptr) selectNode_->setChecked(filter_ == SelectionFilter::Node);
    if (selectElement_ != nullptr) selectElement_->setChecked(filter_ == SelectionFilter::Element);
    if (selectFacet_ != nullptr) selectFacet_->setChecked(filter_ == SelectionFilter::Facet);
}

void GraphicsWorkspace::syncFilterVisibility()
{
    const bool geometry = filterDomain_.has_value() && *filterDomain_ == SelectionDomain::Geometry;
    const bool mesh = filterDomain_.has_value() && *filterDomain_ == SelectionDomain::Mesh;
    for (QAction *action : {selectBody_, selectFace_, selectEdge_, selectVertex_}) {
        if (action != nullptr) action->setVisible(geometry);
    }
    for (QAction *action : {selectNode_, selectElement_, selectFacet_}) {
        if (action != nullptr) action->setVisible(mesh);
    }
}

void GraphicsWorkspace::setSelectionFilterDomain(const std::optional<SelectionDomain> domain)
{
    if (filterDomain_ == domain) {
        syncFilterVisibility();
        syncFilterChecks();
        return;
    }
    filterDomain_ = domain;
    syncFilterVisibility();
    syncFilterChecks();
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
    syncFilterChecks();
}

void GraphicsWorkspace::setMeshSelectionAvailable(const bool nodeAvailable,
                                                   const bool elementAvailable,
                                                   const bool facetAvailable)
{
    selectNode_->setEnabled(nodeAvailable);
    selectElement_->setEnabled(elementAvailable);
    selectFacet_->setEnabled(facetAvailable);
    selectNode_->setToolTip(nodeAvailable ? tr("FEM Node seçimi") : tr("Önce güncel mesh üretin."));
    selectElement_->setToolTip(elementAvailable ? tr("Görünür FEM Element seçimi")
                                                : tr("Önce güncel mesh üretin."));
    selectFacet_->setToolTip(facetAvailable ? tr("HEX8 boundary facet seçimi")
                                            : tr("Önce güncel mesh üretin."));
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
