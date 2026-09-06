#include "meshing/m6/quality/MeanRatioFilter.h"

#include <bit>
#include <cfenv>
#include <charconv>
#include <cmath>
#include <utility>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace q = femcae::meshing::m6::quality;

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

std::uint64_t parseUnsigned(std::string_view text) {
    std::uint64_t value = 0U;
    const auto result =
        std::from_chars(
            text.data(),
            text.data() + text.size(),
            value);
    require(
        result.ec == std::errc{} &&
            result.ptr == text.data() + text.size(),
        "invalid unsigned fixture value");
    return value;
}

int parseInt(std::string_view text) {
    int value = 0;
    const auto result =
        std::from_chars(
            text.data(),
            text.data() + text.size(),
            value);
    require(
        result.ec == std::errc{} &&
            result.ptr == text.data() + text.size(),
        "invalid integer fixture value");
    return value;
}

double parseHexDouble(std::string_view text) {
    require(text.size() == 16U, "fixture hex width mismatch");
    std::uint64_t bits = 0U;
    const auto result =
        std::from_chars(
            text.data(),
            text.data() + text.size(),
            bits,
            16);
    require(
        result.ec == std::errc{} &&
            result.ptr == text.data() + text.size(),
        "invalid fixture binary64 bits");
    return std::bit_cast<double>(bits);
}

q::IndexedTetraCoordinates parseTetra(
    const std::vector<std::string>& fields,
    std::size_t begin) {
    q::IndexedTetraCoordinates tetra;
    for (std::size_t i = 0; i < 4U; ++i) {
        tetra.pointIds[i] =
            parseUnsigned(fields[begin + i]);
    }

    std::size_t cursor = begin + 4U;
    for (std::size_t i = 0; i < 4U; ++i) {
        tetra.coordinates[i] = {
            parseHexDouble(fields[cursor]),
            parseHexDouble(fields[cursor + 1U]),
            parseHexDouble(fields[cursor + 2U])};
        cursor += 3U;
    }
    return tetra;
}

// Exact dyadic comparison of `value` against `reference * 2^exponent`.
//
// The obvious implementation scales the oracle endpoint with std::scalbn and
// compares doubles, but that rounds: the scaled reference can land above the
// true product and turn the upper containment check into a weaker requirement
// than the one intended, and it underflows silently to zero for the large
// exponents this corpus reaches (6*(e_lhs + e_rhs)). A containment check must
// not itself be approximate, so the comparison is done on integer mantissas.
//
// Returns -1, 0 or 1 for value <, == or > reference * 2^exponent.
int compareAgainstScaled(
    double value,
    double reference,
    int exponent) {
    const auto decompose = [](double x) {
        if (x == 0.0) {
            return std::pair<std::int64_t, int>{0, 0};
        }
        int binaryExponent = 0;
        const double fraction = std::frexp(x, &binaryExponent);
        return std::pair<std::int64_t, int>{
            static_cast<std::int64_t>(std::ldexp(fraction, 53)),
            binaryExponent - 53};
    };

    auto [valueMantissa, valueExponent] = decompose(value);
    auto [referenceMantissa, referenceExponent] = decompose(reference);
    referenceExponent += exponent;

    if (valueMantissa == 0 && referenceMantissa == 0) {
        return 0;
    }
    if (valueMantissa == 0) {
        return referenceMantissa > 0 ? -1 : 1;
    }
    if (referenceMantissa == 0) {
        return valueMantissa > 0 ? 1 : -1;
    }
    if ((valueMantissa > 0) != (referenceMantissa > 0)) {
        return valueMantissa > 0 ? 1 : -1;
    }

    // Both non-zero and same sign. Align exponents; a difference beyond the
    // 54-bit significand width already decides the comparison by magnitude.
    __int128 lhs = valueMantissa;
    __int128 rhs = referenceMantissa;
    const int shift = valueExponent - referenceExponent;
    if (shift > 0) {
        if (shift > 64) {
            return valueMantissa > 0 ? 1 : -1;
        }
        lhs <<= shift;
    } else if (shift < 0) {
        if (shift < -64) {
            return valueMantissa > 0 ? -1 : 1;
        }
        rhs <<= -shift;
    }
    return lhs < rhs ? -1 : (lhs > rhs ? 1 : 0);
}

void requireContainsScaled(
    const q::interval::Interval& actual,
    double expectedLo,
    double expectedHi,
    int exponent,
    const std::string& label) {
    require(
        compareAgainstScaled(actual.lo, expectedLo, exponent) <= 0,
        label + " lower containment failure");
    require(
        compareAgainstScaled(actual.hi, expectedHi, exponent) >= 0,
        label + " upper containment failure");
}

