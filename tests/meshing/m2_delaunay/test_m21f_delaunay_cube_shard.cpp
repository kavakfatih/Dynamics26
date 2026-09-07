#include "meshing/m2/DelaunayConstructor.h"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace femcae::meshing;
using namespace femcae::meshing::m2;

std::uint64_t checks = 0U;

void require(bool value, const std::string& message) {
    ++checks;
    if (!value) {
        throw std::runtime_error(message);
    }
}

InputSite input(
    double x,
    double y,
    double z,
    std::uint64_t sourceId) {
    return {{x, y, z}, sourceId};
}

std::vector<InputSite> cubeFixture() {
    return {
        input(0.0, 0.0, 0.0, 201U),
        input(0.0, 0.0, 1.0, 202U),
        input(0.0, 1.0, 0.0, 203U),
        input(0.0, 1.0, 1.0, 204U),
        input(1.0, 0.0, 0.0, 205U),
        input(1.0, 0.0, 1.0, 206U),
        input(1.0, 1.0, 0.0, 207U),
        input(1.0, 1.0, 1.0, 208U)};
}

const std::vector<std::array<PointId, 4>> CubeFinite{{
    {1U, 2U, 3U, 5U},
    {2U, 3U, 4U, 5U},
    {2U, 4U, 5U, 6U},
    {3U, 4U, 5U, 7U},
    {4U, 5U, 6U, 7U},
    {4U, 6U, 7U, 8U},
}};

const std::vector<std::array<PointId, 3>> CubeHull{{
    {1U, 2U, 3U},
    {1U, 2U, 5U},
    {1U, 3U, 5U},
    {2U, 3U, 4U},
    {2U, 4U, 6U},
    {2U, 5U, 6U},
    {3U, 4U, 7U},
    {3U, 5U, 7U},
    {4U, 6U, 8U},
    {4U, 7U, 8U},
    {5U, 6U, 7U},
    {6U, 7U, 8U},
}};

std::uint64_t canonicalBits(double value) {
    std::uint64_t bits =
        std::bit_cast<std::uint64_t>(value);
    if ((bits & 0x7FFFFFFFFFFFFFFFULL) == 0ULL) {
        bits = 0ULL;
    }
    return bits;
}

std::string hex64(std::uint64_t value) {
    std::ostringstream out;
    out << std::hex << std::setfill('0')
        << std::setw(16) << value;
    return out.str();
}

std::string expectedCubeRecord() {
    const auto canonical =
        canonicalizeSites(cubeFixture());
    std::ostringstream out;
    out << "D26DT1\n"
        << "site_policy=D26SITE1\n"
        << "symbolic_policy=D26LIFT1\n"
        << "fingerprint_schema=D26DT1\n"
        << "canonical_sites=" << canonical.size() << "\n"
        << "finite_tets=" << CubeFinite.size() << "\n"
        << "hull_facets=" << CubeHull.size() << "\n";
    for (const auto& site : canonical) {
        out << "site " << site.id << ' '
            << hex64(canonicalBits(site.point.x)) << ' '
            << hex64(canonicalBits(site.point.y)) << ' '
            << hex64(canonicalBits(site.point.z)) << "\n";
    }
    for (const auto& tet : CubeFinite) {
        out << "tet " << tet[0] << ' ' << tet[1]
            << ' ' << tet[2] << ' ' << tet[3] << "\n";
    }
    for (const auto& face : CubeHull) {
        out << "hull " << face[0] << ' '
            << face[1] << ' ' << face[2] << "\n";
    }
    return out.str();
}

std::string bytesToHex(std::string_view bytes) {
    static constexpr char digits[] = "0123456789abcdef";
    if (bytes.size() >
        std::numeric_limits<std::size_t>::max() / 2U) {
        throw std::length_error(
            "P1F cube shard record hex length overflow");
    }
    std::string result;
    result.reserve(bytes.size() * 2U);
    for (unsigned char byte : bytes) {
        result.push_back(digits[(byte >> 4U) & 0xFU]);
        result.push_back(digits[byte & 0xFU]);
    }
    return result;
}

