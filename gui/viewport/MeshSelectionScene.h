#pragma once

// Dynamics26 Alpha.3.4 — VTK-independent FEM selection provenance scene.
//
// SimulationMesh kimlikleri ile display primitive indeksleri ayni sey DEGILDIR.
// Bu sinif generated FEM mesh'ten gorunur boundary selection tablosu uretir ve
// VTK cell/point indeksini yalniz provenance lookup anahtari olarak kullanir.
//
// CAD Geometry != Display Tessellation != FEM Mesh

#include "../core/SelectionTypes.h"

#include <femcae/meshing/MeshTypes.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace d26 {

class MeshSelectionScene final
{
public:
    [[nodiscard]] bool set(const femcae::meshing::SimulationMesh &mesh, const quint64 generation)
    {
        using femcae::meshing::InvalidMeshId;
        using femcae::meshing::MeshEntityId;

        if (generation == 0 || mesh.nodes.empty() || mesh.elements.empty() || mesh.boundaryFacets.empty()) {
            clear();
            return false;
        }

        std::unordered_map<MeshEntityId, std::uint32_t> nodePointIndex;
        nodePointIndex.reserve(mesh.nodes.size());
        std::unordered_set<MeshEntityId> nodeIds;
        nodeIds.reserve(mesh.nodes.size());
        std::unordered_set<MeshEntityId> elementIds;
        elementIds.reserve(mesh.elements.size());
        std::unordered_set<MeshEntityId> facetIds;
        facetIds.reserve(mesh.boundaryFacets.size());

        std::vector<femcae::geometry::Vec3> allPoints;
        std::vector<MeshEntityId> allNodeIds;
        allPoints.reserve(mesh.nodes.size());
        allNodeIds.reserve(mesh.nodes.size());

        for (std::size_t i = 0; i < mesh.nodes.size(); ++i) {
            const auto &node = mesh.nodes[i];
            if (node.id == InvalidMeshId || !nodeIds.insert(node.id).second
                || i > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
                clear();
                return false;
            }
            nodePointIndex.emplace(node.id, static_cast<std::uint32_t>(i));
            allPoints.push_back(node.x);
            allNodeIds.push_back(node.id);
        }

        for (const auto &element : mesh.elements) {
            if (element.id == InvalidMeshId || !elementIds.insert(element.id).second) {
                clear();
                return false;
            }
        }

        std::vector<std::array<std::uint32_t, 4>> boundaryQuads;
        std::vector<MeshEntityId> boundaryFacetIds;
        std::vector<MeshEntityId> ownerElementIds;
        std::unordered_set<MeshEntityId> visibleNodeSet;
        boundaryQuads.reserve(mesh.boundaryFacets.size());
        boundaryFacetIds.reserve(mesh.boundaryFacets.size());
        ownerElementIds.reserve(mesh.boundaryFacets.size());

        for (const auto &facet : mesh.boundaryFacets) {
            if (facet.id == InvalidMeshId || !facetIds.insert(facet.id).second
                || facet.ownerElementId == InvalidMeshId
                || elementIds.find(facet.ownerElementId) == elementIds.end()) {
                clear();
                return false;
            }

            std::array<std::uint32_t, 4> quad{};
            for (std::size_t i = 0; i < facet.nodeIds.size(); ++i) {
                const auto it = nodePointIndex.find(facet.nodeIds[i]);
                if (it == nodePointIndex.end()) {
                    clear();
                    return false;
                }
                quad[i] = it->second;
                visibleNodeSet.insert(facet.nodeIds[i]);
            }
            boundaryQuads.push_back(quad);
            boundaryFacetIds.push_back(facet.id);
            ownerElementIds.push_back(facet.ownerElementId);
        }

        std::vector<femcae::geometry::Vec3> visibleNodePoints;
        std::vector<MeshEntityId> visibleNodeIds;
        visibleNodePoints.reserve(visibleNodeSet.size());
        visibleNodeIds.reserve(visibleNodeSet.size());
        // Deterministik display/pick sirasi SimulationMesh::nodes sirasidir.
        for (std::size_t i = 0; i < allNodeIds.size(); ++i) {
            if (visibleNodeSet.find(allNodeIds[i]) == visibleNodeSet.end()) {
                continue;
            }
            visibleNodePoints.push_back(allPoints[i]);
            visibleNodeIds.push_back(allNodeIds[i]);
        }

        generation_ = generation;
        points_ = std::move(allPoints);
        nodeIds_ = std::move(allNodeIds);
        visibleNodePoints_ = std::move(visibleNodePoints);
        visibleNodeIds_ = std::move(visibleNodeIds);
        boundaryQuads_ = std::move(boundaryQuads);
        facetIds_ = std::move(boundaryFacetIds);
        ownerElementIds_ = std::move(ownerElementIds);
        return complete();
    }

    void clear()
    {
        generation_ = 0;
        points_.clear();
        nodeIds_.clear();
        visibleNodePoints_.clear();
        visibleNodeIds_.clear();
        boundaryQuads_.clear();
        facetIds_.clear();
        ownerElementIds_.clear();
    }

