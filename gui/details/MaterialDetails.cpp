#include "MaterialDetails.h"

#include "../widgets/MaterialCurveWidget.h"
#include "../services/MaterialService.h"
#include "../commands/DomainCommands.h"
#include "../core/DocumentCommandManager.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

namespace d26 {

MaterialDetails::MaterialDetails(const ServiceContext &services, QWidget *parent)
    : DetailsPage(parent), services_(services)
{
    auto *general = addSection(tr("General"));
    name_ = new QLineEdit;
    name_->setMinimumHeight(22);
    general->addRow(tr("Name"), name_);
    type_ = general->addValueRow(tr("Type"));
    density_ = makeDoubleField(0.001, 1.0e6, 1, tr(" kg/m³"));
    general->addRow(tr("Density"), density_);

    auto *modelSection = addSection(tr("Model"));
    model_ = makeCombo({displayName(MaterialModel::LinearElastic), displayName(MaterialModel::NeoHookean),
                        displayName(MaterialModel::MooneyRivlin), displayName(MaterialModel::Yeoh),
                        displayName(MaterialModel::Ogden)});
    modelSection->addRow(tr("Model"), model_);

    elasticSection_ = addSection(tr("Elastic Parameters"));
    young_ = makeDoubleField(0.001, 10000.0, 2, tr(" GPa"));
    poisson_ = makeDoubleField(-0.99, 0.4999, 4, QString());
    youngRow_ = elasticSection_->addRow(tr("Young's Modulus"), young_);
    poissonRow_ = elasticSection_->addRow(tr("Poisson's Ratio"), poisson_);

    hyperelasticSection_ = addSection(tr("Parameters"));
    bulk_ = makeDoubleField(0.001, 1.0e6, 2, tr(" MPa"));
    c10_ = makeDoubleField(0.0, 1.0e5, 4, tr(" MPa"));
    c01_ = makeDoubleField(0.0, 1.0e5, 4, tr(" MPa"));
    c20_ = makeDoubleField(-1.0e5, 1.0e5, 4, tr(" MPa"));
    c30_ = makeDoubleField(-1.0e5, 1.0e5, 4, tr(" MPa"));
    ogdenTerms_ = makeIntField(1, 3);
    bulkRow_ = hyperelasticSection_->addRow(tr("Bulk Modulus K"), bulk_);
    c10Row_ = hyperelasticSection_->addRow(QStringLiteral("C10"), c10_);
    c01Row_ = hyperelasticSection_->addRow(QStringLiteral("C01"), c01_);
    c20Row_ = hyperelasticSection_->addRow(QStringLiteral("C20"), c20_);
    c30Row_ = hyperelasticSection_->addRow(QStringLiteral("C30"), c30_);
    ogdenTermsRow_ = hyperelasticSection_->addRow(tr("Ogden Terms"), ogdenTerms_);
    for (int i = 0; i < 3; ++i) {
        ogdenMu_[static_cast<std::size_t>(i)] = makeDoubleField(-1.0e5, 1.0e5, 4, tr(" MPa"));
        ogdenAlpha_[static_cast<std::size_t>(i)] = makeDoubleField(-100.0, 100.0, 3, QString());
        ogdenMuRow_[static_cast<std::size_t>(i)] =
            hyperelasticSection_->addRow(QStringLiteral("μ%1").arg(i + 1), ogdenMu_[static_cast<std::size_t>(i)]);
        ogdenAlphaRow_[static_cast<std::size_t>(i)] =
            hyperelasticSection_->addRow(QStringLiteral("α%1").arg(i + 1), ogdenAlpha_[static_cast<std::size_t>(i)]);
    }

    testDataSection_ = addSection(tr("Test Data"), true, true);
    auto *evaluate = makeActionButton(tr("Evaluate Uniaxial Curve"));
    testDataSection_->addFullWidth(evaluate);
    curve_ = new MaterialCurveWidget(this);
    curve_->setMinimumHeight(150);
    testDataSection_->addFullWidth(curve_);
    curveStatus_ = makeNoteLabel(QString());
    testDataSection_->addFullWidth(curveStatus_);
    testDataSection_->addNote(tr("Önizleme J = 1 kabulüyle izokorik uniaxial nominal gerilme–uzama eğrisidir. "
                                 "K parametresi mixed u-p formülasyonundan ayrıdır."));

    auto *assignmentSection = addSection(tr("Assignment"));
    assignment_ = assignmentSection->addValueRow(tr("Assigned To"));
    assignButton_ = makeActionButton(tr("Assign to Body"));
    assignmentSection->addFullWidth(assignButton_);

    auto *advanced = addSection(tr("Advanced"), true, true);
    solveNote_ = advanced->addValueRow(tr("Static Structural"));
    advanced->addNote(tr("Static Structural çözüm yolu HEX8 lineer izotropik elastisite kullanır. "
                         "Hyperelastic modeller bu sürümde parametre doğrulama ve eğri önizlemesi "
                         "seviyesinde bağlıdır."));

    addStretch();

    connect(evaluate, &QPushButton::clicked, this, &MaterialDetails::evaluateCurve);
    connect(assignButton_, &QPushButton::clicked, this, [this] {
        if (services_.materials->assignedMaterialId() == objectId_) {
            return;
        }
        services_.commands->push(new commands::AssignMaterialCommand(services_, objectId_));
        emit modelEdited();
    });
    const auto push = [this] { pushDefinition(); };
    connect(name_, &QLineEdit::editingFinished, this, push);
    connect(density_, &QDoubleSpinBox::valueChanged, this, push);
    connect(young_, &QDoubleSpinBox::valueChanged, this, push);
    connect(poisson_, &QDoubleSpinBox::valueChanged, this, push);
    connect(bulk_, &QDoubleSpinBox::valueChanged, this, push);
    connect(c10_, &QDoubleSpinBox::valueChanged, this, push);
    connect(c01_, &QDoubleSpinBox::valueChanged, this, push);
    connect(c20_, &QDoubleSpinBox::valueChanged, this, push);
    connect(c30_, &QDoubleSpinBox::valueChanged, this, push);
    connect(ogdenTerms_, &QSpinBox::valueChanged, this, push);
    for (int i = 0; i < 3; ++i) {
        connect(ogdenMu_[static_cast<std::size_t>(i)], &QDoubleSpinBox::valueChanged, this, push);
        connect(ogdenAlpha_[static_cast<std::size_t>(i)], &QDoubleSpinBox::valueChanged, this, push);
    }
    connect(model_, &QComboBox::currentIndexChanged, this, [this](int) {
        pushDefinition();
        updateVisibility();
        curve_->clearCurve();
        curveStatus_->setText(QString());
    });
}

void MaterialDetails::pushDefinition()
{
    if (updating_) {
        return;
    }
    const MaterialDefinition *existing = services_.materials->byId(objectId_);
    if (existing == nullptr) {
        return;
    }
    MaterialDefinition definition = *existing;
    definition.name = name_->text().trimmed().isEmpty() ? tr("Material 1") : name_->text().trimmed();
    definition.model = static_cast<MaterialModel>(model_->currentIndex());
    definition.densityKgM3 = density_->value();
    definition.youngGPa = young_->value();
    definition.poisson = poisson_->value();
    definition.bulkMPa = bulk_->value();
    definition.c10MPa = c10_->value();
    definition.c01MPa = c01_->value();
    definition.c20MPa = c20_->value();
    definition.c30MPa = c30_->value();
    definition.ogdenTerms = ogdenTerms_->value();
    for (int i = 0; i < 3; ++i) {
        definition.ogdenMuMPa[static_cast<std::size_t>(i)] = ogdenMu_[static_cast<std::size_t>(i)]->value();
        definition.ogdenAlpha[static_cast<std::size_t>(i)] = ogdenAlpha_[static_cast<std::size_t>(i)]->value();
    }
    services_.commands->push(new commands::SetMaterialPropertiesCommand(services_, objectId_, *existing, definition));
    emit modelEdited();
}

void MaterialDetails::updateVisibility()
{
    const auto model = static_cast<MaterialModel>(model_->currentIndex());
    const bool linear = model == MaterialModel::LinearElastic;
    elasticSection_->setVisible(linear);
    hyperelasticSection_->setVisible(!linear);
    testDataSection_->setVisible(!linear);

    const bool ogden = model == MaterialModel::Ogden;
    c10Row_->setVisible(!ogden);
    c01Row_->setVisible(model == MaterialModel::MooneyRivlin);
    c20Row_->setVisible(model == MaterialModel::Yeoh);
    c30Row_->setVisible(model == MaterialModel::Yeoh);
    ogdenTermsRow_->setVisible(ogden);
    for (int i = 0; i < 3; ++i) {
        const bool active = ogden && i < ogdenTerms_->value();
        ogdenMuRow_[static_cast<std::size_t>(i)]->setVisible(active);
        ogdenAlphaRow_[static_cast<std::size_t>(i)]->setVisible(active);
    }
}

void MaterialDetails::evaluateCurve()
{
    const HyperelasticPreview preview = services_.materials->preview(objectId_);
    curveStatus_->setText(preview.message);
    if (preview.ok) {
        curve_->setCurve(preview.curve, tr("Stretch λ"), tr("Nominal Stress [MPa]"));
    } else {
        curve_->clearCurve();
    }
}

void MaterialDetails::refresh()
{
    const MaterialDefinition *material = services_.materials->byId(objectId_);
    if (material == nullptr) {
        return;
    }
    updating_ = true;
    name_->setText(material->name);
    model_->setCurrentIndex(static_cast<int>(material->model));
    type_->setText(material->model == MaterialModel::LinearElastic ? tr("Isotropic Elastic") : tr("Hyperelastic"));
    density_->setValue(material->densityKgM3);
    young_->setValue(material->youngGPa);
    poisson_->setValue(material->poisson);
    bulk_->setValue(material->bulkMPa);
    c10_->setValue(material->c10MPa);
    c01_->setValue(material->c01MPa);
    c20_->setValue(material->c20MPa);
    c30_->setValue(material->c30MPa);
    ogdenTerms_->setValue(material->ogdenTerms);
    for (int i = 0; i < 3; ++i) {
        ogdenMu_[static_cast<std::size_t>(i)]->setValue(material->ogdenMuMPa[static_cast<std::size_t>(i)]);
        ogdenAlpha_[static_cast<std::size_t>(i)]->setValue(material->ogdenAlpha[static_cast<std::size_t>(i)]);
    }
    solveNote_->setText(material->supportsLinearStaticSolve() ? tr("Destekleniyor")
                                                             : tr("Bu malzeme modeliyle etkin değil"));
    const bool isAssigned = services_.materials->assignedMaterialId() == objectId_;
    assignment_->setText(isAssigned ? tr("Body 1") : tr("—"));
    assignButton_->setEnabled(!isAssigned);
    assignButton_->setToolTip(isAssigned ? tr("Bu malzeme zaten gövdeye atanmış.") : QString());
    updateVisibility();
    updating_ = false;
}

} // namespace d26
