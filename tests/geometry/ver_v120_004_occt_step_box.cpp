#include "femcae/geometry/OcctStepImporter.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <STEPControl_StepModelType.hxx>
#include <STEPControl_Writer.hxx>
#include <cstdlib>
#include <filesystem>
#include <iostream>
using namespace femcae::geometry;
static void require(bool c,const char*m){if(!c){std::cerr<<"FAIL: "<<m<<'\n';std::exit(1);}}
int main(){
    const auto path=std::filesystem::temp_directory_path()/"femcae_v120_box.step";
    STEPControl_Writer writer;
    const auto box=BRepPrimAPI_MakeBox(10.0,20.0,30.0).Shape();
    require(writer.Transfer(box,STEPControl_AsIs)==IFSelect_RetDone,"STEP writer transfer");
    require(writer.Write(path.string().c_str())==IFSelect_RetDone,"STEP writer file");
    GeometryDocument doc("v120-box");
    OcctStepImporter importer;
    const auto r=importer.importFile(path.string(),doc);
    std::filesystem::remove(path);
    require(r.success,r.message.c_str());
    require(r.bodyCount>=1,"box body count");
    require(r.faceCount==6,"box face count");
    const auto bodies=doc.entitiesOfKind(GeometryEntityKind::Body);
    require(!bodies.empty(),"box body id");
    const auto tess=importer.tessellate(bodies.front(),0.5);
    require(!tess.points.empty()&&!tess.triangles.empty(),"box display tessellation");
    require(tess.sourceGeometryId==bodies.front(),"tessellation geometry provenance");
    require(tess.sourceRevision==doc.revision(),"tessellation CAD revision provenance");
    std::cout<<"PASS VER-V120-004 OCCT STEP box import/tessellation\n";
}
