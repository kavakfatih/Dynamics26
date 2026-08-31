#include "viewport/GeometryHardwareSelector.h"

#include <QCoreApplication>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>

#include <cstddef>
#include <iostream>
#include <set>
#include <string>

namespace {

int failures = 0;

void check(const bool condition, const std::string &message)
{
    std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
    failures += condition ? 0 : 1;
}

femcae::geometry::TopologyTessellation makeBody(const quint64 bodyId,
                                                 const quint64 faceId,
                                                 const quint64 revision,
                                                 const double x0,
                                                 const double x1)
{
    femcae::geometry::TopologyTessellation body;
    body.display.sourceGeometryId = bodyId;
    body.display.sourceRevision = revision;
    body.display.points = {{x0, -1.0, 0.0}, {x1, -1.0, 0.0},
                           {x1,  1.0, 0.0}, {x0,  1.0, 0.0}};
    body.display.triangles = {{{0, 1, 2}}, {{0, 2, 3}}};
    body.triangleFaceIds = {faceId, faceId};
    return body;
}

femcae::geometry::EdgeDisplayTessellation makeEdges(const quint64 bodyId,
                                                     const quint64 revision,
                                                     const quint64 edgeA,
                                                     const quint64 edgeB,
                                                     const double x0,
                                                     const double x1)
{
    femcae::geometry::EdgeDisplayTessellation edges;
    edges.sourceGeometryId = bodyId;
    edges.sourceRevision = revision;
    edges.points = {{x0, -1.0, 0.02}, {x1, -1.0, 0.02},
                    {x0,  1.0, 0.02}, {x1,  1.0, 0.02}};
    edges.lines = {{{0, 1}}, {{2, 3}}};
    edges.lineEdgeIds = {edgeA, edgeB};
    return edges;
}

femcae::geometry::VertexDisplayPoints makeVertices(const quint64 bodyId,
                                                    const quint64 revision,
                                                    const quint64 vertexA,
                                                    const quint64 vertexB,
                                                    const double x0,
                                                    const double x1)
{
    femcae::geometry::VertexDisplayPoints vertices;
    vertices.sourceGeometryId = bodyId;
    vertices.sourceRevision = revision;
    vertices.points = {{x0, 0.0, 0.04}, {x1, 0.0, 0.04}};
    vertices.pointVertexIds = {vertexA, vertexB};
    return vertices;
}

vtkSmartPointer<vtkActor> makeSurfaceActor(const d26::GeometryTopologyScene &scene)
{
    const auto &surface = scene.surface();
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(static_cast<vtkIdType>(surface.points().size()));
    for (std::size_t i = 0; i < surface.points().size(); ++i) {
        const auto &p = surface.points()[i];
        points->SetPoint(static_cast<vtkIdType>(i), p.x, p.y, p.z);
    }
    vtkNew<vtkCellArray> cells;
    for (const auto &triangle : surface.triangles()) {
        const vtkIdType ids[3] = {static_cast<vtkIdType>(triangle[0]),
                                  static_cast<vtkIdType>(triangle[1]),
                                  static_cast<vtkIdType>(triangle[2])};
        cells->InsertNextCell(3, ids);
    }
    vtkNew<vtkPolyData> data;
    data->SetPoints(points);
    data->SetPolys(cells);
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(data);
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->PickableOn();
    return actor;
}

vtkSmartPointer<vtkActor> makeEdgeActor(const d26::GeometryTopologyScene &scene)
{
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
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetLineWidth(5.0);
    actor->PickableOn();
    return actor;
}

vtkSmartPointer<vtkActor> makeVertexActor(const d26::GeometryTopologyScene &scene)
{
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
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetPointSize(12.0);
    actor->PickableOn();
    return actor;
}

void hardwareWindowTests()
{
    constexpr quint64 revision = 91;
    d26::GeometryTopologyScene scene;
    check(scene.append(makeBody(1001, 1101, revision, -3.0, -1.0),
                       makeEdges(1001, revision, 1201, 1202, -3.0, -1.0),
                       makeVertices(1001, revision, 1301, 1302, -2.8, -1.2))
              && scene.append(makeBody(2001, 2101, revision, 1.0, 3.0),
                              makeEdges(2001, revision, 2201, 2202, 1.0, 3.0),
                              makeVertices(2001, revision, 2301, 2302, 1.2, 2.8))
              && scene.complete(),
          "two CAD Bodies build one complete Body/Face/Edge/Vertex provenance scene");
    if (!scene.complete()) {
        return;
    }

    vtkNew<vtkRenderer> renderer;
    vtkNew<vtkRenderWindow> window;
    window->SetOffScreenRendering(1);
    window->SetSize(640, 480);
    window->AddRenderer(renderer);
    renderer->GetActiveCamera()->SetPosition(0.0, 0.0, 10.0);
    renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.0, 0.0);
    renderer->GetActiveCamera()->SetViewUp(0.0, 1.0, 0.0);
    renderer->GetActiveCamera()->ParallelProjectionOn();

