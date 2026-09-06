#pragma once

#include "../quality/QualityAcceptance.h"
#include "../state/TetraOptimizationState.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace femcae::meshing::m6::optimizer {

enum class SmartSmoothingStatus : std::uint8_t {
    Improved = 0,
    NoImprovingSample,
    ConstraintBlocked,
    InvalidTarget
};

enum class SmartSmoothingCommitStatus : std::uint8_t {
    Committed = 0,
    StaleProposal,
    Rejected
};

struct SmartSmoothingProposal {
    SmartSmoothingStatus status{SmartSmoothingStatus::InvalidTarget};
    PointId target{InvalidPointId};
    geometry::Vec3 originalPoint{};
    geometry::Vec3 proposedPoint{};
    std::size_t sampleOrdinal{0};
    std::vector<TetHandle> incidentTetrahedra;
    quality::QualityVector oldQuality;
    quality::QualityVector newQuality;
};

struct SmartSmoothingTelemetry {
    std::uint64_t calls{0};
    std::uint64_t constraintBlocked{0};
    std::uint64_t invalidTarget{0};
    std::uint64_t samplesEvaluated{0};
    std::uint64_t invalidSamples{0};
    std::uint64_t nonImprovingSamples{0};
    std::uint64_t strictImprovingSamples{0};
    std::uint64_t bestReplacements{0};
    std::uint64_t commitCalls{0};
    std::uint64_t committed{0};
    std::uint64_t staleRejected{0};
    std::uint64_t commitRejected{0};
};

// O1 first reference policy:
// - one-ring centroid direction,
// - fixed count-bounded dyadic line-search samples,
// - authoritative state remains immutable,
// - every accepted sample passes D26QACC1.
[[nodiscard]] SmartSmoothingProposal planSmartSmoothing(
    const state::TetraOptimizationState& state,
    PointId target,
    SmartSmoothingTelemetry* telemetry = nullptr,
    quality::AcceptanceTelemetry* acceptanceTelemetry = nullptr,
    quality::QualityVectorTelemetry* vectorTelemetry = nullptr,
    quality::ExactMeanRatioTelemetry* exactTelemetry = nullptr);

// Serial reference commit: proposal snapshot'i yeniden dogrulanir, strict
// D26QACC1 tekrar calistirilir ve ancak bundan sonra tek coordinate mutation
// transactional olarak authoritative state'e uygulanir.
[[nodiscard]] SmartSmoothingCommitStatus commitSmartSmoothing(
    state::TetraOptimizationState& state,
    const SmartSmoothingProposal& proposal,
    SmartSmoothingTelemetry* telemetry = nullptr,
    quality::AcceptanceTelemetry* acceptanceTelemetry = nullptr,
    quality::QualityVectorTelemetry* vectorTelemetry = nullptr,
    quality::ExactMeanRatioTelemetry* exactTelemetry = nullptr);

} // namespace femcae::meshing::m6::optimizer
