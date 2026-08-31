#pragma once

// Dynamics26 Alpha.3.4 — generated FEM mesh selection interaction bridge.
//
// CAD selection ViewportSelectionBridge'te kalir. Bu sinif yalniz Mesh domain
// Node/Element/Boundary Facet input, provenance lookup ve overlay katmanini
// yonetir. Iki bridge ayni viewport'u gozlemleyebilir; yalniz aktif baglamdaki
// bridge input kabul eder.

#include "../core/SelectionTypes.h"
#include "MeshSelectionScene.h"
#include "ViewportSelectionController.h"

#include <QObject>
#include <QPoint>
#include <QVector>

#include <memory>
#include <optional>

class QEvent;

namespace d26 {

class ViewportWidget;

class ViewportMeshSelectionBridge final : public QObject
{
    Q_OBJECT
public:
    explicit ViewportMeshSelectionBridge(ViewportWidget *viewport, QObject *parent = nullptr);
    ~ViewportMeshSelectionBridge() override;

    [[nodiscard]] bool setScene(const femcae::meshing::SimulationMesh &mesh, quint64 generation);
    void clearScene();

    void setInputEnabled(bool enabled) noexcept;
    [[nodiscard]] bool inputEnabled() const noexcept;

    void setActiveKind(SelectionKind kind);
    [[nodiscard]] SelectionKind activeKind() const noexcept;
    void setSelection(const QVector<SelectionItem> &items);
    void setPreselection(std::optional<SelectionItem> item);

signals:
    void selectionRequested(d26::SelectionKind kind, qint64 meshEntityId,
                            d26::SelectionOperation operation);
    void preselectionRequested(d26::SelectionKind kind, qint64 meshEntityId);
    void contextMenuRequested(d26::SelectionKind kind, qint64 meshEntityId,
                              const QPoint &globalPosition);
    void selectionClearRequested();
    void preselectionClearRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    ViewportWidget *viewport_{nullptr};
    ViewportSelectionController input_;
    bool inputEnabled_{false};
};

} // namespace d26
