#include <bit>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

struct FixtureCase {
    std::string id;
    std::string className;
    int expected = 0;
    std::uint64_t seed = 0;
    std::vector<std::uint64_t> coordinateBits;
};

struct FixtureFile {
    std::string predicate;
    std::size_t dimension = 0;
    std::size_t arity = 0;
    std::size_t coordinateCount = 0;
    std::string generator;
    std::vector<FixtureCase> cases;
};

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        fail(message);
    }
}

std::string stripCarriageReturn(std::string line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    return line;
}

std::vector<std::string> splitTabs(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    for (;;) {
        const std::size_t tab = line.find('\t', start);
        if (tab == std::string::npos) {
            fields.emplace_back(line.substr(start));
            return fields;
        }
        fields.emplace_back(line.substr(start, tab - start));
        start = tab + 1;
    }
}

std::uint64_t parseUnsignedDecimal(std::string_view text, const char* what) {
    require(!text.empty(), std::string("empty ") + what);
    std::uint64_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value, 10);
    require(result.ec == std::errc{} && result.ptr == text.data() + text.size(),
            std::string("invalid ") + what + ": " + std::string(text));
    return value;
}

std::size_t parseSize(std::string_view text, const char* what) {
    const std::uint64_t value = parseUnsignedDecimal(text, what);
    require(value <= static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()),
            std::string(what) + " exceeds size_t");
    return static_cast<std::size_t>(value);
}

std::uint64_t parseHex64(std::string_view text) {
    require(text.size() == 16, "binary64 field must contain exactly 16 hexadecimal digits");
    std::uint64_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value, 16);
    require(result.ec == std::errc{} && result.ptr == text.data() + text.size(),
            "invalid binary64 hexadecimal field");
    return value;
}

int parseExpected(std::string_view text) {
    if (text == "+1") return 1;
    if (text == "-1") return -1;
    if (text == "0") return 0;
    fail("expected sign must be -1, 0 or +1");
}

std::uint64_t canonicalCoordinateBits(std::uint64_t bits) {
    if ((bits & 0x7FFFFFFFFFFFFFFFULL) == 0) {
        return 0;
    }
    return bits;
}

FixtureFile parseFixture(std::istream& input, const std::string& expectedPredicate) {
    std::string line;
    require(static_cast<bool>(std::getline(input, line)), "empty fixture");
    line = stripCarriageReturn(std::move(line));
    require(line == "# D26PRED 1", "unsupported or missing D26PRED schema");

    std::unordered_map<std::string, std::string> headers;
    std::vector<std::string> dataLines;
    const std::set<std::string> allowedHeaders{
        "predicate", "encoding", "dimension", "arity", "coordinate_count", "generator"};

    while (std::getline(input, line)) {
        line = stripCarriageReturn(std::move(line));
        if (line.empty()) fail("blank lines are not permitted in D26PRED fixtures");

        if (line.rfind("# ", 0) == 0) {
            const std::string body = line.substr(2);
            const std::size_t equals = body.find('=');
            require(equals != std::string::npos && equals != 0 && equals + 1 < body.size(),
                    "malformed D26PRED header");
            const std::string key = body.substr(0, equals);
            const std::string value = body.substr(equals + 1);
            require(allowedHeaders.count(key) == 1, "unknown D26PRED header: " + key);
            require(headers.emplace(key, value).second, "duplicate D26PRED header: " + key);
        } else if (line.front() == '#') {
            fail("malformed D26PRED comment/header");
        } else {
            dataLines.push_back(line);
        }
    }

    for (const auto& key : allowedHeaders) {
        require(headers.count(key) == 1, "missing D26PRED header: " + key);
    }

    FixtureFile fixture;
    fixture.predicate = headers.at("predicate");
    fixture.dimension = parseSize(headers.at("dimension"), "dimension");
    fixture.arity = parseSize(headers.at("arity"), "arity");
    fixture.coordinateCount = parseSize(headers.at("coordinate_count"), "coordinate_count");
    fixture.generator = headers.at("generator");

    require(fixture.predicate == expectedPredicate, "predicate/header mismatch");
    require(headers.at("encoding") == "ieee754-binary64-bits-hex", "unsupported coordinate encoding");
    require(fixture.dimension > 0 && fixture.arity > 0, "invalid fixture dimension/arity");
    require(fixture.dimension * fixture.arity == fixture.coordinateCount,
            "coordinate_count does not match dimension*arity");
    require(!fixture.generator.empty(), "empty generator version");
    require(!dataLines.empty(), "fixture contains no cases");

    std::set<std::string> ids;
    for (const std::string& dataLine : dataLines) {
        const std::vector<std::string> fields = splitTabs(dataLine);
        require(fields.size() == 4 + fixture.coordinateCount,
                "fixture case has incorrect field count");

        FixtureCase parsed;
        parsed.id = fields[0];
        parsed.className = fields[1];
        require(!parsed.id.empty() && !parsed.className.empty(), "empty case id/class");
        require(ids.insert(parsed.id).second, "duplicate fixture case id: " + parsed.id);
        parsed.expected = parseExpected(fields[2]);
        parsed.seed = parseUnsignedDecimal(fields[3], "seed");
        parsed.coordinateBits.reserve(fixture.coordinateCount);

        for (std::size_t i = 0; i < fixture.coordinateCount; ++i) {
            const std::uint64_t bits = parseHex64(fields[4 + i]);
            const double value = std::bit_cast<double>(bits);
            require(std::isfinite(value), "non-finite coordinate in predicate truth fixture");
            require(std::bit_cast<std::uint64_t>(value) == bits,
                    "binary64 bit round-trip changed value");
            parsed.coordinateBits.push_back(bits);
        }
        fixture.cases.push_back(std::move(parsed));
    }
    return fixture;
}

