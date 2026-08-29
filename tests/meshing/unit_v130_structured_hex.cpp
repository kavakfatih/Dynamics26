#include <femcae/meshing/StructuredHexMesher.h>
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    using namespace femcae::meshing;
    StructuredHexMesher mesher;
    BoxBoundaryGeometry g{100,101,102,103,104,105,106};
    StructuredHexMesherOptions o; o.nx=2;o.ny=1;o.nz=1;o.firstNodeId=1000;o.firstElementId=500;o.firstFacetId=900;
    const auto m=mesher.meshBox({{0,0,0},{2,1,1}},g,77,o);
    assert(m.sourceGeometryRevision==77);
    assert(m.nodes.size()==12);
    assert(m.elements.size()==2);
    assert(m.boundaryFacets.size()==10);
    assert(m.elements[0].id==500 && m.elements[1].id==501);
    assert(m.elementIdsForGeometry(100).size()==2);
    assert(m.facetIdsForGeometry(101).size()==1);
    assert(m.facetIdsForGeometry(102).size()==1);
    assert(m.facetIdsForGeometry(103).size()==2);
    const auto q=evaluateHexMeshQuality(m);
    assert(q.invertedElementCount==0 && q.degenerateElementCount==0);
    assert(q.minimumScaledJacobian>0.999999);
    assert(std::abs(q.maximumAspectRatio-1.0)<1e-12);
    std::cout<<"V0.13 structured HEX8 mesh/provenance PASS\n";
}
