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

    // Alpha.1 viewport bağlamı: preprocessing ekranları result contour'u
    // taşımamalıdır. Stored result database korunur; yalnız consumer'a geçici
    // boş result gönderilerek nötr FEM mesh görünümü istenir.
    [[nodiscard]] bool showMeshPreview()
    {
        if (!viewportConsumer_ || mesh_.elements.empty()) {
            return false;
        }
        const femcae::meshing::ResultDatabase neutralResults;
        viewportConsumer_(mesh_, neutralResults);
        return true;
    }

    // Sonuçlar bağlamında ise aynı mesh ve saklanan gerçek sonuçlar tekrar
    // görüntülenir. Böylece Navigator seçimi yalnız presentation state'i değiştirir;
    // mesh/result verisi kopyalanmaz veya silinmez.
    [[nodiscard]] bool showResultsPreview()
    {
        if (!viewportConsumer_ || mesh_.elements.empty()) {
            return false;
        }
        const bool hasResults = results_.displacement() != nullptr
            || results_.elementScalar("von_mises") != nullptr;
        if (!hasResults) {
            return false;
        }
        viewportConsumer_(mesh_, results_);
        return true;
    }

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
