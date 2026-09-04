#include "ViewportWidget.h"

#include "Dynamics26InteractorStyle.h"
#include "GeometrySelectionScene.h"
#include "ViewportCameraController.h"
#include "ViewportInputRouter.h"

#include <QApplication>
#include <QEvent>
#include <QLabel>
#include <QMenu>
#include <QPalette>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <vector>

#ifdef FEMCAE_GUI_HAS_VTK
#include <QVTKOpenGLNativeWidget.h>
#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkCaptionActor2D.h>
#include <vtkCommand.h>
#include <vtkMapper.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCellPicker.h>
#include <vtkConeSource.h>
#include <vtkDoubleArray.h>
#include <vtkFeatureEdges.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkGlyph3D.h>
#include <vtkLookupTable.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkScalarBarActor.h>
#include <vtkSmartPointer.h>
#include <vtkTextProperty.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkWindowToImageFilter.h>
#include <vtkImageData.h>
#include <vtkUnsignedCharArray.h>
#include <vtkPointSource.h>
#include <vtkSphereSource.h>
#ifdef FEMCAE_GUI_HAS_CAMERA_ORIENTATION_WIDGET
#include <vtkCameraOrientationWidget.h>
#endif
#endif

using femcae::geometry::GeometryEntityId;
using femcae::geometry::InvalidGeometryId;
using femcae::meshing::MeshEntityId;
using femcae::meshing::SimulationMesh;

namespace d26 {
namespace {

bool systemPrefersDark()
{
    // macOS System Appearance, Qt'nin native paletine yansır. Ayrı bir tema
    // motoru kurulmaz; tek kaynak sistem görünümüdür.
    return qApp->palette().color(QPalette::Window).lightnessF() < 0.5;
}

#ifdef FEMCAE_GUI_HAS_VTK
struct MeshBounds {
    double span{1.0};
    double center[3]{0.0, 0.0, 0.0};
};

MeshBounds computeBounds(const SimulationMesh &mesh)
{
    MeshBounds bounds;
    if (mesh.nodes.empty()) {
        return bounds;
    }
    double lo[3] = {mesh.nodes.front().x.x, mesh.nodes.front().x.y, mesh.nodes.front().x.z};
    double hi[3] = {lo[0], lo[1], lo[2]};
    for (const auto &node : mesh.nodes) {
        const double p[3] = {node.x.x, node.x.y, node.x.z};
        for (int i = 0; i < 3; ++i) {
            lo[i] = std::min(lo[i], p[i]);
            hi[i] = std::max(hi[i], p[i]);
        }
    }
    bounds.span = std::max({hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2], 1.0e-9});
    for (int i = 0; i < 3; ++i) {
        bounds.center[i] = 0.5 * (lo[i] + hi[i]);
    }
    return bounds;
}
#endif

} // namespace

// ---------------------------------------------------------------------------

class ViewportWidget::Impl
{
public:
#ifdef FEMCAE_GUI_HAS_VTK
    QVTKOpenGLNativeWidget *widget{nullptr};
    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkScalarBarActor> scalarBar;
    vtkSmartPointer<vtkLookupTable> lookupTable;
    vtkSmartPointer<vtkCellPicker> picker;
    vtkSmartPointer<Dynamics26InteractorStyle> interactorStyle;
    vtkSmartPointer<vtkAxesActor> axesActor;
    vtkSmartPointer<vtkOrientationMarkerWidget> orientationMarker;
#ifdef FEMCAE_GUI_HAS_CAMERA_ORIENTATION_WIDGET
    vtkSmartPointer<vtkCameraOrientationWidget> cameraOrientation;
#endif
    std::unique_ptr<ViewportCameraController> cameraController;
    std::unique_ptr<ViewportInputRouter> inputRouter;

    // Rol -> aktör kaydı. Renk yeniden uygulaması yalnız bu tablo üzerinden yapılır.
    struct RoleActor {
        vtkSmartPointer<vtkActor> actor;
        RenderRole surfaceRole{RenderRole::GeometrySurface};
        RenderRole edgeRole{RenderRole::GeometryEdge};
        bool useEdgeRole{false};
        bool scalarDriven{false};
    };
    std::vector<RoleActor> actors;

    // Picking: mesh/legacy sahnede hücre -> geometri kimliği; topology-aware
    // CAD sahnede GeometrySelectionScene hücre -> Body/Face provenance taşır.
    std::vector<GeometryEntityId> facetGeometryIds;
    std::vector<MeshEntityId> resultFacetIds;
    bool resultScene{false};
    GeometrySelectionScene geometryScene;
    vtkSmartPointer<vtkActor> surfaceActor;
    vtkSmartPointer<vtkActor> geometryEdgeActor;
    vtkSmartPointer<vtkActor> highlightActor;
    GeometryEntityId highlighted{InvalidGeometryId};
    SimulationMesh cachedMesh;
    bool hasCachedMesh{false};

    ViewportPalette palette{ViewportPalette::forAppearance(false)};

    void applyPalette();
    void render();
    vtkSmartPointer<vtkActor> addActor(vtkSmartPointer<vtkPolyData> data, RenderRole surfaceRole,
                                       RenderRole edgeRole, bool showEdges);
    void rebuildHighlight();
#else
    QLabel *placeholder{nullptr};
#endif
};

#ifdef FEMCAE_GUI_HAS_VTK

void ViewportWidget::Impl::render()
{
    if (widget != nullptr && widget->renderWindow() != nullptr) {
        widget->renderWindow()->Render();
    }
}

