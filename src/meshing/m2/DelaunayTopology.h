#pragma once

#include "femcae/meshing/RobustGeometry.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace femcae::meshing::m2 {

struct InfiniteVertexTag {
    friend bool operator==(const InfiniteVertexTag&, const InfiniteVertexTag&) = default;
};

// Infinite ayri bir topolojik kategoridir. Bir PointId sentinel'i veya koordinat
// degildir; geometric predicate ve symbolic priority yalniz Finite(PointId) alir.
class DelaunayVertexRef {
public:
    DelaunayVertexRef() = default;

    [[nodiscard]] static DelaunayVertexRef finite(PointId id);
    [[nodiscard]] static DelaunayVertexRef infinite() noexcept;

    [[nodiscard]] bool isFinite() const noexcept;
    [[nodiscard]] bool isInfinite() const noexcept;
    [[nodiscard]] std::optional<PointId> finitePointId() const noexcept;

    friend bool operator==(const DelaunayVertexRef&, const DelaunayVertexRef&) = default;
    friend bool operator<(const DelaunayVertexRef& lhs, const DelaunayVertexRef& rhs) noexcept;

private:
    std::variant<InfiniteVertexTag, PointId> storage_{InfiniteVertexTag{}};
};

struct DelaunayCellHandle {
    std::uint32_t slot{std::numeric_limits<std::uint32_t>::max()};
    std::uint32_t generation{0};

    [[nodiscard]] bool isValid() const noexcept {
        return slot != std::numeric_limits<std::uint32_t>::max() && generation != 0U;
    }

    friend bool operator==(const DelaunayCellHandle&, const DelaunayCellHandle&) = default;
};

inline constexpr DelaunayCellHandle InvalidDelaunayCellHandle{};

struct DelaunayCellRecord {
    std::array<DelaunayVertexRef, 4> vertices{};
    std::array<DelaunayCellHandle, 4> neighbors{
        InvalidDelaunayCellHandle,
        InvalidDelaunayCellHandle,
        InvalidDelaunayCellHandle,
        InvalidDelaunayCellHandle};
    std::uint64_t visitEpoch{0};
};

struct DelaunayCellSlot {
    std::uint32_t generation{1};
    bool live{false};
    DelaunayCellRecord record;
};

struct DelaunayFaceKey {
    std::array<DelaunayVertexRef, 3> vertices{};

    friend bool operator==(const DelaunayFaceKey&, const DelaunayFaceKey&) = default;
    friend bool operator<(const DelaunayFaceKey& lhs, const DelaunayFaceKey& rhs) noexcept;
};

enum class DelaunayTopologyIssueCode : std::uint8_t {
    LiveSlotZeroGeneration,
    InvalidCellVertexPattern,
    DuplicateFiniteVertex,
    MissingFinitePoint,
    NonPositiveFiniteCell,
    InvalidNeighborHandle,
    NeighborOutOfRange,
    NeighborDead,
    StaleNeighborGeneration,
    NeighborFaceMismatch,
    NonReciprocalNeighbor,
    FaceIncidenceNotTwo,
    GhostFiniteNeighborMissing,
    GhostLateralNeighborNotGhost,
    GhostHullOrientationInvalid,
    EulerCharacteristicMismatch
};

struct DelaunayTopologyIssue {
    DelaunayTopologyIssueCode code{DelaunayTopologyIssueCode::InvalidCellVertexPattern};
    DelaunayCellHandle cell;
    std::uint8_t localFace{0};
};

struct DelaunayTopologyValidationReport {
    std::vector<DelaunayTopologyIssue> issues;

    [[nodiscard]] bool ok() const noexcept {
        return issues.empty();
    }
};

struct DelaunayComplexStats {
    std::size_t vertices{0};
    std::size_t edges{0};
    std::size_t faces{0};
    std::size_t cells{0};
    std::size_t finiteCells{0};
    std::size_t ghostCells{0};
    std::int64_t eulerCharacteristic{0};
};

// M2.1 reference storage: append-only, generation-checked, slot reuse yok.
class DelaunayReferenceArena {
public:
    void reserve(std::size_t capacity);
    [[nodiscard]] DelaunayCellHandle appendCell(const DelaunayCellRecord& record);

    [[nodiscard]] const DelaunayCellRecord& cell(DelaunayCellHandle handle) const;
    [[nodiscard]] std::span<const DelaunayCellSlot> slots() const noexcept;
    [[nodiscard]] std::size_t liveCount() const noexcept;

private:
    friend struct DelaunayBootstrapBuilderAccess;

    [[nodiscard]] DelaunayCellRecord& mutableCell(DelaunayCellHandle handle);

    std::vector<DelaunayCellSlot> slots_;
    std::size_t liveCount_{0};
};

enum class DelaunayBootstrapStatus : std::uint8_t {
    Ready = 0,
    LowerDimensional = 1
};

struct DelaunayBootstrapResult {
    DelaunayBootstrapStatus status{DelaunayBootstrapStatus::LowerDimensional};
    AffineDimension affineDimension{AffineDimension::Empty};
    std::array<PointId, 4> basisPointIds{
        InvalidPointId, InvalidPointId, InvalidPointId, InvalidPointId};
    DelaunayReferenceArena arena;
};

[[nodiscard]] DelaunayFaceKey canonicalDelaunayFaceKey(
    const DelaunayCellRecord& cell,
    std::size_t oppositeVertex);

[[nodiscard]] std::array<PointId, 3> outwardFiniteFace(
    const DelaunayCellRecord& positiveFiniteCell,
    std::size_t oppositeVertex);

[[nodiscard]] DelaunayComplexStats computeDelaunayComplexStats(
    std::span<const DelaunayCellSlot> slots);

[[nodiscard]] DelaunayTopologyValidationReport validateDelaunayTopology(
    std::span<const DelaunayCellSlot> slots,
    std::span<const CanonicalSite> sites);

[[nodiscard]] DelaunayBootstrapResult buildDelaunayBootstrap(
    std::span<const CanonicalSite> sites);

} // namespace femcae::meshing::m2
