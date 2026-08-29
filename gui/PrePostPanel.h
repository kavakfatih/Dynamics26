#pragma once

#include <QWidget>
#include <QJsonObject>
#include <functional>
#include <memory>

#include <femcae/meshing/Assignments.h>
#include <femcae/meshing/ResultDatabase.h>
#include <femcae/meshing/StructuredHexMesher.h>

class QDoubleSpinBox;
class QLabel;
class QSpinBox;

class PrePostPanel final : public QWidget
{
    Q_OBJECT
public:
    explicit PrePostPanel(QWidget *parent = nullptr);
    void setViewportConsumer(std::function<void(const femcae::meshing::SimulationMesh &,
                                                 const femcae::meshing::ResultDatabase &)> consumer);
    void clearProject();
    [[nodiscard]] QJsonObject projectJson() const;
    void loadProjectJson(const QJsonObject &object);

signals:
    void message(const QString &text);
    void solveCompleted(double maxDisplacementMm, double maxVonMisesMPa, double reactionX, qlonglong probeNodeId, double probeUxMm);

private slots:
    void generateMesh();
    void solveLinear();
    void exportCsv();
    void exportVtk();
    void evaluateMidSectionCut();

private:
    void rebuildAssignments();
    bool ensureMesh();

    QDoubleSpinBox *length_ = nullptr;
    QDoubleSpinBox *width_ = nullptr;
    QDoubleSpinBox *height_ = nullptr;
    QSpinBox *nx_ = nullptr;
    QSpinBox *ny_ = nullptr;
    QSpinBox *nz_ = nullptr;
    QDoubleSpinBox *young_ = nullptr;
    QDoubleSpinBox *poisson_ = nullptr;
    QDoubleSpinBox *force_ = nullptr;
    QLabel *meshSummary_ = nullptr;
    QLabel *solveSummary_ = nullptr;
    QLabel *cutSummary_ = nullptr;

    femcae::meshing::SimulationMesh mesh_;
    femcae::meshing::AssignmentStore assignments_;
    femcae::meshing::ResultDatabase results_;
    std::function<void(const femcae::meshing::SimulationMesh &,
                       const femcae::meshing::ResultDatabase &)> viewportConsumer_;
};