void ViewportWidget::Impl::applyPalette()
{
    const Rgb background = palette.color(RenderRole::Background);
    const Rgb gradient = palette.color(RenderRole::BackgroundGradient);
    renderer->GradientBackgroundOn();
    renderer->SetBackground(gradient.r, gradient.g, gradient.b);
    renderer->SetBackground2(background.r, background.g, background.b);

    for (auto &entry : actors) {
        if (entry.actor == nullptr) {
            continue;
        }
        vtkProperty *property = entry.actor->GetProperty();
        if (!entry.scalarDriven) {
            const Rgb surface = palette.color(entry.surfaceRole);
            property->SetColor(surface.r, surface.g, surface.b);
        }
        if (entry.useEdgeRole) {
            const Rgb edge = palette.color(entry.edgeRole);
            property->SetEdgeColor(edge.r, edge.g, edge.b);
        }
    }

    const Rgb text = palette.color(RenderRole::OverlayText);
    if (scalarBar != nullptr) {
        scalarBar->GetLabelTextProperty()->SetColor(text.r, text.g, text.b);
        scalarBar->GetTitleTextProperty()->SetColor(text.r, text.g, text.b);
        scalarBar->GetLabelTextProperty()->SetShadow(0);
        scalarBar->GetTitleTextProperty()->SetShadow(0);
    }
    if (axesActor != nullptr) {
        vtkCaptionActor2D *captions[] = {axesActor->GetXAxisCaptionActor2D(),
                                         axesActor->GetYAxisCaptionActor2D(),
                                         axesActor->GetZAxisCaptionActor2D()};
        for (vtkCaptionActor2D *caption : captions) {
            caption->GetCaptionTextProperty()->SetColor(text.r, text.g, text.b);
            caption->GetCaptionTextProperty()->SetBold(1);
            caption->GetCaptionTextProperty()->SetShadow(0);
        }
    }
    if (lookupTable != nullptr) {
        lookupTable->SetHueRange(palette.contourHueStart(), palette.contourHueEnd());
        lookupTable->SetSaturationRange(0.72, 0.82);
        lookupTable->SetValueRange(palette.isDark() ? 0.94 : 0.88, palette.isDark() ? 0.97 : 0.93);
        lookupTable->Build();
    }
}

vtkSmartPointer<vtkActor> ViewportWidget::Impl::addActor(vtkSmartPointer<vtkPolyData> data,
                                                        const RenderRole surfaceRole,
                                                        const RenderRole edgeRole,
                                                        const bool showEdges)
{
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(data);
    mapper->ScalarVisibilityOff();

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetEdgeVisibility(showEdges ? 1 : 0);
    actor->GetProperty()->SetLineWidth(1.0);
    actor->GetProperty()->SetAmbient(0.28);
    actor->GetProperty()->SetDiffuse(0.72);
    actor->GetProperty()->SetSpecular(0.10);
    actor->GetProperty()->SetSpecularPower(24.0);

    RoleActor entry;
    entry.actor = actor;
    entry.surfaceRole = surfaceRole;
    entry.edgeRole = edgeRole;
    entry.useEdgeRole = showEdges;
    actors.push_back(entry);
    renderer->AddActor(actor);
    return actor;
}

void ViewportWidget::Impl::rebuildHighlight()
{
    if (highlightActor != nullptr) {
        renderer->RemoveActor(highlightActor);
        actors.erase(std::remove_if(actors.begin(), actors.end(),
                                    [this](const RoleActor &entry) { return entry.actor == highlightActor; }),
                     actors.end());
        highlightActor = nullptr;
    }
    if (highlighted == InvalidGeometryId || !hasCachedMesh) {
        return;
    }

    std::unordered_map<MeshEntityId, vtkIdType> index;
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(static_cast<vtkIdType>(cachedMesh.nodes.size()));
    for (std::size_t i = 0; i < cachedMesh.nodes.size(); ++i) {
        const auto &node = cachedMesh.nodes[i];
        points->SetPoint(static_cast<vtkIdType>(i), node.x.x, node.x.y, node.x.z);
        index[node.id] = static_cast<vtkIdType>(i);
    }
    vtkNew<vtkCellArray> polys;
    bool any = false;
    for (const auto &facet : cachedMesh.boundaryFacets) {
        if (facet.sourceGeometryId != highlighted) {
            continue;
        }
        vtkIdType ids[4];
        bool valid = true;
        for (int i = 0; i < 4; ++i) {
            const auto it = index.find(facet.nodeIds[static_cast<std::size_t>(i)]);
            if (it == index.end()) {
                valid = false;
                break;
            }
            ids[i] = it->second;
        }
        if (!valid) {
            continue;
        }
        polys->InsertNextCell(4, ids);
        any = true;
    }
    if (!any) {
        return;
    }
    vtkSmartPointer<vtkPolyData> data = vtkSmartPointer<vtkPolyData>::New();
    data->SetPoints(points);
    data->SetPolys(polys);

    highlightActor = addActor(data, RenderRole::Selection, RenderRole::Selection, true);
    highlightActor->GetProperty()->SetOpacity(0.55);
    highlightActor->GetProperty()->SetLineWidth(2.0);
    // Vurgu yüzeyi ana yüzeyle z-fighting yapmasın.
    highlightActor->GetMapper()->SetRelativeCoincidentTopologyPolygonOffsetParameters(-4.0, -4.0);
    const Rgb selection = palette.color(RenderRole::Selection);
    highlightActor->GetProperty()->SetColor(selection.r, selection.g, selection.b);
    highlightActor->GetProperty()->SetEdgeColor(selection.r, selection.g, selection.b);
}

#endif // FEMCAE_GUI_HAS_VTK

// ---------------------------------------------------------------------------