void atomicWriteText(
    const std::string& path,
    const std::string& content) {
    const std::filesystem::path destination(path);
    if (destination.has_parent_path()) {
        std::filesystem::create_directories(
            destination.parent_path());
    }
    const std::filesystem::path temporary =
        destination.string() + ".tmp";
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);
    {
        std::ofstream output(
            temporary,
            std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error(
                "could not create P1F cube shard manifest temporary");
        }
        output.write(
            content.data(),
            static_cast<std::streamsize>(content.size()));
        if (!output) {
            throw std::runtime_error(
                "could not write P1F cube shard manifest temporary");
        }
    }
    std::filesystem::remove(destination, ignored);
    std::filesystem::rename(temporary, destination);
}

std::uint64_t factorial(std::uint64_t n) {
    std::uint64_t value = 1U;
    for (std::uint64_t i = 2U; i <= n; ++i) {
        if (value >
            std::numeric_limits<std::uint64_t>::max() / i) {
            throw std::overflow_error(
                "P1F cube shard factorial overflow");
        }
        value *= i;
    }
    return value;
}

std::vector<PointId> unrankPermutation(
    std::uint64_t rank,
    std::size_t count) {
    std::vector<PointId> available;
    available.reserve(count);
    for (std::size_t i = 0U; i < count; ++i) {
        available.push_back(
            static_cast<PointId>(i + 1U));
    }

    std::vector<PointId> result;
    result.reserve(count);
    for (std::size_t position = 0U;
         position < count;
         ++position) {
        const std::size_t remaining =
            count - position - 1U;
        const std::uint64_t block =
            factorial(
                static_cast<std::uint64_t>(remaining));
        const std::uint64_t index =
            block == 0U ? 0U : rank / block;
        rank =
            block == 0U ? 0U : rank % block;
        if (index >= available.size()) {
            throw std::out_of_range(
                "P1F cube permutation rank is out of range");
        }
        result.push_back(
            available[static_cast<std::size_t>(index)]);
        available.erase(
            available.begin() +
            static_cast<std::ptrdiff_t>(index));
    }
    return result;
}

std::string joinIds(const std::vector<PointId>& ids) {
    std::ostringstream out;
    for (std::size_t i = 0U; i < ids.size(); ++i) {
        if (i != 0U) {
            out << ',';
        }
        out << ids[i];
    }
    return out.str();
}

std::uint64_t parseU64(const std::string& text) {
    std::size_t consumed = 0U;
    const unsigned long long value =
        std::stoull(text, &consumed, 10);
    if (consumed != text.size()) {
        throw std::invalid_argument(
            "P1F cube shard numeric argument is invalid");
    }
    return static_cast<std::uint64_t>(value);
}

void writeFailureReplay(
    const std::string& path,
    const std::vector<InputSite>& raw,
    const std::vector<PointId>& order) {
    if (path.empty()) {
        return;
    }
    DelaunayConstructorOptions options;
    options.fullInsertionOrder = order;
    options.captureReplayDecisions = true;
    options.telemetryEnabled = true;
    options.compareBruteForceLocation = true;
    const auto rerun =
        constructDelaunayReference(raw, options);
    if (!rerun.replayRecord.has_value()) {
        return;
    }
    atomicWriteText(
        path,
        serializeDelaunayReplay(
            *rerun.replayRecord));
}

} // namespace

