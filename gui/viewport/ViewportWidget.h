#pragma once

// 3B grafik alanı.
//
// Uygulamanın görsel olarak baskın yüzeyidir. Aktörler semantik rollerle
// (RenderRole) oluşturulur; Light/Dark geçişinde renkler yalnız role bakılarak
// yeniden uygulanır, hiçbir aktör eski paletten renk taşımaz.
//
// Sonuç konturu YALNIZ Results/Modal bağlamında açılır (§17). Geometry / Mesh /
// Loads bağlamlarında nötr mühendislik gösterimi kullanılır.

#include "../core/ProjectTypes.h"
#include "RenderRoles.h"
#include "ViewportNavigation.h"

#include <QImage>
#include <QVector>
#include <QWidget>

#include <femcae/geometry/GeometryTypes.h>
#include <femcae/meshing/MeshTypes.h>
#include <femcae/meshing/ResultDatabase.h>

#include <memory>
#include <array>

namespace d26 {

// Geometri/mesh yüzeyinin gösterim biçimi. Gerçek bir viewport davranışıdır;
// Details panelindeki seçim doğrudan buraya bağlanır.
enum class SurfaceRepresentation {
    Shaded,
    ShadedWithEdges,
    Wireframe
};

enum class ResultField {
    TotalDeformation,
    EquivalentStress,
    ReactionForce
};

// Sınır şartı / yük görselleştirmesi için kapsam tanımı.
struct BoundaryGlyph {
    femcae::geometry::GeometryEntityId geometryId{femcae::geometry::InvalidGeometryId};
    bool isLoad{false};
    double dx{1.0};
    double dy{0.0};
    double dz{0.0};
};

class ViewportWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit ViewportWidget(QWidget *parent = nullptr);
    ~ViewportWidget() override;

    [[nodiscard]] static bool vtkAvailable() noexcept;

    void setContext(ViewportContext context);
    void setRepresentation(SurfaceRepresentation representation);
    [[nodiscard]] SurfaceRepresentation representation() const noexcept { return representation_; }
    [[nodiscard]] bool representationMatchesScene() const noexcept;
    [[nodiscard]] ViewportContext context() const noexcept { return context_; }

    void clearScene();
    // Parametrik/legacy body-level display yolu.
    void showGeometry(const femcae::geometry::GeometryTessellation &tessellation);
    // Alpha.3.2 CAD yolu: birden fazla Body tek display sahnesinde birleştirilir
    // ve her display cell'in Body/Face provenance'i korunur.
    void showGeometry(const QVector<femcae::geometry::TopologyTessellation> &bodies);
    [[nodiscard]] bool hasTopologyFaceProvenance() const noexcept;

    void showMesh(const femcae::meshing::SimulationMesh &mesh, bool showNodes = false);
    void showModelWithBoundaryConditions(const femcae::meshing::SimulationMesh &mesh,
                                         const QVector<BoundaryGlyph> &glyphs);
    void showResult(const femcae::meshing::SimulationMesh &mesh,
                    const femcae::meshing::ResultDatabase &results,
                    ResultField field);

    // Seçili nesnenin kapsadığı CAD/kutu yüzünü vurgular.
    void setHighlightedGeometry(femcae::geometry::GeometryEntityId geometryId);

    // View state only: these operations never enter the document undo stack.
    void fitView();
    void resetCamera(); // Compatibility wrapper for fitView().
    void setStandardView(StandardView view);
    void setIsometricView();
    [[nodiscard]] bool zoomToBounds(const std::array<double, 6> &bounds);

    void setRotationCenter(const std::array<double, 3> &worldPoint);
    [[nodiscard]] bool setRotationCenterToHighlightedGeometry();
    [[nodiscard]] bool setRotationCenterToBounds(const std::array<double, 6> &bounds);
    [[nodiscard]] bool resetRotationCenter();
    [[nodiscard]] std::array<double, 3> rotationCenter() const;

    [[nodiscard]] bool hasAxisTriad() const noexcept;
    [[nodiscard]] bool hasOrientationCube() const noexcept;
    [[nodiscard]] static bool orientationCubeAvailable() noexcept;
    void refreshAppearance();

    // --capture geliştirici modu için render penceresi görüntüsü.
    [[nodiscard]] QImage grabRenderedImage();

    // VTK interactor geri çağırması tarafından kullanılır. FEM/legacy sahnede
    // geometryPicked; topology-aware CAD sahnede topologyPicked yayınlanır.
    void handlePick(int x, int y);

signals:
    void geometryPicked(quint64 geometryId);
    // Rendering katmanı selection kararını vermez; yalnız pick edilen display
    // cell'in gerçek CAD Body/Face provenance'ini yayınlar.
    void topologyPicked(quint64 bodyId, quint64 faceId);

protected:
    bool event(QEvent *event) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    ViewportContext context_{ViewportContext::Empty};
    SurfaceRepresentation representation_{SurfaceRepresentation::ShadedWithEdges};
};

} // namespace d26