ViewportWidget::ViewportWidget(QWidget *parent)
    : QWidget(parent), impl_(std::make_unique<Impl>())
{
    setObjectName(QStringLiteral("Dynamics26Viewport"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

#ifdef FEMCAE_GUI_HAS_VTK
    impl_->widget = new QVTKOpenGLNativeWidget(this);
    impl_->widget->setMinimumSize(480, 320);
    impl_->widget->setFocusPolicy(Qt::StrongFocus);
    impl_->widget->setMouseTracking(true);
    impl_->widget->setAttribute(Qt::WA_AcceptTouchEvents, true);
    layout->addWidget(impl_->widget);

    vtkNew<vtkGenericOpenGLRenderWindow> window;
    impl_->renderer = vtkSmartPointer<vtkRenderer>::New();
    window->AddRenderer(impl_->renderer);
    window->SetMultiSamples(4);
    impl_->widget->setRenderWindow(window);

    impl_->lookupTable = vtkSmartPointer<vtkLookupTable>::New();
    impl_->lookupTable->SetNumberOfTableValues(256);
    impl_->lookupTable->SetRampToLinear();

    impl_->scalarBar = vtkSmartPointer<vtkScalarBarActor>::New();
    impl_->scalarBar->SetLookupTable(impl_->lookupTable);
    impl_->scalarBar->SetNumberOfLabels(9);
    impl_->scalarBar->SetWidth(0.115);
    impl_->scalarBar->SetHeight(0.62);
    impl_->scalarBar->SetPosition(0.868, 0.140);
    impl_->scalarBar->SetVisibility(0);
    impl_->scalarBar->SetBarRatio(0.28);
    impl_->scalarBar->SetTextPositionToSucceedScalarBar();
    impl_->scalarBar->SetLabelFormat("%-#6.4g");
    impl_->scalarBar->UnconstrainedFontSizeOn();
    impl_->scalarBar->GetLabelTextProperty()->SetFontSize(15);
    impl_->scalarBar->GetLabelTextProperty()->SetItalic(0);
    impl_->scalarBar->GetLabelTextProperty()->SetBold(0);
    impl_->scalarBar->GetTitleTextProperty()->SetFontSize(15);
    impl_->scalarBar->GetTitleTextProperty()->SetItalic(0);
    impl_->scalarBar->GetTitleTextProperty()->SetBold(1);
    impl_->renderer->AddViewProp(impl_->scalarBar);

    impl_->picker = vtkSmartPointer<vtkCellPicker>::New();
    impl_->picker->SetTolerance(0.005);

    if (auto *interactor = impl_->widget->interactor()) {
        impl_->interactorStyle = vtkSmartPointer<Dynamics26InteractorStyle>::New();
        impl_->interactorStyle->SetDefaultRenderer(impl_->renderer);
        impl_->interactorStyle->SetPickCallback([this](const int x, const int y) { handlePick(x, y); });
        interactor->SetInteractorStyle(impl_->interactorStyle);

        // Sol-alt eksen triadı ayrı overlay renderer kullanır; dolayısıyla Fit
        // görünür model bounds'una hiçbir zaman katılmaz.
        impl_->axesActor = vtkSmartPointer<vtkAxesActor>::New();
        impl_->axesActor->SetTotalLength(1.0, 1.0, 1.0);
        impl_->axesActor->SetShaftTypeToCylinder();
        impl_->axesActor->SetCylinderRadius(0.045);
        impl_->orientationMarker = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
        impl_->orientationMarker->SetOrientationMarker(impl_->axesActor);
        impl_->orientationMarker->SetInteractor(interactor);
        impl_->orientationMarker->SetViewport(0.0, 0.0, 0.13, 0.13);
        impl_->orientationMarker->SetEnabled(1);
        impl_->orientationMarker->SetInteractive(0);

#ifdef FEMCAE_GUI_HAS_CAMERA_ORIENTATION_WIDGET
        // VTK camera widget consumes its own events (AbortFlagOn), preventing
        // cube clicks from falling through to body picking.
        impl_->cameraOrientation = vtkSmartPointer<vtkCameraOrientationWidget>::New();
        impl_->cameraOrientation->SetParentRenderer(impl_->renderer);
        impl_->cameraOrientation->AnimateOff();
        impl_->cameraOrientation->ShouldResetCameraOn();
        impl_->cameraOrientation->SetEnabled(1);
#endif
    }

    impl_->cameraController = std::make_unique<ViewportCameraController>(
        impl_->renderer, [this] { impl_->render(); });
    impl_->inputRouter = std::make_unique<ViewportInputRouter>([this](const NavigationAction &action) {
        if (impl_->cameraController == nullptr) {
            return;
        }
        switch (action.type) {
        case NavigationActionType::Orbit:
            if (action.phase != NavigationPhase::End) {
                impl_->cameraController->orbit(action.delta);
            }
            break;
        case NavigationActionType::Pan:
            if (action.phase != NavigationPhase::End) {
                impl_->cameraController->pan(action.delta);
            }
            break;
        case NavigationActionType::Zoom:
            if (action.phase != NavigationPhase::End) {
                impl_->cameraController->zoom(action.zoomDelta);
            }
            break;
        case NavigationActionType::Fit:
            (void)impl_->cameraController->fit();
            break;
        case NavigationActionType::SetStandardView:
            impl_->cameraController->setStandardView(action.standardView);
            break;
        case NavigationActionType::SetRotationCenter:
            impl_->cameraController->setRotationCenter(action.worldPoint);
            break;
        case NavigationActionType::None:
            break;
        }
        if (impl_->interactorStyle != nullptr) {
            const auto &pivot = impl_->cameraController->rotationCenter();
            impl_->interactorStyle->SetCenterOfRotation(pivot.data());
        }
    }, this);
    impl_->widget->installEventFilter(impl_->inputRouter.get());
    impl_->widget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(impl_->widget, &QWidget::customContextMenuRequested, this, [this](const QPoint &position) {
        QMenu menu(impl_->widget);
        QAction *fit = menu.addAction(tr("Fit View\tF"));
        QMenu *views = menu.addMenu(tr("Standard View"));
        const auto addView = [this, views](const QString &label, const StandardView view) {
            QAction *action = views->addAction(label);
            connect(action, &QAction::triggered, this, [this, view] { setStandardView(view); });
        };
        addView(tr("Isometric\t0"), StandardView::Isometric);
        addView(tr("Front\t1"), StandardView::Front);
        addView(tr("Back\tShift+1"), StandardView::Back);
        addView(tr("Top\t2"), StandardView::Top);
        addView(tr("Bottom\tShift+2"), StandardView::Bottom);
        addView(tr("Right\t3"), StandardView::Right);
        addView(tr("Left\tShift+3"), StandardView::Left);
        menu.addSeparator();
        QAction *pivotToSelection = menu.addAction(tr("Set Rotation Center to Selection"));
        pivotToSelection->setEnabled(impl_->highlighted != InvalidGeometryId && impl_->surfaceActor != nullptr);
        QAction *resetPivot = menu.addAction(tr("Reset Rotation Center to Model"));
        connect(fit, &QAction::triggered, this, &ViewportWidget::fitView);
        connect(pivotToSelection, &QAction::triggered, this,
                [this] { (void)setRotationCenterToHighlightedGeometry(); });
        connect(resetPivot, &QAction::triggered, this, [this] { (void)resetRotationCenter(); });
        menu.exec(impl_->widget->mapToGlobal(position));
    });

    impl_->palette = ViewportPalette::forAppearance(systemPrefersDark());
    impl_->applyPalette();
    impl_->cameraController->setStandardView(StandardView::Isometric);
#else
    impl_->placeholder = new QLabel(
        tr("3B Grafik Alanı\n\nBu derlemede VTK bulunamadı.\n"
           "macOS GUI derlemesinde VTK etkinken tam etkileşimli viewport açılır."),
        this);
    impl_->placeholder->setAlignment(Qt::AlignCenter);
    layout->addWidget(impl_->placeholder);
#endif
}

ViewportWidget::~ViewportWidget() = default;

bool ViewportWidget::vtkAvailable() noexcept
{
#ifdef FEMCAE_GUI_HAS_VTK
    return true;
#else
    return false;
#endif
}

void ViewportWidget::setContext(const ViewportContext context)
{
    context_ = context;
}

void ViewportWidget::setRepresentation(const SurfaceRepresentation representation)
{
    representation_ = representation;
#ifdef FEMCAE_GUI_HAS_VTK
    if (impl_->surfaceActor == nullptr) {
        return;
    }
    vtkProperty *property = impl_->surfaceActor->GetProperty();
    // Geometri bağlamında kenarlar ayrı feature-edge aktöründen gelir; mesh
    // bağlamında ise element kenarları yüzey aktörünün kendisinde çizilir.
    const bool geometryEdges = impl_->geometryEdgeActor != nullptr;
    switch (representation) {
    case SurfaceRepresentation::Shaded:
        property->SetRepresentationToSurface();
        property->SetEdgeVisibility(0);
        if (geometryEdges) {
            impl_->geometryEdgeActor->SetVisibility(0);
        }
        break;
    case SurfaceRepresentation::ShadedWithEdges:
        property->SetRepresentationToSurface();
        property->SetEdgeVisibility(geometryEdges ? 0 : 1);
        if (geometryEdges) {
            impl_->geometryEdgeActor->SetVisibility(1);
        }
        break;
    case SurfaceRepresentation::Wireframe:
        property->SetRepresentationToWireframe();
        property->SetEdgeVisibility(0);
        if (geometryEdges) {
            impl_->geometryEdgeActor->SetVisibility(0);
        }
        break;
    }
    impl_->render();
#endif
}

bool ViewportWidget::representationMatchesScene() const noexcept
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (impl_->surfaceActor == nullptr) {
        return true;
    }
    vtkProperty *property = impl_->surfaceActor->GetProperty();
    const bool geometryEdges = impl_->geometryEdgeActor != nullptr;
    switch (representation_) {
    case SurfaceRepresentation::Shaded:
        return property->GetRepresentation() == VTK_SURFACE && property->GetEdgeVisibility() == 0
            && (!geometryEdges || impl_->geometryEdgeActor->GetVisibility() == 0);
    case SurfaceRepresentation::ShadedWithEdges:
        return property->GetRepresentation() == VTK_SURFACE
            && property->GetEdgeVisibility() == (geometryEdges ? 0 : 1)
            && (!geometryEdges || impl_->geometryEdgeActor->GetVisibility() != 0);
    case SurfaceRepresentation::Wireframe:
        return property->GetRepresentation() == VTK_WIREFRAME && property->GetEdgeVisibility() == 0
            && (!geometryEdges || impl_->geometryEdgeActor->GetVisibility() == 0);
    }
    return false;
#else
    return true;
#endif
}

void ViewportWidget::clearScene()
{
#ifdef FEMCAE_GUI_HAS_VTK
    for (auto &entry : impl_->actors) {
        impl_->renderer->RemoveActor(entry.actor);
    }
    impl_->actors.clear();
    impl_->surfaceActor = nullptr;
    impl_->geometryEdgeActor = nullptr;
    impl_->highlightActor = nullptr;
    impl_->facetGeometryIds.clear();
    impl_->resultFacetIds.clear();
    impl_->resultScene = false;
    impl_->geometryScene.clear();
    impl_->hasCachedMesh = false;
    impl_->cachedMesh = {};
    impl_->scalarBar->SetVisibility(0);
    impl_->render();
#endif
}

void ViewportWidget::showGeometry(const femcae::geometry::GeometryTessellation &tessellation)
{
#ifdef FEMCAE_GUI_HAS_VTK
    clearScene();
    if (tessellation.triangles.empty()) {
        impl_->render();
        return;
    }
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(static_cast<vtkIdType>(tessellation.points.size()));
    for (std::size_t i = 0; i < tessellation.points.size(); ++i) {
        const auto &p = tessellation.points[i];
        points->SetPoint(static_cast<vtkIdType>(i), p.x, p.y, p.z);
    }
    vtkNew<vtkCellArray> triangles;
    for (const auto &triangle : tessellation.triangles) {
        const vtkIdType ids[3] = {static_cast<vtkIdType>(triangle[0]), static_cast<vtkIdType>(triangle[1]),
                                  static_cast<vtkIdType>(triangle[2])};
        triangles->InsertNextCell(3, ids);
    }
    vtkSmartPointer<vtkPolyData> data = vtkSmartPointer<vtkPolyData>::New();
    data->SetPoints(points);
    data->SetPolys(triangles);

    // Display tessellation: yalnız gösterim. Solver elemanı değildir.
    impl_->surfaceActor = impl_->addActor(data, RenderRole::GeometrySurface, RenderRole::GeometryEdge, false);

    // CAD kenarları üçgenleme kenarlarından ayrı bir aktördür: kullanıcı
    // gövdenin gerçek kenarlarını görür, tessellation köşegenlerini değil.
    vtkNew<vtkFeatureEdges> featureEdges;
    featureEdges->SetInputData(data);
    featureEdges->BoundaryEdgesOn();
    featureEdges->FeatureEdgesOn();
    featureEdges->SetFeatureAngle(24.0);
    featureEdges->ManifoldEdgesOff();
    featureEdges->NonManifoldEdgesOff();
    featureEdges->ColoringOff();
    featureEdges->Update();
    vtkSmartPointer<vtkPolyData> edgeData = vtkSmartPointer<vtkPolyData>::New();
    edgeData->DeepCopy(featureEdges->GetOutput());
    impl_->geometryEdgeActor = impl_->addActor(edgeData, RenderRole::GeometryEdge, RenderRole::GeometryEdge, false);
    impl_->geometryEdgeActor->GetProperty()->SetLineWidth(1.4);
    impl_->geometryEdgeActor->GetProperty()->SetLighting(false);
    // Legacy/parametrik body-level seçim: her üçgen tek kaynak gövdeye aittir.
    impl_->facetGeometryIds.assign(tessellation.triangles.size(), tessellation.sourceGeometryId);
    impl_->applyPalette();
    setRepresentation(representation_);
    setStandardView(StandardView::Isometric);
#else
    Q_UNUSED(tessellation)
#endif
}

void ViewportWidget::showGeometry(const QVector<femcae::geometry::TopologyTessellation> &bodies)
{
#ifdef FEMCAE_GUI_HAS_VTK
    clearScene();
    for (const auto &body : bodies) {
        if (!impl_->geometryScene.append(body)) {
            impl_->geometryScene.clear();
            impl_->render();
            return;
        }
    }
    if (impl_->geometryScene.empty()) {
        impl_->render();
        return;
    }

    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(static_cast<vtkIdType>(impl_->geometryScene.points().size()));
    for (std::size_t i = 0; i < impl_->geometryScene.points().size(); ++i) {
        const auto &p = impl_->geometryScene.points()[i];
        points->SetPoint(static_cast<vtkIdType>(i), p.x, p.y, p.z);
    }
    vtkNew<vtkCellArray> triangles;
    for (const auto &triangle : impl_->geometryScene.triangles()) {
        const vtkIdType ids[3] = {static_cast<vtkIdType>(triangle[0]), static_cast<vtkIdType>(triangle[1]),
                                  static_cast<vtkIdType>(triangle[2])};
        triangles->InsertNextCell(3, ids);
    }
    vtkSmartPointer<vtkPolyData> data = vtkSmartPointer<vtkPolyData>::New();
    data->SetPoints(points);
    data->SetPolys(triangles);

    impl_->surfaceActor = impl_->addActor(data, RenderRole::GeometrySurface, RenderRole::GeometryEdge, false);

    vtkNew<vtkFeatureEdges> featureEdges;
    featureEdges->SetInputData(data);
    featureEdges->BoundaryEdgesOn();
    featureEdges->FeatureEdgesOn();
    featureEdges->SetFeatureAngle(24.0);
    featureEdges->ManifoldEdgesOff();
    featureEdges->NonManifoldEdgesOff();
    featureEdges->ColoringOff();
    featureEdges->Update();
    vtkSmartPointer<vtkPolyData> edgeData = vtkSmartPointer<vtkPolyData>::New();
    edgeData->DeepCopy(featureEdges->GetOutput());
    impl_->geometryEdgeActor = impl_->addActor(edgeData, RenderRole::GeometryEdge, RenderRole::GeometryEdge, false);
    impl_->geometryEdgeActor->GetProperty()->SetLineWidth(1.4);
    impl_->geometryEdgeActor->GetProperty()->SetLighting(false);

    // Topology-aware sahnede facetGeometryIds kullanılmaz. Pick edilen VTK cell
    // GeometrySelectionScene üzerinden Body + Face kimliklerine çözülür.
    impl_->facetGeometryIds.clear();
    impl_->applyPalette();
    setRepresentation(representation_);
    setStandardView(StandardView::Isometric);
#else
    Q_UNUSED(bodies)
#endif
}

bool ViewportWidget::hasTopologyFaceProvenance() const noexcept
{
#ifdef FEMCAE_GUI_HAS_VTK
    return !impl_->geometryScene.empty() && impl_->geometryScene.hasFaceProvenance();
#else
    return false;
#endif
}

#ifdef FEMCAE_GUI_HAS_VTK
namespace {

// Mesh sınır yüzeyini (boundary facet'ler) polydata olarak kurar. Aynı yapı
// Mesh, Loads ve Results bağlamlarında kullanılır: yalnız skaler alanlar ve
// deformasyon ölçeği değişir.
vtkSmartPointer<vtkPolyData> buildBoundarySurface(const SimulationMesh &mesh,
                                                  const femcae::meshing::NodeVectorField *displacement,
                                                  double deformationScale,
                                                  const femcae::meshing::ElementScalarField *elementScalar,
                                                  bool nodalMagnitude,
                                                  std::vector<GeometryEntityId> &facetGeometryIds,
                                                  std::vector<MeshEntityId> *facetIds = nullptr)
{
    facetGeometryIds.clear();
    if (facetIds != nullptr) {
        facetIds->clear();
    }
    vtkSmartPointer<vtkPolyData> data = vtkSmartPointer<vtkPolyData>::New();
    if (mesh.nodes.empty() || mesh.boundaryFacets.empty()) {
        return data;
    }

    std::unordered_map<MeshEntityId, vtkIdType> index;
    index.reserve(mesh.nodes.size());
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(static_cast<vtkIdType>(mesh.nodes.size()));

    vtkSmartPointer<vtkDoubleArray> nodalField;
    if (nodalMagnitude && displacement != nullptr) {
        nodalField = vtkSmartPointer<vtkDoubleArray>::New();
        nodalField->SetName("Total Deformation [mm]");
        nodalField->SetNumberOfComponents(1);
        nodalField->SetNumberOfTuples(static_cast<vtkIdType>(mesh.nodes.size()));
    }

    for (std::size_t i = 0; i < mesh.nodes.size(); ++i) {
        const auto &node = mesh.nodes[i];
        femcae::geometry::Vec3 u{};
        if (displacement != nullptr) {
            const auto it = displacement->values.find(node.id);
            if (it != displacement->values.end()) {
                u = it->second;
            }
        }
        points->SetPoint(static_cast<vtkIdType>(i), node.x.x + deformationScale * u.x,
                         node.x.y + deformationScale * u.y, node.x.z + deformationScale * u.z);
        index[node.id] = static_cast<vtkIdType>(i);
        if (nodalField != nullptr) {
            nodalField->SetValue(static_cast<vtkIdType>(i),
                                 std::sqrt(u.x * u.x + u.y * u.y + u.z * u.z) * 1.0e3);
        }
    }

    // Element -> von Mises eşlemesi hücre skaleri olarak taşınır.
    vtkSmartPointer<vtkDoubleArray> cellField;
    if (elementScalar != nullptr) {
        cellField = vtkSmartPointer<vtkDoubleArray>::New();
        cellField->SetName("Equivalent (von Mises) Stress [MPa]");
        cellField->SetNumberOfComponents(1);
    }

    vtkNew<vtkCellArray> polys;
    for (const auto &facet : mesh.boundaryFacets) {
        vtkIdType ids[4];
        bool valid = true;
        for (int i = 0; i < 4; ++i) {
            const auto it = index.find(facet.nodeIds[static_cast<std::size_t>(i)]);
            if (it == index.end()) {
                valid = false;
                break;
            }
            ids[i] = it->second;
        }
        if (!valid) {
            continue;
        }
        polys->InsertNextCell(4, ids);
        facetGeometryIds.push_back(facet.sourceGeometryId);
        if (facetIds != nullptr) {
            facetIds->push_back(facet.id);
        }
        if (cellField != nullptr) {
            const auto it = elementScalar->values.find(facet.ownerElementId);
            cellField->InsertNextValue(it == elementScalar->values.end() ? 0.0 : it->second / 1.0e6);
        }
    }

    data->SetPoints(points);
    data->SetPolys(polys);
    if (nodalField != nullptr) {
        data->GetPointData()->SetScalars(nodalField);
    }
    if (cellField != nullptr) {
        data->GetCellData()->SetScalars(cellField);
    }
    return data;
}

} // namespace
#endif

void ViewportWidget::showMesh(const SimulationMesh &mesh, const bool showNodes)
{
#ifdef FEMCAE_GUI_HAS_VTK
    clearScene();
    auto data = buildBoundarySurface(mesh, nullptr, 0.0, nullptr, false, impl_->facetGeometryIds);
    if (data->GetNumberOfCells() == 0) {
        impl_->render();
        return;
    }
    // Preprocessing bağlamı: sonuç konturu YOK, nötr FEM yüzeyi + element kenarları.
    impl_->surfaceActor = impl_->addActor(data, RenderRole::MeshSurface, RenderRole::MeshEdge, true);
    impl_->cachedMesh = mesh;
    impl_->hasCachedMesh = true;

    if (showNodes) {
        vtkNew<vtkSphereSource> sphere;
        const auto bounds = computeBounds(mesh);
        sphere->SetRadius(bounds.span * 0.006);
        sphere->SetThetaResolution(8);
        sphere->SetPhiResolution(8);
        vtkNew<vtkGlyph3D> glyph;
        glyph->SetInputData(data);
        glyph->SetSourceConnection(sphere->GetOutputPort());
        glyph->ScalingOff();
        glyph->Update();
        vtkSmartPointer<vtkPolyData> nodes = vtkSmartPointer<vtkPolyData>::New();
        nodes->DeepCopy(glyph->GetOutput());
        impl_->addActor(nodes, RenderRole::MeshNode, RenderRole::MeshNode, false);
    }

    impl_->applyPalette();
    setRepresentation(representation_);
    setStandardView(StandardView::Isometric);
#else
    Q_UNUSED(mesh)
    Q_UNUSED(showNodes)
#endif
}

void ViewportWidget::showModelWithBoundaryConditions(const SimulationMesh &mesh, const QVector<BoundaryGlyph> &glyphs)
{
#ifdef FEMCAE_GUI_HAS_VTK
    showMesh(mesh, false);
    if (!impl_->hasCachedMesh) {
        return;
    }
    const auto bounds = computeBounds(mesh);

    for (const auto &glyph : glyphs) {
        // Kapsanan yüzün düğüm konumlarında sembol yerleştirilir.
        vtkNew<vtkPoints> points;
        for (const auto &facet : mesh.boundaryFacets) {
            if (facet.sourceGeometryId != glyph.geometryId) {
                continue;
            }
            double centre[3] = {0.0, 0.0, 0.0};
            int found = 0;
            for (const auto nodeId : facet.nodeIds) {
                const auto *node = mesh.findNode(nodeId);
                if (node == nullptr) {
                    continue;
                }
                centre[0] += node->x.x;
                centre[1] += node->x.y;
                centre[2] += node->x.z;
                ++found;
            }
            if (found == 0) {
                continue;
            }
            points->InsertNextPoint(centre[0] / found, centre[1] / found, centre[2] / found);
        }
        if (points->GetNumberOfPoints() == 0) {
            continue;
        }
        vtkSmartPointer<vtkPolyData> seeds = vtkSmartPointer<vtkPolyData>::New();
        seeds->SetPoints(points);

        vtkNew<vtkGlyph3D> glyphFilter;
        glyphFilter->SetInputData(seeds);
        glyphFilter->ScalingOff();
        if (glyph.isLoad) {
            vtkNew<vtkArrowSource> arrow;
            arrow->SetTipResolution(12);
            arrow->SetShaftResolution(12);
            arrow->SetTipLength(0.32);
            arrow->SetShaftRadius(0.035);
            arrow->SetTipRadius(0.10);
            glyphFilter->SetSourceConnection(arrow->GetOutputPort());
        } else {
            vtkNew<vtkConeSource> cone;
            cone->SetResolution(4);
            cone->SetHeight(1.0);
            cone->SetRadius(0.42);
            glyphFilter->SetSourceConnection(cone->GetOutputPort());
        }
        glyphFilter->SetScaleFactor(bounds.span * (glyph.isLoad ? 0.10 : 0.055));
        glyphFilter->ScalingOn();
        glyphFilter->SetScaleModeToDataScalingOff();
        glyphFilter->Update();

        vtkSmartPointer<vtkPolyData> glyphData = vtkSmartPointer<vtkPolyData>::New();
        glyphData->DeepCopy(glyphFilter->GetOutput());
        auto actor = impl_->addActor(glyphData,
                                      glyph.isLoad ? RenderRole::LoadGlyph : RenderRole::BoundaryCondition,
                                      glyph.isLoad ? RenderRole::LoadGlyph : RenderRole::BoundaryCondition,
                                      false);
        // Yön: yük vektörü boyunca; mesnet sembolü yüzeye dik içeri bakar.
        const double dx = glyph.dx;
        const double dy = glyph.dy;
        const double dz = glyph.dz;
        const double norm = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (norm > 1.0e-12) {
            const double ux = dx / norm;
            const double uy = dy / norm;
            const double uz = dz / norm;
            // vtkArrowSource/vtkConeSource +X yönünde tanımlıdır.
            const double azimuth = std::atan2(uy, ux) * 180.0 / M_PI;
            const double elevation = std::asin(std::clamp(uz, -1.0, 1.0)) * 180.0 / M_PI;
            actor->SetOrientation(0.0, -elevation, azimuth);
        }
    }

    impl_->applyPalette();
    setStandardView(StandardView::Isometric);
#else
    Q_UNUSED(mesh)
    Q_UNUSED(glyphs)
#endif
}

void ViewportWidget::showResult(const SimulationMesh &mesh, const femcae::meshing::ResultDatabase &results,
                                const ResultField field)
{
#ifdef FEMCAE_GUI_HAS_VTK
    clearScene();
    const auto *displacement = results.displacement();
    const auto *stress = results.elementScalar("von_mises");

    // Deformasyon ölçeği: model açıklığının ~%12'si kadar görsel abartma.
    double maxDisplacement = 0.0;
    if (displacement != nullptr) {
        for (const auto &entry : displacement->values) {
            const auto &u = entry.second;
            maxDisplacement = std::max(maxDisplacement, std::sqrt(u.x * u.x + u.y * u.y + u.z * u.z));
        }
    }
    const auto bounds = computeBounds(mesh);
    const double scale = maxDisplacement > 0.0
        ? std::min(500.0, 0.12 * bounds.span / maxDisplacement)
        : 0.0;

    const bool useNodalMagnitude = field == ResultField::TotalDeformation;
    const bool useCellStress = field == ResultField::EquivalentStress;

    auto data = buildBoundarySurface(mesh, displacement, scale, useCellStress ? stress : nullptr,
                                     useNodalMagnitude, impl_->facetGeometryIds,
                                     &impl_->resultFacetIds);
    if (data->GetNumberOfCells() == 0) {
        impl_->render();
        return;
    }
    impl_->cachedMesh = mesh;
    impl_->hasCachedMesh = true;
    impl_->resultScene = true;

    if (field == ResultField::ReactionForce) {
        // Reaksiyon kuvveti skaler bir alan değildir; deforme model nötr
        // gösterilir, sayısal değer Details panelinde verilir.
        impl_->surfaceActor = impl_->addActor(data, RenderRole::MeshSurface, RenderRole::MeshEdge, true);
        impl_->scalarBar->SetVisibility(0);
        impl_->applyPalette();
        setStandardView(StandardView::Isometric);
        return;
    }

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(data);
    mapper->SetLookupTable(impl_->lookupTable);
    mapper->ScalarVisibilityOn();
    double range[2] = {0.0, 1.0};
    if (useNodalMagnitude) {
        mapper->SetScalarModeToUsePointData();
        data->GetPointData()->GetScalars()->GetRange(range);
        impl_->scalarBar->SetTitle("Total Deformation\n[mm]");
    } else {
        mapper->SetScalarModeToUseCellData();
        data->GetCellData()->GetScalars()->GetRange(range);
        impl_->scalarBar->SetTitle("Equivalent Stress\n[MPa]");
    }
    if (std::abs(range[1] - range[0]) < 1.0e-14) {
        range[1] = range[0] + 1.0e-12;
    }
    impl_->lookupTable->SetTableRange(range);
    impl_->lookupTable->Build();
    mapper->SetScalarRange(range);

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetEdgeVisibility(1);
    actor->GetProperty()->SetLineWidth(0.6);
    actor->GetProperty()->SetAmbient(0.35);
    actor->GetProperty()->SetDiffuse(0.65);
    actor->GetProperty()->SetSpecular(0.0);

    Impl::RoleActor entry;
    entry.actor = actor;
    entry.surfaceRole = RenderRole::ResultContour;
    entry.edgeRole = RenderRole::MeshEdge;
    entry.useEdgeRole = true;
    entry.scalarDriven = true; // rengi lookup table belirler, palet değil
    impl_->actors.push_back(entry);
    impl_->renderer->AddActor(actor);
    impl_->surfaceActor = actor;

    // Deforme olmamış referans şekli ince tel kafes olarak gösterilir.
    if (scale > 0.0) {
        std::vector<GeometryEntityId> ignored;
        auto reference = buildBoundarySurface(mesh, nullptr, 0.0, nullptr, false, ignored);
        auto referenceActor = impl_->addActor(reference, RenderRole::ReferenceShape, RenderRole::ReferenceShape, false);
        referenceActor->GetProperty()->SetRepresentationToWireframe();
        referenceActor->GetProperty()->SetLineWidth(0.8);
        referenceActor->GetProperty()->SetOpacity(0.45);
    }

    impl_->scalarBar->SetVisibility(1);
    impl_->applyPalette();
    setStandardView(StandardView::Isometric);
#else
    Q_UNUSED(mesh)
    Q_UNUSED(results)
    Q_UNUSED(field)
#endif
}

void ViewportWidget::setHighlightedGeometry(const GeometryEntityId geometryId)
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (impl_->highlighted == geometryId) {
        return;
    }
    impl_->highlighted = geometryId;
    impl_->rebuildHighlight();
    impl_->render();
#else
    Q_UNUSED(geometryId)
#endif
}

