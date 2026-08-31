#include "SelectionCoordinator.h"

#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "DetailsHost.h"
#include "../details/GeometryDetails.h"
#include "../services/AnalysisService.h"
#include "../services/GeometryService.h"
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
                this, [this] {
                    QTimer::singleShot(0, this, [this] { refreshGeometryScene(); });
                });
    }

    connect(details_, &DetailsHost::modelEdited, this, [this] {
        QTimer::singleShot(0, this, [this] { refreshGeometryScene(); });
    });
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
    SelectionPolicy policy = SelectionPolicy::preset(SelectionPolicyPreset::NeutralGeometry);
    policy.allowedKinds = {filter == SelectionFilter::Face
        ? SelectionKind::Face
        : SelectionKind::Body};
    policy.allowMultiple = true;
    selection_->setPolicy(policy);
}

void SelectionCoordinator::refreshGeometryScene()
{
    if (bridge_ == nullptr || graphics_ == nullptr || services_.geometry == nullptr) {
        return;
    }

    ViewportWidget *viewport = graphics_->viewport();
    if (viewport == nullptr || viewport->context() != ViewportContext::Geometry) {
        bridge_->clearScene();
        if (selection_ != nullptr) {
            selection_->clearPreselection();
            (void)selection_->clear();
        }
        graphics_->setSelectionLabel(QString());
        return;
    }

    const GeometrySummary summary = services_.geometry->summary();
    if (!summary.hasGeometry) {
        bridge_->clearScene();
        if (selection_ != nullptr) {
            selection_->clearPreselection();
            (void)selection_->clear();
        }
        graphics_->setSelectionFilter(SelectionFilter::Body);
        graphics_->setFaceSelectionAvailable(false);
        return;
    }

    const auto bodies = services_.geometry->displayTopologyScene(tessellationDeflection_);
    if (bodies.isEmpty() || bodies.size() != summary.bodyCount || !bridge_->setScene(bodies)) {
        bridge_->clearScene();
        if (selection_ != nullptr) {
            selection_->clearPreselection();
            (void)selection_->clear();
        }
        graphics_->setSelectionFilter(SelectionFilter::Body);
        graphics_->setFaceSelectionAvailable(false);
        return;
    }

    graphics_->setFaceSelectionAvailable(bridge_->hasFaceProvenance());
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

    // MainWindow'un objectSelected bağlantısı bu koordinatörden önce kurulduğu
    // için burada viewport context'i zaten günceldir.
    refreshGeometryScene();

    const ProjectObject *object = services_.project->object(id);
    if (object == nullptr || object->type != ObjectType::Body || object->tag <= 0
        || !services_.geometry->summary().hasGeometry || selection_ == nullptr) {
        return;
    }

    // Navigator Body seçimi açıkça Body filter semantiğidir.
    graphics_->setSelectionFilter(SelectionFilter::Body);

    SelectionItem item;
    item.domain = SelectionDomain::Geometry;
    item.kind = SelectionKind::Body;
    item.geometryEntityId = static_cast<GeometryEntityId>(object->tag);
    item.sourceRevision = services_.geometry->summary().revision;
    (void)selection_->apply(item, SelectionOperation::Replace);
}

