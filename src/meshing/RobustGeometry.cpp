#include "femcae/meshing/RobustGeometry.h"

#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace femcae::meshing {
namespace {

std::uint64_t canonicalCoordinateBits(double value) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument("meshing site coordinates must be finite");
    }
    std::uint64_t bits = std::bit_cast<std::uint64_t>(value);
    if ((bits & 0x7FFFFFFFFFFFFFFFULL) == 0ULL) {
        bits = 0ULL;
    }
    return bits;
}

double coordinateFromBits(std::uint64_t bits) {
    return std::bit_cast<double>(bits);
}

using SiteKey = std::array<std::uint64_t, 3>;

SiteKey siteKey(const geometry::Vec3& point) {
    return {
        canonicalCoordinateBits(point.x),
        canonicalCoordinateBits(point.y),
        canonicalCoordinateBits(point.z)};
}

bool collinear(
    const geometry::Vec3& a,
    const geometry::Vec3& b,
    const geometry::Vec3& c) {
    using predicates::PredicateSign;
    const auto xy = predicates::orient2d({a.x, a.y}, {b.x, b.y}, {c.x, c.y});
    if (xy.sign != PredicateSign::Zero) {
        return false;
    }
    const auto xz = predicates::orient2d({a.x, a.z}, {b.x, b.z}, {c.x, c.z});
    if (xz.sign != PredicateSign::Zero) {
        return false;
    }
    const auto yz = predicates::orient2d({a.y, a.z}, {b.y, b.z}, {c.y, c.z});
    return yz.sign == PredicateSign::Zero;
}

} // namespace

std::vector<CanonicalSite> canonicalizeSites(std::span<const InputSite> input) {
    struct Aggregate {
        SiteKey key{};
        std::vector<std::uint64_t> sourceRecordIds;
    };

    std::map<SiteKey, Aggregate> groups;
    for (const InputSite& site : input) {
        const SiteKey key = siteKey(site.point);
        auto [iterator, inserted] = groups.try_emplace(key);
        if (inserted) {
            iterator->second.key = key;
        }
        iterator->second.sourceRecordIds.push_back(site.sourceRecordId);
    }

    std::vector<CanonicalSite> result;
    result.reserve(groups.size());

    PointId nextId = 1;
    for (auto& [key, aggregate] : groups) {
        std::sort(aggregate.sourceRecordIds.begin(), aggregate.sourceRecordIds.end());

        CanonicalSite site;
        site.id = nextId++;
        site.point = {
            coordinateFromBits(key[0]),
            coordinateFromBits(key[1]),
            coordinateFromBits(key[2])};
        site.sourceRecordIds = std::move(aggregate.sourceRecordIds);
        result.push_back(std::move(site));
    }
    return result;
}

AffineDimension classifyAffineDimension(std::span<const CanonicalSite> sites) {
    using predicates::PredicateSign;

    if (sites.empty()) {
        return AffineDimension::Empty;
    }
    if (sites.size() == 1U) {
        return AffineDimension::Zero;
    }

    const geometry::Vec3& a = sites[0].point;
    const geometry::Vec3& b = sites[1].point;

    std::size_t third = sites.size();
    for (std::size_t i = 2U; i < sites.size(); ++i) {
        if (!collinear(a, b, sites[i].point)) {
            third = i;
            break;
        }
    }
    if (third == sites.size()) {
        return AffineDimension::One;
    }

    const geometry::Vec3& c = sites[third].point;
    for (std::size_t i = 2U; i < sites.size(); ++i) {
        if (i == third) {
            continue;
        }
        const auto orientation = predicates::orient3d(a, b, c, sites[i].point);
        if (orientation.sign != PredicateSign::Zero) {
            return AffineDimension::Three;
        }
    }
    return AffineDimension::Two;
}

} // namespace femcae::meshing