void verifyFinalOnlyWidenNegativeControl() {
    // Exact: 1 + 4*(2^-53) = 1 + 2^-51.
    // RN at every addition loses each half-ulp tie; widening only once at the
    // end reaches merely one ulp and therefore misses the exact result.
    const double halfUlp = std::ldexp(1.0, -53);
    double unsafe = 1.0;
    for (int i = 0; i < 4; ++i) {
        unsafe += halfUlp;
    }

    const q::interval::Interval finalOnly{
        q::interval::nextDown(unsafe),
        q::interval::nextUp(unsafe)};
    const double exact =
        1.0 + std::ldexp(1.0, -51);

    require(
        exact > finalOnly.hi,
        "final-only widening negative control did not fail");
}

q::IndexedTetraCoordinates regularTetra() {
    return {{
        1U, 2U, 3U, 4U}, {{
        {0.0, 0.0, 0.0},
        {1.0, 0.1, 0.2},
        {0.1, 0.3, 1.2},
        {0.2, 1.1, 0.1}}}};
}

q::IndexedTetraCoordinates sliverTetra() {
    return {{
        11U, 12U, 13U, 14U}, {{
        {0.0, 0.0, 0.0},
        {1.0, 0.2, 0.1},
        {0.45, 0.45, 0.001},
        {0.1, 1.0, 0.2}}}};
}

void verifyEnvironmentFallback() {
    const auto lhs = regularTetra();
    const auto rhs = sliverTetra();
    const q::MeanRatioOrder exact =
        q::compareExactMeanRatio(
            lhs.coordinates,
            rhs.coordinates);

    const int original = std::fegetround();
    const int alternateModes[] = {
        FE_UPWARD,
        FE_DOWNWARD,
        FE_TOWARDZERO};

    for (int mode : alternateModes) {
        if (std::fesetround(mode) == 0) {
            q::MeanRatioFilterTelemetry telemetry;
            const q::MeanRatioEvaluation result =
                q::compareFilteredMeanRatio(
                    lhs,
                    rhs,
                    &telemetry);

            require(
                result.path ==
                    q::MeanRatioEvaluationPath::ExactFallback,
                "alternate rounding mode did not force exact fallback");
            require(
                result.fallbackReason ==
                    q::MeanRatioFallbackReason::UnsupportedEnvironment,
                "alternate rounding fallback reason mismatch");
            require(
                result.order == exact,
                "alternate rounding changed final exact order");
            require(
                telemetry.uncertainEnvironment == 1U &&
                    telemetry.exactFallback == 1U,
                "alternate rounding telemetry mismatch");
        }
    }

    require(
        std::fesetround(original) == 0,
        "failed to restore rounding mode");
}

void verifyCanonicalReplay() {
    const auto base = regularTetra();
    q::IndexedTetraCoordinates permuted;
    const std::size_t permutation[4] = {
        1U, 2U, 0U, 3U}; // even 3-cycle: Positive orientation korunur.

    for (std::size_t i = 0; i < 4U; ++i) {
        permuted.pointIds[i] =
            base.pointIds[permutation[i]];
        permuted.coordinates[i] =
            base.coordinates[permutation[i]];
    }

    q::MeanRatioIntervalKey baseKey;
    q::MeanRatioIntervalKey permutedKey;
    require(
        q::buildMeanRatioIntervalKey(
            base,
            baseKey) ==
            q::MeanRatioIntervalStatus::Ready,
        "canonical base interval key unavailable");
    require(
        q::buildMeanRatioIntervalKey(
            permuted,
            permutedKey) ==
            q::MeanRatioIntervalStatus::Ready,
        "canonical permuted interval key unavailable");

    const auto sameBits = [](double lhs, double rhs) {
        return std::bit_cast<std::uint64_t>(lhs) ==
               std::bit_cast<std::uint64_t>(rhs);
    };

    require(
        baseKey.normalizationExponent ==
            permutedKey.normalizationExponent,
        "canonical normalization replay mismatch");
    require(
        sameBits(
            baseKey.determinant.lo,
            permutedKey.determinant.lo) &&
            sameBits(
                baseKey.determinant.hi,
                permutedKey.determinant.hi),
        "canonical determinant replay mismatch");
    require(
        sameBits(
            baseKey.edgeSum.lo,
            permutedKey.edgeSum.lo) &&
            sameBits(
                baseKey.edgeSum.hi,
                permutedKey.edgeSum.hi),
        "canonical edge-sum replay mismatch");
}

void verifyTieFallback() {
    const auto tetra = regularTetra();
    q::MeanRatioFilterTelemetry telemetry;
    const q::MeanRatioEvaluation result =
        q::compareFilteredMeanRatio(
            tetra,
            tetra,
            &telemetry);

    require(
        result.order == q::MeanRatioOrder::Equal,
        "exact qMR tie order mismatch");
    require(
        result.path ==
            q::MeanRatioEvaluationPath::ExactFallback,
        "fast Equal is forbidden");
    require(
        result.fallbackReason ==
            q::MeanRatioFallbackReason::IntervalOverlap,
        "exact tie must fallback through interval overlap");
    require(
        telemetry.exactEqual == 1U &&
            telemetry.exactFallback == 1U,
        "exact tie telemetry mismatch");
}

