#pragma once

#include "DelaunayLocation.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace femcae::meshing::m2 {

enum class DelaunayWalkStatus : std::uint8_t {
    Success = 0,
    InvalidInput,
    InvalidTopology,
    InvalidStartHint,
    WalkStalled
};

enum class DelaunayConflictSeedSemantic : std::uint8_t {
    CellInterior = 0,
    FacetIncident,
    EdgeIncident,
    OutsideGhost
};

struct DelaunayConflictSeed {
    DelaunayCellHandle cell;
    std::array<DelaunayVertexRef, 4> canonicalCell;
    DelaunayConflictSeedSemantic semantic{
        DelaunayConflictSeedSemantic::CellInterior};
    std::optional<DelaunayFaceKey> witnessFacet;
};

struct DelaunayWalkTrace {
    std::optional<LocatedCellEvidence> startCell;
    std::vector<LocatedCellEvidence> visitedCells;
    std::vector<DelaunayFaceKey> crossedFacets;
    std::size_t stepGuard{0};
};

struct DelaunayWalkTelemetry {
    std::uint64_t locateCalls{0};
    std::uint64_t walkSuccesses{0};
    std::uint64_t walkStalls{0};
    std::uint64_t walkSteps{0};
    std::uint64_t maxWalkSteps{0};
    std::uint64_t diagnosticBruteForceCalls{0};
};

struct DelaunayWalkResult {
    DelaunayWalkStatus status{DelaunayWalkStatus::InvalidInput};
    std::optional<DelaunayLocationResult> location;
    std::optional<DelaunayConflictSeed> conflictSeed;
    // Yalniz WalkStalled diagnosis'inda dolar. Walk kararlarini asla yonetmez.
    std::optional<DelaunayLocationResult> diagnosticOracle;
    DelaunayWalkTrace trace;
    std::string detail;

    [[nodiscard]] bool ok() const noexcept {
        return status == DelaunayWalkStatus::Success &&
               location.has_value();
    }
};

// P1E correctness-first adjacency visibility walk.
// - default baslangic query'den bagimsiz canonical finite-cell identity'dir,
// - yalniz exact outward Orient3D Positive facetler gecilir,
// - exact-zero facet crossing direction degildir,
// - birden cok violated facet varsa canonical face key minimumu secilir,
// - brute-force oracle yalniz explicit WalkStalled diagnosis'inda cagrilir.
// Optional hint yalniz traversal optimization hint'idir; stale/ghost hint typed
// failure verir ve gizli fallback yapilmaz.
[[nodiscard]] DelaunayWalkResult locateDeterministicWalk(
    std::span<const DelaunayCellSlot> slots,
    std::span<const CanonicalSite> sites,
    const geometry::Vec3& query,
    std::optional<DelaunayCellHandle> startHint = std::nullopt,
    DelaunayWalkTelemetry* telemetry = nullptr);

namespace detail {

// Sadece qualification negative-control'u icin. Normal production guard,
// live finite-cell count'undan turetilir.
void setDelaunayWalkStepGuardOverrideForQualification(
    std::optional<std::size_t> guard) noexcept;

} // namespace detail

} // namespace femcae::meshing::m2
