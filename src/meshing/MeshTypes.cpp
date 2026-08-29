#include "femcae/meshing/MeshTypes.h"

namespace femcae::meshing {

const MeshNode* SimulationMesh::findNode(const MeshEntityId id) const noexcept {
    for (const auto& node : nodes) if (node.id == id) return &node;
    return nullptr;
}

const MeshElement* SimulationMesh::findElement(const MeshEntityId id) const noexcept {
    for (const auto& element : elements) if (element.id == id) return &element;
    return nullptr;
}

std::vector<MeshEntityId> SimulationMesh::nodeIdsForGeometry(const geometry::GeometryEntityId geometryId) const {
    std::vector<MeshEntityId> ids;
    for (const auto& node : nodes) if (node.sourceGeometryId == geometryId) ids.push_back(node.id);
    return ids;
}

std::vector<MeshEntityId> SimulationMesh::elementIdsForGeometry(const geometry::GeometryEntityId geometryId) const {
    std::vector<MeshEntityId> ids;
    for (const auto& element : elements) if (element.sourceGeometryId == geometryId) ids.push_back(element.id);
    return ids;
}

std::vector<MeshEntityId> SimulationMesh::facetIdsForGeometry(const geometry::GeometryEntityId geometryId) const {
    std::vector<MeshEntityId> ids;
    for (const auto& facet : boundaryFacets) if (facet.sourceGeometryId == geometryId) ids.push_back(facet.id);
    return ids;
}

} // namespace femcae::meshing
