#pragma once

#include "femcae/geometry/GeometryTypes.h"

#include <cstdint>
#include <span>
#include <vector>

namespace femcae::meshing {

using PointId = std::uint64_t;
inline constexpr PointId InvalidPointId = 0;

struct InputSite {
    geometry::Vec3 point;
    std::uint64_t sourceRecordId{0};
};

struct CanonicalSite {
    PointId id{InvalidPointId};
    geometry::Vec3 point;
    std::vector<std::uint64_t> sourceRecordIds;
};

enum class AffineDimension : std::int8_t {
    Empty = -1,
    Zero = 0,
    One = 1,
    Two = 2,
    Three = 3
};

[[nodiscard]] std::vector<CanonicalSite> canonicalizeSites(
    std::span<const InputSite> input);

[[nodiscard]] AffineDimension classifyAffineDimension(
    std::span<const CanonicalSite> sites);

} // namespace femcae::meshing
