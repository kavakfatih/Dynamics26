#pragma once

// Dynamics26 Alpha.3.2 — CAD display scene ve selection provenance tablosu.
//
// Birden fazla Body'nin TopologyTessellation verisini tek display sahnesinde
// toplar. Her display cell icin ayri Body/Face kimligi saklanir. Bu sinif VTK
// bilmez; vtkCellPicker yalniz cell index uretir, CAD kimligi burada cozulur.
//
// CAD Geometry != Display Tessellation != FEM Mesh kuralini korur.

#include "../core/SelectionTypes.h"

#include <femcae/geometry/GeometryTypes.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace d26 {

struct GeometrySceneCellProvenance {
    femcae::geometry::GeometryEntityId bodyId{femcae::geometry::InvalidGeometryId};
    femcae::geometry::GeometryEntityId faceId{femcae::geometry::InvalidGeometryId};
};

class GeometrySelectionScene final
{
public:
    [[nodiscard]] bool append(const femcae::geometry::TopologyTessellation &body)
    {
        using femcae::geometry::InvalidGeometryId;

        if (!body.hasConsistentProvenance()
            || body.display.sourceGeometryId == InvalidGeometryId
            || body.display.triangles.empty()) {
            return false;
        }
        if (revision_ != 0 && body.display.sourceRevision != revision_) {
            return false;
        }
        if (body.display.points.size()
            > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) - points_.size()) {
            return false;
        }
        for (const auto faceId : body.triangleFaceIds) {
            if (faceId == InvalidGeometryId) {
                return false;
            }
        }

        const std::uint32_t base = static_cast<std::uint32_t>(points_.size());
        const std::size_t oldPointCount = points_.size();
        const std::size_t oldTriangleCount = triangles_.size();
        const std::size_t oldProvenanceCount = provenance_.size();

        points_.insert(points_.end(), body.display.points.begin(), body.display.points.end());
        triangles_.reserve(triangles_.size() + body.display.triangles.size());
        provenance_.reserve(provenance_.size() + body.display.triangles.size());

        for (std::size_t i = 0; i < body.display.triangles.size(); ++i) {
            const auto &triangle = body.display.triangles[i];
            // Bozuk tessellation index'i sahneye alinmaz. Append atomik davranir.
            if (triangle[0] >= body.display.points.size()
                || triangle[1] >= body.display.points.size()
                || triangle[2] >= body.display.points.size()) {
                points_.resize(oldPointCount);
                triangles_.resize(oldTriangleCount);
                provenance_.resize(oldProvenanceCount);
                return false;
            }
            triangles_.push_back({base + triangle[0], base + triangle[1], base + triangle[2]});
            provenance_.push_back({body.display.sourceGeometryId, body.triangleFaceIds[i]});
        }

        if (revision_ == 0) {
            revision_ = body.display.sourceRevision;
        }
        return true;
    }

    void clear()
    {
        revision_ = 0;
        points_.clear();
        triangles_.clear();
        provenance_.clear();
    }

    [[nodiscard]] quint64 sourceRevision() const noexcept { return revision_; }
    [[nodiscard]] bool empty() const noexcept { return triangles_.empty(); }
    [[nodiscard]] bool hasFaceProvenance() const noexcept
    {
        if (provenance_.empty()) {
            return false;
        }
        for (const auto &entry : provenance_) {
            if (entry.faceId == femcae::geometry::InvalidGeometryId) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] const std::vector<femcae::geometry::Vec3> &points() const noexcept { return points_; }
    [[nodiscard]] const std::vector<std::array<std::uint32_t, 3>> &triangles() const noexcept { return triangles_; }
    [[nodiscard]] const std::vector<GeometrySceneCellProvenance> &provenance() const noexcept { return provenance_; }

    [[nodiscard]] std::optional<GeometrySceneCellProvenance> provenanceForCell(const std::size_t cell) const
    {
        if (cell >= provenance_.size()) {
            return std::nullopt;
        }
        return provenance_[cell];
    }

    [[nodiscard]] std::optional<SelectionItem> selectionItemForCell(const std::size_t cell,
                                                                    const SelectionKind kind) const
    {
        const auto hit = provenanceForCell(cell);
        if (!hit.has_value()) {
            return std::nullopt;
        }

        SelectionItem item;
        item.domain = SelectionDomain::Geometry;
        item.sourceRevision = revision_;
        if (kind == SelectionKind::Body) {
            item.kind = SelectionKind::Body;
            item.geometryEntityId = hit->bodyId;
            return item;
        }
        if (kind == SelectionKind::Face) {
            item.kind = SelectionKind::Face;
            item.geometryEntityId = hit->faceId;
            item.parentGeometryId = hit->bodyId;
            return item;
        }
        return std::nullopt;
    }

private:
    quint64 revision_{0};
    std::vector<femcae::geometry::Vec3> points_;
    std::vector<std::array<std::uint32_t, 3>> triangles_;
    std::vector<GeometrySceneCellProvenance> provenance_;
};

} // namespace d26
