#pragma once

#include "femcae/geometry/GeometryDocument.h"
#include "femcae/meshing/MeshTypes.h"

namespace femcae::meshing {

[[nodiscard]] geometry::GeometryAssociationMap buildGeometryAssociationMap(const SimulationMesh& mesh);

} // namespace femcae::meshing
