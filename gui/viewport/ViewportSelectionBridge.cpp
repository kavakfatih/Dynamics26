#include "ViewportSelectionBridge.h"

#include "RenderRoles.h"
#include "ViewportWidget.h"

#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPalette>

#include <algorithm>
#include <cmath>
#include <utility>

#ifdef FEMCAE_GUI_HAS_VTK
#include <QVTKOpenGLNativeWidget.h>

#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkCellArray.h>
#include <vtkCellPicker.h>
#include <vtkMapper.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>
#endif

namespace d26 {
namespace {

bool systemPrefersDark()
{
    return qApp != nullptr
        && qApp->palette().color(QPalette::Window).lightnessF() < 0.5;
}

} // namespace

class ViewportSelectionBridge::Impl
{
public:
    GeometrySelectionScene scene;
    QVector<SelectionItem> selection;
    std::optional<SelectionItem> preselection;

#ifdef FEMCAE_GUI_HAS_VTK
    QVTKOpenGLNativeWidget *widget{nullptr};
    vtkWeakPointer<vtkRenderer> renderer;
    vtkWeakPointer<vtkActor> surfaceActor;
    vtkSmartPointer<vtkCellPicker> picker;
    vtkSmartPointer<vtkActor> selectionActor;
    vtkSmartPointer<vtkActor> preselectionActor;

    void bind(ViewportWidget *viewport)
    {
        widget = viewport != nullptr ? viewport->findChild<QVTKOpenGLNativeWidget *>() : nullptr;
        renderer = nullptr;
        surfaceActor = nullptr;

        if (widget == nullptr || widget->renderWindow() == nullptr) {
            return;
        }
        vtkRendererCollection *renderers = widget->renderWindow()->GetRenderers();
        if (renderers == nullptr) {
            return;
        }
        renderer = renderers->GetFirstRenderer();
        if (picker == nullptr) {
            picker = vtkSmartPointer<vtkCellPicker>::New();
            picker->SetTolerance(0.005);
        }
    }

    void removeOverlay(vtkSmartPointer<vtkActor> &actor)
    {
        if (actor != nullptr && renderer != nullptr) {
            renderer->RemoveActor(actor);
        }
        actor = nullptr;
    }

    void clearVisualState()
    {
        removeOverlay(selectionActor);
        removeOverlay(preselectionActor);
        surfaceActor = nullptr;
    }

    [[nodiscard]] bool resolveSurfaceActor()
    {
        surfaceActor = nullptr;
        if (renderer == nullptr || scene.empty()) {
            return false;
        }

        vtkActorCollection *actors = renderer->GetActors();
        if (actors == nullptr) {
            return false;
        }
        actors->InitTraversal();
        while (vtkActor *actor = actors->GetNextActor()) {
            if (actor == nullptr || actor->GetPickable() == 0) {
                continue;
            }
            auto *mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
            if (mapper == nullptr) {
                continue;
            }
            vtkPolyData *data = mapper->GetInput();
            if (data == nullptr) {
                continue;
            }
            if (data->GetNumberOfPolys() == static_cast<vtkIdType>(scene.triangles().size())
                && data->GetNumberOfPoints() == static_cast<vtkIdType>(scene.points().size())) {
                surfaceActor = actor;
                break;
            }
        }
        return surfaceActor != nullptr;
    }

    void applyOverlayPalette()
    {
        const ViewportPalette palette = ViewportPalette::forAppearance(systemPrefersDark());
        if (selectionActor != nullptr) {
            const Rgb color = palette.color(RenderRole::Selection);
            selectionActor->GetProperty()->SetColor(color.r, color.g, color.b);
            selectionActor->GetProperty()->SetEdgeColor(color.r, color.g, color.b);
        }
        if (preselectionActor != nullptr) {
            const Rgb color = palette.color(RenderRole::Preselection);
            preselectionActor->GetProperty()->SetColor(color.r, color.g, color.b);
            preselectionActor->GetProperty()->SetEdgeColor(color.r, color.g, color.b);
        }
    }

