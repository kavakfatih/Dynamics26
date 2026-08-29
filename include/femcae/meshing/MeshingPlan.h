#pragma once

#include "femcae/meshing/MeshTypes.h"
#include "femcae/meshing/StructuredHexMesher.h"

#include <unordered_map>

namespace femcae::meshing {

struct MeshingPlan {
    double globalTargetSize{0.0};
    MeshTopology preferredTopology{MeshTopology::Hex8};
    std::unordered_map<geometry::GeometryEntityId, double> localTargetSize;
};

// Axis-aligned structured baseline. Face-local sizes refine directions tangent
// to that CAD face. This is deliberately not advertised as a general local
// unstructured refinement algorithm.
[[nodiscard]] StructuredHexMesherOptions structuredOptionsFromSizing(
    const AxisAlignedBox& box,
    const BoxBoundaryGeometry& geometry,
    const MeshingPlan& plan);

} // namespace femcae::meshing
