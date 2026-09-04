#include "ResultDetails.h"

#include "../core/ProjectModel.h"
#include "../services/AnalysisService.h"

#include <QLabel>
#include <QPushButton>

namespace d26 {
namespace {

QString associationText(const femcae::meshing::ResultAssociation association)
{
    using femcae::meshing::ResultAssociation;
    switch (association) {
    case ResultAssociation::Node:    return QStringLiteral("Node");
    case ResultAssociation::Element: return QStringLiteral("Element");
    case ResultAssociation::Unknown: return QStringLiteral("Unknown");
    }
    return QStringLiteral("Unknown");
}

QString semanticText(const std::string &value)
{
    return value.empty() ? QStringLiteral("—") : QString::fromStdString(value);
}

} // namespace

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

    auto *advanced = addSection(tr("Field Semantics"), true, true);
    physicalQuantity_ = advanced->addValueRow(tr("Physical Quantity"));
    physicalQuantity_->setObjectName(QStringLiteral("resultInspector.physicalQuantity"));
    measure_ = advanced->addValueRow(tr("Measure"));
    measure_->setObjectName(QStringLiteral("resultInspector.measure"));
    association_ = advanced->addValueRow(tr("Association"));
    association_->setObjectName(QStringLiteral("resultInspector.association"));
    sourceLocation_ = advanced->addValueRow(tr("Source Location"));
    sourceLocation_->setObjectName(QStringLiteral("resultInspector.sourceLocation"));
    recoveryMethod_ = advanced->addValueRow(tr("Recovery Method"));
    recoveryMethod_->setObjectName(QStringLiteral("resultInspector.recoveryMethod"));
    storageUnit_ = advanced->addValueRow(tr("Storage Unit"));
    storageUnit_->setObjectName(QStringLiteral("resultInspector.storageUnit"));
    displayUnit_ = advanced->addValueRow(tr("Display Unit"));
    displayUnit_->setObjectName(QStringLiteral("resultInspector.displayUnit"));
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
    for (QLabel *label : {physicalQuantity_, measure_, association_, sourceLocation_,
                          recoveryMethod_, storageUnit_, displayUnit_}) {
        label->setText(dash);
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
    const femcae::meshing::ResultDatabase *database =
        services_.analysis->resultDatabase(analysisId);

    const femcae::meshing::ResultFieldMetadata *metadata = nullptr;
    if (field_ == ResultField::TotalDeformation && database != nullptr
        && database->displacement() != nullptr) {
        metadata = &database->displacement()->metadata;
    } else if (field_ == ResultField::EquivalentStress && database != nullptr
               && database->elementScalar("von_mises") != nullptr) {
        metadata = &database->elementScalar("von_mises")->metadata;
    }

    if (metadata != nullptr) {
        physicalQuantity_->setText(semanticText(metadata->physicalQuantity));
        measure_->setText(semanticText(metadata->measure));
        association_->setText(associationText(metadata->association));
        sourceLocation_->setText(semanticText(metadata->sourceLocation));
        recoveryMethod_->setText(semanticText(metadata->recoveryMethod));
        storageUnit_->setText(semanticText(metadata->storageUnit));
        displayUnit_->setText(semanticText(metadata->displayUnit));
    } else if (field_ == ResultField::ReactionForce) {
        // RC1.6 nodal reaction field'i eklenene kadar bu sonuç yalnız gerçek
        // constrained-DOF resultant'ını taşır; stored nodal field varmış gibi
        // sunulmaz.
        physicalQuantity_->setText(tr("Reaction Force"));
        measure_->setText(record->type == AnalysisType::NonlinearStatic
                              ? tr("Constrained DOF equilibrium · R = f_int − λf_ext")
                              : tr("Constrained DOF equilibrium · R = K u − f"));
        association_->setText(tr("Resultant (stored nodal field unavailable)"));
        sourceLocation_->setText(tr("Constrained displacement DOFs"));
        recoveryMethod_->setText(tr("Vector sum of constrained DOF reactions"));
        storageUnit_->setText(tr("N"));
        displayUnit_->setText(tr("N"));
    }

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
