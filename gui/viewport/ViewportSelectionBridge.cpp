#include "ViewportSelectionBridge.h"

#include "RenderRoles.h"
#include "ViewportWidget.h"

#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPalette>
#include <QRect>
#include <QRubberBand>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

#ifdef FEMCAE_GUI_HAS_VTK
#include "GeometryHardwareSelector.h"

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

bool isCadTopologyKind(const SelectionKind kind) noexcept
{
    return kind == SelectionKind::Body || kind == SelectionKind::Face
        || kind == SelectionKind::Edge || kind == SelectionKind::Vertex;
}

femcae::geometry::GeometryEntityId bodyIdFor(const SelectionItem &item) noexcept
{
    return item.kind == SelectionKind::Body ? item.geometryEntityId : item.parentGeometryId;
}

} // namespace

class ViewportSelectionBridge::Impl
{
public:
    GeometryTopologyScene scene;
    QVector<SelectionItem> selection;
    std::optional<SelectionItem> preselection;
    SelectionKind activeKind{SelectionKind::Body};

#ifdef FEMCAE_GUI_HAS_VTK
    QVTKOpenGLNativeWidget *widget{nullptr};
    QRubberBand *windowBand{nullptr};
    vtkWeakPointer<vtkRenderer> renderer;
    vtkWeakPointer<vtkActor> surfaceActor;
    vtkSmartPointer<vtkCellPicker> picker;
    vtkSmartPointer<vtkActor> edgeActor;
    vtkSmartPointer<vtkActor> vertexActor;
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
        if (windowBand == nullptr) {
            windowBand = new QRubberBand(QRubberBand::Rectangle, widget);
            windowBand->hide();
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

    void showWindowPreview(const QPointF &anchor, const QPointF &position)
    {
        if (windowBand == nullptr || widget == nullptr) {
            return;
        }
        QRect area(anchor.toPoint(), position.toPoint());
        area = area.normalized().intersected(widget->rect());
        if (area.width() <= 0 || area.height() <= 0) {
            windowBand->hide();
            return;
        }
        windowBand->setGeometry(area);
        windowBand->show();
        windowBand->raise();
    }

    void hideWindowPreview()
    {
        if (windowBand != nullptr) {
            windowBand->hide();
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
        hideWindowPreview();
        removeActor(selectionActor);
        removeActor(preselectionActor);
        removeActor(edgeActor);
        removeActor(vertexActor);
        surfaceActor = nullptr;
    }

    [[nodiscard]] bool resolveSurfaceActor()
    {
        surfaceActor = nullptr;
        if (renderer == nullptr || scene.empty()) {
            return false;
        }

        const GeometrySelectionScene &surface = scene.surface();
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
            if (data->GetNumberOfPolys() == static_cast<vtkIdType>(surface.triangles().size())
                && data->GetNumberOfPoints() == static_cast<vtkIdType>(surface.points().size())) {
                surfaceActor = actor;
                break;
            }
        }
        return surfaceActor != nullptr;
    }

    vtkSmartPointer<vtkActor> buildCanonicalEdgeActor() const
    {
        if (scene.edgeLines().empty()) {
            return nullptr;
        }
        vtkNew<vtkPoints> points;
        points->SetNumberOfPoints(static_cast<vtkIdType>(scene.edgePoints().size()));
        for (std::size_t i = 0; i < scene.edgePoints().size(); ++i) {
            const auto &p = scene.edgePoints()[i];
            points->SetPoint(static_cast<vtkIdType>(i), p.x, p.y, p.z);
        }
        vtkNew<vtkCellArray> lines;
        for (const auto &line : scene.edgeLines()) {
            const vtkIdType ids[2] = {static_cast<vtkIdType>(line[0]), static_cast<vtkIdType>(line[1])};
            lines->InsertNextCell(2, ids);
        }
        vtkNew<vtkPolyData> data;
        data->SetPoints(points);
        data->SetLines(lines);
        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetInputData(data);
        mapper->ScalarVisibilityOff();

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->PickableOn();
        actor->GetProperty()->SetLineWidth(2.4);
        actor->GetProperty()->SetLighting(false);
        actor->GetProperty()->SetAmbient(1.0);
        actor->GetProperty()->SetDiffuse(0.0);
        return actor;
    }

    vtkSmartPointer<vtkActor> buildCanonicalVertexActor() const
    {
        if (scene.vertexPoints().empty()) {
            return nullptr;
        }
        vtkNew<vtkPoints> points;
        vtkNew<vtkCellArray> verts;
        points->SetNumberOfPoints(static_cast<vtkIdType>(scene.vertexPoints().size()));
        for (std::size_t i = 0; i < scene.vertexPoints().size(); ++i) {
            const auto &p = scene.vertexPoints()[i];
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
        actor->GetProperty()->SetPointSize(8.0);
        actor->GetProperty()->SetLighting(false);
        actor->GetProperty()->SetAmbient(1.0);
        actor->GetProperty()->SetDiffuse(0.0);
        return actor;
    }

    void applyPalette()
    {
        const ViewportPalette palette = ViewportPalette::forAppearance(systemPrefersDark());
        const Rgb topology = palette.color(RenderRole::GeometryEdge);
        if (edgeActor != nullptr) {
            edgeActor->GetProperty()->SetColor(topology.r, topology.g, topology.b);
        }
        if (vertexActor != nullptr) {
            vertexActor->GetProperty()->SetColor(topology.r, topology.g, topology.b);
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

    void updateBaseTopologyVisibility()
    {
        if (edgeActor != nullptr) {
            edgeActor->SetVisibility(activeKind == SelectionKind::Edge ? 1 : 0);
        }
        if (vertexActor != nullptr) {
            vertexActor->SetVisibility(activeKind == SelectionKind::Vertex ? 1 : 0);
        }
    }

    vtkSmartPointer<vtkActor> buildSurfaceOverlay(const QVector<SelectionItem> &items,
                                                  const RenderRole role,
                                                  const double opacity,
                                                  const double lineWidth,
                                                  const double polygonOffset) const
    {
        const GeometrySelectionScene &surface = scene.surface();
        const auto cells = surface.cellIndicesForSelection(items);
        if (cells.empty()) {
            return nullptr;
        }

        vtkNew<vtkPoints> points;
        points->SetNumberOfPoints(static_cast<vtkIdType>(surface.points().size()));
        for (std::size_t i = 0; i < surface.points().size(); ++i) {
            const auto &p = surface.points()[i];
            points->SetPoint(static_cast<vtkIdType>(i), p.x, p.y, p.z);
        }
        vtkNew<vtkCellArray> polys;
        for (const std::size_t cell : cells) {
            const auto &triangle = surface.triangles()[cell];
            const vtkIdType ids[3] = {static_cast<vtkIdType>(triangle[0]),
                                      static_cast<vtkIdType>(triangle[1]),
                                      static_cast<vtkIdType>(triangle[2])};
            polys->InsertNextCell(3, ids);
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
        const Rgb color = ViewportPalette::forAppearance(systemPrefersDark()).color(role);
        actor->GetProperty()->SetColor(color.r, color.g, color.b);
        actor->GetProperty()->SetEdgeColor(color.r, color.g, color.b);
        return actor;
    }

    vtkSmartPointer<vtkActor> buildEdgeOverlay(const QVector<SelectionItem> &items,
                                               const RenderRole role,
                                               const double lineWidth) const
    {
        const auto selected = scene.lineIndicesForSelection(items);
        if (selected.empty()) {
            return nullptr;
        }
        vtkNew<vtkPoints> points;
        points->SetNumberOfPoints(static_cast<vtkIdType>(scene.edgePoints().size()));
        for (std::size_t i = 0; i < scene.edgePoints().size(); ++i) {
            const auto &p = scene.edgePoints()[i];
            points->SetPoint(static_cast<vtkIdType>(i), p.x, p.y, p.z);
        }
        vtkNew<vtkCellArray> lines;
        for (const std::size_t index : selected) {
            const auto &line = scene.edgeLines()[index];
            const vtkIdType ids[2] = {static_cast<vtkIdType>(line[0]), static_cast<vtkIdType>(line[1])};
            lines->InsertNextCell(2, ids);
        }
        vtkNew<vtkPolyData> data;
        data->SetPoints(points);
        data->SetLines(lines);
        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetInputData(data);
        mapper->ScalarVisibilityOff();
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->PickableOff();
        actor->GetProperty()->SetLineWidth(lineWidth);
        actor->GetProperty()->SetLighting(false);
        const Rgb color = ViewportPalette::forAppearance(systemPrefersDark()).color(role);
        actor->GetProperty()->SetColor(color.r, color.g, color.b);
        return actor;
    }

    vtkSmartPointer<vtkActor> buildVertexOverlay(const QVector<SelectionItem> &items,
                                                 const RenderRole role,
                                                 const double pointSize) const
    {
        const auto selected = scene.pointIndicesForSelection(items);
        if (selected.empty()) {
            return nullptr;
        }
        vtkNew<vtkPoints> points;
        vtkNew<vtkCellArray> verts;
        for (const std::size_t index : selected) {
            const auto &p = scene.vertexPoints()[index];
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

    vtkSmartPointer<vtkActor> buildOverlay(const QVector<SelectionItem> &items,
                                           const RenderRole role,
                                           const bool preselectionOverlay) const
    {
        switch (activeKind) {
        case SelectionKind::Body:
        case SelectionKind::Face:
            return buildSurfaceOverlay(items, role,
                                       preselectionOverlay ? 0.24 : 0.42,
                                       preselectionOverlay ? 1.5 : 2.2,
                                       preselectionOverlay ? -6.0 : -4.0);
        case SelectionKind::Edge:
            return buildEdgeOverlay(items, role, preselectionOverlay ? 3.2 : 4.2);
        case SelectionKind::Vertex:
            return buildVertexOverlay(items, role, preselectionOverlay ? 11.0 : 13.0);
        default:
            return nullptr;
        }
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
        preselectionActor = buildOverlay(QVector<SelectionItem>{*preselection},
                                         RenderRole::Preselection, true);
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
        switch (activeKind) {
        case SelectionKind::Body:
        case SelectionKind::Face:
            return surfaceActor;
        case SelectionKind::Edge:
            return edgeActor;
        case SelectionKind::Vertex:
            return vertexActor;
        default:
            return nullptr;
        }
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
        switch (activeKind) {
        case SelectionKind::Body:
        case SelectionKind::Face:
            picker->SetTolerance(0.005);
            break;
        case SelectionKind::Edge:
            picker->SetTolerance(0.010);
            break;
        case SelectionKind::Vertex:
            picker->SetTolerance(0.014);
            break;
        default:
            return std::nullopt;
        }

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
        switch (activeKind) {
        case SelectionKind::Body:
        case SelectionKind::Face:
            return scene.selectionItemForSurfaceCell(cell, activeKind);
        case SelectionKind::Edge:
            return scene.selectionItemForEdgeCell(cell);
        case SelectionKind::Vertex:
            return scene.selectionItemForVertexCell(cell);
        default:
            return std::nullopt;
        }
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
        return selectVisibleGeometryArea(renderer, target, scene, activeKind,
                                         {(*a)[0], (*a)[1], (*b)[0], (*b)[1]});
    }
#endif
};

ViewportSelectionBridge::ViewportSelectionBridge(ViewportWidget *viewport, QObject *parent)
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

ViewportSelectionBridge::~ViewportSelectionBridge() = default;

bool ViewportSelectionBridge::setScene(
    const QVector<femcae::geometry::TopologyTessellation> &surfaces,
    const QVector<femcae::geometry::EdgeDisplayTessellation> &edges,
    const QVector<femcae::geometry::VertexDisplayPoints> &vertices)
{
    if (surfaces.isEmpty() || surfaces.size() != edges.size() || surfaces.size() != vertices.size()) {
        clearScene();
        return false;
    }

    GeometryTopologyScene candidate;
    for (qsizetype i = 0; i < surfaces.size(); ++i) {
        if (!candidate.append(surfaces[i], edges[i], vertices[i])) {
            clearScene();
            return false;
        }
    }
    if (!candidate.complete() || viewport_ == nullptr) {
        clearScene();
        return false;
    }

#ifdef FEMCAE_GUI_HAS_VTK
    impl_->clearVisualState();
    viewport_->showGeometry(surfaces);
    impl_->scene = std::move(candidate);
    impl_->bind(viewport_);
    if (!impl_->resolveSurfaceActor()) {
        impl_->scene.clear();
        return false;
    }

    impl_->edgeActor = impl_->buildCanonicalEdgeActor();
    impl_->vertexActor = impl_->buildCanonicalVertexActor();
    if (impl_->edgeActor == nullptr || impl_->vertexActor == nullptr || impl_->renderer == nullptr) {
        impl_->clearVisualState();
        impl_->scene.clear();
        return false;
    }
    impl_->renderer->AddActor(impl_->edgeActor);
    impl_->renderer->AddActor(impl_->vertexActor);
    impl_->applyPalette();
    impl_->updateBaseTopologyVisibility();
    impl_->rebuildSelectionOverlay();
    impl_->rebuildPreselectionOverlay();
    impl_->render();
    return true;
#else
    Q_UNUSED(surfaces)
    Q_UNUSED(edges)
    Q_UNUSED(vertices)
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
    return impl_->scene.hasFaceProvenance();
}

bool ViewportSelectionBridge::hasEdgeProvenance() const noexcept
{
    return impl_->scene.hasEdgeProvenance();
}

bool ViewportSelectionBridge::hasVertexProvenance() const noexcept
{
    return impl_->scene.hasVertexProvenance();
}

void ViewportSelectionBridge::setActiveKind(const SelectionKind kind)
{
    const SelectionKind resolved = isCadTopologyKind(kind) ? kind : SelectionKind::Body;
    if (impl_->activeKind == resolved) {
        return;
    }
    impl_->activeKind = resolved;
#ifdef FEMCAE_GUI_HAS_VTK
    impl_->hideWindowPreview();
    impl_->updateBaseTopologyVisibility();
    impl_->rebuildSelectionOverlay();
    impl_->rebuildPreselectionOverlay();
    impl_->render();
#endif
}

SelectionKind ViewportSelectionBridge::activeKind() const noexcept
{
    return impl_->activeKind;
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

std::optional<SelectionItem> ViewportSelectionBridge::pickAtGlobalPosition(const QPoint &position)
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (impl_->widget != nullptr) {
        return impl_->pick(impl_->widget->mapFromGlobal(position));
    }
#else
    Q_UNUSED(position)
#endif
    return std::nullopt;
}

std::optional<std::array<double, 6>> ViewportSelectionBridge::selectionDisplayBounds() const
{
    // This is a camera operation, so display points with CAD provenance are
    // appropriate. These bounds are never used as engineering Face area/normal.
    std::optional<std::array<double, 6>> bounds;
    const auto add = [&bounds](const femcae::geometry::Vec3 &p) {
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) return;
        if (!bounds) bounds = std::array<double, 6>{p.x, p.x, p.y, p.y, p.z, p.z};
        const double xyz[] = {p.x, p.y, p.z};
        for (int i = 0; i < 3; ++i) {
            (*bounds)[2*i] = std::min((*bounds)[2*i], xyz[i]);
            (*bounds)[2*i+1] = std::max((*bounds)[2*i+1], xyz[i]);
        }
    };
    const auto &surface = impl_->scene.surface();
    for (auto cell : surface.cellIndicesForSelection(impl_->selection))
        for (auto point : surface.triangles()[cell]) add(surface.points()[point]);
    for (auto line : impl_->scene.lineIndicesForSelection(impl_->selection))
        for (auto point : impl_->scene.edgeLines()[line]) add(impl_->scene.edgePoints()[point]);
    for (auto point : impl_->scene.pointIndicesForSelection(impl_->selection))
        add(impl_->scene.vertexPoints()[point]);
    return bounds;
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
        impl_->applyPalette();
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
        impl_->hideWindowPreview();
        emit selectionClearRequested();
        break;
    case SelectionInputActionType::ClearPreselection:
        impl_->hideWindowPreview();
        emit preselectionClearRequested();
        break;
    case SelectionInputActionType::Hover: {
        const auto hit = impl_->pick(action->position);
        if (hit.has_value()) {
            emit preselectionRequested(hit->kind,
                                       static_cast<quint64>(bodyIdFor(*hit)),
                                       static_cast<quint64>(hit->geometryEntityId));
        } else {
            emit preselectionClearRequested();
        }
        break;
    }
    case SelectionInputActionType::Commit: {
        impl_->hideWindowPreview();
        const auto hit = impl_->pick(action->position);
        if (hit.has_value()) {
            emit selectionRequested(hit->kind,
                                    static_cast<quint64>(bodyIdFor(*hit)),
                                    static_cast<quint64>(hit->geometryEntityId),
                                    action->operation);
        } else {
            emit selectionClearRequested();
        }
        break;
    }
    case SelectionInputActionType::WindowPreview:
        impl_->showWindowPreview(action->anchor, action->position);
        break;
    case SelectionInputActionType::WindowCommit: {
        impl_->hideWindowPreview();
        emit preselectionClearRequested();
        const QVector<SelectionItem> hits = impl_->pickWindow(action->anchor, action->position);
        if (hits.isEmpty()) {
            if (action->operation == SelectionOperation::Replace) {
                emit selectionClearRequested();
            }
            break;
        }
        for (qsizetype i = 0; i < hits.size(); ++i) {
            SelectionOperation operation = action->operation;
            if (action->operation == SelectionOperation::Replace && i > 0) {
                operation = SelectionOperation::Add;
            }
            emit selectionRequested(hits[i].kind,
                                    static_cast<quint64>(bodyIdFor(hits[i])),
                                    static_cast<quint64>(hits[i].geometryEntityId),
                                    operation);
        }
        break;
    }
    case SelectionInputActionType::ContextMenu:
        // Qt QContextMenuEvent owns menu creation; release must never open another.
        break;
    }

    return QObject::eventFilter(watched, event);
#else
    Q_UNUSED(watched)
    Q_UNUSED(event)
    return false;
#endif
}

} // namespace d26
