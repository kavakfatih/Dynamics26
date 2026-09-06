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

} // namespace femcae::meshing::m6::optimizer