    vtkSmartPointer<vtkActor> buildOverlay(const QVector<SelectionItem> &items,
                                           const RenderRole role,
                                           const double opacity,
                                           const double lineWidth,
                                           const double polygonOffset)
    {
        const auto cells = scene.cellIndicesForSelection(items);
        if (cells.empty()) {
            return nullptr;
        }

        vtkNew<vtkPoints> points;
        points->SetNumberOfPoints(static_cast<vtkIdType>(scene.points().size()));
        for (std::size_t i = 0; i < scene.points().size(); ++i) {
            const auto &p = scene.points()[i];
            points->SetPoint(static_cast<vtkIdType>(i), p.x, p.y, p.z);
        }

        vtkNew<vtkCellArray> polys;
        for (const std::size_t cell : cells) {
            if (cell >= scene.triangles().size()) {
                continue;
            }
            const auto &triangle = scene.triangles()[cell];
            const vtkIdType ids[3] = {
                static_cast<vtkIdType>(triangle[0]),
                static_cast<vtkIdType>(triangle[1]),
                static_cast<vtkIdType>(triangle[2])
            };
            polys->InsertNextCell(3, ids);
        }
        if (polys->GetNumberOfCells() == 0) {
            return nullptr;
        }

        vtkNew<vtkPolyData> data;
        data->SetPoints(points);
        data->SetPolys(polys);

        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetInputData(data);
        mapper->ScalarVisibilityOff();
        mapper->SetResolveCoincidentTopologyToPolygonOffset();
        mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(polygonOffset, polygonOffset);

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->PickableOff();
        actor->GetProperty()->SetEdgeVisibility(1);
        actor->GetProperty()->SetLineWidth(lineWidth);
        actor->GetProperty()->SetOpacity(opacity);
        actor->GetProperty()->SetLighting(false);
        actor->GetProperty()->SetAmbient(1.0);
        actor->GetProperty()->SetDiffuse(0.0);

        const ViewportPalette palette = ViewportPalette::forAppearance(systemPrefersDark());
        const Rgb color = palette.color(role);
        actor->GetProperty()->SetColor(color.r, color.g, color.b);
        actor->GetProperty()->SetEdgeColor(color.r, color.g, color.b);
        return actor;
    }

    void rebuildSelectionOverlay()
    {
        removeOverlay(selectionActor);
        if (renderer == nullptr || scene.empty() || selection.isEmpty()) {
            return;
        }
        selectionActor = buildOverlay(selection, RenderRole::Selection, 0.42, 2.2, -4.0);
        if (selectionActor != nullptr) {
            renderer->AddActor(selectionActor);
        }
    }

    void rebuildPreselectionOverlay()
    {
        removeOverlay(preselectionActor);
        if (renderer == nullptr || scene.empty() || !preselection.has_value()) {
            return;
        }
        preselectionActor = buildOverlay(QVector<SelectionItem>{*preselection},
                                         RenderRole::Preselection, 0.24, 1.5, -6.0);
        if (preselectionActor != nullptr) {
            renderer->AddActor(preselectionActor);
        }
    }

    void render()
    {
        if (widget != nullptr && widget->renderWindow() != nullptr) {
            widget->renderWindow()->Render();
        }
    }

    [[nodiscard]] std::optional<GeometrySceneCellProvenance> pick(const QPointF &position)
    {
        if (widget == nullptr || renderer == nullptr || surfaceActor == nullptr
            || picker == nullptr || widget->renderWindow() == nullptr || scene.empty()) {
            return std::nullopt;
        }

        const int *renderSize = widget->renderWindow()->GetSize();
        if (renderSize == nullptr || renderSize[0] <= 0 || renderSize[1] <= 0
            || widget->width() <= 0 || widget->height() <= 0) {
            return std::nullopt;
        }

        const double sx = static_cast<double>(renderSize[0]) / static_cast<double>(widget->width());
        const double sy = static_cast<double>(renderSize[1]) / static_cast<double>(widget->height());
        const int x = std::clamp(static_cast<int>(std::lround(position.x() * sx)), 0, renderSize[0] - 1);
        const int y = std::clamp(static_cast<int>(std::lround(
                                     (static_cast<double>(widget->height() - 1) - position.y()) * sy)),
                                 0, renderSize[1] - 1);

        picker->InitializePickList();
        picker->AddPickList(surfaceActor);
        picker->PickFromListOn();
        if (picker->Pick(x, y, 0.0, renderer) == 0) {
            return std::nullopt;
        }
        const vtkIdType cellId = picker->GetCellId();
        if (cellId < 0) {
            return std::nullopt;
        }
        return scene.provenanceForCell(static_cast<std::size_t>(cellId));
    }
#endif
};

ViewportSelectionBridge::ViewportSelectionBridge(ViewportWidget *viewport, QObject *parent)
    : QObject(parent), impl_(std::make_unique<Impl>()), viewport_(viewport)
{
#ifdef FEMCAE_GUI_HAS_VTK
    impl_->bind(viewport_);
    if (impl_->widget != nullptr) {
        // Bu filtre event'i tüketmez. Navigation event'leri mevcut
        // ViewportInputRouter'a ulaşmaya devam eder; selection state machine
        // Alt/Option + left, middle drag ve wheel'i selection olarak yorumlamaz.
        impl_->widget->installEventFilter(this);
    }
#else
    Q_UNUSED(viewport_)
#endif
}

ViewportSelectionBridge::~ViewportSelectionBridge() = default;

bool ViewportSelectionBridge::setScene(const QVector<femcae::geometry::TopologyTessellation> &bodies)
{
    impl_->scene.clear();
    for (const auto &body : bodies) {
        if (!impl_->scene.append(body)) {
            impl_->scene.clear();
            clearScene();
            return false;
        }
    }
    if (impl_->scene.empty() || viewport_ == nullptr) {
        clearScene();
        return false;
    }

#ifdef FEMCAE_GUI_HAS_VTK
    // Eski transient overlay aktörleri yeni base-scene camera fit hesabına
    // karışmadan önce renderer'dan çıkarılır.
    impl_->clearVisualState();

    // Base CAD render'i yine ViewportWidget üretir. Bridge yalnız bu render'in
    // provenance ve transient overlay katmanını bağlar.
    viewport_->showGeometry(bodies);
    impl_->bind(viewport_);
    if (!impl_->resolveSurfaceActor()) {
        impl_->scene.clear();
        return false;
    }
    impl_->rebuildSelectionOverlay();
    impl_->rebuildPreselectionOverlay();
    impl_->render();
    return impl_->scene.hasFaceProvenance();
#else
    Q_UNUSED(bodies)
    return false;
#endif
}

void ViewportSelectionBridge::clearScene()
{
#ifdef FEMCAE_GUI_HAS_VTK
    impl_->clearVisualState();
    impl_->render();
#endif
    impl_->scene.clear();
}

bool ViewportSelectionBridge::hasFaceProvenance() const noexcept
{
    return !impl_->scene.empty() && impl_->scene.hasFaceProvenance();
}

void ViewportSelectionBridge::setSelection(const QVector<SelectionItem> &items)
{
    impl_->selection = items;
#ifdef FEMCAE_GUI_HAS_VTK
    impl_->rebuildSelectionOverlay();
    impl_->render();
#endif
}

void ViewportSelectionBridge::setPreselection(std::optional<SelectionItem> item)
{
    impl_->preselection = std::move(item);
#ifdef FEMCAE_GUI_HAS_VTK
    impl_->rebuildPreselectionOverlay();
    impl_->render();
#endif
}

bool ViewportSelectionBridge::eventFilter(QObject *watched, QEvent *event)
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (watched != impl_->widget || event == nullptr) {
        return QObject::eventFilter(watched, event);
    }

