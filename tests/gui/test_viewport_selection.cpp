#include "viewport/GeometrySelectionScene.h"
#include "viewport/RenderRoles.h"

#include <QCoreApplication>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellPicker.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

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
    // Tek CAD Face iki display triangle üretir. Böylece overlay testi display
    // triangle sayısını CAD entity sayısıyla karıştıran bir regresyonu yakalar.
    body.triangleFaceIds = {faceId, faceId};
    return body;
}

vtkSmartPointer<vtkActor> makeSurfaceActor(const d26::GeometrySelectionScene &scene)
{
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(static_cast<vtkIdType>(scene.points().size()));
    for (std::size_t i = 0; i < scene.points().size(); ++i) {
        const auto &p = scene.points()[i];
        points->SetPoint(static_cast<vtkIdType>(i), p.x, p.y, p.z);
    }

    vtkNew<vtkCellArray> cells;
    for (const auto &triangle : scene.triangles()) {
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

std::array<double, 3> worldToDisplay(vtkRenderer *renderer,
                                     const std::array<double, 3> &world)
{
    renderer->SetWorldPoint(world[0], world[1], world[2], 1.0);
    renderer->WorldToDisplay();
    double display[3] = {0.0, 0.0, 0.0};
    renderer->GetDisplayPoint(display);
    return {display[0], display[1], display[2]};
}

std::optional<d26::GeometrySceneCellProvenance>
pickWorld(vtkCellPicker *picker,
          vtkRenderer *renderer,
          const d26::GeometrySelectionScene &scene,
          const std::array<double, 3> &world,
          vtkIdType *pickedCell = nullptr)
{
    const auto display = worldToDisplay(renderer, world);
    if (picker->Pick(display[0], display[1], 0.0, renderer) == 0) {
        return std::nullopt;
    }
    const vtkIdType cellId = picker->GetCellId();
    if (cellId < 0) {
        return std::nullopt;
    }
    if (pickedCell != nullptr) {
        *pickedCell = cellId;
    }
    return scene.provenanceForCell(static_cast<std::size_t>(cellId));
}

double linearized(const double component)
{
    return component <= 0.04045 ? component / 12.92
                                : std::pow((component + 0.055) / 1.055, 2.4);
}

double luminance(const d26::Rgb &rgb)
{
    return 0.2126 * linearized(rgb.r)
         + 0.7152 * linearized(rgb.g)
         + 0.0722 * linearized(rgb.b);
}

double contrastRatio(const d26::Rgb &a, const d26::Rgb &b)
{
    const double la = luminance(a);
    const double lb = luminance(b);
    const double light = std::max(la, lb);
    const double dark = std::min(la, lb);
    return (light + 0.05) / (dark + 0.05);
}

bool different(const d26::Rgb &a, const d26::Rgb &b)
{
    return std::abs(a.r - b.r) > 1.0e-6
        || std::abs(a.g - b.g) > 1.0e-6
        || std::abs(a.b - b.b) > 1.0e-6;
}

void pickerAndProvenanceTests()
{
    constexpr quint64 revision = 77;
    constexpr quint64 body1Id = 1001;
    constexpr quint64 face1Id = 1101;
    constexpr quint64 body2Id = 2001;
    constexpr quint64 face2Id = 2101;

    d26::GeometrySelectionScene scene;
    check(scene.append(makeBody(body1Id, face1Id, revision, -3.0, -1.0))
              && scene.append(makeBody(body2Id, face2Id, revision, 1.0, 3.0)),
          "two topology-aware CAD Bodies build one pickable display scene");
    check(scene.triangles().size() == 4 && scene.hasFaceProvenance(),
          "display scene has four triangles with complete Face provenance");

    vtkNew<vtkRenderer> renderer;
    vtkNew<vtkRenderWindow> window;
    window->SetOffScreenRendering(1);
    window->SetSize(640, 480);
    window->AddRenderer(renderer);

    const auto actor = makeSurfaceActor(scene);
    renderer->AddActor(actor);
    renderer->ResetCamera();
    renderer->GetActiveCamera()->ParallelProjectionOn();
    renderer->ResetCameraClippingRange();
    window->Render();

    vtkNew<vtkCellPicker> picker;
    picker->SetTolerance(0.0005);
    picker->InitializePickList();
    picker->AddPickList(actor);
    picker->PickFromListOn();

    vtkIdType cell1 = -1;
    const auto hit1 = pickWorld(picker, renderer, scene, {-1.6666666667, -0.3333333333, 0.0}, &cell1);
    check(hit1.has_value() && hit1->bodyId == body1Id && hit1->faceId == face1Id,
          "vtkCellPicker cell resolves through provenance to Body 1 / Face 1");

    vtkIdType cell2 = -1;
    const auto hit2 = pickWorld(picker, renderer, scene, {2.3333333333, -0.3333333333, 0.0}, &cell2);
    check(hit2.has_value() && hit2->bodyId == body2Id && hit2->faceId == face2Id,
          "vtkCellPicker distinguishes the second Body and its Face provenance");
    check(cell1 != cell2,
          "different CAD Bodies are represented by different display cells without sharing identity");

    const auto bodySelection = scene.selectionItemForCell(static_cast<std::size_t>(cell1),
                                                           d26::SelectionKind::Body);
    const auto faceSelection = scene.selectionItemForCell(static_cast<std::size_t>(cell1),
                                                           d26::SelectionKind::Face);
    check(bodySelection.has_value() && bodySelection->geometryEntityId == body1Id
              && bodySelection->sourceRevision == revision,
          "Body filter converts the picked display cell to the CAD Body identity");
    check(faceSelection.has_value() && faceSelection->geometryEntityId == face1Id
              && faceSelection->parentGeometryId == body1Id
              && faceSelection->sourceRevision == revision,
          "Face filter converts the same picked cell to CAD Face + parent Body identity");

    const auto faceCells = scene.cellIndicesForSelection(*faceSelection);
    check(faceCells.size() == 2,
          "one CAD Face selection highlights all of its display triangles, not one picked triangle");
    const auto bodyCells = scene.cellIndicesForSelection(*bodySelection);
    check(bodyCells.size() == 2,
          "one CAD Body selection highlights every display triangle belonging to that Body");

    const auto face2 = scene.selectionItemForCell(static_cast<std::size_t>(cell2),
                                                   d26::SelectionKind::Face);
    check(face2.has_value(), "second picked cell converts to a Face selection item");
    if (face2.has_value()) {
        const auto multiCells = scene.cellIndicesForSelection(QVector<d26::SelectionItem>{*faceSelection, *face2});
        check(multiCells.size() == 4,
              "multi-Face selection spans display triangles from multiple CAD Bodies");
    }

    d26::SelectionItem stale = *faceSelection;
    stale.sourceRevision = revision + 1;
    check(scene.cellIndicesForSelection(stale).empty(),
          "stale geometry revision cannot generate a CAD selection overlay");

    const auto emptyHit = pickWorld(picker, renderer, scene, {0.0, 0.0, 0.0});
    check(!emptyHit.has_value(),
          "empty viewport location between Bodies produces no CAD provenance hit");
}

void appearanceSemanticTests()
{
    for (const bool dark : {false, true}) {
        const d26::ViewportPalette palette = d26::ViewportPalette::forAppearance(dark);
        const d26::Rgb background = palette.color(d26::RenderRole::Background);
        const d26::Rgb selection = palette.color(d26::RenderRole::Selection);
        const d26::Rgb preselection = palette.color(d26::RenderRole::Preselection);

        check(different(selection, preselection),
              dark ? "Dark: Selection and Preselection remain distinct semantic colors"
                   : "Light: Selection and Preselection remain distinct semantic colors");
        check(contrastRatio(selection, background) >= 3.0,
              dark ? "Dark: committed selection keeps minimum background contrast guard"
                   : "Light: committed selection keeps minimum background contrast guard");
        check(contrastRatio(preselection, background) >= 2.5,
              dark ? "Dark: preselection keeps minimum background contrast guard"
                   : "Light: preselection keeps minimum background contrast guard");
    }

    // Bu sayısal guard yalnız semantic palette regresyonunu yakalar. Gerçek
    // macOS Light/Dark görsel yeterliliği yine kullanıcı acceptance maddesidir.
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    pickerAndProvenanceTests();
    appearanceSemanticTests();
    std::cout << (failures == 0 ? "viewport topology selection PASS\n"
                                : "viewport topology selection FAIL\n");
    return failures == 0 ? 0 : 1;
}
