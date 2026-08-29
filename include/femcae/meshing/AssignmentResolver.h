#pragma once

#include "femcae/meshing/Assignments.h"
#include "femcae/meshing/MeshTypes.h"

#include <unordered_map>
#include <vector>

namespace femcae::meshing {

struct ResolvedAssignment {
    GeometryAssignment source;
    std::vector<MeshEntityId> nodeIds;
    std::vector<MeshEntityId> elementIds;
    std::vector<MeshEntityId> facetIds;
};

[[nodiscard]] std::vector<ResolvedAssignment> resolveAssignments(const SimulationMesh& mesh,
                                                                 const AssignmentStore& store);
[[nodiscard]] std::vector<MeshEntityId> boundaryNodeIdsForGeometry(const SimulationMesh& mesh,
                                                                   geometry::GeometryEntityId geometryId);

} // namespace femcae::meshing
