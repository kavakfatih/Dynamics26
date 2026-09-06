#pragma once

#include "../../internal/exact/ExactDyadicArithmetic.h"

#include "femcae/geometry/GeometryTypes.h"
#include "femcae/meshing/RobustGeometry.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace femcae::meshing::m6::quality {

using TetraCoordinates = std::array<geometry::Vec3, 4>;

// D26QMR1/D26QMRF1/D26QV1 ortak private tetra girisi.
// PointId kalite scalar'inin parcasi degildir; identity/replay zincirini tasir.
struct IndexedTetraCoordinates {
    std::array<PointId, 4> pointIds{};
    TetraCoordinates coordinates{};
};

enum class MeanRatioOrder : std::int8_t {
    Less = -1,
    Equal = 0,
    Greater = 1
};

struct ExactMeanRatioTelemetry {
    std::uint64_t calls{0};
    std::uint64_t exactEqual{0};
    std::uint64_t invalidInput{0};
    std::size_t maxDeterminantBits{0};
    std::size_t maxEdgeSumBits{0};
    std::size_t maxPowerBits{0};
    std::size_t maxCrossBits{0};
};

// Build through buildExactMeanRatioKey only. The comparator reads D^2 and S^3
// directly, so a hand-assembled key with empty power fields compares as zero.
//
// The powers are cached rather than recomputed per comparison because a key is
// compared O(log n) times inside a D26QV1 sort but derived once: caching turns a
// comparison from six big multiplications into two.
struct ExactMeanRatioKey {
    femcae::meshing::internal::exact::BigInt determinantMagnitude;
    femcae::meshing::internal::exact::BigInt edgeSum;
    femcae::meshing::internal::exact::BigInt determinantSquared;
    femcae::meshing::internal::exact::BigInt edgeSumCubed;
};

[[nodiscard]] ExactMeanRatioKey buildExactMeanRatioKey(
    const TetraCoordinates& tetra,
    ExactMeanRatioTelemetry* telemetry = nullptr);

[[nodiscard]] int exactRelativeDeterminantSign(
    const TetraCoordinates& tetra);

[[nodiscard]] MeanRatioOrder compareExactMeanRatioKeys(
    const ExactMeanRatioKey& lhs,
    const ExactMeanRatioKey& rhs,
    ExactMeanRatioTelemetry* telemetry = nullptr);

[[nodiscard]] MeanRatioOrder compareExactMeanRatio(
    const TetraCoordinates& lhs,
    const TetraCoordinates& rhs,
    ExactMeanRatioTelemetry* telemetry = nullptr);

} // namespace femcae::meshing::m6::quality
