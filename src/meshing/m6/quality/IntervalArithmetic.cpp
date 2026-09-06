#include "IntervalArithmetic.h"

#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>

#ifdef __FAST_MATH__
#error "Dynamics26 D26INT1 cannot be compiled with fast-math semantics"
#endif

#ifndef FEMCAE_FP_CONTRACT_CONTROLLED
#error "Dynamics26 D26INT1 must be built through FEMCAE_CERTIFIED_FP_SOURCES so contraction control is enforced"
#endif

namespace femcae::meshing::m6::quality::interval {
namespace {

bool finiteOrdered(Interval value) noexcept {
    return std::isfinite(value.lo) && std::isfinite(value.hi) &&
           value.lo <= value.hi;
}

IntervalResult failure(IntervalFailure why) noexcept {
    return {{}, why};
}

// Bir primitive sonucuna yalnizca bir kez uygulanan outward step.
// Bilesik ifadeler bu yardimciyi son adimda tek kez kullanamaz; her node
// add/sub/mul gibi primitive fonksiyonlar uzerinden tek tek genisletilmelidir.
IntervalResult outwardPrimitive(double lo, double hi) noexcept {
    if (!std::isfinite(lo) || !std::isfinite(hi)) {
        return failure(IntervalFailure::Range);
    }
    const double widenedLo = nextDown(lo);
    const double widenedHi = nextUp(hi);
    if (!std::isfinite(widenedLo) || !std::isfinite(widenedHi)) {
        return failure(IntervalFailure::Range);
    }
    return {{widenedLo, widenedHi}, IntervalFailure::None};
}

IntervalResult productPrimitive(double lhs, double rhs) noexcept {
    const double rounded = lhs * rhs;
    return outwardPrimitive(rounded, rounded);
}

} // namespace

EnvironmentProbe probeEnvironment() noexcept {
    // D26INT1 aktif rounding mode'u degistirmez. Caller ortami RN degilse
    // filter sertifika uretmemeli ve exact D26QMRB1'e dusmelidir.
    EnvironmentProbe result;
    result.binary64 =
        std::numeric_limits<double>::is_iec559 &&
        std::numeric_limits<double>::radix == 2 &&
        std::numeric_limits<double>::digits == 53 &&
        sizeof(double) == sizeof(std::uint64_t);

    result.roundToNearest = std::fegetround() == FE_TONEAREST;

    if (!result.binary64) {
        return result;
    }

    // Volatile runtime islemleri constant-folding'i engeller. Iki carpim da
    // matematiksel olarak exact'tir; FTZ/DAZ-benzeri uyumsuz ortamda probe fail olur.
    volatile double minNormal = std::numeric_limits<double>::min();
    volatile double half = 0.5;
    volatile double halfMinNormal = minNormal * half;
    const double expectedHalfMinNormal =
        std::bit_cast<double>(0x0008000000000000ULL);
    result.gradualSubnormalOutput =
        static_cast<double>(halfMinNormal) == expectedHalfMinNormal;

    volatile double minSubnormal = std::numeric_limits<double>::denorm_min();
    volatile double two = 2.0;
    volatile double secondSubnormal = minSubnormal * two;
    const double expectedSecondSubnormal =
        std::bit_cast<double>(0x0000000000000002ULL);
    result.gradualSubnormalInput =
        static_cast<double>(secondSubnormal) == expectedSecondSubnormal;

    return result;
}

double nextDown(double value) noexcept {
    return std::nextafter(value, -std::numeric_limits<double>::infinity());
}

double nextUp(double value) noexcept {
    return std::nextafter(value, std::numeric_limits<double>::infinity());
}

IntervalResult point(double value) noexcept {
    if (!std::isfinite(value)) {
        return failure(IntervalFailure::InvalidInput);
    }
    return {{value, value}, IntervalFailure::None};
}

IntervalResult add(Interval lhs, Interval rhs) noexcept {
    if (!finiteOrdered(lhs) || !finiteOrdered(rhs)) {
        return failure(IntervalFailure::InvalidInput);
    }
    const double roundedLo = lhs.lo + rhs.lo;
    const double roundedHi = lhs.hi + rhs.hi;
    return outwardPrimitive(roundedLo, roundedHi);
}

IntervalResult subtract(Interval lhs, Interval rhs) noexcept {
    if (!finiteOrdered(lhs) || !finiteOrdered(rhs)) {
        return failure(IntervalFailure::InvalidInput);
    }
    const double roundedLo = lhs.lo - rhs.hi;
    const double roundedHi = lhs.hi - rhs.lo;
    return outwardPrimitive(roundedLo, roundedHi);
}

IntervalResult multiply(Interval lhs, Interval rhs) noexcept {
    // Bilinear interval ekstremumlari dort endpoint carpimindadir. Onemli nokta:
    // min/max seciminden once her carpim kendi outward enclosure'unu alir.
    if (!finiteOrdered(lhs) || !finiteOrdered(rhs)) {
        return failure(IntervalFailure::InvalidInput);
    }

    const IntervalResult p00 = productPrimitive(lhs.lo, rhs.lo);
    const IntervalResult p01 = productPrimitive(lhs.lo, rhs.hi);
    const IntervalResult p10 = productPrimitive(lhs.hi, rhs.lo);
    const IntervalResult p11 = productPrimitive(lhs.hi, rhs.hi);
    if (!p00.ok() || !p01.ok() || !p10.ok() || !p11.ok()) {
        return failure(IntervalFailure::Range);
    }

    const double lo = std::min({
        p00.value.lo, p01.value.lo, p10.value.lo, p11.value.lo});
    const double hi = std::max({
        p00.value.hi, p01.value.hi, p10.value.hi, p11.value.hi});
    return {{lo, hi}, IntervalFailure::None};
}

IntervalResult multiplyNonNegative(Interval lhs, Interval rhs) noexcept {
    if (!finiteOrdered(lhs) || !finiteOrdered(rhs) ||
        lhs.lo < 0.0 || rhs.lo < 0.0) {
        return failure(IntervalFailure::InvalidInput);
    }
    const IntervalResult low = productPrimitive(lhs.lo, rhs.lo);
    const IntervalResult high = productPrimitive(lhs.hi, rhs.hi);
    if (!low.ok() || !high.ok()) {
        return failure(IntervalFailure::Range);
    }
    const double lo = std::max(0.0, low.value.lo);
    return {{lo, high.value.hi}, IntervalFailure::None};
}

IntervalResult square(Interval value) noexcept {
    // Aralik sifiri iceriyorsa matematiksel alt sinir tam +0'dir; -minsub'a
    // gereksiz genisletme yapmayiz. Ust sinir yine primitive carpimlardan gelir.
    if (!finiteOrdered(value)) {
        return failure(IntervalFailure::InvalidInput);
    }

    if (value.lo <= 0.0 && value.hi >= 0.0) {
        const IntervalResult loSquare = productPrimitive(value.lo, value.lo);
        const IntervalResult hiSquare = productPrimitive(value.hi, value.hi);
        if (!loSquare.ok() || !hiSquare.ok()) {
            return failure(IntervalFailure::Range);
        }
        return {{
            0.0,
            std::max(loSquare.value.hi, hiSquare.value.hi)},
            IntervalFailure::None};
    }

    if (value.lo >= 0.0) {
        const IntervalResult low = productPrimitive(value.lo, value.lo);
        const IntervalResult high = productPrimitive(value.hi, value.hi);
        if (!low.ok() || !high.ok()) {
            return failure(IntervalFailure::Range);
        }
        return {{
            std::max(0.0, low.value.lo),
            high.value.hi},
            IntervalFailure::None};
    }

    const IntervalResult low = productPrimitive(value.hi, value.hi);
    const IntervalResult high = productPrimitive(value.lo, value.lo);
    if (!low.ok() || !high.ok()) {
        return failure(IntervalFailure::Range);
    }
    return {{
        std::max(0.0, low.value.lo),
        high.value.hi},
        IntervalFailure::None};
}

IntervalResult cubePositive(Interval value) noexcept {
    if (!finiteOrdered(value) || value.lo <= 0.0) {
        return failure(IntervalFailure::InvalidInput);
    }
    const IntervalResult squared = square(value);
    if (!squared.ok()) {
        return squared;
    }
    return multiplyNonNegative(squared.value, value);
}

IntervalResult scalePowerOfTwo(Interval value, int exponent) noexcept {
    // std::scalbn radix-2 olceklemede normal aralikta exact'tir. Ilk backend,
    // negatif exponent ile subnormal bolgeye inen endpointleri konservatif olarak
    // Range sayar; rounding/underflow ayrintisini tahmin etmek yerine exact fallback
    // yolunun kullanilmasini saglar.
    if (!finiteOrdered(value)) {
        return failure(IntervalFailure::InvalidInput);
    }

    const double lo = std::scalbn(value.lo, exponent);
    const double hi = std::scalbn(value.hi, exponent);
    if (!std::isfinite(lo) || !std::isfinite(hi)) {
        return failure(IntervalFailure::Range);
    }

    const auto unsupportedDownscale =
        [exponent](double before, double after) noexcept {
            if (before == 0.0) {
                return false;
            }
            if (after == 0.0) {
                return true;
            }
            return exponent < 0 && std::fpclassify(after) == FP_SUBNORMAL;
        };

    if (unsupportedDownscale(value.lo, lo) ||
        unsupportedDownscale(value.hi, hi)) {
        return failure(IntervalFailure::Range);
    }
    return {{lo, hi}, IntervalFailure::None};
}

bool normalizationExponent(
    double positiveMaximum,
    int& exponent) noexcept {
    if (!std::isfinite(positiveMaximum) || positiveMaximum <= 0.0) {
        return false;
    }
    const int e = std::ilogb(positiveMaximum);
    if (e == FP_ILOGB0 ||
        e == FP_ILOGBNAN ||
        e == std::numeric_limits<int>::max()) {
        return false;
    }
    exponent = -e - 1;
    return true;
}

} // namespace femcae::meshing::m6::quality::interval
