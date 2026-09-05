#pragma once

#include "femcae/geometry/GeometryTypes.h"

#include <cstdint>

namespace femcae::meshing::predicates {

enum class PredicateSign : std::int8_t {
    Negative = -1,
    Zero = 0,
    Positive = 1
};

enum class PredicateEvaluationPath : std::uint8_t {
    ExactDyadic = 0,
    FastCertified = 1
};

struct PredicateEvaluation {
    PredicateSign sign{PredicateSign::Zero};
    PredicateEvaluationPath path{PredicateEvaluationPath::ExactDyadic};
};

struct PredicateTelemetry {
    std::uint64_t calls{0};
    std::uint64_t fastCertified{0};
    std::uint64_t exactFallback{0};
    std::uint64_t exactZero{0};
    std::uint64_t invalidInput{0};
};

[[nodiscard]] PredicateEvaluation orient2d(
    const geometry::Vec2& a,
    const geometry::Vec2& b,
    const geometry::Vec2& c,
    PredicateTelemetry* telemetry = nullptr);

[[nodiscard]] PredicateEvaluation orient3d(
    const geometry::Vec3& a,
    const geometry::Vec3& b,
    const geometry::Vec3& c,
    const geometry::Vec3& d,
    PredicateTelemetry* telemetry = nullptr);

[[nodiscard]] PredicateEvaluation incircle(
    const geometry::Vec2& a,
    const geometry::Vec2& b,
    const geometry::Vec2& c,
    const geometry::Vec2& d,
    PredicateTelemetry* telemetry = nullptr);

[[nodiscard]] PredicateEvaluation insphere(
    const geometry::Vec3& a,
    const geometry::Vec3& b,
    const geometry::Vec3& c,
    const geometry::Vec3& d,
    const geometry::Vec3& e,
    PredicateTelemetry* telemetry = nullptr);

} // namespace femcae::meshing::predicates
