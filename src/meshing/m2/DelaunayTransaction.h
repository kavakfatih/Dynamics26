#pragma once

#include "DelaunayTopology.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace femcae::meshing::m2 {

enum class DelaunayTransactionFailure : std::uint8_t {
    None = 0,
    InvalidQuery,
    DuplicateLiveSite,
    InvalidTopology,
    EmptyConflict,
    VerifiedSeedNotConflict,
    DisconnectedCavity,
    InvalidFaceMultiplicity,
    InvalidBoundaryManifold,
    DegenerateCandidate,
    InvalidGhostOrientation,
    InvalidStitching,
    DuplicateCandidate,
    InvalidCandidateTopology,
    StalePlan,
    CapacityOverflow,
    ResourceLimit,
    ResourceFailure,
    NotReserved,
    TopologyVersionExhausted
};

struct DelaunayInternalFacetRecord {
    DelaunayFaceKey key;
    DelaunayCellHandle firstCell;
    std::uint8_t firstLocalFace{0};
    DelaunayCellHandle secondCell;
    std::uint8_t secondLocalFace{0};
};

struct DelaunayBoundaryFacetRecord {
    DelaunayFaceKey key;
    // Inside owner'in local face sirasidir; identity icin key kullanilir.
    std::array<DelaunayVertexRef, 3> orientedVertices;
    DelaunayCellHandle insideCell;
    std::uint8_t insideLocalFace{0};
    DelaunayCellHandle outsideCell;
    std::uint8_t outsideLocalFace{0};
    bool finiteFacet{false};
};

struct DelaunayCandidateCell {
    DelaunayCellRecord record;
    DelaunayFaceKey baseFace;
    std::uint8_t baseLocalFace{0};
    DelaunayCellHandle outsideCell;
    std::uint8_t outsideLocalFace{0};
    DelaunayCellHandle futureHandle;
};

struct DelaunayExternalRewire {
    DelaunayCellHandle outsideCell;
    std::uint8_t outsideLocalFace{0};
    DelaunayCellHandle expectedOldNeighbor;
    DelaunayCellHandle newCell;
};

struct DelaunaySiteSnapshotEntry {
    PointId id{InvalidPointId};
    std::uint64_t xBits{0};
    std::uint64_t yBits{0};
    std::uint64_t zBits{0};

    friend bool operator==(
        const DelaunaySiteSnapshotEntry&,
        const DelaunaySiteSnapshotEntry&) = default;
};

struct DelaunayInsertionPlan {
    PointId queryId{InvalidPointId};
    geometry::Vec3 queryPoint;
    std::uint64_t sourceTopologyVersion{0};
    std::size_t sourceSlotCount{0};
    std::size_t sourceLiveCount{0};

    // Collision-free correctness authority: PointId-artan M1-canonical
    // binary64 coordinate bit snapshot. Hash/digest bunun yerine gecemez.
    std::vector<DelaunaySiteSnapshotEntry> sourceSiteSnapshot;

    // Iki ayri yolun canonical siralanmis snapshot handle sonuclari.
    std::vector<DelaunayCellHandle> conflictOracle;
    std::vector<DelaunayCellHandle> conflictFlood;
    // P1F nominal constructor path: P1E'nin exact-verified seed handle'i.
    // Standalone P1D path'te null kalir ve legacy canonical oracle seed'i kullanilir.
    std::optional<DelaunayCellHandle> verifiedConflictSeed;

    std::vector<DelaunayInternalFacetRecord> internalFacets;
    std::vector<DelaunayBoundaryFacetRecord> boundaryFacets;
    std::vector<DelaunayCandidateCell> candidateCells;
    std::vector<DelaunayExternalRewire> externalRewires;

    std::size_t requiredSlotCount{0};
    bool validated{false};
    bool reserved{false};
};

struct DelaunayPlanResult {
    DelaunayTransactionFailure failure{DelaunayTransactionFailure::None};
    std::string detail;
    std::optional<DelaunayInsertionPlan> plan;

