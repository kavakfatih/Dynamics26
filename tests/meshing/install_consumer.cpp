#include <femcae/meshing/StructuredHexMesher.h>
#include <femcae/meshing/GeometryMeshBridge.h>
#include <iostream>
int main(){using namespace femcae::meshing;StructuredHexMesher m;BoxBoundaryGeometry g{10,11,12,13,14,15,16};const auto mesh=m.meshBox({{0,0,0},{1,1,1}},g,1,{});const auto q=evaluateHexMeshQuality(mesh);const auto a=buildGeometryAssociationMap(mesh);if(mesh.elements.size()!=1||q.invertedElementCount!=0||!a.find(10))return 1;std::cout<<"installed femcae_meshing consumer PASS\n";}
