#include "femcae/geometry/OcctStepImporter.h"

#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <STEPControl_StepModelType.hxx>
#include <STEPControl_Writer.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_set>

using namespace femcae::geometry;

static void require(bool c,const char*m){if(!c){std::cerr<<"FAIL: "<<m<<'\n';std::exit(1);}}

static std::filesystem::path writeStep(const std::string& name,const TopoDS_Shape& shape){
    const auto path=std::filesystem::temp_directory_path()/name;
    STEPControl_Writer writer;
    require(writer.Transfer(shape,STEPControl_AsIs)==IFSelect_RetDone,"STEP writer transfer");
    require(writer.Write(path.string().c_str())==IFSelect_RetDone,"STEP writer file");
    return path;
}

static GeometryEntityId requireSingleBody(const GeometryDocument& doc){
    const auto bodies=doc.entitiesOfKind(GeometryEntityKind::Body);
    require(bodies.size()==1,"expected exactly one Body");
    return bodies.front();
}

static void requireBodyTopology(const GeometryDocument& doc,
                                const GeometryEntityId bodyId,
                                const std::size_t faceCount,
                                const std::size_t edgeCount,
                                const std::size_t vertexCount){
    std::size_t faces=0,edges=0,vertices=0;
    std::unordered_set<std::string> persistentKeys;

    const auto verifyKind=[&](const GeometryEntityKind kind,std::size_t& count){
        for(const GeometryEntityId id:doc.entitiesOfKind(kind)){
            const GeometryEntity* entity=doc.find(id);
            require(entity!=nullptr,"topology entity exists");
            if(entity->parentId!=bodyId) continue;
            ++count;
            require(!entity->persistentKey.empty(),"topology persistent key non-empty");
            require(persistentKeys.insert(entity->persistentKey).second,"topology persistent key unique");
        }
    };

    verifyKind(GeometryEntityKind::Face,faces);
    verifyKind(GeometryEntityKind::Edge,edges);
    verifyKind(GeometryEntityKind::Vertex,vertices);

    require(faces==faceCount,"Body canonical Face count");
    require(edges==edgeCount,"Body canonical Edge count");
    require(vertices==vertexCount,"Body canonical Vertex count");
}

int main(){
    GeometryDocument doc("v120-topology");
    OcctStepImporter importer;

    // Canonical solid baseline: bir dikdortgen prizma tam olarak 6 Face,
    // 12 unique Edge ve 8 unique Vertex tasir. TopExp traversal tekrarlarinin
    // document entity sayisini sisirmesine izin verilmez.
    const auto boxPath=writeStep("femcae_v120_box.step",BRepPrimAPI_MakeBox(10.0,20.0,30.0).Shape());
    const auto boxResult=importer.importFile(boxPath.string(),doc);
    std::filesystem::remove(boxPath);
    require(boxResult.success,boxResult.message.c_str());
    require(boxResult.bodyCount==1,"box body count");
    require(boxResult.faceCount==6,"box canonical face count");
    require(boxResult.edgeCount==12,"box canonical edge count");
    require(boxResult.vertexCount==8,"box canonical vertex count");

    const GeometryEntityId bodyId=requireSingleBody(doc);
    requireBodyTopology(doc,bodyId,6,12,8);

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

    // Eski API geriye uyumlu kalmali: canonical topology map Alpha.3.2
    // Body/Face display contract'ini degistirmez.
    const auto legacy=importer.tessellate(bodyId,0.5);
    require(legacy.sourceGeometryId==bodyId,"legacy tessellation body provenance preserved");
    require(legacy.sourceRevision==doc.revision(),"legacy tessellation revision preserved");
    require(legacy.triangles.size()==tess.triangles.size(),"legacy tessellation triangle count preserved");

    // Solid olmayan STEP fallback de ayni topology registration helper'ini
    // kullanmali. Tek dikdortgen Face: 1 Face / 4 unique Edge / 4 Vertex.
    const gp_Pln plane(gp_Pnt(0.0,0.0,0.0),gp_Dir(0.0,0.0,1.0));
    const auto planarFace=BRepBuilderAPI_MakeFace(plane,0.0,10.0,0.0,5.0).Shape();
    const auto facePath=writeStep("femcae_v120_face.step",planarFace);
    const auto faceResult=importer.importFile(facePath.string(),doc);
    std::filesystem::remove(facePath);
    require(faceResult.success,faceResult.message.c_str());
    require(faceResult.bodyCount==1,"surface fallback body count");
    require(faceResult.faceCount==1,"surface fallback face count");
    require(faceResult.edgeCount==4,"surface fallback canonical edge count");
    require(faceResult.vertexCount==4,"surface fallback canonical vertex count");

    const GeometryEntityId surfaceBodyId=requireSingleBody(doc);
    requireBodyTopology(doc,surfaceBodyId,1,4,4);
    const auto surfaceTopology=importer.tessellateWithTopology(surfaceBodyId,0.5);
    require(surfaceTopology.hasConsistentProvenance(),"surface fallback Face provenance consistent");
    require(!surfaceTopology.display.triangles.empty(),"surface fallback display tessellation");

    std::cout<<"PASS VER-V120-004 OCCT canonical Face/Edge/Vertex topology provenance\n";
}
