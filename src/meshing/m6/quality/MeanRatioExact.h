#pragma once

#include "../../internal/exact/ExactDyadicArithmetic.h"

#include "femcae/geometry/GeometryTypes.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace femcae::meshing::m6::quality {

using TetraCoordinates = std::array<geometry::Vec3, 4>;

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

struct ExactMeanRatioKey {
    femcae::meshing::internal::exact::BigInt determinantMagnitude;
    femcae::meshing::internal::exact::BigInt edgeSum;
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
