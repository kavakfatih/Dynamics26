#include "BoundaryConditionDetails.h"

#include "../commands/DomainCommands.h"
#include "../core/DocumentCommandManager.h"
#include "../services/AnalysisService.h"
#include "../services/MeshService.h"
#include "../services/NamedSelectionService.h"
#include "../shell/Dynamics26MainWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>

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

bool isGeometryFaceScope(const NamedSelectionDefinition &definition)
{
    if (definition.scope.entities.isEmpty()) {
        return false;
    }
    return std::all_of(definition.scope.entities.cbegin(), definition.scope.entities.cend(),
                       [](const ScopeEntityReference &entity) {
                           return entity.domain == SelectionDomain::Geometry
                               && entity.kind == SelectionKind::Face;
                       });
}

quint64 singleResolvedGeometryId(const BoundaryScopeResolution &resolution)
{
    // Mevcut viewport highlight API tek CAD Face kimliği kabul eder. Persistent
    // consumer scope current model üzerinde tam olarak bir Face'e çözülüyorsa
    // gerçek kimlik highlight edilir. Multi-face/stale/dangling kapsamda ilk ID
    // seçilmiş gibi gösterilmez; highlight güvenli biçimde temizlenir.
    if (!resolution.valid || resolution.geometryFaceIds.size() != 1) {
        return 0;
    }
    return static_cast<quint64>(resolution.geometryFaceIds.front());
}

} // namespace

BoundaryConditionDetails::BoundaryConditionDetails(const ServiceContext &services, QWidget *parent)
    : DetailsPage(parent), services_(services)
{
    auto *scopeSection = addSection(tr("Scope"));
    name_ = new QLineEdit;
    name_->setMinimumHeight(22);
    scopeSection->addRow(tr("Name"), name_);

    scopingMethod_ = makeCombo({tr("Geometry Selection"), tr("Named Selection")});
    scopingMethod_->setObjectName(QStringLiteral("Dynamics26BoundaryScopingMethod"));
    scopingMethod_->setItemData(0, static_cast<int>(BoundaryScopingMethod::GeometrySelection));
    scopingMethod_->setItemData(1, static_cast<int>(BoundaryScopingMethod::NamedSelection));
    scopeSection->addRow(tr("Scoping Method"), scopingMethod_);

    scope_ = makeCombo(faceNames());
    scope_->setObjectName(QStringLiteral("Dynamics26BoundaryGeometryScope"));
    geometryScopeRow_ = scopeSection->addRow(tr("Geometry"), scope_);

    auto *namedSelectionWidget = new QWidget(this);
    auto *namedSelectionLayout = new QHBoxLayout(namedSelectionWidget);
    namedSelectionLayout->setContentsMargins(0, 0, 0, 0);
    namedSelectionLayout->setSpacing(5);
    namedSelection_ = makeCombo({});
    namedSelection_->setObjectName(QStringLiteral("Dynamics26BoundaryNamedSelection"));
    namedSelectionLayout->addWidget(namedSelection_, 1);
    showNamedSelection_ = new QToolButton(namedSelectionWidget);
    showNamedSelection_->setText(tr("Göster"));
    showNamedSelection_->setAutoRaise(true);
    showNamedSelection_->setToolButtonStyle(Qt::ToolButtonTextOnly);
    showNamedSelection_->setObjectName(QStringLiteral("Dynamics26BoundaryShowNamedSelection"));
    showNamedSelection_->setToolTip(tr("Referans verilen Named Selection nesnesini model ağacında göster"));
    namedSelectionLayout->addWidget(showNamedSelection_);
    namedSelectionRow_ = scopeSection->addRow(tr("Named Selection"), namedSelectionWidget);
    scopeStatistics_ = scopeSection->addValueRow(tr("Resolved"));
    scopeStatistics_->setObjectName(QStringLiteral("Dynamics26BoundaryScopeResolved"));

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
    advanced->addNote(tr("Geometry Selection tek CAD yüzünü doğrudan kapsar. Named Selection ise yalnız "
                         "persistent ObjectId referansı taşır; yüz kimlikleri Fixed Support / Force içine "
                         "kopyalanmaz. Birden fazla yüz kapsayan Force için bütün FEM düğümleri önce tek "
                         "union kümesine alınır ve toplam kuvvet bu kümeye yalnız bir kez dağıtılır. "
                         "Display tessellation solver scope değildir."));

    addStretch();

    const auto pushEdit = [this] { push(); };
    connect(name_, &QLineEdit::editingFinished, this, pushEdit);
    connect(scopingMethod_, &QComboBox::currentIndexChanged, this, pushEdit);
    connect(scope_, &QComboBox::currentIndexChanged, this, pushEdit);
    connect(namedSelection_, &QComboBox::currentIndexChanged, this, pushEdit);
    connect(fixX_, &QCheckBox::toggled, this, pushEdit);
    connect(fixY_, &QCheckBox::toggled, this, pushEdit);
    connect(fixZ_, &QCheckBox::toggled, this, pushEdit);
    connect(fx_, &QDoubleSpinBox::valueChanged, this, pushEdit);
    connect(fy_, &QDoubleSpinBox::valueChanged, this, pushEdit);
    connect(fz_, &QDoubleSpinBox::valueChanged, this, pushEdit);
    connect(showNamedSelection_, &QToolButton::clicked, this, [this] {
        const ObjectId id = selectedNamedSelectionId();
        if (id == InvalidObjectId || services_.namedSelections == nullptr
            || services_.namedSelections->byId(id) == nullptr) {
            return;
        }
        // Persistent reference navigation document mutation değildir. Aynı
        // canonical MainWindow::selectObject yolu Navigator, Details ve
        // Named Selection persistent overlay context'ini birlikte çözer.
        if (auto *mainWindow = qobject_cast<Dynamics26MainWindow *>(window())) {
            mainWindow->selectObject(id);
        }
    });
}