std::optional<SelectionItem> SelectionCoordinator::selectionItemForHit(const quint64 bodyId,
                                                                       const quint64 faceId) const
{
    if (graphics_ == nullptr || services_.geometry == nullptr || bodyId == 0) {
        return std::nullopt;
    }

    SelectionItem item;
    item.domain = SelectionDomain::Geometry;
    item.sourceRevision = services_.geometry->summary().revision;

    if (graphics_->selectionFilter() == SelectionFilter::Body) {
        item.kind = SelectionKind::Body;
        item.geometryEntityId = static_cast<GeometryEntityId>(bodyId);
    } else {
        if (faceId == 0) {
            return std::nullopt;
        }
        item.kind = SelectionKind::Face;
        item.geometryEntityId = static_cast<GeometryEntityId>(faceId);
        item.parentGeometryId = static_cast<GeometryEntityId>(bodyId);
    }

    const auto *entity = services_.geometry->document().find(item.geometryEntityId);
    if (entity == nullptr) {
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

void SelectionCoordinator::handleViewportSelection(const quint64 bodyId,
                                                    const quint64 faceId,
                                                    const SelectionOperation operation)
{
    if (selection_ == nullptr) {
        return;
    }
    selection_->clearPreselection();
    const auto item = selectionItemForHit(bodyId, faceId);
    if (!item.has_value()) {
        (void)selection_->clear();
        return;
    }
    (void)selection_->apply(*item, operation);
}

void SelectionCoordinator::handleViewportPreselection(const quint64 bodyId, const quint64 faceId)
{
    if (selection_ == nullptr) {
        return;
    }
    selection_->setPreselection(selectionItemForHit(bodyId, faceId));
}

void SelectionCoordinator::handleViewportContextMenu(const quint64 bodyId,
                                                      const quint64 faceId,
                                                      const QPoint &globalPosition)
{
    if (selection_ == nullptr || window_ == nullptr) {
        return;
    }

    const auto item = selectionItemForHit(bodyId, faceId);
    if (!item.has_value()) {
        // Empty secondary click committed selection'i korur.
        return;
    }

    selection_->clearPreselection();
    const bool alreadySelected = selectionContains(*item);
    if (!alreadySelected) {
        // CAE masaüstü davranışı: unselected entity üzerinde secondary click o
        // entity'yi current/primary yapar; önceki multi-selection korunmaz.
        (void)selection_->apply(*item, SelectionOperation::Replace);
    }

    const GeometryEntityId contextBodyId = item->kind == SelectionKind::Body
        ? item->geometryEntityId
        : item->parentGeometryId;
    const ObjectId contextObject = bodyObjectForGeometryId(contextBodyId);
    if (contextObject == InvalidObjectId) {
        return;
    }

    // Already-selected bir entity farklı Body'de olsa dahi selection setini
    // bozmadan yalnız Project Current Object o Body'ye alınır. Bu yol
    // MainWindow::handleSelection() çağırmaz; kamera/CAD scene rebuild edilmez.
    (void)syncNavigatorToGeometryBody(contextBodyId);
    window_->showObjectContextMenu(contextObject, globalPosition);
}

void SelectionCoordinator::handleSelectionChanged()
{
    if (selection_ == nullptr || bridge_ == nullptr) {
        return;
    }

    // Viewport selection başka bir Body'ye geçtiyse Navigator parent Body'yi
    // izler. Bu senkron kamera/sahne rebuild'i yapmadan yalnız project-object
    // bağlamını günceller; CAD overlay state'i aynı display scene üzerinde kalır.
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

    GeometryEntityId bodyId = InvalidGeometryId;
    if (primary->kind == SelectionKind::Body) {
        bodyId = primary->geometryEntityId;
    } else if (primary->kind == SelectionKind::Face) {
        bodyId = primary->parentGeometryId;
    }
    return syncNavigatorToGeometryBody(bodyId);
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
        // Viewport-originated selection/context Navigator/Details bağlamını izler
        // fakat MainWindow::selectObject() çağrılmaz: o yol syncViewport()
        // üzerinden CAD sahnesini yeniden kurup kamerayı isometric'e döndürebilir.
        const QSignalBlocker navigatorSignals(navigator_);
        navigator_->selectObject(objectId);
    }

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
    if (entity == nullptr) {
        return {};
    }
    if (!entity->name.empty()) {
        return QString::fromStdString(entity->name);
    }
    return {};
}

QString SelectionCoordinator::bodyNameFor(const SelectionItem &item) const
{
    const GeometryEntityId bodyId = item.kind == SelectionKind::Body
        ? item.geometryEntityId
        : item.parentGeometryId;
    QString name = geometryEntityName(bodyId);
    if (!name.isEmpty()) {
        return name;
    }
    return tr("Body");
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
        const QString kind = primary.kind == SelectionKind::Face ? tr("Face") : tr("Body");
        graphics_->setSelectionLabel(tr("%1 %2 seçildi").arg(items.size()).arg(kind));

        QSet<GeometryEntityId> bodyIds;
        for (const SelectionItem &item : items) {
            if (item.kind == SelectionKind::Body) {
                bodyIds.insert(item.geometryEntityId);
            } else if (item.kind == SelectionKind::Face) {
                bodyIds.insert(item.parentGeometryId);
            }
        }

        if (details_ != nullptr) {
            if (primary.kind == SelectionKind::Face) {
                const QString scopeText = bodyIds.size() == 1
                    ? tr("SELECTION  ·  %1 Face  ·  %2").arg(items.size()).arg(bodyNameFor(primary))
                    : tr("SELECTION  ·  %1 Face  ·  %2 Body").arg(items.size()).arg(bodyIds.size());
                details_->setSelectionSummary(scopeText);
            } else {
                details_->setSelectionSummary(tr("SELECTION  ·  %1 Body").arg(items.size()));
            }
        }

        if (status_ != nullptr) {
            if (primary.kind == SelectionKind::Face) {
                if (bodyIds.size() == 1) {
                    status_->setSelection(tr("%1 Face  •  %2  •  Global Coordinate System")
                                              .arg(items.size())
                                              .arg(bodyNameFor(primary)));
                } else {
                    status_->setSelection(tr("%1 Face  •  %2 Body  •  Global Coordinate System")
                                              .arg(items.size())
                                              .arg(bodyIds.size()));
                }
            } else {
                status_->setSelection(tr("%1 Body  •  Global Coordinate System").arg(items.size()));
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
            label = hover->kind == SelectionKind::Face ? tr("Face") : tr("Body");
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
