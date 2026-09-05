#pragma once

#include "femcae/meshing/RobustGeometry.h"

#include <array>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace femcae::meshing {

struct TetHandle {
    std::uint32_t slot{std::numeric_limits<std::uint32_t>::max()};
    std::uint32_t generation{0};

    [[nodiscard]] bool isValid() const noexcept {
        return slot != std::numeric_limits<std::uint32_t>::max() && generation != 0U;
    }

    friend bool operator==(const TetHandle&, const TetHandle&) = default;
};

inline constexpr TetHandle InvalidTetHandle{};

struct TetRecord {
    std::array<PointId, 4> vertices{
        InvalidPointId, InvalidPointId, InvalidPointId, InvalidPointId};
    std::array<TetHandle, 4> neighbors{
        InvalidTetHandle, InvalidTetHandle, InvalidTetHandle, InvalidTetHandle};
    std::uint64_t visitEpoch{0};
};

struct TetSlot {
    std::uint32_t generation{1};
    bool live{false};
    TetRecord record;
};

struct CanonicalFaceKey {
    std::array<PointId, 3> vertices{
        InvalidPointId, InvalidPointId, InvalidPointId};

    friend bool operator==(const CanonicalFaceKey&, const CanonicalFaceKey&) = default;
    friend bool operator<(const CanonicalFaceKey& lhs, const CanonicalFaceKey& rhs) noexcept {
        return lhs.vertices < rhs.vertices;
    }
};

enum class TopologyIssueCode : std::uint8_t {
    LiveSlotZeroGeneration,
    InvalidVertex,
    DuplicateVertex,
    DuplicateTetrahedron,
    InvalidNeighborHandle,
    NeighborOutOfRange,
    NeighborDead,
    StaleNeighborGeneration,
    NeighborFaceMismatch,
    NonReciprocalNeighbor,
    MissingNeighborForSharedFace,
    NonManifoldFace
};

struct TopologyIssue {
    TopologyIssueCode code{TopologyIssueCode::InvalidVertex};
    TetHandle tetra;
    std::uint8_t localFace{0};
};

struct TopologyValidationReport {
    std::vector<TopologyIssue> issues;

    [[nodiscard]] bool ok() const noexcept {
        return issues.empty();
    }
};

[[nodiscard]] CanonicalFaceKey canonicalFaceKey(
    const TetRecord& tet,
    std::size_t oppositeVertex);

[[nodiscard]] TopologyValidationReport validateTetTopology(
    std::span<const TetSlot> slots);

} // namespace femcae::meshing
