#include "BoundaryConditionDetails.h"

#include "../services/AnalysisService.h"
#include "../services/MeshService.h"
#include "../commands/DomainCommands.h"
#include "../core/DocumentCommandManager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>

#include <algorithm>
#include <array>
#include <iterator>

namespace d26 {
namespace {

const std::array<BoxFace, 6> kFaces{BoxFace::XMin, BoxFace::XMax, BoxFace::YMin,
                                    BoxFace::YMax, BoxFace::ZMin, BoxFace::ZMax};

QStringList faceNames()
{
    QStringList names;
    for (const auto face : kFaces) {
        names << displayName(face);
    }
    return names;
}

} // namespace

BoundaryConditionDetails::BoundaryConditionDetails(const ServiceContext &services, QWidget *parent)
    : DetailsPage(parent), services_(services)
{
    auto *scopeSection = addSection(tr("Scope"));
    name_ = new QLineEdit;
    name_->setMinimumHeight(22);
    scopeSection->addRow(tr("Name"), name_);
    scopingMethod_ = scopeSection->addValueRow(tr("Scoping Method"), tr("Geometry Selection"));
    scope_ = makeCombo(faceNames());
    scopeSection->addRow(tr("Geometry"), scope_);
    scopeStatistics_ = scopeSection->addValueRow(tr("Resolved"));

    supportSection_ = addSection(tr("Definition"));
    behavior_ = supportSection_->addValueRow(tr("Behavior"));
    auto *dofWidget = new QWidget(this);
    auto *dofLayout = new QHBoxLayout(dofWidget);
    dofLayout->setContentsMargins(0, 0, 0, 0);
    dofLayout->setSpacing(10);
    fixX_ = new QCheckBox(QStringLiteral("X"), dofWidget);
    fixY_ = new QCheckBox(QStringLiteral("Y"), dofWidget);
    fixZ_ = new QCheckBox(QStringLiteral("Z"), dofWidget);
    dofLayout->addWidget(fixX_);
    dofLayout->addWidget(fixY_);
    dofLayout->addWidget(fixZ_);
    dofLayout->addStretch(1);
    supportSection_->addRow(tr("Constrained DOF"), dofWidget);

    loadSection_ = addSection(tr("Definition"));
    defineBy_ = loadSection_->addValueRow(tr("Define By"), tr("Components"));
    fx_ = makeDoubleField(-1.0e9, 1.0e9, 2, tr(" N"));
    fy_ = makeDoubleField(-1.0e9, 1.0e9, 2, tr(" N"));
    fz_ = makeDoubleField(-1.0e9, 1.0e9, 2, tr(" N"));
    loadSection_->addRow(tr("X Component"), fx_);
    loadSection_->addRow(tr("Y Component"), fy_);
    loadSection_->addRow(tr("Z Component"), fz_);
    magnitude_ = loadSection_->addValueRow(tr("Magnitude"));

    auto *coordinateSection = addSection(tr("Coordinate System"));
    coordinateSystem_ = coordinateSection->addValueRow(tr("System"), tr("Global"));

    auto *advanced = addSection(tr("Advanced"), true, true);
    advanced->addNote(tr("Toplam kuvvet, kapsanan yüzün çözülmüş FEM düğümlerine eşit dağıtılır. "
                         "Düğüm listesi geometri provenance'ı üzerinden AssignmentResolver ile bulunur; "
                         "görüntüleme üçgenlemesi bu zincirin hiçbir adımında kullanılmaz."));

    addStretch();

    const auto pushEdit = [this] { push(); };
    connect(name_, &QLineEdit::editingFinished, this, pushEdit);
    connect(scope_, &QComboBox::currentIndexChanged, this, pushEdit);
    connect(fixX_, &QCheckBox::toggled, this, pushEdit);
    connect(fixY_, &QCheckBox::toggled, this, pushEdit);
    connect(fixZ_, &QCheckBox::toggled, this, pushEdit);
    connect(fx_, &QDoubleSpinBox::valueChanged, this, pushEdit);
    connect(fy_, &QDoubleSpinBox::valueChanged, this, pushEdit);
    connect(fz_, &QDoubleSpinBox::valueChanged, this, pushEdit);
}

void BoundaryConditionDetails::push()
{
    if (updating_) {
        return;
    }
    const int index = qBound(0, scope_->currentIndex(), 5);
    const BoxFace face = kFaces[static_cast<std::size_t>(index)];
    if (isLoad_) {
        const LoadDefinition *existing = services_.analysis->load(objectId_);
        if (existing == nullptr) {
            return;
        }
        LoadDefinition definition = *existing;
        definition.name = name_->text().trimmed().isEmpty() ? existing->name : name_->text().trimmed();
        definition.scope = face;
        definition.fxN = fx_->value();
        definition.fyN = fy_->value();
        definition.fzN = fz_->value();
        if (definition.name == existing->name && definition.scope == existing->scope
            && qFuzzyCompare(definition.fxN, existing->fxN) && qFuzzyCompare(definition.fyN, existing->fyN)
            && qFuzzyCompare(definition.fzN, existing->fzN)) {
            return;
        }
        services_.commands->push(new commands::SetForceCommand(services_, objectId_, *existing, definition));
    } else {
        const SupportDefinition *existing = services_.analysis->support(objectId_);
        if (existing == nullptr) {
            return;
        }
        SupportDefinition definition = *existing;
        definition.name = name_->text().trimmed().isEmpty() ? existing->name : name_->text().trimmed();
        definition.scope = face;
        definition.fixX = fixX_->isChecked();
        definition.fixY = fixY_->isChecked();
        definition.fixZ = fixZ_->isChecked();
        if (definition.name == existing->name && definition.scope == existing->scope
            && definition.fixX == existing->fixX && definition.fixY == existing->fixY
            && definition.fixZ == existing->fixZ) {
            return;
        }
        services_.commands->push(new commands::SetSupportCommand(services_, objectId_, *existing, definition));
    }
    emit scopeHighlightRequested(services_.mesh->geometryIdFor(face));
    emit modelEdited();
}

void BoundaryConditionDetails::refresh()
{
    const SupportDefinition *support = services_.analysis->support(objectId_);
    const LoadDefinition *load = services_.analysis->load(objectId_);
    if (support == nullptr && load == nullptr) {
        return;
    }
    updating_ = true;
    isLoad_ = load != nullptr;
    supportSection_->setVisible(!isLoad_);
    loadSection_->setVisible(isLoad_);

    const BoxFace face = isLoad_ ? load->scope : support->scope;
    const auto position = std::find(kFaces.begin(), kFaces.end(), face);
    scope_->setCurrentIndex(static_cast<int>(std::distance(kFaces.begin(), position)));
    name_->setText(isLoad_ ? load->name : support->name);

    if (services_.mesh->hasMesh()) {
        scopeStatistics_->setText(tr("%1 facet · %2 node")
                                      .arg(services_.mesh->facetCountFor(face))
                                      .arg(services_.mesh->nodeCountFor(face)));
    } else {
        scopeStatistics_->setText(tr("Mesh üretilmedi"));
    }

    if (isLoad_) {
        fx_->setValue(load->fxN);
        fy_->setValue(load->fyN);
        fz_->setValue(load->fzN);
        magnitude_->setText(tr("%1 N").arg(load->magnitudeN(), 0, 'g', 8));
    } else {
        fixX_->setChecked(support->fixX);
        fixY_->setChecked(support->fixY);
        fixZ_->setChecked(support->fixZ);
        const bool all = support->fixX && support->fixY && support->fixZ;
        behavior_->setText(all ? tr("Fixed") : tr("Partially Constrained"));
    }
    updating_ = false;

    emit scopeHighlightRequested(services_.mesh->geometryIdFor(face));
}

} // namespace d26