int main(int argc, char** argv) {
    try {
        std::optional<std::uint64_t> shardIndex;
        std::optional<std::uint64_t> shardCount;
        std::optional<std::string> manifestPath;
        std::string failureReplayPath;
        std::string buildConfig{"unspecified"};

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--shard-index" && i + 1 < argc) {
                shardIndex = parseU64(argv[++i]);
            } else if (arg == "--shard-count" && i + 1 < argc) {
                shardCount = parseU64(argv[++i]);
            } else if (arg == "--manifest" && i + 1 < argc) {
                manifestPath = argv[++i];
            } else if (arg == "--failure-replay" && i + 1 < argc) {
                failureReplayPath = argv[++i];
            } else if (arg == "--build-config" && i + 1 < argc) {
                buildConfig = argv[++i];
            } else {
                throw std::runtime_error(
                    "usage: unit_m21f_delaunay_cube_shard "
                    "--shard-index <i> --shard-count <n> "
                    "--manifest <path> [--failure-replay <path>] "
                    "[--build-config <name>]");
            }
        }

        require(
            shardIndex.has_value() &&
            shardCount.has_value() &&
            manifestPath.has_value(),
            "P1F cube shard requires index/count/manifest");
        require(
            *shardCount > 0U &&
            *shardIndex < *shardCount,
            "P1F cube shard index/count is invalid");

        const std::uint64_t total = factorial(8U);
        require(total == 40320U,
                "P1F cube factorial authority is not 40320");
        require(
            *shardCount <= total,
            "P1F cube shard count exceeds permutation count");

        const std::uint64_t rankBegin =
            (total * *shardIndex) / *shardCount;
        const std::uint64_t rankEnd =
            (total * (*shardIndex + 1U)) / *shardCount;
        require(
            rankBegin < rankEnd &&
            rankEnd <= total,
            "P1F cube shard rank range is empty/invalid");

        const auto raw = cubeFixture();
        const std::string expectedRecord =
            expectedCubeRecord();
        std::string verifiedDigest;

        std::uint64_t executed = 0U;
        std::uint64_t constructorFailures = 0U;
        std::uint64_t fingerprintMismatches = 0U;
        std::uint64_t cubeMismatches = 0U;
        std::uint64_t validatorStates = 0U;

        const auto begin =
            std::chrono::steady_clock::now();

        for (std::uint64_t rank = rankBegin;
             rank < rankEnd;
             ++rank) {
            const std::vector<PointId> order =
                unrankPermutation(rank, 8U);

            DelaunayConstructorOptions options;
            options.fullInsertionOrder = order;
            options.telemetryEnabled = false;
            options.captureReplayDecisions = false;
            options.compareBruteForceLocation = true;
            const auto result =
                constructDelaunayReference(raw, options);
            ++executed;

            if (!result.ok()) {
                ++constructorFailures;
                writeFailureReplay(
                    failureReplayPath, raw, order);
                std::ostringstream error;
                error
                    << "cube shard constructor failure"
                    << " rank=" << rank
                    << " full_order=" << joinIds(order)
                    << " effective_order="
                    << joinIds(result.effectiveInsertionOrder)
                    << " status="
                    << delaunayConstructorStatusName(
                           result.status)
                    << " failure_point="
                    << (result.failurePointId.has_value()
                            ? std::to_string(
                                  *result.failurePointId)
                            : std::string("none"))
                    << " failure_index="
                    << (result.failureInsertionIndex.has_value()
                            ? std::to_string(
                                  *result.failureInsertionIndex)
                            : std::string("none"))
                    << " detail=" << result.detail;
                throw std::runtime_error(error.str());
            }

            if (result.validatedStates.size() !=
                    result.completedInsertions + 1U) {
                throw std::runtime_error(
                    "cube shard constructor did not validate bootstrap and every committed insertion state");
            }
            if (result.validatedStates.size() >
                std::numeric_limits<std::uint64_t>::max() -
                    validatorStates) {
                throw std::overflow_error(
                    "cube shard validator-state counter overflow");
            }
            validatorStates +=
                static_cast<std::uint64_t>(
                    result.validatedStates.size());

            if (result.finalFingerprint->canonicalRecord !=
                expectedRecord) {
                ++fingerprintMismatches;
                ++cubeMismatches;
                writeFailureReplay(
                    failureReplayPath, raw, order);
                std::ostringstream error;
                error
                    << "cube shard D26DT1 mismatch"
                    << " rank=" << rank
                    << " full_order=" << joinIds(order)
                    << " effective_order="
                    << joinIds(result.effectiveInsertionOrder)
                    << " digest="
                    << result.finalFingerprint->digestHex;
                throw std::runtime_error(error.str());
            }

            if (result.finalFingerprint->finiteTets !=
                    CubeFinite ||
                result.finalFingerprint->hullFacets !=
                    CubeHull) {
                ++cubeMismatches;
                writeFailureReplay(
                    failureReplayPath, raw, order);
                throw std::runtime_error(
                    "cube shard frozen finite/hull topology mismatch at rank=" +
                    std::to_string(rank));
            }

            const auto complex =
                computeDelaunayComplexStats(
                    result.arena.slots());
            if (complex.vertices != 9U ||
                complex.edges != 27U ||
                complex.faces != 36U ||
                complex.cells != 18U ||
                complex.finiteCells != 6U ||
                complex.ghostCells != 12U ||
                complex.eulerCharacteristic != 0) {
                ++cubeMismatches;
                writeFailureReplay(
                    failureReplayPath, raw, order);
                throw std::runtime_error(
                    "cube shard frozen unified counts mismatch at rank=" +
                    std::to_string(rank));
            }

            if (verifiedDigest.empty()) {
                verifiedDigest =
                    result.finalFingerprint->digestHex;
            } else if (verifiedDigest !=
                       result.finalFingerprint->digestHex) {
                ++fingerprintMismatches;
                throw std::runtime_error(
                    "cube shard digest changed after canonical-record equality");
            }
        }

        const auto end =
            std::chrono::steady_clock::now();
        const auto duration =
            std::chrono::duration_cast<
                std::chrono::milliseconds>(
                    end - begin).count();
        require(duration >= 0,
                "P1F cube shard steady-clock duration was negative");

        require(
            executed == rankEnd - rankBegin &&
            constructorFailures == 0U &&
            fingerprintMismatches == 0U &&
            cubeMismatches == 0U,
            "P1F cube shard did not execute its exact assigned interval");

        std::ostringstream manifest;
        manifest
            << "schema=D26M21FQ1\n"
            << "completion=1\n"
            << "kind=cube_shard\n"
            << "build_config=" << buildConfig << "\n"
            << "site_policy=" << D26SitePolicyId << "\n"
            << "symbolic_policy=" << D26SymbolicPolicyId << "\n"
            << "fingerprint_schema=" << D26FingerprintSchemaId << "\n"
            << "replay_schema=" << D26ReplaySchemaId << "\n"
            << "shard_index=" << *shardIndex << "\n"
            << "shard_count=" << *shardCount << "\n"
            << "rank_begin=" << rankBegin << "\n"
            << "rank_end=" << rankEnd << "\n"
            << "executed_permutations=" << executed << "\n"
            << "constructor_failures=" << constructorFailures << "\n"
            << "fingerprint_mismatches=" << fingerprintMismatches << "\n"
            << "cube_mismatches=" << cubeMismatches << "\n"
            << "validator_states=" << validatorStates << "\n"
            << "cube_digest=" << verifiedDigest << "\n"
            << "cube_record_hex=" << bytesToHex(expectedRecord) << "\n"
            << "runtime_ms="
            << static_cast<std::uint64_t>(duration) << "\n";
        atomicWriteText(*manifestPath, manifest.str());

        std::cout
            << "M2.1-F cube shard PASS"
            << " shard_index=" << *shardIndex
            << " shard_count=" << *shardCount
            << " rank_begin=" << rankBegin
            << " rank_end=" << rankEnd
            << " executed_permutations=" << executed
            << " constructor_failures=" << constructorFailures
            << " fingerprint_mismatches=" << fingerprintMismatches
            << " cube_mismatches=" << cubeMismatches
            << " validator_states=" << validatorStates
            << " cube_digest=" << verifiedDigest
            << " runtime_ms="
            << static_cast<std::uint64_t>(duration)
            << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr
            << "M2.1-F cube shard FAIL: "
            << error.what() << '\n';
        return 1;
    }
}
