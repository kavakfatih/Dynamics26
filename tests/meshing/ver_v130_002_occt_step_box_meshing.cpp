#include <femcae/geometry/OcctStepImporter.h>
#include <femcae/meshing/GeometryMeshBridge.h>
#include <femcae/meshing/StructuredHexMesher.h>
#include <BRepPrimAPI_MakeBox.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <STEPControl_StepModelType.hxx>
#include <STEPControl_Writer.hxx>
#include <cassert>
#include <filesystem>
#include <iostream>
int main(){
    using namespace femcae::geometry; using namespace femcae::meshing;
    const auto path=std::filesystem::temp_directory_path()/"femcae_v130_box.step";
    STEPControl_Writer writer;const auto box=BRepPrimAPI_MakeBox(2.0,1.0,0.5).Shape();
    assert(writer.Transfer(box,STEPControl_AsIs)==IFSelect_RetDone);assert(writer.Write(path.string().c_str())==IFSelect_RetDone);
    GeometryDocument doc("v130-step-box");OcctStepImporter importer;const auto imported=importer.importFile(path.string(),doc);std::filesystem::remove(path);assert(imported.success);
    const auto bodies=doc.entitiesOfKind(GeometryEntityKind::Body);assert(bodies.size()==1);
    const auto descriptor=importer.axisAlignedBoxDescriptor(bodies.front());assert(descriptor);
    StructuredHexMesherOptions o;o.nx=2;o.ny=1;o.nz=1;StructuredHexMesher mesher;
    BoxBoundaryGeometry g{descriptor->bodyId,descriptor->xMinFace,descriptor->xMaxFace,descriptor->yMinFace,descriptor->yMaxFace,descriptor->zMinFace,descriptor->zMaxFace};
    const auto mesh=mesher.meshBox({descriptor->min,descriptor->max},g,doc.revision(),o);const auto map=buildGeometryAssociationMap(mesh);
    assert(mesh.elements.size()==2);assert(mesh.sourceGeometryRevision==doc.revision());
    assert(map.find(descriptor->xMinFace) && map.find(descriptor->xMaxFace));assert(map.find(descriptor->xMinFace)->femFacetIds.size()==1);
    std::cout<<"V0.13 native STEP box -> CAD face provenance -> HEX8 mesh PASS\n";
}
