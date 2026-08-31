#include "SelectionCoordinator.h"

#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "DetailsHost.h"
#include "../details/GeometryDetails.h"
#include "../services/AnalysisService.h"
#include "../services/GeometryService.h"
#include "../services/MeshService.h"
#include "../viewport/ViewportSelectionBridge.h"
#include "Dynamics26MainWindow.h"
#include "EngineeringStatusBar.h"
#include "ProjectNavigator.h"

#include <QSet>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTimer>

using femcae::geometry::GeometryEntityId;
using femcae::geometry::InvalidGeometryId;

namespace d26 {
namespace {

SelectionKind selectionKindForFilter(const SelectionFilter filter) noexcept
{
    switch (filter) {
    case SelectionFilter::Body: return SelectionKind::Body;
    case SelectionFilter::Face: return SelectionKind::Face;
    case SelectionFilter::Edge: return SelectionKind::Edge;
    case SelectionFilter::Vertex: return SelectionKind::Vertex;
    }
    return SelectionKind::Body;
}

QString selectionKindText(const SelectionKind kind)
{
    switch (kind) {
    case SelectionKind::Body: return QStringLiteral("Body");
    case SelectionKind::Face: return QStringLiteral("Face");
    case SelectionKind::Edge: return QStringLiteral("Edge");
    case SelectionKind::Vertex: return QStringLiteral("Vertex");
    default: return QStringLiteral("Entity");
    }
}

GeometryEntityId parentBodyFor(const SelectionItem &item) noexcept
{
    return item.kind == SelectionKind::Body ? item.geometryEntityId : item.parentGeometryId;
}

bool isMeshBackedContext(const ViewportContext context) noexcept
{
    return context == ViewportContext::Mesh || context == ViewportContext::Loads
        || context == ViewportContext::Analysis || context == ViewportContext::Results;
}

bool usesPassiveCadBackdrop(const ViewportContext context, const bool hasMesh) noexcept
{
    if (context == ViewportContext::Materials || context == ViewportContext::Connections
        || context == ViewportContext::Modal || context == ViewportContext::Empty) {
        return true;
    }
    return isMeshBackedContext(context) && !hasMesh;
}

} // namespace

SelectionCoordinator::SelectionCoordinator(Dynamics26MainWindow *window, QObject *parent)
    : QObject(parent), window_(window)
{
    if (window_ == nullptr) {
        return;
    }

    services_ = window_->services();
    navigator_ = window_->navigator();
    graphics_ = window_->graphics();
    details_ = window_->detailsHost();
    status_ = qobject_cast<EngineeringStatusBar *>(window_->statusBar());

    if (services_.project == nullptr || services_.geometry == nullptr
        || navigator_ == nullptr || graphics_ == nullptr || details_ == nullptr) {
        return;
    }

    selection_ = new SelectionManager(this);
    bridge_ = new ViewportSelectionBridge(graphics_->viewport(), this);

    connect(graphics_, &GraphicsWorkspace::selectionFilterChanged,
            this, [this](const SelectionFilter filter) {
                configurePolicy(filter);
                updateFeedback();
            });

    connect(navigator_, &ProjectNavigator::objectSelected,
            this, &SelectionCoordinator::handleNavigatorSelection);

    connect(bridge_, &ViewportSelectionBridge::selectionRequested,
            this, &SelectionCoordinator::handleViewportSelection);
    connect(bridge_, &ViewportSelectionBridge::preselectionRequested,
            this, &SelectionCoordinator::handleViewportPreselection);
    connect(bridge_, &ViewportSelectionBridge::contextMenuRequested,
            this, &SelectionCoordinator::handleViewportContextMenu);
    connect(bridge_, &ViewportSelectionBridge::selectionClearRequested,
            this, [this] {
                if (selection_ != nullptr) {
                    selection_->clearPreselection();
                    (void)selection_->clear();
                }
            });
    connect(bridge_, &ViewportSelectionBridge::preselectionClearRequested,
            this, [this] {
                if (selection_ != nullptr) {
                    selection_->clearPreselection();
                }
            });

    connect(selection_, &SelectionManager::selectionChanged,
            this, &SelectionCoordinator::handleSelectionChanged);
    connect(selection_, &SelectionManager::preselectionChanged,
            this, &SelectionCoordinator::handlePreselectionChanged);

    connect(services_.geometry, &GeometryService::changed, this, [this] {
        if (selection_ != nullptr) {
            selection_->clearPreselection();
            (void)selection_->clear();
        }
        if (bridge_ != nullptr) {
            bridge_->clearScene();
        }
        QTimer::singleShot(0, this, [this] { refreshGeometryScene(); });
    });

    if (services_.commands != nullptr) {
        connect(services_.commands, &DocumentCommandManager::documentMutated,
                this, [this] { QTimer::singleShot(0, this, [this] { refreshGeometryScene(); }); });
    }

    connect(details_, &DetailsHost::modelEdited,
            this, [this] { QTimer::singleShot(0, this, [this] { refreshGeometryScene(); }); });
    connect(details_->geometryPage(), &GeometryDetails::tessellationQualityChanged,
            this, [this](const double value) {
                tessellationDeflection_ = value;
                QTimer::singleShot(0, this, [this] { refreshGeometryScene(); });
            });

    configurePolicy(graphics_->selectionFilter());
    QTimer::singleShot(0, this, [this] {
        refreshGeometryScene();
        updateFeedback();
    });
}

ScopeReferenceBuildResult SelectionCoordinator::currentGeometryScope() const
{
    if (selection_ == nullptr || services_.geometry == nullptr) {
        ScopeReferenceBuildResult result;
        result.error = ScopeReferenceBuildError::EmptySelection;
        return result;
    }
    return buildGeometryScopeReference(selection_->items(), services_.geometry->document());
}

void SelectionCoordinator::configurePolicy(const SelectionFilter filter)
{
    if (selection_ == nullptr) {
        return;
    }
    const SelectionKind kind = selectionKindForFilter(filter);
    if (bridge_ != nullptr) {
        // Picker actor/filter önce değiştirilir; policy değişimi eski selection'ı
        // temizlerken kısa süreli olarak yanlış primitive actor'ı aktif kalmaz.
        bridge_->setActiveKind(kind);
    }
    SelectionPolicy policy = SelectionPolicy::preset(SelectionPolicyPreset::NeutralGeometry);
    policy.allowedKinds = {kind};
    policy.allowMultiple = true;
    selection_->setPolicy(policy);
}

void SelectionCoordinator::refreshGeometryScene()
{
    if (bridge_ == nullptr || graphics_ == nullptr || services_.geometry == nullptr) {
        return;
    }

    ViewportWidget *viewport = graphics_->viewport();
    if (viewport == nullptr) {
        return;
    }

    const ViewportContext context = viewport->context();
    const GeometrySummary summary = services_.geometry->summary();

    if (context != ViewportContext::Geometry) {
        bridge_->clearScene();
        graphics_->setTopologySelectionAvailable(false, false, false);
        if (selection_ != nullptr) {
            selection_->clearPreselection();
            (void)selection_->clear();
        }
        graphics_->setSelectionLabel(QString());

        // MainWindow'un legacy model-backdrop yolu tek Body tessellation'i
        // çiziyordu. SelectionCoordinator burada selection actor'ı kurmaz;
        // yalnız CAD-backdrop kullanan bağlamlarda all-body display scene'i
        // yeniden gösterir. Gerçek FEM mesh/results mevcutsa viewport'a dokunmaz.
        const bool hasMesh = services_.mesh != nullptr && services_.mesh->hasMesh();
        if (summary.hasGeometry && usesPassiveCadBackdrop(context, hasMesh)) {
            const auto surfaces = services_.geometry->displayTopologyScene(tessellationDeflection_);
            if (surfaces.size() == static_cast<qsizetype>(summary.bodyCount)) {
                viewport->showGeometry(surfaces);
            }
        }
        return;
    }

    if (!summary.hasGeometry) {
        bridge_->clearScene();
        graphics_->setTopologySelectionAvailable(false, false, false);
        graphics_->setSelectionFilter(SelectionFilter::Body);
        if (selection_ != nullptr) {
            selection_->clearPreselection();
            (void)selection_->clear();
        }
        return;
    }

    const auto surfaces = services_.geometry->displayTopologyScene(tessellationDeflection_);
    const auto edges = services_.geometry->displayEdgeScene(tessellationDeflection_);
    const auto vertices = services_.geometry->displayVertexScene();
    const qsizetype expected = static_cast<qsizetype>(summary.bodyCount);
    if (surfaces.size() != expected || edges.size() != expected || vertices.size() != expected
        || !bridge_->setScene(surfaces, edges, vertices)) {
        bridge_->clearScene();
        graphics_->setTopologySelectionAvailable(false, false, false);
        graphics_->setSelectionFilter(SelectionFilter::Body);
        if (selection_ != nullptr) {
            selection_->clearPreselection();
            (void)selection_->clear();
        }
        return;
    }

    graphics_->setTopologySelectionAvailable(bridge_->hasFaceProvenance(),
                                             bridge_->hasEdgeProvenance(),
                                             bridge_->hasVertexProvenance());
    configurePolicy(graphics_->selectionFilter());
    if (selection_ != nullptr) {
        (void)selection_->invalidateGeometryRevision(summary.revision);
        bridge_->setSelection(selection_->items());
        bridge_->setPreselection(selection_->preselection());
    }
}

void SelectionCoordinator::handleNavigatorSelection(const ObjectId id)
{
    if (syncingNavigator_ || services_.project == nullptr || services_.geometry == nullptr) {
        return;
    }

    refreshGeometryScene();

    const ProjectObject *object = services_.project->object(id);
    if (object == nullptr || object->type != ObjectType::Body || object->tag <= 0
        || !services_.geometry->summary().hasGeometry || selection_ == nullptr) {
        return;
    }

    graphics_->setSelectionFilter(SelectionFilter::Body);

    SelectionItem item;
    item.domain = SelectionDomain::Geometry;
    item.kind = SelectionKind::Body;
    item.geometryEntityId = static_cast<GeometryEntityId>(object->tag);
    item.sourceRevision = services_.geometry->summary().revision;
    (void)selection_->apply(item, SelectionOperation::Replace);
}

std::optional<SelectionItem> SelectionCoordinator::selectionItemForHit(const SelectionKind kind,
                                                                       const quint64 bodyId,
                                                                       const quint64 geometryId) const
{
    if (graphics_ == nullptr || services_.geometry == nullptr || bodyId == 0 || geometryId == 0
        || kind != selectionKindForFilter(graphics_->selectionFilter())) {
        return std::nullopt;
    }

    SelectionItem item;
    item.domain = SelectionDomain::Geometry;
    item.kind = kind;
    item.sourceRevision = services_.geometry->summary().revision;
    if (kind == SelectionKind::Body) {
        if (geometryId != bodyId) {
            return std::nullopt;
        }
        item.geometryEntityId = static_cast<GeometryEntityId>(bodyId);
    } else {
        item.geometryEntityId = static_cast<GeometryEntityId>(geometryId);
        item.parentGeometryId = static_cast<GeometryEntityId>(bodyId);
    }

    const auto expectedKind = geometryEntityKindForSelectionKind(kind);
    const auto *entity = services_.geometry->document().find(item.geometryEntityId);
    if (!expectedKind.has_value() || entity == nullptr || entity->kind != *expectedKind) {
        return std::nullopt;
    }
    if (kind != SelectionKind::Body && entity->parentId != item.parentGeometryId) {
        return std::nullopt;
    }
    return item;
}

bool SelectionCoordinator::selectionContains(const SelectionItem &item) const noexcept
{
    if (selection_ == nullptr) {
        return false;
    }
    for (const SelectionItem &selected : selection_->items()) {
        if (selected.sameIdentity(item)) {
            return true;
        }
    }
    return false;
}

void SelectionCoordinator::handleViewportSelection(const SelectionKind kind,
                                                    const quint64 bodyId,
                                                    const quint64 geometryId,
                                                    const SelectionOperation operation)
{
    if (selection_ == nullptr) {
        return;
    }
    selection_->clearPreselection();
    const auto item = selectionItemForHit(kind, bodyId, geometryId);
    if (!item.has_value()) {
        (void)selection_->clear();
        return;
    }
    (void)selection_->apply(*item, operation);
}

void SelectionCoordinator::handleViewportPreselection(const SelectionKind kind,
                                                       const quint64 bodyId,
                                                       const quint64 geometryId)
{
    if (selection_ == nullptr) {
        return;
    }
    selection_->setPreselection(selectionItemForHit(kind, bodyId, geometryId));
}

void SelectionCoordinator::handleViewportContextMenu(const SelectionKind kind,
                                                      const quint64 bodyId,
                                                      const quint64 geometryId,
                                                      const QPoint &globalPosition)
{
    if (selection_ == nullptr || window_ == nullptr) {
        return;
    }

    const auto item = selectionItemForHit(kind, bodyId, geometryId);
    if (!item.has_value()) {
        return;
    }

    selection_->clearPreselection();
    if (!selectionContains(*item)) {
        (void)selection_->apply(*item, SelectionOperation::Replace);
    }

    const GeometryEntityId contextBodyId = parentBodyFor(*item);
    const ObjectId contextObject = bodyObjectForGeometryId(contextBodyId);
    if (contextObject == InvalidObjectId) {
        return;
    }

    (void)syncNavigatorToGeometryBody(contextBodyId);
    window_->showObjectContextMenu(contextObject, globalPosition);
}

void SelectionCoordinator::handleSelectionChanged()
{
    if (selection_ == nullptr || bridge_ == nullptr) {
        return;
    }
    (void)syncNavigatorToPrimary();
    bridge_->setSelection(selection_->items());
    updateFeedback();
}

void SelectionCoordinator::handlePreselectionChanged()
{
    if (selection_ == nullptr || bridge_ == nullptr) {
        return;
    }
    bridge_->setPreselection(selection_->preselection());
    updateFeedback();
}

bool SelectionCoordinator::syncNavigatorToPrimary()
{
    if (selection_ == nullptr) {
        return false;
    }
    const auto primary = selection_->primary();
    if (!primary.has_value() || primary->domain != SelectionDomain::Geometry) {
        return false;
    }
    return syncNavigatorToGeometryBody(parentBodyFor(*primary));
}

bool SelectionCoordinator::syncNavigatorToGeometryBody(const GeometryEntityId bodyId)
{
    if (bodyId == InvalidGeometryId || window_ == nullptr || navigator_ == nullptr) {
        return false;
    }

    const ObjectId objectId = bodyObjectForGeometryId(bodyId);
    if (objectId == InvalidObjectId || navigator_->selectedObject() == objectId) {
        return false;
    }

    syncingNavigator_ = true;
    {
        const QSignalBlocker navigatorSignals(navigator_);
        navigator_->selectObject(objectId);
    }

    // SelectionCoordinator kontrollü composition-helper'dır. Bu yol viewport
    // scene/camera rebuild'i yapmadan yalnız current project-object bağlamını
    // günceller; MainWindow friend sınırı dışında kullanılmaz.
    window_->selected_ = objectId;
    const ObjectId owning = window_->analysis_->owningAnalysis(objectId);
    if (owning != InvalidObjectId) {
        window_->activeAnalysis_ = owning;
    }
    details_->showObject(objectId);
    window_->syncCommandStates();
    window_->syncContextualSurface();
    window_->syncStatusBar();

    syncingNavigator_ = false;
    return true;
}

ObjectId SelectionCoordinator::bodyObjectForGeometryId(const GeometryEntityId bodyId) const
{
    if (services_.project == nullptr) {
        return InvalidObjectId;
    }
    for (const ObjectId id : services_.project->childrenOf(services_.project->geometryNode())) {
        const ProjectObject *object = services_.project->object(id);
        if (object != nullptr && object->type == ObjectType::Body
            && object->tag > 0 && static_cast<GeometryEntityId>(object->tag) == bodyId) {
            return id;
        }
    }
    return InvalidObjectId;
}

QString SelectionCoordinator::geometryEntityName(const GeometryEntityId id) const
{
    if (services_.geometry == nullptr || id == InvalidGeometryId) {
        return {};
    }
    const auto *entity = services_.geometry->document().find(id);
    if (entity == nullptr || entity->name.empty()) {
        return {};
    }
    return QString::fromStdString(entity->name);
}

QString SelectionCoordinator::bodyNameFor(const SelectionItem &item) const
{
    QString name = geometryEntityName(parentBodyFor(item));
    return name.isEmpty() ? tr("Body") : name;
}

void SelectionCoordinator::updateFeedback()
{
    if (selection_ == nullptr || graphics_ == nullptr) {
        return;
    }

    const auto &items = selection_->items();
    if (!items.isEmpty()) {
        const auto primaryValue = selection_->primary();
        const SelectionItem &primary = primaryValue.has_value() ? *primaryValue : items.back();
        const QString kind = selectionKindText(primary.kind);
        graphics_->setSelectionLabel(tr("%1 %2 seçildi").arg(items.size()).arg(kind));

        QSet<GeometryEntityId> bodyIds;
        for (const SelectionItem &item : items) {
            const GeometryEntityId bodyId = parentBodyFor(item);
            if (bodyId != InvalidGeometryId) {
                bodyIds.insert(bodyId);
            }
        }

        if (details_ != nullptr) {
            if (primary.kind == SelectionKind::Body) {
                details_->setSelectionSummary(tr("SELECTION  ·  %1 Body").arg(items.size()));
            } else {
                const QString scopeText = bodyIds.size() == 1
                    ? tr("SELECTION  ·  %1 %2  ·  %3").arg(items.size()).arg(kind).arg(bodyNameFor(primary))
                    : tr("SELECTION  ·  %1 %2  ·  %3 Body").arg(items.size()).arg(kind).arg(bodyIds.size());
                details_->setSelectionSummary(scopeText);
            }
        }

        if (status_ != nullptr) {
            if (primary.kind == SelectionKind::Body) {
                status_->setSelection(tr("%1 Body  •  Global Coordinate System").arg(items.size()));
            } else if (bodyIds.size() == 1) {
                status_->setSelection(tr("%1 %2  •  %3  •  Global Coordinate System")
                                          .arg(items.size()).arg(kind).arg(bodyNameFor(primary)));
            } else {
                status_->setSelection(tr("%1 %2  •  %3 Body  •  Global Coordinate System")
                                          .arg(items.size()).arg(kind).arg(bodyIds.size()));
            }
        }
        return;
    }

    if (details_ != nullptr) {
        details_->setSelectionSummary(QString());
    }

    const auto hover = selection_->preselection();
    if (hover.has_value() && graphics_->viewport()->context() == ViewportContext::Geometry) {
        QString label = geometryEntityName(hover->geometryEntityId);
        if (label.isEmpty()) {
            label = selectionKindText(hover->kind);
        }
        graphics_->setSelectionLabel(tr("%1  ·  ön seçim").arg(label));
    } else {
        graphics_->setSelectionLabel(QString());
    }

    if (status_ != nullptr) {
        const ProjectObject *object = services_.project != nullptr
            ? services_.project->object(navigator_ != nullptr ? navigator_->selectedObject() : InvalidObjectId)
            : nullptr;
        status_->setSelection(object != nullptr ? object->name : QString());
    }
}

} // namespace d26
