#include "ViewportWidget.h"

#include <QLabel>
#include <QVBoxLayout>

#include <cmath>
#include <algorithm>
#include <unordered_map>

#ifdef FEMCAE_GUI_HAS_VTK
#include <QVTKOpenGLNativeWidget.h>
#include <vtkActor.h>
#include <vtkCubeSource.h>
#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkHexahedron.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDataSetMapper.h>
#include <vtkDoubleArray.h>
#include <vtkCellData.h>
#endif

ViewportWidget::ViewportWidget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

#ifdef FEMCAE_GUI_HAS_VTK
    vtkWidget_ = new QVTKOpenGLNativeWidget(this);
    layout->addWidget(vtkWidget_);

    vtkNew<vtkGenericOpenGLRenderWindow> window;
    vtkNew<vtkRenderer> renderer;
    renderer_ = renderer;
    window->AddRenderer(renderer);
    vtkWidget_->setRenderWindow(window);

    vtkNew<vtkCubeSource> cube;
    cube->SetXLength(2.4);
    cube->SetYLength(0.9);
    cube->SetZLength(0.45);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(cube->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetRepresentationToWireframe();
    actor->GetProperty()->SetLineWidth(1.5);
    actor->GetProperty()->SetColor(0.18, 0.45, 0.88);

    renderer->AddActor(actor);
    renderer->SetBackground(0.965, 0.968, 0.975);
    renderer->ResetCamera();
#else
    auto *placeholder = new QLabel(
        tr("3B Görünüm\n\nVTK bulunamadığı için bu derlemede Qt placeholder viewport kullanılıyor.\n"
           "macOS GUI build'inde VTK etkinleştirildiğinde gerçek interaktif viewport açılır."), this);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setObjectName("viewportPlaceholder");
    layout->addWidget(placeholder);
#endif
}

void ViewportWidget::resetCamera()
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (renderer_ != nullptr) {
        renderer_->ResetCamera();
        vtkWidget_->renderWindow()->Render();
    }
#endif
}


void ViewportWidget::showAxialBarResult(double lengthM, double displacementM, double stressPa)
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (renderer_ == nullptr) {
        return;
    }
    renderer_->RemoveAllViewProps();
    vtkNew<vtkCubeSource> undeformed;
    undeformed->SetXLength(lengthM);
    undeformed->SetYLength(qMax(lengthM * 0.08, 1.0e-4));
    undeformed->SetZLength(qMax(lengthM * 0.08, 1.0e-4));
    undeformed->SetCenter(0.5 * lengthM, 0.0, 0.0);
    vtkNew<vtkPolyDataMapper> undeformedMapper;
    undeformedMapper->SetInputConnection(undeformed->GetOutputPort());
    vtkNew<vtkActor> undeformedActor;
    undeformedActor->SetMapper(undeformedMapper);
    undeformedActor->GetProperty()->SetRepresentationToWireframe();
    undeformedActor->GetProperty()->SetColor(0.55, 0.55, 0.58);

    const double shownDisplacement = displacementM * 20.0;
    vtkNew<vtkCubeSource> deformed;
    deformed->SetXLength(qMax(lengthM + shownDisplacement, 1.0e-6));
    deformed->SetYLength(qMax(lengthM * 0.06, 1.0e-4));
    deformed->SetZLength(qMax(lengthM * 0.06, 1.0e-4));
    deformed->SetCenter(0.5 * (lengthM + shownDisplacement), 0.0, 0.0);
    vtkNew<vtkPolyDataMapper> deformedMapper;
    deformedMapper->SetInputConnection(deformed->GetOutputPort());
    vtkNew<vtkActor> deformedActor;
    deformedActor->SetMapper(deformedMapper);
    if (stressPa >= 0.0) {
        deformedActor->GetProperty()->SetColor(0.85, 0.20, 0.16);
    } else {
        deformedActor->GetProperty()->SetColor(0.10, 0.42, 0.86);
    }
    renderer_->AddActor(undeformedActor);
    renderer_->AddActor(deformedActor);
    renderer_->ResetCamera();
    vtkWidget_->renderWindow()->Render();
#else
    Q_UNUSED(lengthM)
    Q_UNUSED(displacementM)
    Q_UNUSED(stressPa)
#endif
}


