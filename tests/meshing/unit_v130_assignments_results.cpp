#include <femcae/meshing/Assignments.h>
#include <femcae/meshing/ResultDatabase.h>
#include <femcae/meshing/StructuredHexMesher.h>
#include <cassert>
#include <filesystem>
#include <iostream>
int main(){
    using namespace femcae::meshing;
    AssignmentStore store;
    GeometryAssignment bc;bc.kind=AssignmentKind::Constraint;bc.targetGeometryId=11;bc.constrained={true,true,true};
    GeometryAssignment load;load.kind=AssignmentKind::Load;load.targetGeometryId=12;load.vectorValue={100,0,0};
    store.add(bc);store.add(load);assert(store.all().size()==2);assert(store.forGeometry(11).size()==1);
    StructuredHexMesher mesher;BoxBoundaryGeometry g{10,11,12,13,14,15,16};auto m=mesher.meshBox({{0,0,0},{1,1,1}},g,5,{});
    ResultDatabase db;NodeVectorField u;u.name="displacement";for(const auto&n:m.nodes)u.values[n.id]={0.1*n.x.x,0,0};db.setDisplacement(u);
    ElementScalarField vm;vm.name="von_mises";vm.values[m.elements[0].id]=123.0;db.setElementScalar(vm);
    const auto probe=db.probeNearestNode(m,{1,1,1});assert(probe && probe->vectorValue.x>0.099);
    const auto cut=db.cutElements(m,{{0.5,0,0},{1,0,0},1e-12});assert(cut.size()==1);
    const auto csv=std::filesystem::temp_directory_path()/"femcae_v130.csv";const auto vtk=std::filesystem::temp_directory_path()/"femcae_v130.vtk";
    db.exportCsv(m,csv);db.exportLegacyVtk(m,vtk,1.0);assert(std::filesystem::file_size(csv)>0 && std::filesystem::file_size(vtk)>0);
    std::filesystem::remove(csv);std::filesystem::remove(vtk);
    std::cout<<"V0.13 assignment/result/probe/export PASS\n";
}
