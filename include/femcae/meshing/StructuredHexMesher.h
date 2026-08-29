#pragma once

#include "femcae/meshing/MeshTypes.h"

#include <cstddef>

namespace femcae::meshing {

struct StructuredHexMesherOptions {
    std::size_t nx{1};
    std::size_t ny{1};
    std::size_t nz{1};
    MeshEntityId firstNodeId{1};
    MeshEntityId firstElementId{1};
    MeshEntityId firstFacetId{1};
};

class StructuredHexMesher {
public:
    [[nodiscard]] SimulationMesh meshBox(const AxisAlignedBox& box,
                                         const BoxBoundaryGeometry& geometry,
                                         std::uint64_t sourceGeometryRevision,
                                         const StructuredHexMesherOptions& options = {}) const;
};

[[nodiscard]] MeshQuality evaluateHexMeshQuality(const SimulationMesh& mesh);

} // namespace femcae::meshing