FixtureFile parseFixtureText(const std::string& text, const std::string& expectedPredicate) {
    std::istringstream stream(text);
    return parseFixture(stream, expectedPredicate);
}

void expectRejected(const std::string& text, const std::string& predicate) {
    try {
        (void)parseFixtureText(text, predicate);
    } catch (const std::exception&) {
        return;
    }
    fail("malformed fixture was accepted");
}

std::string minimalFixture(const std::string& dataLine) {
    return "# D26PRED 1\n"
           "# predicate=orient2d\n"
           "# encoding=ieee754-binary64-bits-hex\n"
           "# dimension=2\n"
           "# arity=3\n"
           "# coordinate_count=6\n"
           "# generator=m1.6-test\n" +
           dataLine + "\n";
}

void runStrictParserNegativeTests() {
    expectRejected(
        "# D26PRED 2\n"
        "# predicate=orient2d\n"
        "# encoding=ieee754-binary64-bits-hex\n"
        "# dimension=2\n"
        "# arity=3\n"
        "# coordinate_count=6\n"
        "# generator=m1.6-test\n"
        "bad\tcanonical\t+1\t0\t0000000000000000\t0000000000000000\t"
        "3ff0000000000000\t0000000000000000\t0000000000000000\t3ff0000000000000\n",
        "orient2d");

    expectRejected(
        minimalFixture(
            "bad_hex\tcanonical\t+1\t0\t0\t0000000000000000\t3ff0000000000000\t"
            "0000000000000000\t0000000000000000\t3ff0000000000000"),
        "orient2d");

    expectRejected(
        minimalFixture(
            "bad_sign\tcanonical\t1\t0\t0000000000000000\t0000000000000000\t"
            "3ff0000000000000\t0000000000000000\t0000000000000000\t3ff0000000000000"),
        "orient2d");

    expectRejected(
        minimalFixture(
            "inf\tcanonical\t+1\t0\t7ff0000000000000\t0000000000000000\t"
            "3ff0000000000000\t0000000000000000\t0000000000000000\t3ff0000000000000"),
        "orient2d");

    const std::string duplicate =
        "# D26PRED 1\n"
        "# predicate=orient2d\n"
        "# encoding=ieee754-binary64-bits-hex\n"
        "# dimension=2\n"
        "# arity=3\n"
        "# coordinate_count=6\n"
        "# generator=m1.6-test\n"
        "same\tcanonical\t+1\t0\t0000000000000000\t0000000000000000\t"
        "3ff0000000000000\t0000000000000000\t0000000000000000\t3ff0000000000000\n"
        "same\tcanonical\t+1\t0\t0000000000000000\t0000000000000000\t"
        "3ff0000000000000\t0000000000000000\t0000000000000000\t3ff0000000000000\n";
    expectRejected(duplicate, "orient2d");
}

}  // namespace

int main(int argc, char** argv) {
    static_assert(sizeof(double) == sizeof(std::uint64_t));
    static_assert(std::numeric_limits<double>::is_iec559);
    static_assert(std::numeric_limits<double>::radix == 2);
    static_assert(std::numeric_limits<double>::digits == 53);

#ifdef __FAST_MATH__
#error "M1.6 robust-geometry verification must not be compiled with fast-math"
#endif

    try {
        require(argc == 2, "fixture directory argument required");
        const std::filesystem::path fixtureDirectory(argv[1]);
        const std::vector<std::string> predicates{"orient2d", "orient3d", "incircle", "insphere"};

        std::size_t totalCases = 0;
        bool negativeZeroSeen = false;

        for (const std::string& predicate : predicates) {
            const std::filesystem::path path = fixtureDirectory / (predicate + ".d26pred");
            std::ifstream input(path);
            require(input.good(), "cannot open fixture: " + path.string());
            const FixtureFile fixture = parseFixture(input, predicate);
            totalCases += fixture.cases.size();

            for (const FixtureCase& fixtureCase : fixture.cases) {
                for (const std::uint64_t bits : fixtureCase.coordinateBits) {
                    if (bits == 0x8000000000000000ULL) {
                        negativeZeroSeen = true;
                        require(canonicalCoordinateBits(bits) == 0,
                                "negative zero did not normalize to canonical +0");
                    }
                }
            }
        }

        require(totalCases >= 20, "unexpectedly small committed predicate corpus");
        require(negativeZeroSeen, "signed-zero fixture coverage missing");
        require(canonicalCoordinateBits(0) == 0, "positive zero canonicalization changed");
        require(canonicalCoordinateBits(0x3FF0000000000000ULL) == 0x3FF0000000000000ULL,
                "non-zero coordinate was modified by zero canonicalization");

        runStrictParserNegativeTests();

        std::cout << "M1.6 fixture-reader PASS cases=" << totalCases
                  << " predicates=4 bit_roundtrip=yes strict_parser=yes signed_zero=yes\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "M1.6 fixture-reader FAIL: " << error.what() << '\n';
        return 1;
    }
}
