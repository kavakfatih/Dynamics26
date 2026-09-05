#pragma once

// Dynamics26 Alpha.3.6+ / Beta.1 — application-level transient selection coordinator.
//
// ProjectModel current object, CAD topology selection ve generated FEM mesh
// selection ayni merkezi SelectionManager etrafinda koordine edilir. Geometry ve
// Mesh kimlik uzaylari ayridir; raw selection document undo/redo verisi değildir.
// Persistent Named Selection ve Contact Source/Target üretimi raw picker
// ID'lerinden değil, burada doğrulanan ScopeReference / SelectionItem akışından geçer.

#include "../commands/ContactCommands.h"
#include "../commands/NamedSelectionCommands.h"
#include "../core/DocumentCommandManager.h"
#include "../core/ScopeReferenceBuilder.h"
#include "../core/ScopeSelectionBridge.h"
#include "../core/SelectionManager.h"
#include "../core/ServiceContext.h"
#include "../services/ContactService.h"
#include "../services/NamedSelectionService.h"
#include "../viewport/ViewportMeshSelectionBridge.h"
#include "../viewport/ViewportSelectionBridge.h"
#include "DetailsHost.h"
#include "Dynamics26MainWindow.h"
#include "GraphicsWorkspace.h"
#include "ProjectNavigator.h"

#include <QMetaObject>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QMenu>
#include <QString>

#include <optional>

namespace d26 {

class EngineeringStatusBar;

enum class BoundaryFromSelectionKind {
    FixedSupport = 0,
    TotalForce
};

struct BoundaryFromSelectionCreateResult {
    ObjectId namedSelectionId{InvalidObjectId};
    ObjectId boundaryConditionId{InvalidObjectId};
    ScopeReferenceBuildError buildError{ScopeReferenceBuildError::None};

    [[nodiscard]] bool success() const noexcept
    {
        return buildError == ScopeReferenceBuildError::None
            && namedSelectionId != InvalidObjectId
            && boundaryConditionId != InvalidObjectId;
    }
};

class SelectionCoordinator final : public QObject
{
public:
    explicit SelectionCoordinator(Dynamics26MainWindow *window, QObject *parent = nullptr);

    [[nodiscard]] SelectionManager *selectionManager() const noexcept { return selection_; }
    [[nodiscard]] ScopeReferenceBuildResult currentGeometryScope() const;

    // Seçili gerçek CAD Face setini önce persistent Named Selection'a, sonra
    // BC/Load içindeki yalnız ObjectId referansına dönüştüren canonical hızlı
    // authoring yolu. İki document command tek Undo macro'sunda tutulur.
    [[nodiscard]] BoundaryFromSelectionCreateResult
        createBoundaryConditionFromCurrentFaceSelection(BoundaryFromSelectionKind kind);

    // UI persistent engineering scope istediğinde SelectionManager/VTK internals
    // görmez. Aynı tek converter zinciri Geometry için CAD revision+persistentKey,
    // Mesh için generation+MeshEntityId doğrulamasını uygular.
    [[nodiscard]] ScopeReferenceBuildResult currentPersistentScope() const
    {
        ScopeReferenceBuildResult result;
        if (selection_ == nullptr || selection_->items().isEmpty()) {
            result.error = ScopeReferenceBuildError::EmptySelection;
            return result;
        }

        const QVector<SelectionItem> &items = selection_->items();
        switch (items.front().domain) {
        case SelectionDomain::Geometry:
            if (services_.mesh == nullptr) {
                result.error = ScopeReferenceBuildError::UnsupportedDomain;
                return result;
            }
            return buildGeometryScopeReference(items, services_.mesh->selectionGeometryDocument());
        case SelectionDomain::Mesh:
            if (services_.mesh == nullptr) {
                result.error = ScopeReferenceBuildError::UnsupportedDomain;
                return result;
            }
            return buildMeshScopeReference(items, services_.mesh->mesh(), services_.mesh->generation());
        case SelectionDomain::ProjectObject:
            result.error = ScopeReferenceBuildError::UnsupportedDomain;
            return result;
        }
        result.error = ScopeReferenceBuildError::UnsupportedDomain;
        return result;
    }