    const std::array<unsigned int, 4> fullArea{0U, 0U, 639U, 479U};

    const auto surfaceActor = makeSurfaceActor(scene);
    renderer->AddActor(surfaceActor);
    renderer->ResetCamera();
    renderer->ResetCameraClippingRange();
    window->Render();

    const auto bodies = d26::selectVisibleGeometryArea(renderer, surfaceActor, scene,
                                                        d26::SelectionKind::Body, fullArea);
    std::set<quint64> bodyIds;
    for (const auto &item : bodies) {
        bodyIds.insert(static_cast<quint64>(item.geometryEntityId));
    }
    check(bodies.size() == 2 && bodyIds == std::set<quint64>{1001, 2001},
          "surface window de-duplicates display triangles to two canonical CAD Body identities");

    const auto faces = d26::selectVisibleGeometryArea(renderer, surfaceActor, scene,
                                                       d26::SelectionKind::Face, fullArea);
    std::set<quint64> faceIds;
    bool faceParentsValid = true;
    for (const auto &item : faces) {
        faceIds.insert(static_cast<quint64>(item.geometryEntityId));
        faceParentsValid = faceParentsValid
            && (item.parentGeometryId == 1001 || item.parentGeometryId == 2001)
            && item.sourceRevision == revision;
    }
    check(faces.size() == 2 && faceIds == std::set<quint64>{1101, 2101} && faceParentsValid,
          "surface window resolves Face identity plus parent Body provenance");
    check(!faces.isEmpty() && static_cast<quint64>(faces.front().geometryEntityId) > 100,
          "VTK surface cell index is never exposed as GeometryEntityId");

    renderer->RemoveActor(surfaceActor);
    const auto edgeActor = makeEdgeActor(scene);
    renderer->AddActor(edgeActor);
    renderer->ResetCamera();
    renderer->ResetCameraClippingRange();
    window->Render();

    const auto edges = d26::selectVisibleGeometryArea(renderer, edgeActor, scene,
                                                       d26::SelectionKind::Edge, fullArea);
    std::set<quint64> edgeIds;
    for (const auto &item : edges) {
        edgeIds.insert(static_cast<quint64>(item.geometryEntityId));
    }
    check(edges.size() == 4 && edgeIds == std::set<quint64>{1201, 1202, 2201, 2202},
          "line-cell hardware window resolves canonical CAD Edge identities");

    renderer->RemoveActor(edgeActor);
    const auto vertexActor = makeVertexActor(scene);
    renderer->AddActor(vertexActor);
    renderer->ResetCamera();
    renderer->ResetCameraClippingRange();
    window->Render();

    const auto vertices = d26::selectVisibleGeometryArea(renderer, vertexActor, scene,
                                                          d26::SelectionKind::Vertex, fullArea);
    std::set<quint64> vertexIds;
    for (const auto &item : vertices) {
        vertexIds.insert(static_cast<quint64>(item.geometryEntityId));
    }
    check(vertices.size() == 4 && vertexIds == std::set<quint64>{1301, 1302, 2301, 2302},
          "point hardware window resolves canonical CAD Vertex identities");

    const auto empty = d26::selectVisibleGeometryArea(renderer, vertexActor, scene,
                                                       d26::SelectionKind::Vertex,
                                                       {0U, 0U, 1U, 1U});
    check(empty.isEmpty(), "background-only CAD window returns no engineering identity");
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    hardwareWindowTests();
    std::cout << (failures == 0 ? "CAD hardware window selection PASS\n"
                                : "CAD hardware window selection FAIL\n");
    return failures == 0 ? 0 : 1;
}
