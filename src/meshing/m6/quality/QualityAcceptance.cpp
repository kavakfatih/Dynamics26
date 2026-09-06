#include "QualityAcceptance.h"

#include <stdexcept>
#include <utility>

namespace femcae::meshing::m6::quality {
namespace {

AcceptanceEvaluation invalidCandidate(
    AcceptanceTelemetry* telemetry) noexcept {
    if (telemetry != nullptr) {
        ++telemetry->invalidCandidate;
    }
    return {
        AcceptanceDecision::InvalidCandidate,
        QualityVectorOrder::Equal};
}

AcceptanceEvaluation invalidPriorState(
    AcceptanceTelemetry* telemetry) noexcept {
    if (telemetry != nullptr) {
        ++telemetry->invalidPriorState;
    }
    return {
        AcceptanceDecision::InvalidPriorState,
        QualityVectorOrder::Equal};
}

} // namespace

AcceptanceEvaluation evaluateReplacement(
    std::span<const IndexedTetraCoordinates> oldCells,
    std::span<const IndexedTetraCoordinates> newCells,
    const ReplacementValidationEvidence& evidence,
    AcceptanceTelemetry* telemetry,
    QualityVectorTelemetry* vectorTelemetry,
    ExactMeanRatioTelemetry* exactTelemetry,
    AcceptanceVectorReuse reuse) {
    if (telemetry != nullptr) {
        ++telemetry->calls;
    }

    // Frozen D26QACC1 sequence:
    // old/new structural assumptions -> exact-positive new TET4 -> cavity
    // equivalence -> constraints/provenance -> strict D26QV1 -> commit.
    if (!evidence.structuralAssumptionsValid ||
        oldCells.empty() ||
        newCells.empty()) {
        return invalidCandidate(telemetry);
    }

    // The old side is part of step 1, not an implicit consequence of step 5.
    // Before this check the only thing that noticed an invalid prior cell was
    // buildQualityVector throwing further down, caught at the bottom and
    // reported as InvalidCandidate -- so "your replacement is bad" and "the
    // mesh you gave me was already bad" were the same answer with the same
    // counter, and the second one was attributed to the first.
    //
    // Skipped when the caller supplies a prepared old-side vector: building it
    // ran this very check, and re-running it per candidate is the per-sample
    // cost the reuse hook exists to remove.
    if (reuse.preparedOld == nullptr) {
        for (const IndexedTetraCoordinates& tetra : oldCells) {
            if (!isExactPositiveQualityCell(tetra)) {
                return invalidPriorState(telemetry);
            }
        }
    }

    for (const IndexedTetraCoordinates& tetra : newCells) {
        if (!isExactPositiveQualityCell(tetra)) {
            return invalidCandidate(telemetry);
        }
    }

    if (!evidence.cavityBoundaryEquivalent) {
        return invalidCandidate(telemetry);
    }

    if (!evidence.constraintsLegal) {
        if (telemetry != nullptr) {
            ++telemetry->constraintBlocked;
        }
        return {
            AcceptanceDecision::ConstraintBlocked,
            QualityVectorOrder::Equal};
    }

    try {
        QualityVector rebuiltOldVector;
        if (reuse.preparedOld == nullptr) {
            rebuiltOldVector =
                buildQualityVector(
                    oldCells,
                    vectorTelemetry,
                    exactTelemetry);
        }
        const QualityVector& oldVector =
            reuse.preparedOld != nullptr
                ? *reuse.preparedOld
                : rebuiltOldVector;

        QualityVector newVector =
            buildQualityVector(
                newCells,
                vectorTelemetry,
                exactTelemetry);

        const QualityVectorOrder order =
            compareQualityVectors(
                newVector,
                oldVector,
                vectorTelemetry,
                exactTelemetry);

        if (reuse.capturedNew != nullptr) {
            *reuse.capturedNew = std::move(newVector);
        }

        if (order == QualityVectorOrder::Better) {
            if (telemetry != nullptr) {
                ++telemetry->accepted;
            }
            return {
                AcceptanceDecision::Accept,
                order};
        }

        if (telemetry != nullptr) {
            ++telemetry->noImprovement;
            if (order == QualityVectorOrder::Equal) {
                ++telemetry->exactQualityTie;
            }
        }

        // Worse veya exact-equal quality: NO MUTATION.
        return {
            AcceptanceDecision::NoImprovement,
            order};
    } catch (const std::invalid_argument&) {
        return invalidCandidate(telemetry);
    }
}

} // namespace femcae::meshing::m6::quality