    // Persistent object oluşturma için tek application bridge. Transient seçim
    // document Undo state'i değildir. Scope doğrulandıktan sonra oluşturma
    // QUndoStack'e tek bir mühendislik komutu olarak girer.
    [[nodiscard]] NamedSelectionCreateResult createNamedSelectionFromCurrentSelection(
        const QString &requestedName = QStringLiteral("Named Selection"))
    {
        NamedSelectionCreateResult result;
        if (selection_ == nullptr || services_.namedSelections == nullptr
            || services_.commands == nullptr) {
            result.buildError = ScopeReferenceBuildError::UnsupportedDomain;
            return result;
        }

        const ScopeReferenceBuildResult build = currentPersistentScope();
        result.buildError = build.error;
        if (!build.success()) {
            return result;
        }

        NamedSelectionDefinition definition;
        definition.name = requestedName;
        definition.scope = build.scope;
        auto *command = new commands::CreateNamedSelectionCommand(services_, definition);
        services_.commands->push(command);
        result.id = command->createdId();
        if (result.id == InvalidObjectId) {
            result.buildError = ScopeReferenceBuildError::UnsupportedDomain;
        }
        return result;
    }

    // Edit Scope Apply aşamasının tek transaction bridge'i. UI hiçbir zaman
    // NamedSelectionService::replaceScope() çağırmaz; önce transient CAD/FEM
    // selection aynı canonical builder zincirinden doğrulanır, ardından değişim
    // document Undo stack'ine tek ReplaceNamedSelectionScopeCommand olarak girer.
    // Böylece pointer hareketleri Undo geçmişine karışmaz; yalnız Apply kalıcıdır.
    [[nodiscard]] ScopeReferenceBuildResult replaceNamedSelectionScopeFromCurrentSelection(
        const ObjectId target)
    {
        ScopeReferenceBuildResult result;
        if (selection_ == nullptr || services_.namedSelections == nullptr
            || services_.commands == nullptr || target == InvalidObjectId
            || services_.namedSelections->byId(target) == nullptr) {
            result.error = ScopeReferenceBuildError::UnsupportedDomain;
            return result;
        }

        result = currentPersistentScope();
        if (!result.success()) {
            return result;
        }

        services_.commands->push(
            new commands::ReplaceNamedSelectionScopeCommand(services_, target, result.scope));
        return result;
    }