ObjectId BoundaryConditionDetails::selectedNamedSelectionId() const
{
    if (namedSelection_ == nullptr || namedSelection_->currentIndex() < 0) {
        return InvalidObjectId;
    }
    bool ok = false;
    const qulonglong value = namedSelection_->currentData().toString().toULongLong(&ok, 10);
    return ok ? static_cast<ObjectId>(value) : InvalidObjectId;
}

void BoundaryConditionDetails::populateNamedSelections(const ObjectId currentId)
{
    namedSelection_->clear();
    namedSelection_->addItem(tr("— Seçin —"), QString());

    bool currentFound = currentId == InvalidObjectId;
    if (services_.namedSelections != nullptr) {
        for (const ObjectId id : services_.namedSelections->order()) {
            const NamedSelectionDefinition *definition = services_.namedSelections->byId(id);
            if (definition == nullptr) {
                continue;
            }
            const bool compatible = isGeometryFaceScope(*definition);
            if (!compatible && id != currentId) {
                continue;
            }

            QString label = definition->name;
            if (!compatible) {
                label += tr(" — Uyumsuz kapsam");
            } else if (services_.namedSelections->validate(id) != ScopeReferenceValidationError::None) {
                label += tr(" — Out of Date");
            }
            namedSelection_->addItem(label, QString::number(static_cast<qulonglong>(id)));
            if (id == currentId) {
                namedSelection_->setCurrentIndex(namedSelection_->count() - 1);
                currentFound = true;
            }
        }
    }

    // Dosyada artık bulunmayan bir persistent ObjectId sessizce placeholder'a
    // dönüştürülmez. Kullanıcı dangling referansı Details'ta açıkça görür.
    if (!currentFound && currentId != InvalidObjectId) {
        namedSelection_->addItem(
            tr("Eksik Named Selection — ID %1").arg(static_cast<qulonglong>(currentId)),
            QString::number(static_cast<qulonglong>(currentId)));
        namedSelection_->setCurrentIndex(namedSelection_->count() - 1);
    }
}

