#include "ConstraintView.h"

#include <algorithm>
#include <stdexcept>

namespace femcae::meshing::m6::state {
namespace {

bool validFace(const CanonicalFaceKey& face) noexcept {
    return face.vertices[0] != InvalidPointId &&
           face.vertices[1] != InvalidPointId &&
           face.vertices[2] != InvalidPointId &&
           face.vertices[0] != face.vertices[1] &&
           face.vertices[0] != face.vertices[2] &&
           face.vertices[1] != face.vertices[2];
}

CanonicalFaceKey normalizedFace(CanonicalFaceKey face) {
    if (!validFace(face)) {
        throw std::invalid_argument(
            "M6 protected face requires three distinct non-zero PointIds");
    }
    std::sort(face.vertices.begin(), face.vertices.end());
    return face;
}

} // namespace

ProtectedEdgeKey canonicalProtectedEdgeKey(
    PointId a,
    PointId b) {
    if (a == InvalidPointId ||
        b == InvalidPointId ||
        a == b) {
        throw std::invalid_argument(
            "M6 protected edge requires two distinct non-zero PointIds");
    }

    ProtectedEdgeKey result{{a, b}};
    if (result.vertices[1] < result.vertices[0]) {
        std::swap(result.vertices[0], result.vertices[1]);
    }
    return result;
}

ConstraintView::ConstraintView(
    std::vector<PointMobilityEntry> pointMobility,
    std::vector<ProtectedEdgeKey> protectedEdges,
    std::vector<CanonicalFaceKey> protectedFaces)
    : pointMobility_(std::move(pointMobility)),
      protectedEdges_(std::move(protectedEdges)),
      protectedFaces_(std::move(protectedFaces)) {
    for (const PointMobilityEntry& entry : pointMobility_) {
        if (entry.point == InvalidPointId) {
            throw std::invalid_argument(
                "M6 point mobility requires non-zero PointId");
        }
    }

    std::sort(
        pointMobility_.begin(),
        pointMobility_.end(),
        [](const PointMobilityEntry& lhs, const PointMobilityEntry& rhs) {
            return lhs.point < rhs.point;
        });
    if (std::adjacent_find(
            pointMobility_.begin(),
            pointMobility_.end(),
            [](const PointMobilityEntry& lhs, const PointMobilityEntry& rhs) {
                return lhs.point == rhs.point;
            }) != pointMobility_.end()) {
        throw std::invalid_argument(
            "M6 point mobility contains duplicate PointId");
    }

    for (ProtectedEdgeKey& edge : protectedEdges_) {
        edge = canonicalProtectedEdgeKey(
            edge.vertices[0],
            edge.vertices[1]);
    }
    std::sort(protectedEdges_.begin(), protectedEdges_.end());
    protectedEdges_.erase(
        std::unique(protectedEdges_.begin(), protectedEdges_.end()),
        protectedEdges_.end());

    for (CanonicalFaceKey& face : protectedFaces_) {
        face = normalizedFace(face);
    }
    std::sort(protectedFaces_.begin(), protectedFaces_.end());
    protectedFaces_.erase(
        std::unique(protectedFaces_.begin(), protectedFaces_.end()),
        protectedFaces_.end());
}

std::optional<PointMobility> ConstraintView::mobility(
    PointId point) const noexcept {
    const auto it = std::lower_bound(
        pointMobility_.begin(),
        pointMobility_.end(),
        point,
        [](const PointMobilityEntry& entry, PointId value) {
            return entry.point < value;
        });

    if (it == pointMobility_.end() ||
        it->point != point) {
        return std::nullopt;
    }
    return it->mobility;
}

bool ConstraintView::isInteriorFree(
    PointId point) const noexcept {
    const auto value = mobility(point);
    return value.has_value() &&
           *value == PointMobility::InteriorFree;
}

bool ConstraintView::isFixed(
    PointId point) const noexcept {
    const auto value = mobility(point);
    return value.has_value() &&
           *value == PointMobility::Fixed;
}

bool ConstraintView::pointTouchesProtectedTopology(
    PointId point) const noexcept {
    for (const ProtectedEdgeKey& edge : protectedEdges_) {
        if (edge.vertices[0] == point ||
            edge.vertices[1] == point) {
            return true;
        }
    }

    for (const CanonicalFaceKey& face : protectedFaces_) {
        if (std::find(
                face.vertices.begin(),
                face.vertices.end(),
                point) != face.vertices.end()) {
            return true;
        }
    }
    return false;
}

bool ConstraintView::isProtected(
    const ProtectedEdgeKey& edge) const noexcept {
    return std::binary_search(
        protectedEdges_.begin(),
        protectedEdges_.end(),
        edge);
}

bool ConstraintView::isProtected(
    const CanonicalFaceKey& face) const noexcept {
    return std::binary_search(
        protectedFaces_.begin(),
        protectedFaces_.end(),
        face);
}

} // namespace femcae::meshing::m6::state