    // Named Selection kapsam düzenleme oturumu document state DEĞİLDİR.
    // Navigator/Details Named Selection üzerinde kalırken yalnız viewport geçici
    // Geometry veya Mesh seçim bağlamına girer. Geçerli scope güvenli biçimde
    // preload edilir; stale scope eski ID'leri asla transient selection'a taşımaz.
    [[nodiscard]] bool beginNamedSelectionEdit(const ObjectId target)
    {
        if (editingNamedSelection_ != InvalidObjectId) {
            return editingNamedSelection_ == target;
        }
        if (editingContact_ != InvalidObjectId) {
            return false;
        }
        if (target == InvalidObjectId || selection_ == nullptr || graphics_ == nullptr
            || details_ == nullptr || navigator_ == nullptr || services_.namedSelections == nullptr) {
            return false;
        }

        const NamedSelectionDefinition *definition = services_.namedSelections->byId(target);
        if (definition == nullptr || definition->scope.entities.isEmpty()) {
            return false;
        }
        const ScopeEntityReference &first = definition->scope.entities.front();
        const auto filter = selectionFilterForKind(first.kind);
        if (!filter.has_value()) {
            return false;
        }
        if (first.domain == SelectionDomain::Geometry) {
            if (services_.mesh == nullptr
                || services_.mesh->selectionGeometryDocument().entitiesOfKind(
                       femcae::geometry::GeometryEntityKind::Body).empty()) {
                return false;
            }
        } else if (first.domain == SelectionDomain::Mesh) {
            if (services_.mesh == nullptr || !services_.mesh->hasMesh()) {
                return false;
            }
        } else {
            return false;
        }

        editingNamedSelection_ = target;
        editDomain_ = first.domain;
        editKind_ = first.kind;
        previousFilter_ = graphics_->selectionFilter();

        // Normal transient selection -> Project Navigator current-object senkronu
        // edit sırasında kapatılır. Selection overlay ve Inspector özeti yaşamaya
        // devam eder; böylece kullanıcı Face/Node seçerken Named Selection current
        // project object olarak kalır.
        disconnect(selection_, &SelectionManager::selectionChanged,
                   this, &SelectionCoordinator::handleSelectionChanged);
        editSelectionConnection_ = connect(selection_, &SelectionManager::selectionChanged,
                                           this, [this] {
            if (bridge_ != nullptr) {
                bridge_->setSelection(selection_->items());
            }
            if (meshBridge_ != nullptr) {
                meshBridge_->setSelection(selection_->items());
            }
            updateFeedback();
            if (details_ != nullptr) {
                details_->refresh();
            }
        });

        // Navigator'da başka bir project object seçmek explicit Cancel anlamına
        // gelir. Yalnız coordinator slot'u değiştirilir; MainWindow'ın kendi
        // object-selection sinyali engellenmez.
        disconnect(navigator_, &ProjectNavigator::objectSelected,
                   this, &SelectionCoordinator::handleNavigatorSelection);
        editNavigatorConnection_ = connect(navigator_, &ProjectNavigator::objectSelected,
                                           this, [this](const ObjectId id) {
            if (editingNamedSelection_ != InvalidObjectId && id != editingNamedSelection_) {
                cancelNamedSelectionEdit();
            }
        });

        // Edit sırasında sağ tık generic object context menu açıp Navigator'ı
        // Body/Mesh'e taşımamalıdır. Pointer selection soldaki standart akıştan
        // devam eder; context menu finish sonrasında geri bağlanır.
        if (bridge_ != nullptr) {
            disconnect(bridge_, &ViewportSelectionBridge::contextMenuRequested,
                       this, &SelectionCoordinator::handleViewportContextMenu);
        }
        if (meshBridge_ != nullptr) {
            disconnect(meshBridge_, &ViewportMeshSelectionBridge::contextMenuRequested,
                       this, &SelectionCoordinator::handleMeshContextMenu);
        }

        graphics_->setSelectionFilter(*filter);
        activateNamedSelectionEditViewport();

        selection_->clearPreselection();
        (void)selection_->clear();

        ScopeSelectionItemsResult preload;
        if (editDomain_ == SelectionDomain::Geometry) {
            preload = selectionItemsForGeometryScope(
                definition->scope, services_.mesh->selectionGeometryDocument());
        } else {
            preload = selectionItemsForMeshScope(definition->scope,
                                                 services_.mesh->mesh(), services_.mesh->generation());
        }
        editPreloadError_ = preload.error;
        if (preload.success()) {
            (void)selection_->apply(preload.items, SelectionOperation::Replace);
        } else {
            updateFeedback();
            details_->refresh();
        }
        return true;
    }

    [[nodiscard]] bool applyNamedSelectionEdit()
    {
        if (editingNamedSelection_ == InvalidObjectId) {
            return false;
        }
        const ObjectId target = editingNamedSelection_;
        const ScopeReferenceBuildResult result = replaceNamedSelectionScopeFromCurrentSelection(target);
        if (!result.success()) {
            return false;
        }
        finishNamedSelectionEdit();
        return true;
    }

    void cancelNamedSelectionEdit()
    {
        if (editingNamedSelection_ == InvalidObjectId) {
            return;
        }
        finishNamedSelectionEdit();
    }

    [[nodiscard]] bool namedSelectionEditActive() const noexcept
    {
        return editingNamedSelection_ != InvalidObjectId;
    }

    [[nodiscard]] ObjectId editingNamedSelection() const noexcept { return editingNamedSelection_; }

    // Contact Source/Target edit oturumları Named Selection ile aynı transient
    // selection altyapısını kullanır; ikinci bir picker veya ID uzayı oluşturmaz.
    // Her Contact side yalnız surface scope kabul eder: Geometry/Face veya
    // Mesh/Facet. Bir taraf tanımlıysa diğer taraf aynı domain'de düzenlenir.
    [[nodiscard]] bool beginContactSourceEdit(const ObjectId target)
    {
        return beginContactEdit(target, true);
    }

    [[nodiscard]] bool beginContactTargetEdit(const ObjectId target)
    {
        return beginContactEdit(target, false);
    }

