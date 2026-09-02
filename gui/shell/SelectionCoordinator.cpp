#include "SelectionCoordinator.h"

#include "../commands/DomainCommands.h"
#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "DetailsHost.h"
#include "../details/GeometryDetails.h"
#include "../services/AnalysisService.h"
#include "../services/GeometryService.h"
#include "../services/MeshService.h"
#include "../viewport/ViewportMeshSelectionBridge.h"
#include "../viewport/ViewportSelectionBridge.h"
#include "Dynamics26MainWindow.h"
#include "EngineeringStatusBar.h"
#include "ProjectNavigator.h"

#include <QAction>
#include <QHash>
#include <QMenu>
#include <QSet>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTimer>

#include <algorithm>

using femcae::geometry::GeometryEntityId;
using femcae::geometry::InvalidGeometryId;
using femcae::meshing::InvalidMeshId;
using femcae::meshing::MeshEntityId;

namespace d26 {
namespace {

struct PendingWindowSelectionBatch {
    bool armed{false};
    bool flushScheduled{false};
    std::optional<SelectionDomain> domain;
    std::optional<SelectionOperation> operation;
    QVector<SelectionItem> items;
};

QHash<SelectionCoordinator *, PendingWindowSelectionBatch> pendingWindowSelectionBatches;

SelectionKind selectionKindForFilter(const SelectionFilter filter) noexcept
{
    switch (filter) {
    case SelectionFilter::Body: return SelectionKind::Body;
    case SelectionFilter::Face: return SelectionKind::Face;
    case SelectionFilter::Edge: return SelectionKind::Edge;
    case SelectionFilter::Vertex: return SelectionKind::Vertex;
    case SelectionFilter::Node: return SelectionKind::Node;
    case SelectionFilter::Element: return SelectionKind::Element;
    case SelectionFilter::Facet: return SelectionKind::Facet;
    }
    return SelectionKind::Body;
}

SelectionDomain selectionDomainForFilter(const SelectionFilter filter) noexcept
{
    switch (filter) {
    case SelectionFilter::Body:
    case SelectionFilter::Face:
    case SelectionFilter::Edge:
    case SelectionFilter::Vertex:
        return SelectionDomain::Geometry;
    case SelectionFilter::Node:
    case SelectionFilter::Element:
    case SelectionFilter::Facet:
        return SelectionDomain::Mesh;
    }
    return SelectionDomain::Geometry;
}

bool isMeshFilter(const SelectionFilter filter) noexcept
{
    return selectionDomainForFilter(filter) == SelectionDomain::Mesh;
}

QString selectionKindText(const SelectionKind kind)
{
    switch (kind) {
    case SelectionKind::Body: return QStringLiteral("Body");
    case SelectionKind::Face: return QStringLiteral("Face");
    case SelectionKind::Edge: return QStringLiteral("Edge");
    case SelectionKind::Vertex: return QStringLiteral("Vertex");
    case SelectionKind::Node: return QStringLiteral("Node");
    case SelectionKind::Element: return QStringLiteral("Element");
    case SelectionKind::Facet: return QStringLiteral("Facet");
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

bool compatibleWindowOperation(const SelectionOperation existing,
                               const SelectionOperation incoming) noexcept
{
    // Replace window bridge contract'i ilk engineering hit'te Replace,
    // kalan hit'lerde Add üretir. Bunlar tek atomik Replace batch'inin parçalarıdır.
    return existing == incoming
        || (existing == SelectionOperation::Replace && incoming == SelectionOperation::Add);
}

void cancelPendingWindowBatch(SelectionCoordinator *coordinator)
{
    pendingWindowSelectionBatches.remove(coordinator);
}

void flushPendingWindowBatch(SelectionCoordinator *coordinator)
{
    auto it = pendingWindowSelectionBatches.find(coordinator);
    if (it == pendingWindowSelectionBatches.end()) {
        return;
    }

    PendingWindowSelectionBatch batch = std::move(it.value());
    pendingWindowSelectionBatches.erase(it);
    SelectionManager *manager = coordinator != nullptr ? coordinator->selectionManager() : nullptr;
    if (manager == nullptr || !batch.operation.has_value()) {
        return;
    }

    // SelectionManager::apply(batch) engineering identity'leri de-duplicate eder,
    // primary state'i tek transaction'da kurar ve en fazla bir selectionChanged
    // signal'i üretir. Raw VTK/display sırası Undo/Redo tarihçesine girmez.
    (void)manager->apply(batch.items, *batch.operation);
}

void armPendingWindowBatch(SelectionCoordinator *coordinator)
{
    if (coordinator == nullptr) {
        return;
    }

    auto existing = pendingWindowSelectionBatches.find(coordinator);
    if (existing != pendingWindowSelectionBatches.end() && existing->operation.has_value()) {
        // Normalde her Qt pointer event'i arasında queued flush çalışır. Yine de
        // yeni bir preselection-clear gelmeden önce önceki batch dolu kalmışsa
        // engineering state'i kaybetmek yerine önce onu tamamla.
        flushPendingWindowBatch(coordinator);
    }

    PendingWindowSelectionBatch &batch = pendingWindowSelectionBatches[coordinator];
    batch.armed = true;
    batch.domain.reset();
    batch.operation.reset();
    batch.items.clear();
    if (batch.flushScheduled) {
        return;
    }
    batch.flushScheduled = true;

    QTimer::singleShot(0, coordinator, [coordinator] {
        flushPendingWindowBatch(coordinator);
    });
}

bool queuePendingWindowItem(SelectionCoordinator *coordinator,
                            const SelectionDomain domain,
                            const SelectionItem &item,
                            const SelectionOperation operation)
{
    auto it = pendingWindowSelectionBatches.find(coordinator);
    if (it == pendingWindowSelectionBatches.end() || !it->armed) {
        return false;
    }

    PendingWindowSelectionBatch &batch = it.value();
    if (!batch.operation.has_value()) {
        batch.domain = domain;
        batch.operation = operation;
    } else if (!batch.domain.has_value() || *batch.domain != domain
               || !compatibleWindowOperation(*batch.operation, operation)) {
        // Farklı domain/semantik aynı batch'e sessizce karışamaz. Önce bekleyen
        // state'i tamamla; caller mevcut hit'i normal single-selection olarak
        // işleyebilir.
        flushPendingWindowBatch(coordinator);
        return false;
    }

    batch.items.push_back(item);
    return true;
}

bool hasPendingWindowBatch(SelectionCoordinator *coordinator)
{
    const auto it = pendingWindowSelectionBatches.constFind(coordinator);
    return it != pendingWindowSelectionBatches.constEnd() && it->armed;
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

    if (services_.project == nullptr || services_.geometry == nullptr || services_.mesh == nullptr
        || navigator_ == nullptr || graphics_ == nullptr || details_ == nullptr) {
        return;
    }

    selection_ = new SelectionManager(this);
    bridge_ = new ViewportSelectionBridge(graphics_->viewport(), this);
    meshBridge_ = new ViewportMeshSelectionBridge(graphics_->viewport(), this);

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

    connect(meshBridge_, &ViewportMeshSelectionBridge::selectionRequested,
            this, &SelectionCoordinator::handleMeshSelection);
    connect(meshBridge_, &ViewportMeshSelectionBridge::preselectionRequested,
            this, &SelectionCoordinator::handleMeshPreselection);
    connect(meshBridge_, &ViewportMeshSelectionBridge::contextMenuRequested,
            this, &SelectionCoordinator::handleMeshContextMenu);

    const auto clearSelection = [this] {
        cancelPendingWindowBatch(this);
        if (selection_ != nullptr) {
            selection_->clearPreselection();
            (void)selection_->clear();
        }
    };
    const auto clearPreselection = [this] {
        if (selection_ != nullptr) {
            selection_->clearPreselection();
        }

        // WindowCommit bridge contract'i area hit'lerinden hemen önce bu signal'i
        // synchronous yayar. Hover miss/Leave de aynı signal'i kullanabildiği için
        // batch bir sonraki event-loop turunda otomatik kapanır; hit gelmezse state
        // değişmez. Normal click selectionRequested yolu anında çalışmaya devam eder.
        armPendingWindowBatch(this);
    };
    connect(bridge_, &ViewportSelectionBridge::selectionClearRequested, this, clearSelection);
    connect(bridge_, &ViewportSelectionBridge::preselectionClearRequested, this, clearPreselection);
    connect(meshBridge_, &ViewportMeshSelectionBridge::selectionClearRequested, this, clearSelection);
    connect(meshBridge_, &ViewportMeshSelectionBridge::preselectionClearRequested, this, clearPreselection);

    connect(selection_, &SelectionManager::selectionChanged,
            this, &SelectionCoordinator::handleSelectionChanged);
    connect(selection_, &SelectionManager::preselectionChanged,
            this, &SelectionCoordinator::handlePreselectionChanged);

    connect(services_.geometry, &GeometryService::changed, this, [this] {
        cancelPendingWindowBatch(this);
        if (selection_ != nullptr) {
            selection_->clearPreselection();
            (void)selection_->clear();
        }
        bridge_->clearScene();
        meshBridge_->clearScene();
        meshBridge_->setInputEnabled(false);
        QTimer::singleShot(0, this, [this] { refreshSelectionScene(); });
    });

    connect(services_.mesh, &MeshService::changed, this, [this] {
        cancelPendingWindowBatch(this);
        QTimer::singleShot(0, this, [this] { refreshSelectionScene(); });
    });

    if (services_.commands != nullptr) {
        connect(services_.commands, &DocumentCommandManager::documentMutated,
                this, [this] {
                    cancelPendingWindowBatch(this);
                    QTimer::singleShot(0, this, [this] { refreshSelectionScene(); });
                });
    }

    connect(details_, &DetailsHost::modelEdited,
            this, [this] {
                cancelPendingWindowBatch(this);
                QTimer::singleShot(0, this, [this] { refreshSelectionScene(); });
            });
    connect(details_->geometryPage(), &GeometryDetails::tessellationQualityChanged,
            this, [this](const double value) {
                cancelPendingWindowBatch(this);
                tessellationDeflection_ = value;
                QTimer::singleShot(0, this, [this] { refreshSelectionScene(); });
            });

    connect(this, &QObject::destroyed, [this] {
        cancelPendingWindowBatch(this);
    });

    configurePolicy(graphics_->selectionFilter());
    QTimer::singleShot(0, this, [this] {
        refreshSelectionScene();
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
    const SelectionDomain domain = selectionDomainForFilter(filter);

    if (domain == SelectionDomain::Geometry) {
        bridge_->setActiveKind(kind);
        SelectionPolicy policy = SelectionPolicy::preset(SelectionPolicyPreset::NeutralGeometry);
        policy.allowedKinds = {kind};
        policy.allowMultiple = true;
        selection_->setPolicy(policy);
        return;
    }

    meshBridge_->setActiveKind(kind);
    SelectionPolicyPreset preset = SelectionPolicyPreset::MeshNodeScope;
    if (kind == SelectionKind::Element) {
        preset = SelectionPolicyPreset::MeshElementScope;
    } else if (kind == SelectionKind::Facet) {
        preset = SelectionPolicyPreset::MeshFacetScope;
    }
    selection_->setPolicy(SelectionPolicy::preset(preset));
}

void SelectionCoordinator::refreshSelectionScene()
{
    if (bridge_ == nullptr || meshBridge_ == nullptr || graphics_ == nullptr
        || services_.geometry == nullptr || services_.mesh == nullptr) {
        return;
    }

    ViewportWidget *viewport = graphics_->viewport();
    if (viewport == nullptr) {
        return;
    }

    // Named Selection normal görünümünde viewport domain'i ObjectType'tan değil
    // persistent ScopeReference'tan çözülür. Overlay transient SelectionManager
    // state'i değildir; stale scope eski engineering ID'lerini asla göstermez.
    const NamedSelectionDefinition *persistentDefinition = nullptr;
    ScopeSelectionItemsResult persistentScopeView;
    std::optional<SelectionFilter> persistentFilter;
    if (editingNamedSelection_ == InvalidObjectId && navigator_ != nullptr
        && services_.project != nullptr && services_.namedSelections != nullptr) {
        const ObjectId currentObject = navigator_->selectedObject();
        if (services_.project->typeOf(currentObject) == ObjectType::NamedSelection) {
            persistentDefinition = services_.namedSelections->byId(currentObject);
            if (persistentDefinition != nullptr && !persistentDefinition->scope.entities.isEmpty()) {
                const ScopeEntityReference &first = persistentDefinition->scope.entities.front();
                persistentFilter = selectionFilterForKind(first.kind);
                if (!persistentFilter.has_value()) {
                    persistentDefinition = nullptr;
                } else if (first.domain == SelectionDomain::Geometry
                           && services_.geometry->summary().hasGeometry) {
                    viewport->setContext(ViewportContext::Geometry);
                    graphics_->setContextLabel(tr("Geometry — Named Selection"));
                    persistentScopeView = selectionItemsForGeometryScope(
                        persistentDefinition->scope, services_.geometry->document());
                } else if (first.domain == SelectionDomain::Mesh && services_.mesh->hasMesh()) {
                    viewport->setContext(ViewportContext::Mesh);
                    graphics_->setContextLabel(tr("Mesh — Named Selection"));
                    viewport->showMesh(services_.mesh->mesh(), false);
                    persistentScopeView = selectionItemsForMeshScope(
                        persistentDefinition->scope, services_.mesh->mesh(), services_.mesh->generation());
                } else {
                    selection_->clearPreselection();
                    (void)selection_->clear();
                    bridge_->clearScene();
                    meshBridge_->setInputEnabled(false);
                    meshBridge_->clearScene();
                    graphics_->setSelectionFilterDomain(std::nullopt);
                    graphics_->setSelectionLabel(tr("%1 · Out of Date").arg(persistentDefinition->name));
                    if (status_ != nullptr) {
                        status_->setSelection(tr("%1  •  Out of Date  •  Persistent Scope")
                                                  .arg(persistentDefinition->name));
                    }
                    return;
                }

                selection_->clearPreselection();
                if (!selection_->items().isEmpty()) {
                    (void)selection_->clear();
                }
            }
        }
    }

    const ViewportContext context = viewport->context();
    const GeometrySummary summary = services_.geometry->summary();

    if (context == ViewportContext::Geometry) {
        meshBridge_->setInputEnabled(false);
        meshBridge_->clearScene();
        graphics_->setSelectionFilterDomain(SelectionDomain::Geometry);
        graphics_->setMeshSelectionAvailable(false, false, false);

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
        if (persistentDefinition != nullptr && persistentFilter.has_value()) {
            graphics_->setSelectionFilter(*persistentFilter);
        } else if (isMeshFilter(graphics_->selectionFilter())) {
            graphics_->setSelectionFilter(SelectionFilter::Body);
        }
        configurePolicy(graphics_->selectionFilter());
        (void)selection_->invalidateGeometryRevision(summary.revision);
        bridge_->setSelection(persistentDefinition != nullptr
                                  ? (persistentScopeView.success()
                                         ? persistentScopeView.items
                                         : QVector<SelectionItem>{})
                                  : selection_->items());
        bridge_->setPreselection(persistentDefinition != nullptr
                                     ? std::nullopt
                                     : selection_->preselection());
        if (persistentDefinition != nullptr) {
            if (persistentScopeView.success()) {
                graphics_->setSelectionLabel(
                    tr("%1 · %2 entities · Persistent Scope")
                        .arg(persistentDefinition->name).arg(persistentScopeView.items.size()));
                if (status_ != nullptr) {
                    status_->setSelection(
                        tr("%1  •  %2 entities  •  Persistent Scope")
                            .arg(persistentDefinition->name).arg(persistentScopeView.items.size()));
                }
            } else {
                graphics_->setSelectionLabel(tr("%1 · Out of Date").arg(persistentDefinition->name));
                if (status_ != nullptr) {
                    status_->setSelection(
                        tr("%1  •  Out of Date  •  Persistent Scope").arg(persistentDefinition->name));
                }
            }
        }
        return;
    }

    // Geometry picker/overlay bu noktadan sonra aktif değildir.
    bridge_->clearScene();
    graphics_->setTopologySelectionAvailable(false, false, false);

    if (context == ViewportContext::Mesh && services_.mesh->hasMesh()) {
        graphics_->setSelectionFilterDomain(SelectionDomain::Mesh);
        graphics_->setMeshSelectionAvailable(true, true, true);
        if (persistentDefinition != nullptr && persistentFilter.has_value()) {
            graphics_->setSelectionFilter(*persistentFilter);
        } else if (!isMeshFilter(graphics_->selectionFilter())) {
            graphics_->setSelectionFilter(SelectionFilter::Node);
        }
        configurePolicy(graphics_->selectionFilter());

        if (!meshBridge_->setScene(services_.mesh->mesh(), services_.mesh->generation())) {
            meshBridge_->setInputEnabled(false);
            meshBridge_->clearScene();
            graphics_->setMeshSelectionAvailable(false, false, false);
            graphics_->setSelectionFilterDomain(std::nullopt);
            selection_->clearPreselection();
            (void)selection_->clear();
            return;
        }

        meshBridge_->setInputEnabled(true);
        (void)selection_->invalidateMeshGeneration(services_.mesh->generation());
        meshBridge_->setSelection(persistentDefinition != nullptr
                                      ? (persistentScopeView.success()
                                             ? persistentScopeView.items
                                             : QVector<SelectionItem>{})
                                      : selection_->items());
        meshBridge_->setPreselection(persistentDefinition != nullptr
                                         ? std::nullopt
                                         : selection_->preselection());
        if (persistentDefinition != nullptr) {
            if (persistentScopeView.success()) {
                graphics_->setSelectionLabel(
                    tr("%1 · %2 entities · Persistent Scope")
                        .arg(persistentDefinition->name).arg(persistentScopeView.items.size()));
                if (status_ != nullptr) {
                    status_->setSelection(
                        tr("%1  •  %2 entities  •  Persistent Scope")
                            .arg(persistentDefinition->name).arg(persistentScopeView.items.size()));
                }
            } else {
                graphics_->setSelectionLabel(tr("%1 · Out of Date").arg(persistentDefinition->name));
                if (status_ != nullptr) {
                    status_->setSelection(
                        tr("%1  •  Out of Date  •  Persistent Scope").arg(persistentDefinition->name));
                }
            }
        }
        return;
    }

    meshBridge_->setInputEnabled(false);
    meshBridge_->clearScene();
    graphics_->setMeshSelectionAvailable(false, false, false);
    graphics_->setSelectionFilterDomain(std::nullopt);
    selection_->clearPreselection();
    (void)selection_->clear();
    graphics_->setSelectionLabel(QString());

    // CAD backdrop kullanan non-selection bağlamlarında bütün imported Body'ler
    // görünür kalır. Gerçek FEM mesh/results sahnesi varsa üzerine yazılmaz.
    const bool hasMesh = services_.mesh->hasMesh();
    if (summary.hasGeometry && usesPassiveCadBackdrop(context, hasMesh)) {
        const auto surfaces = services_.geometry->displayTopologyScene(tessellationDeflection_);
        if (surfaces.size() == static_cast<qsizetype>(summary.bodyCount)) {
            viewport->showGeometry(surfaces);
        }
    }
}

void SelectionCoordinator::handleNavigatorSelection(const ObjectId id)
{
    if (syncingNavigator_ || services_.project == nullptr || services_.geometry == nullptr) {
        return;
    }

    refreshSelectionScene();

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
        || kind != selectionKindForFilter(graphics_->selectionFilter())
        || selectionDomainForFilter(graphics_->selectionFilter()) != SelectionDomain::Geometry) {
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

std::optional<SelectionItem> SelectionCoordinator::meshSelectionItemForHit(const SelectionKind kind,
                                                                           const qint64 meshEntityId) const
{
    if (graphics_ == nullptr || services_.mesh == nullptr || !services_.mesh->hasMesh()
        || selectionDomainForFilter(graphics_->selectionFilter()) != SelectionDomain::Mesh
        || kind != selectionKindForFilter(graphics_->selectionFilter())) {
        return std::nullopt;
    }

    const MeshEntityId id = static_cast<MeshEntityId>(meshEntityId);
    if (id == InvalidMeshId) {
        return std::nullopt;
    }
    const auto &mesh = services_.mesh->mesh();
    bool exists = false;
    if (kind == SelectionKind::Node) {
        exists = mesh.findNode(id) != nullptr;
    } else if (kind == SelectionKind::Element) {
        exists = mesh.findElement(id) != nullptr;
    } else if (kind == SelectionKind::Facet) {
        for (const auto &facet : mesh.boundaryFacets) {
            if (facet.id == id) {
                exists = true;
                break;
            }
        }
    }
    if (!exists) {
        return std::nullopt;
    }

    SelectionItem item;
    item.domain = SelectionDomain::Mesh;
    item.kind = kind;
    item.meshEntityId = id;
    item.sourceRevision = services_.mesh->generation();
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

void SelectionCoordinator::showSelectionContextMenu(const ObjectId objectId,
                                                     const QPoint &globalPosition)
{
    if (window_ == nullptr || objectId == InvalidObjectId) {
        return;
    }

    QMenu *menu = window_->buildContextMenu(objectId, window_);
    if (menu == nullptr) {
        return;
    }

    const ScopeReferenceBuildResult scope = currentPersistentScope();
    if (scope.success() && services_.namedSelections != nullptr && services_.commands != nullptr) {
        const bool geometryFaceScope = !scope.scope.entities.isEmpty()
            && std::all_of(scope.scope.entities.cbegin(), scope.scope.entities.cend(),
                           [](const ScopeEntityReference &entity) {
                               return entity.domain == SelectionDomain::Geometry
                                   && entity.kind == SelectionKind::Face;
                           });
        const ObjectId analysisId = window_->currentAnalysis();

        // ANSYS/COMSOL benzeri hızlı authoring yolu: viewport'ta seçilmiş gerçek
        // CAD Face seti önce persistent Named Selection olarak dondurulur, ardından
        // Fixed Support / Force bu ObjectId'ye bağlanır. BC/load içine topology ID
        // kopyalanmaz; existing stale-detection ve resolver tek doğruluk kaynağı
        // olarak kalır. İki document mutation tek Undo transaction'dır.
        const auto createBoundaryFromSelection = [this, analysisId](const bool force) {
            if (window_ == nullptr || services_.commands == nullptr || services_.analysis == nullptr
                || analysisId == InvalidObjectId) {
                return;
            }
            const ScopeReferenceBuildResult current = currentGeometryScope();
            const bool currentIsFaceScope = current.success() && !current.scope.entities.isEmpty()
                && std::all_of(current.scope.entities.cbegin(), current.scope.entities.cend(),
                               [](const ScopeEntityReference &entity) {
                                   return entity.domain == SelectionDomain::Geometry
                                       && entity.kind == SelectionKind::Face;
                               });
            if (!currentIsFaceScope) {
                return;
            }

            const QString transactionName = force ? tr("Add Force from Selection")
                                                  : tr("Add Fixed Support from Selection");
            services_.commands->beginMacro(transactionName);
            const NamedSelectionCreateResult named = createNamedSelectionFromCurrentSelection(
                force ? tr("Force Scope") : tr("Fixed Support Scope"));

            ObjectId createdBoundary = InvalidObjectId;
            if (named.success()) {
                if (force) {
                    LoadDefinition definition;
                    definition.scopingMethod = BoundaryScopingMethod::NamedSelection;
                    definition.namedSelectionId = named.id;
                    auto *command = new commands::CreateForceCommand(
                        services_, analysisId, definition, -1, tr("Add Force"));
                    services_.commands->push(command);
                    createdBoundary = command->createdId();
                } else {
                    SupportDefinition definition;
                    definition.scopingMethod = BoundaryScopingMethod::NamedSelection;
                    definition.namedSelectionId = named.id;
                    auto *command = new commands::CreateFixedSupportCommand(
                        services_, analysisId, definition, -1, tr("Add Fixed Support"));
                    services_.commands->push(command);
                    createdBoundary = command->createdId();
                }
            }
            services_.commands->endMacro();

            if (createdBoundary != InvalidObjectId) {
                window_->selectObject(createdBoundary);
                window_->syncAll();
            }
        };

        QAction *insertBefore = menu->actions().isEmpty() ? nullptr : menu->actions().constFirst();
        if (geometryFaceScope && analysisId != InvalidObjectId && services_.analysis != nullptr) {
            auto *supportAction = new QAction(tr("Fixed Support from Selection"), menu);
            supportAction->setToolTip(
                tr("Seçili CAD Face kapsamını kalıcı scope olarak kaydet ve Fixed Support oluştur"));
            connect(supportAction, &QAction::triggered, this,
                    [createBoundaryFromSelection] { createBoundaryFromSelection(false); });

            auto *forceAction = new QAction(tr("Force from Selection"), menu);
            forceAction->setToolTip(
                tr("Seçili CAD Face kapsamını kalıcı scope olarak kaydet ve Force oluştur"));
            connect(forceAction, &QAction::triggered, this,
                    [createBoundaryFromSelection] { createBoundaryFromSelection(true); });

            if (insertBefore == nullptr) {
                menu->addAction(supportAction);
                menu->addAction(forceAction);
            } else {
                menu->insertAction(insertBefore, supportAction);
                menu->insertAction(insertBefore, forceAction);
            }
        }

        auto *createAction = new QAction(tr("Create Named Selection"), menu);
        createAction->setToolTip(tr("Geçerli CAD/FEM seçimini kalıcı engineering scope olarak kaydet"));
        connect(createAction, &QAction::triggered, this, [this] {
            const NamedSelectionCreateResult created = createNamedSelectionFromCurrentSelection();
            if (!created.success() || window_ == nullptr) {
                return;
            }
            // Persistence tamamlandıktan sonra transient seçim document state'i
            // değildir; yeni Named Selection current project object yapılır.
            window_->selectObject(created.id);
        });

        if (insertBefore == nullptr) {
            menu->addAction(createAction);
        } else {
            menu->insertAction(insertBefore, createAction);
            menu->insertSeparator(insertBefore);
        }
    }

    menu->exec(globalPosition);
    menu->deleteLater();
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
        if (!hasPendingWindowBatch(this)) {
            (void)selection_->clear();
        }
        return;
    }

    if (queuePendingWindowItem(this, SelectionDomain::Geometry, *item, operation)) {
        return;
    }
    (void)selection_->apply(*item, operation);
}

void SelectionCoordinator::handleViewportPreselection(const SelectionKind kind,
                                                       const quint64 bodyId,
                                                       const quint64 geometryId)
{
    if (selection_ != nullptr) {
        selection_->setPreselection(selectionItemForHit(kind, bodyId, geometryId));
    }
}

void SelectionCoordinator::handleViewportContextMenu(const SelectionKind kind,
                                                      const quint64 bodyId,
                                                      const quint64 geometryId,
                                                      const QPoint &globalPosition)
{
    if (selection_ == nullptr || window_ == nullptr) {
        return;
    }
    flushPendingWindowBatch(this);
    const auto item = selectionItemForHit(kind, bodyId, geometryId);
    if (!item.has_value()) {
        return;
    }
    selection_->clearPreselection();
    if (!selectionContains(*item)) {
        (void)selection_->apply(*item, SelectionOperation::Replace);
    }
    const GeometryEntityId body = parentBodyFor(*item);
    const ObjectId object = bodyObjectForGeometryId(body);
    if (object == InvalidObjectId) {
        return;
    }
    (void)syncNavigatorToGeometryBody(body);
    showSelectionContextMenu(object, globalPosition);
}

void SelectionCoordinator::handleMeshSelection(const SelectionKind kind,
                                                const qint64 meshEntityId,
                                                const SelectionOperation operation)
{
    if (selection_ == nullptr) {
        return;
    }
    selection_->clearPreselection();
    const auto item = meshSelectionItemForHit(kind, meshEntityId);
    if (!item.has_value()) {
        if (!hasPendingWindowBatch(this)) {
            (void)selection_->clear();
        }
        return;
    }

    if (queuePendingWindowItem(this, SelectionDomain::Mesh, *item, operation)) {
        return;
    }
    (void)selection_->apply(*item, operation);
}

void SelectionCoordinator::handleMeshPreselection(const SelectionKind kind, const qint64 meshEntityId)
{
    if (selection_ != nullptr) {
        selection_->setPreselection(meshSelectionItemForHit(kind, meshEntityId));
    }
}

void SelectionCoordinator::handleMeshContextMenu(const SelectionKind kind,
                                                  const qint64 meshEntityId,
                                                  const QPoint &globalPosition)
{
    if (selection_ == nullptr || window_ == nullptr || services_.project == nullptr) {
        return;
    }
    flushPendingWindowBatch(this);
    const auto item = meshSelectionItemForHit(kind, meshEntityId);
    if (!item.has_value()) {
        return;
    }
    selection_->clearPreselection();
    if (!selectionContains(*item)) {
        (void)selection_->apply(*item, SelectionOperation::Replace);
    }
    const ObjectId meshObject = services_.project->meshNode();
    if (meshObject == InvalidObjectId) {
        return;
    }
    (void)syncNavigatorToObject(meshObject);
    showSelectionContextMenu(meshObject, globalPosition);
}

void SelectionCoordinator::handleSelectionChanged()
{
    if (selection_ == nullptr) {
        return;
    }
    (void)syncNavigatorToPrimary();
    if (bridge_ != nullptr) {
        bridge_->setSelection(selection_->items());
    }
    if (meshBridge_ != nullptr) {
        meshBridge_->setSelection(selection_->items());
    }
    updateFeedback();
}

void SelectionCoordinator::handlePreselectionChanged()
{
    if (selection_ == nullptr) {
        return;
    }
    if (bridge_ != nullptr) {
        bridge_->setPreselection(selection_->preselection());
    }
    if (meshBridge_ != nullptr) {
        meshBridge_->setPreselection(selection_->preselection());
    }
    updateFeedback();
}

bool SelectionCoordinator::syncNavigatorToPrimary()
{
    if (selection_ == nullptr || services_.project == nullptr) {
        return false;
    }
    const auto primary = selection_->primary();
    if (!primary.has_value()) {
        return false;
    }
    if (primary->domain == SelectionDomain::Geometry) {
        return syncNavigatorToGeometryBody(parentBodyFor(*primary));
    }
    if (primary->domain == SelectionDomain::Mesh) {
        return syncNavigatorToObject(services_.project->meshNode());
    }
    return false;
}

bool SelectionCoordinator::syncNavigatorToGeometryBody(const GeometryEntityId bodyId)
{
    if (bodyId == InvalidGeometryId) {
        return false;
    }
    return syncNavigatorToObject(bodyObjectForGeometryId(bodyId));
}

bool SelectionCoordinator::syncNavigatorToObject(const ObjectId objectId)
{
    if (objectId == InvalidObjectId || window_ == nullptr || navigator_ == nullptr
        || details_ == nullptr || navigator_->selectedObject() == objectId) {
        return false;
    }

    syncingNavigator_ = true;
    {
        const QSignalBlocker navigatorSignals(navigator_);
        navigator_->selectObject(objectId);
    }

    // Viewport-originated subentity selection current project object'i izler,
    // fakat MainWindow::selectObject() çağrılmaz; o yol sahne/kamerayı yeniden
    // kurabilir. Selection overlay aynı render scene üzerinde kalır.
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

        if (primary.domain == SelectionDomain::Mesh) {
            if (details_ != nullptr) {
                details_->setSelectionSummary(tr("SELECTION  ·  %1 %2  ·  Mesh Gen %3")
                                                  .arg(items.size()).arg(kind).arg(primary.sourceRevision));
            }
            if (status_ != nullptr) {
                status_->setSelection(tr("%1 %2  •  Mesh Generation %3  •  Global Coordinate System")
                                          .arg(items.size()).arg(kind).arg(primary.sourceRevision));
            }
            return;
        }

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
    if (hover.has_value() && hover->domain == SelectionDomain::Geometry
        && graphics_->viewport()->context() == ViewportContext::Geometry) {
        QString label = geometryEntityName(hover->geometryEntityId);
        if (label.isEmpty()) {
            label = selectionKindText(hover->kind);
        }
        graphics_->setSelectionLabel(tr("%1  ·  ön seçim").arg(label));
    } else if (hover.has_value() && hover->domain == SelectionDomain::Mesh
               && graphics_->viewport()->context() == ViewportContext::Mesh) {
        graphics_->setSelectionLabel(tr("%1 %2  ·  ön seçim")
                                         .arg(selectionKindText(hover->kind))
                                         .arg(static_cast<qint64>(hover->meshEntityId)));
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