    [[nodiscard]] bool ok() const noexcept {
        return failure == DelaunayTransactionFailure::None && plan.has_value();
    }
};

struct DelaunayTransactionResult {
    DelaunayTransactionFailure failure{DelaunayTransactionFailure::None};
    std::string detail;
    bool commitBarrierCrossed{false};

    [[nodiscard]] bool ok() const noexcept {
        return failure == DelaunayTransactionFailure::None;
    }
};

struct DelaunayResourceLimits {
    std::size_t maxTotalSlots{
        static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())};
};

namespace detail {

using DelaunayCommitBarrierAuditCallback = void (*)() noexcept;

struct DelaunayCommitBarrierAuditHooks {
    DelaunayCommitBarrierAuditCallback onBarrierCrossed{nullptr};
    DelaunayCommitBarrierAuditCallback onMechanicalCommitCompleted{nullptr};
};

// Private P1D qualification observer. Default hooks are null; production
// topology semantics and fingerprints do not depend on this state.
void setDelaunayCommitBarrierAuditHooks(
    DelaunayCommitBarrierAuditHooks hooks) noexcept;

struct DelaunayRequiredSlotCountCheck {
    DelaunayTransactionFailure failure{DelaunayTransactionFailure::None};
    std::size_t required{0};

    [[nodiscard]] bool ok() const noexcept {
        return failure == DelaunayTransactionFailure::None;
    }
};

// Pure checked arithmetic authority shared by production planning/validation
// and the G29 boundary fixtures.
[[nodiscard]] DelaunayRequiredSlotCountCheck checkedDelaunayRequiredSlots(
    std::size_t current,
    std::size_t additional) noexcept;

} // namespace detail

// P1D correctness-first plan:
// all-live-cell semantic conflict oracle -> exact-conflicting deterministic seed
// -> adjacency flood -> cavity extraction -> complete candidate patch -> validation.
// Arena mutation yoktur.
[[nodiscard]] DelaunayPlanResult buildDelaunayInsertionPlan(
    const DelaunayReferenceArena& arena,
    std::span<const CanonicalSite> sites,
    PointId queryId);

// P1F composition path: P1E tarafindan geometrik olarak dogrulanmis seed
// adjacency flood'u gercekten surer. P1D all-live exact conflict oracle
// bagimsiz correctness authority olarak kalir ve flood==oracle zorunludur.
[[nodiscard]] DelaunayPlanResult buildDelaunayInsertionPlanFromVerifiedSeed(
    const DelaunayReferenceArena& arena,
    std::span<const CanonicalSite> sites,
    PointId queryId,
    DelaunayCellHandle verifiedSeed);

// Untrusted/test-tampered plan dahil, commit barrier oncesi full structural +
// geometric candidate validation. Arena mutation yapmaz.
[[nodiscard]] DelaunayTransactionResult validateDelaunayInsertionPlan(
    const DelaunayReferenceArena& arena,
    std::span<const CanonicalSite> sites,
    const DelaunayInsertionPlan& plan);

// Checked count arithmetic + vector capacity reservation. Resource failure
// topological/geometric verdict degildir; live/dead/connectivity state degismez.
[[nodiscard]] DelaunayTransactionResult reserveDelaunayInsertion(
    DelaunayReferenceArena& arena,
    std::span<const CanonicalSite> sites,
    DelaunayInsertionPlan& plan,
    DelaunayResourceLimits limits = {});

// Plan -> Validate -> Reserve tamamlanmadan barrier gecilmez.
// Barrier sonrasi yalniz deterministic mekanik mutation vardir.
[[nodiscard]] DelaunayTransactionResult commitDelaunayInsertion(
    DelaunayReferenceArena& arena,
    std::span<const CanonicalSite> sites,
    DelaunayInsertionPlan& plan);

} // namespace femcae::meshing::m2
