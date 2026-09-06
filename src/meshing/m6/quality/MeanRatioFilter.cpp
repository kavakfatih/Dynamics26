#include "MeanRatioFilter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>

#ifdef __FAST_MATH__
#error "Dynamics26 D26QMRF1 cannot be compiled with fast-math semantics"
#endif

namespace femcae::meshing::m6::quality {
namespace {

using interval::Interval;
using interval::IntervalFailure;
using interval::IntervalResult;

using IntervalVec3 = std::array<Interval, 3>;
using RelativeTetra = std::array<IntervalVec3, 4>;

MeanRatioIntervalStatus statusFrom(
    const IntervalResult& result) noexcept {
    if (result.ok()) {
        return MeanRatioIntervalStatus::Ready;
    }
    if (result.failure == IntervalFailure::InvalidInput) {
        return MeanRatioIntervalStatus::InvalidInput;
    }
    return MeanRatioIntervalStatus::Range;
}

bool validPointIds(
    const std::array<PointId, 4>& ids) noexcept {
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (ids[i] == InvalidPointId) {
            return false;
        }
        for (std::size_t j = i + 1U; j < ids.size(); ++j) {
            if (ids[i] == ids[j]) {
                return false;
            }
        }
    }
    return true;
}

std::array<std::size_t, 4> canonicalPositiveOrder(
    const std::array<PointId, 4>& ids) noexcept {
    std::array<std::size_t, 4> order{0U, 1U, 2U, 3U};
    std::sort(
        order.begin(),
        order.end(),
        [&ids](std::size_t lhs, std::size_t rhs) {
            return ids[lhs] < ids[rhs];
        });

    std::size_t inversions = 0U;
    for (std::size_t i = 0; i < order.size(); ++i) {
        for (std::size_t j = i + 1U; j < order.size(); ++j) {
            if (order[i] > order[j]) {
                ++inversions;
            }
        }
    }

    // Incoming tetra exact-positive'dir. PointId siralamasi orientation'i
    // tersine cevirmisse tek deterministik swap positive parity'yi geri kurar.
    // Boylece fast path exact predicate maliyeti odemeden replay-stable kalir.
    if ((inversions & 1U) != 0U) {
        std::swap(order[2], order[3]);
    }
    return order;
}

bool finitePoint(const geometry::Vec3& point) noexcept {
    return std::isfinite(point.x) &&
           std::isfinite(point.y) &&
           std::isfinite(point.z);
}

IntervalResult addChecked(
    const IntervalResult& lhs,
    const IntervalResult& rhs) noexcept {
    if (!lhs.ok()) {
        return lhs;
    }
    if (!rhs.ok()) {
        return rhs;
    }
    return interval::add(lhs.value, rhs.value);
}

IntervalResult subChecked(
    const IntervalResult& lhs,
    const IntervalResult& rhs) noexcept {
    if (!lhs.ok()) {
        return lhs;
    }
    if (!rhs.ok()) {
        return rhs;
    }
    return interval::subtract(lhs.value, rhs.value);
}

IntervalResult determinant3(
    const std::array<IntervalVec3, 3>& relative) noexcept {
    // Frozen D26INT1 expression tree:
    // m0=e*i-f*h; m1=d*i-f*g; m2=d*h-e*g;
    // D=(a*m0-b*m1)+c*m2.
    const IntervalResult ei =
        interval::multiply(relative[1][1], relative[2][2]);
    const IntervalResult fh =
        interval::multiply(relative[1][2], relative[2][1]);
    const IntervalResult m0 = subChecked(ei, fh);

    const IntervalResult di =
        interval::multiply(relative[1][0], relative[2][2]);
    const IntervalResult fg =
        interval::multiply(relative[1][2], relative[2][0]);
    const IntervalResult m1 = subChecked(di, fg);

    const IntervalResult dh =
        interval::multiply(relative[1][0], relative[2][1]);
    const IntervalResult eg =
        interval::multiply(relative[1][1], relative[2][0]);
    const IntervalResult m2 = subChecked(dh, eg);

    const IntervalResult t0 = m0.ok()
        ? interval::multiply(relative[0][0], m0.value)
        : m0;
    const IntervalResult t1 = m1.ok()
        ? interval::multiply(relative[0][1], m1.value)
        : m1;
    const IntervalResult t2 = m2.ok()
        ? interval::multiply(relative[0][2], m2.value)
        : m2;

    return addChecked(subChecked(t0, t1), t2);
}

IntervalResult squaredDistance(
    const IntervalVec3& lhs,
    const IntervalVec3& rhs) noexcept {
    std::array<IntervalResult, 3> squares{};
    for (std::size_t axis = 0; axis < 3U; ++axis) {
        const IntervalResult difference =
            interval::subtract(lhs[axis], rhs[axis]);
        if (!difference.ok()) {
            return difference;
        }
        squares[axis] = interval::square(difference.value);
        if (!squares[axis].ok()) {
            return squares[axis];
        }
    }

    const IntervalResult xy =
        interval::add(squares[0].value, squares[1].value);
    if (!xy.ok()) {
        return xy;
    }
    return interval::add(xy.value, squares[2].value);
}

IntervalResult balancedEdgeSum(
    const RelativeTetra& points) noexcept {
    std::array<IntervalResult, 6> edges{};
    std::size_t edge = 0U;

    for (std::size_t i = 0; i < 4U; ++i) {
        for (std::size_t j = i + 1U; j < 4U; ++j) {
            edges[edge] = squaredDistance(points[i], points[j]);
            if (!edges[edge].ok()) {
                return edges[edge];
            }
            ++edge;
        }
    }

    // Frozen balanced S tree:
    // (e0+e1), (e2+e3), (e4+e5), then ((01)+(23))+(45).
    const IntervalResult s01 =
        interval::add(edges[0].value, edges[1].value);
    const IntervalResult s23 =
        interval::add(edges[2].value, edges[3].value);
    const IntervalResult s45 =
        interval::add(edges[4].value, edges[5].value);
    if (!s01.ok()) {
        return s01;
    }
    if (!s23.ok()) {
        return s23;
    }
    if (!s45.ok()) {
        return s45;
    }

    const IntervalResult s0123 =
        interval::add(s01.value, s23.value);
    if (!s0123.ok()) {
        return s0123;
    }
    return interval::add(s0123.value, s45.value);
}

MeanRatioIntervalStatus makeNormalizedRelativeTetra(
    const IndexedTetraCoordinates& tetra,
    RelativeTetra& normalized,
    int& normalizationExponent) noexcept {
    if (!validPointIds(tetra.pointIds)) {
        return MeanRatioIntervalStatus::InvalidInput;
    }
    for (const geometry::Vec3& point : tetra.coordinates) {
        if (!finitePoint(point)) {
            return MeanRatioIntervalStatus::InvalidInput;
        }
    }

    const std::array<std::size_t, 4> order =
        canonicalPositiveOrder(tetra.pointIds);
    const geometry::Vec3& anchor =
        tetra.coordinates[order[0]];

    const Interval zero{0.0, 0.0};
    normalized[0] = {zero, zero, zero};

    double maximumMagnitude = 0.0;
    std::array<IntervalVec3, 3> relative{};

    for (std::size_t row = 0; row < 3U; ++row) {
        const geometry::Vec3& point =
            tetra.coordinates[order[row + 1U]];
        const double p[3] = {point.x, point.y, point.z};
        const double a[3] = {anchor.x, anchor.y, anchor.z};

        for (std::size_t axis = 0; axis < 3U; ++axis) {
            const IntervalResult difference =
                interval::subtract(
                    {p[axis], p[axis]},
                    {a[axis], a[axis]});
            if (!difference.ok()) {
                return statusFrom(difference);
            }
            relative[row][axis] = difference.value;
            maximumMagnitude = std::max(
                maximumMagnitude,
                std::max(
                    std::abs(difference.value.lo),
                    std::abs(difference.value.hi)));
        }
    }

    if (!interval::normalizationExponent(
            maximumMagnitude,
            normalizationExponent)) {
        return MeanRatioIntervalStatus::Range;
    }

    for (std::size_t row = 0; row < 3U; ++row) {
        for (std::size_t axis = 0; axis < 3U; ++axis) {
            const IntervalResult scaled =
                interval::scalePowerOfTwo(
                    relative[row][axis],
                    normalizationExponent);
            if (!scaled.ok()) {
                return statusFrom(scaled);
            }
            normalized[row + 1U][axis] = scaled.value;
        }
    }

    return MeanRatioIntervalStatus::Ready;
}

MeanRatioFallbackReason reasonFrom(
    MeanRatioIntervalStatus status) noexcept {
    switch (status) {
        case MeanRatioIntervalStatus::UnsupportedEnvironment:
            return MeanRatioFallbackReason::UnsupportedEnvironment;
        case MeanRatioIntervalStatus::InvalidInput:
            return MeanRatioFallbackReason::InvalidFilterInput;
        case MeanRatioIntervalStatus::Range:
            return MeanRatioFallbackReason::Range;
        case MeanRatioIntervalStatus::Overlap:
            return MeanRatioFallbackReason::IntervalOverlap;
        case MeanRatioIntervalStatus::Ready:
            return MeanRatioFallbackReason::None;
    }
    return MeanRatioFallbackReason::Range;
}

void recordFallback(
    MeanRatioFilterTelemetry* telemetry,
    MeanRatioFallbackReason reason) noexcept {
    if (telemetry == nullptr) {
        return;
    }

    ++telemetry->exactFallback;
    switch (reason) {
        case MeanRatioFallbackReason::UnsupportedEnvironment:
            ++telemetry->uncertainEnvironment;
            break;
        case MeanRatioFallbackReason::InvalidFilterInput:
            ++telemetry->uncertainInvalidInput;
            break;
        case MeanRatioFallbackReason::Range:
            ++telemetry->uncertainRange;
            break;
        case MeanRatioFallbackReason::IntervalOverlap:
            ++telemetry->uncertainOverlap;
            break;
        case MeanRatioFallbackReason::None:
            break;
    }
}

MeanRatioEvaluation exactFallback(
    const IndexedTetraCoordinates& lhs,
    const IndexedTetraCoordinates& rhs,
    MeanRatioFallbackReason reason,
    MeanRatioFilterTelemetry* telemetry,
    ExactMeanRatioTelemetry* exactTelemetry) {
    recordFallback(telemetry, reason);

    const MeanRatioOrder order =
        compareExactMeanRatio(
            lhs.coordinates,
            rhs.coordinates,
            exactTelemetry);

    if (telemetry != nullptr &&
        order == MeanRatioOrder::Equal) {
        ++telemetry->exactEqual;
    }

    return {
        order,
        MeanRatioEvaluationPath::ExactFallback,
        reason};
}

} // namespace