    [[nodiscard]] bool applyContactEdit()
    {
        if (editingContact_ == InvalidObjectId || services_.contacts == nullptr
            || services_.commands == nullptr) {
            return false;
        }

        ScopeReferenceBuildResult result = currentPersistentScope();
        if (!result.success() || result.scope.entities.isEmpty()) {
            return false;
        }
        const ScopeEntityReference &first = result.scope.entities.front();
        if (first.domain != editDomain_ || first.kind != editKind_
            || !((first.domain == SelectionDomain::Geometry && first.kind == SelectionKind::Face)
                 || (first.domain == SelectionDomain::Mesh && first.kind == SelectionKind::Facet))) {
            return false;
        }

        const ContactDefinition *definition = services_.contacts->byId(editingContact_);
        if (definition == nullptr) {
            return false;
        }
        const ScopeReference &other = editingContactSource_
            ? definition->targetScope : definition->sourceScope;
        if (!other.entities.isEmpty() && other.entities.front().domain != first.domain) {
            return false;
        }

        if (editingContactSource_) {
            services_.commands->push(new commands::ReplaceContactSourceScopeCommand(
                services_, editingContact_, result.scope));
        } else {
            services_.commands->push(new commands::ReplaceContactTargetScopeCommand(
                services_, editingContact_, result.scope));
        }
        finishContactEdit();
        return true;
    }

    void cancelContactEdit()
    {
        if (editingContact_ != InvalidObjectId) {
            finishContactEdit();
        }
    }

    [[nodiscard]] bool contactEditActive() const noexcept
    {
        return editingContact_ != InvalidObjectId;
    }
    [[nodiscard]] ObjectId editingContact() const noexcept { return editingContact_; }
    [[nodiscard]] bool editingContactSource() const noexcept
    {
        return editingContact_ != InvalidObjectId && editingContactSource_;
    }
    [[nodiscard]] bool editingContactTarget() const noexcept
    {
        return editingContact_ != InvalidObjectId && !editingContactSource_;
    }

    [[nodiscard]] ScopeReferenceValidationError editPreloadError() const noexcept { return editPreloadError_; }

private:
    [[nodiscard]] static std::optional<SelectionFilter> selectionFilterForKind(const SelectionKind kind)
    {
        switch (kind) {
        case SelectionKind::Body: return SelectionFilter::Body;
        case SelectionKind::Face: return SelectionFilter::Face;
        case SelectionKind::Edge: return SelectionFilter::Edge;
        case SelectionKind::Vertex: return SelectionFilter::Vertex;
        case SelectionKind::Node: return SelectionFilter::Node;
        case SelectionKind::Element: return SelectionFilter::Element;
        case SelectionKind::Facet: return SelectionFilter::Facet;
        default: return std::nullopt;
        }
    }

    void activateNamedSelectionEditViewport()
    {
        if (graphics_ == nullptr || selection_ == nullptr) {
            return;
        }
        ViewportWidget *viewport = graphics_->viewport();
        if (viewport == nullptr) {
            return;
        }

        if (editDomain_ == SelectionDomain::Geometry && services_.mesh != nullptr) {
            viewport->setContext(ViewportContext::Geometry);
            graphics_->setContextLabel(tr("Geometry — Edit Named Selection"));
            const auto surfaces = services_.mesh->displaySelectionTopologyScene(tessellationDeflection_);
            if (!surfaces.isEmpty()) {
                viewport->showGeometry(surfaces);
            }
        } else if (editDomain_ == SelectionDomain::Mesh && services_.mesh != nullptr
                   && services_.mesh->hasMesh()) {
            viewport->setContext(ViewportContext::Mesh);
            graphics_->setContextLabel(tr("Mesh — Edit Named Selection"));
            viewport->showMesh(services_.mesh->mesh(), false);
        }
        refreshSelectionScene();
    }

