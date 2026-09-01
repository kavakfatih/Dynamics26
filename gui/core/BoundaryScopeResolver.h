#pragma once

// Dynamics26 Alpha.3.6 — boundary-condition scope -> FEM node union helper.
//
// Bir Fixed Support / Force birden fazla CAD Face kapsayabilir. Solver'a her Face
// için ayrı load assignment göndermek aynı toplam kuvveti birden fazla kez
// uygular. Bu saf helper bütün CAD Face boundary node'larını TEK engineering node
// kümesine indirger. Display/VTK indeksleri bu katmana giremez.
//
// CAD GeometryEntityId -> FEM boundary provenance -> MeshEntityId

#include <femcae/geometry/GeometryTypes.h>
#include <femcae/meshing/AssignmentResolver.h>
#include <femcae/meshing/MeshTypes.h>

#include <QVector>

#include <set>
#include <vector>

namespace d26 {

[[nodiscard]] inline std::vector<femcae::meshing::MeshEntityId>
boundaryNodeUnionForGeometryFaces(
    const femcae::meshing::SimulationMesh &mesh,
    const QVector<femcae::geometry::GeometryEntityId> &geometryFaceIds)
{
    std::set<femcae::meshing::MeshEntityId> uniqueNodes;
    for (const femcae::geometry::GeometryEntityId geometryId : geometryFaceIds) {
        if (geometryId == femcae::geometry::InvalidGeometryId) {
            continue;
        }
        const auto ids = femcae::meshing::boundaryNodeIdsForGeometry(mesh, geometryId);
        uniqueNodes.insert(ids.begin(), ids.end());
    }
    return {uniqueNodes.begin(), uniqueNodes.end()};
}

} // namespace d26
