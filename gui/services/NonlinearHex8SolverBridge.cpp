#include "NonlinearHex8SolverBridge.h"

#include <femcae/femcae.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace d26 {

NonlinearHex8SolveOutput NonlinearHex8SolverBridge::solve(
    const femcae::application::Hex8SolverInput &input)
{
    using namespace femcae::application;

    NonlinearHex8SolveOutput output;
    output.telemetry.summary.executionMode = SolverExecutionMode::NonlinearNewton;
    output.telemetry.summary.state = SolverConvergenceState::Failed;

    const bool constraintShapeValid = input.constraintNodeIds.size() == input.constraintComponents.size()
        && input.constraintNodeIds.size() == input.constraintValues.size();
    const bool loadShapeValid = input.loadNodeIds.size() == input.loadComponents.size()
        && input.loadNodeIds.size() == input.loadValues.size();
    const bool methodValid = input.nonlinearControls.method == SnapshotNonlinearMethod::FullNewton
        || input.nonlinearControls.method == SnapshotNonlinearMethod::ModifiedNewton;
    if (input.apiVersion != AnalysisSnapshotApiVersion
        || input.analysisKind != SnapshotAnalysisKind::NonlinearStatic
        || input.formulation != SnapshotHex8Formulation::TotalLagrangianDisplacement
        || input.linearSystem.backend != SnapshotLinearBackend::DenseReference
        || input.nodeIds.empty() || input.elementIds.empty()
        || input.coordinatesXYZ.size() != 3 * input.nodeIds.size()
        || input.connectivity8.size() != 8 * input.elementIds.size()
        || !constraintShapeValid || !loadShapeValid || !methodValid
        || input.nodeIds.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())
        || input.elementIds.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())
        || input.constraintNodeIds.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())
        || input.loadNodeIds.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return output;
    }

    const SnapshotNonlinearControls &controls = input.nonlinearControls;
    const long long historyUpperBound = static_cast<long long>(controls.maximumStepAttempts)
        * (static_cast<long long>(controls.maximumIterations) + 1LL);
    if (historyUpperBound < 1
        || historyUpperBound > static_cast<long long>(std::numeric_limits<int>::max())) {
        return output;
    }
    // Inspector future'da daha buyuk limitler acsa bile GUI process'i kontrolsuz
    // bellek ayirmasin. ABI required_count ile eksik kapasiteyi acikca bildirir;
    // Beta.3 default'u (200 * 26 = 5200) bu sinirin cok altindadir.
    constexpr long long MaximumRetainedHistoryRows = 100000LL;
    const int historyCapacity = static_cast<int>(
        std::min(historyUpperBound, MaximumRetainedHistoryRows));

    output.displacementsXYZ.assign(3 * input.nodeIds.size(), 0.0);
    output.reactionsXYZ.assign(3 * input.nodeIds.size(), 0.0);
    output.elementEquivalentCauchy.assign(input.elementIds.size(), 0.0);

    std::vector<int> attempts(static_cast<std::size_t>(historyCapacity));
    std::vector<int> acceptedStepBefore(static_cast<std::size_t>(historyCapacity));
    std::vector<int> iterations(static_cast<std::size_t>(historyCapacity));
    std::vector<int> converged(static_cast<std::size_t>(historyCapacity));
    std::vector<double> loadFactors(static_cast<std::size_t>(historyCapacity));
    std::vector<double> loadIncrements(static_cast<std::size_t>(historyCapacity));
    std::vector<double> residualNorms(static_cast<std::size_t>(historyCapacity));
    std::vector<double> relativeResiduals(static_cast<std::size_t>(historyCapacity));
    std::vector<double> displacementIncrementNorms(static_cast<std::size_t>(historyCapacity));
    std::vector<double> relativeDisplacements(static_cast<std::size_t>(historyCapacity));
    std::vector<double> alphas(static_cast<std::size_t>(historyCapacity));
    std::vector<double> minimumJacobians(static_cast<std::size_t>(historyCapacity));

    int convergedFlag = 0;
    double completedLoadFactor = 0.0;
    double finalResidualNorm = 0.0;
    double minimumJacobian = 0.0;
    int acceptedSteps = 0;
    int stepAttempts = 0;
    int totalIterations = 0;
    int cutbacks = 0;
    int historyCount = 0;
    int historyRequiredCount = 0;

    output.status = fem_solve_nonlinear_static_hex8_v1(
        FEM_NONLINEAR_STATIC_HEX8_API_VERSION,
        static_cast<int>(input.nodeIds.size()), input.nodeIds.data(),
        input.coordinatesXYZ.data(),
        static_cast<int>(input.elementIds.size()), input.elementIds.data(),
        input.connectivity8.data(), input.youngModulusPa, input.poissonRatio,
        static_cast<int>(input.constraintNodeIds.size()), input.constraintNodeIds.data(),
        input.constraintComponents.data(), input.constraintValues.data(),
        static_cast<int>(input.loadNodeIds.size()), input.loadNodeIds.data(),
        input.loadComponents.data(), input.loadValues.data(),
        controls.method == SnapshotNonlinearMethod::ModifiedNewton
            ? FEM_NONLINEAR_METHOD_MODIFIED_NEWTON : FEM_NONLINEAR_METHOD_FULL_NEWTON,
        controls.maximumIterations, controls.maximumStepAttempts,
        controls.adaptiveStepping ? 1 : 0,
        controls.initialLoadIncrement, controls.minimumLoadIncrement,
        controls.maximumLoadIncrement, controls.cutbackFactor, controls.growthFactor,
        controls.targetIterations, controls.lineSearch ? 1 : 0,
        controls.lineSearchMaximumIterations, controls.lineSearchReduction,
        controls.lineSearchMinimumAlpha, controls.useResidualCriterion ? 1 : 0,
        controls.useDisplacementCriterion ? 1 : 0,
        controls.residualRelativeTolerance, controls.residualAbsoluteTolerance,
        controls.displacementRelativeTolerance, FEM_LINEAR_BACKEND_DENSE_REFERENCE,
        output.displacementsXYZ.data(), output.reactionsXYZ.data(),
        output.elementEquivalentCauchy.data(), &convergedFlag, &completedLoadFactor,
        &finalResidualNorm, &minimumJacobian, &acceptedSteps, &stepAttempts,
        &totalIterations, &cutbacks, historyCapacity, &historyCount,
        &historyRequiredCount, attempts.data(), acceptedStepBefore.data(),
        iterations.data(), loadFactors.data(), loadIncrements.data(),
        residualNorms.data(), relativeResiduals.data(),
        displacementIncrementNorms.data(), relativeDisplacements.data(),
        alphas.data(), minimumJacobians.data(), converged.data());

    output.converged = convergedFlag != 0;
    output.stepAttempts = stepAttempts;
    output.historyRequiredCount = historyRequiredCount;
    output.historyTruncated = historyRequiredCount > historyCount;
    SolverConvergenceSummary &summary = output.telemetry.summary;
    summary.state = output.status == 0 && output.converged
        ? SolverConvergenceState::Converged : SolverConvergenceState::Failed;
    summary.completedLoadFactor = completedLoadFactor;
    summary.finalResidualNorm = finalResidualNorm;
    summary.acceptedSteps = acceptedSteps;
    summary.totalIterations = totalIterations;
    summary.cutbackCount = cutbacks;
    if (std::isfinite(minimumJacobian)
        && std::abs(minimumJacobian) < std::numeric_limits<double>::max()) {
        summary.minimumJacobian = minimumJacobian;
    }
    // Displacement-only StVK path pressure/contact metric uretmez. Gercek sifir
    // degeri uydurmak yerine mevcut typed Unavailable contract'i korunur.
    summary.pressureMetrics = SolverMetricAvailability::Unavailable;
    summary.contactMetrics = SolverMetricAvailability::Unavailable;

    const int safeHistoryCount = std::clamp(historyCount, 0, historyCapacity);
    output.telemetry.entries.reserve(safeHistoryCount);
    for (int i = 0; i < safeHistoryCount; ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        SolverConvergenceEntry entry;
        entry.attempt = attempts[index];
        entry.acceptedStepBefore = acceptedStepBefore[index];
        entry.iteration = iterations[index];
        entry.loadFactor = loadFactors[index];
        entry.loadIncrement = loadIncrements[index];
        entry.residualNorm = residualNorms[index];
        entry.relativeResidual = relativeResiduals[index];
        entry.displacementIncrementNorm = displacementIncrementNorms[index];
        entry.relativeDisplacement = relativeDisplacements[index];
        entry.lineSearchAlpha = alphas[index];
        if (std::isfinite(minimumJacobians[index])
            && std::abs(minimumJacobians[index]) < std::numeric_limits<double>::max()) {
            entry.minimumJacobian = minimumJacobians[index];
        }
        entry.converged = converged[index] != 0;
        output.telemetry.entries.push_back(entry);
    }
    finalizeNonlinearDiagnostics(output.telemetry,
                                 controls.residualRelativeTolerance,
                                 controls.displacementRelativeTolerance);
    return output;
}

} // namespace d26
