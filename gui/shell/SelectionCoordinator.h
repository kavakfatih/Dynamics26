#pragma once

// Dynamics26 Alpha.3.6 — application-level transient selection coordinator.
//
// ProjectModel current object, CAD topology selection ve generated FEM mesh
// selection ayni merkezi SelectionManager etrafinda koordine edilir. Geometry ve
// Mesh kimlik uzaylari ayridir; raw selection document undo/redo verisi değildir.
// Persistent Named Selection üretimi de raw picker ID'lerinden değil, burada
// doğrulanan ScopeReference / SelectionItem akışından geçer.

#include "../commands/NamedSelectionCommands.h"
#include "../core/DocumentCommandManager.h"
#include "../core/ScopeReferenceBuilder.h"
#include "../core/ScopeSelectionBridge.h"
#include "../core/SelectionManager.h"
#include "../core/ServiceContext.h"
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
#include <QString>

#include <optional>

namespace d26 {

class EngineeringStatusBar;

class SelectionCoordinator final : public QObject
{
public:
    explicit SelectionCoordinator(Dynamics26MainWindow *window, QObject *parent = nullptr);

    [[nodiscard]] SelectionManager *selectionManager() const noexcept { return selection_; }
    [[nodiscard]] ScopeReferenceBuildResult currentGeometryScope() const;

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
            if (services_.geometry == nullptr) {
                result.error = ScopeReferenceBuildError::UnsupportedDomain;
                return result;
            }
            return buildGeometryScopeReference(items, services_.geometry->document());
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
            if (services_.geometry == nullptr || !services_.geometry->summary().hasGeometry) {
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
            preload = selectionItemsForGeometryScope(definition->scope, services_.geometry->document());
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

        if (editDomain_ == SelectionDomain::Geometry && services_.geometry != nullptr) {
            viewport->setContext(ViewportContext::Geometry);
            graphics_->setContextLabel(tr("Geometry — Edit Named Selection"));
            const auto surfaces = services_.geometry->displayTopologyScene(tessellationDeflection_);
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
        const ObjectId finishedTarget = editingNamedSelection_;
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
        Q_UNUSED(finishedTarget);
    }

    void configurePolicy(SelectionFilter filter);
    void refreshSelectionScene();
    void handleNavigatorSelection(ObjectId id);
    void showSelectionContextMenu(ObjectId objectId, const QPoint &globalPosition);

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

    ObjectId editingNamedSelection_{InvalidObjectId};
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