MeanRatioIntervalStatus buildMeanRatioIntervalKey(
    const IndexedTetraCoordinates& tetra,
    MeanRatioIntervalKey& output) noexcept {
    if (!interval::probeEnvironment().supported()) {
        return MeanRatioIntervalStatus::UnsupportedEnvironment;
    }

    RelativeTetra normalized{};
    int exponent = 0;
    const MeanRatioIntervalStatus normalizationStatus =
        makeNormalizedRelativeTetra(
            tetra,
            normalized,
            exponent);
    if (normalizationStatus != MeanRatioIntervalStatus::Ready) {
        return normalizationStatus;
    }

    const std::array<IntervalVec3, 3> relative{
        normalized[1],
        normalized[2],
        normalized[3]};

    const IntervalResult determinant =
        determinant3(relative);
    if (!determinant.ok()) {
        return statusFrom(determinant);
    }

    const IntervalResult edgeSum =
        balancedEdgeSum(normalized);
    if (!edgeSum.ok()) {
        return statusFrom(edgeSum);
    }

    // S exact olarak positive olsa bile interval lower bound sifiri kapsiyorsa
    // filter kanit uretmez; bu geometry failure degil exact-fallback sebebidir.
    if (!(edgeSum.value.lo > 0.0)) {
        return MeanRatioIntervalStatus::Overlap;
    }

    output = {
        determinant.value,
        edgeSum.value,
        exponent};
    return MeanRatioIntervalStatus::Ready;
}