void verifyInvalidIdentityFallback() {
    auto lhs = regularTetra();
    const auto rhs = sliverTetra();
    lhs.pointIds[3] = lhs.pointIds[2];

    const q::MeanRatioEvaluation result =
        q::compareFilteredMeanRatio(lhs, rhs);

    require(
        result.path ==
            q::MeanRatioEvaluationPath::ExactFallback,
        "invalid filter identity did not fallback");
    require(
        result.fallbackReason ==
            q::MeanRatioFallbackReason::InvalidFilterInput,
        "invalid filter identity fallback reason mismatch");
}

std::size_t verifyCorpus(
    const std::filesystem::path& path) {
    std::ifstream input(path);
    require(input.good(), "cannot open D26QMRF1 corpus");

    std::string line;
    std::size_t cases = 0U;
    std::size_t ready = 0U;
    std::size_t certified = 0U;
    std::size_t exactTies = 0U;
    q::MeanRatioFilterTelemetry telemetry;

    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line.front() == '#') {
            continue;
        }

        const std::vector<std::string> fields =
            splitTabs(line);
        require(
            fields.size() == 44U,
            "D26QMRF1 fixture field count mismatch");

        const int expectedOrder =
            parseInt(fields[1]);
        const q::IndexedTetraCoordinates lhs =
            parseTetra(fields, 2U);
        const q::IndexedTetraCoordinates rhs =
            parseTetra(fields, 18U);

        q::MeanRatioPairIntervals diagnostics;
        const q::MeanRatioIntervalStatus status =
            q::buildMeanRatioPairIntervals(
                lhs,
                rhs,
                diagnostics);

        if (status == q::MeanRatioIntervalStatus::Ready) {
            ++ready;
            requireContainsScaled(
                diagnostics.lhs.determinant,
                parseHexDouble(fields[34]),
                parseHexDouble(fields[35]),
                3 * diagnostics.lhs.normalizationExponent,
                line + " lhs D");
            requireContainsScaled(
                diagnostics.lhs.edgeSum,
                parseHexDouble(fields[36]),
                parseHexDouble(fields[37]),
                2 * diagnostics.lhs.normalizationExponent,
                line + " lhs S");
            requireContainsScaled(
                diagnostics.rhs.determinant,
                parseHexDouble(fields[38]),
                parseHexDouble(fields[39]),
                3 * diagnostics.rhs.normalizationExponent,
                line + " rhs D");
            requireContainsScaled(
                diagnostics.rhs.edgeSum,
                parseHexDouble(fields[40]),
                parseHexDouble(fields[41]),
                2 * diagnostics.rhs.normalizationExponent,
                line + " rhs S");
            requireContainsScaled(
                diagnostics.crossPolynomial,
                parseHexDouble(fields[42]),
                parseHexDouble(fields[43]),
                6 * (
                    diagnostics.lhs.normalizationExponent +
                    diagnostics.rhs.normalizationExponent),
                line + " F");
        }

        const q::MeanRatioEvaluation result =
            q::compareFilteredMeanRatio(
                lhs,
                rhs,
                &telemetry);

        require(
            static_cast<int>(result.order) ==
                expectedOrder,
            line +
                " final order disagrees independent exact oracle");

        if (result.path ==
            q::MeanRatioEvaluationPath::IntervalCertified) {
            require(
                expectedOrder != 0,
                line + " fast Equal is forbidden");
            ++certified;
        }

        if (expectedOrder == 0) {
            require(
                result.path ==
                    q::MeanRatioEvaluationPath::ExactFallback,
                line + " exact tie did not reach fallback");
            ++exactTies;
        }
        ++cases;
    }

    require(cases > 100U, "D26QMRF1 corpus too small");
    require(
        ready > 80U,
        "too few interval-ready D/S/F diagnostics");
    require(
        certified > 20U,
        "too few interval-certified ordinary comparisons");
    require(
        exactTies >= 2U,
        "D26QMRF1 exact-tie coverage missing");
    require(
        telemetry.calls == cases,
        "D26QMRF1 telemetry call count mismatch");
    require(
        telemetry.intervalCertifiedLess +
            telemetry.intervalCertifiedGreater +
            telemetry.exactFallback ==
            telemetry.calls,
        "D26QMRF1 telemetry result partition mismatch");

    return cases;
}

} // namespace

int main(int argc, char** argv) {
    try {
        require(
            argc == 2,
            "generated D26QMRF1 fixture path required");
        require(
            q::interval::probeEnvironment().supported(),
            "D26INT1 baseline environment unsupported");

        verifyFinalOnlyWidenNegativeControl();
        verifyCanonicalReplay();
        verifyTieFallback();
        verifyInvalidIdentityFallback();
        verifyEnvironmentFallback();

        const std::size_t cases =
            verifyCorpus(argv[1]);

        std::cout
            << "M6 I2b filtered qMR PASS"
            << " cases=" << cases
            << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr
            << "M6 I2b filtered qMR FAIL: "
            << error.what()
            << '\n';
        return 1;
    }
}
