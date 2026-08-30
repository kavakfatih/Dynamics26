#pragma once

#include <QPointF>
#include <QVector>
#include <QWidget>

class MaterialCurveWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit MaterialCurveWidget(QWidget *parent = nullptr);
    void setCurve(const QVector<QPointF> &points, const QString &xLabel, const QString &yLabel);
    void clearCurve();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<QPointF> points_;
    QString xLabel_;
    QString yLabel_;
};