MeanRatioIntervalStatus buildMeanRatioPairIntervals(
    const IndexedTetraCoordinates& lhs,
    const IndexedTetraCoordinates& rhs,
    MeanRatioPairIntervals& output) noexcept {
    MeanRatioIntervalKey lhsKey{};
    MeanRatioIntervalKey rhsKey{};

    const MeanRatioIntervalStatus lhsStatus =
        buildMeanRatioIntervalKey(lhs, lhsKey);
    if (lhsStatus != MeanRatioIntervalStatus::Ready) {
        return lhsStatus;
    }

    const MeanRatioIntervalStatus rhsStatus =
        buildMeanRatioIntervalKey(rhs, rhsKey);
    if (rhsStatus != MeanRatioIntervalStatus::Ready) {
        return rhsStatus;
    }

    const IntervalResult lhsD2 =
        interval::square(lhsKey.determinant);
    const IntervalResult rhsD2 =
        interval::square(rhsKey.determinant);
    const IntervalResult lhsS3 =
        interval::cubePositive(lhsKey.edgeSum);
    const IntervalResult rhsS3 =
        interval::cubePositive(rhsKey.edgeSum);

    if (!lhsD2.ok()) {
        return statusFrom(lhsD2);
    }
    if (!rhsD2.ok()) {
        return statusFrom(rhsD2);
    }
    if (!lhsS3.ok()) {
        return statusFrom(lhsS3);
    }
    if (!rhsS3.ok()) {
        return statusFrom(rhsS3);
    }

    const IntervalResult left =
        interval::multiplyNonNegative(
            lhsD2.value,
            rhsS3.value);
    const IntervalResult right =
        interval::multiplyNonNegative(
            rhsD2.value,
            lhsS3.value);

    if (!left.ok()) {
        return statusFrom(left);
    }
    if (!right.ok()) {
        return statusFrom(right);
    }

    // Frozen polynomial:
    // F = D_A^2 S_B^3 - D_B^2 S_A^3.
    const IntervalResult cross =
        interval::subtract(left.value, right.value);
    if (!cross.ok()) {
        return statusFrom(cross);
    }

    output = {
        lhsKey,
        rhsKey,
        cross.value};
    return MeanRatioIntervalStatus::Ready;
}