    void finishNamedSelectionEdit()
    {
        editingNamedSelection_ = InvalidObjectId;
        editDomain_ = SelectionDomain::ProjectObject;
        editKind_ = SelectionKind::Object;
        editPreloadError_ = ScopeReferenceValidationError::None;

        QObject::disconnect(editSelectionConnection_);
        QObject::disconnect(editNavigatorConnection_);
        editSelectionConnection_ = {};
        editNavigatorConnection_ = {};

        if (selection_ != nullptr) {
            connect(selection_, &SelectionManager::selectionChanged,
                    this, &SelectionCoordinator::handleSelectionChanged);
            selection_->clearPreselection();
            (void)selection_->clear();
        }
        if (navigator_ != nullptr) {
            connect(navigator_, &ProjectNavigator::objectSelected,
                    this, &SelectionCoordinator::handleNavigatorSelection);
        }
        if (bridge_ != nullptr) {
            connect(bridge_, &ViewportSelectionBridge::contextMenuRequested,
                    this, &SelectionCoordinator::handleViewportContextMenu);
        }
        if (meshBridge_ != nullptr) {
            connect(meshBridge_, &ViewportMeshSelectionBridge::contextMenuRequested,
                    this, &SelectionCoordinator::handleMeshContextMenu);
        }

        if (graphics_ != nullptr) {
            graphics_->setSelectionFilter(previousFilter_);
        }
        restoreProjectObjectViewportAfterEdit();
    }

    [[nodiscard]] bool beginContactEdit(const ObjectId target, const bool sourceSide)
    {
        if (editingNamedSelection_ != InvalidObjectId) {
            return false;
        }
        if (editingContact_ != InvalidObjectId) {
            return editingContact_ == target && editingContactSource_ == sourceSide;
        }
        if (target == InvalidObjectId || selection_ == nullptr || graphics_ == nullptr
            || details_ == nullptr || navigator_ == nullptr || services_.contacts == nullptr
            || services_.commands == nullptr) {
            return false;
        }

        const ContactDefinition *definition = services_.contacts->byId(target);
        if (definition == nullptr) {
            return false;
        }
        const ScopeReference &selected = sourceSide ? definition->sourceScope : definition->targetScope;
        const ScopeReference &other = sourceSide ? definition->targetScope : definition->sourceScope;

        SelectionDomain domain = SelectionDomain::ProjectObject;
        SelectionKind kind = SelectionKind::Object;
        if (!selected.entities.isEmpty()) {
            domain = selected.entities.front().domain;
            kind = selected.entities.front().kind;
        } else if (!other.entities.isEmpty()) {
            domain = other.entities.front().domain;
            kind = other.entities.front().kind;
        } else if (services_.geometry != nullptr && services_.geometry->summary().hasGeometry) {
            domain = SelectionDomain::Geometry;
            kind = SelectionKind::Face;
        } else if (services_.mesh != nullptr && services_.mesh->hasMesh()) {
            domain = SelectionDomain::Mesh;
            kind = SelectionKind::Facet;
        } else if (services_.mesh != nullptr
                   && !services_.mesh->selectionGeometryDocument().entitiesOfKind(
                           femcae::geometry::GeometryEntityKind::Body).empty()) {
            // Mesh yokken parametrik analytic Face authoring yine mümkündür.
            // Mesh varsa mevcut Contact Mesh/Facet fallback sözleşmesi korunur.
            domain = SelectionDomain::Geometry;
            kind = SelectionKind::Face;
        } else {
            return false;
        }

        if (!((domain == SelectionDomain::Geometry && kind == SelectionKind::Face)
              || (domain == SelectionDomain::Mesh && kind == SelectionKind::Facet))) {
            return false;
        }
        if (domain == SelectionDomain::Geometry
            && (services_.mesh == nullptr
                || services_.mesh->selectionGeometryDocument().entitiesOfKind(
                       femcae::geometry::GeometryEntityKind::Body).empty())) {
            return false;
        }
        if (domain == SelectionDomain::Mesh
            && (services_.mesh == nullptr || !services_.mesh->hasMesh())) {
            return false;
        }

        const auto filter = selectionFilterForKind(kind);
        if (!filter.has_value()) {
            return false;
        }

        editingContact_ = target;
        editingContactSource_ = sourceSide;
        editDomain_ = domain;
        editKind_ = kind;
        previousFilter_ = graphics_->selectionFilter();
        editPreloadError_ = ScopeReferenceValidationError::None;

        disconnect(selection_, &SelectionManager::selectionChanged,
                   this, &SelectionCoordinator::handleSelectionChanged);
        editSelectionConnection_ = connect(selection_, &SelectionManager::selectionChanged,
                                           this, [this] {
            if (bridge_ != nullptr) {
                bridge_->setSelection(selection_->items());
            }
            if (meshBridge_ != nullptr) {
                meshBridge_->setSelection(selection_->items());
            }
            updateFeedback();
            if (details_ != nullptr) {
                details_->refresh();
            }
        });

        disconnect(navigator_, &ProjectNavigator::objectSelected,
                   this, &SelectionCoordinator::handleNavigatorSelection);
        editNavigatorConnection_ = connect(navigator_, &ProjectNavigator::objectSelected,
                                           this, [this](const ObjectId id) {
            if (editingContact_ != InvalidObjectId && id != editingContact_) {
                cancelContactEdit();
            }
        });

        if (bridge_ != nullptr) {
            disconnect(bridge_, &ViewportSelectionBridge::contextMenuRequested,
                       this, &SelectionCoordinator::handleViewportContextMenu);
        }
        if (meshBridge_ != nullptr) {
            disconnect(meshBridge_, &ViewportMeshSelectionBridge::contextMenuRequested,
                       this, &SelectionCoordinator::handleMeshContextMenu);
        }

        // Contact edit normalde Connections context'inden başlar. Bu context'te
        // Mesh Facet (ve bazı CAD topology) filter action'ları gizli olabilir;
        // filter'ı önce set etmek GraphicsWorkspace tarafından bilinçli olarak
        // reddedilir. Önce doğru engineering viewport/domain'i aktive et, sonra
        // zorunlu Face/Facet filter ve SelectionPolicy'yi kesin olarak uygula.
        activateContactEditViewport();
        graphics_->setSelectionFilter(*filter);
        configurePolicy(*filter);
        selection_->clearPreselection();
        (void)selection_->clear();

        if (selected.entities.isEmpty()) {
            updateFeedback();
            details_->refresh();
            return true;
        }

        ScopeSelectionItemsResult preload;
        if (domain == SelectionDomain::Geometry) {
            preload = selectionItemsForGeometryScope(
                selected, services_.mesh->selectionGeometryDocument());
        } else {
            preload = selectionItemsForMeshScope(selected, services_.mesh->mesh(), services_.mesh->generation());
        }
        editPreloadError_ = preload.error;
        if (preload.success()) {
            (void)selection_->apply(preload.items, SelectionOperation::Replace);
        } else {
            updateFeedback();
            details_->refresh();
        }
        return true;
    }

