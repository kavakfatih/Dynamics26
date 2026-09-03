#include "ResultDetails.h"

#include "../core/ProjectModel.h"
#include "../services/AnalysisService.h"

#include <QLabel>
#include <QPushButton>

namespace d26 {

ResultDetails::ResultDetails(const ServiceContext &services, QWidget *parent)
    : DetailsPage(parent), services_(services)
{
    auto *definition = addSection(tr("Definition"));
    type_ = definition->addValueRow(tr("Type"));
    scope_ = definition->addValueRow(tr("Scope"), tr("All Bodies"));
    analysis_ = definition->addValueRow(tr("Analysis"));

    contourSection_ = addSection(tr("Results"));
    maximum_ = contourSection_->addValueRow(tr("Maximum"));
    minimum_ = contourSection_->addValueRow(tr("Minimum"));
    unit_ = contourSection_->addValueRow(tr("Unit"));

    reactionSection_ = addSection(tr("Reaction"));
    rx_ = reactionSection_->addValueRow(QStringLiteral("ΣRx"));
    ry_ = reactionSection_->addValueRow(QStringLiteral("ΣRy"));
    rz_ = reactionSection_->addValueRow(QStringLiteral("ΣRz"));

    auto *display = addSection(tr("Display"));
    deformationScale_ = display->addValueRow(tr("Deformation Scale"));
    legend_ = display->addValueRow(tr("Legend"));

    auto *actions = addSection(tr("Actions"));
    auto *csv = makeActionButton(tr("Export CSV…"));
    auto *vtk = makeActionButton(tr("Export VTK…"));
    actions->addFullWidth(csv);
    actions->addFullWidth(vtk);

    auto *advanced = addSection(tr("Advanced"), true, true);
    measure_ = advanced->addValueRow(tr("Result Definition"));
    measure_->setObjectName(QStringLiteral("resultInspector.measure"));
    solveTime_ = advanced->addValueRow(tr("Solve Wall Clock"));
    probe_ = advanced->addValueRow(tr("Corner Probe"));

    addStretch();

    connect(csv, &QPushButton::clicked, this, [this] { emit requestCommand(QStringLiteral("results.exportCsv")); });
    connect(vtk, &QPushButton::clicked, this, [this] { emit requestCommand(QStringLiteral("results.exportVtk")); });
}

void ResultDetails::refresh()
{
    const ObjectType type = services_.project->typeOf(objectId_);
    switch (type) {
    case ObjectType::EquivalentStress: field_ = ResultField::EquivalentStress; break;
    case ObjectType::ReactionForce:    field_ = ResultField::ReactionForce; break;
    default:                           field_ = ResultField::TotalDeformation; break;
    }

    const ObjectId analysisId = services_.analysis->owningAnalysis(objectId_);
    const AnalysisRecord *record = services_.analysis->analysis(analysisId);
    const ProjectObject *analysisObject = services_.project->object(analysisId);
    analysis_->setText(analysisObject != nullptr ? analysisObject->name : tr("—"));
    type_->setText(displayName(type));

    const bool isReaction = field_ == ResultField::ReactionForce;
    contourSection_->setVisible(!isReaction);
    reactionSection_->setVisible(isReaction);

    const bool solved = record != nullptr && record->solved;
    const bool stale = record != nullptr && services_.analysis->solutionIsOutOfDate(analysisId);
    const bool suppressed = services_.project->isSuppressed(objectId_);
    const QString dash = tr("—");
    if (field_ == ResultField::EquivalentStress) {
        measure_->setText(record != nullptr && record->type == AnalysisType::NonlinearStatic
                              ? tr("Final Cauchy von Mises · 8-GP element mean")
                              : tr("Small-strain Cauchy von Mises · element mean"));
    } else if (field_ == ResultField::ReactionForce) {
        measure_->setText(record != nullptr && record->type == AnalysisType::NonlinearStatic
                              ? tr("Constrained DOF equilibrium · R = f_int − λf_ext")
                              : tr("Constrained DOF equilibrium · R = K u − f"));
    } else {
        measure_->setText(tr("Final nodal displacement magnitude"));
    }
    if (!solved) {
        // Sonuç TANIMI vardır fakat hesaplanmış DEĞER yoktur. Sahte sayı
        // gösterilmez; durum açıkça yazılır.
        for (QLabel *label : {maximum_, minimum_, unit_, rx_, ry_, rz_}) {
            label->setText(dash);
        }
        legend_->setText(suppressed ? tr("Bastırıldı") : tr("Çözüm çalıştırılmadı"));
        deformationScale_->setText(dash);
        solveTime_->setText(dash);
        probe_->setText(dash);
        return;
    }
    const SolveResults &results = record->solveResults;

    if (field_ == ResultField::TotalDeformation) {
        maximum_->setText(QStringLiteral("%1 mm").arg(results.maxDisplacementMm, 0, 'g', 8));
        minimum_->setText(QStringLiteral("0 mm"));
        unit_->setText(tr("mm"));
        legend_->setText(tr("0 … %1 mm").arg(results.maxDisplacementMm, 0, 'g', 5));
    } else if (field_ == ResultField::EquivalentStress) {
        maximum_->setText(QStringLiteral("%1 MPa").arg(results.maxVonMisesMPa, 0, 'g', 8));
        minimum_->setText(QStringLiteral("%1 MPa").arg(results.minVonMisesMPa, 0, 'g', 8));
        unit_->setText(tr("MPa"));
        legend_->setText(tr("%1 … %2 MPa")
                             .arg(results.minVonMisesMPa, 0, 'g', 5)
                             .arg(results.maxVonMisesMPa, 0, 'g', 5));
    } else {
        rx_->setText(QStringLiteral("%1 N").arg(results.reactionXN, 0, 'g', 8));
        ry_->setText(QStringLiteral("%1 N").arg(results.reactionYN, 0, 'g', 8));
        rz_->setText(QStringLiteral("%1 N").arg(results.reactionZN, 0, 'g', 8));
        legend_->setText(tr("Kontur gösterilmez"));
    }

    deformationScale_->setText(tr("Otomatik (model açıklığının %12'si)"));
    if (stale) {
        legend_->setText(tr("Girdiler değişti — yeniden çözün"));
    }
    solveTime_->setText(QStringLiteral("%1 s").arg(results.wallClockSeconds, 0, 'f', 3));
    probe_->setText(results.probeNodeId >= 0
                        ? tr("Node %1 · ux = %2 mm").arg(results.probeNodeId).arg(results.probeUxMm, 0, 'g', 6)
                        : tr("—"));
}

} // namespace d26
