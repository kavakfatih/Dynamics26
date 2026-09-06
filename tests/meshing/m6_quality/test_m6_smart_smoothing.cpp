#include "meshing/m6/optimizer/SmartSmoothing.h"

#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

namespace o = femcae::meshing::m6::optimizer;
namespace q = femcae::meshing::m6::quality;
namespace s = femcae::meshing::m6::state;

using femcae::geometry::Vec3;
using femcae::meshing::CanonicalFaceKey;
using femcae::meshing::CanonicalSite;
using femcae::meshing::InvalidTetHandle;
using femcae::meshing::PointId;
using femcae::meshing::TetHandle;
using femcae::meshing::TetSlot;
namespace predicates = femcae::meshing::predicates;

[[noreturn]] void fail(const char* message) {
    throw std::runtime_error(message);
}

void require(bool condition, const char* message) {
    if (!condition) {
        fail(message);
    }
}

bool sameBits(double lhs, double rhs) {
    return std::bit_cast<std::uint64_t>(lhs) ==
           std::bit_cast<std::uint64_t>(rhs);
}

bool samePointBits(const Vec3& lhs, const Vec3& rhs) {
    return sameBits(lhs.x, rhs.x) &&
           sameBits(lhs.y, rhs.y) &&
           sameBits(lhs.z, rhs.z);
}

std::vector<CanonicalSite> sites(
    const Vec3& center) {
    return {
        {10U, center, {}},
        {30U, {1.0, 0.0, 0.0}, {}},
        {50U, {0.0, 1.0, 0.0}, {}},
        {70U, {0.0, 0.0, 1.0}, {}},
        {90U, {-1.0, -1.0, -1.0}, {}},
    };
}

const Vec3& coordinate(
    const std::map<PointId, Vec3>& points,
    PointId id) {
    const auto it = points.find(id);
    if (it == points.end()) {
        fail("test point lookup failed");
    }
    return it->second;
}

void orientPositive(
    std::array<PointId, 4>& vertices,
    const std::map<PointId, Vec3>& points) {
    const auto evaluate = [&]() {
        return predicates::orient3d(
            coordinate(points, vertices[0]),
            coordinate(points, vertices[1]),
            coordinate(points, vertices[2]),
            coordinate(points, vertices[3]));
    };

    auto orientation = evaluate();
    require(
        orientation.sign != predicates::PredicateSign::Zero,
        "test tetra unexpectedly degenerate");

    if (orientation.sign ==
        predicates::PredicateSign::Negative) {
        std::swap(vertices[2], vertices[3]);
        orientation = evaluate();
    }

    require(
        orientation.sign ==
            predicates::PredicateSign::Positive,
        "test tetra could not be oriented positive");
}

std::vector<TetSlot> makeStarSlots(
    const std::vector<CanonicalSite>& canonicalSites,
    bool reverseOrder = false) {
    std::map<PointId, Vec3> points;
    for (const CanonicalSite& site : canonicalSites) {
        points.emplace(site.id, site.point);
    }

    std::vector<std::array<PointId, 4>> cells{
        {10U, 30U, 50U, 70U},
        {10U, 30U, 90U, 50U},
        {10U, 30U, 70U, 90U},
        {10U, 50U, 90U, 70U},
    };

    if (reverseOrder) {
        std::reverse(cells.begin(), cells.end());
    }

    std::vector<TetSlot> slots(cells.size());
    for (std::size_t index = 0U;
         index < cells.size();
         ++index) {
        orientPositive(cells[index], points);
        slots[index].generation =
            static_cast<std::uint32_t>(index + 11U);
        slots[index].live = true;
        slots[index].record.vertices =
            cells[index];
        slots[index].record.neighbors = {
            InvalidTetHandle,
            InvalidTetHandle,
            InvalidTetHandle,
            InvalidTetHandle};
    }

    for (std::size_t lhs = 0U;
         lhs < slots.size();
         ++lhs) {
        for (std::size_t rhs = lhs + 1U;
             rhs < slots.size();
             ++rhs) {
            for (std::size_t lhsFace = 0U;
                 lhsFace < 4U;
                 ++lhsFace) {
                const CanonicalFaceKey lhsKey =
                    femcae::meshing::canonicalFaceKey(
                        slots[lhs].record,
                        lhsFace);

                for (std::size_t rhsFace = 0U;
                     rhsFace < 4U;
                     ++rhsFace) {
                    const CanonicalFaceKey rhsKey =
                        femcae::meshing::canonicalFaceKey(
                            slots[rhs].record,
                            rhsFace);
                    if (!(lhsKey == rhsKey)) {
                        continue;
                    }

                    slots[lhs].record.neighbors[lhsFace] = {
                        static_cast<std::uint32_t>(rhs),
                        slots[rhs].generation};
                    slots[rhs].record.neighbors[rhsFace] = {
                        static_cast<std::uint32_t>(lhs),
                        slots[lhs].generation};
                }
            }
        }
    }

    return slots;
}

