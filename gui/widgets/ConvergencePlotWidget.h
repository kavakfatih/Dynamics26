#pragma once

#include "../core/SolverTelemetry.h"

#include <QWidget>

#include <optional>

namespace d26 {

// Ağır plotting dependency'si eklemeden, typed solver telemetry'sinin iki temel
// convergence metriğini deterministik olarak çizer. Widget solver state sahibi
// değildir; snapshot kopyası yalnız transient presentation state'idir.
class ConvergencePlotWidget final : public QWidget
{
    Q_OBJECT

public:
    enum class Metric { RelativeResidual, RelativeDisplacementCorrection };

    explicit ConvergencePlotWidget(Metric metric, QWidget *parent = nullptr);

    void setSnapshot(const SolverConvergenceSnapshot &snapshot,
                     std::optional<double> tolerance);
    [[nodiscard]] int sampleCount() const noexcept
    {
        return static_cast<int>(snapshot_.entries.size());
    }
    [[nodiscard]] std::optional<double> tolerance() const noexcept { return tolerance_; }

    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    [[nodiscard]] double metricValue(const SolverConvergenceEntry &entry) const noexcept;
    [[nodiscard]] QString titleText() const;

    Metric metric_;
    SolverConvergenceSnapshot snapshot_;
    std::optional<double> tolerance_;
};

} // namespace d26
