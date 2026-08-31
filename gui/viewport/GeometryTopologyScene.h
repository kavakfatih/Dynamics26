#pragma once

// Dynamics26 Alpha.3.3 — CAD Body/Face/Edge/Vertex display provenance sahnesi.
//
// Bu sınıf VTK bilmez. CAD topolojisini display primitive indekslerinden ayırır:
//   triangle -> Face, line -> Edge, point -> Vertex.
// Display indeksleri hiçbir zaman CAD GeometryEntityId veya FEM kimliği değildir.
//
// CAD Geometry != Display Tessellation != FEM Mesh

#include "GeometrySelectionScene.h"

#include <femcae/geometry/GeometryTypes.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace d26 {

struct GeometrySceneLineProvenance {
    femcae::geometry::GeometryEntityId bodyId{femcae::geometry::InvalidGeometryId};
    femcae::geometry::GeometryEntityId edgeId{femcae::geometry::InvalidGeometryId};
};

struct GeometryScenePointProvenance {
    femcae::geometry::GeometryEntityId bodyId{femcae::geometry::InvalidGeometryId};
    femcae::geometry::GeometryEntityId vertexId{femcae::geometry::InvalidGeometryId};
};

class GeometryTopologyScene final
{
public:
    [[nodiscard]] bool append(const femcae::geometry::TopologyTessellation &surface,
                              const femcae::geometry::EdgeDisplayTessellation &edges,
                              const femcae::geometry::VertexDisplayPoints &vertices)
    {
        using femcae::geometry::InvalidGeometryId;

        const auto bodyId = surface.display.sourceGeometryId;
        const quint64 revision = surface.display.sourceRevision;
        if (bodyId == InvalidGeometryId || revision == 0
            || edges.sourceGeometryId != bodyId || vertices.sourceGeometryId != bodyId
            || edges.sourceRevision != revision || vertices.sourceRevision != revision
            || !surface.hasConsistentProvenance()
            || !edges.hasConsistentProvenance()
            || !vertices.hasConsistentProvenance()
            || surface.display.triangles.empty() || edges.lines.empty() || vertices.points.empty()) {
            return false;
        }
        if (revision_ != 0 && revision_ != revision) {
            return false;
        }
        if (edges.points.size()
                > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) - edgePoints_.size()
            || vertices.points.size()
                > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) - vertexPoints_.size()) {
            return false;
        }

        for (std::size_t i = 0; i < edges.lines.size(); ++i) {
            const auto &line = edges.lines[i];
            if (line[0] >= edges.points.size() || line[1] >= edges.points.size()
                || edges.lineEdgeIds[i] == InvalidGeometryId) {
                return false;
            }
        }
        for (const auto vertexId : vertices.pointVertexIds) {
            if (vertexId == InvalidGeometryId) {
                return false;
            }
        }

        // Üç primitive ailesi tek body transaction'ı olarak eklenir. Surface
        // append başarısızsa Edge/Vertex state hiç değişmez; Edge/Vertex doğrulaması
        // yukarıda tamamlandığı için devam eden mutation başarısız olamaz.
        GeometrySelectionScene nextSurface = surface_;
        if (!nextSurface.append(surface)) {
            return false;
        }

        const std::uint32_t edgeBase = static_cast<std::uint32_t>(edgePoints_.size());
        surface_ = std::move(nextSurface);
        edgePoints_.insert(edgePoints_.end(), edges.points.begin(), edges.points.end());
        edgeLines_.reserve(edgeLines_.size() + edges.lines.size());
        edgeProvenance_.reserve(edgeProvenance_.size() + edges.lines.size());
        for (std::size_t i = 0; i < edges.lines.size(); ++i) {
            const auto &line = edges.lines[i];
            edgeLines_.push_back({edgeBase + line[0], edgeBase + line[1]});
            edgeProvenance_.push_back({bodyId, edges.lineEdgeIds[i]});
        }

        vertexPoints_.insert(vertexPoints_.end(), vertices.points.begin(), vertices.points.end());
        vertexProvenance_.reserve(vertexProvenance_.size() + vertices.points.size());
        for (const auto vertexId : vertices.pointVertexIds) {
            vertexProvenance_.push_back({bodyId, vertexId});
        }

        if (revision_ == 0) {
            revision_ = revision;
        }
        return true;
    }

    void clear()
    {
        revision_ = 0;
        surface_.clear();
        edgePoints_.clear();
        edgeLines_.clear();
        edgeProvenance_.clear();
        vertexPoints_.clear();
        vertexProvenance_.clear();
    }

    [[nodiscard]] quint64 sourceRevision() const noexcept { return revision_; }
    [[nodiscard]] bool empty() const noexcept { return surface_.empty(); }
    [[nodiscard]] bool hasFaceProvenance() const noexcept { return surface_.hasFaceProvenance(); }
    [[nodiscard]] bool hasEdgeProvenance() const noexcept
    {
        return !edgeLines_.empty() && edgeLines_.size() == edgeProvenance_.size();
    }
    [[nodiscard]] bool hasVertexProvenance() const noexcept
    {
        return !vertexPoints_.empty() && vertexPoints_.size() == vertexProvenance_.size();
    }
    [[nodiscard]] bool complete() const noexcept
    {
        return revision_ != 0 && hasFaceProvenance() && hasEdgeProvenance() && hasVertexProvenance();
    }

    [[nodiscard]] const GeometrySelectionScene &surface() const noexcept { return surface_; }
    [[nodiscard]] const std::vector<femcae::geometry::Vec3> &edgePoints() const noexcept { return edgePoints_; }
    [[nodiscard]] const std::vector<std::array<std::uint32_t, 2>> &edgeLines() const noexcept { return edgeLines_; }
    [[nodiscard]] const std::vector<GeometrySceneLineProvenance> &edgeProvenance() const noexcept
    {
        return edgeProvenance_;
    }
    [[nodiscard]] const std::vector<femcae::geometry::Vec3> &vertexPoints() const noexcept { return vertexPoints_; }
    [[nodiscard]] const std::vector<GeometryScenePointProvenance> &vertexProvenance() const noexcept
    {
        return vertexProvenance_;
    }

    [[nodiscard]] std::optional<SelectionItem> selectionItemForSurfaceCell(const std::size_t cell,
                                                                           const SelectionKind kind) const
    {
        return surface_.selectionItemForCell(cell, kind);
    }

    [[nodiscard]] std::optional<SelectionItem> selectionItemForEdgeCell(const std::size_t cell) const
    {
        if (cell >= edgeProvenance_.size()) {
            return std::nullopt;
        }
        const auto &hit = edgeProvenance_[cell];
        SelectionItem item;
        item.domain = SelectionDomain::Geometry;
        item.kind = SelectionKind::Edge;
        item.geometryEntityId = hit.edgeId;
        item.parentGeometryId = hit.bodyId;
        item.sourceRevision = revision_;
        return item;
    }

    [[nodiscard]] std::optional<SelectionItem> selectionItemForVertexCell(const std::size_t cell) const
    {
        if (cell >= vertexProvenance_.size()) {
            return std::nullopt;
        }
        const auto &hit = vertexProvenance_[cell];
        SelectionItem item;
        item.domain = SelectionDomain::Geometry;
        item.kind = SelectionKind::Vertex;
        item.geometryEntityId = hit.vertexId;
        item.parentGeometryId = hit.bodyId;
        item.sourceRevision = revision_;
        return item;
    }

    [[nodiscard]] std::vector<std::size_t> lineIndicesForSelection(const QVector<SelectionItem> &items) const
    {
        std::vector<std::size_t> indices;
        if (items.isEmpty() || revision_ == 0) {
            return indices;
        }
        indices.reserve(edgeProvenance_.size());
        for (std::size_t line = 0; line < edgeProvenance_.size(); ++line) {
            const auto &entry = edgeProvenance_[line];
            for (const SelectionItem &item : items) {
                if (item.domain != SelectionDomain::Geometry || item.sourceRevision != revision_) {
                    continue;
                }
                if (item.kind == SelectionKind::Edge && item.geometryEntityId == entry.edgeId
                    && (item.parentGeometryId == femcae::geometry::InvalidGeometryId
                        || item.parentGeometryId == entry.bodyId)) {
                    indices.push_back(line);
                    break;
                }
            }
        }
        return indices;
    }

    [[nodiscard]] std::vector<std::size_t> pointIndicesForSelection(const QVector<SelectionItem> &items) const
    {
        std::vector<std::size_t> indices;
        if (items.isEmpty() || revision_ == 0) {
            return indices;
        }
        indices.reserve(vertexProvenance_.size());
        for (std::size_t point = 0; point < vertexProvenance_.size(); ++point) {
            const auto &entry = vertexProvenance_[point];
            for (const SelectionItem &item : items) {
                if (item.domain != SelectionDomain::Geometry || item.sourceRevision != revision_) {
                    continue;
                }
                if (item.kind == SelectionKind::Vertex && item.geometryEntityId == entry.vertexId
                    && (item.parentGeometryId == femcae::geometry::InvalidGeometryId
                        || item.parentGeometryId == entry.bodyId)) {
                    indices.push_back(point);
                    break;
                }
            }
        }
        return indices;
    }

private:
    quint64 revision_{0};
    GeometrySelectionScene surface_;
    std::vector<femcae::geometry::Vec3> edgePoints_;
    std::vector<std::array<std::uint32_t, 2>> edgeLines_;
    std::vector<GeometrySceneLineProvenance> edgeProvenance_;
    std::vector<femcae::geometry::Vec3> vertexPoints_;
    std::vector<GeometryScenePointProvenance> vertexProvenance_;
};

} // namespace d26