void ViewportWidget::fitView()
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (impl_->cameraController != nullptr) {
        (void)impl_->cameraController->fit();
        if (impl_->interactorStyle != nullptr) {
            const auto &pivot = impl_->cameraController->rotationCenter();
            impl_->interactorStyle->SetCenterOfRotation(pivot.data());
        }
    }
#endif
}

void ViewportWidget::resetCamera()
{
    fitView();
}

void ViewportWidget::setStandardView(const StandardView view)
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (impl_->cameraController != nullptr) {
        impl_->cameraController->setStandardView(view);
        if (impl_->interactorStyle != nullptr) {
            const auto &pivot = impl_->cameraController->rotationCenter();
            impl_->interactorStyle->SetCenterOfRotation(pivot.data());
        }
    }
#else
    Q_UNUSED(view)
#endif
}

void ViewportWidget::setIsometricView()
{
    setStandardView(StandardView::Isometric);
}

bool ViewportWidget::zoomToBounds(const std::array<double, 6> &bounds)
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (impl_->cameraController == nullptr || !impl_->cameraController->zoomToBounds(bounds)) {
        return false;
    }
    if (impl_->interactorStyle != nullptr) {
        const auto &pivot = impl_->cameraController->rotationCenter();
        impl_->interactorStyle->SetCenterOfRotation(pivot.data());
    }
    return true;