    std::optional<SelectionInputAction> action;
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove: {
        auto *mouse = static_cast<QMouseEvent *>(event);
        SelectionPointerInput input;
        input.position = mouse->position();
        input.button = mouse->button();
        input.buttons = mouse->buttons();
        input.modifiers = mouse->modifiers();
        input.type = event->type() == QEvent::MouseButtonPress
            ? SelectionPointerEventType::Press
            : (event->type() == QEvent::MouseButtonRelease
                   ? SelectionPointerEventType::Release
                   : SelectionPointerEventType::Move);
        action = input_.routePointer(input);
        break;
    }
    case QEvent::Leave: {
        SelectionPointerInput input;
        input.type = SelectionPointerEventType::Leave;
        action = input_.routePointer(input);
        break;
    }
    case QEvent::KeyPress: {
        auto *key = static_cast<QKeyEvent *>(event);
        action = input_.routeKey(key->key(), key->modifiers());
        break;
    }
    case QEvent::PaletteChange:
    case QEvent::ApplicationPaletteChange:
        impl_->applyOverlayPalette();
        impl_->render();
        break;
    default:
        break;
    }

    if (!action.has_value()) {
        return QObject::eventFilter(watched, event);
    }

    switch (action->type) {
    case SelectionInputActionType::Clear:
        emit selectionClearRequested();
        break;
    case SelectionInputActionType::ClearPreselection:
        emit preselectionClearRequested();
        break;
    case SelectionInputActionType::Hover: {
        const auto hit = impl_->pick(action->position);
        if (hit.has_value()) {
            emit preselectionRequested(static_cast<quint64>(hit->bodyId),
                                       static_cast<quint64>(hit->faceId));
        } else {
            emit preselectionClearRequested();
        }
        break;
    }
    case SelectionInputActionType::Commit: {
        const auto hit = impl_->pick(action->position);
        if (hit.has_value()) {
            emit selectionRequested(static_cast<quint64>(hit->bodyId),
                                    static_cast<quint64>(hit->faceId),
                                    action->operation);
        } else {
            emit selectionClearRequested();
        }
        break;
    }
    case SelectionInputActionType::ContextMenu: {
        // Empty secondary click committed selection'i korur ve menu acmaz.
        // Hit varsa provenance application coordinator'a aktarilir; preserve vs
        // Replace karari rendering katmaninda verilmez.
        const auto hit = impl_->pick(action->position);
        if (hit.has_value()) {
            emit contextMenuRequested(static_cast<quint64>(hit->bodyId),
                                      static_cast<quint64>(hit->faceId),
                                      impl_->widget->mapToGlobal(action->position.toPoint()));
        }
        break;
    }
    }

    // Selection view-state gözlemci olarak çalışır; Qt/VTK event'i tüketilmez.
    // ViewportInputRouter plain right-click'i context-menu territory olarak
    // bırakır; VTK interactor'un right-button camera davranışı da inerttir.
    return QObject::eventFilter(watched, event);
#else
    Q_UNUSED(watched)
    Q_UNUSED(event)
    return false;
#endif
}

} // namespace d26