MeanRatioEvaluation compareFilteredMeanRatio(
    const IndexedTetraCoordinates& lhs,
    const IndexedTetraCoordinates& rhs,
    MeanRatioFilterTelemetry* telemetry,
    ExactMeanRatioTelemetry* exactTelemetry) {
    if (telemetry != nullptr) {
        ++telemetry->calls;
    }

    MeanRatioPairIntervals intervals{};
    const MeanRatioIntervalStatus status =
        buildMeanRatioPairIntervals(
            lhs,
            rhs,
            intervals);

    if (status != MeanRatioIntervalStatus::Ready) {
        return exactFallback(
            lhs,
            rhs,
            reasonFrom(status),
            telemetry,
            exactTelemetry);
    }

    if (intervals.crossPolynomial.lo > 0.0) {
        if (telemetry != nullptr) {
            ++telemetry->intervalCertifiedGreater;
        }
        return {
            MeanRatioOrder::Greater,
            MeanRatioEvaluationPath::IntervalCertified,
            MeanRatioFallbackReason::None};
    }

    if (intervals.crossPolynomial.hi < 0.0) {
        if (telemetry != nullptr) {
            ++telemetry->intervalCertifiedLess;
        }
        return {
            MeanRatioOrder::Less,
            MeanRatioEvaluationPath::IntervalCertified,
            MeanRatioFallbackReason::None};
    }

    // Fast Equal yoktur. Sifiri kapsayan her F intervali exact authority'ye gider.
    return exactFallback(
        lhs,
        rhs,
        MeanRatioFallbackReason::IntervalOverlap,
        telemetry,
        exactTelemetry);
}

} // namespace femcae::meshing::m6::quality