    [[nodiscard]] bool empty() const noexcept { return boundaryQuads_.empty(); }
    [[nodiscard]] bool complete() const noexcept
    {
        return generation_ != 0 && !points_.empty() && !visibleNodePoints_.empty()
            && !boundaryQuads_.empty() && boundaryQuads_.size() == facetIds_.size()
            && boundaryQuads_.size() == ownerElementIds_.size()
            && visibleNodePoints_.size() == visibleNodeIds_.size();
    }
    [[nodiscard]] quint64 generation() const noexcept { return generation_; }

    [[nodiscard]] const std::vector<femcae::geometry::Vec3> &points() const noexcept { return points_; }
    [[nodiscard]] const std::vector<femcae::meshing::MeshEntityId> &nodeIds() const noexcept { return nodeIds_; }
    [[nodiscard]] const std::vector<femcae::geometry::Vec3> &visibleNodePoints() const noexcept
    {
        return visibleNodePoints_;
    }
    [[nodiscard]] const std::vector<femcae::meshing::MeshEntityId> &visibleNodeIds() const noexcept
    {
        return visibleNodeIds_;
    }
    [[nodiscard]] const std::vector<std::array<std::uint32_t, 4>> &boundaryQuads() const noexcept
    {
        return boundaryQuads_;
    }
    [[nodiscard]] const std::vector<femcae::meshing::MeshEntityId> &facetIds() const noexcept { return facetIds_; }
    [[nodiscard]] const std::vector<femcae::meshing::MeshEntityId> &ownerElementIds() const noexcept
    {
        return ownerElementIds_;
    }

    [[nodiscard]] std::optional<SelectionItem> selectionItemForVisibleNode(const std::size_t point) const
    {
        if (!complete() || point >= visibleNodeIds_.size()) {
            return std::nullopt;
        }
        SelectionItem item;
        item.domain = SelectionDomain::Mesh;
        item.kind = SelectionKind::Node;
        item.meshEntityId = visibleNodeIds_[point];
        item.sourceRevision = generation_;
        return item;
    }

    [[nodiscard]] std::optional<SelectionItem> selectionItemForBoundaryCell(const std::size_t cell,
                                                                            const SelectionKind kind) const
    {
        if (!complete() || cell >= boundaryQuads_.size()) {
            return std::nullopt;
        }
        SelectionItem item;
        item.domain = SelectionDomain::Mesh;
        item.kind = kind;
        item.sourceRevision = generation_;
        if (kind == SelectionKind::Facet) {
            item.meshEntityId = facetIds_[cell];
            return item;
        }
        if (kind == SelectionKind::Element) {
            item.meshEntityId = ownerElementIds_[cell];
            return item;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::vector<std::size_t> boundaryCellsForSelection(const QVector<SelectionItem> &items) const
    {
        std::vector<std::size_t> cells;
        if (!complete() || items.isEmpty()) {
            return cells;
        }
        cells.reserve(boundaryQuads_.size());
        for (std::size_t cell = 0; cell < boundaryQuads_.size(); ++cell) {
            for (const SelectionItem &item : items) {
                if (item.domain != SelectionDomain::Mesh || item.sourceRevision != generation_) {
                    continue;
                }
                const bool selectedFacet = item.kind == SelectionKind::Facet
                    && item.meshEntityId == facetIds_[cell];
                const bool selectedElement = item.kind == SelectionKind::Element
                    && item.meshEntityId == ownerElementIds_[cell];
                if (selectedFacet || selectedElement) {
                    cells.push_back(cell);
                    break;
                }
            }
        }
        return cells;
    }

    [[nodiscard]] std::vector<std::size_t> visibleNodeIndicesForSelection(const QVector<SelectionItem> &items) const
    {
        std::vector<std::size_t> points;
        if (!complete() || items.isEmpty()) {
            return points;
        }
        points.reserve(visibleNodeIds_.size());
        for (std::size_t point = 0; point < visibleNodeIds_.size(); ++point) {
            for (const SelectionItem &item : items) {
                if (item.domain == SelectionDomain::Mesh && item.kind == SelectionKind::Node
                    && item.sourceRevision == generation_ && item.meshEntityId == visibleNodeIds_[point]) {
                    points.push_back(point);
                    break;
                }
            }
        }
        return points;
    }

private:
    quint64 generation_{0};
    std::vector<femcae::geometry::Vec3> points_;
    std::vector<femcae::meshing::MeshEntityId> nodeIds_;
    std::vector<femcae::geometry::Vec3> visibleNodePoints_;
    std::vector<femcae::meshing::MeshEntityId> visibleNodeIds_;
    std::vector<std::array<std::uint32_t, 4>> boundaryQuads_;
    std::vector<femcae::meshing::MeshEntityId> facetIds_;
    std::vector<femcae::meshing::MeshEntityId> ownerElementIds_;
};

} // namespace d26
