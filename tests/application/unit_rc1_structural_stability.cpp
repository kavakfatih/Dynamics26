#include <femcae/application/StructuralStability.h>

#include <algorithm>
#include <array>
#include <iostream>
#include <string_view>
#include <vector>

using namespace femcae::application;
using namespace femcae::meshing;

namespace {

int failures = 0;

void expect(const bool condition, const std::string_view label)
{
    if (condition) {
        std::cout << "PASS " << label << '\n';
    } else {
        std::cerr << "FAIL " << label << '\n';
        ++failures;
    }
}

SimulationMesh oneHex(const double scale = 1.0, const MeshEntityId offset = 0)
{
    SimulationMesh mesh;
    mesh.nodes = {
        {offset + 1, {0.0, 0.0, 0.0}},
        {offset + 2, {scale, 0.0, 0.0}},
        {offset + 3, {scale, scale, 0.0}},
        {offset + 4, {0.0, scale, 0.0}},
        {offset + 5, {0.0, 0.0, scale}},
        {offset + 6, {scale, 0.0, scale}},
        {offset + 7, {scale, scale, scale}},
        {offset + 8, {0.0, scale, scale}}
    };
    mesh.elements = {{offset + 101, MeshTopology::Hex8,
                      {offset + 1, offset + 2, offset + 3, offset + 4,
                       offset + 5, offset + 6, offset + 7, offset + 8}}};
    return mesh;
}

std::vector<StructuralConstraintDof> fullFaceConstraints(const MeshEntityId offset = 0)
{
    std::vector<StructuralConstraintDof> constraints;
    for (const MeshEntityId nodeId : {offset + 1, offset + 4, offset + 5, offset + 8}) {
        for (int component = 1; component <= 3; ++component) {
            constraints.push_back({nodeId, component});
        }
    }
    return constraints;
}

void appendMesh(SimulationMesh &destination, const SimulationMesh &source)
{
    destination.nodes.insert(destination.nodes.end(), source.nodes.begin(), source.nodes.end());
    destination.elements.insert(destination.elements.end(), source.elements.begin(), source.elements.end());
}

} // namespace