s::ConstraintView constraints(
    s::PointMobility centerMobility =
        s::PointMobility::InteriorFree) {
    return s::ConstraintView({
        {10U, centerMobility},
        {30U, s::PointMobility::Fixed},
        {50U, s::PointMobility::Fixed},
        {70U, s::PointMobility::Fixed},
        {90U, s::PointMobility::Fixed},
    });
}

s::TetraOptimizationState makeState(
    const Vec3& center,
    bool reverseSlots = false,
    s::PointMobility centerMobility =
        s::PointMobility::InteriorFree) {
    const auto canonicalSites = sites(center);
    return s::TetraOptimizationState::fromCanonicalSites(
        canonicalSites,
        makeStarSlots(
            canonicalSites,
            reverseSlots),
        constraints(centerMobility));
}

void verifyImprovingProposal() {
    const Vec3 original{0.25, 0.1, -0.1};
    const s::TetraOptimizationState state =
        makeState(original);

    o::SmartSmoothingTelemetry telemetry;
    q::AcceptanceTelemetry acceptanceTelemetry;
    q::QualityVectorTelemetry vectorTelemetry;

    const o::SmartSmoothingProposal proposal =
        o::planSmartSmoothing(
            state,
            10U,
            &telemetry,
            &acceptanceTelemetry,
            &vectorTelemetry);

    require(
        proposal.status ==
            o::SmartSmoothingStatus::Improved,
        "offset star did not produce strict smoothing proposal");
    require(
        proposal.incidentTetrahedra.size() == 4U,
        "smoothing proposal incident-star size mismatch");
    require(
        proposal.sampleOrdinal == 0U,
        "centroid sample was not best on reference star");

    const Vec3 expectedCentroid{0.0, 0.0, 0.0};
    require(
        samePointBits(
            proposal.proposedPoint,
            expectedCentroid),
        "reference centroid proposal coordinate mismatch");

    require(
        q::compareQualityVectors(
            proposal.newQuality,
            proposal.oldQuality) ==
            q::QualityVectorOrder::Better,
        "O1 proposal is not strict D26QV1 improvement");

    require(
        samePointBits(
            state.point(10U),
            original),
        "O1 planning mutated authoritative point state");

    require(
        telemetry.calls == 1U &&
            telemetry.samplesEvaluated == 5U &&
            telemetry.strictImprovingSamples > 0U &&
            acceptanceTelemetry.accepted ==
                telemetry.strictImprovingSamples,
        "O1 telemetry partition mismatch");
}

void verifyNoImprovementAtCentroid() {
    const s::TetraOptimizationState state =
        makeState({0.0, 0.0, 0.0});

    const o::SmartSmoothingProposal proposal =
        o::planSmartSmoothing(
            state,
            10U);

    require(
        proposal.status ==
            o::SmartSmoothingStatus::NoImprovingSample,
        "centroid fixed point incorrectly produced strict proposal");
    require(
        samePointBits(
            state.point(10U),
            {0.0, 0.0, 0.0}),
        "no-improvement search mutated authoritative state");
}

void verifyConstraintAndInvalidTargets() {
    const s::TetraOptimizationState fixed =
        makeState(
            {0.25, 0.1, -0.1},
            false,
            s::PointMobility::Fixed);

    require(
        o::planSmartSmoothing(
            fixed,
            10U).status ==
            o::SmartSmoothingStatus::ConstraintBlocked,
        "fixed target was not blocked");

    const s::TetraOptimizationState free =
        makeState({0.25, 0.1, -0.1});
    require(
        o::planSmartSmoothing(
            free,
            999U).status ==
            o::SmartSmoothingStatus::InvalidTarget,
        "missing target did not return InvalidTarget");
}

void verifySlotPermutationReplay() {
    const Vec3 original{0.25, 0.1, -0.1};

    const s::TetraOptimizationState forward =
        makeState(
            original,
            false);
    const s::TetraOptimizationState reverse =
        makeState(
            original,
            true);

    const o::SmartSmoothingProposal lhs =
        o::planSmartSmoothing(
            forward,
            10U);
    const o::SmartSmoothingProposal rhs =
        o::planSmartSmoothing(
            reverse,
            10U);

    require(
        lhs.status ==
            o::SmartSmoothingStatus::Improved &&
            rhs.status ==
            o::SmartSmoothingStatus::Improved,
        "slot permutation changed O1 success status");
    require(
        lhs.sampleOrdinal ==
            rhs.sampleOrdinal,
        "slot permutation changed selected O1 sample");
    require(
        samePointBits(
            lhs.proposedPoint,
            rhs.proposedPoint),
        "slot permutation changed proposed coordinate");
    require(
        q::compareQualityVectors(
            lhs.newQuality,
            rhs.newQuality) ==
            q::QualityVectorOrder::Equal,
        "slot permutation changed semantic new D26QV1");
}