    void activateContactEditViewport()
    {
        if (graphics_ == nullptr || selection_ == nullptr) {
            return;
        }
        ViewportWidget *viewport = graphics_->viewport();
        if (viewport == nullptr) {
            return;
        }
        const QString side = editingContactSource_ ? tr("Source") : tr("Target");
        if (editDomain_ == SelectionDomain::Geometry && services_.mesh != nullptr) {
            viewport->setContext(ViewportContext::Geometry);
            graphics_->setContextLabel(tr("Geometry — Edit Contact %1").arg(side));
            const auto surfaces = services_.mesh->displaySelectionTopologyScene(tessellationDeflection_);
            if (!surfaces.isEmpty()) {
                viewport->showGeometry(surfaces);
            }
        } else if (editDomain_ == SelectionDomain::Mesh && services_.mesh != nullptr
                   && services_.mesh->hasMesh()) {
            viewport->setContext(ViewportContext::Mesh);
            graphics_->setContextLabel(tr("Mesh — Edit Contact %1").arg(side));
            viewport->showMesh(services_.mesh->mesh(), false);
        }
        refreshSelectionScene();
    }

    void finishContactEdit()
    {
        editingContact_ = InvalidObjectId;
        editingContactSource_ = false;
        editDomain_ = SelectionDomain::ProjectObject;
        editKind_ = SelectionKind::Object;
        editPreloadError_ = ScopeReferenceValidationError::None;

        QObject::disconnect(editSelectionConnection_);
        QObject::disconnect(editNavigatorConnection_);
        editSelectionConnection_ = {};
        editNavigatorConnection_ = {};

        if (selection_ != nullptr) {
            connect(selection_, &SelectionManager::selectionChanged,
                    this, &SelectionCoordinator::handleSelectionChanged);
            selection_->clearPreselection();
            (void)selection_->clear();
        }
        if (navigator_ != nullptr) {
            connect(navigator_, &ProjectNavigator::objectSelected,
                    this, &SelectionCoordinator::handleNavigatorSelection);
        }
        if (bridge_ != nullptr) {
            connect(bridge_, &ViewportSelectionBridge::contextMenuRequested,
                    this, &SelectionCoordinator::handleViewportContextMenu);
        }
        if (meshBridge_ != nullptr) {
            connect(meshBridge_, &ViewportMeshSelectionBridge::contextMenuRequested,
                    this, &SelectionCoordinator::handleMeshContextMenu);
        }
        if (graphics_ != nullptr) {
            graphics_->setSelectionFilter(previousFilter_);
        }
        restoreProjectObjectViewportAfterEdit();
    }

