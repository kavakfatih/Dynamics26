#pragma once

#include "femcae/meshing/RobustPredicates.h"

namespace femcae::meshing::predicates::internal {

struct PredicateDetailedTelemetry {
    PredicateTelemetry orient2d;
    PredicateTelemetry orient3d;
    PredicateTelemetry incircle;
    PredicateTelemetry insphere;
};

// Private qualification sinks. They are intentionally absent from installed
// headers and thread-local so observational qualification state cannot enter
// predicate, topology or allocation identity.
void setPredicateCallAuditSink(PredicateTelemetry* telemetry) noexcept;
void setPredicateDetailedAuditSink(
    PredicateDetailedTelemetry* telemetry) noexcept;

} // namespace femcae::meshing::predicates::internal
