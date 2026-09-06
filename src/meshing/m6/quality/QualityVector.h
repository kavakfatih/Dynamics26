#pragma once

#include "MeanRatioExact.h"

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
