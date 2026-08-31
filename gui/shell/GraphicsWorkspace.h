#pragma once

// Merkez grafik çalışma alanı.
//
// Viewport + ince bir grafik araç çubuğu + bağlam göstergesi. Araç çubuğu
// COMSOL'daki grafik penceresi çubuğu gibi tek satır, ikon boyutlu ve
// bağlama duyarlıdır; ikinci bir ribbon değildir.

#include "../core/ProjectTypes.h"
#include "../core/SelectionTypes.h"
#include "../viewport/ViewportWidget.h"

#include <QFrame>

#include <optional>

class QLabel;
class QToolBar;
class QToolButton;
class QActionGroup;

namespace d26 {

enum class SelectionFilter { Body, Face, Edge, Vertex, Node, Element, Facet };

class GraphicsWorkspace final : public QFrame
{
    Q_OBJECT
public:
    explicit GraphicsWorkspace(QWidget *parent = nullptr);

    [[nodiscard]] ViewportWidget *viewport() const noexcept { return viewport_; }
    void setContextLabel(const QString &text);
    void setSelectionLabel(const QString &text);
    [[nodiscard]] SelectionFilter selectionFilter() const noexcept { return filter_; }
    void setSelectionFilter(SelectionFilter filter);

    // Filter grubu yalnız aktif selection domainini gösterir. Nullopt Geometry
    // veya Mesh selection'ın bu viewport bağlamında aktif olmadığını belirtir.
    void setSelectionFilterDomain(std::optional<SelectionDomain> domain);

    // CAD topology filter availability gerçek display provenance'a bağlıdır.
    void setTopologySelectionAvailable(bool faceAvailable,
                                       bool edgeAvailable,
                                       bool vertexAvailable);
    // FEM filter availability generated mesh provenance'a bağlıdır.
    void setMeshSelectionAvailable(bool nodeAvailable,
                                   bool elementAvailable,
                                   bool facetAvailable);

    // Alpha.3.2'den kalan kabuk çağrısı için geçici kaynak uyumluluğu. CAD Face
    // capability FEM mesh'ten türetilemez; bu eski setter bilinçli olarak no-op'tur.
    void setFaceSelectionAvailable(bool legacyMeshDerivedAvailability);

    void refreshIcons();

signals:
    void fitViewRequested();
    void isometricViewRequested();
    void selectionFilterChanged(SelectionFilter filter);

private:
    [[nodiscard]] bool filterAvailable(SelectionFilter filter) const noexcept;
    void syncFilterChecks();
    void syncFilterVisibility();

    ViewportWidget *viewport_{nullptr};
    QToolBar *toolbar_{nullptr};
    QLabel *contextLabel_{nullptr};
    QLabel *selectionLabel_{nullptr};
    QAction *selectBody_{nullptr};
    QAction *selectFace_{nullptr};
    QAction *selectEdge_{nullptr};
    QAction *selectVertex_{nullptr};
    QAction *selectNode_{nullptr};
    QAction *selectElement_{nullptr};
    QAction *selectFacet_{nullptr};
    QAction *fit_{nullptr};
    QAction *isometric_{nullptr};
    SelectionFilter filter_{SelectionFilter::Body};
    std::optional<SelectionDomain> filterDomain_{SelectionDomain::Geometry};
};

} // namespace d26
