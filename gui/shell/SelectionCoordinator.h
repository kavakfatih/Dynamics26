#pragma once

// Dynamics26 Alpha.3.4 — application-level transient selection coordinator.
//
// ProjectModel current object, CAD topology selection ve generated FEM mesh
// selection ayni merkezi SelectionManager etrafinda koordine edilir. Geometry ve
// Mesh kimlik uzaylari ayridir; raw selection document undo/redo verisi değildir.

#include "../core/ScopeReferenceBuilder.h"
#include "../core/SelectionManager.h"
#include "../core/ServiceContext.h"
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

private:
    void configurePolicy(SelectionFilter filter);
    void refreshSelectionScene();
    void handleNavigatorSelection(ObjectId id);

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
