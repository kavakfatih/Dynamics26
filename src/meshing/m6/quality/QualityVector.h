#pragma once

#include "MeanRatioExact.h"
#include "MeanRatioFilter.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace femcae::meshing::m6::quality {

using CanonicalQualityTetKey = std::array<PointId, 4>;

struct QualityVectorEntry {
    CanonicalQualityTetKey tetra{};
    ExactMeanRatioKey quality{};

    // D26QMRF1 certificate material, built once per cell alongside the exact key.
    // `quality` stays the sole authority: `filter` may only prove a strict order
    // that D26QMRB1 would also reach, and proves nothing when `filterReady` is
    // false. Entry order and acceptance therefore do not depend on whether the
    // filter happened to certify.
    MeanRatioIntervalKey filter{};
    bool filterReady{false};
};

// D26QV1 irrasyonel qMR scalar'i saklamaz.
// Entries ascending exact quality order'inda tutulur: index 0 en kotu tetra'dir.
struct QualityVector {
    std::vector<QualityVectorEntry> entries;
};

enum class QualityVectorOrder : std::int8_t {
    Worse = -1,
    Equal = 0,
    Better = 1
};

struct QualityVectorTelemetry {
    std::uint64_t builds{0};
    std::uint64_t vectorComparisons{0};
    std::uint64_t entryComparisons{0};
    std::uint64_t exactQualityTies{0};
    std::uint64_t prefixDecisions{0};
    std::uint64_t mergeCalls{0};
    std::uint64_t mergedEntries{0};

    // Backend split over entryComparisons. These always sum to entryComparisons,
    // so a qualification run can show how much of the exact backend the filter
    // actually displaced.
    std::uint64_t intervalCertifiedComparisons{0};
    std::uint64_t exactComparisons{0};
    std::uint64_t filterUnavailableCells{0};
};

[[nodiscard]] bool isExactPositiveQualityCell(
    const IndexedTetraCoordinates& tetra) noexcept;

[[nodiscard]] CanonicalQualityTetKey canonicalQualityTetKey(
    const IndexedTetraCoordinates& tetra);

[[nodiscard]] QualityVector buildQualityVector(
    std::span<const IndexedTetraCoordinates> tetrahedra,
    QualityVectorTelemetry* telemetry = nullptr,
    ExactMeanRatioTelemetry* exactTelemetry = nullptr);

[[nodiscard]] QualityVectorOrder compareQualityVectors(
    const QualityVector& lhs,
    const QualityVector& rhs,
    QualityVectorTelemetry* telemetry = nullptr,
    ExactMeanRatioTelemetry* exactTelemetry = nullptr);

[[nodiscard]] QualityVector mergeQualityVectors(
    const QualityVector& lhs,
    const QualityVector& rhs,
    QualityVectorTelemetry* telemetry = nullptr,
    ExactMeanRatioTelemetry* exactTelemetry = nullptr);

} // namespace femcae::meshing::m6::quality
