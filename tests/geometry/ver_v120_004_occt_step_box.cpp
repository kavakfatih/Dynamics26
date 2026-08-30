#include "femcae/geometry/OcctStepImporter.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <STEPControl_StepModelType.hxx>
#include <STEPControl_Writer.hxx>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <unordered_set>

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
    const GeometryEntityId bodyId=bodies.front();

    const auto topology=importer.tessellateWithTopology(bodyId,0.5);
    const auto& tess=topology.display;
    require(!tess.points.empty()&&!tess.triangles.empty(),"box display tessellation");
    require(tess.sourceGeometryId==bodyId,"tessellation body provenance");
    require(tess.sourceRevision==doc.revision(),"tessellation CAD revision provenance");
    require(topology.hasConsistentProvenance(),"triangle/face provenance sizes match");
    require(topology.triangleFaceIds.size()==tess.triangles.size(),"each display triangle has one CAD Face id");

    std::unordered_set<GeometryEntityId> representedFaces;
    for(const GeometryEntityId faceId:topology.triangleFaceIds){
        require(faceId!=InvalidGeometryId,"triangle Face id valid");
        const GeometryEntity* face=doc.find(faceId);
        require(face!=nullptr,"triangle Face id exists in GeometryDocument");
        require(face->kind==GeometryEntityKind::Face,"triangle provenance points to Face kind");
        require(face->parentId==bodyId,"triangle Face parent is selected Body");
        representedFaces.insert(faceId);
    }
    require(representedFaces.size()==6,"all six CAD box faces are represented by display triangles");

    std::size_t bodyFaceCount=0;
    for(const GeometryEntityId faceId:doc.entitiesOfKind(GeometryEntityKind::Face)){
        const GeometryEntity* face=doc.find(faceId);
        if(face!=nullptr && face->parentId==bodyId){
            ++bodyFaceCount;
            require(representedFaces.find(faceId)!=representedFaces.end(),"every Body Face appears in tessellation provenance");
        }
    }
    require(bodyFaceCount==6,"Body owns six CAD Face entities");

    // Eski API geriye uyumlu kalmali: body-level tessellation ayni geometriyi
    // verir, ancak Face provenance isteyen yeni GUI yolu companion API'yi kullanir.
    const auto legacy=importer.tessellate(bodyId,0.5);
    require(legacy.sourceGeometryId==bodyId,"legacy tessellation body provenance preserved");
    require(legacy.sourceRevision==doc.revision(),"legacy tessellation revision preserved");
    require(legacy.triangles.size()==tess.triangles.size(),"legacy tessellation triangle count preserved");

    std::cout<<"PASS VER-V120-004 OCCT STEP box topology provenance\n";
}
