#include "ViewportMeshSelectionBridge.h"

#include "RenderRoles.h"
#include "ViewportWidget.h"

#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPalette>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

#ifdef FEMCAE_GUI_HAS_VTK
#include "MeshHardwareSelector.h"

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

bool isMeshKind(const SelectionKind kind) noexcept
{
    return kind == SelectionKind::Node || kind == SelectionKind::Element
        || kind == SelectionKind::Facet;
}

} // namespace

class ViewportMeshSelectionBridge::Impl
{
public:
    MeshSelectionScene scene;
    QVector<SelectionItem> selection;
    std::optional<SelectionItem> preselection;
    SelectionKind activeKind{SelectionKind::Node};

#ifdef FEMCAE_GUI_HAS_VTK
    QVTKOpenGLNativeWidget *widget{nullptr};
    vtkWeakPointer<vtkRenderer> renderer;
    vtkWeakPointer<vtkActor> surfaceActor;
    vtkSmartPointer<vtkCellPicker> picker;
    vtkSmartPointer<vtkActor> nodeActor;
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
        }
    }

    void removeActor(vtkSmartPointer<vtkActor> &actor)
    {
        if (actor != nullptr && renderer != nullptr) {
            renderer->RemoveActor(actor);
        }
        actor = nullptr;
    }

    void clearVisualState()
    {
        removeActor(selectionActor);
        removeActor(preselectionActor);
        removeActor(nodeActor);
        surfaceActor = nullptr;
    }

    [[nodiscard]] bool resolveMeshSurfaceActor()
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
            if (data->GetNumberOfPolys() == static_cast<vtkIdType>(scene.boundaryQuads().size())
                && data->GetNumberOfPoints() == static_cast<vtkIdType>(scene.points().size())) {
                surfaceActor = actor;
                break;
            }
        }
        return surfaceActor != nullptr;
    }

    [[nodiscard]] vtkSmartPointer<vtkActor> buildNodeActor() const
    {
        if (scene.visibleNodePoints().empty()) {
            return nullptr;
        }
        vtkNew<vtkPoints> points;
        vtkNew<vtkCellArray> verts;
        points->SetNumberOfPoints(static_cast<vtkIdType>(scene.visibleNodePoints().size()));
        for (std::size_t i = 0; i < scene.visibleNodePoints().size(); ++i) {
            const auto &p = scene.visibleNodePoints()[i];
            points->SetPoint(static_cast<vtkIdType>(i), p.x, p.y, p.z);
            const vtkIdType id = static_cast<vtkIdType>(i);
            verts->InsertNextCell(1, &id);
        }
        vtkNew<vtkPolyData> data;
        data->SetPoints(points);
        data->SetVerts(verts);
        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetInputData(data);
        mapper->ScalarVisibilityOff();

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->PickableOn();
        actor->GetProperty()->SetPointSize(7.0);
        actor->GetProperty()->SetLighting(false);
        actor->GetProperty()->SetAmbient(1.0);
        actor->GetProperty()->SetDiffuse(0.0);
        return actor;
    }

    void applyPalette()
    {
        const ViewportPalette palette = ViewportPalette::forAppearance(systemPrefersDark());
        if (nodeActor != nullptr) {
            const Rgb color = palette.color(RenderRole::MeshNode);
            nodeActor->GetProperty()->SetColor(color.r, color.g, color.b);
        }
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

    void updateBaseVisibility()
    {
        if (nodeActor != nullptr) {
            nodeActor->SetVisibility(activeKind == SelectionKind::Node ? 1 : 0);
        }
    }

    [[nodiscard]] vtkSmartPointer<vtkActor> buildNodeOverlay(const QVector<SelectionItem> &items,
                                                              const RenderRole role,
                                                              const double pointSize) const
    {
        const auto selected = scene.visibleNodeIndicesForSelection(items);
        if (selected.empty()) {
            return nullptr;
        }
        vtkNew<vtkPoints> points;
        vtkNew<vtkCellArray> verts;
        for (const std::size_t index : selected) {
            const auto &p = scene.visibleNodePoints()[index];
            const vtkIdType local = points->InsertNextPoint(p.x, p.y, p.z);
            verts->InsertNextCell(1, &local);
        }
        vtkNew<vtkPolyData> data;
        data->SetPoints(points);
        data->SetVerts(verts);
        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetInputData(data);
        mapper->ScalarVisibilityOff();

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->PickableOff();
        actor->GetProperty()->SetPointSize(pointSize);
        actor->GetProperty()->SetLighting(false);
        const Rgb color = ViewportPalette::forAppearance(systemPrefersDark()).color(role);
        actor->GetProperty()->SetColor(color.r, color.g, color.b);
        return actor;
    }

    [[nodiscard]] vtkSmartPointer<vtkActor> buildSurfaceOverlay(const QVector<SelectionItem> &items,
                                                                 const RenderRole role,
                                                                 const bool preselectionOverlay) const
    {
        const auto selected = scene.boundaryCellsForSelection(items);
        if (selected.empty()) {
            return nullptr;
        }
        vtkNew<vtkPoints> points;
        points->SetNumberOfPoints(static_cast<vtkIdType>(scene.points().size()));
        for (std::size_t i = 0; i < scene.points().size(); ++i) {
            const auto &p = scene.points()[i];
            points->SetPoint(static_cast<vtkIdType>(i), p.x, p.y, p.z);
        }
        vtkNew<vtkCellArray> polys;
        for (const std::size_t cell : selected) {
            const auto &quad = scene.boundaryQuads()[cell];
            const vtkIdType ids[4] = {static_cast<vtkIdType>(quad[0]), static_cast<vtkIdType>(quad[1]),
                                      static_cast<vtkIdType>(quad[2]), static_cast<vtkIdType>(quad[3])};
            polys->InsertNextCell(4, ids);
        }
        vtkNew<vtkPolyData> data;
        data->SetPoints(points);
        data->SetPolys(polys);
        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetInputData(data);
        mapper->ScalarVisibilityOff();
        mapper->SetResolveCoincidentTopologyToPolygonOffset();
        mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(preselectionOverlay ? -6.0 : -4.0,
                                                                      preselectionOverlay ? -6.0 : -4.0);

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->PickableOff();
        actor->GetProperty()->SetEdgeVisibility(1);
        actor->GetProperty()->SetLineWidth(preselectionOverlay ? 1.8 : 2.4);
        actor->GetProperty()->SetOpacity(preselectionOverlay ? 0.24 : 0.40);
        actor->GetProperty()->SetLighting(false);
        actor->GetProperty()->SetAmbient(1.0);
        actor->GetProperty()->SetDiffuse(0.0);
        const Rgb color = ViewportPalette::forAppearance(systemPrefersDark()).color(role);
        actor->GetProperty()->SetColor(color.r, color.g, color.b);
        actor->GetProperty()->SetEdgeColor(color.r, color.g, color.b);
        return actor;
    }

    [[nodiscard]] vtkSmartPointer<vtkActor> buildOverlay(const QVector<SelectionItem> &items,
                                                          const RenderRole role,
                                                          const bool preselectionOverlay) const
    {
        if (activeKind == SelectionKind::Node) {
            return buildNodeOverlay(items, role, preselectionOverlay ? 10.0 : 12.0);
        }
        if (activeKind == SelectionKind::Element || activeKind == SelectionKind::Facet) {
            return buildSurfaceOverlay(items, role, preselectionOverlay);
        }
        return nullptr;
    }

    void rebuildSelectionOverlay()
    {
        removeActor(selectionActor);
        if (renderer == nullptr || scene.empty() || selection.isEmpty()) {
            return;
        }
        selectionActor = buildOverlay(selection, RenderRole::Selection, false);
        if (selectionActor != nullptr) {
            renderer->AddActor(selectionActor);
        }
    }

    void rebuildPreselectionOverlay()
    {
        removeActor(preselectionActor);
        if (renderer == nullptr || scene.empty() || !preselection.has_value()) {
            return;
        }
        preselectionActor = buildOverlay(QVector<SelectionItem>{*preselection}, RenderRole::Preselection, true);
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

    [[nodiscard]] vtkActor *targetActor() const noexcept
    {
        if (activeKind == SelectionKind::Node) {
            return nodeActor;
        }
        if (activeKind == SelectionKind::Element || activeKind == SelectionKind::Facet) {
            return surfaceActor;
        }
        return nullptr;
    }

    [[nodiscard]] std::optional<std::array<unsigned int, 2>> renderPosition(const QPointF &position) const
    {
        if (widget == nullptr || widget->renderWindow() == nullptr) {
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
        return std::array<unsigned int, 2>{static_cast<unsigned int>(x), static_cast<unsigned int>(y)};
    }

    [[nodiscard]] std::optional<SelectionItem> pick(const QPointF &position)
    {
        if (widget == nullptr || renderer == nullptr || picker == nullptr
            || widget->renderWindow() == nullptr || scene.empty()) {
            return std::nullopt;
        }

        vtkActor *target = targetActor();
        if (target == nullptr) {
            return std::nullopt;
        }
        picker->SetTolerance(activeKind == SelectionKind::Node ? 0.014 : 0.005);

        const auto renderPoint = renderPosition(position);
        if (!renderPoint.has_value()) {
            return std::nullopt;
        }

        picker->InitializePickList();
        picker->AddPickList(target);
        picker->PickFromListOn();
        if (picker->Pick((*renderPoint)[0], (*renderPoint)[1], 0.0, renderer) == 0) {
            return std::nullopt;
        }
        const vtkIdType cellId = picker->GetCellId();
        if (cellId < 0) {
            return std::nullopt;
        }
        const std::size_t cell = static_cast<std::size_t>(cellId);
        if (activeKind == SelectionKind::Node) {
            return scene.selectionItemForVisibleNode(cell);
        }
        return scene.selectionItemForBoundaryCell(cell, activeKind);
    }

    [[nodiscard]] QVector<SelectionItem> pickWindow(const QPointF &anchor, const QPointF &position)
    {
        QVector<SelectionItem> items;
        if (renderer == nullptr || scene.empty()) {
            return items;
        }
        vtkActor *target = targetActor();
        if (target == nullptr) {
            return items;
        }
        const auto a = renderPosition(anchor);
        const auto b = renderPosition(position);
        if (!a.has_value() || !b.has_value()) {
            return items;
        }
        return selectVisibleMeshArea(renderer, target, scene, activeKind,
                                     {(*a)[0], (*a)[1], (*b)[0], (*b)[1]});
    }
#endif
};

ViewportMeshSelectionBridge::ViewportMeshSelectionBridge(ViewportWidget *viewport, QObject *parent)
    : QObject(parent), impl_(std::make_unique<Impl>()), viewport_(viewport)
{
#ifdef FEMCAE_GUI_HAS_VTK
    impl_->bind(viewport_);
    if (impl_->widget != nullptr) {
        impl_->widget->installEventFilter(this);
    }
#else
    Q_UNUSED(viewport_)
#endif
}

ViewportMeshSelectionBridge::~ViewportMeshSelectionBridge() = default;

bool ViewportMeshSelectionBridge::setScene(const femcae::meshing::SimulationMesh &mesh,
                                           const quint64 generation)
{
    MeshSelectionScene candidate;
    if (!candidate.set(mesh, generation) || viewport_ == nullptr) {
        clearScene();
        return false;
    }
#ifdef FEMCAE_GUI_HAS_VTK
    impl_->clearVisualState();
    impl_->scene = std::move(candidate);
    impl_->bind(viewport_);
    if (!impl_->resolveMeshSurfaceActor()) {
        impl_->scene.clear();
        return false;
    }
    impl_->nodeActor = impl_->buildNodeActor();
    if (impl_->nodeActor == nullptr || impl_->renderer == nullptr) {
        impl_->clearVisualState();
        impl_->scene.clear();
        return false;
    }
    impl_->renderer->AddActor(impl_->nodeActor);
    impl_->applyPalette();
    impl_->updateBaseVisibility();
    impl_->rebuildSelectionOverlay();
    impl_->rebuildPreselectionOverlay();
    impl_->render();
    return true;
#else
    Q_UNUSED(mesh)
    Q_UNUSED(generation)
    return false;
#endif
}

void ViewportMeshSelectionBridge::clearScene()
{
#ifdef FEMCAE_GUI_HAS_VTK
    impl_->clearVisualState();
    impl_->render();
#endif
    impl_->scene.clear();
}

void ViewportMeshSelectionBridge::setInputEnabled(const bool enabled) noexcept
{
    inputEnabled_ = enabled;
    if (!inputEnabled_) {
        input_.cancelPointerGesture();
    }
}

bool ViewportMeshSelectionBridge::inputEnabled() const noexcept
{
    return inputEnabled_;
}

void ViewportMeshSelectionBridge::setActiveKind(const SelectionKind kind)
{
    const SelectionKind resolved = isMeshKind(kind) ? kind : SelectionKind::Node;
    if (impl_->activeKind == resolved) {
        return;
    }
    impl_->activeKind = resolved;
#ifdef FEMCAE_GUI_HAS_VTK
    impl_->updateBaseVisibility();
    impl_->rebuildSelectionOverlay();
    impl_->rebuildPreselectionOverlay();
    impl_->render();
#endif
}

SelectionKind ViewportMeshSelectionBridge::activeKind() const noexcept
{
    return impl_->activeKind;
}

void ViewportMeshSelectionBridge::setSelection(const QVector<SelectionItem> &items)
{
    impl_->selection = items;
#ifdef FEMCAE_GUI_HAS_VTK
    impl_->rebuildSelectionOverlay();
    impl_->render();
#endif
}

void ViewportMeshSelectionBridge::setPreselection(std::optional<SelectionItem> item)
{
    impl_->preselection = std::move(item);
#ifdef FEMCAE_GUI_HAS_VTK
    impl_->rebuildPreselectionOverlay();
    impl_->render();
#endif
}

bool ViewportMeshSelectionBridge::eventFilter(QObject *watched, QEvent *event)
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (!inputEnabled_ || watched != impl_->widget || event == nullptr) {
        return QObject::eventFilter(watched, event);
    }

    // Mesh context'te selection ve camera navigation ayni Qt widget'i izler.
    // Plain Left/Right selection gesture'lari burada sahiplenilir ve consume
    // edilir; boylece ayni click/drag eski CAD bridge'e veya VTK legacy pick
    // yoluna ikinci kez dusmez. Alt+Left, Middle, wheel/native gesture ve camera
    // keyboard komutlari bu blok tarafindan sahiplenilmez ve navigation router'a
    // aynen gecer.
    bool selectionOwnedEvent = false;
    const bool gestureWasInProgress = input_.clickInProgress();
    std::optional<SelectionInputAction> action;

    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove: {
        auto *mouse = static_cast<QMouseEvent *>(event);
        if (event->type() == QEvent::MouseButtonPress) {
            const bool plainLeft = mouse->button() == Qt::LeftButton
                && !mouse->modifiers().testFlag(Qt::AltModifier);
            const bool secondary = mouse->button() == Qt::RightButton;
            selectionOwnedEvent = plainLeft || secondary;
        } else if (gestureWasInProgress) {
            selectionOwnedEvent = true;
        } else if (event->type() == QEvent::MouseMove && mouse->buttons() == Qt::NoButton) {
            selectionOwnedEvent = true;
        }

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
        selectionOwnedEvent = action.has_value();
        break;
    }
    case QEvent::KeyPress: {
        auto *key = static_cast<QKeyEvent *>(event);
        action = input_.routeKey(key->key(), key->modifiers());
        selectionOwnedEvent = action.has_value();
        break;
    }
    case QEvent::PaletteChange:
    case QEvent::ApplicationPaletteChange:
        impl_->applyPalette();
        impl_->render();
        break;
    default:
        break;
    }

    if (action.has_value()) {
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
                emit preselectionRequested(hit->kind, static_cast<qint64>(hit->meshEntityId));
            } else {
                emit preselectionClearRequested();
            }
            break;
        }
        case SelectionInputActionType::Commit: {
            const auto hit = impl_->pick(action->position);
            if (hit.has_value()) {
                emit selectionRequested(hit->kind, static_cast<qint64>(hit->meshEntityId), action->operation);
            } else {
                emit selectionClearRequested();
            }
            break;
        }
        case SelectionInputActionType::WindowCommit: {
            emit preselectionClearRequested();
            const QVector<SelectionItem> hits = impl_->pickWindow(action->anchor, action->position);
            if (hits.isEmpty()) {
                if (action->operation == SelectionOperation::Replace) {
                    emit selectionClearRequested();
                }
                break;
            }

            // Coordinator halen single-hit signal contract'ini kullanir. Replace
            // window semantigi ilk engineering entity ile Replace, kalanlarla Add
            // olarak uygulanir. SelectionManager'in Alpha.3.5 batch API'si sonraki
            // coordinator contract adiminda bu sequence'i tek signal/state
            // transition'a indirecektir.
            for (qsizetype i = 0; i < hits.size(); ++i) {
                SelectionOperation operation = action->operation;
                if (action->operation == SelectionOperation::Replace && i > 0) {
                    operation = SelectionOperation::Add;
                }
                emit selectionRequested(hits[i].kind,
                                        static_cast<qint64>(hits[i].meshEntityId),
                                        operation);
            }
            break;
        }
        case SelectionInputActionType::ContextMenu: {
            const auto hit = impl_->pick(action->position);
            if (hit.has_value()) {
                emit contextMenuRequested(hit->kind, static_cast<qint64>(hit->meshEntityId),
                                          impl_->widget->mapToGlobal(action->position.toPoint()));
            }
            break;
        }
        }
    }

    if (selectionOwnedEvent) {
        event->accept();
        return true;
    }
    return QObject::eventFilter(watched, event);
#else
    Q_UNUSED(watched)
    Q_UNUSED(event)
    return false;
#endif
}

} // namespace d26
