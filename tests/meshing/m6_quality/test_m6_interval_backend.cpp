#include "meshing/m6/quality/IntervalArithmetic.h"

#include <bit>
#include <cfenv>
#include <cmath>
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

namespace d26int = femcae::meshing::m6::quality::interval;

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

double parseDouble(std::string_view text) {
    return std::bit_cast<double>(parseHex(text));
}

int parseInt(std::string_view text) {
    int value = 0;
    const auto parsed =
        std::from_chars(text.data(), text.data() + text.size(), value);
    require(
        parsed.ec == std::errc{} &&
            parsed.ptr == text.data() + text.size(),
        "invalid fixture integer");
    return value;
}

d26int::IntervalResult evaluate(
    const std::vector<std::string>& fields) {
    const d26int::Interval a{
        parseDouble(fields[2]), parseDouble(fields[3])};
    const d26int::Interval b{
        parseDouble(fields[4]), parseDouble(fields[5])};
    const int parameter = parseInt(fields[6]);
    const std::string& op = fields[1];

    if (op == "add") return d26int::add(a, b);
    if (op == "sub") return d26int::subtract(a, b);
    if (op == "mul") return d26int::multiply(a, b);
    if (op == "square") return d26int::square(a);
    if (op == "cube") return d26int::cubePositive(a);
    if (op == "scale") return d26int::scalePowerOfTwo(a, parameter);
    fail("unknown D26INT1 fixture operation");
}

std::size_t verifyFixture(const std::filesystem::path& path) {
    std::ifstream input(path);
    require(input.good(), "cannot open D26INT1 fixture");

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
        require(fields.size() == 9U, "D26INT1 fixture field count mismatch");
        const d26int::IntervalResult actual = evaluate(fields);
        require(actual.ok(), fields[0] + " unexpectedly returned uncertain");

        const double expectedLo = parseDouble(fields[7]);
        const double expectedHi = parseDouble(fields[8]);
        require(
            actual.value.lo <= expectedLo,
            fields[0] + " lower containment failure");
        require(
            actual.value.hi >= expectedHi,
            fields[0] + " upper containment failure");
        ++cases;
    }

    require(cases > 100U, "D26INT1 fixture is unexpectedly small");
    return cases;
}

void verifyEnvironment() {
    const d26int::EnvironmentProbe baseline = d26int::probeEnvironment();
    require(baseline.binary64, "D26INT1 requires IEC-60559 binary64");
    require(baseline.roundToNearest, "test must start in FE_TONEAREST");
    require(
        baseline.gradualSubnormalOutput,
        "subnormal output probe failed");
    require(
        baseline.gradualSubnormalInput,
        "subnormal input probe failed");
    require(baseline.supported(), "qualified D26INT1 environment unavailable");

    const int original = std::fegetround();
    const int alternateModes[] = {
        FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO};

    for (int mode : alternateModes) {
        if (std::fesetround(mode) == 0) {
            const d26int::EnvironmentProbe alternate =
                d26int::probeEnvironment();
            require(
                !alternate.supported(),
                "alternate rounding mode incorrectly qualified D26INT1");
            require(
                !alternate.roundToNearest,
                "alternate rounding mode guard failed");
        }
    }

    require(
        std::fesetround(original) == 0,
        "failed to restore rounding mode");
    require(
        d26int::probeEnvironment().supported(),
        "D26INT1 environment did not recover after rounding test");
}

void verifyAdjacencyAndRanges() {
    const double positiveZero = 0.0;
    const double negativeZero = -0.0;
    require(
        std::bit_cast<std::uint64_t>(
            d26int::nextDown(positiveZero)) == 0x8000000000000001ULL,
        "nextDown(+0) mismatch");
    require(
        std::bit_cast<std::uint64_t>(
            d26int::nextUp(positiveZero)) == 0x0000000000000001ULL,
        "nextUp(+0) mismatch");
    require(
        std::bit_cast<std::uint64_t>(
            d26int::nextDown(negativeZero)) == 0x8000000000000001ULL,
        "nextDown(-0) mismatch");
    require(
        std::bit_cast<std::uint64_t>(
            d26int::nextUp(negativeZero)) == 0x0000000000000001ULL,
        "nextUp(-0) mismatch");

    const double minSubnormal =
        std::numeric_limits<double>::denorm_min();
    const double minNormal =
        std::numeric_limits<double>::min();

    require(
        d26int::nextDown(minSubnormal) == 0.0,
        "nextDown(min subnormal) must reach zero");
    require(
        std::bit_cast<std::uint64_t>(
            d26int::nextDown(minNormal)) == 0x000fffffffffffffULL,
        "nextDown(min normal) must reach max subnormal");

    const double maxFinite =
        std::numeric_limits<double>::max();
    require(
        std::isfinite(d26int::nextDown(maxFinite)),
        "nextDown(max finite) must remain finite");
    require(
        std::isinf(d26int::nextUp(maxFinite)),
        "nextUp(max finite) must reach infinity");

    const auto overflow =
        d26int::multiply({maxFinite, maxFinite}, {2.0, 2.0});
    require(
        !overflow.ok() &&
            overflow.failure == d26int::IntervalFailure::Range,
        "overflow must return D26INT1 range uncertainty");

    const auto invalid =
        d26int::add({2.0, 1.0}, {0.0, 0.0});
    require(
        !invalid.ok() &&
            invalid.failure == d26int::IntervalFailure::InvalidInput,
        "unordered interval must be rejected");

    int exponent = 0;
    require(
        d26int::normalizationExponent(minSubnormal, exponent),
        "subnormal normalization exponent failed");
    require(
        exponent == 1073,
        "subnormal normalization exponent mismatch");

    const auto normalized =
        d26int::scalePowerOfTwo(
            {minSubnormal, minSubnormal}, exponent);
    require(
        normalized.ok() &&
            normalized.value.lo == 0.5 &&
            normalized.value.hi == 0.5,
        "subnormal power-of-two normalization mismatch");

    require(
        d26int::normalizationExponent(maxFinite, exponent),
        "max-finite normalization exponent failed");
    const double scaled = std::scalbn(maxFinite, exponent);
    require(
        scaled >= 0.5 && scaled < 1.0,
        "max-finite normalization target mismatch");

    const auto downscaleSubnormal =
        d26int::scalePowerOfTwo(
            {minNormal, minNormal}, -1);
    require(
        !downscaleSubnormal.ok() &&
            downscaleSubnormal.failure == d26int::IntervalFailure::Range,
        "subnormal downscale must conservatively return range uncertainty");
}

} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "generated D26INT1 fixture path required");
        verifyEnvironment();
        verifyAdjacencyAndRanges();
        const std::size_t cases = verifyFixture(argv[1]);

        std::cout
            << "M6 I2a interval backend PASS"
            << " cases=" << cases
            << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr
            << "M6 I2a interval backend FAIL: "
            << error.what()
            << '\n';
        return 1;
    }
}
