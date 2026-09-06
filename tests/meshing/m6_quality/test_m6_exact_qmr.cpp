#include "meshing/m6/quality/MeanRatioExact.h"
#include "meshing/internal/exact/ExactDyadicArithmetic.h"

#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using femcae::geometry::Vec3;
using femcae::meshing::m6::quality::ExactMeanRatioTelemetry;
using femcae::meshing::m6::quality::MeanRatioOrder;
using femcae::meshing::m6::quality::TetraCoordinates;

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        fail(message);
    }
}

std::vector<std::string> splitTabs(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t begin = 0U;
    for (;;) {
        const std::size_t tab = line.find('\t', begin);
        if (tab == std::string::npos) {
            fields.push_back(line.substr(begin));
            return fields;
        }
        fields.push_back(line.substr(begin, tab - begin));
        begin = tab + 1U;
    }
}

std::uint64_t parseHex(std::string_view text) {
    require(text.size() == 16U, "fixture hex width mismatch");
    std::uint64_t value = 0U;
    const auto parsed =
        std::from_chars(text.data(), text.data() + text.size(), value, 16);
    require(
        parsed.ec == std::errc{} &&
            parsed.ptr == text.data() + text.size(),
        "invalid fixture hex");
    return value;
}

int parseExpected(std::string_view text) {
    if (text == "+1") return 1;
    if (text == "-1") return -1;
    if (text == "0") return 0;
    fail("invalid expected order");
}

int toInt(MeanRatioOrder order) {
    return static_cast<int>(order);
}

int toInt(femcae::meshing::predicates::PredicateSign sign) {
    return static_cast<int>(sign);
}

TetraCoordinates tetraFromFields(
    const std::vector<std::string>& fields,
    std::size_t firstField) {
    TetraCoordinates tetra{};
    for (std::size_t vertex = 0U; vertex < 4U; ++vertex) {
        const std::size_t base = firstField + 3U * vertex;
        tetra[vertex] = {
            std::bit_cast<double>(parseHex(fields[base])),
            std::bit_cast<double>(parseHex(fields[base + 1U])),
            std::bit_cast<double>(parseHex(fields[base + 2U]))};
    }
    return tetra;
}

int genericExactOrientSign(const TetraCoordinates& tetra) {
    using femcae::meshing::internal::exact::BigInt;
    using femcae::meshing::internal::exact::Matrix;

    std::vector<double> coordinates;
    coordinates.reserve(12U);
    for (const Vec3& point : tetra) {
        coordinates.push_back(point.x);
        coordinates.push_back(point.y);
        coordinates.push_back(point.z);
    }

    const std::vector<BigInt> p =
        femcae::meshing::internal::exact::exactIntegerCoordinates(coordinates);

    Matrix matrix{
        {p[0], p[1], p[2], BigInt::one()},
        {p[3], p[4], p[5], BigInt::one()},
        {p[6], p[7], p[8], BigInt::one()},
        {p[9], p[10], p[11], BigInt::one()}};
    return femcae::meshing::internal::exact::determinant(matrix).sign();
}

void verifyDeterminantAuthority(const TetraCoordinates& tetra) {
    const int fixed =
        femcae::meshing::m6::quality::exactRelativeDeterminantSign(tetra);
    const int generic = genericExactOrientSign(tetra);
    const auto publicResult = femcae::meshing::predicates::orient3d(
        tetra[0], tetra[1], tetra[2], tetra[3]);

    require(fixed != 0, "fixture unexpectedly degenerate");
    require(fixed == generic, "fixed 3x3 determinant disagrees with generic exact determinant");
    require(
        fixed == toInt(publicResult.sign),
        "fixed 3x3 determinant disagrees with M1 Orient3D");
}