void ViewportWidget::showAxialBarMode(double lengthM, double midNormalized, double tipNormalized, double phase)
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (renderer_ == nullptr) {
        return;
    }
    renderer_->RemoveAllViewProps();

    vtkNew<vtkPoints> referencePoints;
    referencePoints->InsertNextPoint(0.0, 0.0, 0.0);
    referencePoints->InsertNextPoint(0.5 * lengthM, 0.0, 0.0);
    referencePoints->InsertNextPoint(lengthM, 0.0, 0.0);
    vtkNew<vtkCellArray> referenceLines;
    vtkIdType referenceIds[3] = {0, 1, 2};
    referenceLines->InsertNextCell(3, referenceIds);
    vtkNew<vtkPolyData> referenceData;
    referenceData->SetPoints(referencePoints);
    referenceData->SetLines(referenceLines);
    vtkNew<vtkPolyDataMapper> referenceMapper;
    referenceMapper->SetInputData(referenceData);
    vtkNew<vtkActor> referenceActor;
    referenceActor->SetMapper(referenceMapper);
    referenceActor->GetProperty()->SetColor(0.58, 0.58, 0.61);
    referenceActor->GetProperty()->SetLineWidth(2.0);

    const double visualScale = 0.14 * lengthM;
    vtkNew<vtkPoints> modePoints;
    modePoints->InsertNextPoint(0.0, 0.0, 0.0);
    modePoints->InsertNextPoint(0.5 * lengthM + visualScale * midNormalized * phase, 0.0, 0.0);
    modePoints->InsertNextPoint(lengthM + visualScale * tipNormalized * phase, 0.0, 0.0);
    vtkNew<vtkCellArray> modeLines;
    vtkIdType modeIds[3] = {0, 1, 2};
    modeLines->InsertNextCell(3, modeIds);
    vtkNew<vtkPolyData> modeData;
    modeData->SetPoints(modePoints);
    modeData->SetLines(modeLines);
    vtkNew<vtkPolyDataMapper> modeMapper;
    modeMapper->SetInputData(modeData);
    vtkNew<vtkActor> modeActor;
    modeActor->SetMapper(modeMapper);
    modeActor->GetProperty()->SetColor(0.12, 0.42, 0.88);
    modeActor->GetProperty()->SetLineWidth(5.0);

    renderer_->AddActor(referenceActor);
    renderer_->AddActor(modeActor);
    if (phase == 0.0) {
        renderer_->ResetCamera();
    }
    vtkWidget_->renderWindow()->Render();
#else
    Q_UNUSED(lengthM)
    Q_UNUSED(midNormalized)
    Q_UNUSED(tipNormalized)
    Q_UNUSED(phase)
#endif
}

void ViewportWidget::showNonlinearHex8Result(double lengthM, double areaM2, double displacementM)
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (renderer_ == nullptr) {
        return;
    }
    renderer_->RemoveAllViewProps();
    const double side = std::sqrt(qMax(areaM2, 1.0e-12));

    vtkNew<vtkCubeSource> reference;
    reference->SetXLength(lengthM);
    reference->SetYLength(side);
    reference->SetZLength(side);
    reference->SetCenter(0.5 * lengthM, 0.0, 0.0);
    vtkNew<vtkPolyDataMapper> referenceMapper;
    referenceMapper->SetInputConnection(reference->GetOutputPort());
    vtkNew<vtkActor> referenceActor;
    referenceActor->SetMapper(referenceMapper);
    referenceActor->GetProperty()->SetRepresentationToWireframe();
    referenceActor->GetProperty()->SetColor(0.55, 0.55, 0.58);
    referenceActor->GetProperty()->SetLineWidth(1.5);

    const double deformedLength = qMax(lengthM + displacementM, 1.0e-9);
    vtkNew<vtkCubeSource> deformed;
    deformed->SetXLength(deformedLength);
    deformed->SetYLength(side);
    deformed->SetZLength(side);
    deformed->SetCenter(0.5 * deformedLength, 0.0, 0.0);
    vtkNew<vtkPolyDataMapper> deformedMapper;
    deformedMapper->SetInputConnection(deformed->GetOutputPort());
    vtkNew<vtkActor> deformedActor;
    deformedActor->SetMapper(deformedMapper);
    deformedActor->GetProperty()->SetColor(0.13, 0.46, 0.86);
    deformedActor->GetProperty()->SetOpacity(0.82);

    renderer_->AddActor(referenceActor);
    renderer_->AddActor(deformedActor);
    renderer_->ResetCamera();
    vtkWidget_->renderWindow()->Render();
