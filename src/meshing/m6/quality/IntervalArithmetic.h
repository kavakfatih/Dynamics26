#pragma once

#include <cstdint>

namespace femcae::meshing::m6::quality::interval {

// D26INT1 private interval arithmetic kernel.
//
// Her arithmetic primitive binary64 round-to-nearest ile bir kez degerlendirilir
// ve hemen ardindan komsu temsil edilebilir sayiya dogru disari genisletilir.
// Bu tipler authoritative mesh/quality verisi degildir; yalnizca D26QMRF1'in
// bir isareti hizli ve sertifikali bicimde kanitlamasina hizmet eder.

enum class IntervalFailure : std::uint8_t {
    None = 0,
    InvalidInput,
    UnsupportedEnvironment,
    Range
};

struct Interval {
    double lo{0.0};
    double hi{0.0};
};

struct IntervalResult {
    Interval value{};
    IntervalFailure failure{IntervalFailure::None};

    [[nodiscard]] bool ok() const noexcept {
        return failure == IntervalFailure::None;
    }
};

struct EnvironmentProbe {
    bool binary64{false};
    bool roundToNearest{false};
    bool gradualSubnormalOutput{false};
    bool gradualSubnormalInput{false};

    [[nodiscard]] bool supported() const noexcept {
        return binary64 && roundToNearest &&
               gradualSubnormalOutput && gradualSubnormalInput;
    }
};

[[nodiscard]] EnvironmentProbe probeEnvironment() noexcept;
[[nodiscard]] double nextDown(double value) noexcept;
[[nodiscard]] double nextUp(double value) noexcept;
[[nodiscard]] IntervalResult point(double value) noexcept;
[[nodiscard]] IntervalResult add(Interval lhs, Interval rhs) noexcept;
[[nodiscard]] IntervalResult subtract(Interval lhs, Interval rhs) noexcept;
[[nodiscard]] IntervalResult multiply(Interval lhs, Interval rhs) noexcept;
[[nodiscard]] IntervalResult multiplyNonNegative(Interval lhs, Interval rhs) noexcept;
[[nodiscard]] IntervalResult square(Interval value) noexcept;
[[nodiscard]] IntervalResult cubePositive(Interval value) noexcept;
[[nodiscard]] IntervalResult scalePowerOfTwo(Interval value, int exponent) noexcept;
[[nodiscard]] bool normalizationExponent(double positiveMaximum, int& exponent) noexcept;

} // namespace femcae::meshing::m6::quality::interval
