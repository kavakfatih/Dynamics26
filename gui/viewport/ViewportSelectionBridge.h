#pragma once

// Dynamics26 Alpha.3.2 — topology-aware CAD selection interaction bridge.
//
// ViewportWidget temel render/camera davranışının sahibidir. Bu bridge yalnız
// transient selection input, CAD cell provenance pick'i ve selection overlay
// aktörlerini yönetir. Doküman/solver verisi yazmaz.
//
// CAD Geometry != Display Tessellation != FEM Mesh kuralı korunur.

#include "../core/SelectionTypes.h"
#include "GeometrySelectionScene.h"
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

    // Viewport'ta topology-aware multi-body CAD sahnesini gösterir ve aynı
    // display cell -> Body/Face provenance tablosunu selection için bağlar.
    [[nodiscard]] bool setScene(const QVector<femcae::geometry::TopologyTessellation> &bodies);
    void clearScene();
    [[nodiscard]] bool hasFaceProvenance() const noexcept;

    void setSelection(const QVector<SelectionItem> &items);
    void setPreselection(std::optional<SelectionItem> item);

signals:
    void selectionRequested(quint64 bodyId, quint64 faceId, d26::SelectionOperation operation);
    void preselectionRequested(quint64 bodyId, quint64 faceId);
    // Secondary click hit-test sonucu. Selection setini preserve/replace etme
    // kararı rendering katmanında değil application coordinator'da verilir.
    void contextMenuRequested(quint64 bodyId, quint64 faceId, const QPoint &globalPosition);
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
