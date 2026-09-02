#pragma once

// Beta.2 solver / convergence presentation contract.
//
// Bu tipler DOCUMENT STATE değildir. Solver'ın ürettiği türetilmiş çözüm
// telemetry'sini application katmanından Utility Workspace'e tip güvenli olarak
// taşır. Widget bu verinin sahibi veya validator'ı değildir; persistence ve
// document Undo history'sine girmez.
//
// B2.1 bilinçli olarak bugün C ABI'den güvenilir şekilde çıkan ortak subset'i
// tanımlar. Load increment, mixed u-p ve Contact metrikleri gerçek application
// solve consumer'ları bağlandıkça bu contract'a ayrıca eklenecektir; mevcut
// olmayan metrikler sahte 0 değerleriyle temsil edilmez.

#include <QVector>

namespace d26 {

enum class SolverConvergenceState {
    Unavailable = 0,
    Running,
    Converged,
    Failed
};

struct SolverConvergenceEntry {
    int attempt{0};
    int iteration{0};
    double loadFactor{0.0};
    double relativeResidual{0.0};
    double relativeDisplacement{0.0};
    double lineSearchAlpha{1.0};
    bool converged{false};
};

struct SolverConvergenceSummary {
    SolverConvergenceState state{SolverConvergenceState::Unavailable};
    double completedLoadFactor{0.0};
    double finalResidualNorm{0.0};
    int acceptedSteps{0};
    int totalIterations{0};
    int cutbackCount{0};
};

struct SolverConvergenceSnapshot {
    SolverConvergenceSummary summary;
    QVector<SolverConvergenceEntry> entries;

    [[nodiscard]] bool empty() const noexcept { return entries.isEmpty(); }
};

} // namespace d26
