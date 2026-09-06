#include "meshing/m6/quality/QualityAcceptance.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
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
        "invalid integer fixture field");
    return value;
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

std::vector<int> parseSymbols(const std::string& text) {
    if (text == "-") {
        return {};
    }

    std::vector<int> result;
    std::size_t begin = 0U;
    for (;;) {
        const std::size_t comma = text.find(',', begin);
        const std::size_t end =
            comma == std::string::npos
                ? text.size()
                : comma;
        result.push_back(
            parseInt(
                std::string_view(text).substr(
                    begin,
                    end - begin)));

        if (comma == std::string::npos) {
            return result;
        }
        begin = comma + 1U;
    }
}

const std::array<q::TetraCoordinates, 4>& alphabet() {
    static const std::array<q::TetraCoordinates, 4> value{{
        {{
            {0.0, 0.0, 0.0},
            {1.0, 0.0, 0.0},
            {0.0, 0.0, 1.0},
            {0.0, 1.0, 0.0},
        }},
        {{
            {0.0, 0.0, 0.0},
            {1.0, 0.1, 0.2},
            {0.1, 0.3, 1.2},
            {0.2, 1.1, 0.1},
        }},
        {{
            {0.0, 0.0, 0.0},
            {1.0, 0.2, 0.1},
            {0.1, 1.0, 0.2},
            {0.45, 0.45, 0.001},
        }},
        {{
            {0.0, 0.0, 0.0},
            {2.0, 0.1, 0.2},
            {0.2, 0.3, 0.8},
            {0.3, 0.4, 0.1},
        }},
    }};
    return value;
}

std::vector<q::IndexedTetraCoordinates> cellsFromSymbols(
    const std::vector<int>& symbols,
    femcae::meshing::PointId identityBase = 1U) {
    std::vector<q::IndexedTetraCoordinates> cells;
    cells.reserve(symbols.size());

    for (std::size_t i = 0; i < symbols.size(); ++i) {
        const int symbol = symbols[i];
        require(
            symbol >= 0 &&
                symbol < static_cast<int>(alphabet().size()),
            "quality-vector symbol out of range");

        const femcae::meshing::PointId first =
            identityBase +
            static_cast<femcae::meshing::PointId>(i) * 10U;

        cells.push_back({
            {first, first + 1U, first + 2U, first + 3U},
            alphabet()[static_cast<std::size_t>(symbol)]});
    }

    return cells;
}

q::QualityVector buildFromSymbols(
    const std::vector<int>& symbols,
    femcae::meshing::PointId identityBase = 1U) {
    const auto cells =
        cellsFromSymbols(symbols, identityBase);
    return q::buildQualityVector(cells);
}

struct OracleData {
    std::vector<std::vector<int>> definitions;
    std::vector<std::vector<int>> pairExpected;
    struct UnionCase {
        int oldId{0};
        int newId{0};
        int commonId{0};
        int localExpected{0};
        int unionExpected{0};
    };
    std::vector<UnionCase> unions;
};

OracleData readOracle(const std::filesystem::path& path) {
    std::ifstream input(path);
    require(input.good(), "cannot open D26QV1 oracle corpus");

    OracleData data;
    std::string line;
    std::vector<std::array<int, 3>> pairs;

    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line.front() == '#') {
            continue;
        }

        const std::vector<std::string> fields =
            splitTabs(line);
        require(!fields.empty(), "empty D26QV1 record");

        if (fields[0] == "V") {
            require(fields.size() == 3U, "invalid V record");
            const int id = parseInt(fields[1]);
            require(
                id == static_cast<int>(data.definitions.size()),
                "non-contiguous D26QV1 vector id");
            data.definitions.push_back(
                parseSymbols(fields[2]));
        } else if (fields[0] == "P") {
            require(fields.size() == 4U, "invalid P record");
            pairs.push_back({
                parseInt(fields[1]),
                parseInt(fields[2]),
                parseInt(fields[3])});
        } else if (fields[0] == "U") {
            require(fields.size() == 6U, "invalid U record");
            data.unions.push_back({
                parseInt(fields[1]),
                parseInt(fields[2]),
                parseInt(fields[3]),
                parseInt(fields[4]),
                parseInt(fields[5])});
        } else {
            fail("unknown D26QV1 oracle record");
        }
    }

    require(
        data.definitions.size() == 35U,
        "unexpected D26QV1 finite vector count");

    data.pairExpected.assign(
        data.definitions.size(),
        std::vector<int>(
            data.definitions.size(),
            99));

    for (const auto& record : pairs) {
        const int lhs = record[0];
        const int rhs = record[1];
        require(
            lhs >= 0 &&
                rhs >= 0 &&
                lhs < static_cast<int>(data.definitions.size()) &&
                rhs < static_cast<int>(data.definitions.size()),
            "D26QV1 pair id out of range");
        data.pairExpected[
            static_cast<std::size_t>(lhs)][
            static_cast<std::size_t>(rhs)] =
                record[2];
    }

    for (const auto& row : data.pairExpected) {
        require(
            std::none_of(
                row.begin(),
                row.end(),
                [](int value) { return value == 99; }),
            "D26QV1 pair matrix incomplete");
    }

    require(
        data.unions.size() > 10000U,
        "D26QV1 union oracle unexpectedly small");
    return data;
}

