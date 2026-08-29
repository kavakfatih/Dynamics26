#pragma once

#include "femcae/geometry/GeometryTypes.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace femcae::meshing {

using MeshEntityId = std::int64_t;
inline constexpr MeshEntityId InvalidMeshId = -1;

enum class MeshTopology : std::uint8_t { Hex8 = 1, Quad4 = 2 };

struct MeshNode {
    MeshEntityId id{InvalidMeshId};
    geometry::Vec3 x;
    geometry::GeometryEntityId sourceGeometryId{geometry::InvalidGeometryId};
};

struct MeshElement {
    MeshEntityId id{InvalidMeshId};
    MeshTopology topology{MeshTopology::Hex8};
    std::array<MeshEntityId, 8> nodeIds{};
    geometry::GeometryEntityId sourceGeometryId{geometry::InvalidGeometryId};
};

struct MeshFacet {
    MeshEntityId id{InvalidMeshId};
    std::array<MeshEntityId, 4> nodeIds{};
    MeshEntityId ownerElementId{InvalidMeshId};
    geometry::GeometryEntityId sourceGeometryId{geometry::InvalidGeometryId};
};

struct MeshQuality {
    double minimumScaledJacobian{0.0};
    double maximumAspectRatio{0.0};
    std::size_t invertedElementCount{0};
    std::size_t degenerateElementCount{0};
};

struct SimulationMesh {
    std::uint64_t sourceGeometryRevision{0};
    std::vector<MeshNode> nodes;
    std::vector<MeshElement> elements;
    std::vector<MeshFacet> boundaryFacets;

    [[nodiscard]] const MeshNode* findNode(MeshEntityId id) const noexcept;
    [[nodiscard]] const MeshElement* findElement(MeshEntityId id) const noexcept;
    [[nodiscard]] std::vector<MeshEntityId> nodeIdsForGeometry(geometry::GeometryEntityId geometryId) const;
    [[nodiscard]] std::vector<MeshEntityId> elementIdsForGeometry(geometry::GeometryEntityId geometryId) const;
    [[nodiscard]] std::vector<MeshEntityId> facetIdsForGeometry(geometry::GeometryEntityId geometryId) const;
};

struct AxisAlignedBox {
    geometry::Vec3 min;
    geometry::Vec3 max;
};

struct BoxBoundaryGeometry {
    geometry::GeometryEntityId body{geometry::InvalidGeometryId};
    geometry::GeometryEntityId xMin{geometry::InvalidGeometryId};
    geometry::GeometryEntityId xMax{geometry::InvalidGeometryId};
    geometry::GeometryEntityId yMin{geometry::InvalidGeometryId};
    geometry::GeometryEntityId yMax{geometry::InvalidGeometryId};
    geometry::GeometryEntityId zMin{geometry::InvalidGeometryId};
    geometry::GeometryEntityId zMax{geometry::InvalidGeometryId};
};

} // namespace femcae::meshing
