#include "femcae/meshing/RobustGeometry.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using femcae::geometry::Vec3;
using femcae::meshing::AffineDimension;
using femcae::meshing::CanonicalSite;
using femcae::meshing::InputSite;
using femcae::meshing::PointId;

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        fail(message);
    }
}

std::uint64_t bits(double value) {
    return std::bit_cast<std::uint64_t>(value);
}

void verifyDuplicateCanonicalization() {
    const std::vector<InputSite> input{
        {{1.0, -0.0, 2.0}, 40},
        {{1.0, +0.0, 2.0}, 10},
        {{-2.0, 3.0, 4.0}, 30},
        {{1.0, -0.0, 2.0}, 20},
    };

    const std::vector<CanonicalSite> sites = femcae::meshing::canonicalizeSites(input);
    require(sites.size() == 2U, "exact duplicate canonicalization failed");

    const auto duplicate = std::find_if(
        sites.begin(), sites.end(),
        [](const CanonicalSite& site) { return site.point.x == 1.0; });
    require(duplicate != sites.end(), "canonical duplicate site missing");
    require(bits(duplicate->point.y) == 0ULL, "signed zero was not normalized to +0");
    require(
        duplicate->sourceRecordIds == std::vector<std::uint64_t>({10, 20, 40}),
        "duplicate provenance is not deterministic");
}

void verifyInputOrderIndependence() {
    std::vector<InputSite> a{
        {{2.0, 0.0, 0.0}, 2},
        {{-1.0, 0.0, 0.0}, 1},
        {{0.0, 1.0, 0.0}, 3},
        {{2.0, -0.0, 0.0}, 4},
    };
    std::vector<InputSite> b = a;
    std::reverse(b.begin(), b.end());

    const auto lhs = femcae::meshing::canonicalizeSites(a);
    const auto rhs = femcae::meshing::canonicalizeSites(b);
    require(lhs.size() == rhs.size(), "canonical site count depends on input order");

    for (std::size_t i = 0; i < lhs.size(); ++i) {
        require(lhs[i].id == rhs[i].id, "PointId depends on input order");
        require(bits(lhs[i].point.x) == bits(rhs[i].point.x), "x ordering mismatch");
        require(bits(lhs[i].point.y) == bits(rhs[i].point.y), "y ordering mismatch");
        require(bits(lhs[i].point.z) == bits(rhs[i].point.z), "z ordering mismatch");
        require(lhs[i].sourceRecordIds == rhs[i].sourceRecordIds, "provenance order mismatch");
    }
}

std::vector<CanonicalSite> canonical(std::initializer_list<Vec3> points) {
    std::vector<InputSite> input;
    std::uint64_t sourceId = 1;
    for (const Vec3& point : points) {
        input.push_back({point, sourceId++});
    }
    return femcae::meshing::canonicalizeSites(input);
}

void verifyAffineDimension() {
    const std::vector<CanonicalSite> empty;
    require(
        femcae::meshing::classifyAffineDimension(empty) == AffineDimension::Empty,
        "empty affine dimension failed");

    auto point = canonical({{1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}});
    require(point.size() == 1U, "duplicate point was not canonicalized");
    require(
        femcae::meshing::classifyAffineDimension(point) == AffineDimension::Zero,
        "0D affine dimension failed");

    auto line = canonical({
        {0.0, 0.0, 0.0},
        {1.0, 2.0, 3.0},
        {2.0, 4.0, 6.0},
        {-1.0, -2.0, -3.0}});
    require(
        femcae::meshing::classifyAffineDimension(line) == AffineDimension::One,
        "1D affine dimension failed");

    auto plane = canonical({
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {2.0, 3.0, 0.0}});
    require(
        femcae::meshing::classifyAffineDimension(plane) == AffineDimension::Two,
        "2D affine dimension failed");

    auto volume = canonical({
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.25, 0.25, std::ldexp(1.0, -200)}});
    require(
        femcae::meshing::classifyAffineDimension(volume) == AffineDimension::Three,
        "near-coplanar exact 3D set was collapsed to 2D");
}

void verifyInvalidInput() {
    const double infinity = std::numeric_limits<double>::infinity();
    bool rejected = false;
    try {
        const std::vector<InputSite> bad{{{infinity, 0.0, 0.0}, 1}};
        (void)femcae::meshing::canonicalizeSites(bad);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "canonicalizeSites accepted infinity");
}

} // namespace

int main() {
    try {
        verifyDuplicateCanonicalization();
        verifyInputOrderIndependence();
        verifyAffineDimension();
        verifyInvalidInput();
        std::cout << "M1.9-B robust-geometry sites PASS "
                     "duplicates=yes pointid=stable affine=exact signed_zero=yes\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "M1.9-B robust-geometry sites FAIL: " << error.what() << '\n';
        return 1;
    }
}
