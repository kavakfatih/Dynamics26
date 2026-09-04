#include <femcae/meshing/Assignments.h>
#include <femcae/meshing/ResultDatabase.h>
#include <femcae/meshing/StructuredHexMesher.h>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <unordered_set>
int main(){
    using namespace femcae::meshing;
    AssignmentStore store;
    GeometryAssignment bc;bc.kind=AssignmentKind::Constraint;bc.targetGeometryId=11;bc.constrained={true,true,true};
    GeometryAssignment load;load.kind=AssignmentKind::Load;load.targetGeometryId=12;load.vectorValue={100,0,0};
    store.add(bc);store.add(load);assert(store.all().size()==2);assert(store.forGeometry(11).size()==1);
    StructuredHexMesher mesher;BoxBoundaryGeometry g{10,11,12,13,14,15,16};auto m=mesher.meshBox({{0,0,0},{1,1,1}},g,5,{});
    ResultDatabase db;NodeVectorField u;u.name="displacement";
    u.metadata={ResultPhysicalQuantity::Displacement,ResultMeasure::Magnitude,
                ResultAssociation::Node,ResultSourceLocation::MeshNode,
                ResultRecoveryMethod::Direct,ResultUnit::Meter,ResultUnit::Millimeter,
                ResultConfiguration::FinalConverged,0};
    for(const auto&n:m.nodes)u.values[n.id]={0.1*n.x.x,0,0};
    db.setDisplacement(u);
    NodeVectorField reaction;reaction.name="reaction_force";
    reaction.metadata={ResultPhysicalQuantity::ReactionForce,ResultMeasure::Vector,ResultAssociation::Node,
                       ResultSourceLocation::ConstrainedDegreesOfFreedom,ResultRecoveryMethod::EquilibriumRecovery,
                       ResultUnit::Newton,ResultUnit::Newton,ResultConfiguration::FinalConverged,
                       0};
    for(const auto&n:m.nodes)reaction.values[n.id]={0,0,0};
    std::vector<MeshEntityId> supportNodes;std::unordered_set<MeshEntityId> uniqueSupportNodes;
    for(const auto&facet:m.boundaryFacets)if(facet.sourceGeometryId==g.xMin)for(const auto nodeId:facet.nodeIds)
        if(uniqueSupportNodes.insert(nodeId).second)supportNodes.push_back(nodeId);
    for(const auto nodeId:supportNodes)reaction.values[nodeId]={-100.0/static_cast<double>(supportNodes.size()),0,0};
    db.setReaction(reaction);
    ElementScalarField vm;vm.name="von_mises";
    vm.metadata={ResultPhysicalQuantity::Stress,ResultMeasure::CauchyVonMises,
                 ResultAssociation::Element,ResultSourceLocation::IntegrationPoints,
                 ResultRecoveryMethod::ArithmeticMean,ResultUnit::Pascal,ResultUnit::MegaPascal,
                 ResultConfiguration::FinalConverged,8};
    vm.values[m.elements[0].id]=123.0;db.setElementScalar(vm);
    const auto probe=db.probeNearestNode(m,{1,1,1});assert(probe && probe->vectorValue.x>0.099);
    assert(db.displacement()->metadata.association==ResultAssociation::Node);
    assert(db.reaction()!=nullptr && db.reaction()->metadata.quantity==ResultPhysicalQuantity::ReactionForce);
    assert(db.elementScalar("von_mises")->metadata.measure==ResultMeasure::CauchyVonMises);
    assert(db.elementScalar("von_mises")->metadata.integrationPointCount==8);
    const auto stressProbe=db.probeBoundaryFacet(m,m.boundaryFacets.front().id,"von_mises");
    assert(stressProbe && stressProbe->elementId==m.boundaryFacets.front().ownerElementId
           && std::abs(stressProbe->scalarValue-123.0)<1.0e-12);
    const auto supportReaction=db.reactionResultant(m,supportNodes);
    assert(supportReaction && supportReaction->nodeCount==supportNodes.size()
           && std::abs(supportReaction->value.x+100.0)<1.0e-12
           && std::abs(supportReaction->centroid.x)<1.0e-12);
    const auto cut=db.cutElements(m,{{0.5,0,0},{1,0,0},1e-12});assert(cut.size()==1);
    const auto csv=std::filesystem::temp_directory_path()/"femcae_v130.csv";const auto vtk=std::filesystem::temp_directory_path()/"femcae_v130.vtk";
    db.exportCsv(m,csv);db.exportLegacyVtk(m,vtk,1.0);assert(std::filesystem::file_size(csv)>0 && std::filesystem::file_size(vtk)>0);
    std::filesystem::remove(csv);std::filesystem::remove(vtk);
    std::cout<<"V0.13 assignment/result/probe/export PASS\n";
}
