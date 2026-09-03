#pragma once

// Immutable AnalysisSnapshot'tan uretilen owning Hex8SolverInput ile general
// Fortran nonlinear product ABI arasindaki tek application bridge'i. Bu sinir
// document, Qt widget, live mesh veya selection nesnesi okumaz.

#include "../core/SolverTelemetry.h"

#include <femcae/application/SolverInputBuilder.h>

#include <vector>

namespace d26 {

struct NonlinearHex8SolveOutput {
    int status{10};
    bool converged{false};
    int stepAttempts{0};
    int historyRequiredCount{0};
    bool historyTruncated{false};
    std::vector<double> displacementsXYZ;
    std::vector<double> reactionsXYZ;
    std::vector<double> elementEquivalentCauchy;
    SolverConvergenceSnapshot telemetry;
};

class NonlinearHex8SolverBridge final
{
public:
    [[nodiscard]] static NonlinearHex8SolveOutput solve(
        const femcae::application::Hex8SolverInput &input);
};

} // namespace d26
