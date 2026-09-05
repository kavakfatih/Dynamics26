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

[[nodiscard]] PredicateEvaluation orient2d(
    const geometry::Vec2& a,
    const geometry::Vec2& b,
    const geometry::Vec2& c);

[[nodiscard]] PredicateEvaluation orient3d(
    const geometry::Vec3& a,
    const geometry::Vec3& b,
    const geometry::Vec3& c,
    const geometry::Vec3& d);

[[nodiscard]] PredicateEvaluation incircle(
    const geometry::Vec2& a,
    const geometry::Vec2& b,
    const geometry::Vec2& c,
    const geometry::Vec2& d);

[[nodiscard]] PredicateEvaluation insphere(
    const geometry::Vec3& a,
    const geometry::Vec3& b,
    const geometry::Vec3& c,
    const geometry::Vec3& d,
    const geometry::Vec3& e);

} // namespace femcae::meshing::predicates