#else
    Q_UNUSED(lengthM)
    Q_UNUSED(areaM2)
    Q_UNUSED(displacementM)
#endif
}


void ViewportWidget::showMixedShearHex8Result(double gamma)
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (renderer_ == nullptr) { return; }
    renderer_->RemoveAllViewProps();
    const double xyz[8][3] = {{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};
    const vtkIdType edges[12][2] = {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
    auto addWire = [&](bool deformed, double r, double g, double b, double width) {
        vtkNew<vtkPoints> points;
        for (const auto &p : xyz) {
            const double xx = p[0] + (deformed ? gamma*p[1] : 0.0);
            points->InsertNextPoint(xx,p[1],p[2]);
        }
        vtkNew<vtkCellArray> lines;
        for (const auto &edge : edges) { lines->InsertNextCell(2,edge); }
        vtkNew<vtkPolyData> data; data->SetPoints(points); data->SetLines(lines);
        vtkNew<vtkPolyDataMapper> mapper; mapper->SetInputData(data);
        vtkNew<vtkActor> actor; actor->SetMapper(mapper); actor->GetProperty()->SetColor(r,g,b); actor->GetProperty()->SetLineWidth(width);
        renderer_->AddActor(actor);
    };
    addWire(false,0.58,0.58,0.61,1.5);
    addWire(true,0.12,0.42,0.88,3.0);
    renderer_->ResetCamera();
    vtkWidget_->renderWindow()->Render();
#else
    Q_UNUSED(gamma)
#endif
}


void ViewportWidget::showContactHex8Result(double penetrationM)
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (renderer_ == nullptr) { return; }
    renderer_->RemoveAllViewProps();
    vtkNew<vtkCubeSource> master;
    master->SetXLength(1.35); master->SetYLength(1.35); master->SetZLength(0.025);
    master->SetCenter(0.0,0.0,-0.0125);
    vtkNew<vtkPolyDataMapper> masterMapper; masterMapper->SetInputConnection(master->GetOutputPort());
    vtkNew<vtkActor> masterActor; masterActor->SetMapper(masterMapper);
    masterActor->GetProperty()->SetColor(0.33,0.34,0.37); masterActor->GetProperty()->SetOpacity(0.88);

    vtkNew<vtkCubeSource> reference;
    reference->SetXLength(1.0); reference->SetYLength(1.0); reference->SetZLength(1.0);
    reference->SetCenter(0.0,0.0,0.5);
    vtkNew<vtkPolyDataMapper> referenceMapper; referenceMapper->SetInputConnection(reference->GetOutputPort());
    vtkNew<vtkActor> referenceActor; referenceActor->SetMapper(referenceMapper);
    referenceActor->GetProperty()->SetRepresentationToWireframe(); referenceActor->GetProperty()->SetColor(0.58,0.58,0.61);

    const double shownPenetration = qMin(0.12, qMax(0.002, penetrationM*200.0));
    vtkNew<vtkCubeSource> deformed;
    deformed->SetXLength(1.0); deformed->SetYLength(1.0); deformed->SetZLength(1.0-shownPenetration);
    deformed->SetCenter(0.0,0.0,0.5-shownPenetration*0.5);
    vtkNew<vtkPolyDataMapper> deformedMapper; deformedMapper->SetInputConnection(deformed->GetOutputPort());
    vtkNew<vtkActor> deformedActor; deformedActor->SetMapper(deformedMapper);
    deformedActor->GetProperty()->SetColor(0.12,0.42,0.88); deformedActor->GetProperty()->SetOpacity(0.80);

    renderer_->AddActor(masterActor); renderer_->AddActor(referenceActor); renderer_->AddActor(deformedActor);
    renderer_->ResetCamera(); vtkWidget_->renderWindow()->Render();
#else
    Q_UNUSED(penetrationM)
#endif
}


void ViewportWidget::showGeometryTessellation(const femcae::geometry::GeometryTessellation &tessellation)
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (renderer_ == nullptr) { return; }
    renderer_->RemoveAllViewProps();
    vtkNew<vtkPoints> points;
    for (const auto &point : tessellation.points) {
        points->InsertNextPoint(point.x, point.y, point.z);
    }
    vtkNew<vtkCellArray> triangles;
    for (const auto &triangle : tessellation.triangles) {
        vtkIdType ids[3] = {
            static_cast<vtkIdType>(triangle[0]),
            static_cast<vtkIdType>(triangle[1]),
            static_cast<vtkIdType>(triangle[2])
        };
        triangles->InsertNextCell(3, ids);
    }
    vtkNew<vtkPolyData> data;
    data->SetPoints(points);
    data->SetPolys(triangles);
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(data);
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(0.76, 0.78, 0.82);
    actor->GetProperty()->SetEdgeVisibility(true);
    actor->GetProperty()->SetEdgeColor(0.26, 0.28, 0.33);
    actor->GetProperty()->SetLineWidth(1.0);
    renderer_->AddActor(actor);
    renderer_->ResetCamera();
    vtkWidget_->renderWindow()->Render();
