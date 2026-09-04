#include "ResultDetails.h"

#include "../core/ProjectModel.h"
#include "../services/AnalysisService.h"

#include <QLabel>
#include <QPushButton>

#include <cmath>

namespace d26 {
namespace {

using femcae::meshing::ResultAssociation;
using femcae::meshing::ResultConfiguration;
using femcae::meshing::ResultFieldMetadata;
using femcae::meshing::ResultMeasure;
using femcae::meshing::ResultPhysicalQuantity;
using femcae::meshing::ResultRecoveryMethod;
using femcae::meshing::ResultSourceLocation;
using femcae::meshing::ResultUnit;

QString quantityName(const ResultPhysicalQuantity value)
{
    switch (value) {
    case ResultPhysicalQuantity::Displacement: return QObject::tr("Displacement");
    case ResultPhysicalQuantity::Stress: return QObject::tr("Stress");
    case ResultPhysicalQuantity::ReactionForce: return QObject::tr("Reaction Force");
    case ResultPhysicalQuantity::Unknown: break;
    }
    return QObject::tr("Unavailable");
}

QString associationName(const ResultAssociation value)
{
    switch (value) {
    case ResultAssociation::Node: return QObject::tr("Node");
    case ResultAssociation::Element: return QObject::tr("Element");
    case ResultAssociation::Unknown: break;
    }
    return QObject::tr("Unavailable");
}

QString sourceLocationName(const ResultFieldMetadata &metadata)
{
    switch (metadata.sourceLocation) {
    case ResultSourceLocation::MeshNode: return QObject::tr("Mesh Nodes");
    case ResultSourceLocation::IntegrationPoints:
        return QObject::tr("%1 Integration Points").arg(metadata.integrationPointCount);
    case ResultSourceLocation::ConstrainedDegreesOfFreedom:
        return QObject::tr("Constrained DOFs");
    case ResultSourceLocation::Unknown: break;
    }
    return QObject::tr("Unavailable");
}

QString recoveryName(const ResultRecoveryMethod value)
{
    switch (value) {
    case ResultRecoveryMethod::Direct: return QObject::tr("Direct Field Value");
    case ResultRecoveryMethod::ArithmeticMean: return QObject::tr("Arithmetic Mean");
    case ResultRecoveryMethod::EquilibriumRecovery: return QObject::tr("Equilibrium Recovery");
    case ResultRecoveryMethod::Unknown: break;
    }
    return QObject::tr("Unavailable");
}

QString unitName(const ResultUnit value)
{
    switch (value) {
    case ResultUnit::Meter: return QStringLiteral("m");
    case ResultUnit::Millimeter: return QStringLiteral("mm");
    case ResultUnit::Pascal: return QStringLiteral("Pa");
    case ResultUnit::MegaPascal: return QStringLiteral("MPa");
    case ResultUnit::Newton: return QStringLiteral("N");
    case ResultUnit::Unitless: break;
    }
    return QObject::tr("Unitless");
}

QString configurationName(const ResultConfiguration value)
{
    switch (value) {
    case ResultConfiguration::Reference: return QObject::tr("Reference");
    case ResultConfiguration::FinalConverged: return QObject::tr("Final Converged");
    case ResultConfiguration::Unknown: break;
    }
    return QObject::tr("Unavailable");
}

QString measureName(const ResultMeasure value)
{
    switch (value) {
    case ResultMeasure::Magnitude: return QObject::tr("Magnitude");
    case ResultMeasure::CauchyVonMises: return QObject::tr("Cauchy von Mises");
    case ResultMeasure::Vector: return QObject::tr("Vector");
    case ResultMeasure::Unknown: break;
    }
    return QObject::tr("Unavailable");
}

const ResultFieldMetadata *metadataFor(const ResultField field,
                                       const femcae::meshing::ResultDatabase *database)
{
    if (database == nullptr) {
        return nullptr;
    }
    if (field == ResultField::TotalDeformation) {
        const auto *value = database->displacement();
        return value != nullptr ? &value->metadata : nullptr;
    }
    if (field == ResultField::EquivalentStress) {
        const auto *value = database->elementScalar("von_mises");
        return value != nullptr ? &value->metadata : nullptr;
    }
    const auto *value = database->reaction();
    return value != nullptr ? &value->metadata : nullptr;
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
    configuration_ = advanced->addValueRow(tr("Configuration"));
    configuration_->setObjectName(QStringLiteral("resultInspector.configuration"));
    solveTime_ = advanced->addValueRow(tr("Solve Wall Clock"));

    auto *probeSection = addSection(tr("Probe"));
    probeMethod_ = probeSection->addValueRow(tr("Method"));
    probeMethod_->setObjectName(QStringLiteral("resultInspector.probeMethod"));
    probe_ = probeSection->addValueRow(tr("Value"));
    probe_->setObjectName(QStringLiteral("resultInspector.probe"));

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
    const auto *database = solved ? services_.analysis->resultDatabase(analysisId) : nullptr;
    const ResultFieldMetadata *metadata = metadataFor(field_, database);
    for (QLabel *label : {physicalQuantity_, measure_, association_, sourceLocation_,
                          recoveryMethod_, storageUnit_, displayUnit_, configuration_}) {
        label->setText(dash);
    }
    if (metadata != nullptr) {
        physicalQuantity_->setText(quantityName(metadata->quantity));
        measure_->setText(measureName(metadata->measure));
        association_->setText(associationName(metadata->association));
        sourceLocation_->setText(sourceLocationName(*metadata));
        recoveryMethod_->setText(recoveryName(metadata->recovery));
        storageUnit_->setText(unitName(metadata->storageUnit));
        displayUnit_->setText(unitName(metadata->displayUnit));
        configuration_->setText(configurationName(metadata->configuration));
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
        probeMethod_->setText(dash);
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
    if (field_ == ResultField::ReactionForce) {
        probeMethod_->setText(tr("Not applicable"));
        probe_->setText(dash);
    } else {
        clearProbe();
    }
}

void ResultDetails::clearProbe()
{
    if (field_ == ResultField::TotalDeformation) {
        probeMethod_->setText(tr("Nearest FEM Node"));
        probe_->setText(tr("Click viewport to probe"));
    } else if (field_ == ResultField::EquivalentStress) {
        probeMethod_->setText(tr("Boundary Facet → Owner Element"));
        probe_->setText(tr("Click viewport to probe"));
    } else {
        probeMethod_->setText(tr("Not applicable"));
        probe_->setText(tr("—"));
    }
}

void ResultDetails::showDisplacementProbe(const qint64 nodeId, const double uxMm,
                                          const double uyMm, const double uzMm)
{
    const double magnitudeMm = std::sqrt(uxMm * uxMm + uyMm * uyMm + uzMm * uzMm);
    probeMethod_->setText(tr("Nearest FEM Node"));
    probe_->setText(tr("Node %1 · U=(%2, %3, %4) mm · |U|=%5 mm")
                        .arg(nodeId)
                        .arg(uxMm, 0, 'g', 7)
                        .arg(uyMm, 0, 'g', 7)
                        .arg(uzMm, 0, 'g', 7)
                        .arg(magnitudeMm, 0, 'g', 7));
}

void ResultDetails::showEquivalentStressProbe(const qint64 elementId,
                                              const double vonMisesMPa)
{
    probeMethod_->setText(tr("Boundary Facet → Owner Element"));
    probe_->setText(tr("Element %1 · Cauchy von Mises · 8-GP Mean = %2 MPa")
                        .arg(elementId)
                        .arg(vonMisesMPa, 0, 'g', 8));
}

} // namespace d26
