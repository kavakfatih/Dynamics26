#include "femcae/meshing/TetraTopology.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using femcae::meshing::CanonicalFaceKey;
using femcae::meshing::TetHandle;
using femcae::meshing::TetRecord;
using femcae::meshing::TetSlot;
using femcae::meshing::TopologyIssueCode;

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        fail(message);
    }
}

TetSlot liveSlot(
    std::uint32_t generation,
    std::array<femcae::meshing::PointId, 4> vertices) {
    TetSlot slot;
    slot.generation = generation;
    slot.live = true;
    slot.record.vertices = vertices;
    return slot;
}

bool hasIssue(
    const femcae::meshing::TopologyValidationReport& report,
    TopologyIssueCode code) {
    return std::any_of(
        report.issues.begin(),
        report.issues.end(),
        [code](const auto& issue) { return issue.code == code; });
}

std::vector<TetSlot> validPair() {
    std::vector<TetSlot> slots;
    slots.push_back(liveSlot(1, {1, 2, 3, 4}));
    slots.push_back(liveSlot(7, {1, 3, 2, 5}));

    // Opposite vertex[3] is the shared canonical face {1,2,3}.
    slots[0].record.neighbors[3] = TetHandle{1, 7};
    slots[1].record.neighbors[3] = TetHandle{0, 1};
    return slots;
}

void verifyFaceConvention() {
    const TetRecord tet{{10, 20, 30, 40}};
    const CanonicalFaceKey f0 = femcae::meshing::canonicalFaceKey(tet, 0);
    const CanonicalFaceKey f1 = femcae::meshing::canonicalFaceKey(tet, 1);
    const CanonicalFaceKey f2 = femcae::meshing::canonicalFaceKey(tet, 2);
    const CanonicalFaceKey f3 = femcae::meshing::canonicalFaceKey(tet, 3);

    require(f0.vertices == std::array<femcae::meshing::PointId, 3>{20, 30, 40},
            "face[0] is not opposite vertex[0]");
    require(f1.vertices == std::array<femcae::meshing::PointId, 3>{10, 30, 40},
            "face[1] is not opposite vertex[1]");
    require(f2.vertices == std::array<femcae::meshing::PointId, 3>{10, 20, 40},
            "face[2] is not opposite vertex[2]");
    require(f3.vertices == std::array<femcae::meshing::PointId, 3>{10, 20, 30},
            "face[3] is not opposite vertex[3]");
}

void verifyValidPair() {
    const auto slots = validPair();
    const auto report = femcae::meshing::validateTetTopology(slots);
    require(report.ok(), "valid reciprocal tetra pair was rejected");
}

void verifyCorruptionCorpus() {
    {
        auto slots = validPair();
        slots[0].record.vertices[1] = slots[0].record.vertices[0];
        const auto report = femcae::meshing::validateTetTopology(slots);
        require(hasIssue(report, TopologyIssueCode::DuplicateVertex),
                "duplicate vertex corruption was not detected");
    }
    {
        auto slots = validPair();
        slots[0].record.neighbors[3].generation = 99;
        const auto report = femcae::meshing::validateTetTopology(slots);
        require(hasIssue(report, TopologyIssueCode::StaleNeighborGeneration),
                "stale neighbor generation was not detected");
    }
    {
        auto slots = validPair();
        slots[1].record.neighbors[3] = femcae::meshing::InvalidTetHandle;
        const auto report = femcae::meshing::validateTetTopology(slots);
        require(hasIssue(report, TopologyIssueCode::NonReciprocalNeighbor),
                "non-reciprocal neighbor was not detected");
    }
    {
        auto slots = validPair();
        slots[1].record.vertices = {6, 7, 8, 9};
        const auto report = femcae::meshing::validateTetTopology(slots);
        require(hasIssue(report, TopologyIssueCode::NeighborFaceMismatch),
                "neighbor face mismatch was not detected");
    }
    {
        auto slots = validPair();
        slots[1].live = false;
        const auto report = femcae::meshing::validateTetTopology(slots);
        require(hasIssue(report, TopologyIssueCode::NeighborDead),
                "dead neighbor was not detected");
    }
    {
        auto slots = validPair();
        slots.push_back(liveSlot(3, {4, 3, 2, 1}));
        const auto report = femcae::meshing::validateTetTopology(slots);
        require(hasIssue(report, TopologyIssueCode::DuplicateTetrahedron),
                "duplicate tetra connectivity was not detected");
    }
    {
        auto slots = validPair();
        slots[0].record.neighbors[0] = TetHandle{99, 1};
        const auto report = femcae::meshing::validateTetTopology(slots);
        require(hasIssue(report, TopologyIssueCode::NeighborOutOfRange),
                "out-of-range neighbor was not detected");
    }
}

void verifyHandleSemantics() {
    require(!femcae::meshing::InvalidTetHandle.isValid(), "invalid handle reports valid");
    require(TetHandle{3, 8}.isValid(), "valid generation handle reports invalid");
    require(TetHandle{3, 8} == TetHandle{3, 8}, "handle equality failed");
    require(!(TetHandle{3, 8} == TetHandle{3, 9}), "generation was ignored in handle equality");
}

} // namespace

int main() {
    try {
        verifyHandleSemantics();
        verifyFaceConvention();
        verifyValidPair();
        verifyCorruptionCorpus();
        std::cout << "M1.9-C tetra topology PASS "
                     "handles=yes opposite-face=yes reciprocal=yes corruption=yes\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "M1.9-C tetra topology FAIL: " << error.what() << '\n';
        return 1;
    }
}
