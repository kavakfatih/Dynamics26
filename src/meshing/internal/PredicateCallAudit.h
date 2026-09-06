#pragma once

#include "femcae/meshing/RobustPredicates.h"

namespace femcae::meshing::predicates::internal {

// Private qualification sink. It reuses PredicateTelemetry::calls and is
// intentionally absent from installed/public headers. The active sink is
// thread-local so qualification state cannot enter topology identity.
void setPredicateCallAuditSink(PredicateTelemetry* telemetry) noexcept;

} // namespace femcae::meshing::predicates::internal