#else
    Q_UNUSED(bounds)
    return false;
#endif
}

void ViewportWidget::setRotationCenter(const std::array<double, 3> &worldPoint)
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (impl_->cameraController != nullptr) {
        impl_->cameraController->setRotationCenter(worldPoint);
        if (impl_->interactorStyle != nullptr) {
            impl_->interactorStyle->SetCenterOfRotation(impl_->cameraController->rotationCenter().data());
        }
    }
#else
    Q_UNUSED(worldPoint)
#endif
}

bool ViewportWidget::setRotationCenterToHighlightedGeometry()
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (impl_->highlighted == InvalidGeometryId || impl_->surfaceActor == nullptr) {
        return false;
    }
    vtkActor *boundsActor = impl_->highlightActor != nullptr ? impl_->highlightActor.GetPointer()
                                                            : impl_->surfaceActor.GetPointer();
    const double *actorBounds = boundsActor->GetBounds();
    if (actorBounds == nullptr) {
        return false;
    }
    std::array<double, 6> bounds{};
    std::copy(actorBounds, actorBounds + 6, bounds.begin());
    return setRotationCenterToBounds(bounds);
#else
    return false;
#endif
}

bool ViewportWidget::setRotationCenterToBounds(const std::array<double, 6> &bounds)
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (impl_->cameraController == nullptr || !impl_->cameraController->setRotationCenterToBounds(bounds)) {
        return false;
    }
    if (impl_->interactorStyle != nullptr) {
        impl_->interactorStyle->SetCenterOfRotation(impl_->cameraController->rotationCenter().data());
    }
    return true;