std::size_t verifyFixture(
    const std::filesystem::path& path,
    ExactMeanRatioTelemetry& telemetry) {
    std::ifstream input(path);
    require(input.good(), "cannot open D26QMR1 fixture");

    std::string line;
    std::size_t cases = 0U;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line.front() == '#') {
            continue;
        }

        const std::vector<std::string> fields = splitTabs(line);
        require(fields.size() == 26U, "D26QMR1 fixture field count mismatch");

        const std::string& caseId = fields[0];
        const int expected = parseExpected(fields[1]);
        const TetraCoordinates lhs = tetraFromFields(fields, 2U);
        const TetraCoordinates rhs = tetraFromFields(fields, 14U);

        verifyDeterminantAuthority(lhs);
        verifyDeterminantAuthority(rhs);

        const MeanRatioOrder actual =
            femcae::meshing::m6::quality::compareExactMeanRatio(
                lhs, rhs, &telemetry);
        if (toInt(actual) != expected) {
            fail(
                caseId + " expected=" + std::to_string(expected) +
                " actual=" + std::to_string(toInt(actual)));
        }

        const MeanRatioOrder reverse =
            femcae::meshing::m6::quality::compareExactMeanRatio(
                rhs, lhs, &telemetry);
        require(
            toInt(reverse) == -expected,
            caseId + " reverse comparison mismatch");

        ++cases;
    }

    require(cases > 100U, "D26QMR1 generated corpus is unexpectedly small");
    return cases;
}

void verifyAllPermutations(ExactMeanRatioTelemetry& telemetry) {
    const TetraCoordinates base{{
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0}}};

    std::array<std::size_t, 4> order{0U, 1U, 2U, 3U};
    std::size_t count = 0U;
    do {
        TetraCoordinates candidate{};
        for (std::size_t i = 0U; i < 4U; ++i) {
            candidate[i] = base[order[i]];
        }

        verifyDeterminantAuthority(candidate);
        require(
            femcae::meshing::m6::quality::compareExactMeanRatio(
                base, candidate, &telemetry) == MeanRatioOrder::Equal,
            "vertex permutation changed exact mean-ratio key");
        ++count;
    } while (std::next_permutation(order.begin(), order.end()));

    require(count == 24U, "did not exercise all 24 tetra vertex permutations");
}

void verifyInvalidInputs(ExactMeanRatioTelemetry& telemetry) {
    const TetraCoordinates base{{
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0}}};

    const TetraCoordinates degenerate{{
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {1.0, 1.0, 0.0}}};

    bool rejected = false;
    try {
        (void)femcae::meshing::m6::quality::compareExactMeanRatio(
            base, degenerate, &telemetry);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "degenerate tetra entered D26QMR1 comparison");

    TetraCoordinates nonFinite = base;
    nonFinite[3].z = std::numeric_limits<double>::infinity();

    rejected = false;
    try {
        (void)femcae::meshing::m6::quality::compareExactMeanRatio(
            base, nonFinite, &telemetry);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "non-finite tetra entered D26QMR1 comparison");
}

void verifyWidthBounds(const ExactMeanRatioTelemetry& telemetry) {
    require(
        telemetry.maxDeterminantBits <= 6300U,
        "D26QMR1 determinant width exceeded research bound");
    require(
        telemetry.maxEdgeSumBits <= 4203U,
        "D26QMR1 edge-sum width exceeded research bound");
    require(
        telemetry.maxPowerBits <= 12609U,
        "D26QMR1 power width exceeded research bound");
    require(
        telemetry.maxCrossBits < 25210U,
        "D26QMR1 cross-product width exceeded research bound");
}

} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "generated D26QMR1 fixture path required");

        ExactMeanRatioTelemetry telemetry;
        const std::size_t cases = verifyFixture(argv[1], telemetry);
        verifyAllPermutations(telemetry);
        verifyInvalidInputs(telemetry);
        verifyWidthBounds(telemetry);

        require(telemetry.calls > 0U, "D26QMR1 telemetry recorded no calls");
        require(telemetry.exactEqual > 0U, "D26QMR1 exact-equality path untested");
        require(telemetry.invalidInput == 2U, "D26QMR1 invalid-input telemetry mismatch");

        std::cout
            << "M6 I1 exact-qMR PASS"
            << " cases=" << cases
            << " calls=" << telemetry.calls
            << " equal=" << telemetry.exactEqual
            << " invalid=" << telemetry.invalidInput
            << " d_bits=" << telemetry.maxDeterminantBits
            << " s_bits=" << telemetry.maxEdgeSumBits
            << " power_bits=" << telemetry.maxPowerBits
            << " cross_bits=" << telemetry.maxCrossBits
            << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "M6 I1 exact-qMR FAIL: " << error.what() << '\n';
        return 1;
    }
}
