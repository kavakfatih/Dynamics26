#pragma once

// Dynamics26 Alpha.3.2 — application-level transient selection coordinator.
//
// Bu sınıf görünür pencere kompozisyonunu değiştirmez. ProjectModel, CAD
// topology kimlikleri ve Viewport transient selection state'i arasında açık
// koordinasyon kurar. Selection document undo/redo verisi değildir.

#include "../core/ScopeReferenceBuilder.h"
#include "../core/SelectionManager.h"
#include "../core/ServiceContext.h"
#include "GraphicsWorkspace.h"

#include <QObject>
#include <QString>

#include <optional>

namespace d26 {

class DetailsHost;
class Dynamics26MainWindow;
class EngineeringStatusBar;
class ProjectNavigator;
class ViewportSelectionBridge;

class SelectionCoordinator final : public QObject
{
public:
    explicit SelectionCoordinator(Dynamics26MainWindow *window, QObject *parent = nullptr);

    [[nodiscard]] SelectionManager *selectionManager() const noexcept { return selection_; }
    // Committed transient CAD selection'i current GeometryDocument ile doğrular
    // ve persistentKey taşıyan engineering scope kontratına çevirir. Existing
    // BC/Load BoxFace şeması Alpha.3.2'de bu API'ye migrate edilmez.
    [[nodiscard]] ScopeReferenceBuildResult currentGeometryScope() const;

private:
    void configurePolicy(SelectionFilter filter);
    void refreshGeometryScene();
    void handleNavigatorSelection(ObjectId id);
    void handleViewportSelection(quint64 bodyId, quint64 faceId, SelectionOperation operation);
    void handleViewportPreselection(quint64 bodyId, quint64 faceId);
    void handleSelectionChanged();
    void handlePreselectionChanged();
    void updateFeedback();
    [[nodiscard]] bool syncNavigatorToPrimary();
    [[nodiscard]] ObjectId bodyObjectForGeometryId(femcae::geometry::GeometryEntityId bodyId) const;
    [[nodiscard]] std::optional<SelectionItem> selectionItemForHit(quint64 bodyId, quint64 faceId) const;
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

    double tessellationDeflection_{0.15};
    bool syncingNavigator_{false};
};

} // namespace d26