#else
    Q_UNUSED(bounds)
    return false;
#endif
}

bool ViewportWidget::resetRotationCenter()
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (impl_->cameraController == nullptr || !impl_->cameraController->resetRotationCenter()) {
        return false;
    }
    if (impl_->interactorStyle != nullptr) {
        impl_->interactorStyle->SetCenterOfRotation(impl_->cameraController->rotationCenter().data());
    }
    return true;
#else
    return false;
#endif
}

std::array<double, 3> ViewportWidget::rotationCenter() const
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (impl_->cameraController != nullptr) {
        return impl_->cameraController->rotationCenter();
    }
#endif
    return {0.0, 0.0, 0.0};
}

bool ViewportWidget::hasAxisTriad() const noexcept
{
#ifdef FEMCAE_GUI_HAS_VTK
    return impl_->axesActor != nullptr && impl_->orientationMarker != nullptr
        && impl_->orientationMarker->GetEnabled() != 0 && impl_->orientationMarker->GetInteractive() == 0;
#else
    return false;
#endif
}

bool ViewportWidget::hasOrientationCube() const noexcept
{
#ifdef FEMCAE_GUI_HAS_CAMERA_ORIENTATION_WIDGET
    return impl_->cameraOrientation != nullptr && impl_->cameraOrientation->GetEnabled() != 0
        && impl_->cameraOrientation->GetParentRenderer() == impl_->renderer;
#else
    return false;
#endif
}

