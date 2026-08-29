#include <femcae/meshing/AssignmentResolver.h>
#include <femcae/meshing/MeshingPlan.h>
#include <cassert>
#include <iostream>
int main(){
    using namespace femcae::meshing;
    AxisAlignedBox box{{0,0,0},{2,1,1}};BoxBoundaryGeometry g{10,11,12,13,14,15,16};
    MeshingPlan plan;plan.globalTargetSize=1.0;plan.localTargetSize[g.xMax]=0.25;
    const auto o=structuredOptionsFromSizing(box,g,plan);
    // xMax face local size refines y/z tangential directions; x stays global.
    assert(o.nx==2 && o.ny==4 && o.nz==4);
    StructuredHexMesher mesher;const auto m=mesher.meshBox(box,g,3,o);
    AssignmentStore store;
    GeometryAssignment mat;mat.kind=AssignmentKind::Material;mat.targetGeometryId=g.body;mat.referencedEntityId=77;store.add(mat);
    GeometryAssignment bc;bc.kind=AssignmentKind::Constraint;bc.targetGeometryId=g.xMin;bc.constrained={true,true,true};store.add(bc);
    GeometryAssignment load;load.kind=AssignmentKind::Load;load.targetGeometryId=g.xMax;load.vectorValue={100,0,0};store.add(load);
    const auto r=resolveAssignments(m,store);assert(r.size()==3);
    assert(r[0].elementIds.size()==m.elements.size());
    assert(r[1].nodeIds.size()==(o.ny+1)*(o.nz+1));
    assert(r[2].facetIds.size()==o.ny*o.nz);
    std::cout<<"V0.13 sizing + geometry assignment resolution PASS\n";
}
