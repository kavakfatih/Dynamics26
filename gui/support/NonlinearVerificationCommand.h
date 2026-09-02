#pragma once

// Beta.2 B2.5 canonical nonlinear verification command.
//
// Eski verify.nonlinear QAction/menu/shortcut identity korunur; composition root
// bu handler'a route eder. Tek solver çağrısı advanced diagnostics C ABI'den hem
// B2.1 basic convergence subset'ini hem B2.5 advanced metrikleri üretir.
// Widget solver state sahibi değildir ve burada document mutation yapılmaz.

#include "../core/SolverTelemetry.h"
#include "../services/MaterialService.h"
#include "../shell/Dynamics26MainWindow.h"

#include <femcae/femcae.h>

#include <cstddef>
#include <vector>

namespace d26 {

inline void runAdvancedNonlinearVerification(Dynamics26MainWindow &window)
{
    window.showUtility(UtilityWorkspace::Tab::SolverOutput, true);
    window.utility_->appendSolverOutput(QStringLiteral("──────────────────────────────────────────────"));
    window.utility_->appendSolverOutput(
        window.tr("Doğrulama preset'i: Total-Lagrangian HEX8 nonlineer çubuk"));

    const MaterialDefinition *material = window.materials_->assigned();
    const double young = (material != nullptr ? material->youngGPa : 210.0) * 1.0e9;
    const double poisson = material != nullptr ? material->poisson : 0.30;

    constexpr int kCapacity = 512;
    std::vector<int> attempts(kCapacity);
    std::vector<int> acceptedStepBefore(kCapacity);
    std::vector<int> iterations(kCapacity);
    std::vector<int> converged(kCapacity);
    std::vector<double> loadFactors(kCapacity);
    std::vector<double> loadIncrements(kCapacity);
    std::vector<double> residualNorms(kCapacity);
    std::vector<double> relativeResiduals(kCapacity);
    std::vector<double> displacementIncrementNorms(kCapacity);
    std::vector<double> relativeDisplacements(kCapacity);
    std::vector<double> alphas(kCapacity);
    std::vector<double> minimumJacobians(kCapacity);

    double displacement = 0.0;
    double completedLoadFactor = 0.0;
    double finalResidual = 0.0;
    double minimumJacobian = 0.0;
    int acceptedSteps = 0;
    int totalIterations = 0;
    int cutbacks = 0;
    int historyCount = 0;

    const int status = fem_demo_nonlinear_hex8_diagnostics(
        young, poisson, 1.0e-4, 1.0, 1000.0,
        0.25, 0.01, 0.5, 1, 1, 25, 1,
        &displacement, &completedLoadFactor, &finalResidual, &minimumJacobian,
        &acceptedSteps, &totalIterations, &cutbacks,
        kCapacity, &historyCount,
        attempts.data(), acceptedStepBefore.data(), iterations.data(),
        loadFactors.data(), loadIncrements.data(), residualNorms.data(),
        relativeResiduals.data(), displacementIncrementNorms.data(),
        relativeDisplacements.data(), alphas.data(), minimumJacobians.data(),
        converged.data());

    if (status != 0) {
        window.utility_->appendSolverOutput(
            window.tr("  BAŞARISIZ — engine status %1").arg(status));
        window.reportMessage(
            window.tr("Nonlineer doğrulama başarısız (status %1).").arg(status),
            Severity::Error);
        return;
    }

    window.utility_->appendSolverOutput(
        window.tr("  Uç deplasman        = %1 mm").arg(displacement * 1.0e3, 0, 'g', 8));
    window.utility_->appendSolverOutput(
        window.tr("  Tamamlanan λ        = %1").arg(completedLoadFactor, 0, 'g', 8));
    window.utility_->appendSolverOutput(
        window.tr("  Kabul edilen adım   = %1").arg(acceptedSteps));
    window.utility_->appendSolverOutput(
        window.tr("  Newton düzeltmesi   = %1").arg(totalIterations));
    window.utility_->appendSolverOutput(
        window.tr("  Cutback             = %1").arg(cutbacks));
    window.utility_->appendSolverOutput(
        window.tr("  Minimum J           = %1").arg(minimumJacobian, 0, 'g', 8));

    SolverConvergenceSnapshot snapshot;
    snapshot.summary.executionMode = SolverExecutionMode::NonlinearNewton;
    snapshot.summary.state = SolverConvergenceState::Converged;
    snapshot.summary.completedLoadFactor = completedLoadFactor;
    snapshot.summary.finalResidualNorm = finalResidual;
    snapshot.summary.acceptedSteps = acceptedSteps;
    snapshot.summary.totalIterations = totalIterations;
    snapshot.summary.cutbackCount = cutbacks;
    snapshot.summary.minimumJacobian = minimumJacobian;
    // Bu preset displacement-only ve Contact'sizdir. B2.5 engineering kuralı:
    // uygulanmayan metrics 0 olarak uydurulmaz, explicit Unavailable kalır.
    snapshot.summary.pressureMetrics = SolverMetricAvailability::Unavailable;
    snapshot.summary.contactMetrics = SolverMetricAvailability::Unavailable;
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
        snapshot.entries.push_back(entry);
    }

    // Verification preset'in nonlinear_solver_options_t default convergence
    // toleransları bunlardır. Criterion state bu gerçek solver ayarlarından
    // türetilir; widget kendi eşik değerini icat etmez.
    finalizeNonlinearDiagnostics(snapshot, 1.0e-8, 1.0e-8);

    window.utility_->setConvergenceData(snapshot);
    window.showUtility(UtilityWorkspace::Tab::Convergence, true);
    window.reportMessage(
        window.tr("Nonlineer doğrulama tamamlandı: %1 yakınsama kaydı.").arg(historyCount),
        Severity::Success);
}

} // namespace d26
