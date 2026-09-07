#include "femcae/meshing/RobustPredicates.h"

#include "internal/exact/ExactDyadicArithmetic.h"
#include "internal/PredicateCallAudit.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#ifdef __FAST_MATH__
#error "Dynamics26 RobustPredicates cannot be compiled with fast-math semantics"
#endif

#ifndef FEMCAE_FP_CONTRACT_CONTROLLED
#error "Dynamics26 M1 robust predicates must be built through FEMCAE_CERTIFIED_FP_SOURCES so contraction control is enforced"
#endif

namespace femcae::meshing::predicates {
namespace {

using BigInt = femcae::meshing::internal::exact::BigInt;
using Matrix = femcae::meshing::internal::exact::Matrix;
using femcae::meshing::internal::exact::determinant;
using femcae::meshing::internal::exact::exactIntegerCoordinates;

thread_local PredicateTelemetry* qualificationCallAuditSink = nullptr;
thread_local internal::PredicateDetailedTelemetry* qualificationDetailedAuditSink = nullptr;

bool fastValueAllowed(double value) noexcept {
    return std::isfinite(value) &&
           (value == 0.0 || std::fpclassify(value) == FP_NORMAL);
}

bool safeMultiply(double lhs, double rhs, double& result) noexcept {
    result = lhs * rhs;
    if (!std::isfinite(result)) {
        return false;
    }
    if (result != 0.0 && std::fpclassify(result) != FP_NORMAL) {
        return false;
    }
    if (result == 0.0 && lhs != 0.0 && rhs != 0.0) {
        return false;
    }
    return true;
}

bool safeAdd(double lhs, double rhs, double& result) noexcept {
    result = lhs + rhs;
    if (!std::isfinite(result)) {
        return false;
    }
    if (result != 0.0 && std::fpclassify(result) != FP_NORMAL) {
        return false;
    }
    if (result == 0.0 && lhs != -rhs) {
        return false;
    }
    return true;
}

bool safeLift2(double x, double y, double& lift) noexcept {
    if (!fastValueAllowed(x) || !fastValueAllowed(y)) {
        return false;
    }
    double xx = 0.0;
    double yy = 0.0;
    if (!safeMultiply(x, x, xx) || !safeMultiply(y, y, yy)) {
        return false;
    }
    return safeAdd(xx, yy, lift);
}

bool safeLift3(double x, double y, double z, double& lift) noexcept {
    if (!fastValueAllowed(x) || !fastValueAllowed(y) || !fastValueAllowed(z)) {
        return false;
    }
    double xx = 0.0;
    double yy = 0.0;
    double zz = 0.0;
    double partial = 0.0;
    if (!safeMultiply(x, x, xx) ||
        !safeMultiply(y, y, yy) ||
        !safeMultiply(z, z, zz) ||
        !safeAdd(xx, yy, partial)) {
        return false;
    }
    return safeAdd(partial, zz, lift);
}

int permutationParity(const std::vector<std::size_t>& permutation) noexcept {
    std::size_t inversions = 0U;
    for (std::size_t i = 0; i < permutation.size(); ++i) {
        for (std::size_t j = i + 1U; j < permutation.size(); ++j) {
            if (permutation[i] > permutation[j]) {
                ++inversions;
            }
        }
    }
    return (inversions & 1U) == 0U ? 1 : -1;
}

std::optional<PredicateEvaluation> tryFastDeterminant(
    const std::vector<std::vector<double>>& matrix) {
    const std::size_t size = matrix.size();
    if (size < 2U || size > 5U) {
        return std::nullopt;
    }
    for (const auto& row : matrix) {
        if (row.size() != size) {
            return std::nullopt;
        }
        for (double value : row) {
            if (!fastValueAllowed(value)) {
                return std::nullopt;
            }
        }
    }

    std::vector<std::size_t> permutation(size);
    for (std::size_t i = 0; i < size; ++i) {
        permutation[i] = i;
    }

    double determinantValue = 0.0;
    double permanent = 0.0;

    do {
        double term = 1.0;
        for (std::size_t row = 0; row < size; ++row) {
            double product = 0.0;
            if (!safeMultiply(term, matrix[row][permutation[row]], product)) {
                return std::nullopt;
            }
            term = product;
        }

        if (permutationParity(permutation) < 0) {
            term = -term;
        }

        double updatedDeterminant = 0.0;
        if (!safeAdd(determinantValue, term, updatedDeterminant)) {
            return std::nullopt;
        }
        determinantValue = updatedDeterminant;

        double updatedPermanent = 0.0;
        if (!safeAdd(permanent, std::abs(term), updatedPermanent)) {
            return std::nullopt;
        }
        permanent = updatedPermanent;
    } while (std::next_permutation(permutation.begin(), permutation.end()));

    if (permanent == 0.0 || determinantValue == 0.0) {
        return std::nullopt;
    }

    // M1.8 F0 uniform envelope.
    //
    // For every supported determinant (up to lifted 5x5 insphere), one
    // permutation term contains at most one rounded lift plus four rounded
    // multiplications. The term and 120-term accumulation error is bounded by
    // the M1.8 gamma-model derivation well below 1024*u times the computed
    // absolute-term sum. 1024*u = 2^-43 is exactly representable in binary64.
    //
    // This deliberately trades additional exact fallbacks for a simple,
    // independently auditable no-false-certification envelope.
    constexpr double unitRoundoff = 0x1p-53;
    constexpr auto gamma = [](double operations) constexpr {
        return (operations * unitRoundoff) /
               (1.0 - operations * unitRoundoff);
    };

    // Worst supported graph is 5x5 insphere:
    // - lift path: <= 3 rounded operations,
    // - one determinant term: <= 5 rounded multiplies,
    // - signed determinant accumulation: conservatively <= 120 additions,
    // - permanent accumulation: conservatively <= 120 additions.
    //
    // Thus an exact term differs from its computed counterpart by <= gamma_8,
    // and the full determinant forward error is bounded by gamma_128 times the
    // exact absolute-term sum. Converting that exact sum to the computed
    // permanent adds the (1-gamma_8)(1-gamma_120) denominator corrections.
    constexpr double worstCaseHomogeneousBound =
        gamma(128.0) /
        ((1.0 - gamma(8.0)) * (1.0 - gamma(120.0)));

    constexpr double conservativeCoefficient = 0x1p-43; // 1024*u
    static_assert(
        conservativeCoefficient * (1.0 - unitRoundoff) >
        worstCaseHomogeneousBound,
        "M1.8 fast-filter coefficient no longer dominates the proved error bound");

    double errorBound = 0.0;
    if (!safeMultiply(permanent, conservativeCoefficient, errorBound) ||
        errorBound == 0.0) {
        return std::nullopt;
    }

    if (determinantValue > errorBound) {
        return PredicateEvaluation{
            PredicateSign::Positive, PredicateEvaluationPath::FastCertified};
    }
    if (determinantValue < -errorBound) {
        return PredicateEvaluation{
            PredicateSign::Negative, PredicateEvaluationPath::FastCertified};
    }
    return std::nullopt;
}

std::optional<PredicateEvaluation> tryFastOrient2d(
    const geometry::Vec2& a,
    const geometry::Vec2& b,
    const geometry::Vec2& c) {
    return tryFastDeterminant({
        {a.x, a.y, 1.0},
        {b.x, b.y, 1.0},
        {c.x, c.y, 1.0}});
}

std::optional<PredicateEvaluation> tryFastOrient3d(
    const geometry::Vec3& a,
    const geometry::Vec3& b,
    const geometry::Vec3& c,
    const geometry::Vec3& d) {
    return tryFastDeterminant({
        {a.x, a.y, a.z, 1.0},
        {b.x, b.y, b.z, 1.0},
        {c.x, c.y, c.z, 1.0},
        {d.x, d.y, d.z, 1.0}});
}

std::optional<PredicateEvaluation> tryFastIncircle(
    const geometry::Vec2& a,
    const geometry::Vec2& b,
    const geometry::Vec2& c,
    const geometry::Vec2& d) {
    double alift = 0.0;
    double blift = 0.0;
    double clift = 0.0;
    double dlift = 0.0;
    if (!safeLift2(a.x, a.y, alift) ||
        !safeLift2(b.x, b.y, blift) ||
        !safeLift2(c.x, c.y, clift) ||
        !safeLift2(d.x, d.y, dlift)) {
        return std::nullopt;
    }
    return tryFastDeterminant({
        {a.x, a.y, alift, 1.0},
        {b.x, b.y, blift, 1.0},
        {c.x, c.y, clift, 1.0},
        {d.x, d.y, dlift, 1.0}});
}

std::optional<PredicateEvaluation> tryFastInsphere(
    const geometry::Vec3& a,
    const geometry::Vec3& b,
    const geometry::Vec3& c,
    const geometry::Vec3& d,
    const geometry::Vec3& e) {
    double alift = 0.0;
    double blift = 0.0;
    double clift = 0.0;
    double dlift = 0.0;
    double elift = 0.0;
    if (!safeLift3(a.x, a.y, a.z, alift) ||
        !safeLift3(b.x, b.y, b.z, blift) ||
        !safeLift3(c.x, c.y, c.z, clift) ||
        !safeLift3(d.x, d.y, d.z, dlift) ||
        !safeLift3(e.x, e.y, e.z, elift)) {
        return std::nullopt;
    }
    return tryFastDeterminant({
        {a.x, a.y, a.z, alift, 1.0},
        {b.x, b.y, b.z, blift, 1.0},
        {c.x, c.y, c.z, clift, 1.0},
        {d.x, d.y, d.z, dlift, 1.0},
        {e.x, e.y, e.z, elift, 1.0}});
}

PredicateSign predicateSign(int sign) noexcept {
    if (sign < 0) {
        return PredicateSign::Negative;
    }
    if (sign > 0) {
        return PredicateSign::Positive;
    }
    return PredicateSign::Zero;
}

PredicateEvaluation exactEvaluation(const Matrix& matrix) {
    return {predicateSign(determinant(matrix).sign()), PredicateEvaluationPath::ExactDyadic};
}

void recordEvaluation(
    PredicateTelemetry* telemetry,
    const PredicateEvaluation& evaluation) noexcept {
    if (telemetry == nullptr) {
        return;
    }
    if (evaluation.path == PredicateEvaluationPath::FastCertified) {
        ++telemetry->fastCertified;
    } else {
        ++telemetry->exactFallback;
    }
    if (evaluation.sign == PredicateSign::Zero) {
        ++telemetry->exactZero;
    }
}

void recordCall(PredicateTelemetry* telemetry) noexcept {
    if (telemetry != nullptr) {
        ++telemetry->calls;
    }
    if (qualificationCallAuditSink != nullptr &&
        qualificationCallAuditSink != telemetry) {
        ++qualificationCallAuditSink->calls;
    }
}

void recordInvalid(PredicateTelemetry* telemetry) noexcept {
    if (telemetry != nullptr) {
        ++telemetry->invalidInput;
    }
}

void recordDetailedCall(
    PredicateTelemetry* explicitTelemetry,
    PredicateTelemetry* detailedTelemetry) noexcept {
    if (detailedTelemetry != nullptr &&
        detailedTelemetry != explicitTelemetry) {
        ++detailedTelemetry->calls;
    }
}

void recordDetailedEvaluation(
    PredicateTelemetry* explicitTelemetry,
    PredicateTelemetry* detailedTelemetry,
    const PredicateEvaluation& evaluation) noexcept {
    if (detailedTelemetry == nullptr ||
        detailedTelemetry == explicitTelemetry) {
        return;
    }
    if (evaluation.path == PredicateEvaluationPath::FastCertified) {
        ++detailedTelemetry->fastCertified;
    } else {
        ++detailedTelemetry->exactFallback;
    }
    if (evaluation.sign == PredicateSign::Zero) {
        ++detailedTelemetry->exactZero;
    }
}

void recordDetailedInvalid(
    PredicateTelemetry* explicitTelemetry,
    PredicateTelemetry* detailedTelemetry) noexcept {
    if (detailedTelemetry != nullptr &&
        detailedTelemetry != explicitTelemetry) {
        ++detailedTelemetry->invalidInput;
    }
}

} // namespace

namespace internal {

void setPredicateCallAuditSink(PredicateTelemetry* telemetry) noexcept {
    qualificationCallAuditSink = telemetry;
}

void setPredicateDetailedAuditSink(
    PredicateDetailedTelemetry* telemetry) noexcept {
    qualificationDetailedAuditSink = telemetry;
}

} // namespace internal

PredicateEvaluation orient2d(
    const geometry::Vec2& a,
    const geometry::Vec2& b,
    const geometry::Vec2& c,
    PredicateTelemetry* telemetry) {
    PredicateTelemetry* detailedTelemetry =
        qualificationDetailedAuditSink != nullptr
            ? &qualificationDetailedAuditSink->orient2d
            : nullptr;
    recordCall(telemetry);
    recordDetailedCall(telemetry, detailedTelemetry);
    try {
        if (const auto fast = tryFastOrient2d(a, b, c)) {
            recordEvaluation(telemetry, *fast);
            recordDetailedEvaluation(
                telemetry, detailedTelemetry, *fast);
            return *fast;
        }

        const std::vector<BigInt> p = exactIntegerCoordinates({
            a.x, a.y, b.x, b.y, c.x, c.y});

        Matrix matrix{
            {p[0], p[1], BigInt::one()},
            {p[2], p[3], BigInt::one()},
            {p[4], p[5], BigInt::one()}};
        const PredicateEvaluation result = exactEvaluation(matrix);
        recordEvaluation(telemetry, result);
        recordDetailedEvaluation(
            telemetry, detailedTelemetry, result);
        return result;
    } catch (const std::invalid_argument&) {
        recordInvalid(telemetry);
        recordDetailedInvalid(telemetry, detailedTelemetry);
        throw;
    }
}

PredicateEvaluation orient3d(
    const geometry::Vec3& a,
    const geometry::Vec3& b,
    const geometry::Vec3& c,
    const geometry::Vec3& d,
    PredicateTelemetry* telemetry) {
    PredicateTelemetry* detailedTelemetry =
        qualificationDetailedAuditSink != nullptr
            ? &qualificationDetailedAuditSink->orient3d
            : nullptr;
    recordCall(telemetry);
    recordDetailedCall(telemetry, detailedTelemetry);
    try {
        if (const auto fast = tryFastOrient3d(a, b, c, d)) {
            recordEvaluation(telemetry, *fast);
            recordDetailedEvaluation(
                telemetry, detailedTelemetry, *fast);
            return *fast;
        }

        const std::vector<BigInt> p = exactIntegerCoordinates({
            a.x, a.y, a.z,
            b.x, b.y, b.z,
            c.x, c.y, c.z,
            d.x, d.y, d.z});

        Matrix matrix{
            {p[0], p[1], p[2], BigInt::one()},
            {p[3], p[4], p[5], BigInt::one()},
            {p[6], p[7], p[8], BigInt::one()},
            {p[9], p[10], p[11], BigInt::one()}};
        const PredicateEvaluation result = exactEvaluation(matrix);
        recordEvaluation(telemetry, result);
        recordDetailedEvaluation(
            telemetry, detailedTelemetry, result);
        return result;
    } catch (const std::invalid_argument&) {
        recordInvalid(telemetry);
        recordDetailedInvalid(telemetry, detailedTelemetry);
        throw;
    }
}

PredicateEvaluation incircle(
    const geometry::Vec2& a,
    const geometry::Vec2& b,
    const geometry::Vec2& c,
    const geometry::Vec2& d,
    PredicateTelemetry* telemetry) {
    PredicateTelemetry* detailedTelemetry =
        qualificationDetailedAuditSink != nullptr
            ? &qualificationDetailedAuditSink->incircle
            : nullptr;
    recordCall(telemetry);
    recordDetailedCall(telemetry, detailedTelemetry);
    try {
        if (const auto fast = tryFastIncircle(a, b, c, d)) {
            recordEvaluation(telemetry, *fast);
            recordDetailedEvaluation(
                telemetry, detailedTelemetry, *fast);
            return *fast;
        }

        const std::vector<BigInt> p = exactIntegerCoordinates({
            a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y});

        Matrix matrix;
        matrix.reserve(4U);
        for (std::size_t i = 0; i < 4U; ++i) {
            const BigInt& x = p[2U * i];
            const BigInt& y = p[2U * i + 1U];
            const BigInt lift = x * x + y * y;
            matrix.push_back({x, y, lift, BigInt::one()});
        }
        const PredicateEvaluation result = exactEvaluation(matrix);
        recordEvaluation(telemetry, result);
        recordDetailedEvaluation(
            telemetry, detailedTelemetry, result);
        return result;
    } catch (const std::invalid_argument&) {
        recordInvalid(telemetry);
        recordDetailedInvalid(telemetry, detailedTelemetry);
        throw;
    }
}

PredicateEvaluation insphere(
    const geometry::Vec3& a,
    const geometry::Vec3& b,
    const geometry::Vec3& c,
    const geometry::Vec3& d,
    const geometry::Vec3& e,
    PredicateTelemetry* telemetry) {
    PredicateTelemetry* detailedTelemetry =
        qualificationDetailedAuditSink != nullptr
            ? &qualificationDetailedAuditSink->insphere
            : nullptr;
    recordCall(telemetry);
    recordDetailedCall(telemetry, detailedTelemetry);
    try {
        if (const auto fast = tryFastInsphere(a, b, c, d, e)) {
            recordEvaluation(telemetry, *fast);
            recordDetailedEvaluation(
                telemetry, detailedTelemetry, *fast);
            return *fast;
        }

        const std::vector<BigInt> p = exactIntegerCoordinates({
            a.x, a.y, a.z,
            b.x, b.y, b.z,
            c.x, c.y, c.z,
            d.x, d.y, d.z,
            e.x, e.y, e.z});

        Matrix matrix;
        matrix.reserve(5U);
        for (std::size_t i = 0; i < 5U; ++i) {
            const BigInt& x = p[3U * i];
            const BigInt& y = p[3U * i + 1U];
            const BigInt& z = p[3U * i + 2U];
            const BigInt lift = x * x + y * y + z * z;
            matrix.push_back({x, y, z, lift, BigInt::one()});
        }
        const PredicateEvaluation result = exactEvaluation(matrix);
        recordEvaluation(telemetry, result);
        recordDetailedEvaluation(
            telemetry, detailedTelemetry, result);
        return result;
    } catch (const std::invalid_argument&) {
        recordInvalid(telemetry);
        recordDetailedInvalid(telemetry, detailedTelemetry);
        throw;
    }
}

} // namespace femcae::meshing::predicates