bool ViewportWidget::orientationCubeAvailable() noexcept
{
#ifdef FEMCAE_GUI_HAS_CAMERA_ORIENTATION_WIDGET
    return true;
#else
    return false;
#endif
}

void ViewportWidget::refreshAppearance()
{
#ifdef FEMCAE_GUI_HAS_VTK
    // Light -> Dark -> Light geçişinde tüm roller yeniden uygulanır; hiçbir
    // aktör önceki paletten renk taşımaz.
    impl_->palette = ViewportPalette::forAppearance(systemPrefersDark());
    impl_->applyPalette();
    impl_->rebuildHighlight();
    impl_->render();
#endif
}

QImage ViewportWidget::grabRenderedImage()
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (impl_->widget == nullptr || impl_->widget->renderWindow() == nullptr) {
        return {};
    }
    impl_->widget->renderWindow()->Render();
    return impl_->widget->grabFramebuffer();
#else
    return {};
#endif
}

void ViewportWidget::handlePick(const int x, const int y)
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (impl_->surfaceActor == nullptr || (impl_->facetGeometryIds.empty() && impl_->geometryScene.empty())) {
        return;
    }
    impl_->picker->InitializePickList();
    impl_->picker->AddPickList(impl_->surfaceActor);
    impl_->picker->PickFromListOn();
    if (impl_->picker->Pick(x, y, 0, impl_->renderer) == 0) {
        if (impl_->resultScene) {
            emit resultPicked(0.0, 0.0, 0.0, static_cast<qint64>(femcae::meshing::InvalidMeshId));
        } else if (!impl_->geometryScene.empty()) {
            emit topologyPicked(0, 0);
        } else {
            emit geometryPicked(0);
        }
        return;
    }
    const vtkIdType cellId = impl_->picker->GetCellId();
    if (cellId < 0) {
        return;
    }
    if (impl_->resultScene) {
        const MeshEntityId facetId =
            cellId < static_cast<vtkIdType>(impl_->resultFacetIds.size())
            ? impl_->resultFacetIds[static_cast<std::size_t>(cellId)]
            : femcae::meshing::InvalidMeshId;
        double world[3] = {0.0, 0.0, 0.0};
        impl_->picker->GetPickPosition(world);
        emit resultPicked(world[0], world[1], world[2], static_cast<qint64>(facetId));
        return;
    }
    if (!impl_->geometryScene.empty()) {
        const auto provenance = impl_->geometryScene.provenanceForCell(static_cast<std::size_t>(cellId));
        if (!provenance.has_value()) {
            return;
        }
        emit topologyPicked(static_cast<quint64>(provenance->bodyId),
                            static_cast<quint64>(provenance->faceId));
        return;
    }
    if (cellId >= static_cast<vtkIdType>(impl_->facetGeometryIds.size())) {
        return;
    }
    emit geometryPicked(static_cast<quint64>(impl_->facetGeometryIds[static_cast<std::size_t>(cellId)]));
#else
    Q_UNUSED(x)
    Q_UNUSED(y)
#endif
}

bool ViewportWidget::event(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
        refreshAppearance();
    }
    return QWidget::event(event);
}

} // namespace d26
