#pragma once

#include "IntervalArithmetic.h"
#include "MeanRatioExact.h"

#include <cstdint>

namespace femcae::meshing::m6::quality {

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

// Certification over two prebuilt interval keys, for callers that already hold a
// per-cell key and must not re-derive it per comparison.
//
// Each key carries its own normalization exponent. That stays sound because the
// frozen polynomial is homogeneous of the same degree on both sides: D is degree
// 3 and S degree 2 in the relative coordinates, so D^2 S^3 carries 2^(6e) for its
// own cell and the cross difference carries the common positive factor
// 2^(6e_A) * 2^(6e_B), which cannot change the sign.
//
// Returns Ready only when the interval proof is strict; `order` is written only
// then, and is never Equal. Every other status means the caller must ask
// D26QMRB1.
[[nodiscard]] MeanRatioIntervalStatus compareMeanRatioIntervalKeys(
    const MeanRatioIntervalKey& lhs,
    const MeanRatioIntervalKey& rhs,
    MeanRatioOrder& order) noexcept;

// Final semantic comparator: filter yalniz strict Less/Greater kanitlayabilir.
// Equal dahil tum belirsizlikler D26QMRB1 exact comparator'a gider.
[[nodiscard]] MeanRatioEvaluation compareFilteredMeanRatio(
    const IndexedTetraCoordinates& lhs,
    const IndexedTetraCoordinates& rhs,
    MeanRatioFilterTelemetry* telemetry = nullptr,
    ExactMeanRatioTelemetry* exactTelemetry = nullptr);

} // namespace femcae::meshing::m6::quality
