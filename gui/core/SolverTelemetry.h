#pragma once

// Beta.2 solver / convergence presentation contract.
//
// Bu tipler DOCUMENT STATE değildir. Solver'ın ürettiği türetilmiş çözüm
// telemetry'sini application katmanından Utility Workspace'e tip güvenli olarak
// taşır. Widget bu verinin sahibi veya validator'ı değildir; persistence ve
// document Undo history'sine girmez.
//
// B2.4 execution mode ayrımı kritik bir semantik sınırdır: doğrudan lineer solve
// Newton geçmişi üretmez. Bu yüzden Newton iteration/cutback/load-factor gibi
// unavailable metrikler 0 ile doldurulup kullanıcıya gerçek ölçüm gibi sunulmaz.
//
// B2.5 advanced diagnostics alanlari std::optional ile temsil edilir. Solver veya
// application boundary bir metriği gerçekten üretmiyorsa 0.0 mühendislik değeri
// uydurmak yerine std::nullopt kullanılır.

#include <QVector>

#include <algorithm>
#include <cmath>
#include <optional>

namespace d26 {

enum class SolverExecutionMode {
    Unavailable = 0,
    DirectLinear,
    NonlinearNewton
};

enum class SolverConvergenceState {
    Unavailable = 0,
    Running,
    Completed,
    Converged,
    Failed
};

enum class SolverAdaptiveEvent {
    Unavailable = 0,
    None,
    Growth,
    Cutback
};

enum class SolverCriterionState {
    Unavailable = 0,
    Satisfied,
    Unsatisfied
};

enum class SolverMetricAvailability {
    Unavailable = 0,
    Available
};

struct SolverConvergenceEntry {
    int attempt{0};
    int iteration{0};
    double loadFactor{0.0};
    double relativeResidual{0.0};
    double relativeDisplacement{0.0};
    double lineSearchAlpha{1.0};
    bool converged{false};

    // B2.5 authoritative advanced subset. Eski B2.1/B2.2 aggregate
    // initializers ilk yedi alanla geriye uyumlu kalır.
    std::optional<int> acceptedStepBefore;
    std::optional<double> loadIncrement;
    std::optional<double> residualNorm;
    std::optional<double> displacementIncrementNorm;
    std::optional<double> minimumJacobian;
    SolverAdaptiveEvent adaptiveEvent{SolverAdaptiveEvent::Unavailable};
    SolverCriterionState residualCriterion{SolverCriterionState::Unavailable};
    SolverCriterionState displacementCriterion{SolverCriterionState::Unavailable};
};

struct SolverConvergenceSummary {
    SolverExecutionMode executionMode{SolverExecutionMode::Unavailable};
    SolverConvergenceState state{SolverConvergenceState::Unavailable};
    double completedLoadFactor{0.0};
    double finalResidualNorm{0.0};
    int acceptedSteps{0};
    int totalIterations{0};
    int cutbackCount{0};

    std::optional<double> minimumJacobian;
    SolverMetricAvailability pressureMetrics{SolverMetricAvailability::Unavailable};
    SolverMetricAvailability contactMetrics{SolverMetricAvailability::Unavailable};
};

struct SolverConvergenceSnapshot {
    SolverConvergenceSummary summary;
    QVector<SolverConvergenceEntry> entries;

    [[nodiscard]] bool empty() const noexcept { return entries.isEmpty(); }
};

// Widget convergence kriterlerini veya adaptive-step olaylarini tahmin etmez.
// Bu helper typed telemetry katmaninda, producer tarafinda çağrılmak içindir.
// Toleranslar solver session'in GERÇEK ayarları olmalıdır.
inline void finalizeNonlinearDiagnostics(SolverConvergenceSnapshot &snapshot,
                                         const double residualRelativeTolerance,
                                         const double displacementRelativeTolerance)
{
    if (snapshot.summary.executionMode != SolverExecutionMode::NonlinearNewton) {
        return;
    }

    int previousAttempt = -1;
    std::optional<int> previousAcceptedStepBefore;
    std::optional<double> previousLoadIncrement;

    for (SolverConvergenceEntry &entry : snapshot.entries) {
        if (std::isfinite(entry.relativeResidual) && residualRelativeTolerance >= 0.0) {
            entry.residualCriterion = entry.relativeResidual <= residualRelativeTolerance
                ? SolverCriterionState::Satisfied
                : SolverCriterionState::Unsatisfied;
        }
        if (std::isfinite(entry.relativeDisplacement) && displacementRelativeTolerance >= 0.0) {
            entry.displacementCriterion = entry.relativeDisplacement <= displacementRelativeTolerance
                ? SolverCriterionState::Satisfied
                : SolverCriterionState::Unsatisfied;
        }

        if (!entry.acceptedStepBefore.has_value() || !entry.loadIncrement.has_value()) {
            entry.adaptiveEvent = SolverAdaptiveEvent::Unavailable;
            continue;
        }

        if (entry.attempt == previousAttempt) {
            entry.adaptiveEvent = SolverAdaptiveEvent::None;
            continue;
        }

        entry.adaptiveEvent = SolverAdaptiveEvent::None;
        if (previousLoadIncrement.has_value() && previousAcceptedStepBefore.has_value()) {
            const double oldIncrement = *previousLoadIncrement;
            const double newIncrement = *entry.loadIncrement;
            const double scale = std::max({std::abs(oldIncrement), std::abs(newIncrement), 1.0e-14});
            const double difference = newIncrement - oldIncrement;
            const double epsilon = 1.0e-10 * scale;

            if (*entry.acceptedStepBefore == *previousAcceptedStepBefore && difference < -epsilon) {
                entry.adaptiveEvent = SolverAdaptiveEvent::Cutback;
            } else if (*entry.acceptedStepBefore > *previousAcceptedStepBefore && difference > epsilon) {
                entry.adaptiveEvent = SolverAdaptiveEvent::Growth;
            }
        }

        previousAttempt = entry.attempt;
        previousAcceptedStepBefore = entry.acceptedStepBefore;
        previousLoadIncrement = entry.loadIncrement;
    }
}

} // namespace d26
