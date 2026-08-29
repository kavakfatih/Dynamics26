#include <femcae/meshing/GeometryMeshBridge.h>
#include <femcae/meshing/StructuredHexMesher.h>
#include <cassert>
#include <iostream>
int main(){
    using namespace femcae::meshing;BoxBoundaryGeometry g{10,11,12,13,14,15,16};StructuredHexMesher mesher;StructuredHexMesherOptions o;o.nx=2;o.ny=2;o.nz=1;
    const auto m=mesher.meshBox({{0,0,0},{2,2,1}},g,44,o);const auto map=buildGeometryAssociationMap(m);
    const auto* body=map.find(g.body);const auto* xmin=map.find(g.xMin);const auto* xmax=map.find(g.xMax);
    assert(body && body->femElementIds.size()==4);assert(xmin && xmin->femFacetIds.size()==2);assert(xmax && xmax->femNodeIds.size()==6);
    assert(m.sourceGeometryRevision==44);
    std::cout<<"V0.13 GeometryAssociationMap bridge PASS\n";
}
