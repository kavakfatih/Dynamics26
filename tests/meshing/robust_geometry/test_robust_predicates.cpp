#include "femcae/meshing/RobustPredicates.h"

#include <bit>
#include <charconv>
#include <cmath>
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

using femcae::geometry::Vec2;
using femcae::geometry::Vec3;
using femcae::meshing::predicates::PredicateEvaluation;
using femcae::meshing::predicates::PredicateEvaluationPath;
using femcae::meshing::predicates::PredicateSign;

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
    std::size_t begin = 0;
    for (;;) {
        const std::size_t tab = line.find('\t', begin);
        if (tab == std::string::npos) {
            fields.push_back(line.substr(begin));
            return fields;
        }
        fields.push_back(line.substr(begin, tab - begin));
        begin = tab + 1;
    }
}

std::uint64_t parseHex(std::string_view text) {
    require(text.size() == 16U, "fixture hex width mismatch");
    std::uint64_t value = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value, 16);
    require(parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size(),
            "invalid fixture hex");
    return value;
}

int parseExpected(std::string_view text) {
    if (text == "+1") return 1;
    if (text == "-1") return -1;
    if (text == "0") return 0;
    fail("invalid expected sign");
}

int toInt(PredicateSign sign) {
    return static_cast<int>(sign);
}

PredicateEvaluation evaluate(
    const std::string& predicate,
    const std::vector<double>& values) {
    using namespace femcae::meshing::predicates;

    if (predicate == "orient2d") {
        require(values.size() == 6U, "orient2d fixture arity mismatch");
        return orient2d(
            {values[0], values[1]},
            {values[2], values[3]},
            {values[4], values[5]});
    }
    if (predicate == "orient3d") {
        require(values.size() == 12U, "orient3d fixture arity mismatch");
        return orient3d(
            {values[0], values[1], values[2]},
            {values[3], values[4], values[5]},
            {values[6], values[7], values[8]},
            {values[9], values[10], values[11]});
    }
    if (predicate == "incircle") {
        require(values.size() == 8U, "incircle fixture arity mismatch");
        return incircle(
            {values[0], values[1]},
            {values[2], values[3]},
            {values[4], values[5]},
            {values[6], values[7]});
    }
    if (predicate == "insphere") {
        require(values.size() == 15U, "insphere fixture arity mismatch");
        return insphere(
            {values[0], values[1], values[2]},
            {values[3], values[4], values[5]},
            {values[6], values[7], values[8]},
            {values[9], values[10], values[11]},
            {values[12], values[13], values[14]});
    }
    fail("unsupported predicate fixture");
}

std::size_t verifyFile(
    const std::filesystem::path& path,
    const std::string& predicate) {
    std::ifstream input(path);
    require(input.good(), "cannot open fixture " + path.string());

    std::string line;
    std::size_t caseCount = 0;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line.front() == '#') {
            continue;
        }

        const std::vector<std::string> fields = splitTabs(line);
        require(fields.size() >= 5U, "fixture case too short");

        const std::string& caseId = fields[0];
        const int expected = parseExpected(fields[2]);
        std::vector<double> values;
        values.reserve(fields.size() - 4U);
        for (std::size_t i = 4U; i < fields.size(); ++i) {
            const std::uint64_t bits = parseHex(fields[i]);
            values.push_back(std::bit_cast<double>(bits));
        }

        const PredicateEvaluation result = evaluate(predicate, values);
        if (toInt(result.sign) != expected) {
            fail(
                predicate + "/" + caseId +
                " expected=" + std::to_string(expected) +
                " actual=" + std::to_string(toInt(result.sign)));
        }
        require(
            result.path == PredicateEvaluationPath::ExactDyadic,
            predicate + "/" + caseId + " did not use M1.7 exact path");
        ++caseCount;
    }
    require(caseCount > 0U, "fixture file contains no cases");
    return caseCount;
}

void verifyInvalidInput() {
    using namespace femcae::meshing::predicates;
    const double infinity = std::numeric_limits<double>::infinity();

    bool rejected = false;
    try {
        (void)orient2d({infinity, 0.0}, {1.0, 0.0}, {0.0, 1.0});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "orient2d accepted infinity");

    rejected = false;
    try {
        (void)insphere(
            {1.0, 0.0, 0.0},
            {0.0, 1.0, 0.0},
            {0.0, 0.0, 1.0},
            {0.0, 0.0, 0.0},
            {std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "insphere accepted NaN");
}

} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "fixture directory argument required");
        const std::filesystem::path directory(argv[1]);
        std::size_t total = 0;
        total += verifyFile(directory / "orient2d.d26pred", "orient2d");
        total += verifyFile(directory / "orient3d.d26pred", "orient3d");
        total += verifyFile(directory / "incircle.d26pred", "incircle");
        total += verifyFile(directory / "insphere.d26pred", "insphere");
        verifyInvalidInput();

        std::cout << "M1.7 robust-predicates PASS cases=" << total
                  << " exact_dyadic=yes invalid_input=yes\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "M1.7 robust-predicates FAIL: " << error.what() << '\n';
        return 1;
    }
}
