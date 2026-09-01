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
namespace {

bool sameMaterialDefinition(const MaterialDefinition &left, const MaterialDefinition &right)
{
    if (left.name != right.name || left.model != right.model || left.youngGPa != right.youngGPa
        || left.poisson != right.poisson || left.densityKgM3 != right.densityKgM3
        || left.bulkMPa != right.bulkMPa || left.c10MPa != right.c10MPa
        || left.c01MPa != right.c01MPa || left.c20MPa != right.c20MPa
        || left.c30MPa != right.c30MPa || left.ogdenTerms != right.ogdenTerms) {
        return false;
    }
    for (int i = 0; i < 3; ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        if (left.ogdenMuMPa[index] != right.ogdenMuMPa[index]
            || left.ogdenAlpha[index] != right.ogdenAlpha[index]) {
            return false;
        }
    }
    return true;
}

} // namespace

MaterialDetails::MaterialDetails(const ServiceContext &services, QWidget *parent)
    : DetailsPage(parent), services_(services)
{
    setObjectName(QStringLiteral("Dynamics26MaterialInspector"));

    auto *general = addSection(tr("General"));
    name_ = new QLineEdit;
    name_->setObjectName(QStringLiteral("Dynamics26MaterialName"));
    name_->setMinimumHeight(22);
    general->addRow(tr("Name"), name_);
    type_ = general->addValueRow(tr("Type"));
    type_->setObjectName(QStringLiteral("Dynamics26MaterialType"));
    density_ = makeDoubleField(0.001, 1.0e6, 1, tr(" kg/m³"));
    density_->setObjectName(QStringLiteral("Dynamics26MaterialDensity"));
    general->addRow(tr("Density"), density_);

    auto *modelSection = addSection(tr("Model"));
    model_ = makeCombo({displayName(MaterialModel::LinearElastic), displayName(MaterialModel::NeoHookean),
                        displayName(MaterialModel::MooneyRivlin), displayName(MaterialModel::Yeoh),
                        displayName(MaterialModel::Ogden)});
    model_->setObjectName(QStringLiteral("Dynamics26MaterialModel"));
    modelSection->addRow(tr("Model"), model_);

    elasticSection_ = addSection(tr("Elastic Parameters"));
    young_ = makeDoubleField(0.001, 10000.0, 2, tr(" GPa"));
    young_->setObjectName(QStringLiteral("Dynamics26MaterialYoungGPa"));
    poisson_ = makeDoubleField(-0.99, 0.4999, 4, QString());
    poisson_->setObjectName(QStringLiteral("Dynamics26MaterialPoisson"));
    youngRow_ = elasticSection_->addRow(tr("Young's Modulus"), young_);
    poissonRow_ = elasticSection_->addRow(tr("Poisson's Ratio"), poisson_);

    hyperelasticSection_ = addSection(tr("Parameters"));
    bulk_ = makeDoubleField(0.001, 1.0e6, 2, tr(" MPa"));
    bulk_->setObjectName(QStringLiteral("Dynamics26MaterialBulkMPa"));
    c10_ = makeDoubleField(0.0, 1.0e5, 4, tr(" MPa"));
    c10_->setObjectName(QStringLiteral("Dynamics26MaterialC10MPa"));
    c01_ = makeDoubleField(0.0, 1.0e5, 4, tr(" MPa"));
    c01_->setObjectName(QStringLiteral("Dynamics26MaterialC01MPa"));
    c20_ = makeDoubleField(-1.0e5, 1.0e5, 4, tr(" MPa"));
    c20_->setObjectName(QStringLiteral("Dynamics26MaterialC20MPa"));
    c30_ = makeDoubleField(-1.0e5, 1.0e5, 4, tr(" MPa"));
    c30_->setObjectName(QStringLiteral("Dynamics26MaterialC30MPa"));
    ogdenTerms_ = makeIntField(1, 3);
    ogdenTerms_->setObjectName(QStringLiteral("Dynamics26MaterialOgdenTerms"));
    bulkRow_ = hyperelasticSection_->addRow(tr("Bulk Modulus K"), bulk_);
    c10Row_ = hyperelasticSection_->addRow(QStringLiteral("C10"), c10_);
    c01Row_ = hyperelasticSection_->addRow(QStringLiteral("C01"), c01_);
    c20Row_ = hyperelasticSection_->addRow(QStringLiteral("C20"), c20_);
    c30Row_ = hyperelasticSection_->addRow(QStringLiteral("C30"), c30_);
    ogdenTermsRow_ = hyperelasticSection_->addRow(tr("Ogden Terms"), ogdenTerms_);
    for (int i = 0; i < 3; ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        ogdenMu_[index] = makeDoubleField(-1.0e5, 1.0e5, 4, tr(" MPa"));
        ogdenMu_[index]->setObjectName(QStringLiteral("Dynamics26MaterialOgdenMu%1").arg(i + 1));
        ogdenAlpha_[index] = makeDoubleField(-100.0, 100.0, 3, QString());
        ogdenAlpha_[index]->setObjectName(QStringLiteral("Dynamics26MaterialOgdenAlpha%1").arg(i + 1));
        ogdenMuRow_[index] = hyperelasticSection_->addRow(QStringLiteral("μ%1").arg(i + 1), ogdenMu_[index]);
        ogdenAlphaRow_[index] = hyperelasticSection_->addRow(QStringLiteral("α%1").arg(i + 1), ogdenAlpha_[index]);
    }

    testDataSection_ = addSection(tr("Test Data"), true, true);
    auto *evaluate = makeActionButton(tr("Evaluate Uniaxial Curve"));
    evaluate->setObjectName(QStringLiteral("Dynamics26MaterialEvaluateCurve"));
    testDataSection_->addFullWidth(evaluate);
    curve_ = new MaterialCurveWidget(this);
    curve_->setObjectName(QStringLiteral("Dynamics26MaterialCurve"));
    curve_->setMinimumHeight(150);
    testDataSection_->addFullWidth(curve_);
    curveStatus_ = makeNoteLabel(QString());
    curveStatus_->setObjectName(QStringLiteral("Dynamics26MaterialCurveStatus"));
    testDataSection_->addFullWidth(curveStatus_);
    testDataSection_->addNote(tr("Önizleme J = 1 kabulüyle izokorik uniaxial nominal gerilme–uzama eğrisidir. "
                                 "K parametresi mixed u-p formülasyonundan ayrıdır."));

    auto *assignmentSection = addSection(tr("Assignment"));
    assignment_ = assignmentSection->addValueRow(tr("Assigned To"));
    assignment_->setObjectName(QStringLiteral("Dynamics26MaterialAssignment"));
    assignButton_ = makeActionButton(tr("Assign to Body"));
    assignButton_->setObjectName(QStringLiteral("Dynamics26MaterialAssign"));
    assignmentSection->addFullWidth(assignButton_);

    auto *advanced = addSection(tr("Engineering State"), true, true);
    identity_ = advanced->addValueRow(tr("Object ID"));
    identity_->setObjectName(QStringLiteral("Dynamics26MaterialObjectId"));
    revision_ = advanced->addValueRow(tr("Material Revision"));
    revision_->setObjectName(QStringLiteral("Dynamics26MaterialRevision"));
    solveNote_ = advanced->addValueRow(tr("Static Structural"));
    solveNote_->setObjectName(QStringLiteral("Dynamics26MaterialSolveCompatibility"));
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

    // Engineering property'leri kullanıcının edit commit anında document'a
    // yazılır. Böylece spinbox yazımı sırasında her ara rakam ayrı Undo adımı
    // üretmez ve farklı alanlar zaman yakınlığı nedeniyle birbirine karışmaz.
    const auto push = [this] { pushDefinition(); };
    connect(name_, &QLineEdit::editingFinished, this, push);
    connect(density_, &QDoubleSpinBox::editingFinished, this, push);
    connect(young_, &QDoubleSpinBox::editingFinished, this, push);
    connect(poisson_, &QDoubleSpinBox::editingFinished, this, push);
    connect(bulk_, &QDoubleSpinBox::editingFinished, this, push);
    connect(c10_, &QDoubleSpinBox::editingFinished, this, push);
    connect(c01_, &QDoubleSpinBox::editingFinished, this, push);
    connect(c20_, &QDoubleSpinBox::editingFinished, this, push);
    connect(c30_, &QDoubleSpinBox::editingFinished, this, push);
    connect(ogdenTerms_, &QSpinBox::editingFinished, this, push);
    for (int i = 0; i < 3; ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        connect(ogdenMu_[index], &QDoubleSpinBox::editingFinished, this, push);
        connect(ogdenAlpha_[index], &QDoubleSpinBox::editingFinished, this, push);
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
        const std::size_t index = static_cast<std::size_t>(i);
        definition.ogdenMuMPa[index] = ogdenMu_[index]->value();
        definition.ogdenAlpha[index] = ogdenAlpha_[index]->value();
    }

    // Focus değişimi veya refresh sonrası aynı değer tekrar signal üretse bile
    // document history'ye sahte bir engineering mutation eklenmez.
    if (sameMaterialDefinition(*existing, definition)) {
        return;
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
        const std::size_t index = static_cast<std::size_t>(i);
        ogdenMu_[index]->setValue(material->ogdenMuMPa[index]);
        ogdenAlpha_[index]->setValue(material->ogdenAlpha[index]);
    }

    solveNote_->setText(material->supportsLinearStaticSolve()
                            ? tr("Ready — lineer izotropik solver ile uyumlu")
                            : tr("Preview only — mevcut Static Structural solver desteklemiyor"));
    identity_->setText(QString::number(objectId_));
    revision_->setText(QString::number(services_.materials->revision()));

    const bool isAssigned = services_.materials->assignedMaterialId() == objectId_;
    assignment_->setText(isAssigned ? tr("Model body · current material") : tr("Atanmamış"));
    assignButton_->setEnabled(!isAssigned);
    assignButton_->setToolTip(isAssigned ? tr("Bu malzeme zaten model gövdesine atanmış.") : QString());
    updateVisibility();
    updating_ = false;
}

} // namespace d26
