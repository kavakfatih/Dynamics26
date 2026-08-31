#pragma once

// Dynamics26 Alpha.3.3 — topology-aware CAD selection interaction bridge.
//
// ViewportWidget temel render/camera davranışının sahibidir. Bu bridge transient
// selection input, CAD display provenance pick'i ve selection/preselection
// overlay aktörlerini yönetir. Doküman/solver verisi yazmaz.
//
// CAD Geometry != Display Tessellation != FEM Mesh

#include "../core/SelectionTypes.h"
#include "GeometryTopologyScene.h"
#include "ViewportSelectionController.h"

#include <QObject>
#include <QPoint>
#include <QVector>

#include <memory>
#include <optional>

class QEvent;

namespace d26 {

class ViewportWidget;

class ViewportSelectionBridge final : public QObject
{
    Q_OBJECT
public:
    explicit ViewportSelectionBridge(ViewportWidget *viewport, QObject *parent = nullptr);
    ~ViewportSelectionBridge() override;

    // Face/Edge/Vertex display kaynaklari ayni Body sirasi ve CAD revision'i ile
    // tek topology scene'e bağlanır. Kısmi scene kabul edilmez.
    [[nodiscard]] bool setScene(const QVector<femcae::geometry::TopologyTessellation> &surfaces,
                                const QVector<femcae::geometry::EdgeDisplayTessellation> &edges,
                                const QVector<femcae::geometry::VertexDisplayPoints> &vertices);
    void clearScene();

    [[nodiscard]] bool hasFaceProvenance() const noexcept;
    [[nodiscard]] bool hasEdgeProvenance() const noexcept;
    [[nodiscard]] bool hasVertexProvenance() const noexcept;

    void setActiveKind(SelectionKind kind);
    [[nodiscard]] SelectionKind activeKind() const noexcept;
    void setSelection(const QVector<SelectionItem> &items);
    void setPreselection(std::optional<SelectionItem> item);

signals:
    void selectionRequested(d26::SelectionKind kind, quint64 bodyId, quint64 geometryId,
                            d26::SelectionOperation operation);
    void preselectionRequested(d26::SelectionKind kind, quint64 bodyId, quint64 geometryId);
    void contextMenuRequested(d26::SelectionKind kind, quint64 bodyId, quint64 geometryId,
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
};

} // namespace d26
