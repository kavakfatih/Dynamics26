#pragma once

#include <QWidget>

#include <femcae/geometry/GeometryTypes.h>
#include <femcae/meshing/ResultDatabase.h>

class QVBoxLayout;

#ifdef FEMCAE_GUI_HAS_VTK
class QVTKOpenGLNativeWidget;
class vtkRenderer;
#endif

class ViewportWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit ViewportWidget(QWidget *parent = nullptr);
    void resetCamera();
    void refreshSystemAppearance();
    void showAxialBarResult(double lengthM, double displacementM, double stressPa);
    void showAxialBarMode(double lengthM, double midNormalized, double tipNormalized, double phase);
    void showNonlinearHex8Result(double lengthM, double areaM2, double displacementM);
    void showMixedShearHex8Result(double gamma);
    void showContactHex8Result(double penetrationM);
    void showGeometryTessellation(const femcae::geometry::GeometryTessellation &tessellation);
    void showSimulationMeshResult(const femcae::meshing::SimulationMesh &mesh, const femcae::meshing::ResultDatabase &results);

private:
#ifdef FEMCAE_GUI_HAS_VTK
    QVTKOpenGLNativeWidget *vtkWidget_ = nullptr;
    vtkRenderer *renderer_ = nullptr;
#endif
};