void verifyTransactionalCommit() {
    const Vec3 original{0.25, 0.1, -0.1};
    s::TetraOptimizationState state =
        makeState(original);

    o::SmartSmoothingTelemetry telemetry;
    q::AcceptanceTelemetry acceptanceTelemetry;
    q::QualityVectorTelemetry vectorTelemetry;

    const o::SmartSmoothingProposal proposal =
        o::planSmartSmoothing(
            state,
            10U,
            &telemetry,
            &acceptanceTelemetry,
            &vectorTelemetry);

    require(
        proposal.status ==
            o::SmartSmoothingStatus::Improved,
        "transaction fixture did not produce proposal");

    const o::SmartSmoothingCommitStatus committed =
        o::commitSmartSmoothing(
            state,
            proposal,
            &telemetry,
            &acceptanceTelemetry,
            &vectorTelemetry);

    require(
        committed ==
            o::SmartSmoothingCommitStatus::Committed,
        "strict smart-smoothing proposal did not commit");
    require(
        samePointBits(
            state.point(10U),
            proposal.proposedPoint),
        "committed coordinate does not match proposal");
    require(
        telemetry.commitCalls == 1U &&
            telemetry.committed == 1U &&
            telemetry.staleRejected == 0U &&
            telemetry.commitRejected == 0U,
        "smart-smoothing commit telemetry mismatch");

    const o::SmartSmoothingProposal after =
        o::planSmartSmoothing(
            state,
            10U);
    require(
        after.status ==
            o::SmartSmoothingStatus::NoImprovingSample,
        "committed reference centroid should be O1-stalled");
}

void verifyStaleProposalRejected() {
    const Vec3 sourceCenter{0.25, 0.1, -0.1};
    const s::TetraOptimizationState source =
        makeState(sourceCenter);
    const o::SmartSmoothingProposal proposal =
        o::planSmartSmoothing(
            source,
            10U);

    require(
        proposal.status ==
            o::SmartSmoothingStatus::Improved,
        "stale fixture source proposal missing");

    const Vec3 newerCenter{0.2, 0.1, -0.1};
    s::TetraOptimizationState newer =
        makeState(newerCenter);

    o::SmartSmoothingTelemetry telemetry;
    const o::SmartSmoothingCommitStatus status =
        o::commitSmartSmoothing(
            newer,
            proposal,
            &telemetry);

    require(
        status ==
            o::SmartSmoothingCommitStatus::StaleProposal,
        "stale snapshot proposal was not rejected");
    require(
        samePointBits(
            newer.point(10U),
            newerCenter),
        "stale proposal mutated newer authoritative state");
    require(
        telemetry.staleRejected == 1U &&
            telemetry.committed == 0U,
        "stale commit telemetry mismatch");
}

void verifyNonImprovingProposalCannotCommit() {
    s::TetraOptimizationState state =
        makeState({0.0, 0.0, 0.0});
    const o::SmartSmoothingProposal proposal =
        o::planSmartSmoothing(
            state,
            10U);

    require(
        proposal.status ==
            o::SmartSmoothingStatus::NoImprovingSample,
        "non-improving commit fixture unexpectedly improved");

    require(
        o::commitSmartSmoothing(
            state,
            proposal) ==
            o::SmartSmoothingCommitStatus::Rejected,
        "non-improving proposal entered commit path");
}

void verifyProtectedTopologyBlocksMotion() {
    const auto canonicalSites =
        sites({0.25, 0.1, -0.1});

    s::ConstraintView protectedConstraints(
        {
            {10U, s::PointMobility::InteriorFree},
            {30U, s::PointMobility::Fixed},
            {50U, s::PointMobility::Fixed},
            {70U, s::PointMobility::Fixed},
            {90U, s::PointMobility::Fixed},
        },
        {
            s::canonicalProtectedEdgeKey(
                10U,
                30U),
        });

    const s::TetraOptimizationState state =
        s::TetraOptimizationState::fromCanonicalSites(
            canonicalSites,
            makeStarSlots(canonicalSites),
            std::move(protectedConstraints));

    require(
        o::planSmartSmoothing(
            state,
            10U).status ==
            o::SmartSmoothingStatus::ConstraintBlocked,
        "protected topology target was allowed to move");
}

} // namespace

int main() {
    try {
        verifyImprovingProposal();
        verifyNoImprovementAtCentroid();
        verifyConstraintAndInvalidTargets();
        verifySlotPermutationReplay();
        verifyTransactionalCommit();
        verifyStaleProposalRejected();
        verifyNonImprovingProposalCannotCommit();
        verifyProtectedTopologyBlocksMotion();

        std::cout
            << "M6 I4a smart smoothing PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr
            << "M6 I4a smart smoothing FAIL: "
            << error.what()
            << '\n';
        return 1;
    }
}
