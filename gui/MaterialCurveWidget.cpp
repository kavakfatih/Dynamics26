#include "MaterialCurveWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QPalette>

MaterialCurveWidget::MaterialCurveWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(190);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void MaterialCurveWidget::setCurve(const QVector<QPointF> &points, const QString &xLabel, const QString &yLabel)
{
    points_ = points;
    xLabel_ = xLabel;
    yLabel_ = yLabel;
    update();
}

void MaterialCurveWidget::clearCurve()
{
    points_.clear();
    update();
}

void MaterialCurveWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), palette().base());

    const QRectF plot = rect().adjusted(48.0, 18.0, -18.0, -38.0);
    painter.setPen(QPen(palette().mid().color(), 1.0));
    painter.drawRect(plot);
    painter.drawText(QRectF(plot.left(), plot.bottom() + 8.0, plot.width(), 24.0), Qt::AlignCenter, xLabel_);
    painter.save();
    painter.translate(16.0, plot.center().y());
    painter.rotate(-90.0);
    painter.drawText(QRectF(-plot.height()/2.0, -12.0, plot.height(), 24.0), Qt::AlignCenter, yLabel_);
    painter.restore();

    if (points_.size() < 2) {
        painter.setPen(palette().text().color());
        painter.drawText(plot, Qt::AlignCenter, tr("Malzeme eğrisi önizlemesi"));
        return;
    }

    double xmin = points_.front().x(), xmax = xmin, ymin = points_.front().y(), ymax = ymin;
    for (const auto &p : points_) {
        xmin = qMin(xmin, p.x()); xmax = qMax(xmax, p.x());
        ymin = qMin(ymin, p.y()); ymax = qMax(ymax, p.y());
    }
    if (qFuzzyCompare(xmin, xmax)) xmax = xmin + 1.0;
    if (qFuzzyCompare(ymin, ymax)) { ymin -= 1.0; ymax += 1.0; }
    const double ypad = 0.08 * (ymax - ymin);
    ymin -= ypad; ymax += ypad;

    auto mapPoint = [&](const QPointF &p) {
        const double x = plot.left() + (p.x() - xmin) / (xmax - xmin) * plot.width();
        const double y = plot.bottom() - (p.y() - ymin) / (ymax - ymin) * plot.height();
        return QPointF(x, y);
    };

    QPainterPath path;
    path.moveTo(mapPoint(points_.front()));
    for (int i = 1; i < points_.size(); ++i) path.lineTo(mapPoint(points_[i]));
    QPen curvePen(palette().highlight().color(), 2.0);
    curvePen.setCapStyle(Qt::RoundCap);
    curvePen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(curvePen);
    painter.drawPath(path);

    painter.setPen(palette().text().color());
    painter.drawText(QRectF(plot.left(), 0.0, plot.width(), 18.0), Qt::AlignLeft,
                     QString::number(ymax, 'g', 5) + "  " + yLabel_);
}
