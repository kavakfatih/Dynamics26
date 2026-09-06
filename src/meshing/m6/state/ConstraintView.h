#pragma once

#include "femcae/meshing/TetraTopology.h"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace femcae::meshing::m6::state {

enum class PointMobility : std::uint8_t {
    InteriorFree = 0,
    Fixed
};

struct PointMobilityEntry {
    PointId point{InvalidPointId};
    PointMobility mobility{PointMobility::Fixed};
};

struct ProtectedEdgeKey {
    std::array<PointId, 2> vertices{
        InvalidPointId, InvalidPointId};

    friend bool operator==(const ProtectedEdgeKey&, const ProtectedEdgeKey&) = default;
    friend bool operator<(const ProtectedEdgeKey& lhs, const ProtectedEdgeKey& rhs) noexcept {
        return lhs.vertices < rhs.vertices;
    }
};

[[nodiscard]] ProtectedEdgeKey canonicalProtectedEdgeKey(
    PointId a,
    PointId b);

class ConstraintView {
public:
    ConstraintView() = default;

    ConstraintView(
        std::vector<PointMobilityEntry> pointMobility,
        std::vector<ProtectedEdgeKey> protectedEdges = {},
        std::vector<CanonicalFaceKey> protectedFaces = {});

    [[nodiscard]] std::optional<PointMobility> mobility(
        PointId point) const noexcept;

    [[nodiscard]] bool isInteriorFree(PointId point) const noexcept;
    [[nodiscard]] bool isFixed(PointId point) const noexcept;

    [[nodiscard]] bool isProtected(
        const ProtectedEdgeKey& edge) const noexcept;
    [[nodiscard]] bool isProtected(
        const CanonicalFaceKey& face) const noexcept;

    [[nodiscard]] const std::vector<PointMobilityEntry>&
    pointMobilityEntries() const noexcept {
        return pointMobility_;
    }

    [[nodiscard]] const std::vector<ProtectedEdgeKey>&
    protectedEdges() const noexcept {
        return protectedEdges_;
    }

    [[nodiscard]] const std::vector<CanonicalFaceKey>&
    protectedFaces() const noexcept {
        return protectedFaces_;
    }

private:
    std::vector<PointMobilityEntry> pointMobility_;
    std::vector<ProtectedEdgeKey> protectedEdges_;
    std::vector<CanonicalFaceKey> protectedFaces_;
};

} // namespace femcae::meshing::m6::state
