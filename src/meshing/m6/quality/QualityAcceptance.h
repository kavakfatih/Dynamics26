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

// Optional reuse hooks for a caller that evaluates several candidates against
// one unchanged old side, such as a line search over a vertex star. The frozen
// D26QACC1 sequence is unaffected; these only remove recomputation.
//
// `preparedOld` MUST be the result of buildQualityVector over the very same
// oldCells. Supplying it also transfers that call's exact-positive validation
// duty to the caller, because the old side is no longer rebuilt here and so no
// longer revalidated here.
//
// `capturedNew` receives the new-side vector this evaluation built, and is
// written only when the evaluation got far enough to build one.
struct AcceptanceVectorReuse {
    const QualityVector* preparedOld{nullptr};
    QualityVector* capturedNew{nullptr};
};

[[nodiscard]] AcceptanceEvaluation evaluateReplacement(
    std::span<const IndexedTetraCoordinates> oldCells,
    std::span<const IndexedTetraCoordinates> newCells,
    const ReplacementValidationEvidence& evidence,
    AcceptanceTelemetry* telemetry = nullptr,
    QualityVectorTelemetry* vectorTelemetry = nullptr,
    ExactMeanRatioTelemetry* exactTelemetry = nullptr,
    AcceptanceVectorReuse reuse = {});

} // namespace femcae::meshing::m6::quality
