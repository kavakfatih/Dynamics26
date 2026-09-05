#include "femcae/meshing/TetraTopology.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <set>
#include <stdexcept>

namespace femcae::meshing {
namespace {

using CanonicalTetKey = std::array<PointId, 4>;

CanonicalTetKey canonicalTetKey(const TetRecord& tet) {
    CanonicalTetKey key = tet.vertices;
    std::sort(key.begin(), key.end());
    return key;
}

TetHandle selfHandle(std::size_t slotIndex, const TetSlot& slot) {
    return {
        static_cast<std::uint32_t>(slotIndex),
        slot.generation};
}

void addIssue(
    TopologyValidationReport& report,
    TopologyIssueCode code,
    TetHandle tetra,
    std::size_t localFace = 0U) {
    report.issues.push_back({
        code,
        tetra,
        static_cast<std::uint8_t>(localFace)});
}

} // namespace

CanonicalFaceKey canonicalFaceKey(
    const TetRecord& tet,
    std::size_t oppositeVertex) {
    if (oppositeVertex >= 4U) {
        throw std::out_of_range("tetra local face index must be in [0,3]");
    }

    CanonicalFaceKey key;
    std::size_t write = 0U;
    for (std::size_t vertex = 0U; vertex < 4U; ++vertex) {
        if (vertex != oppositeVertex) {
            key.vertices[write++] = tet.vertices[vertex];
        }
    }
    std::sort(key.vertices.begin(), key.vertices.end());
    return key;
}

TopologyValidationReport validateTetTopology(std::span<const TetSlot> slots) {
    TopologyValidationReport report;
    std::map<CanonicalTetKey, TetHandle> seenTetrahedra;

    for (std::size_t slotIndex = 0U; slotIndex < slots.size(); ++slotIndex) {
        const TetSlot& slot = slots[slotIndex];
        if (!slot.live) {
            continue;
        }

        const TetHandle self = selfHandle(slotIndex, slot);
        if (slot.generation == 0U) {
            addIssue(report, TopologyIssueCode::LiveSlotZeroGeneration, self);
        }

        bool hasInvalidVertex = false;
        for (PointId vertex : slot.record.vertices) {
            if (vertex == InvalidPointId) {
                hasInvalidVertex = true;
                break;
            }
        }
        if (hasInvalidVertex) {
            addIssue(report, TopologyIssueCode::InvalidVertex, self);
        }

        const CanonicalTetKey tetKey = canonicalTetKey(slot.record);
        if (std::adjacent_find(tetKey.begin(), tetKey.end()) != tetKey.end()) {
            addIssue(report, TopologyIssueCode::DuplicateVertex, self);
        }

        const auto [seenIt, inserted] = seenTetrahedra.emplace(tetKey, self);
        if (!inserted) {
            addIssue(report, TopologyIssueCode::DuplicateTetrahedron, self);
        }

        for (std::size_t localFace = 0U; localFace < 4U; ++localFace) {
            const TetHandle neighbor = slot.record.neighbors[localFace];
            if (!neighbor.isValid()) {
                continue;
            }
            if (neighbor.slot >= slots.size()) {
                addIssue(report, TopologyIssueCode::NeighborOutOfRange, self, localFace);
                continue;
            }

            const TetSlot& neighborSlot = slots[neighbor.slot];
            if (!neighborSlot.live) {
                addIssue(report, TopologyIssueCode::NeighborDead, self, localFace);
                continue;
            }
            if (neighborSlot.generation != neighbor.generation) {
                addIssue(report, TopologyIssueCode::StaleNeighborGeneration, self, localFace);
                continue;
            }

            const CanonicalFaceKey expectedFace =
                canonicalFaceKey(slot.record, localFace);

            std::size_t matchingFace = 4U;
            std::size_t matchCount = 0U;
            for (std::size_t neighborFace = 0U; neighborFace < 4U; ++neighborFace) {
                if (canonicalFaceKey(neighborSlot.record, neighborFace) == expectedFace) {
                    matchingFace = neighborFace;
                    ++matchCount;
                }
            }

            if (matchCount != 1U) {
                addIssue(report, TopologyIssueCode::NeighborFaceMismatch, self, localFace);
                continue;
            }

            const TetHandle reciprocal = neighborSlot.record.neighbors[matchingFace];
            if (reciprocal != self) {
                addIssue(report, TopologyIssueCode::NonReciprocalNeighbor, self, localFace);
            }
        }
    }

    return report;
}

} // namespace femcae::meshing
