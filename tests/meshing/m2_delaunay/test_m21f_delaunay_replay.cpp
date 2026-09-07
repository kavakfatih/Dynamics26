#include "meshing/m2/DelaunayConstructor.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

using namespace femcae::meshing::m2;

std::uint64_t checks = 0U;

void require(bool value, const std::string& message) {
    ++checks;
    if (!value) {
        throw std::runtime_error(message);
    }
}

std::string readAll(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error(
            "could not open serialized P1F replay fixture");
    }
    std::ostringstream out;
    out << input.rdbuf();
    if (!input.good() && !input.eof()) {
        throw std::runtime_error(
            "could not read serialized P1F replay fixture");
    }
    return out.str();
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            throw std::runtime_error(
                "usage: unit_m21f_delaunay_replay <serialized-replay>");
        }

        const std::string serialized = readAll(argv[1]);
        const DelaunayReplayResult replay =
            replaySerializedDelaunay(serialized);

        require(
            replay.ok() &&
                replay.constructor.has_value(),
            "standalone replay did not reproduce the serialized constructor execution");
        require(
            replay.constructor->status ==
                DelaunayConstructorStatus::ResourceLimit,
            "standalone replay did not reproduce typed ResourceLimit");
        require(
            replay.constructor->failurePointId.has_value() &&
                replay.constructor->failureInsertionIndex.has_value() &&
                replay.constructor->partialFingerprint.has_value() &&
                !replay.constructor->finalFingerprint.has_value(),
            "standalone replay lost failure identity/partial-state semantics");

        std::string tampered = serialized;
        const std::string authority =
            "site_policy=D26SITE1";
        const std::size_t position =
            tampered.find(authority);
        require(
            position != std::string::npos,
            "serialized replay did not contain D26SITE1 authority");
        tampered.replace(
            position,
            authority.size(),
            "site_policy=D26SITE999");

        const DelaunayReplayResult policyFailure =
            replaySerializedDelaunay(tampered);
        require(
            policyFailure.status ==
                DelaunayReplayStatus::PolicyMismatch,
            "tampered replay policy was silently accepted");

        std::cout
            << "M2.1-F standalone replay PASS"
            << " checks=" << checks
            << " replays=1"
            << " replay_mismatches=0"
            << " policy_mismatch_rejections=1"
            << " reproduced_status="
            << delaunayConstructorStatusName(
                   replay.constructor->status)
            << " failure_point="
            << *replay.constructor->failurePointId
            << " failure_index="
            << *replay.constructor->failureInsertionIndex
            << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr
            << "M2.1-F standalone replay FAIL: "
            << error.what() << '\n';
        return 1;
    }
}
