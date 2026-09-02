#pragma once

// Beta.2 B2.5 mixed u-p ve Contact verification telemetry routing.
//
// Bu helper'lar yalnız repository'de zaten var olan real verification solver
// consumer'larini typed telemetry'ye taşır. General AnalysisService nonlinear veya
// Contact model solve desteği iddia etmez ve document state üretmez.

#include "../core/SolverTelemetry.h"
#include "../services/MaterialService.h"
#include "../shell/Dynamics26MainWindow.h"

#include <femcae/femcae.h>

#include <cstddef>
#include <vector>

namespace d26 {

inline void runAdvancedMixedUpVerification(Dynamics26MainWindow &window)
{
    window.showUtility(UtilityWorkspace::Tab::SolverOutput, true);
    window.utility_->appendSolverOutput(QStringLiteral("──────────────────────────────────────────────"));
    window.utility_->appendSolverOutput(
        window.tr("Doğrulama preset'i: Mixed u-p HEX8/P0 manufactured simple shear"));

    const MaterialDefinition *material = window.materials_->assigned();
    const double c10 = (material != nullptr ? material->c10MPa : 1.0) * 1.0e6;
    const double bulk = (material != nullptr ? material->bulkMPa : 2000.0) * 1.0e6;

    constexpr int kCapacity = 512;
    std::vector<int> attempts(kCapacity), acceptedStepBefore(kCapacity), iterations(kCapacity), converged(kCapacity);
    std::vector<double> loadFactors(kCapacity), loadIncrements(kCapacity), residualNorms(kCapacity);
    std::vector<double> relativeResiduals(kCapacity), displacementIncrementNorms(kCapacity);
    std::vector<double> relativeDisplacements(kCapacity), pressureResidualNorms(kCapacity);
    std::vector<double> relativePressureResiduals(kCapacity), pressureIncrementNorms(kCapacity);
    std::vector<double> alphas(kCapacity), minimumJacobians(kCapacity);

    double recoveredGamma = 0.0;
    double pressure = 0.0;
    double completedLoadFactor = 0.0;
    double finalResidualNorm = 0.0;
    double finalPressureResidualNorm = 0.0;
    double minimumJacobian = 0.0;
    int acceptedSteps = 0;
    int totalIterations = 0;
    int cutbacks = 0;
    int historyCount = 0;

    const int status = fem_demo_mixed_up_hex8_shear_diagnostics(
        c10, bulk, 0.12,
        &recoveredGamma, &pressure, &completedLoadFactor,
        &finalResidualNorm, &finalPressureResidualNorm, &minimumJacobian,
        &acceptedSteps, &totalIterations, &cutbacks,
        kCapacity, &historyCount,
        attempts.data(), acceptedStepBefore.data(), iterations.data(),
        loadFactors.data(), loadIncrements.data(), residualNorms.data(),
        relativeResiduals.data(), displacementIncrementNorms.data(),
        relativeDisplacements.data(), pressureResidualNorms.data(),
        relativePressureResiduals.data(), pressureIncrementNorms.data(),
        alphas.data(), minimumJacobians.data(), converged.data());

    if (status != 0) {
        window.utility_->appendSolverOutput(
            window.tr("  BAŞARISIZ — engine status %1").arg(status));
        window.reportMessage(
            window.tr("Mixed u-p doğrulama başarısız (status %1).").arg(status),
            Severity::Error);
        return;
    }

    window.utility_->appendSolverOutput(
        window.tr("  Recovered shear γ   = %1").arg(recoveredGamma, 0, 'g', 8));
    window.utility_->appendSolverOutput(
        window.tr("  Element P0 pressure = %1 Pa").arg(pressure, 0, 'g', 8));
    window.utility_->appendSolverOutput(
        window.tr("  Final |Rp|          = %1").arg(finalPressureResidualNorm, 0, 'g', 8));
    window.utility_->appendSolverOutput(
        window.tr("  Minimum J           = %1").arg(minimumJacobian, 0, 'g', 8));

    SolverConvergenceSnapshot snapshot;
    snapshot.summary.executionMode = SolverExecutionMode::NonlinearNewton;
    snapshot.summary.state = SolverConvergenceState::Converged;
    snapshot.summary.completedLoadFactor = completedLoadFactor;
    snapshot.summary.finalResidualNorm = finalResidualNorm;
    snapshot.summary.acceptedSteps = acceptedSteps;
    snapshot.summary.totalIterations = totalIterations;
    snapshot.summary.cutbackCount = cutbacks;
    snapshot.summary.minimumJacobian = minimumJacobian;
    snapshot.summary.pressureMetrics = SolverMetricAvailability::Available;
    snapshot.summary.contactMetrics = SolverMetricAvailability::Unavailable;
    snapshot.summary.finalPressureResidualNorm = finalPressureResidualNorm;
    snapshot.entries.reserve(historyCount);

    for (int i = 0; i < historyCount; ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        SolverConvergenceEntry entry;
        entry.attempt = attempts[index];
        entry.iteration = iterations[index];
        entry.loadFactor = loadFactors[index];
        entry.relativeResidual = relativeResiduals[index];
        entry.relativeDisplacement = relativeDisplacements[index];
        entry.lineSearchAlpha = alphas[index];
        entry.converged = converged[index] != 0;
        entry.acceptedStepBefore = acceptedStepBefore[index];
        entry.loadIncrement = loadIncrements[index];
        entry.residualNorm = residualNorms[index];
        entry.displacementIncrementNorm = displacementIncrementNorms[index];
        entry.minimumJacobian = minimumJacobians[index];
        entry.pressureResidualNorm = pressureResidualNorms[index];
        entry.relativePressureResidual = relativePressureResiduals[index];
        entry.pressureIncrementNorm = pressureIncrementNorms[index];
        snapshot.entries.push_back(entry);
    }
    finalizeNonlinearDiagnostics(snapshot, 1.0e-8, 1.0e-8);

    window.utility_->setConvergenceData(snapshot);
    window.showUtility(UtilityWorkspace::Tab::Convergence, true);
    window.reportMessage(
        window.tr("Mixed u-p verification telemetry tamamlandı: %1 Newton kaydı.").arg(historyCount),
        Severity::Success);
}

inline void runAdvancedContactVerification(Dynamics26MainWindow &window)
{
    window.showUtility(UtilityWorkspace::Tab::SolverOutput, true);
    window.utility_->appendSolverOutput(QStringLiteral("──────────────────────────────────────────────"));
    window.utility_->appendSolverOutput(
        window.tr("Doğrulama preset'i: Rigid-master frictionless Contact"));

    const MaterialDefinition *material = window.materials_->assigned();
    const double young = (material != nullptr ? material->youngGPa : 210.0) * 1.0e9;
    const double poisson = material != nullptr ? material->poisson : 0.30;

    constexpr int kCapacity = 512;
    std::vector<int> attempts(kCapacity), acceptedStepBefore(kCapacity), iterations(kCapacity), converged(kCapacity);
    std::vector<int> active(kCapacity), stick(kCapacity), slip(kCapacity);
    std::vector<double> loadFactors(kCapacity), loadIncrements(kCapacity), residualNorms(kCapacity);
    std::vector<double> relativeResiduals(kCapacity), displacementIncrementNorms(kCapacity);
    std::vector<double> relativeDisplacements(kCapacity), alphas(kCapacity), minimumJacobians(kCapacity);
    std::vector<double> penetrations(kCapacity);

    double maximumPenetration = 0.0;
    double totalNormalForce = 0.0;
    double completedLoadFactor = 0.0;
    double finalResidualNorm = 0.0;
    double minimumJacobian = 0.0;
    int activeContacts = 0;
    int stickContacts = 0;
    int slipContacts = 0;
    int acceptedSteps = 0;
    int totalIterations = 0;
    int cutbacks = 0;
    int historyCount = 0;

    const int status = fem_demo_contact_hex8_diagnostics(
        young, poisson, young * 100.0, 1000.0, 1,
        &maximumPenetration, &totalNormalForce,
        &activeContacts, &stickContacts, &slipContacts,
        &completedLoadFactor, &finalResidualNorm, &minimumJacobian,
        &acceptedSteps, &totalIterations, &cutbacks,
        kCapacity, &historyCount,
        attempts.data(), acceptedStepBefore.data(), iterations.data(),
        loadFactors.data(), loadIncrements.data(), residualNorms.data(),
        relativeResiduals.data(), displacementIncrementNorms.data(),
        relativeDisplacements.data(), alphas.data(), minimumJacobians.data(),
        active.data(), stick.data(), slip.data(), penetrations.data(), converged.data());

    if (status != 0) {
        window.utility_->appendSolverOutput(
            window.tr("  BAŞARISIZ — engine status %1").arg(status));
        window.reportMessage(
            window.tr("Contact doğrulama başarısız (status %1).").arg(status),
            Severity::Error);
        return;
    }

    window.utility_->appendSolverOutput(
        window.tr("  Active / Stick / Slip = %1 / %2 / %3")
            .arg(activeContacts).arg(stickContacts).arg(slipContacts));
    window.utility_->appendSolverOutput(
        window.tr("  Max penetration       = %1 m").arg(maximumPenetration, 0, 'g', 8));
    window.utility_->appendSolverOutput(
        window.tr("  Normal contact force  = %1 N").arg(totalNormalForce, 0, 'g', 8));

    SolverConvergenceSnapshot snapshot;
    snapshot.summary.executionMode = SolverExecutionMode::NonlinearNewton;
    snapshot.summary.state = SolverConvergenceState::Converged;
    snapshot.summary.completedLoadFactor = completedLoadFactor;
    snapshot.summary.finalResidualNorm = finalResidualNorm;
    snapshot.summary.acceptedSteps = acceptedSteps;
    snapshot.summary.totalIterations = totalIterations;
    snapshot.summary.cutbackCount = cutbacks;
    snapshot.summary.minimumJacobian = minimumJacobian;
    snapshot.summary.pressureMetrics = SolverMetricAvailability::Unavailable;
    snapshot.summary.contactMetrics = SolverMetricAvailability::Available;
    snapshot.summary.finalActiveContactCount = activeContacts;
    snapshot.summary.finalStickContactCount = stickContacts;
    snapshot.summary.finalSlipContactCount = slipContacts;
    snapshot.summary.maximumPenetration = maximumPenetration;
    snapshot.summary.totalContactNormalForce = totalNormalForce;
    snapshot.entries.reserve(historyCount);

    for (int i = 0; i < historyCount; ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        SolverConvergenceEntry entry;
        entry.attempt = attempts[index];
        entry.iteration = iterations[index];
        entry.loadFactor = loadFactors[index];
        entry.relativeResidual = relativeResiduals[index];
        entry.relativeDisplacement = relativeDisplacements[index];
        entry.lineSearchAlpha = alphas[index];
        entry.converged = converged[index] != 0;
        entry.acceptedStepBefore = acceptedStepBefore[index];
        entry.loadIncrement = loadIncrements[index];
        entry.residualNorm = residualNorms[index];
        entry.displacementIncrementNorm = displacementIncrementNorms[index];
        entry.minimumJacobian = minimumJacobians[index];
        entry.activeContactCount = active[index];
        entry.stickContactCount = stick[index];
        entry.slipContactCount = slip[index];
        entry.maximumPenetration = penetrations[index];
        snapshot.entries.push_back(entry);
    }
    finalizeNonlinearDiagnostics(snapshot, 1.0e-8, 1.0e-8);

    window.utility_->setConvergenceData(snapshot);
    window.showUtility(UtilityWorkspace::Tab::Convergence, true);
    window.reportMessage(
        window.tr("Contact verification telemetry tamamlandı: %1 Newton kaydı.").arg(historyCount),
        Severity::Success);
}

} // namespace d26