int asInt(q::QualityVectorOrder order) {
    return static_cast<int>(order);
}

void verifyPairOracle(
    const OracleData& oracle,
    const std::vector<q::QualityVector>& vectors) {
    std::size_t secondTailRefinements = 0U;
    std::size_t prefixDecisions = 0U;

    for (std::size_t lhs = 0; lhs < vectors.size(); ++lhs) {
        for (std::size_t rhs = 0; rhs < vectors.size(); ++rhs) {
            const int expected =
                oracle.pairExpected[lhs][rhs];
            const int actual =
                asInt(
                    q::compareQualityVectors(
                        vectors[lhs],
                        vectors[rhs]));

            require(
                actual == expected,
                "D26QV1 pairwise oracle mismatch");

            if (oracle.definitions[lhs].size() !=
                    oracle.definitions[rhs].size() &&
                expected != 0) {
                ++prefixDecisions;
            }

            if (oracle.definitions[lhs].size() >= 2U &&
                oracle.definitions[lhs].size() ==
                    oracle.definitions[rhs].size() &&
                !oracle.definitions[lhs].empty() &&
                oracle.definitions[lhs].front() ==
                    oracle.definitions[rhs].front() &&
                expected != 0) {
                ++secondTailRefinements;
            }
        }
    }

    require(
        prefixDecisions > 0U,
        "D26QV1 prefix fixtures missing");
    require(
        secondTailRefinements > 0U,
        "D26QV1 deeper-tail refinement fixtures missing");
}

void verifyUnionOracle(
    const OracleData& oracle,
    const std::vector<q::QualityVector>& vectors) {
    for (const OracleData::UnionCase& item : oracle.unions) {
        const auto oldId =
            static_cast<std::size_t>(item.oldId);
        const auto newId =
            static_cast<std::size_t>(item.newId);
        const auto commonId =
            static_cast<std::size_t>(item.commonId);

        require(
            asInt(
                q::compareQualityVectors(
                    vectors[newId],
                    vectors[oldId])) ==
                item.localExpected,
            "D26QV1 local strict oracle mismatch");

        const q::QualityVector oldUnion =
            q::mergeQualityVectors(
                vectors[oldId],
                vectors[commonId]);
        const q::QualityVector newUnion =
            q::mergeQualityVectors(
                vectors[newId],
                vectors[commonId]);

        require(
            asInt(
                q::compareQualityVectors(
                    newUnion,
                    oldUnion)) ==
                item.unionExpected,
            "D26QV1 union compatibility mismatch");
        require(
            item.unionExpected > 0,
            "strict local improvement lost globally");
    }
}

void verifyIdentityIsNotQuality() {
    const std::vector<int> oneSymbol{1};
    const q::QualityVector lhs =
        buildFromSymbols(oneSymbol, 100U);
    const q::QualityVector rhs =
        buildFromSymbols(oneSymbol, 1000U);

    require(
        q::compareQualityVectors(lhs, rhs) ==
            q::QualityVectorOrder::Equal,
        "canonical identity changed exact quality tie");

    std::vector<q::IndexedTetraCoordinates> two =
        cellsFromSymbols({1, 1}, 100U);
    std::swap(two[0], two[1]);

    const q::QualityVector sorted =
        q::buildQualityVector(two);
    require(
        sorted.entries.size() == 2U &&
            sorted.entries[0].tetra <
                sorted.entries[1].tetra,
        "canonical identity did not stabilize tie representation");
}

