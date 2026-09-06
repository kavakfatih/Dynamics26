#include "ConstraintView.h"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <utility>

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

// Query-side canonicalization. Stored keys are canonicalized by the constructor,
// so a lookup must canonicalize too: an operation planner holding a face in
// mesh-local winding order or an edge in traversal order must not be told that a
// protected feature is unprotected. Invalid keys resolve to nullopt, which the
// callers report as "not protected" rather than "protected" -- an invalid key
// never names a stored feature.
std::optional<ProtectedEdgeKey> canonicalEdgeForQuery(
    const ProtectedEdgeKey& edge) noexcept {
    if (edge.vertices[0] == InvalidPointId ||
        edge.vertices[1] == InvalidPointId ||
        edge.vertices[0] == edge.vertices[1]) {
        return std::nullopt;
    }

    ProtectedEdgeKey result = edge;
    if (result.vertices[1] < result.vertices[0]) {
        std::swap(result.vertices[0], result.vertices[1]);
    }
    return result;
}

std::optional<CanonicalFaceKey> canonicalFaceForQuery(
    const CanonicalFaceKey& face) noexcept {
    if (!validFace(face)) {
        return std::nullopt;
    }

    CanonicalFaceKey result = face;
    std::sort(result.vertices.begin(), result.vertices.end());
    return result;
}

CanonicalFaceKey normalizedFace(const CanonicalFaceKey& face) {
    const std::optional<CanonicalFaceKey> canonical =
        canonicalFaceForQuery(face);
    if (!canonical.has_value()) {
        throw std::invalid_argument(
            "M6 protected face requires three distinct non-zero PointIds");
    }
    return *canonical;
}

} // namespace

ProtectedEdgeKey canonicalProtectedEdgeKey(
    PointId a,
    PointId b) {
    const std::optional<ProtectedEdgeKey> canonical =
        canonicalEdgeForQuery(ProtectedEdgeKey{{a, b}});
    if (!canonical.has_value()) {
        throw std::invalid_argument(
            "M6 protected edge requires two distinct non-zero PointIds");
    }
    return *canonical;
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
    const std::optional<ProtectedEdgeKey> canonical =
        canonicalEdgeForQuery(edge);
    if (!canonical.has_value()) {
        return false;
    }
    return std::binary_search(
        protectedEdges_.begin(),
        protectedEdges_.end(),
        *canonical);
}

bool ConstraintView::isProtected(
    const CanonicalFaceKey& face) const noexcept {
    const std::optional<CanonicalFaceKey> canonical =
        canonicalFaceForQuery(face);
    if (!canonical.has_value()) {
        return false;
    }
    return std::binary_search(
        protectedFaces_.begin(),
        protectedFaces_.end(),
        *canonical);
}

} // namespace femcae::meshing::m6::state
