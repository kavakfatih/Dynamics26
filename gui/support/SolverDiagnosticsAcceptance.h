#pragma once

#include "../core/DocumentCommandManager.h"
#include "../core/SolverTelemetry.h"
#include "../shell/Dynamics26MainWindow.h"
#include "../shell/UtilityWorkspace.h"

#include <femcae/femcae.h>

#include <QApplication>
#include <QLabel>
#include <QTableWidget>
#include <QUndoStack>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace d26 {

inline int runSolverDiagnosticsAcceptanceTest(QApplication &app, Dynamics26MainWindow &window)
{
    int checks = 0;
    int failures = 0;
    const auto check = [&](const bool condition, const char *message) {
        ++checks;
        if (condition) {
            std::cout << "PASS Solver diagnostics acceptance: " << message << '\n';
        } else {
            ++failures;
            std::cerr << "FAIL Solver diagnostics acceptance: " << message << '\n';
        }
    };
    const auto renderedValueMatches = [](const QTableWidgetItem *item, const double expected) {
        if (item == nullptr) {
            return false;
        }
        bool ok = false;
        const double actual = item->text().toDouble(&ok);
        const double tolerance = 1.0e-7 * std::max(std::abs(expected), 1.0e-12);
        return ok && std::abs(actual - expected) <= tolerance;
    };

    UtilityWorkspace *utility = window.utility();
    QUndoStack *undoStack = window.documentCommands() != nullptr
        ? window.documentCommands()->stack() : nullptr;
    check(utility != nullptr, "Utility Workspace exists");
    check(undoStack != nullptr, "Document Undo stack exists");
    if (utility == nullptr || undoStack == nullptr) {
        return 1;
    }

    auto *basicTable = utility->findChild<QTableWidget *>(
        QStringLiteral("Dynamics26UtilityConvergence"));
    auto *diagnosticsTable = utility->findChild<QTableWidget *>(
        QStringLiteral("Dynamics26UtilityConvergenceDiagnostics"));
    auto *diagnosticsSummary = utility->findChild<QLabel *>(
        QStringLiteral("Dynamics26UtilityConvergenceDiagnosticsSummary"));
    check(basicTable != nullptr, "B2.1 convergence table remains addressable");
    check(diagnosticsTable != nullptr, "B2.5 diagnostics table has stable objectName");
    check(diagnosticsSummary != nullptr, "B2.5 diagnostics summary has stable objectName");
    if (basicTable == nullptr || diagnosticsTable == nullptr || diagnosticsSummary == nullptr) {
        return 1;
    }

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

    constexpr double young = 6.0e6;
    constexpr double poisson = 0.29;
    constexpr double area = 1.0;
    constexpr double length = 1.0;
    constexpr double stretch = 1.10;
    const double shear = young / (2.0 * (1.0 + poisson));
    const double lame = young * poisson / ((1.0 + poisson) * (1.0 - 2.0 * poisson));
    const double greenLagrange = 0.5 * (stretch * stretch - 1.0);
    const double force = area * stretch * (lame + 2.0 * shear) * greenLagrange;

    double displacement = 0.0;
    double completedLoadFactor = 0.0;
    double finalResidualNorm = 0.0;
    double minimumJacobian = 0.0;
    int acceptedSteps = 0;
    int totalIterations = 0;
    int cutbacks = 0;
    int historyCount = 0;

    const int status = fem_demo_nonlinear_hex8_diagnostics(
        young, poisson, area, length, force,
        0.25, 0.01, 0.5, 1, 1, 25, 1,
        &displacement, &completedLoadFactor, &finalResidualNorm, &minimumJacobian,
        &acceptedSteps, &totalIterations, &cutbacks,
        kCapacity, &historyCount,
        attempts.data(), acceptedStepBefore.data(), iterations.data(),
        loadFactors.data(), loadIncrements.data(), residualNorms.data(),
        relativeResiduals.data(), displacementIncrementNorms.data(),
        relativeDisplacements.data(), alphas.data(), minimumJacobians.data(),
        converged.data());

    check(status == 0, "Advanced diagnostics C ABI solve succeeds");
    check(historyCount > 0, "Advanced diagnostics C ABI returns real history rows");
    check(minimumJacobian > 0.0, "Session minimum J is physically valid and available");
    if (status != 0 || historyCount <= 0) {
        return 1;
    }

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
    snapshot.summary.contactMetrics = SolverMetricAvailability::Unavailable;
    snapshot.entries.reserve(historyCount);

    for (int i = 0; i < historyCount; ++i) {
        SolverConvergenceEntry entry;
        entry.attempt = attempts[static_cast<std::size_t>(i)];
        entry.iteration = iterations[static_cast<std::size_t>(i)];
        entry.loadFactor = loadFactors[static_cast<std::size_t>(i)];
        entry.relativeResidual = relativeResiduals[static_cast<std::size_t>(i)];
        entry.relativeDisplacement = relativeDisplacements[static_cast<std::size_t>(i)];
        entry.lineSearchAlpha = alphas[static_cast<std::size_t>(i)];
        entry.converged = converged[static_cast<std::size_t>(i)] != 0;
        entry.acceptedStepBefore = acceptedStepBefore[static_cast<std::size_t>(i)];
        entry.loadIncrement = loadIncrements[static_cast<std::size_t>(i)];
        entry.residualNorm = residualNorms[static_cast<std::size_t>(i)];
        entry.displacementIncrementNorm = displacementIncrementNorms[static_cast<std::size_t>(i)];
        entry.minimumJacobian = minimumJacobians[static_cast<std::size_t>(i)];
        snapshot.entries.push_back(entry);
    }
    finalizeNonlinearDiagnostics(snapshot, 1.0e-8, 1.0e-8);

    const int undoCountBefore = undoStack->count();
    const int undoIndexBefore = undoStack->index();
    const bool dirtyBefore = window.documentCommands()->isDirty();
    utility->setConvergenceData(snapshot);
    app.processEvents();

    check(undoStack->count() == undoCountBefore
              && undoStack->index() == undoIndexBefore
              && window.documentCommands()->isDirty() == dirtyBefore,
          "Advanced diagnostics presentation creates no document mutation");
    check(basicTable->columnCount() == 7,
          "B2.5 preserves the existing seven-column basic convergence contract");
    check(diagnosticsTable->columnCount() == 8,
          "Advanced diagnostics table has the expected typed metric contract");
    check(diagnosticsTable->rowCount() == historyCount,
          "Advanced diagnostics table renders every authoritative history row");

    bool loadIncrementMatches = diagnosticsTable->rowCount() == historyCount;
    bool residualNormMatches = loadIncrementMatches;
    bool displacementIncrementMatches = loadIncrementMatches;
    bool minimumJacobianMatches = loadIncrementMatches;
    bool adaptiveStateAvailable = loadIncrementMatches;
    bool criterionStateAvailable = loadIncrementMatches;
    for (int row = 0; row < historyCount && row < diagnosticsTable->rowCount(); ++row) {
        const std::size_t i = static_cast<std::size_t>(row);
        loadIncrementMatches = loadIncrementMatches
            && renderedValueMatches(diagnosticsTable->item(row, 2), loadIncrements[i]);
        residualNormMatches = residualNormMatches
            && renderedValueMatches(diagnosticsTable->item(row, 3), residualNorms[i]);
        displacementIncrementMatches = displacementIncrementMatches
            && renderedValueMatches(diagnosticsTable->item(row, 4), displacementIncrementNorms[i]);
        minimumJacobianMatches = minimumJacobianMatches
            && renderedValueMatches(diagnosticsTable->item(row, 5), minimumJacobians[i]);
        adaptiveStateAvailable = adaptiveStateAvailable
            && snapshot.entries.at(row).adaptiveEvent != SolverAdaptiveEvent::Unavailable;
        criterionStateAvailable = criterionStateAvailable
            && snapshot.entries.at(row).residualCriterion != SolverCriterionState::Unavailable
            && snapshot.entries.at(row).displacementCriterion != SolverCriterionState::Unavailable;
    }
    check(loadIncrementMatches, "Delta-lambda values come from authoritative nonlinear history");
    check(residualNormMatches, "Absolute residual norms come from authoritative nonlinear history");
    check(displacementIncrementMatches, "Absolute displacement increments come from authoritative history");
    check(minimumJacobianMatches, "Per-iteration minimum J values come from authoritative history");
    check(adaptiveStateAvailable, "Adaptive event state is derived outside the widget from real step provenance");
    check(criterionStateAvailable, "Residual/displacement criterion states are derived outside the widget");

    const SolverConvergenceEntry &last = snapshot.entries.back();
    check(last.converged
              && last.residualCriterion == SolverCriterionState::Satisfied
              && last.displacementCriterion == SolverCriterionState::Satisfied,
          "Final converged iteration satisfies both configured verification criteria");
    check(diagnosticsSummary->text().contains(QStringLiteral("min J"))
              && diagnosticsSummary->text().contains(QStringLiteral("Mixed u-p: Unavailable"))
              && diagnosticsSummary->text().contains(QStringLiteral("Contact: Unavailable")),
          "Advanced summary exposes min J and does not invent unsupported pressure/contact metrics");

    SolverConvergenceSnapshot unavailableSnapshot;
    unavailableSnapshot.summary.executionMode = SolverExecutionMode::NonlinearNewton;
    unavailableSnapshot.summary.state = SolverConvergenceState::Converged;
    unavailableSnapshot.entries = {
        SolverConvergenceEntry{1, 1, 1.0, 0.0, 0.0, 1.0, true}
    };
    utility->setConvergenceData(unavailableSnapshot);
    app.processEvents();
    check(diagnosticsTable->rowCount() == 0,
          "Basic nonlinear telemetry does not fabricate advanced diagnostic rows");
    check(diagnosticsSummary->text().contains(QStringLiteral("Unavailable")),
          "Unavailable advanced source is explicit in the UI");

    std::cout << "Solver diagnostics acceptance: " << (checks - failures) << '/' << checks
              << (failures == 0 ? " PASS\n" : " FAIL\n");
    return failures == 0 ? 0 : 1;
}

} // namespace d26