void verifyAcceptance(
    const OracleData& oracle) {
    int oldId = -1;
    int newId = -1;

    for (std::size_t oldIndex = 0;
         oldIndex < oracle.definitions.size() &&
         oldId < 0;
         ++oldIndex) {
        if (oracle.definitions[oldIndex].empty()) {
            continue;
        }

        for (std::size_t newIndex = 0;
             newIndex < oracle.definitions.size();
             ++newIndex) {
            if (oracle.definitions[newIndex].empty()) {
                continue;
            }

            if (oracle.pairExpected[newIndex][oldIndex] > 0) {
                oldId = static_cast<int>(oldIndex);
                newId = static_cast<int>(newIndex);
                break;
            }
        }
    }

    require(oldId >= 0 && newId >= 0, "strict acceptance fixture missing");

    const auto oldCells =
        cellsFromSymbols(
            oracle.definitions[
                static_cast<std::size_t>(oldId)],
            100U);
    const auto newCells =
        cellsFromSymbols(
            oracle.definitions[
                static_cast<std::size_t>(newId)],
            1000U);

    q::AcceptanceTelemetry telemetry;
    const q::ReplacementValidationEvidence valid{};

    const q::AcceptanceEvaluation improved =
        q::evaluateReplacement(
            oldCells,
            newCells,
            valid,
            &telemetry);

    require(
        improved.decision == q::AcceptanceDecision::Accept &&
            improved.qualityOrder == q::QualityVectorOrder::Better,
        "strict D26QV1 improvement was not accepted");

    const q::AcceptanceEvaluation equal =
        q::evaluateReplacement(
            oldCells,
            oldCells,
            valid,
            &telemetry);
    require(
        equal.decision ==
            q::AcceptanceDecision::NoImprovement &&
            equal.qualityOrder ==
                q::QualityVectorOrder::Equal,
        "exact D26QV1 tie must be NO MUTATION");

    const q::AcceptanceEvaluation worse =
        q::evaluateReplacement(
            newCells,
            oldCells,
            valid,
            &telemetry);
    require(
        worse.decision ==
            q::AcceptanceDecision::NoImprovement &&
            worse.qualityOrder ==
                q::QualityVectorOrder::Worse,
        "D26QV1 regression must be rejected");

    q::ReplacementValidationEvidence blocked = valid;
    blocked.constraintsLegal = false;
    require(
        q::evaluateReplacement(
            oldCells,
            newCells,
            blocked).decision ==
            q::AcceptanceDecision::ConstraintBlocked,
        "constraint evidence did not block mutation");

    q::ReplacementValidationEvidence cavity = valid;
    cavity.cavityBoundaryEquivalent = false;
    require(
        q::evaluateReplacement(
            oldCells,
            newCells,
            cavity).decision ==
            q::AcceptanceDecision::InvalidCandidate,
        "cavity mismatch did not reject candidate");

    q::ReplacementValidationEvidence structural = valid;
    structural.structuralAssumptionsValid = false;
    require(
        q::evaluateReplacement(
            oldCells,
            newCells,
            structural).decision ==
            q::AcceptanceDecision::InvalidCandidate,
        "structural failure did not reject candidate");

    auto inverted = newCells;
    require(!inverted.empty(), "inversion fixture missing");
    std::swap(
        inverted[0].coordinates[2],
        inverted[0].coordinates[3]);
    require(
        q::evaluateReplacement(
            oldCells,
            inverted,
            valid).decision ==
            q::AcceptanceDecision::InvalidCandidate,
        "non-positive TET4 entered D26QACC1");

    require(
        telemetry.accepted == 1U &&
            telemetry.exactQualityTie == 1U &&
            telemetry.noImprovement == 2U,
        "D26QACC1 telemetry partition mismatch");
}

// D26QMRF1 is a backend, never a semantic. Whatever the filter certifies must be
// what the exact backend alone would have said, so the same comparison is run
// twice: once as built, and once with every filter certificate stripped so the
// exact path is forced. Any disagreement means the fast path changed an answer.
q::QualityVector withoutFilter(const q::QualityVector& source) {
    q::QualityVector stripped = source;
    for (q::QualityVectorEntry& entry : stripped.entries) {
        entry.filterReady = false;
        entry.filter = {};
    }
    return stripped;
}

void verifyFilterBackendAgreement(
    const std::vector<q::QualityVector>& vectors) {
    q::QualityVectorTelemetry filtered{};
    q::QualityVectorTelemetry exactOnly{};

    for (const q::QualityVector& lhs : vectors) {
        for (const q::QualityVector& rhs : vectors) {
            const q::QualityVectorOrder viaFilter =
                q::compareQualityVectors(lhs, rhs, &filtered);
            const q::QualityVectorOrder viaExact =
                q::compareQualityVectors(
                    withoutFilter(lhs),
                    withoutFilter(rhs),
                    &exactOnly);

            require(
                viaFilter == viaExact,
                "certified filter result disagrees with the exact backend");
        }
    }

    require(
        filtered.intervalCertifiedComparisons +
                filtered.exactComparisons ==
            filtered.entryComparisons,
        "filter/exact backend split does not account for every comparison");

    require(
        exactOnly.intervalCertifiedComparisons == 0U,
        "stripped vectors must not produce interval certificates");
    require(
        exactOnly.exactComparisons == exactOnly.entryComparisons,
        "stripped vectors must route every comparison to the exact backend");
}

} // namespace

int main(int argc, char** argv) {
    try {
        require(
            argc == 2,
            "generated D26QV1 oracle path required");

        const OracleData oracle =
            readOracle(argv[1]);

        std::vector<q::QualityVector> vectors;
        vectors.reserve(oracle.definitions.size());
        for (std::size_t i = 0;
             i < oracle.definitions.size();
             ++i) {
            vectors.push_back(
                buildFromSymbols(
                    oracle.definitions[i],
                    1U +
                        static_cast<femcae::meshing::PointId>(i) *
                            100U));
        }

        verifyPairOracle(oracle, vectors);
        verifyUnionOracle(oracle, vectors);
        verifyIdentityIsNotQuality();
        verifyAcceptance(oracle);
        verifyFilterBackendAgreement(vectors);

        std::cout
            << "M6 I3 D26QV1/D26QACC1 PASS"
            << " vectors=" << oracle.definitions.size()
            << " unions=" << oracle.unions.size()
            << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr
            << "M6 I3 D26QV1/D26QACC1 FAIL: "
            << error.what()
            << '\n';
        return 1;
    }
}