int main()
{
    const SimulationMesh cube = oneHex();

    const StructuralStabilityResult fullFace =
        StructuralStabilityDiagnostic::evaluate(cube, fullFaceConstraints());
    expect(fullFace.success() && fullFace.stable() && fullFace.components.size() == 1
               && fullFace.components.front().restraintRank == 6,
           "full-face XYZ Fixed Support has rigid-body restraint rank 6/6");

    const StructuralStabilityResult oneNode = StructuralStabilityDiagnostic::evaluate(
        cube, {{1, 1}, {1, 2}, {1, 3}});
    expect(oneNode.success() && !oneNode.stable()
               && oneNode.components.front().restraintRank == 3
               && oneNode.components.front().freeRigidBodyModeCount == 3,
           "one-node XYZ constraint leaves three rotational rigid-body modes");

    // 3-2-1 minimum restraint: node 1 XYZ, x-axis node 2 YZ, xy-plane node 4 Z.
    const StructuralStabilityResult minimum321 = StructuralStabilityDiagnostic::evaluate(
        cube, {{1, 1}, {1, 2}, {1, 3}, {2, 2}, {2, 3}, {4, 3}});
    expect(minimum321.success() && minimum321.stable(),
           "proper 3-2-1 restraint reaches rank 6/6");

    std::vector<StructuralConstraintDof> directional;
    for (const MeshEntityId nodeId : {1, 4, 5, 8}) directional.push_back({nodeId, 1});
    for (const MeshEntityId nodeId : {1, 2, 5, 6}) directional.push_back({nodeId, 2});
    for (const MeshEntityId nodeId : {1, 2, 3, 4}) directional.push_back({nodeId, 3});
    const StructuralStabilityResult combinedDirectional =
        StructuralStabilityDiagnostic::evaluate(cube, directional);
    expect(combinedDirectional.success() && combinedDirectional.stable(),
           "directional face constraints can jointly stabilize the component");

    SimulationMesh disconnected;
    appendMesh(disconnected, oneHex(1.0, 0));
    SimulationMesh second = oneHex(1.0, 1000);
    for (auto &node : second.nodes) node.x.x += 3.0;
    appendMesh(disconnected, second);
    const StructuralStabilityResult oneFree =
        StructuralStabilityDiagnostic::evaluate(disconnected, fullFaceConstraints());
    expect(oneFree.success() && !oneFree.stable() && oneFree.components.size() == 2
               && oneFree.components[0].restraintRank == 6
               && oneFree.components[1].restraintRank == 0,
           "disconnected constrained/free cubes report the free component");

    std::vector<StructuralConstraintDof> bothFixed = fullFaceConstraints();
    const auto secondConstraints = fullFaceConstraints(1000);
    bothFixed.insert(bothFixed.end(), secondConstraints.begin(), secondConstraints.end());
    const StructuralStabilityResult bothStable =
        StructuralStabilityDiagnostic::evaluate(disconnected, bothFixed);
    expect(bothStable.success() && bothStable.stable()
               && bothStable.components.size() == 2,
           "two disconnected cubes are checked and stabilized independently");

    SimulationMesh connected = oneHex();
    connected.nodes.insert(connected.nodes.end(), {
        {9, {2.0, 0.0, 0.0}}, {10, {2.0, 1.0, 0.0}},
        {11, {2.0, 0.0, 1.0}}, {12, {2.0, 1.0, 1.0}}
    });
    connected.elements.push_back({202, MeshTopology::Hex8,
                                  {2, 9, 10, 3, 6, 11, 12, 7}});
    const StructuralStabilityResult sharedNodes =
        StructuralStabilityDiagnostic::evaluate(connected, fullFaceConstraints());
    expect(sharedNodes.success() && sharedNodes.stable()
               && sharedNodes.components.size() == 1
               && sharedNodes.components.front().elementIds.size() == 2,
           "HEX8 elements sharing nodes form one connected component");

    const StructuralStabilityResult millimetreScale =
        StructuralStabilityDiagnostic::evaluate(oneHex(1.0e-3),
                                                 fullFaceConstraints());
    const StructuralStabilityResult metreScale =
        StructuralStabilityDiagnostic::evaluate(oneHex(1.0),
                                                 fullFaceConstraints());
    expect(millimetreScale.stable() == metreScale.stable()
               && millimetreScale.components.front().restraintRank
                   == metreScale.components.front().restraintRank,
           "coordinate scale change preserves the rank decision");

    SimulationMesh nearCollinear = oneHex();
    nearCollinear.nodes[3].x = {2.0, 1.0e-12, 0.0};
    const StructuralStabilityResult nearCollinearResult =
        StructuralStabilityDiagnostic::evaluate(
            nearCollinear, {{1, 1}, {1, 2}, {1, 3}, {2, 2}, {2, 3}, {4, 3}});
    expect(nearCollinearResult.success() && !nearCollinearResult.stable()
               && nearCollinearResult.components.front().restraintRank == 5,
           "near-collinear 3-2-1 points use deterministic rank tolerance");

    std::vector<StructuralConstraintDof> duplicates = fullFaceConstraints();
    const std::vector<StructuralConstraintDof> duplicateCopy = duplicates;
    duplicates.insert(duplicates.end(), duplicateCopy.begin(), duplicateCopy.end());
    const StructuralStabilityResult duplicateRows =
        StructuralStabilityDiagnostic::evaluate(cube, duplicates);
    expect(duplicateRows.stable()
               && duplicateRows.components.front().constrainedDofCount == 12,
           "duplicate support DOFs are canonicalized before rank evaluation");

    expect(StructuralStabilityDiagnostic::evaluate(cube, {{99999, 1}}).error
               == StructuralStabilityError::InvalidConstraint,
           "dangling constraint node is rejected explicitly");

    std::cout << "RC1 structural stability failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
