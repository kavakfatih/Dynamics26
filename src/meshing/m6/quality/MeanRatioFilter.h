#pragma once

#include "IntervalArithmetic.h"
#include "MeanRatioExact.h"

#include "femcae/meshing/RobustGeometry.h"

#include <array>
#include <cstdint>

namespace femcae::meshing::m6::quality {

// D26QMRF1 private filter girisi. pointIds siralama/replay authority'sidir;
// coordinates ise frozen binary64 geometry state'idir. Incoming tetra exact
// validity katmaninda Positive olarak dogrulanmis olmalidir.
struct IndexedTetraCoordinates {
    std::array<PointId, 4> pointIds{};
    TetraCoordinates coordinates{};
};

enum class MeanRatioEvaluationPath : std::uint8_t {
    IntervalCertified = 0,
    ExactFallback
};

enum class MeanRatioFallbackReason : std::uint8_t {
    None = 0,
    UnsupportedEnvironment,
    InvalidFilterInput,
    Range,
    IntervalOverlap
};

enum class MeanRatioIntervalStatus : std::uint8_t {
    Ready = 0,
    UnsupportedEnvironment,
    InvalidInput,
    Range,
    Overlap
};

struct MeanRatioIntervalKey {
    interval::Interval determinant{};
    interval::Interval edgeSum{};
    int normalizationExponent{0};
};

struct MeanRatioPairIntervals {
    MeanRatioIntervalKey lhs{};
    MeanRatioIntervalKey rhs{};
    interval::Interval crossPolynomial{};
};

struct MeanRatioEvaluation {
    MeanRatioOrder order{MeanRatioOrder::Equal};
    MeanRatioEvaluationPath path{MeanRatioEvaluationPath::ExactFallback};
    MeanRatioFallbackReason fallbackReason{MeanRatioFallbackReason::None};
};

struct MeanRatioFilterTelemetry {
    std::uint64_t calls{0};
    std::uint64_t intervalCertifiedLess{0};
    std::uint64_t intervalCertifiedGreater{0};
    std::uint64_t uncertainEnvironment{0};
    std::uint64_t uncertainInvalidInput{0};
    std::uint64_t uncertainRange{0};
    std::uint64_t uncertainOverlap{0};
    std::uint64_t exactFallback{0};
    std::uint64_t exactEqual{0};
};

// Diagnostics functions private M6 qualification/replay evidence icindir.
// D26INT1 ortam guard'i uygun degilse authoritative cevap uretmezler.
[[nodiscard]] MeanRatioIntervalStatus buildMeanRatioIntervalKey(
    const IndexedTetraCoordinates& tetra,
    MeanRatioIntervalKey& output) noexcept;

[[nodiscard]] MeanRatioIntervalStatus buildMeanRatioPairIntervals(
    const IndexedTetraCoordinates& lhs,
    const IndexedTetraCoordinates& rhs,
    MeanRatioPairIntervals& output) noexcept;

// Final semantic comparator: filter yalniz strict Less/Greater kanitlayabilir.
// Equal dahil tum belirsizlikler D26QMRB1 exact comparator'a gider.
[[nodiscard]] MeanRatioEvaluation compareFilteredMeanRatio(
    const IndexedTetraCoordinates& lhs,
    const IndexedTetraCoordinates& rhs,
    MeanRatioFilterTelemetry* telemetry = nullptr,
    ExactMeanRatioTelemetry* exactTelemetry = nullptr);

} // namespace femcae::meshing::m6::quality
