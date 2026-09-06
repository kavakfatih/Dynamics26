#pragma once

#include "QualityVector.h"

#include <cstdint>
#include <span>

namespace femcae::meshing::m6::quality {

// I3'te topology/provenance authority henuz operation planner'larinda kurulmadigi
// icin bu evidence explicit caller-owned'dur. I4+ planner'lar cavity/constraint
// validator sonucunu burada true/false olarak tasiyacak; kalite katmani bu kaniti
// atlayamaz veya override edemez.
struct ReplacementValidationEvidence {
    bool structuralAssumptionsValid{true};
    bool cavityBoundaryEquivalent{true};
    bool constraintsLegal{true};
};

enum class AcceptanceDecision : std::uint8_t {
    Accept = 0,
    NoImprovement,
    InvalidCandidate,
    ConstraintBlocked
};

struct AcceptanceEvaluation {
    AcceptanceDecision decision{AcceptanceDecision::InvalidCandidate};
    QualityVectorOrder qualityOrder{QualityVectorOrder::Equal};
};

struct AcceptanceTelemetry {
    std::uint64_t calls{0};
    std::uint64_t accepted{0};
    std::uint64_t noImprovement{0};
    std::uint64_t exactQualityTie{0};
    std::uint64_t invalidCandidate{0};
    std::uint64_t constraintBlocked{0};
};

[[nodiscard]] AcceptanceEvaluation evaluateReplacement(
    std::span<const IndexedTetraCoordinates> oldCells,
    std::span<const IndexedTetraCoordinates> newCells,
    const ReplacementValidationEvidence& evidence,
    AcceptanceTelemetry* telemetry = nullptr,
    QualityVectorTelemetry* vectorTelemetry = nullptr,
    ExactMeanRatioTelemetry* exactTelemetry = nullptr);

} // namespace femcae::meshing::m6::quality
