#include "MeanRatioExact.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>
#include <vector>

namespace femcae::meshing::m6::quality {
namespace {

using femcae::meshing::internal::exact::BigInt;
using IntegerPoint = std::array<BigInt, 3>;
using IntegerTetra = std::array<IntegerPoint, 4>;

BigInt subtract(const BigInt& lhs, const BigInt& rhs) {
    return lhs + rhs.negated();
}

IntegerPoint difference(const IntegerPoint& lhs, const IntegerPoint& rhs) {
    return {
        subtract(lhs[0], rhs[0]),
        subtract(lhs[1], rhs[1]),
        subtract(lhs[2], rhs[2])};
}

BigInt determinant3Rows(
    const IntegerPoint& r0,
    const IntegerPoint& r1,
    const IntegerPoint& r2) {
    const BigInt c00 = subtract(r1[1] * r2[2], r1[2] * r2[1]);
    const BigInt c01 = subtract(r1[0] * r2[2], r1[2] * r2[0]);
    const BigInt c02 = subtract(r1[0] * r2[1], r1[1] * r2[0]);

    BigInt result = r0[0] * c00;
    result = result + (r0[1] * c01).negated();
    result = result + r0[2] * c02;
    return result;
}

IntegerTetra integerTetra(const TetraCoordinates& tetra) {
    std::vector<double> coordinates;
    coordinates.reserve(12U);
    for (const geometry::Vec3& point : tetra) {
        coordinates.push_back(point.x);
        coordinates.push_back(point.y);
        coordinates.push_back(point.z);
    }

    const std::vector<BigInt> exact =
        femcae::meshing::internal::exact::exactIntegerCoordinates(coordinates);

    IntegerTetra result;
    for (std::size_t vertex = 0U; vertex < 4U; ++vertex) {
        for (std::size_t component = 0U; component < 3U; ++component) {
            result[vertex][component] = exact[3U * vertex + component];
        }
    }
    return result;
}

BigInt signedOrientDeterminant(const IntegerTetra& tetra) {
    const IntegerPoint e01 = difference(tetra[1], tetra[0]);
    const IntegerPoint e02 = difference(tetra[2], tetra[0]);
    const IntegerPoint e03 = difference(tetra[3], tetra[0]);

    // M1 Orient3D uses the homogeneous row determinant
    // [x y z 1]. After subtracting row 0 from rows 1..3 and expanding
    // along the final column, its sign is the negative of the relative
    // 3x3 row determinant.
    return determinant3Rows(e01, e02, e03).negated();
}

BigInt edgeSquared(const IntegerPoint& edge) {
    BigInt value = edge[0] * edge[0];
    value = value + edge[1] * edge[1];
    value = value + edge[2] * edge[2];
    return value;
}

BigInt edgeSum(const IntegerTetra& tetra) {
    BigInt sum;
    for (std::size_t i = 0U; i < 4U; ++i) {
        for (std::size_t j = i + 1U; j < 4U; ++j) {
            sum = sum + edgeSquared(difference(tetra[j], tetra[i]));
        }
    }
    return sum;
}

void updateKeyTelemetry(
    const ExactMeanRatioKey& key,
    ExactMeanRatioTelemetry* telemetry) noexcept {
    if (telemetry == nullptr) {
        return;
    }
    telemetry->maxDeterminantBits = std::max(
        telemetry->maxDeterminantBits,
        key.determinantMagnitude.bitLength());
    telemetry->maxEdgeSumBits = std::max(
        telemetry->maxEdgeSumBits,
        key.edgeSum.bitLength());
}

struct PoweredKey {
    BigInt determinantSquared;
    BigInt edgeSumCubed;
};

PoweredKey powerKey(
    const ExactMeanRatioKey& key,
    ExactMeanRatioTelemetry* telemetry) {
    const BigInt determinantSquared =
        key.determinantMagnitude * key.determinantMagnitude;
    const BigInt edgeSumSquared = key.edgeSum * key.edgeSum;
    const BigInt edgeSumCubed = edgeSumSquared * key.edgeSum;

    if (telemetry != nullptr) {
        telemetry->maxPowerBits = std::max({
            telemetry->maxPowerBits,
            determinantSquared.bitLength(),
            edgeSumCubed.bitLength()});
    }

    return {determinantSquared, edgeSumCubed};
}

MeanRatioOrder orderFromSign(int sign) noexcept {
    if (sign < 0) {
        return MeanRatioOrder::Less;
    }
    if (sign > 0) {
        return MeanRatioOrder::Greater;
    }
    return MeanRatioOrder::Equal;
}

} // namespace

ExactMeanRatioKey buildExactMeanRatioKey(
    const TetraCoordinates& tetra,
    ExactMeanRatioTelemetry* telemetry) {
    const IntegerTetra exact = integerTetra(tetra);
    BigInt determinant = signedOrientDeterminant(exact);
    if (determinant.sign() == 0) {
        throw std::invalid_argument(
            "D26QMR1 requires a non-degenerate tetrahedron");
    }
    if (determinant.sign() < 0) {
        determinant = determinant.negated();
    }

    BigInt sum = edgeSum(exact);
    if (sum.sign() <= 0) {
        throw std::invalid_argument(
            "D26QMR1 requires a positive squared-edge sum");
    }

    ExactMeanRatioKey key{std::move(determinant), std::move(sum)};
    updateKeyTelemetry(key, telemetry);
    return key;
}

int exactRelativeDeterminantSign(const TetraCoordinates& tetra) {
    return signedOrientDeterminant(integerTetra(tetra)).sign();
}

MeanRatioOrder compareExactMeanRatioKeys(
    const ExactMeanRatioKey& lhs,
    const ExactMeanRatioKey& rhs,
    ExactMeanRatioTelemetry* telemetry) {
    const PoweredKey lhsPower = powerKey(lhs, telemetry);
    const PoweredKey rhsPower = powerKey(rhs, telemetry);

    const BigInt leftCross =
        lhsPower.determinantSquared * rhsPower.edgeSumCubed;
    const BigInt rightCross =
        rhsPower.determinantSquared * lhsPower.edgeSumCubed;

    if (telemetry != nullptr) {
        telemetry->maxCrossBits = std::max({
            telemetry->maxCrossBits,
            leftCross.bitLength(),
            rightCross.bitLength()});
    }

    const BigInt delta = leftCross + rightCross.negated();
    const MeanRatioOrder result = orderFromSign(delta.sign());
    if (result == MeanRatioOrder::Equal && telemetry != nullptr) {
        ++telemetry->exactEqual;
    }
    return result;
}

MeanRatioOrder compareExactMeanRatio(
    const TetraCoordinates& lhs,
    const TetraCoordinates& rhs,
    ExactMeanRatioTelemetry* telemetry) {
    if (telemetry != nullptr) {
        ++telemetry->calls;
    }

    try {
        const ExactMeanRatioKey lhsKey =
            buildExactMeanRatioKey(lhs, telemetry);
        const ExactMeanRatioKey rhsKey =
            buildExactMeanRatioKey(rhs, telemetry);
        return compareExactMeanRatioKeys(lhsKey, rhsKey, telemetry);
    } catch (const std::invalid_argument&) {
        if (telemetry != nullptr) {
            ++telemetry->invalidInput;
        }
        throw;
    }
}

} // namespace femcae::meshing::m6::quality
