#include "meshing/m2/DelaunayConstructor.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace femcae::meshing::m2;

std::uint64_t checks = 0U;

void require(bool value, const std::string& message) {
    ++checks;
    if (!value) {
        throw std::runtime_error(message);
    }
}

using Manifest = std::map<std::string, std::string>;

Manifest readManifest(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error(
            "missing P1F qualification manifest: " +
            path.string());
    }

    Manifest result;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const std::size_t equals = line.find('=');
        if (equals == std::string::npos ||
            equals == 0U) {
            throw std::runtime_error(
                "invalid P1F manifest line in " +
                path.string());
        }
        const std::string key =
            line.substr(0U, equals);
        const std::string value =
            line.substr(equals + 1U);
        if (!result.emplace(key, value).second) {
            throw std::runtime_error(
                "duplicate P1F manifest key '" +
                key + "' in " + path.string());
        }
    }
    return result;
}

const std::string& field(
    const Manifest& manifest,
    const std::string& key) {
    const auto iterator = manifest.find(key);
    if (iterator == manifest.end()) {
        throw std::runtime_error(
            "P1F manifest is missing key '" +
            key + "'");
    }
    return iterator->second;
}

std::uint64_t parseU64(
    const Manifest& manifest,
    const std::string& key) {
    const std::string& text = field(manifest, key);
    std::size_t consumed = 0U;
    const unsigned long long value =
        std::stoull(text, &consumed, 10);
    if (consumed != text.size()) {
        throw std::runtime_error(
            "P1F manifest numeric key '" +
            key + "' is invalid");
    }
    return static_cast<std::uint64_t>(value);
}

void checkedAdd(
    std::uint64_t& value,
    std::uint64_t add,
    const char* context) {
    if (add >
        std::numeric_limits<std::uint64_t>::max() -
            value) {
        throw std::overflow_error(context);
    }
    value += add;
}

void validateAuthority(
    const Manifest& manifest,
    const std::string& kind,
    const std::string& buildConfig) {
    require(
        field(manifest, "schema") ==
            "D26M21FQ1",
        "P1F manifest schema mismatch");
    require(
        field(manifest, "completion") == "1",
        "P1F manifest was not atomically published as complete");
    require(
        field(manifest, "kind") == kind,
        "P1F manifest kind mismatch");
    require(
        field(manifest, "build_config") ==
            buildConfig,
        "P1F manifest build configuration mismatch");
    require(
        field(manifest, "site_policy") ==
            D26SitePolicyId,
        "P1F manifest site policy mismatch");
    require(
        field(manifest, "symbolic_policy") ==
            D26SymbolicPolicyId,
        "P1F manifest symbolic policy mismatch");
    require(
        field(manifest, "fingerprint_schema") ==
            D26FingerprintSchemaId,
        "P1F manifest fingerprint schema mismatch");
    require(
        field(manifest, "replay_schema") ==
            D26ReplaySchemaId,
        "P1F manifest replay schema mismatch");
}

void requireZero(
    const Manifest& manifest,
    const std::string& key) {
    require(
        parseU64(manifest, key) == 0U,
        "P1F mandatory failure counter is non-zero: " +
        key);
}

void atomicWriteText(
    const std::filesystem::path& path,
    const std::string& content) {
    if (path.has_parent_path()) {
        std::filesystem::create_directories(
            path.parent_path());
    }
    const std::filesystem::path temporary =
        path.string() + ".tmp";
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);
    {
        std::ofstream output(
            temporary,
            std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error(
                "could not create P1F aggregate manifest temporary");
        }
        output.write(
            content.data(),
            static_cast<std::streamsize>(
                content.size()));
        if (!output) {
            throw std::runtime_error(
                "could not write P1F aggregate manifest temporary");
        }
    }
    std::filesystem::remove(path, ignored);
    std::filesystem::rename(temporary, path);
}

std::string shardName(std::uint64_t index) {
    return "cube-shard-" +
           std::to_string(index) +
           ".manifest";
}

std::uint64_t parseArgument(
    const std::string& text,
    const char* name) {
    std::size_t consumed = 0U;
    const unsigned long long value =
        std::stoull(text, &consumed, 10);
    if (consumed != text.size()) {
        throw std::runtime_error(
            std::string("invalid ") + name);
    }
    return static_cast<std::uint64_t>(value);
}

} // namespace

