#include "MeshDetails.h"

#include "../commands/DomainCommands.h"
#include "../core/DocumentCommandManager.h"
#include "../core/ScopeReferenceBuilder.h"
#include "../services/AnalysisService.h"
#include "../services/ContactService.h"
#include "../services/GeometryService.h"
#include "../services/MeshService.h"
#include "../services/NamedSelectionService.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

namespace d26 {
namespace {

bool isStaleMeshScope(const ScopeReference &scope, const MeshService *mesh)
{
    if (mesh == nullptr || scope.entities.isEmpty()
        || scope.entities.front().domain != SelectionDomain::Mesh) {
        return false;
    }
    return validateMeshScopeReference(scope, mesh->mesh(), mesh->generation())
        == ScopeReferenceValidationError::StaleMeshGeneration;
}

} // namespace

MeshDetails::MeshDetails(const ServiceContext &services, QWidget *parent)
    : DetailsPage(parent), services_(services)
{
    auto *definition = addSection(tr("Definition"));
    method_ = definition->addValueRow(tr("Method"), tr("Structured HEX8"));
    method_->setObjectName(QStringLiteral("Dynamics26MeshMethod"));
    elementType_ = definition->addValueRow(tr("Element Type"), tr("HEX8"));
    elementType_->setObjectName(QStringLiteral("Dynamics26MeshElementType"));
    source_ = makeCombo({tr("Parametric Box"), tr("From Geometry")});
    source_->setObjectName(QStringLiteral("Dynamics26MeshSource"));
    definition->addRow(tr("Source"), source_);

    auto *sizing = addSection(tr("Sizing"));
    length_ = makeDoubleField(0.001, 1.0e5, 2, tr(" mm"));
    width_ = makeDoubleField(0.001, 1.0e5, 2, tr(" mm"));
    height_ = makeDoubleField(0.001, 1.0e5, 2, tr(" mm"));
    length_->setObjectName(QStringLiteral("Dynamics26MeshLength"));
    width_->setObjectName(QStringLiteral("Dynamics26MeshWidth"));
    height_->setObjectName(QStringLiteral("Dynamics26MeshHeight"));
    sizing->addRow(tr("Length"), length_);
    sizing->addRow(tr("Width"), width_);
    sizing->addRow(tr("Height"), height_);

    auto *divisions = addSection(tr("Divisions"));
    nx_ = makeIntField(1, 200);
    ny_ = makeIntField(1, 200);
    nz_ = makeIntField(1, 200);
    nx_->setObjectName(QStringLiteral("Dynamics26MeshNx"));
    ny_->setObjectName(QStringLiteral("Dynamics26MeshNy"));
    nz_->setObjectName(QStringLiteral("Dynamics26MeshNz"));
    divisions->addRow(QStringLiteral("Nx"), nx_);
    divisions->addRow(QStringLiteral("Ny"), ny_);
    divisions->addRow(QStringLiteral("Nz"), nz_);

    auto *lifecycle = addSection(tr("Lifecycle"));
    status_ = lifecycle->addValueRow(tr("State"));
    generation_ = lifecycle->addValueRow(tr("Mesh Generation"));
    settingsRevision_ = lifecycle->addValueRow(tr("Settings Revision"));
    sourceGeometryRevision_ = lifecycle->addValueRow(tr("Current Geometry Revision"));
    meshedGeometryRevision_ = lifecycle->addValueRow(tr("Meshed Geometry Revision"));
    staleScopes_ = lifecycle->addValueRow(tr("Stale FEM Scopes"));
    status_->setObjectName(QStringLiteral("Dynamics26MeshStatus"));
    generation_->setObjectName(QStringLiteral("Dynamics26MeshGeneration"));
    settingsRevision_->setObjectName(QStringLiteral("Dynamics26MeshSettingsRevision"));
    sourceGeometryRevision_->setObjectName(QStringLiteral("Dynamics26MeshSourceGeometryRevision"));
    meshedGeometryRevision_->setObjectName(QStringLiteral("Dynamics26MeshMeshedGeometryRevision"));
    staleScopes_->setObjectName(QStringLiteral("Dynamics26MeshStaleScopes"));
    lifecycle->addNote(tr("FEM scope kimlikleri mesh generation'a bağlıdır. Regenerate, Clear veya Reset sonrası "
                          "eski Mesh/Node/Element/Facet kapsamları yeni ID'lere otomatik bağlanmaz."));

    auto *statistics = addSection(tr("Statistics"));
    nodes_ = statistics->addValueRow(tr("Nodes"));
    elements_ = statistics->addValueRow(tr("Elements"));
    facets_ = statistics->addValueRow(tr("Boundary Facets"));
    dof_ = statistics->addValueRow(tr("Total DOF"));
    nodes_->setObjectName(QStringLiteral("Dynamics26MeshNodes"));
    elements_->setObjectName(QStringLiteral("Dynamics26MeshElements"));
    facets_->setObjectName(QStringLiteral("Dynamics26MeshFacets"));
    dof_->setObjectName(QStringLiteral("Dynamics26MeshDof"));

    auto *quality = addSection(tr("Quality"));
    scaledJacobian_ = quality->addValueRow(tr("Min Scaled Jacobian"));
    aspectRatio_ = quality->addValueRow(tr("Max Aspect Ratio"));
    inverted_ = quality->addValueRow(tr("Inverted Elements"));
    scaledJacobian_->setObjectName(QStringLiteral("Dynamics26MeshScaledJacobian"));
    aspectRatio_->setObjectName(QStringLiteral("Dynamics26MeshAspectRatio"));
    inverted_->setObjectName(QStringLiteral("Dynamics26MeshInverted"));

    auto *actions = addSection(tr("Actions"));
    generate_ = makeActionButton(tr("Generate Mesh"));
    generate_->setObjectName(QStringLiteral("Dynamics26MeshGenerate"));
    clearGenerated_ = makeActionButton(tr("Clear Generated Mesh"));
    clearGenerated_->setObjectName(QStringLiteral("Dynamics26MeshClearGenerated"));
    actions->addFullWidth(generate_);
    actions->addFullWidth(clearGenerated_);

    auto *advanced = addSection(tr("Advanced"), true, true);
    predicted_ = advanced->addValueRow(tr("Predicted Size"));
    solverLimit_ = advanced->addValueRow(tr("Solver Limit"));
    predicted_->setObjectName(QStringLiteral("Dynamics26MeshPredicted"));
    solverLimit_->setObjectName(QStringLiteral("Dynamics26MeshSolverLimit"));
    advanced->addNote(tr("Structured HEX8 baseline yalnız eksen hizalı kutu gövdeyi mesher. "
                         "Keyfi STEP hacim meshleme henüz üretim seviyesinde değildir; "
                         "bu nedenle kutu dışı gövdeler için parametrik kutu kullanılır."));

    addStretch();

    connect(generate_, &QPushButton::clicked, this,
            [this] { emit requestCommand(QStringLiteral("mesh.generate")); });
    connect(clearGenerated_, &QPushButton::clicked, this,
            [this] { emit requestCommand(QStringLiteral("mesh.clearGenerated")); });
    const auto push = [this] { pushDefinition(); };
    connect(length_, &QDoubleSpinBox::valueChanged, this, push);
    connect(width_, &QDoubleSpinBox::valueChanged, this, push);
    connect(height_, &QDoubleSpinBox::valueChanged, this, push);
    connect(nx_, &QSpinBox::valueChanged, this, push);
    connect(ny_, &QSpinBox::valueChanged, this, push);
    connect(nz_, &QSpinBox::valueChanged, this, push);
    connect(source_, &QComboBox::currentIndexChanged, this, [this](const int index) {
        if (updating_) {
            return;
        }
        MeshService::Definition after = services_.mesh->definition();
        after.source = index == 1 ? MeshSource::GeometryBoundingBox : MeshSource::ParametricBox;
        pushMeshCommand(after, tr("Change Mesh Source"));
    });
}

void MeshDetails::pushDefinition()
{
    if (updating_) {
        return;
    }
    const MeshService::Definition before = services_.mesh->definition();
    MeshService::Definition after = before;
    if (!services_.mesh->dimensionsAreDerived()) {
        after.lengthMm = length_->value();
        after.widthMm = width_->value();
        after.heightMm = height_->value();
    }
    after.nx = nx_->value();
    after.ny = ny_->value();
    after.nz = nz_->value();

    const bool divisionsChanged = after.nx != before.nx || after.ny != before.ny || after.nz != before.nz;
    const bool dimensionsChanged = !qFuzzyCompare(after.lengthMm, before.lengthMm)
        || !qFuzzyCompare(after.widthMm, before.widthMm) || !qFuzzyCompare(after.heightMm, before.heightMm);
    if (!divisionsChanged && !dimensionsChanged) {
        return;
    }
    pushMeshCommand(after, divisionsChanged ? tr("Change Mesh Divisions") : tr("Change Mesh Dimensions"));
}

void MeshDetails::pushMeshCommand(const MeshService::Definition &after, const QString &text)
{
    const MeshService::Definition before = services_.mesh->definition();
    if (services_.commands == nullptr || after == before) {
        return;
    }
    services_.commands->push(new commands::SetMeshDefinitionCommand(services_, before, after, text));
    emit modelEdited();
}

int MeshDetails::staleFemScopeCount() const
{
    if (services_.mesh == nullptr) {
        return 0;
    }

    int count = 0;
    if (services_.namedSelections != nullptr) {
        for (const ObjectId id : services_.namedSelections->order()) {
            const NamedSelectionDefinition *definition = services_.namedSelections->byId(id);
            if (definition != nullptr && isStaleMeshScope(definition->scope, services_.mesh)) {
                ++count;
            }
        }
    }
    if (services_.contacts != nullptr) {
        for (const ObjectId id : services_.contacts->order()) {
            const ContactDefinition *definition = services_.contacts->byId(id);
            if (definition == nullptr) {
                continue;
            }
            if (isStaleMeshScope(definition->sourceScope, services_.mesh)) {
                ++count;
            }
            if (isStaleMeshScope(definition->targetScope, services_.mesh)) {
                ++count;
            }
        }
    }
    return count;
}

void MeshDetails::refresh()
{
    if (services_.mesh == nullptr || services_.geometry == nullptr) {
        return;
    }

    updating_ = true;
    const MeshService::Definition &definition = services_.mesh->definition();
    const bool derived = services_.mesh->dimensionsAreDerived();
    const GeometrySummary geometrySummary = services_.geometry->summary();
    const bool hasGeometry = geometrySummary.hasGeometry;

    source_->setCurrentIndex(definition.source == MeshSource::GeometryBoundingBox ? 1 : 0);
    source_->setEnabled(hasGeometry);
    source_->setToolTip(hasGeometry ? QString()
                                    : tr("Geometri içe aktarılmadığı için yalnız parametrik kutu kullanılabilir."));

    length_->setValue(definition.lengthMm);
    width_->setValue(definition.widthMm);
    height_->setValue(definition.heightMm);
    for (auto *field : {length_, width_, height_}) {
        field->setEnabled(!derived);
        field->setToolTip(derived ? tr("Ölçüler içe aktarılan CAD gövdesinin sınır kutusundan türetiliyor.")
                                  : QString());
    }
    nx_->setValue(definition.nx);
    ny_->setValue(definition.ny);
    nz_->setValue(definition.nz);

    generation_->setText(QString::number(services_.mesh->generation()));
    settingsRevision_->setText(QString::number(services_.mesh->settingsRevision()));
    const bool geometryDriven = definition.source == MeshSource::GeometryBoundingBox;
    sourceGeometryRevision_->setText(
        geometryDriven ? QString::number(geometrySummary.revision) : tr("— (Parametric)"));
    meshedGeometryRevision_->setText(
        geometryDriven && services_.mesh->hasMesh()
            ? QString::number(services_.mesh->meshedGeometryRevision())
            : tr("—"));
    staleScopes_->setText(QString::number(staleFemScopeCount()));

    if (services_.mesh->hasMesh()) {
        nodes_->setText(QString::number(services_.mesh->nodeCount()));
        elements_->setText(QString::number(services_.mesh->elementCount()));
        facets_->setText(QString::number(services_.mesh->boundaryFacetCount()));
        dof_->setText(QString::number(services_.mesh->dofCount()));
        const auto quality = services_.mesh->quality();
        scaledJacobian_->setText(QString::number(quality.minimumScaledJacobian, 'f', 4));
        aspectRatio_->setText(QString::number(quality.maximumAspectRatio, 'f', 3));
        inverted_->setText(QString::number(quality.invertedElementCount));
        status_->setText(services_.mesh->isUpToDate() ? tr("Up to date")
                                                      : tr("Out of date — yeniden üretin"));
    } else {
        const QString dash = tr("—");
        nodes_->setText(dash);
        elements_->setText(dash);
        facets_->setText(dash);
        dof_->setText(dash);
        scaledJacobian_->setText(dash);
        aspectRatio_->setText(dash);
        inverted_->setText(dash);
        status_->setText(tr("Mesh üretilmedi"));
    }

    generate_->setEnabled(true);
    clearGenerated_->setEnabled(services_.mesh->hasMesh());
    clearGenerated_->setToolTip(services_.mesh->hasMesh()
                                    ? tr("Üretilmiş FEM mesh'i temizler; mesh tanımını korur.")
                                    : tr("Temizlenecek üretilmiş mesh yok."));

    const int predictedDof = services_.mesh->predictedDofCount();
    predicted_->setText(tr("%1 node · %2 element · %3 DOF")
                            .arg(services_.mesh->predictedNodeCount())
                            .arg(services_.mesh->predictedElementCount())
                            .arg(predictedDof));
    if (predictedDof > AnalysisService::maximumDofThreshold()) {
        solverLimit_->setText(tr("%1 DOF sınırı aşıldı (%2)")
                                  .arg(AnalysisService::maximumDofThreshold())
                                  .arg(predictedDof));
    } else if (predictedDof > AnalysisService::warningDofThreshold()) {
        solverLimit_->setText(tr("Yoğun çözücü — %1 DOF üzeri yavaşlar").arg(AnalysisService::warningDofThreshold()));
    } else {
        solverLimit_->setText(tr("Uygun (yoğun referans çözücü)"));
    }

    updating_ = false;
}

} // namespace d26
