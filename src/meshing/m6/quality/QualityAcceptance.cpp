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
    // structural assumptions -> exact-positive new TET4 -> cavity equivalence
    // -> constraints/provenance -> strict D26QV1 -> commit.
    if (!evidence.structuralAssumptionsValid ||
        oldCells.empty() ||
        newCells.empty()) {
        return invalidCandidate(telemetry);
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