void BoundaryConditionDetails::push()
{
    if (updating_) {
        return;
    }

    const int index = qBound(0, scope_->currentIndex(), 5);
    const BoxFace face = kFaces[static_cast<std::size_t>(index)];
    const QVariant methodData = scopingMethod_->currentData();
    const auto method = methodData.isValid()
        ? static_cast<BoundaryScopingMethod>(methodData.toInt())
        : BoundaryScopingMethod::GeometrySelection;
    const ObjectId namedSelectionId = method == BoundaryScopingMethod::NamedSelection
        ? selectedNamedSelectionId() : InvalidObjectId;

    if (isLoad_) {
        const LoadDefinition *existing = services_.analysis->load(objectId_);
        if (existing == nullptr) {
            return;
        }
        LoadDefinition definition = *existing;
        definition.name = name_->text().trimmed().isEmpty() ? existing->name : name_->text().trimmed();
        definition.scopingMethod = method;
        definition.scope = face;
        definition.namedSelectionId = namedSelectionId;
        definition.fxN = fx_->value();
        definition.fyN = fy_->value();
        definition.fzN = fz_->value();
        if (definition.name == existing->name
            && definition.scopingMethod == existing->scopingMethod
            && definition.scope == existing->scope
            && definition.namedSelectionId == existing->namedSelectionId
            && qFuzzyCompare(definition.fxN, existing->fxN)
            && qFuzzyCompare(definition.fyN, existing->fyN)
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
        definition.scopingMethod = method;
        definition.scope = face;
        definition.namedSelectionId = namedSelectionId;
        definition.fixX = fixX_->isChecked();
        definition.fixY = fixY_->isChecked();
        definition.fixZ = fixZ_->isChecked();
        if (definition.name == existing->name
            && definition.scopingMethod == existing->scopingMethod
            && definition.scope == existing->scope
            && definition.namedSelectionId == existing->namedSelectionId
            && definition.fixX == existing->fixX && definition.fixY == existing->fixY
            && definition.fixZ == existing->fixZ) {
            return;
        }
        services_.commands->push(new commands::SetSupportCommand(services_, objectId_, *existing, definition));
    }

    const SupportDefinition *support = services_.analysis->support(objectId_);
    const LoadDefinition *load = services_.analysis->load(objectId_);
    const BoundaryScopeResolution resolved = load != nullptr
        ? services_.analysis->resolveBoundaryScope(*load)
        : (support != nullptr ? services_.analysis->resolveBoundaryScope(*support) : BoundaryScopeResolution{});
    emit scopeHighlightRequested(singleResolvedGeometryId(resolved));
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

    const BoundaryScopingMethod method = isLoad_ ? load->scopingMethod : support->scopingMethod;
    const ObjectId namedSelectionId = isLoad_ ? load->namedSelectionId : support->namedSelectionId;
    const BoxFace face = isLoad_ ? load->scope : support->scope;

    const int methodIndex = scopingMethod_->findData(static_cast<int>(method));
    scopingMethod_->setCurrentIndex(methodIndex >= 0 ? methodIndex : 0);
    const auto position = std::find(kFaces.begin(), kFaces.end(), face);
    scope_->setCurrentIndex(position != kFaces.end()
                                ? static_cast<int>(std::distance(kFaces.begin(), position)) : 0);
    populateNamedSelections(namedSelectionId);
    geometryScopeRow_->setVisible(method == BoundaryScopingMethod::GeometrySelection);
    namedSelectionRow_->setVisible(method == BoundaryScopingMethod::NamedSelection);
    const bool namedSelectionExists = method == BoundaryScopingMethod::NamedSelection
        && namedSelectionId != InvalidObjectId
        && services_.namedSelections != nullptr
        && services_.namedSelections->byId(namedSelectionId) != nullptr;
    showNamedSelection_->setEnabled(namedSelectionExists);
    showNamedSelection_->setToolTip(namedSelectionExists
        ? tr("Referans verilen Named Selection nesnesini model ağacında göster")
        : tr("Referans verilen Named Selection bulunamadı"));
    name_->setText(isLoad_ ? load->name : support->name);

    const BoundaryScopeResolution resolved = isLoad_
        ? services_.analysis->resolveBoundaryScope(*load)
        : services_.analysis->resolveBoundaryScope(*support);
    if (!resolved.valid) {
        scopeStatistics_->setText(resolved.error.isEmpty() ? tr("Scope çözülemedi") : resolved.error);
    } else if (services_.mesh->hasMesh() && !services_.mesh->isOutOfDate()) {
        const int nodeCount = isLoad_
            ? services_.analysis->resolvedBoundaryNodeCount(*load)
            : services_.analysis->resolvedBoundaryNodeCount(*support);
        scopeStatistics_->setText(tr("%1 face · %2 node")
                                      .arg(static_cast<qlonglong>(resolved.geometryFaceIds.size()))
                                      .arg(nodeCount));
    } else {
        scopeStatistics_->setText(tr("%1 face · Mesh üretilmedi/güncel değil")
                                      .arg(static_cast<qlonglong>(resolved.geometryFaceIds.size())));
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

    emit scopeHighlightRequested(singleResolvedGeometryId(resolved));
}

} // namespace d26
