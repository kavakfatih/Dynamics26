#include "ConvergencePlotWidget.h"

#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPolygonF>

#include <algorithm>
#include <cmath>
#include <limits>

namespace d26 {
namespace {

constexpr double MinimumPositiveMetric = 1.0e-300;

QColor withAlpha(QColor color, const int alpha)
{
    color.setAlpha(alpha);
    return color;
}

} // namespace

ConvergencePlotWidget::ConvergencePlotWidget(const Metric metric, QWidget *parent)
    : QWidget(parent), metric_(metric)
{
    setMinimumHeight(132);
    setFocusPolicy(Qt::StrongFocus);
    setAccessibleName(metric == Metric::RelativeResidual
                          ? tr("Relative residual convergence plot")
                          : tr("Relative displacement correction convergence plot"));
    setAccessibleDescription(
        tr("Logarithmic convergence history with tolerance, cutback, retry and accepted-step markers."));
}

void ConvergencePlotWidget::setSnapshot(const SolverConvergenceSnapshot &snapshot,
                                        const std::optional<double> tolerance)
{
    snapshot_ = snapshot;
    tolerance_ = tolerance;
    update();
}

QSize ConvergencePlotWidget::minimumSizeHint() const
{
    return {260, 132};
}

double ConvergencePlotWidget::metricValue(const SolverConvergenceEntry &entry) const noexcept
{
    return metric_ == Metric::RelativeResidual
        ? entry.relativeResidual : entry.relativeDisplacement;
}

QString ConvergencePlotWidget::titleText() const
{
    return metric_ == Metric::RelativeResidual
        ? tr("CONVERGENCE — log10(Relative Residual)")
        : tr("CONVERGENCE — log10(Relative Δu)");
}

void ConvergencePlotWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), palette().color(QPalette::Base));

    const QRectF plot = QRectF(rect()).adjusted(48.0, 25.0, -12.0, -25.0);
    const QColor text = palette().color(QPalette::Text);
    const QColor grid = withAlpha(text, 45);
    const QColor curve = palette().color(QPalette::Highlight);
    painter.setPen(text);
    painter.drawText(QRectF(8.0, 4.0, width() - 16.0, 18.0), titleText());

    QVector<double> values;
    values.reserve(snapshot_.entries.size());
    double yMinimum = std::numeric_limits<double>::infinity();
    double yMaximum = -std::numeric_limits<double>::infinity();
    for (const SolverConvergenceEntry &entry : snapshot_.entries) {
        const double raw = metricValue(entry);
        const double value = std::isfinite(raw) && raw > 0.0
            ? std::log10(std::max(raw, MinimumPositiveMetric))
            : std::numeric_limits<double>::quiet_NaN();
        values.push_back(value);
        if (std::isfinite(value)) {
            yMinimum = std::min(yMinimum, value);
            yMaximum = std::max(yMaximum, value);
        }
    }
    std::optional<double> toleranceLog;
    if (tolerance_.has_value() && std::isfinite(*tolerance_) && *tolerance_ > 0.0) {
        toleranceLog = std::log10(*tolerance_);
        yMinimum = std::min(yMinimum, *toleranceLog);
        yMaximum = std::max(yMaximum, *toleranceLog);
    }

    painter.setPen(QPen(grid, 1.0));
    painter.drawRect(plot);
    if (values.isEmpty() || !std::isfinite(yMinimum) || !std::isfinite(yMaximum)) {
        painter.setPen(withAlpha(text, 140));
        painter.drawText(plot, Qt::AlignCenter, tr("No convergence samples"));
        return;
    }
    if (std::abs(yMaximum - yMinimum) < 1.0e-12) {
        yMinimum -= 1.0;
        yMaximum += 1.0;
    } else {
        const double padding = 0.08 * (yMaximum - yMinimum);
        yMinimum -= padding;
        yMaximum += padding;
    }

    const int valueCount = static_cast<int>(values.size());
    const auto pointFor = [&](const int index, const double value) {
        const double denominator = valueCount > 1
            ? static_cast<double>(valueCount - 1) : 1.0;
        const double x = plot.left() + plot.width() * static_cast<double>(index) / denominator;
        const double y = plot.bottom() - plot.height() * (value - yMinimum) / (yMaximum - yMinimum);
        return QPointF(x, y);
    };

    if (toleranceLog.has_value()) {
        const double y = pointFor(0, *toleranceLog).y();
        painter.setPen(QPen(QColor(214, 140, 24), 1.0, Qt::DashLine));
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        painter.drawText(QRectF(plot.left() + 4.0, y - 17.0, 120.0, 16.0),
                         tr("tolerance %1").arg(*tolerance_, 0, 'g', 3));
    }

    QPainterPath path;
    bool pathStarted = false;
    int previousAttempt = -1;
    for (int i = 0; i < valueCount; ++i) {
        if (!std::isfinite(values.at(i))) {
            pathStarted = false;
            continue;
        }
        const QPointF point = pointFor(i, values.at(i));
        if (!pathStarted) {
            path.moveTo(point);
            pathStarted = true;
        } else {
            path.lineTo(point);
        }

        const SolverConvergenceEntry &entry = snapshot_.entries.at(i);
        if (previousAttempt >= 0 && entry.attempt != previousAttempt) {
            painter.setPen(QPen(withAlpha(text, 70), 1.0, Qt::DashLine));
            painter.drawLine(QPointF(point.x(), plot.top()), QPointF(point.x(), plot.bottom()));
        }
        if (entry.adaptiveEvent == SolverAdaptiveEvent::Cutback) {
            painter.setBrush(QColor(210, 63, 63));
            painter.setPen(Qt::NoPen);
            painter.drawPolygon(QPolygonF({point + QPointF(0.0, -5.0),
                                            point + QPointF(5.0, 0.0),
                                            point + QPointF(0.0, 5.0),
                                            point + QPointF(-5.0, 0.0)}));
        } else if (entry.adaptiveEvent == SolverAdaptiveEvent::Retry) {
            painter.setBrush(QColor(214, 140, 24));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(point, 3.5, 3.5);
        }
        if (entry.converged) {
            painter.setPen(QPen(QColor(55, 158, 90), 1.0));
            painter.drawLine(QPointF(point.x(), plot.top()), QPointF(point.x(), plot.bottom()));
        }
        previousAttempt = entry.attempt;
    }
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(curve, 1.8));
    painter.drawPath(path);

    painter.setPen(withAlpha(text, 170));
    painter.drawText(QRectF(2.0, plot.top() - 8.0, 42.0, 16.0),
                     Qt::AlignRight, QString::number(yMaximum, 'g', 3));
    painter.drawText(QRectF(2.0, plot.bottom() - 8.0, 42.0, 16.0),
                     Qt::AlignRight, QString::number(yMinimum, 'g', 3));
    painter.drawText(QRectF(plot.left(), plot.bottom() + 5.0, plot.width(), 18.0),
                     Qt::AlignCenter, tr("Cumulative Newton iteration"));
}

} // namespace d26