int main(int argc, char** argv) {
    try {
        std::optional<std::filesystem::path> manifestDir;
        std::optional<std::filesystem::path> aggregatePath;
        std::optional<std::uint64_t> shardCount;
        std::string buildConfig{"unspecified"};

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--manifest-dir" &&
                i + 1 < argc) {
                manifestDir =
                    std::filesystem::path(argv[++i]);
            } else if (arg == "--aggregate-manifest" &&
                       i + 1 < argc) {
                aggregatePath =
                    std::filesystem::path(argv[++i]);
            } else if (arg == "--shard-count" &&
                       i + 1 < argc) {
                shardCount =
                    parseArgument(
                        argv[++i], "shard count");
            } else if (arg == "--build-config" &&
                       i + 1 < argc) {
                buildConfig = argv[++i];
            } else {
                throw std::runtime_error(
                    "usage: unit_m21f_delaunay_aggregate "
                    "--manifest-dir <dir> --shard-count <n> "
                    "--build-config <name> "
                    "[--aggregate-manifest <path>]");
            }
        }

        require(
            manifestDir.has_value() &&
            shardCount.has_value() &&
            *shardCount > 0U,
            "P1F aggregate requires manifest directory and non-zero shard count");

        const Manifest core =
            readManifest(
                *manifestDir / "core.manifest");
        validateAuthority(
            core, "core", buildConfig);

        require(
            parseU64(core, "five_site_permutations") ==
                120U,
            "P1F core did not execute all 120 five-site permutations");
        requireZero(core, "five_site_mismatches");
        requireZero(core, "global_oracle_failures");
        requireZero(core, "local_legality_failures");
        requireZero(core, "symbolic_tie_failures");
        requireZero(core, "S3_failures");
        requireZero(core, "hull_support_violations");
        requireZero(core, "hull_symbolic_failures");
        requireZero(core, "metamorphic_failures");
        requireZero(core, "fingerprint_mismatches");

        require(
            parseU64(core, "global_oracle_sets") > 0U &&
            parseU64(core, "global_tet_site_tests") > 0U &&
            parseU64(core, "local_facets_checked") > 0U &&
            parseU64(core, "symbolic_ties_checked") > 0U &&
            parseU64(core, "S3_states_checked") > 0U &&
            parseU64(core, "hull_faces_checked") > 0U &&
            parseU64(core, "coplanar_hull_edges") > 0U &&
            parseU64(core, "hull_symbolic_ties") > 0U &&
            parseU64(core, "resource_cases") > 0U &&
            parseU64(core, "metamorphic_cases") > 0U,
            "P1F core evidence counters are unexpectedly empty");

        const std::string cubeRecord =
            field(core, "cube_record_hex");
        const std::string cubeDigest =
            field(core, "cube_digest");
        require(
            !cubeRecord.empty() &&
            !cubeDigest.empty(),
            "P1F core cube D26DT1 authority is empty");

        std::uint64_t expectedBegin = 0U;
        std::uint64_t cubePermutations = 0U;
        std::uint64_t cubeMismatches = 0U;
        std::uint64_t constructorFailures = 0U;
        std::uint64_t shardFingerprintMismatches = 0U;
        std::uint64_t shardValidatorStates = 0U;
        std::uint64_t shardRuntimeSumMs = 0U;
        std::uint64_t shardRuntimeMaxMs = 0U;

        for (std::uint64_t index = 0U;
             index < *shardCount;
             ++index) {
            const Manifest shard =
                readManifest(
                    *manifestDir / shardName(index));
            validateAuthority(
                shard, "cube_shard", buildConfig);

            require(
                parseU64(shard, "shard_index") ==
                    index,
                "P1F cube shard index mismatch");
            require(
                parseU64(shard, "shard_count") ==
                    *shardCount,
                "P1F cube shard count mismatch");

            const std::uint64_t rankBegin =
                parseU64(shard, "rank_begin");
            const std::uint64_t rankEnd =
                parseU64(shard, "rank_end");
            const std::uint64_t executed =
                parseU64(
                    shard,
                    "executed_permutations");

            require(
                rankBegin == expectedBegin,
                "P1F cube shard ranges have gap or overlap");
            require(
                rankEnd > rankBegin &&
                executed == rankEnd - rankBegin,
                "P1F cube shard did not execute its full rank interval");
            expectedBegin = rankEnd;

            require(
                field(shard, "cube_record_hex") ==
                    cubeRecord,
                "P1F cube shard canonical D26DT1 record differs from core authority");
            require(
                field(shard, "cube_digest") ==
                    cubeDigest,
                "P1F cube shard digest differs after canonical-record equality");

            const std::uint64_t shardConstructorFailures =
                parseU64(
                    shard,
                    "constructor_failures");
            const std::uint64_t shardMismatches =
                parseU64(
                    shard,
                    "cube_mismatches");
            const std::uint64_t shardFingerprint =
                parseU64(
                    shard,
                    "fingerprint_mismatches");
            require(
                shardConstructorFailures == 0U &&
                shardMismatches == 0U &&
                shardFingerprint == 0U,
                "P1F cube shard mandatory failure counter is non-zero");

            checkedAdd(
                cubePermutations,
                executed,
                "P1F cube permutation aggregate overflow");
            checkedAdd(
                constructorFailures,
                shardConstructorFailures,
                "P1F constructor-failure aggregate overflow");
            checkedAdd(
                cubeMismatches,
                shardMismatches,
                "P1F cube-mismatch aggregate overflow");
            checkedAdd(
                shardFingerprintMismatches,
                shardFingerprint,
                "P1F fingerprint-mismatch aggregate overflow");
            checkedAdd(
                shardValidatorStates,
                parseU64(shard, "validator_states"),
                "P1F validator-state aggregate overflow");

            const std::uint64_t runtimeMs =
                parseU64(shard, "runtime_ms");
            checkedAdd(
                shardRuntimeSumMs,
                runtimeMs,
                "P1F shard-runtime aggregate overflow");
            shardRuntimeMaxMs =
                std::max(
                    shardRuntimeMaxMs,
                    runtimeMs);
        }

        require(
            expectedBegin == 40320U,
            "P1F cube shard ranges do not end at permutation rank 40320");
        require(
            cubePermutations == 40320U,
            "P1F cube shard aggregate did not execute all 40320 permutations");
        require(
            constructorFailures == 0U &&
            cubeMismatches == 0U &&
            shardFingerprintMismatches == 0U,
            "P1F cube aggregate has non-zero failure counters");

        std::uint64_t constructorFixtures =
            parseU64(core, "constructor_fixtures");
        checkedAdd(
            constructorFixtures,
            cubePermutations,
            "P1F constructor-fixture aggregate overflow");

        std::uint64_t successfulBuilds =
            parseU64(core, "successful_builds");
        checkedAdd(
            successfulBuilds,
            cubePermutations,
            "P1F successful-build aggregate overflow");

        std::uint64_t validatorStates =
            parseU64(core, "validator_states");
        checkedAdd(
            validatorStates,
            shardValidatorStates,
            "P1F validator-state total overflow");

        std::uint64_t fingerprintMismatches =
            parseU64(core, "fingerprint_mismatches");
        checkedAdd(
            fingerprintMismatches,
            shardFingerprintMismatches,
            "P1F fingerprint-mismatch total overflow");

        std::ostringstream aggregate;
        aggregate
            << "schema=D26M21FQ1\n"
            << "completion=1\n"
            << "kind=aggregate\n"
            << "build_config=" << buildConfig << "\n"
            << "site_policy=" << D26SitePolicyId << "\n"
            << "symbolic_policy=" << D26SymbolicPolicyId << "\n"
            << "fingerprint_schema=" << D26FingerprintSchemaId << "\n"
            << "replay_schema=" << D26ReplaySchemaId << "\n"
            << "shard_count=" << *shardCount << "\n"
            << "constructor_fixtures=" << constructorFixtures << "\n"
            << "successful_builds=" << successfulBuilds << "\n"
            << "failed_expected_builds="
            << parseU64(core, "failed_expected_builds") << "\n"
            << "five_site_permutations=120\n"
            << "five_site_mismatches=0\n"
            << "cube_permutations=" << cubePermutations << "\n"
            << "cube_mismatches=" << cubeMismatches << "\n"
            << "constructor_failures=" << constructorFailures << "\n"
            << "global_oracle_sets="
            << parseU64(core, "global_oracle_sets") << "\n"
            << "global_tet_site_tests="
            << parseU64(core, "global_tet_site_tests") << "\n"
            << "global_oracle_failures=0\n"
            << "local_facets_checked="
            << parseU64(core, "local_facets_checked") << "\n"
            << "local_legality_failures=0\n"
            << "symbolic_ties_checked="
            << parseU64(core, "symbolic_ties_checked") << "\n"
            << "symbolic_tie_failures=0\n"
            << "S3_states_checked="
            << parseU64(core, "S3_states_checked") << "\n"
            << "S3_failures=0\n"
            << "hull_faces_checked="
            << parseU64(core, "hull_faces_checked") << "\n"
            << "hull_support_violations=0\n"
            << "coplanar_hull_edges="
            << parseU64(core, "coplanar_hull_edges") << "\n"
            << "hull_symbolic_ties="
            << parseU64(core, "hull_symbolic_ties") << "\n"
            << "hull_symbolic_failures=0\n"
            << "validator_states=" << validatorStates << "\n"
            << "resource_cases="
            << parseU64(core, "resource_cases") << "\n"
            << "metamorphic_cases="
            << parseU64(core, "metamorphic_cases") << "\n"
            << "metamorphic_failures=0\n"
            << "fingerprint_mismatches="
            << fingerprintMismatches << "\n"
            << "five_digest=" << field(core, "five_digest") << "\n"
            << "cube_digest=" << cubeDigest << "\n"
            << "interior_digest=" << field(core, "interior_digest") << "\n"
            << "five_record_hex=" << field(core, "five_record_hex") << "\n"
            << "cube_record_hex=" << cubeRecord << "\n"
            << "interior_record_hex=" << field(core, "interior_record_hex") << "\n"
            << "core_runtime_ms=" << parseU64(core, "runtime_ms") << "\n"
            << "cube_shard_runtime_sum_ms=" << shardRuntimeSumMs << "\n"
            << "cube_shard_runtime_max_ms=" << shardRuntimeMaxMs << "\n";

        if (aggregatePath.has_value()) {
            atomicWriteText(
                *aggregatePath,
                aggregate.str());
        }

        std::cout
            << "M2.1-F serial constructor qualification PASS"
            << " checks=" << checks
            << " constructor_fixtures=" << constructorFixtures
            << " successful_builds=" << successfulBuilds
            << " failed_expected_builds="
            << parseU64(core, "failed_expected_builds")
            << " five_site_permutations=120"
            << " five_site_mismatches=0"
            << " cube_permutations=" << cubePermutations
            << " cube_mismatches=" << cubeMismatches
            << " constructor_failures=" << constructorFailures
            << " global_oracle_sets="
            << parseU64(core, "global_oracle_sets")
            << " global_tet_site_tests="
            << parseU64(core, "global_tet_site_tests")
            << " global_oracle_failures=0"
            << " local_facets_checked="
            << parseU64(core, "local_facets_checked")
            << " local_legality_failures=0"
            << " symbolic_ties_checked="
            << parseU64(core, "symbolic_ties_checked")
            << " symbolic_tie_failures=0"
            << " S3_states_checked="
            << parseU64(core, "S3_states_checked")
            << " S3_failures=0"
            << " hull_faces_checked="
            << parseU64(core, "hull_faces_checked")
            << " hull_support_violations=0"
            << " coplanar_hull_edges="
            << parseU64(core, "coplanar_hull_edges")
            << " hull_symbolic_ties="
            << parseU64(core, "hull_symbolic_ties")
            << " hull_symbolic_failures=0"
            << " validator_states=" << validatorStates
            << " resource_cases="
            << parseU64(core, "resource_cases")
            << " metamorphic_cases="
            << parseU64(core, "metamorphic_cases")
            << " metamorphic_failures=0"
            << " fingerprint_mismatches="
            << fingerprintMismatches
            << '\n';

        std::cout
            << "determinism_manifest"
            << " site_policy=" << D26SitePolicyId
            << " symbolic_policy=" << D26SymbolicPolicyId
            << " fingerprint_schema=" << D26FingerprintSchemaId
            << " replay_schema=" << D26ReplaySchemaId
            << " five_digest=" << field(core, "five_digest")
            << " cube_digest=" << cubeDigest
            << " interior_digest=" << field(core, "interior_digest")
            << " canonical_records_equal_to_core=1"
            << '\n';

        std::cout
            << "qualification_runtime"
            << " core_ms=" << parseU64(core, "runtime_ms")
            << " cube_shard_sum_ms=" << shardRuntimeSumMs
            << " cube_shard_max_ms=" << shardRuntimeMaxMs
            << " shard_count=" << *shardCount
            << '\n';

        return 0;
    } catch (const std::exception& error) {
        std::cerr
            << "M2.1-F qualification aggregate FAIL: "
            << error.what() << '\n';
        return 1;
    }
}
