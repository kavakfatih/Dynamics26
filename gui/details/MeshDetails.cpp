#include "MeshDetails.h"

#include "../services/AnalysisService.h"
#include "../services/GeometryService.h"
#include "../services/MeshService.h"
#include "../commands/DomainCommands.h"
#include "../core/DocumentCommandManager.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

namespace d26 {

MeshDetails::MeshDetails(const ServiceContext &services, QWidget *parent)
    : DetailsPage(parent), services_(services)
{
    auto *definition = addSection(tr("Definition"));
    method_ = definition->addValueRow(tr("Method"), tr("Structured HEX8"));
    elementType_ = definition->addValueRow(tr("Element Type"), tr("HEX8"));
    source_ = makeCombo({tr("Parametric Box"), tr("From Geometry")});
    definition->addRow(tr("Source"), source_);
    status_ = definition->addValueRow(tr("Status"));

    auto *sizing = addSection(tr("Sizing"));
    length_ = makeDoubleField(0.001, 1.0e5, 2, tr(" mm"));
    width_ = makeDoubleField(0.001, 1.0e5, 2, tr(" mm"));
    height_ = makeDoubleField(0.001, 1.0e5, 2, tr(" mm"));
    sizing->addRow(tr("Length"), length_);
    sizing->addRow(tr("Width"), width_);
    sizing->addRow(tr("Height"), height_);

    auto *divisions = addSection(tr("Divisions"));
    nx_ = makeIntField(1, 200);
    ny_ = makeIntField(1, 200);
    nz_ = makeIntField(1, 200);
    divisions->addRow(QStringLiteral("Nx"), nx_);
    divisions->addRow(QStringLiteral("Ny"), ny_);
    divisions->addRow(QStringLiteral("Nz"), nz_);

    auto *statistics = addSection(tr("Statistics"));
    nodes_ = statistics->addValueRow(tr("Nodes"));
    elements_ = statistics->addValueRow(tr("Elements"));
    facets_ = statistics->addValueRow(tr("Boundary Facets"));
    dof_ = statistics->addValueRow(tr("Total DOF"));

    auto *quality = addSection(tr("Quality"));
    scaledJacobian_ = quality->addValueRow(tr("Min Scaled Jacobian"));
    aspectRatio_ = quality->addValueRow(tr("Max Aspect Ratio"));
    inverted_ = quality->addValueRow(tr("Inverted Elements"));

    auto *actions = addSection(tr("Actions"));
    auto *generate = makeActionButton(tr("Generate Mesh"));
    actions->addFullWidth(generate);

    auto *advanced = addSection(tr("Advanced"), true, true);
    predicted_ = advanced->addValueRow(tr("Predicted Size"));
    solverLimit_ = advanced->addValueRow(tr("Solver Limit"));
    advanced->addNote(tr("Structured HEX8 baseline yalnız eksen hizalı kutu gövdeyi mesher. "
                         "Keyfi STEP hacim meshleme henüz üretim seviyesinde değildir; "
                         "bu nedenle kutu dışı gövdeler için parametrik kutu kullanılır."));

    addStretch();

    connect(generate, &QPushButton::clicked, this, [this] { emit requestCommand(QStringLiteral("mesh.generate")); });
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
    services_.commands->push(new commands::SetMeshDefinitionCommand(services_, before, after, text));
    emit modelEdited();
}

void MeshDetails::refresh()
{
    updating_ = true;
    const MeshService::Definition &definition = services_.mesh->definition();
    const bool derived = services_.mesh->dimensionsAreDerived();
    const bool hasGeometry = services_.geometry->summary().hasGeometry;

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

    if (services_.mesh->hasMesh()) {
        nodes_->setText(QString::number(services_.mesh->nodeCount()));
        elements_->setText(QString::number(services_.mesh->elementCount()));
        facets_->setText(QString::number(services_.mesh->boundaryFacetCount()));
        dof_->setText(QString::number(services_.mesh->dofCount()));
        const auto quality = services_.mesh->quality();
        scaledJacobian_->setText(QString::number(quality.minimumScaledJacobian, 'f', 4));
        aspectRatio_->setText(QString::number(quality.maximumAspectRatio, 'f', 3));
        inverted_->setText(QString::number(quality.invertedElementCount));
        status_->setText(services_.mesh->isUpToDate() ? tr("Up to date") : tr("Geometri değişti — yeniden üretin"));
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