    void restoreProjectObjectViewportAfterEdit()
    {
        if (window_ != nullptr) {
            // Edit state temizlendikten sonra normal project-object viewport
            // bağlamı tek canonical MainWindow yolundan yeniden kurulur.
            window_->syncViewport();
            window_->syncCommandStates();
            window_->syncContextualSurface();
            window_->syncStatusBar();
        }
        refreshSelectionScene();
        updateFeedback();
        if (details_ != nullptr) {
            details_->refresh();
        }
    }

    void configurePolicy(SelectionFilter filter);
    void refreshSelectionScene();
    void handleNavigatorSelection(ObjectId id);
    void showSelectionContextMenu(ObjectId objectId, const QPoint &globalPosition);
    void routeViewportContextMenu(const QPoint &globalPosition);

    void handleViewportSelection(SelectionKind kind, quint64 bodyId, quint64 geometryId,
                                 SelectionOperation operation);
    void handleViewportPreselection(SelectionKind kind, quint64 bodyId, quint64 geometryId);
    void handleViewportContextMenu(SelectionKind kind, quint64 bodyId, quint64 geometryId,
                                   const QPoint &globalPosition);

    void handleMeshSelection(SelectionKind kind, qint64 meshEntityId, SelectionOperation operation);
    void handleMeshPreselection(SelectionKind kind, qint64 meshEntityId);
    void handleMeshContextMenu(SelectionKind kind, qint64 meshEntityId, const QPoint &globalPosition);

    void handleSelectionChanged();
    void handlePreselectionChanged();
    void updateFeedback();

    [[nodiscard]] bool syncNavigatorToPrimary();
    [[nodiscard]] bool syncNavigatorToGeometryBody(femcae::geometry::GeometryEntityId bodyId);
    [[nodiscard]] bool syncNavigatorToObject(ObjectId objectId);
    [[nodiscard]] ObjectId bodyObjectForGeometryId(femcae::geometry::GeometryEntityId bodyId) const;
    [[nodiscard]] std::optional<SelectionItem> selectionItemForHit(SelectionKind kind,
                                                                   quint64 bodyId,
                                                                   quint64 geometryId) const;
    [[nodiscard]] std::optional<SelectionItem> meshSelectionItemForHit(SelectionKind kind,
                                                                       qint64 meshEntityId) const;
    [[nodiscard]] bool selectionContains(const SelectionItem &item) const noexcept;
    [[nodiscard]] QString geometryEntityName(femcae::geometry::GeometryEntityId id) const;
    [[nodiscard]] QString bodyNameFor(const SelectionItem &item) const;

    Dynamics26MainWindow *window_{nullptr};
    ServiceContext services_{};
    ProjectNavigator *navigator_{nullptr};
    GraphicsWorkspace *graphics_{nullptr};
    DetailsHost *details_{nullptr};
    EngineeringStatusBar *status_{nullptr};
    SelectionManager *selection_{nullptr};
    ViewportSelectionBridge *bridge_{nullptr};
    ViewportMeshSelectionBridge *meshBridge_{nullptr};
    QPointer<QMenu> viewportMenu_;

    ObjectId editingNamedSelection_{InvalidObjectId};
    ObjectId editingContact_{InvalidObjectId};
    bool editingContactSource_{false};
    SelectionDomain editDomain_{SelectionDomain::ProjectObject};
    SelectionKind editKind_{SelectionKind::Object};
    SelectionFilter previousFilter_{SelectionFilter::Body};
    ScopeReferenceValidationError editPreloadError_{ScopeReferenceValidationError::None};
    QMetaObject::Connection editSelectionConnection_{};
    QMetaObject::Connection editNavigatorConnection_{};

    double tessellationDeflection_{0.15};
    bool syncingNavigator_{false};
};

} // namespace d26