#else
    Q_UNUSED(tessellation)
#endif
}


void ViewportWidget::showSimulationMeshResult(const femcae::meshing::SimulationMesh &mesh,
                                              const femcae::meshing::ResultDatabase &results)
{
#ifdef FEMCAE_GUI_HAS_VTK
    if (renderer_ == nullptr) { return; }
    renderer_->RemoveAllViewProps();
    vtkNew<vtkPoints> points;
    std::unordered_map<femcae::meshing::MeshEntityId, vtkIdType> index;
    const auto *disp = results.displacement();
    double maxU = 0.0;
    if (disp != nullptr) {
        for (const auto &entry : disp->values) {
            const auto &u = entry.second;
            maxU = std::max(maxU, std::sqrt(u.x*u.x + u.y*u.y + u.z*u.z));
        }
    }
    double span = 0.0;
    if (!mesh.nodes.empty()) {
        double xmin=mesh.nodes.front().x.x,xmax=xmin,ymin=mesh.nodes.front().x.y,ymax=ymin,zmin=mesh.nodes.front().x.z,zmax=zmin;
        for (const auto &n : mesh.nodes) { xmin=std::min(xmin,n.x.x);xmax=std::max(xmax,n.x.x);ymin=std::min(ymin,n.x.y);ymax=std::max(ymax,n.x.y);zmin=std::min(zmin,n.x.z);zmax=std::max(zmax,n.x.z); }
        span=std::max({xmax-xmin,ymax-ymin,zmax-zmin});
    }
    const double scale = maxU > 0.0 ? std::min(50.0, 0.15*std::max(span,1.0e-9)/maxU) : 0.0;
    for (std::size_t i=0;i<mesh.nodes.size();++i) {
        const auto &n=mesh.nodes[i]; femcae::geometry::Vec3 u{};
        if (disp != nullptr) { const auto it=disp->values.find(n.id); if (it!=disp->values.end()) u=it->second; }
        points->InsertNextPoint(n.x.x+scale*u.x,n.x.y+scale*u.y,n.x.z+scale*u.z);
        index[n.id]=static_cast<vtkIdType>(i);
    }
    vtkNew<vtkUnstructuredGrid> grid; grid->SetPoints(points);
    for (const auto &e : mesh.elements) {
        vtkNew<vtkHexahedron> cell;
        for (int a=0;a<8;++a) cell->GetPointIds()->SetId(a,index.at(e.nodeIds[static_cast<std::size_t>(a)]));
        grid->InsertNextCell(cell->GetCellType(),cell->GetPointIds());
    }
    const auto *vm = results.elementScalar("von_mises");
    if (vm != nullptr) {
        vtkNew<vtkDoubleArray> scalar; scalar->SetName("von_mises_MPa"); scalar->SetNumberOfComponents(1);
        for (const auto &e : mesh.elements) { const auto it=vm->values.find(e.id); scalar->InsertNextValue((it==vm->values.end()?0.0:it->second)/1.0e6); }
        grid->GetCellData()->SetScalars(scalar);
    }
    vtkNew<vtkDataSetMapper> mapper; mapper->SetInputData(grid);
    if (vm != nullptr) { double range[2]; grid->GetScalarRange(range); mapper->SetScalarRange(range); mapper->ScalarVisibilityOn(); } else { mapper->ScalarVisibilityOff(); }
    vtkNew<vtkActor> actor; actor->SetMapper(mapper); actor->GetProperty()->SetEdgeVisibility(true); actor->GetProperty()->SetEdgeColor(0.18,0.20,0.24); actor->GetProperty()->SetLineWidth(1.0);
    if (vm == nullptr) actor->GetProperty()->SetColor(0.72,0.76,0.84);
    renderer_->AddActor(actor); renderer_->ResetCamera(); vtkWidget_->renderWindow()->Render();
#else
    Q_UNUSED(mesh)
    Q_UNUSED(results)
#endif
}
