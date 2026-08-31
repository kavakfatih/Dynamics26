#include "viewport/MeshSelectionScene.h"

#include <QCoreApplication>

#include <femcae/meshing/StructuredHexMesher.h>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellPicker.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <optional>
#include <string>

namespace {

int failures = 0;

void check(const bool condition, const std::string &message)
{
    std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
    failures += condition ? 0 : 1;
}

femcae::meshing::SimulationMesh makeMesh()
{
    femcae::meshing::AxisAlignedBox box;
    box.min = {0.0, 0.0, 0.0};
    box.max = {1.0, 1.0, 1.0};

    femcae::meshing::BoxBoundaryGeometry geometry;
    geometry.body = 100;
    geometry.xMin = 101;
    geometry.xMax = 102;
    geometry.yMin = 103;
    geometry.yMax = 104;
    geometry.zMin = 105;
    geometry.zMax = 106;

    femcae::meshing::StructuredHexMesherOptions options;
    options.nx = 2;
    options.ny = 1;
    options.nz = 1;
    options.firstNodeId = 10;
    options.firstElementId = 100;
    options.firstFacetId = 1000;

    femcae::meshing::StructuredHexMesher mesher;
    return mesher.meshBox(box, geometry, 5, options);
}

vtkSmartPointer<vtkActor> makeBoundaryActor(const d26::MeshSelectionScene &scene)
{
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(static_cast<vtkIdType>(scene.points().size()));
    for (std::size_t i = 0; i < scene.points().size(); ++i) {
        const auto &p = scene.points()[i];
        points->SetPoint(static_cast<vtkIdType>(i), p.x, p.y, p.z);
    }

    vtkNew<vtkCellArray> cells;
    for (const auto &quad : scene.boundaryQuads()) {
        const vtkIdType ids[4] = {static_cast<vtkIdType>(quad[0]), static_cast<vtkIdType>(quad[1]),
                                  static_cast<vtkIdType>(quad[2]), static_cast<vtkIdType>(quad[3])};
        cells->InsertNextCell(4, ids);
    }

    vtkNew<vtkPolyData> data;
    data->SetPoints(points);
    data->SetPolys(cells);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(data);
    mapper->ScalarVisibilityOff();

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->PickableOn();
    return actor;
}

vtkSmartPointer<vtkActor> makeNodeActor(const d26::MeshSelectionScene &scene)
{
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
    actor->GetProperty()->SetPointSize(12.0);
    actor->PickableOn();
    return actor;
}

std::array<double, 3> worldToDisplay(vtkRenderer *renderer,
                                     const std::array<double, 3> &world)
{
    renderer->SetWorldPoint(world[0], world[1], world[2], 1.0);
    renderer->WorldToDisplay();
    double display[3] = {0.0, 0.0, 0.0};
    renderer->GetDisplayPoint(display);
    return {display[0], display[1], display[2]};
}

std::array<double, 3> facetCentre(const femcae::meshing::SimulationMesh &mesh,
                                  const femcae::meshing::MeshFacet &facet)
{
    std::array<double, 3> centre{0.0, 0.0, 0.0};
    for (const auto nodeId : facet.nodeIds) {
        const auto *node = mesh.findNode(nodeId);
        if (node == nullptr) {
            continue;
        }
        centre[0] += node->x.x;
        centre[1] += node->x.y;
        centre[2] += node->x.z;
    }
    for (double &value : centre) {
        value *= 0.25;
    }
    return centre;
}

std::optional<std::size_t> firstFacetOnGeometry(const femcae::meshing::SimulationMesh &mesh,
                                                const femcae::geometry::GeometryEntityId geometryId)
{
    for (std::size_t i = 0; i < mesh.boundaryFacets.size(); ++i) {
        if (mesh.boundaryFacets[i].sourceGeometryId == geometryId) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> visibleNodeIndex(const d26::MeshSelectionScene &scene,
                                            const femcae::meshing::MeshEntityId id)
{
    for (std::size_t i = 0; i < scene.visibleNodeIds().size(); ++i) {
        if (scene.visibleNodeIds()[i] == id) {
            return i;
        }
    }
    return std::nullopt;
}

void pickerTests()
{
    constexpr quint64 generation = 12;
    const auto mesh = makeMesh();
    d26::MeshSelectionScene scene;
    check(scene.set(mesh, generation) && scene.complete(),
          "generated HEX8 mesh builds complete VTK picker provenance scene");
    if (!scene.complete()) {
        return;
    }

    vtkNew<vtkRenderer> renderer;
    vtkNew<vtkRenderWindow> window;
    window->SetOffScreenRendering(1);
    window->SetSize(640, 480);
    window->AddRenderer(renderer);

    const auto surfaceActor = makeBoundaryActor(scene);
    renderer->AddActor(surfaceActor);
    renderer->GetActiveCamera()->SetPosition(0.5, 0.5, 4.0);
    renderer->GetActiveCamera()->SetFocalPoint(0.5, 0.5, 0.5);
    renderer->GetActiveCamera()->SetViewUp(0.0, 1.0, 0.0);
    renderer->GetActiveCamera()->ParallelProjectionOn();
    renderer->ResetCameraClippingRange();
    window->Render();

    const auto topFacetIndex = firstFacetOnGeometry(mesh, 106);
    check(topFacetIndex.has_value(),
          "structured mesh exposes a canonical Z-Max boundary Facet for picker test");
    if (!topFacetIndex.has_value()) {
        return;
    }

    const auto topCentre = facetCentre(mesh, mesh.boundaryFacets[*topFacetIndex]);
    const auto topDisplay = worldToDisplay(renderer, topCentre);

    vtkNew<vtkCellPicker> surfacePicker;
    surfacePicker->SetTolerance(0.0005);
    surfacePicker->InitializePickList();
    surfacePicker->AddPickList(surfaceActor);
    surfacePicker->PickFromListOn();
    const bool pickedSurface = surfacePicker->Pick(topDisplay[0], topDisplay[1], 0.0, renderer) != 0;
    const vtkIdType pickedCell = surfacePicker->GetCellId();
    check(pickedSurface && pickedCell >= 0,
          "vtkCellPicker resolves a visible HEX8 boundary display cell");
    if (pickedCell < 0) {
        return;
    }

    const std::size_t cell = static_cast<std::size_t>(pickedCell);
    const auto facet = scene.selectionItemForBoundaryCell(cell, d26::SelectionKind::Facet);
    const auto element = scene.selectionItemForBoundaryCell(cell, d26::SelectionKind::Element);
    check(facet.has_value() && facet->domain == d26::SelectionDomain::Mesh
              && facet->kind == d26::SelectionKind::Facet
              && facet->meshEntityId == mesh.boundaryFacets[cell].id
              && facet->sourceRevision == generation,
          "picked VTK boundary cell resolves through provenance to FEM Facet MeshEntityId");
    check(element.has_value() && element->domain == d26::SelectionDomain::Mesh
              && element->kind == d26::SelectionKind::Element
              && element->meshEntityId == mesh.boundaryFacets[cell].ownerElementId
              && element->sourceRevision == generation,
          "same VTK boundary cell resolves to owner FEM Element MeshEntityId in Element mode");
    check(facet.has_value()
              && static_cast<femcae::meshing::MeshEntityId>(pickedCell) != facet->meshEntityId,
          "VTK display cell index is never treated as FEM Facet identity");

    if (element.has_value()) {
        const auto exposed = scene.boundaryCellsForSelection(QVector<d26::SelectionItem>{*element});
        check(exposed.size() > 1,
              "selected boundary Element overlay expands to all of its visible boundary facets");
    }

    renderer->RemoveActor(surfaceActor);
    const auto nodeActor = makeNodeActor(scene);
    renderer->AddActor(nodeActor);
    window->Render();

    // Isometric camera prevents front/back nodes with equal x-y coordinates
    // from projecting to the same display location.
    renderer->GetActiveCamera()->SetPosition(3.0, 3.0, 3.0);
    renderer->GetActiveCamera()->SetFocalPoint(0.5, 0.5, 0.5);
    renderer->GetActiveCamera()->SetViewUp(0.0, 0.0, 1.0);
    renderer->GetActiveCamera()->ParallelProjectionOn();
    renderer->ResetCamera();
    renderer->ResetCameraClippingRange();
    window->Render();

    const femcae::meshing::MeshNode *targetNode = nullptr;
    for (const auto &node : mesh.nodes) {
        if (std::abs(node.x.x - 1.0) < 1.0e-12
            && std::abs(node.x.y - 1.0) < 1.0e-12
            && std::abs(node.x.z - 1.0) < 1.0e-12) {
            targetNode = &node;
            break;
        }
    }
    check(targetNode != nullptr,
          "structured mesh exposes a visible corner Node for picker test");
    if (targetNode == nullptr) {
        return;
    }

    const auto expectedNodeIndex = visibleNodeIndex(scene, targetNode->id);
    check(expectedNodeIndex.has_value(),
          "corner Node is part of visible-node provenance table");
    if (!expectedNodeIndex.has_value()) {
        return;
    }

    const auto nodeDisplay = worldToDisplay(renderer, {targetNode->x.x, targetNode->x.y, targetNode->x.z});
    vtkNew<vtkCellPicker> nodePicker;
    nodePicker->SetTolerance(0.014);
    nodePicker->InitializePickList();
    nodePicker->AddPickList(nodeActor);
    nodePicker->PickFromListOn();
    const bool pickedNode = nodePicker->Pick(nodeDisplay[0], nodeDisplay[1], 0.0, renderer) != 0;
    const vtkIdType pickedNodeCell = nodePicker->GetCellId();
    check(pickedNode && pickedNodeCell >= 0,
          "vtkCellPicker resolves a visible FEM Node display vertex");
    if (pickedNodeCell >= 0) {
        const auto node = scene.selectionItemForVisibleNode(static_cast<std::size_t>(pickedNodeCell));
        check(node.has_value() && node->kind == d26::SelectionKind::Node
                  && node->meshEntityId == targetNode->id
                  && node->sourceRevision == generation,
              "picked VTK vertex resolves through provenance to FEM Node MeshEntityId");
        check(static_cast<std::size_t>(pickedNodeCell) == *expectedNodeIndex,
              "VTK vertex-cell order matches the explicit visible-node provenance table");
    }

    if (facet.has_value()) {
        auto stale = *facet;
        stale.sourceRevision = generation + 1;
        check(scene.boundaryCellsForSelection(QVector<d26::SelectionItem>{stale}).empty(),
              "stale mesh generation cannot recreate a VTK FEM selection overlay");
    }
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    pickerTests();
    std::cout << (failures == 0 ? "FEM VTK picker provenance PASS\n"
                                : "FEM VTK picker provenance FAIL\n");
    return failures == 0 ? 0 : 1;
}
