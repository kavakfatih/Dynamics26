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
#include "../core/SelectionManager.h"
#include "../core/ServiceContext.h"
#include "../services/NamedSelectionService.h"
#include "GraphicsWorkspace.h"

#include <QObject>
#include <QPoint>
#include <QString>

#include <optional>

namespace d26 {

class DetailsHost;
class Dynamics26MainWindow;
class EngineeringStatusBar;
class ProjectNavigator;
class ViewportSelectionBridge;
class ViewportMeshSelectionBridge;

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

private:
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

    double tessellationDeflection_{0.15};
    bool syncingNavigator_{false};
};

} // namespace d26
