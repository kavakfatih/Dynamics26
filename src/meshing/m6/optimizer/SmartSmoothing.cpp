#include "SmartSmoothing.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#ifdef __FAST_MATH__
#error "Dynamics26 M6 SmartSmoothing cannot be compiled with fast-math semantics"
#endif

#ifndef FEMCAE_FP_CONTRACT_CONTROLLED
#error "Dynamics26 M6 SmartSmoothing must be built through FEMCAE_CERTIFIED_FP_SOURCES so contraction control is enforced"
#endif

namespace femcae::meshing::m6::optimizer {
namespace {

// Frozen first-reference O1 private search policy.
//
// alpha = 1, 1/2, 1/4, 1/8, 1/16.
// These are exact binary fractions. Search is finite/count-bounded and sample
// order is semantic replay policy: exact-quality ties retain the earlier sample.
constexpr std::array<double, 5> kLineSearchAlpha{
    1.0,
    0.5,
    0.25,
    0.125,
    0.0625};

bool finitePoint(const geometry::Vec3& point) noexcept {
    return std::isfinite(point.x) &&
           std::isfinite(point.y) &&
           std::isfinite(point.z);
}

bool samePointBits(
    const geometry::Vec3& lhs,
    const geometry::Vec3& rhs) noexcept {
    return std::bit_cast<std::uint64_t>(lhs.x) ==
               std::bit_cast<std::uint64_t>(rhs.x) &&
           std::bit_cast<std::uint64_t>(lhs.y) ==
               std::bit_cast<std::uint64_t>(rhs.y) &&
           std::bit_cast<std::uint64_t>(lhs.z) ==
               std::bit_cast<std::uint64_t>(rhs.z);
}

geometry::Vec3 oneRingCentroid(
    const state::TetraOptimizationState& optimizationState,
    const std::vector<PointId>& neighbors) {
    geometry::Vec3 centroid{};
    const double count =
        static_cast<double>(neighbors.size());

    // Divide each finite coordinate before accumulation. The exact convex
    // centroid is bounded by the input range; this avoids a naive full sum
    // overflowing merely because many large same-sign coordinates are present.
    for (PointId neighbor : neighbors) {
        const geometry::Vec3& point =
            optimizationState.point(neighbor);
        centroid.x += point.x / count;
        centroid.y += point.y / count;
        centroid.z += point.z / count;
    }
    return centroid;
}

geometry::Vec3 lineSample(
    const geometry::Vec3& original,
    const geometry::Vec3& centroid,
    double alpha) noexcept {
    const double beta = 1.0 - alpha;

    // Convex form avoids forming centroid-original, which can overflow for
    // opposite-sign extreme finite coordinates. D26QACC1 remains authority for
    // the resulting binary64 candidate.
    return {
        beta * original.x + alpha * centroid.x,
        beta * original.y + alpha * centroid.y,
        beta * original.z + alpha * centroid.z};
}

std::vector<quality::IndexedTetraCoordinates> starCells(
    const state::TetraOptimizationState& optimizationState,
    const std::vector<TetHandle>& incident,
    std::optional<state::PointCoordinateOverride> override = std::nullopt) {
    std::vector<quality::IndexedTetraCoordinates> result;
    result.reserve(incident.size());

    for (TetHandle tetra : incident) {
        result.push_back(
            optimizationState.qualityCell(
                tetra,
                override));
    }
    return result;
}

} // namespace

SmartSmoothingProposal planSmartSmoothing(
    const state::TetraOptimizationState& optimizationState,
    PointId target,
    SmartSmoothingTelemetry* telemetry,
    quality::AcceptanceTelemetry* acceptanceTelemetry,
    quality::QualityVectorTelemetry* vectorTelemetry,
    quality::ExactMeanRatioTelemetry* exactTelemetry) {
    if (telemetry != nullptr) {
        ++telemetry->calls;
    }

    SmartSmoothingProposal result;
    result.target = target;

    if (!optimizationState.hasPoint(target)) {
        if (telemetry != nullptr) {
            ++telemetry->invalidTarget;
        }
        result.status = SmartSmoothingStatus::InvalidTarget;
        return result;
    }

    result.originalPoint =
        optimizationState.point(target);
    result.proposedPoint =
        result.originalPoint;

    const state::ConstraintView& constraints =
        optimizationState.constraints();

    if (!constraints.isInteriorFree(target) ||
        constraints.pointTouchesProtectedTopology(target)) {
        if (telemetry != nullptr) {
            ++telemetry->constraintBlocked;
        }
        result.status =
            SmartSmoothingStatus::ConstraintBlocked;
        return result;
    }

    result.incidentTetrahedra =
        optimizationState.incidentTetrahedra(target);
    const std::vector<PointId> neighbors =
        optimizationState.oneRingNeighbors(target);

    if (result.incidentTetrahedra.empty() ||
        neighbors.empty()) {
        if (telemetry != nullptr) {
            ++telemetry->invalidTarget;
        }
        result.status =
            SmartSmoothingStatus::InvalidTarget;
        return result;
    }

    const geometry::Vec3 centroid =
        oneRingCentroid(
            optimizationState,
            neighbors);
    if (!finitePoint(centroid)) {
        // Candidate-generator arithmetic failure is not equivalent to a
        // mathematical no-improvement claim.
        if (telemetry != nullptr) {
            ++telemetry->invalidTarget;
        }
        result.status =
            SmartSmoothingStatus::InvalidTarget;
        return result;
    }

    const std::vector<quality::IndexedTetraCoordinates>
        oldCells =
            starCells(
                optimizationState,
                result.incidentTetrahedra);

    result.oldQuality =
        quality::buildQualityVector(
            oldCells,
            vectorTelemetry,
            exactTelemetry);

    bool haveBest = false;
    quality::QualityVector bestQuality;
    geometry::Vec3 bestPoint =
        result.originalPoint;
    std::size_t bestOrdinal = 0U;

    const quality::ReplacementValidationEvidence evidence{
        true,
        true,
        true};

    for (std::size_t ordinal = 0U;
         ordinal < kLineSearchAlpha.size();
         ++ordinal) {
        const geometry::Vec3 candidate =
            lineSample(
                result.originalPoint,
                centroid,
                kLineSearchAlpha[ordinal]);

        if (telemetry != nullptr) {
            ++telemetry->samplesEvaluated;
        }

        if (!finitePoint(candidate)) {
            if (telemetry != nullptr) {
                ++telemetry->invalidSamples;
            }
            continue;
        }

        const state::PointCoordinateOverride override{
            target,
            candidate};

        const std::vector<quality::IndexedTetraCoordinates>
            newCells =
                starCells(
                    optimizationState,
                    result.incidentTetrahedra,
                    override);

        // The old side is the same unchanged star for every sample and was
        // already built above, and the new side is needed again below when the
        // sample wins. Both are handed to the evaluation rather than rebuilt.
        quality::QualityVector candidateQuality;
        const quality::AcceptanceEvaluation acceptance =
            quality::evaluateReplacement(
                oldCells,
                newCells,
                evidence,
                acceptanceTelemetry,
                vectorTelemetry,
                exactTelemetry,
                {&result.oldQuality, &candidateQuality});

        // Only a decision that actually compared quality counts as
        // non-improving. Every other non-Accept outcome is a sample that never
        // got that far, so it is counted as invalid rather than folded into
        // "evaluated, not better" -- including outcomes added to D26QACC1
        // later, which must not silently land in the wrong bucket.
        if (acceptance.decision ==
            quality::AcceptanceDecision::NoImprovement) {
            if (telemetry != nullptr) {
                ++telemetry->nonImprovingSamples;
            }
            continue;
        }

        if (acceptance.decision !=
            quality::AcceptanceDecision::Accept) {
            if (telemetry != nullptr) {
                ++telemetry->invalidSamples;
            }
            continue;
        }

        if (telemetry != nullptr) {
            ++telemetry->strictImprovingSamples;
        }

        bool replaceBest = !haveBest;
        if (haveBest) {
            replaceBest =
                quality::compareQualityVectors(
                    candidateQuality,
                    bestQuality,
                    vectorTelemetry,
                    exactTelemetry) ==
                quality::QualityVectorOrder::Better;
        }

        if (replaceBest) {
            haveBest = true;
            bestQuality =
                std::move(candidateQuality);
            bestPoint = candidate;
            bestOrdinal = ordinal;
            if (telemetry != nullptr) {
                ++telemetry->bestReplacements;
            }
        }
    }

    if (!haveBest) {
        result.status =
            SmartSmoothingStatus::NoImprovingSample;
        return result;
    }

    result.status =
        SmartSmoothingStatus::Improved;
    result.proposedPoint = bestPoint;
    result.sampleOrdinal = bestOrdinal;
    result.newQuality = std::move(bestQuality);
    return result;
}

SmartSmoothingCommitStatus commitSmartSmoothing(
    state::TetraOptimizationState& optimizationState,
    const SmartSmoothingProposal& proposal,
    SmartSmoothingTelemetry* telemetry,
    quality::AcceptanceTelemetry* acceptanceTelemetry,
    quality::QualityVectorTelemetry* vectorTelemetry,
    quality::ExactMeanRatioTelemetry* exactTelemetry) {
    if (telemetry != nullptr) {
        ++telemetry->commitCalls;
    }

    const auto reject = [telemetry]() {
        if (telemetry != nullptr) {
            ++telemetry->commitRejected;
        }
        return SmartSmoothingCommitStatus::Rejected;
    };
    const auto stale = [telemetry]() {
        if (telemetry != nullptr) {
            ++telemetry->staleRejected;
        }
        return SmartSmoothingCommitStatus::StaleProposal;
    };

    if (proposal.status != SmartSmoothingStatus::Improved ||
        proposal.target == InvalidPointId ||
        !finitePoint(proposal.proposedPoint)) {
        return reject();
    }

    if (!optimizationState.hasPoint(proposal.target) ||
        !samePointBits(
            optimizationState.point(proposal.target),
            proposal.originalPoint)) {
        return stale();
    }

    const state::ConstraintView& constraints =
        optimizationState.constraints();
    if (!constraints.isInteriorFree(proposal.target) ||
        constraints.pointTouchesProtectedTopology(
            proposal.target)) {
        return reject();
    }

    const std::vector<TetHandle> currentIncident =
        optimizationState.incidentTetrahedra(
            proposal.target);
    if (currentIncident != proposal.incidentTetrahedra) {
        return stale();
    }

    const std::vector<quality::IndexedTetraCoordinates>
        oldCells =
            starCells(
                optimizationState,
                currentIncident);
    const quality::QualityVector currentOldQuality =
        quality::buildQualityVector(
            oldCells,
            vectorTelemetry,
            exactTelemetry);

    if (quality::compareQualityVectors(
            currentOldQuality,
            proposal.oldQuality,
            vectorTelemetry,
            exactTelemetry) !=
        quality::QualityVectorOrder::Equal) {
        return stale();
    }

    const state::PointCoordinateOverride override{
        proposal.target,
        proposal.proposedPoint};
    const std::vector<quality::IndexedTetraCoordinates>
        newCells =
            starCells(
                optimizationState,
                currentIncident,
                override);

    const quality::ReplacementValidationEvidence evidence{
        true,
        true,
        true};
    quality::QualityVector currentNewQuality;
    const quality::AcceptanceEvaluation acceptance =
        quality::evaluateReplacement(
            oldCells,
            newCells,
            evidence,
            acceptanceTelemetry,
            vectorTelemetry,
            exactTelemetry,
            {&currentOldQuality, &currentNewQuality});
    if (acceptance.decision !=
        quality::AcceptanceDecision::Accept) {
        return reject();
    }

    if (quality::compareQualityVectors(
            currentNewQuality,
            proposal.newQuality,
            vectorTelemetry,
            exactTelemetry) !=
        quality::QualityVectorOrder::Equal) {
        return stale();
    }

    if (!optimizationState.applyValidatedPointCoordinate(
            proposal.target,
            proposal.proposedPoint)) {
        return reject();
    }

    if (telemetry != nullptr) {
        ++telemetry->committed;
    }
    return SmartSmoothingCommitStatus::Committed;
}

} // namespace femcae::meshing::m6::optimizer
